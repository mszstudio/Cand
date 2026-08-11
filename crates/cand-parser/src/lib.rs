//! C& Parser — Recursive descent parser converting Token stream into AST.

use cand_ast::*;
use cand_lexer::{Token, TokenKind};

#[derive(Debug, thiserror::Error)]
pub enum ParseError {
    #[error("{span:?}: expected {expected}, found {found}")]
    Expected {
        expected: String,
        found: String,
        span: Span,
    },
    #[error("{span:?}: unexpected end of file")]
    UnexpectedEof { span: Span },
    #[error("{span:?}: custom error: {message}")]
    Custom { message: String, span: Span },
}

pub struct Parser {
    tokens: Vec<Token>,
    pos: usize,
    file: String,
}

impl Parser {
    pub fn new(tokens: Vec<Token>, file: impl Into<String>) -> Self {
        Parser {
            tokens,
            pos: 0,
            file: file.into(),
        }
    }

    fn peek(&self) -> &Token {
        self.tokens.get(self.pos).unwrap_or_else(|| {
            self.tokens.last().expect("tokens list cannot be empty")
        })
    }

    fn peek_kind(&self) -> &TokenKind {
        &self.peek().kind
    }

    fn peek_ahead(&self, offset: usize) -> &TokenKind {
        &self.tokens.get(self.pos + offset).map(|t| &t.kind).unwrap_or(&TokenKind::Eof)
    }

    fn is_at_end(&self) -> bool {
        matches!(self.peek_kind(), TokenKind::Eof)
    }

    fn advance(&mut self) -> &Token {
        if !self.is_at_end() {
            self.pos += 1;
        }
        self.tokens.get(self.pos - 1).unwrap()
    }

    fn check(&self, kind: &TokenKind) -> bool {
        std::mem::discriminant(self.peek_kind()) == std::mem::discriminant(kind)
    }

    fn match_token(&mut self, kind: &TokenKind) -> bool {
        if self.check(kind) {
            self.advance();
            true
        } else {
            false
        }
    }

    fn expect(&mut self, kind: TokenKind, expected_name: &str) -> Result<Token, ParseError> {
        if self.check(&kind) {
            Ok(self.advance().clone())
        } else {
            let tok = self.peek();
            Err(ParseError::Expected {
                expected: expected_name.to_string(),
                found: tok.raw.clone(),
                span: tok.span.clone(),
            })
        }
    }

    fn expect_ident(&mut self) -> Result<(String, Span), ParseError> {
        let tok = self.peek().clone();
        if let TokenKind::Ident(ref name) = tok.kind {
            self.advance();
            Ok((name.clone(), tok.span))
        } else {
            Err(ParseError::Expected {
                expected: "identifier".to_string(),
                found: tok.raw.clone(),
                span: tok.span,
            })
        }
    }

    // ── Parse Entry Point ──────────────────────────────────────────────────

    pub fn parse_program(&mut self) -> Result<Program, ParseError> {
        let mut program = Program::new(&self.file);

        while !self.is_at_end() {
            let item = self.parse_item()?;
            program.items.push(item);
        }

        Ok(program)
    }

    // ── Items ──────────────────────────────────────────────────────────────

    pub fn parse_item(&mut self) -> Result<Item, ParseError> {
        let start_span = self.peek().span.clone();
        let mut is_pub = false;
        let mut is_export = false;

        if self.match_token(&TokenKind::Pub) {
            is_pub = true;
        }
        if self.match_token(&TokenKind::Export) {
            is_export = true;
        }

        let vis = if is_export {
            Visibility::Export
        } else if is_pub {
            Visibility::Public
        } else {
            Visibility::Private
        };

        match self.peek_kind() {
            TokenKind::Fn | TokenKind::Async => {
                let is_async = self.match_token(&TokenKind::Async);
                self.expect(TokenKind::Fn, "fn")?;
                let fn_def = self.parse_fn_def(vis, is_async, false)?;
                let span = fn_def.body.as_ref().map(|b| b.span.clone()).unwrap_or(start_span);
                Ok(Item {
                    kind: ItemKind::Fn(fn_def),
                    attrs: vec![],
                    span,
                })
            }
            TokenKind::Struct => {
                self.advance();
                let (name, _) = self.expect_ident()?;
                self.expect(TokenKind::LBrace, "{")?;
                let mut fields = vec![];
                while !self.check(&TokenKind::RBrace) && !self.is_at_end() {
                    let field_vis = if self.match_token(&TokenKind::Pub) {
                        Visibility::Public
                    } else {
                        Visibility::Private
                    };
                    let (fname, _) = self.expect_ident()?;
                    self.expect(TokenKind::Colon, ":")?;
                    let fty = self.parse_type()?;
                    if !self.match_token(&TokenKind::Comma) {
                        self.match_token(&TokenKind::Semicolon);
                    }
                    fields.push(FieldDef {
                        name: fname,
                        ty: fty,
                        visibility: field_vis,
                    });
                }
                let end_tok = self.expect(TokenKind::RBrace, "}")?;
                Ok(Item {
                    kind: ItemKind::Struct(StructDef {
                        name,
                        generics: vec![],
                        fields,
                        visibility: vis,
                    }),
                    attrs: vec![],
                    span: end_tok.span,
                })
            }
            TokenKind::Enum => {
                self.advance();
                let (name, _) = self.expect_ident()?;
                self.expect(TokenKind::LBrace, "{")?;
                let mut variants = vec![];
                while !self.check(&TokenKind::RBrace) && !self.is_at_end() {
                    let (vname, _) = self.expect_ident()?;
                    self.match_token(&TokenKind::Comma);
                    variants.push(EnumVariant {
                        name: vname,
                        kind: VariantKind::Unit,
                    });
                }
                let end_tok = self.expect(TokenKind::RBrace, "}")?;
                Ok(Item {
                    kind: ItemKind::Enum(EnumDef {
                        name,
                        generics: vec![],
                        variants,
                        visibility: vis,
                    }),
                    attrs: vec![],
                    span: end_tok.span,
                })
            }
            TokenKind::Import => {
                self.advance();
                let mut segments = vec![];
                let (first, _) = self.expect_ident()?;
                segments.push(first);
                while self.match_token(&TokenKind::Dot) {
                    let (seg, _) = self.expect_ident()?;
                    segments.push(seg);
                }
                self.expect(TokenKind::Semicolon, ";")?;
                Ok(Item {
                    kind: ItemKind::Import(ImportPath {
                        segments,
                        alias: None,
                        glob: false,
                    }),
                    attrs: vec![],
                    span: start_span,
                })
            }
            TokenKind::Extern => {
                self.advance();
                let abi = if let TokenKind::StringLit(s) = self.peek_kind().clone() {
                    self.advance();
                    s
                } else {
                    "C".to_string()
                };
                self.expect(TokenKind::LBrace, "{")?;
                let mut items = vec![];
                while !self.check(&TokenKind::RBrace) && !self.is_at_end() {
                    self.expect(TokenKind::Fn, "fn")?;
                    let fn_def = self.parse_fn_def(Visibility::Public, false, true)?;
                    items.push(fn_def);
                }
                let end_tok = self.expect(TokenKind::RBrace, "}")?;
                Ok(Item {
                    kind: ItemKind::Extern(ExternBlock { abi, items }),
                    attrs: vec![],
                    span: end_tok.span,
                })
            }
            TokenKind::Test => {
                self.advance();
                let test_name = if let TokenKind::StringLit(s) = self.peek_kind().clone() {
                    self.advance();
                    s
                } else {
                    "unnamed".to_string()
                };
                let body = self.parse_block()?;
                let span = body.span.clone();
                Ok(Item {
                    kind: ItemKind::Test(test_name, body),
                    attrs: vec![],
                    span,
                })
            }
            _ => {
                let tok = self.peek();
                Err(ParseError::Expected {
                    expected: "fn, struct, enum, import, extern, test".to_string(),
                    found: tok.raw.clone(),
                    span: tok.span.clone(),
                })
            }
        }
    }

    fn parse_fn_def(
        &mut self,
        vis: Visibility,
        is_async: bool,
        is_extern: bool,
    ) -> Result<FnDef, ParseError> {
        let (name, _) = self.expect_ident()?;
        self.expect(TokenKind::LParen, "(")?;

        let mut params = vec![];
        while !self.check(&TokenKind::RParen) && !self.is_at_end() {
            let is_mut = self.match_token(&TokenKind::Mut);
            let (pname, _) = self.expect_ident()?;
            self.expect(TokenKind::Colon, ":")?;
            let pty = self.parse_type()?;
            params.push(Param {
                name: pname,
                ty: pty,
                mutable: is_mut,
                is_self: false,
            });
            if !self.match_token(&TokenKind::Comma) {
                break;
            }
        }
        self.expect(TokenKind::RParen, ")")?;

        let ret = if self.match_token(&TokenKind::Arrow) {
            self.parse_type()?
        } else {
            Type::Primitive(PrimType::Void)
        };

        let body = if is_extern || self.match_token(&TokenKind::Semicolon) {
            None
        } else {
            Some(self.parse_block()?)
        };

        Ok(FnDef {
            name,
            generics: vec![],
            params,
            ret,
            body,
            is_async,
            is_extern,
            visibility: vis,
        })
    }

    // ── Types ──────────────────────────────────────────────────────────────

    pub fn parse_type(&mut self) -> Result<Type, ParseError> {
        let tok = self.peek().clone();
        match tok.kind {
            TokenKind::TyInt => { self.advance(); Ok(Type::Primitive(PrimType::Int)) }
            TokenKind::TyI8 => { self.advance(); Ok(Type::Primitive(PrimType::I8)) }
            TokenKind::TyI16 => { self.advance(); Ok(Type::Primitive(PrimType::I16)) }
            TokenKind::TyI32 => { self.advance(); Ok(Type::Primitive(PrimType::I32)) }
            TokenKind::TyI64 => { self.advance(); Ok(Type::Primitive(PrimType::I64)) }
            TokenKind::TyU8 => { self.advance(); Ok(Type::Primitive(PrimType::U8)) }
            TokenKind::TyU16 => { self.advance(); Ok(Type::Primitive(PrimType::U16)) }
            TokenKind::TyU32 => { self.advance(); Ok(Type::Primitive(PrimType::U32)) }
            TokenKind::TyU64 => { self.advance(); Ok(Type::Primitive(PrimType::U64)) }
            TokenKind::TyF32 => { self.advance(); Ok(Type::Primitive(PrimType::F32)) }
            TokenKind::TyF64 => { self.advance(); Ok(Type::Primitive(PrimType::F64)) }
            TokenKind::TyFloat => { self.advance(); Ok(Type::Primitive(PrimType::Float)) }
            TokenKind::TyBool => { self.advance(); Ok(Type::Primitive(PrimType::Bool)) }
            TokenKind::TyChar => { self.advance(); Ok(Type::Primitive(PrimType::Char)) }
            TokenKind::TyString => { self.advance(); Ok(Type::Primitive(PrimType::String)) }
            TokenKind::TyVoid => { self.advance(); Ok(Type::Primitive(PrimType::Void)) }
            TokenKind::TyUsize => { self.advance(); Ok(Type::Primitive(PrimType::Usize)) }
            TokenKind::TyIsize => { self.advance(); Ok(Type::Primitive(PrimType::Isize)) }
            TokenKind::Star => {
                self.advance();
                let inner = self.parse_type()?;
                Ok(Type::Pointer(Box::new(inner)))
            }
            TokenKind::Ampersand => {
                self.advance();
                let inner = self.parse_type()?;
                Ok(Type::Ref(Box::new(inner)))
            }
            TokenKind::Ident(ref name) => {
                let name = name.clone();
                self.advance();
                Ok(Type::Named(name))
            }
            _ => Err(ParseError::Expected {
                expected: "type".to_string(),
                found: tok.raw,
                span: tok.span,
            }),
        }
    }

    // ── Block ──────────────────────────────────────────────────────────────

    pub fn parse_block(&mut self) -> Result<Block, ParseError> {
        let start_tok = self.expect(TokenKind::LBrace, "{")?;
        let mut stmts = vec![];

        while !self.check(&TokenKind::RBrace) && !self.is_at_end() {
            let stmt = self.parse_stmt()?;
            stmts.push(stmt);
        }

        let end_tok = self.expect(TokenKind::RBrace, "}")?;
        Ok(Block {
            stmts,
            expr: None,
            span: Span::new(&self.file, start_tok.span.start, end_tok.span.end, start_tok.span.line, start_tok.span.col),
        })
    }

    // ── Statement ──────────────────────────────────────────────────────────

    pub fn parse_stmt(&mut self) -> Result<Stmt, ParseError> {
        let tok = self.peek().clone();
        match tok.kind {
            TokenKind::Let => {
                self.advance();
                let is_mut = self.match_token(&TokenKind::Mut);
                let (name, _) = self.expect_ident()?;

                let ty = if self.match_token(&TokenKind::Colon) {
                    Some(self.parse_type()?)
                } else {
                    None
                };

                let value = if self.match_token(&TokenKind::Eq) {
                    Some(self.parse_expr()?)
                } else {
                    None
                };

                self.expect(TokenKind::Semicolon, ";")?;
                Ok(Stmt {
                    kind: StmtKind::Let {
                        name,
                        ty,
                        value,
                        mutable: is_mut,
                    },
                    span: tok.span,
                })
            }
            TokenKind::Const => {
                self.advance();
                let (name, _) = self.expect_ident()?;
                self.expect(TokenKind::Colon, ":")?;
                let ty = self.parse_type()?;
                self.expect(TokenKind::Eq, "=")?;
                let value = self.parse_expr()?;
                self.expect(TokenKind::Semicolon, ";")?;
                Ok(Stmt {
                    kind: StmtKind::Const {
                        name,
                        ty: Some(ty),
                        value,
                    },
                    span: tok.span,
                })
            }
            _ => {
                let expr = self.parse_expr()?;
                if self.match_token(&TokenKind::Semicolon) {
                    Ok(Stmt {
                        kind: StmtKind::Semi(expr),
                        span: tok.span,
                    })
                } else {
                    Ok(Stmt {
                        kind: StmtKind::Expr(expr),
                        span: tok.span,
                    })
                }
            }
        }
    }

    // ── Expression ─────────────────────────────────────────────────────────

    pub fn parse_expr(&mut self) -> Result<Expr, ParseError> {
        self.parse_assignment()
    }

    fn parse_assignment(&mut self) -> Result<Expr, ParseError> {
        let left = self.parse_binary(0)?;

        if self.match_token(&TokenKind::Eq) {
            let span = left.span.clone();
            let right = self.parse_assignment()?;
            Ok(Expr::new(ExprKind::Assign(Box::new(left), Box::new(right)), span))
        } else {
            Ok(left)
        }
    }

    fn parse_binary(&mut self, min_prec: u8) -> Result<Expr, ParseError> {
        let mut left = self.parse_unary()?;

        loop {
            let prec = match self.peek_kind() {
                TokenKind::Plus | TokenKind::Minus => 10,
                TokenKind::Star | TokenKind::Slash | TokenKind::Percent => 20,
                TokenKind::EqEq | TokenKind::BangEq | TokenKind::Lt | TokenKind::LtEq | TokenKind::Gt | TokenKind::GtEq => 5,
                _ => break,
            };

            if prec < min_prec {
                break;
            }

            let op_tok = self.advance().clone();
            let op = match op_tok.kind {
                TokenKind::Plus => BinOp::Add,
                TokenKind::Minus => BinOp::Sub,
                TokenKind::Star => BinOp::Mul,
                TokenKind::Slash => BinOp::Div,
                TokenKind::Percent => BinOp::Rem,
                TokenKind::EqEq => BinOp::Eq,
                TokenKind::BangEq => BinOp::Ne,
                TokenKind::Lt => BinOp::Lt,
                TokenKind::LtEq => BinOp::Le,
                TokenKind::Gt => BinOp::Gt,
                TokenKind::GtEq => BinOp::Ge,
                _ => unreachable!(),
            };

            let right = self.parse_binary(prec + 1)?;
            let span = left.span.clone();
            left = Expr::new(ExprKind::Binary(op, Box::new(left), Box::new(right)), span);
        }

        Ok(left)
    }

    fn parse_unary(&mut self) -> Result<Expr, ParseError> {
        let tok = self.peek().clone();
        match tok.kind {
            TokenKind::Minus => {
                self.advance();
                let operand = self.parse_unary()?;
                Ok(Expr::new(ExprKind::Unary(UnaryOp::Neg, Box::new(operand)), tok.span))
            }
            TokenKind::Bang => {
                self.advance();
                let operand = self.parse_unary()?;
                Ok(Expr::new(ExprKind::Unary(UnaryOp::Not, Box::new(operand)), tok.span))
            }
            _ => self.parse_primary(),
        }
    }

    fn parse_primary(&mut self) -> Result<Expr, ParseError> {
        let tok = self.advance().clone();

        let mut expr = match tok.kind {
            TokenKind::Int(val) => Expr::new(ExprKind::Int(val), tok.span),
            TokenKind::Float(val) => Expr::new(ExprKind::Float(val), tok.span),
            TokenKind::Bool(val) => Expr::new(ExprKind::Bool(val), tok.span),
            TokenKind::StringLit(ref val) => Expr::new(ExprKind::Str(val.clone()), tok.span),
            TokenKind::CharLit(val) => Expr::new(ExprKind::Char(val), tok.span),
            TokenKind::Ident(ref name) => Expr::new(ExprKind::Ident(name.clone()), tok.span),
            TokenKind::LParen => {
                let expr = self.parse_expr()?;
                self.expect(TokenKind::RParen, ")")?;
                expr
            }
            TokenKind::Return => {
                let val = if !self.check(&TokenKind::Semicolon) && !self.check(&TokenKind::RBrace) {
                    Some(Box::new(self.parse_expr()?))
                } else {
                    None
                };
                Expr::new(ExprKind::Return(val), tok.span)
            }
            _ => return Err(ParseError::Expected {
                expected: "expression".to_string(),
                found: tok.raw,
                span: tok.span,
            }),
        };

        // Postfix operators: function call `(args...)`, field access `.field`
        loop {
            if self.match_token(&TokenKind::LParen) {
                let mut args = vec![];
                while !self.check(&TokenKind::RParen) && !self.is_at_end() {
                    args.push(self.parse_expr()?);
                    if !self.match_token(&TokenKind::Comma) {
                        break;
                    }
                }
                self.expect(TokenKind::RParen, ")")?;
                let span = expr.span.clone();
                expr = Expr::new(ExprKind::Call {
                    callee: Box::new(expr),
                    args,
                }, span);
            } else if self.match_token(&TokenKind::Dot) {
                let (fname, fspan) = self.expect_ident()?;
                expr = Expr::new(ExprKind::Field(Box::new(expr), fname), fspan);
            } else {
                break;
            }
        }

        Ok(expr)
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use cand_lexer::Lexer;

    fn parse(src: &str) -> Program {
        let mut lexer = Lexer::new(src, "test.cand");
        let tokens = lexer.tokenize().unwrap();
        let mut parser = Parser::new(tokens, "test.cand");
        parser.parse_program().unwrap()
    }

    #[test]
    fn test_parse_hello_world() {
        let src = r#"fn main() { println("Hello, C&!"); }"#;
        let prog = parse(src);
        assert_eq!(prog.items.len(), 1);
        if let ItemKind::Fn(ref def) = prog.items[0].kind {
            assert_eq!(def.name, "main");
            assert!(def.body.is_some());
        } else {
            panic!("Expected Fn item");
        }
    }
}
