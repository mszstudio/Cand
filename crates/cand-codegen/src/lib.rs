//! C& Codegen — Generates official LLVM IR (.ll) from C& AST.

use cand_ast::*;
use std::collections::HashMap;
use std::fmt::Write;

#[derive(Debug, thiserror::Error)]
pub enum CodegenError {
    #[error("Codegen formatting error: {0}")]
    FmtError(#[from] std::fmt::Error),
    #[error("Undefined variable: {0}")]
    UndefinedVar(String),
    #[error("Unsupported feature: {0}")]
    Unsupported(String),
}

pub struct CodeGenerator {
    pub module_name: String,
    ir: String,
    global_strings: HashMap<String, String>, // string content -> global name
    string_counter: usize,
    var_map: HashMap<String, String>, // var name -> LLVM register/ptr
    reg_counter: usize,
}

impl CodeGenerator {
    pub fn new(module_name: impl Into<String>) -> Self {
        CodeGenerator {
            module_name: module_name.into(),
            ir: String::new(),
            global_strings: HashMap::new(),
            string_counter: 0,
            var_map: HashMap::new(),
            reg_counter: 0,
        }
    }

    fn next_reg(&mut self) -> String {
        let r = format!("%{}", self.reg_counter);
        self.reg_counter += 1;
        r
    }

    fn get_or_create_str_global(&mut self, text: &str) -> (String, usize) {
        if let Some(name) = self.global_strings.get(text) {
            let len = text.as_bytes().len() + 1;
            return (name.clone(), len);
        }

        let name = format!("@.str.{}", self.string_counter);
        self.string_counter += 1;
        self.global_strings.insert(text.to_string(), name.clone());
        let len = text.as_bytes().len() + 1;
        (name, len)
    }

    pub fn compile_program(&mut self, program: &Program) -> Result<String, CodegenError> {
        let mut body_ir = String::new();

        for item in &program.items {
            self.compile_item(item, &mut body_ir)?;
        }

        // Header
        let mut header = String::new();
        writeln!(header, "; ModuleID = '{}'", self.module_name)?;
        writeln!(header, "source_filename = \"{}\"", program.file)?;
        writeln!(header, "target datalayout = \"e-m:w-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128\"")?;
        writeln!(header)?;

        // Global string constants
        for (text, name) in &self.global_strings {
            let escaped = escape_llvm_string(text);
            let len = text.as_bytes().len() + 1; // including null terminator
            writeln!(header, "{} = private unnamed_addr constant [{} x i8] c\"{}\\00\", align 1", name, len, escaped)?;
        }

        if !self.global_strings.is_empty() {
            writeln!(header)?;
        }

        // Declare C stdlib printf / puts
        writeln!(header, "declare i32 @printf(ptr noundef, ...)")?;
        writeln!(header, "declare i32 @puts(ptr noundef)")?;
        writeln!(header)?;

        let mut final_ir = String::new();
        final_ir.push_str(&header);
        final_ir.push_str(&body_ir);

        self.ir = final_ir.clone();
        Ok(final_ir)
    }

    fn compile_item(&mut self, item: &Item, out: &mut String) -> Result<(), CodegenError> {
        match &item.kind {
            ItemKind::Fn(fn_def) => self.compile_fn(fn_def, out),
            _ => Ok(()),
        }
    }

    fn compile_fn(&mut self, fn_def: &FnDef, out: &mut String) -> Result<(), CodegenError> {
        self.var_map.clear();
        self.reg_counter = 1;

        let ret_ty = if fn_def.name == "main" { "i32" } else { "i32" };

        writeln!(out, "define {} @{}() {{", ret_ty, fn_def.name)?;
        writeln!(out, "entry:")?;

        if let Some(ref body) = fn_def.body {
            self.compile_block(body, out)?;
        }

        if fn_def.name == "main" {
            writeln!(out, "  ret i32 0")?;
        } else {
            writeln!(out, "  ret i32 0")?;
        }

        writeln!(out, "}}")?;
        writeln!(out)?;

        Ok(())
    }

    fn compile_block(&mut self, block: &Block, out: &mut String) -> Result<(), CodegenError> {
        for stmt in &block.stmts {
            self.compile_stmt(stmt, out)?;
        }
        Ok(())
    }

    fn compile_stmt(&mut self, stmt: &Stmt, out: &mut String) -> Result<(), CodegenError> {
        match &stmt.kind {
            StmtKind::Let { name, value, .. } => {
                let ptr_reg = format!("%var_{}", name);
                writeln!(out, "  {} = alloca i32, align 4", ptr_reg)?;
                if let Some(ref val_expr) = value {
                    let (val_reg, _ty) = self.compile_expr(val_expr, out)?;
                    writeln!(out, "  store i32 {}, ptr {}, align 4", val_reg, ptr_reg)?;
                }
                self.var_map.insert(name.clone(), ptr_reg);
                Ok(())
            }
            StmtKind::Expr(expr) | StmtKind::Semi(expr) => {
                self.compile_expr(expr, out)?;
                Ok(())
            }
            _ => Ok(()),
        }
    }

    fn compile_expr(&mut self, expr: &Expr, out: &mut String) -> Result<(String, String), CodegenError> {
        match &expr.kind {
            ExprKind::Int(val) => Ok((format!("{}", val), "i32".to_string())),
            ExprKind::Str(val) => {
                let (str_name, _len) = self.get_or_create_str_global(val);
                Ok((str_name, "ptr".to_string()))
            }
            ExprKind::Ident(name) => {
                if let Some(ptr_reg) = self.var_map.get(name).cloned() {
                    let reg = self.next_reg();
                    writeln!(out, "  {} = load i32, ptr {}, align 4", reg, ptr_reg)?;
                    Ok((reg, "i32".to_string()))
                } else {
                    Err(CodegenError::UndefinedVar(name.clone()))
                }
            }
            ExprKind::Call { callee, args } => {
                if let ExprKind::Ident(ref name) = callee.kind {
                    if name == "println" || name == "print" {
                        return self.compile_println(args, name == "println", out);
                    }
                }
                Err(CodegenError::Unsupported("custom function call".to_string()))
            }
            ExprKind::Binary(op, left, right) => {
                let (lhs, _) = self.compile_expr(left, out)?;
                let (rhs, _) = self.compile_expr(right, out)?;
                let res_reg = self.next_reg();
                let op_str = match op {
                    BinOp::Add => "add nsw i32",
                    BinOp::Sub => "sub nsw i32",
                    BinOp::Mul => "mul nsw i32",
                    BinOp::Div => "sdiv i32",
                    _ => "add i32",
                };
                writeln!(out, "  {} = {} {}, {}", res_reg, op_str, lhs, rhs)?;
                Ok((res_reg, "i32".to_string()))
            }
            _ => Err(CodegenError::Unsupported("expression kind".to_string())),
        }
    }

    fn compile_println(
        &mut self,
        args: &[Expr],
        newline: bool,
        out: &mut String,
    ) -> Result<(String, String), CodegenError> {
        if let Some(arg) = args.first() {
            let (arg_val, arg_ty) = self.compile_expr(arg, out)?;
            let call_reg = self.next_reg();

            if arg_ty == "ptr" || arg_val.starts_with("@.str") {
                if newline {
                    // Use puts for simple string literals or printf with \n
                    let fmt_str = if newline { "%s\n" } else { "%s" };
                    let (fmt_glob, _) = self.get_or_create_str_global(fmt_str);
                    writeln!(out, "  {} = call i32 (ptr, ...) @printf(ptr noundef {}, ptr noundef {})", call_reg, fmt_glob, arg_val)?;
                } else {
                    let fmt_str = "%s";
                    let (fmt_glob, _) = self.get_or_create_str_global(fmt_str);
                    writeln!(out, "  {} = call i32 (ptr, ...) @printf(ptr noundef {}, ptr noundef {})", call_reg, fmt_glob, arg_val)?;
                }
            } else {
                let fmt_str = if newline { "%d\n" } else { "%d" };
                let (fmt_glob, _) = self.get_or_create_str_global(fmt_str);
                writeln!(out, "  {} = call i32 (ptr, ...) @printf(ptr noundef {}, i32 noundef {})", call_reg, fmt_glob, arg_val)?;
            }
            Ok((call_reg, "i32".to_string()))
        } else {
            let fmt_str = if newline { "\n" } else { "" };
            let (fmt_glob, _) = self.get_or_create_str_global(fmt_str);
            let call_reg = self.next_reg();
            writeln!(out, "  {} = call i32 (ptr, ...) @printf(ptr noundef {})", call_reg, fmt_glob)?;
            Ok((call_reg, "i32".to_string()))
        }
    }
}

fn escape_llvm_string(s: &str) -> String {
    let mut res = String::new();
    for b in s.as_bytes() {
        match b {
            b'\\' => res.push_str("\\5C"),
            b'"' => res.push_str("\\22"),
            b'\n' => res.push_str("\\0A"),
            b'\r' => res.push_str("\\0D"),
            b'\t' => res.push_str("\\09"),
            32..=126 => res.push(*b as char),
            _ => res.push_str(&format!("\\{:02X}", b)),
        }
    }
    res
}
