//! C& Abstract Syntax Tree (AST) node definitions.
//!
//! Every construct in the C& language is represented as a node here.
//! The parser produces these nodes; the codegen consumes them.

// Re-export Span from lexer
pub use cand_lexer::Span;

// ─── Types ────────────────────────────────────────────────────────────────────

/// A C& type expression.
#[derive(Debug, Clone, PartialEq)]
pub enum Type {
    /// Primitive: int, float, bool, char, string, void, u8, i64, etc.
    Primitive(PrimType),
    /// Named type (struct, enum, type alias): `User`, `Point`
    Named(String),
    /// Pointer: `*T`
    Pointer(Box<Type>),
    /// Mutable pointer: `*mut T`
    PtrMut(Box<Type>),
    /// Reference (borrow): `&T`
    Ref(Box<Type>),
    /// Mutable reference: `&mut T`
    RefMut(Box<Type>),
    /// Array: `[T; N]`
    Array(Box<Type>, usize),
    /// Slice: `[T]`
    Slice(Box<Type>),
    /// Optional: `T?`
    Option(Box<Type>),
    /// Result: `Result<T, E>`
    Result(Box<Type>, Box<Type>),
    /// Owned smart pointer: `owned<T>`
    Owned(Box<Type>),
    /// Shared smart pointer: `shared<T>`
    Shared(Box<Type>),
    /// Generic instantiation: `Vec<T>`
    Generic(String, Vec<Type>),
    /// Function type: `fn(A, B) -> R`
    Fn(Vec<Type>, Box<Type>),
    /// Inferred (written as `_` or omitted)
    Inferred,
}

#[derive(Debug, Clone, PartialEq)]
pub enum PrimType {
    Int, I8, I16, I32, I64,
    U8, U16, U32, U64,
    F32, F64, Float,
    Bool, Char, String, Void,
    Usize, Isize,
}

// ─── Expressions ─────────────────────────────────────────────────────────────

/// An expression node.
#[derive(Debug, Clone)]
pub struct Expr {
    pub kind: ExprKind,
    pub span: Span,
}

impl Expr {
    pub fn new(kind: ExprKind, span: Span) -> Self {
        Expr { kind, span }
    }
}

#[derive(Debug, Clone)]
pub enum ExprKind {
    // Literals
    Int(i64),
    Float(f64),
    Bool(bool),
    Str(String),
    Char(char),
    Null,

    // Identifier (variable reference)
    Ident(String),

    // Self keyword
    SelfExpr,

    // Binary operation: a + b
    Binary(BinOp, Box<Expr>, Box<Expr>),

    // Unary operation: -x, !x, *x, &x
    Unary(UnaryOp, Box<Expr>),

    // Function call: foo(a, b)
    Call {
        callee: Box<Expr>,
        args: Vec<Expr>,
    },

    // Method call: obj.method(args)
    MethodCall {
        object: Box<Expr>,
        method: String,
        args: Vec<Expr>,
    },

    // Field access: obj.field
    Field(Box<Expr>, String),

    // Index: arr[i]
    Index(Box<Expr>, Box<Expr>),

    // Assignment: x = expr
    Assign(Box<Expr>, Box<Expr>),

    // Compound assign: x += expr
    CompoundAssign(BinOp, Box<Expr>, Box<Expr>),

    // Block expression: { stmts... expr? }
    Block(Block),

    // If expression: if cond { ... } else { ... }
    If {
        cond: Box<Expr>,
        then: Block,
        else_: Option<Box<Expr>>,
    },

    // While loop: while cond { ... }
    While {
        cond: Box<Expr>,
        body: Block,
    },

    // Infinite loop: loop { ... }
    Loop(Block),

    // For-in loop: for x in iter { ... }
    For {
        var: String,
        iter: Box<Expr>,
        body: Block,
    },

    // Match expression
    Match {
        expr: Box<Expr>,
        arms: Vec<MatchArm>,
    },

    // Return expression
    Return(Option<Box<Expr>>),

    // Break / Continue
    Break(Option<Box<Expr>>),
    Continue,

    // Cast: expr as Type
    Cast(Box<Expr>, Type),

    // Struct literal: User { id: 1, name: "Bob" }
    StructLit {
        name: String,
        fields: Vec<(String, Expr)>,
    },

    // Array literal: [1, 2, 3]
    ArrayLit(Vec<Expr>),

    // Tuple: (a, b, c) - future use
    Tuple(Vec<Expr>),

    // Range: a..b or a..=b
    Range(Box<Expr>, Box<Expr>, bool /* inclusive */),

    // Await: expr.await or await expr
    Await(Box<Expr>),

    // Spawn block
    Spawn(Block),

    // Unsafe block
    Unsafe(Block),

    // Comptime block
    Comptime(Block),

    // Error propagation: expr?
    Try(Box<Expr>),

    // Path: std::io or Mod::SubMod
    Path(Vec<String>),
}

#[derive(Debug, Clone, PartialEq)]
pub enum BinOp {
    Add, Sub, Mul, Div, Rem,
    And, Or,
    BitAnd, BitOr, BitXor,
    Shl, Shr,
    Eq, Ne, Lt, Le, Gt, Ge,
}

#[derive(Debug, Clone, PartialEq)]
pub enum UnaryOp {
    Neg,    // -x
    Not,    // !x
    Deref,  // *x
    Ref,    // &x
    RefMut, // &mut x
}

// ─── Statements ───────────────────────────────────────────────────────────────

#[derive(Debug, Clone)]
pub struct Stmt {
    pub kind: StmtKind,
    pub span: Span,
}

#[derive(Debug, Clone)]
pub enum StmtKind {
    /// let x: T = expr;
    Let {
        name: String,
        ty: Option<Type>,
        value: Option<Expr>,
        mutable: bool,
    },
    /// const X: T = expr;
    Const {
        name: String,
        ty: Option<Type>,
        value: Expr,
    },
    /// A free-standing expression statement: foo(); or x + 1;
    Expr(Expr),
    /// A semicolon-terminated expression: foo();
    Semi(Expr),
}

// ─── Block ────────────────────────────────────────────────────────────────────

#[derive(Debug, Clone)]
pub struct Block {
    pub stmts: Vec<Stmt>,
    /// Optional trailing expression (the block's value)
    pub expr: Option<Box<Expr>>,
    pub span: Span,
}

// ─── Match ────────────────────────────────────────────────────────────────────

#[derive(Debug, Clone)]
pub struct MatchArm {
    pub pattern: Pattern,
    pub guard: Option<Expr>,
    pub body: Expr,
    pub span: Span,
}

#[derive(Debug, Clone)]
pub enum Pattern {
    /// _
    Wildcard,
    /// ident (binding)
    Ident(String),
    /// literal: 42, "hello", true
    Lit(LitPattern),
    /// enum variant: Ok(x), Err(e), None, Some(x)
    Enum(String, Vec<Pattern>),
    /// struct pattern: User { id, name }
    Struct(String, Vec<(String, Pattern)>),
    /// range: 1..=10
    Range(Box<Pattern>, Box<Pattern>, bool),
    /// OR pattern: pat1 | pat2
    Or(Vec<Pattern>),
    /// tuple: (a, b)
    Tuple(Vec<Pattern>),
}

#[derive(Debug, Clone)]
pub enum LitPattern {
    Int(i64),
    Float(f64),
    Bool(bool),
    Str(String),
    Char(char),
    Null,
}

// ─── Top-level Items ──────────────────────────────────────────────────────────

#[derive(Debug, Clone)]
pub struct Item {
    pub kind: ItemKind,
    pub attrs: Vec<Attribute>,
    pub span: Span,
}

#[derive(Debug, Clone)]
pub enum ItemKind {
    /// fn name<Generics>(params) -> RetTy { body }
    Fn(FnDef),
    /// struct Name { fields }
    Struct(StructDef),
    /// enum Name { variants }
    Enum(EnumDef),
    /// impl Type { methods }
    Impl(ImplBlock),
    /// trait Name { methods }
    Trait(TraitDef),
    /// import std.io;
    Import(ImportPath),
    /// export fn / export struct etc.
    Export(Box<Item>),
    /// extern "C" { ... }
    Extern(ExternBlock),
    /// const X: T = v;
    Const(ConstDef),
    /// type Alias = T;
    TypeAlias(String, Vec<GenericParam>, Type),
    /// test "name" { ... }
    Test(String, Block),
}

// ─── Function ─────────────────────────────────────────────────────────────────

#[derive(Debug, Clone)]
pub struct FnDef {
    pub name: String,
    pub generics: Vec<GenericParam>,
    pub params: Vec<Param>,
    pub ret: Type,
    pub body: Option<Block>,
    pub is_async: bool,
    pub is_extern: bool,
    pub visibility: Visibility,
}

#[derive(Debug, Clone)]
pub struct Param {
    pub name: String,
    pub ty: Type,
    pub mutable: bool,
    pub is_self: bool,
}

// ─── Struct ───────────────────────────────────────────────────────────────────

#[derive(Debug, Clone)]
pub struct StructDef {
    pub name: String,
    pub generics: Vec<GenericParam>,
    pub fields: Vec<FieldDef>,
    pub visibility: Visibility,
}

#[derive(Debug, Clone)]
pub struct FieldDef {
    pub name: String,
    pub ty: Type,
    pub visibility: Visibility,
}

// ─── Enum ─────────────────────────────────────────────────────────────────────

#[derive(Debug, Clone)]
pub struct EnumDef {
    pub name: String,
    pub generics: Vec<GenericParam>,
    pub variants: Vec<EnumVariant>,
    pub visibility: Visibility,
}

#[derive(Debug, Clone)]
pub struct EnumVariant {
    pub name: String,
    pub kind: VariantKind,
}

#[derive(Debug, Clone)]
pub enum VariantKind {
    Unit,
    Tuple(Vec<Type>),
    Struct(Vec<FieldDef>),
}

// ─── Impl ─────────────────────────────────────────────────────────────────────

#[derive(Debug, Clone)]
pub struct ImplBlock {
    pub target: Type,
    pub trait_: Option<String>,
    pub generics: Vec<GenericParam>,
    pub methods: Vec<FnDef>,
}

// ─── Trait ────────────────────────────────────────────────────────────────────

#[derive(Debug, Clone)]
pub struct TraitDef {
    pub name: String,
    pub generics: Vec<GenericParam>,
    pub methods: Vec<FnDef>,
    pub visibility: Visibility,
}

// ─── Extern ───────────────────────────────────────────────────────────────────

#[derive(Debug, Clone)]
pub struct ExternBlock {
    pub abi: String,
    pub items: Vec<FnDef>,
}

// ─── Const ────────────────────────────────────────────────────────────────────

#[derive(Debug, Clone)]
pub struct ConstDef {
    pub name: String,
    pub ty: Option<Type>,
    pub value: Expr,
    pub visibility: Visibility,
}

// ─── Import ───────────────────────────────────────────────────────────────────

#[derive(Debug, Clone)]
pub struct ImportPath {
    pub segments: Vec<String>,
    pub alias: Option<String>,
    pub glob: bool,
}

// ─── Generics ─────────────────────────────────────────────────────────────────

#[derive(Debug, Clone)]
pub struct GenericParam {
    pub name: String,
    pub bounds: Vec<String>,
}

// ─── Attributes ───────────────────────────────────────────────────────────────

#[derive(Debug, Clone)]
pub struct Attribute {
    pub name: String,
    pub args: Vec<String>,
}

// ─── Visibility ───────────────────────────────────────────────────────────────

#[derive(Debug, Clone, PartialEq)]
pub enum Visibility {
    Private,
    Public,
    Export,
}

// ─── Program ──────────────────────────────────────────────────────────────────

/// The top-level AST node representing one `.cand` source file.
#[derive(Debug, Clone)]
pub struct Program {
    pub file: String,
    pub items: Vec<Item>,
}

impl Program {
    pub fn new(file: impl Into<String>) -> Self {
        Program { file: file.into(), items: Vec::new() }
    }
}
