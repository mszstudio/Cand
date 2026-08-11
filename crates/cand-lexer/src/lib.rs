//! C& Lexer — Tokenizes .cand source files into a stream of tokens.
//!
//! Supports all v0.1 syntax plus full keyword set for future phases.

#[derive(Debug, Clone, PartialEq)]
pub struct Span {
    pub file: String,
    pub start: usize,
    pub end: usize,
    pub line: u32,
    pub col: u32,
}

impl Span {
    pub fn new(file: impl Into<String>, start: usize, end: usize, line: u32, col: u32) -> Self {
        Span { file: file.into(), start, end, line, col }
    }
    pub fn dummy() -> Self {
        Span { file: String::new(), start: 0, end: 0, line: 0, col: 0 }
    }
}

/// All token kinds in the C& language.
#[derive(Debug, Clone, PartialEq)]
pub enum TokenKind {
    // ── Literals ─────────────────────────────────────────────
    Int(i64),
    Float(f64),
    Bool(bool),
    StringLit(String),
    CharLit(char),

    // ── Identifiers ──────────────────────────────────────────
    Ident(String),

    // ── Keywords ─────────────────────────────────────────────
    Fn,
    Let,
    Const,
    Return,
    If,
    Else,
    While,
    For,
    Loop,
    Break,
    Continue,
    Struct,
    Enum,
    Impl,
    Trait,
    Import,
    Export,
    Pub,
    Unsafe,
    Async,
    Await,
    Spawn,
    Match,
    In,
    As,
    Extern,
    Comptime,
    Test,
    Mut,
    Ref,
    True,
    False,
    Null,
    Self_,
    Type,

    // ── Primitive types ───────────────────────────────────────
    TyInt,
    TyI8,
    TyI16,
    TyI32,
    TyI64,
    TyU8,
    TyU16,
    TyU32,
    TyU64,
    TyF32,
    TyF64,
    TyFloat,
    TyBool,
    TyChar,
    TyString,
    TyVoid,
    TyUsize,
    TyIsize,

    // ── Operators ─────────────────────────────────────────────
    Plus,       // +
    Minus,      // -
    Star,       // *
    Slash,      // /
    Percent,    // %
    Ampersand,  // &
    Pipe,       // |
    Caret,      // ^
    Tilde,      // ~
    Bang,       // !
    Question,   // ?
    Eq,         // =
    EqEq,       // ==
    BangEq,     // !=
    Lt,         // <
    LtEq,       // <=
    Gt,         // >
    GtEq,       // >=
    AmpAmp,     // &&
    PipePipe,   // ||
    Shl,        // <<
    Shr,        // >>
    Arrow,      // ->
    FatArrow,   // =>
    DotDot,     // ..
    DotDotEq,   // ..=
    Dot,        // .
    ColonColon, // ::
    Colon,      // :
    Semicolon,  // ;
    Comma,      // ,
    PlusEq,     // +=
    MinusEq,    // -=
    StarEq,     // *=
    SlashEq,    // /=

    // ── Delimiters ────────────────────────────────────────────
    LParen,   // (
    RParen,   // )
    LBrace,   // {
    RBrace,   // }
    LBracket, // [
    RBracket, // ]

    // ── Special ───────────────────────────────────────────────
    Hash,      // #
    At,        // @
    Underscore,// _

    // ── Meta ──────────────────────────────────────────────────
    Eof,
    Newline,
    Comment(String),
}

#[derive(Debug, Clone)]
pub struct Token {
    pub kind: TokenKind,
    pub span: Span,
    pub raw: String,
}

impl Token {
    pub fn new(kind: TokenKind, span: Span, raw: impl Into<String>) -> Self {
        Token { kind, span, raw: raw.into() }
    }
}

// ─── Lexer ───────────────────────────────────────────────────────────────────

#[derive(Debug, thiserror::Error)]
pub enum LexError {
    #[error("{file}:{line}:{col}: unexpected character '{ch}'")]
    UnexpectedChar { file: String, line: u32, col: u32, ch: char },
    #[error("{file}:{line}:{col}: unterminated string literal")]
    UnterminatedString { file: String, line: u32, col: u32 },
    #[error("{file}:{line}:{col}: unterminated character literal")]
    UnterminatedChar { file: String, line: u32, col: u32 },
    #[error("{file}:{line}:{col}: invalid escape sequence '\\{ch}'")]
    InvalidEscape { file: String, line: u32, col: u32, ch: char },
}

pub struct Lexer {
    source: Vec<char>,
    pos: usize,
    line: u32,
    col: u32,
    file: String,
}

impl Lexer {
    pub fn new(source: &str, file: impl Into<String>) -> Self {
        Lexer {
            source: source.chars().collect(),
            pos: 0,
            line: 1,
            col: 1,
            file: file.into(),
        }
    }

    pub fn tokenize(&mut self) -> Result<Vec<Token>, LexError> {
        let mut tokens = Vec::new();
        loop {
            self.skip_whitespace_and_comments(&mut tokens)?;
            if self.pos >= self.source.len() {
                tokens.push(Token::new(
                    TokenKind::Eof,
                    self.span(self.pos, self.pos),
                    "",
                ));
                break;
            }
            let tok = self.next_token()?;
            tokens.push(tok);
        }
        Ok(tokens)
    }

    fn peek(&self) -> Option<char> {
        self.source.get(self.pos).copied()
    }

    fn peek_at(&self, offset: usize) -> Option<char> {
        self.source.get(self.pos + offset).copied()
    }

    fn advance(&mut self) -> Option<char> {
        let ch = self.source.get(self.pos).copied()?;
        self.pos += 1;
        if ch == '\n' {
            self.line += 1;
            self.col = 1;
        } else {
            self.col += 1;
        }
        Some(ch)
    }

    fn span(&self, start: usize, end: usize) -> Span {
        Span::new(&self.file, start, end, self.line, self.col)
    }

    fn skip_whitespace_and_comments(&mut self, _tokens: &mut Vec<Token>) -> Result<(), LexError> {
        loop {
            match self.peek() {
                Some(' ') | Some('\t') | Some('\r') | Some('\n') => { self.advance(); }
                Some('/') if self.peek_at(1) == Some('/') => {
                    // Line comment
                    while self.peek().is_some() && self.peek() != Some('\n') {
                        self.advance();
                    }
                }
                Some('/') if self.peek_at(1) == Some('*') => {
                    // Block comment
                    self.advance(); self.advance(); // consume /*
                    loop {
                        match self.peek() {
                            None => break,
                            Some('*') if self.peek_at(1) == Some('/') => {
                                self.advance(); self.advance();
                                break;
                            }
                            _ => { self.advance(); }
                        }
                    }
                }
                _ => break,
            }
        }
        Ok(())
    }

    fn next_token(&mut self) -> Result<Token, LexError> {
        let start = self.pos;
        let start_line = self.line;
        let start_col = self.col;

        let ch = match self.advance() {
            Some(c) => c,
            None => return Ok(Token::new(TokenKind::Eof, self.span(start, start), "")),
        };

        let kind = match ch {
            // ── String literal ────────────────────────────────
            '"' => self.lex_string(start_line, start_col)?,

            // ── Char literal ──────────────────────────────────
            '\'' => self.lex_char(start_line, start_col)?,

            // ── Numbers ───────────────────────────────────────
            '0'..='9' => self.lex_number(ch)?,

            // ── Identifiers / Keywords ────────────────────────
            'a'..='z' | 'A'..='Z' | '_' => self.lex_ident_or_keyword(ch),

            // ── Two-char operators ────────────────────────────
            '+' => match self.peek() {
                Some('=') => { self.advance(); TokenKind::PlusEq }
                _ => TokenKind::Plus,
            },
            '-' => match self.peek() {
                Some('>') => { self.advance(); TokenKind::Arrow }
                Some('=') => { self.advance(); TokenKind::MinusEq }
                _ => TokenKind::Minus,
            },
            '*' => match self.peek() {
                Some('=') => { self.advance(); TokenKind::StarEq }
                _ => TokenKind::Star,
            },
            '/' => match self.peek() {
                Some('=') => { self.advance(); TokenKind::SlashEq }
                _ => TokenKind::Slash,
            },
            '%' => TokenKind::Percent,
            '&' => match self.peek() {
                Some('&') => { self.advance(); TokenKind::AmpAmp }
                _ => TokenKind::Ampersand,
            },
            '|' => match self.peek() {
                Some('|') => { self.advance(); TokenKind::PipePipe }
                _ => TokenKind::Pipe,
            },
            '^' => TokenKind::Caret,
            '~' => TokenKind::Tilde,
            '!' => match self.peek() {
                Some('=') => { self.advance(); TokenKind::BangEq }
                _ => TokenKind::Bang,
            },
            '?' => TokenKind::Question,
            '=' => match self.peek() {
                Some('=') => { self.advance(); TokenKind::EqEq }
                Some('>') => { self.advance(); TokenKind::FatArrow }
                _ => TokenKind::Eq,
            },
            '<' => match self.peek() {
                Some('=') => { self.advance(); TokenKind::LtEq }
                Some('<') => { self.advance(); TokenKind::Shl }
                _ => TokenKind::Lt,
            },
            '>' => match self.peek() {
                Some('=') => { self.advance(); TokenKind::GtEq }
                Some('>') => { self.advance(); TokenKind::Shr }
                _ => TokenKind::Gt,
            },
            '.' => match self.peek() {
                Some('.') => {
                    self.advance();
                    match self.peek() {
                        Some('=') => { self.advance(); TokenKind::DotDotEq }
                        _ => TokenKind::DotDot,
                    }
                }
                _ => TokenKind::Dot,
            },
            ':' => match self.peek() {
                Some(':') => { self.advance(); TokenKind::ColonColon }
                _ => TokenKind::Colon,
            },
            ';' => TokenKind::Semicolon,
            ',' => TokenKind::Comma,
            '(' => TokenKind::LParen,
            ')' => TokenKind::RParen,
            '{' => TokenKind::LBrace,
            '}' => TokenKind::RBrace,
            '[' => TokenKind::LBracket,
            ']' => TokenKind::RBracket,
            '#' => TokenKind::Hash,
            '@' => TokenKind::At,

            other => {
                return Err(LexError::UnexpectedChar {
                    file: self.file.clone(),
                    line: start_line,
                    col: start_col,
                    ch: other,
                });
            }
        };

        let end = self.pos;
        let raw: String = self.source[start..end].iter().collect();
        Ok(Token::new(kind, Span::new(&self.file, start, end, start_line, start_col), raw))
    }

    fn lex_string(&mut self, line: u32, col: u32) -> Result<TokenKind, LexError> {
        let mut s = String::new();
        loop {
            match self.advance() {
                None | Some('\n') => return Err(LexError::UnterminatedString {
                    file: self.file.clone(), line, col,
                }),
                Some('"') => break,
                Some('\\') => s.push(self.lex_escape(line, col)?),
                Some(c) => s.push(c),
            }
        }
        Ok(TokenKind::StringLit(s))
    }

    fn lex_char(&mut self, line: u32, col: u32) -> Result<TokenKind, LexError> {
        let ch = match self.advance() {
            None => return Err(LexError::UnterminatedChar { file: self.file.clone(), line, col }),
            Some('\\') => self.lex_escape(line, col)?,
            Some(c) => c,
        };
        match self.advance() {
            Some('\'') => Ok(TokenKind::CharLit(ch)),
            _ => Err(LexError::UnterminatedChar { file: self.file.clone(), line, col }),
        }
    }

    fn lex_escape(&mut self, line: u32, col: u32) -> Result<char, LexError> {
        match self.advance() {
            Some('n') => Ok('\n'),
            Some('t') => Ok('\t'),
            Some('r') => Ok('\r'),
            Some('\\') => Ok('\\'),
            Some('"') => Ok('"'),
            Some('\'') => Ok('\''),
            Some('0') => Ok('\0'),
            Some(c) => Err(LexError::InvalidEscape { file: self.file.clone(), line, col, ch: c }),
            None => Err(LexError::UnterminatedString { file: self.file.clone(), line, col }),
        }
    }

    fn lex_number(&mut self, first: char) -> Result<TokenKind, LexError> {
        let mut num = String::from(first);
        let mut is_float = false;

        // Hex: 0x...
        if first == '0' && matches!(self.peek(), Some('x') | Some('X')) {
            num.push(self.advance().unwrap());
            while matches!(self.peek(), Some('0'..='9') | Some('a'..='f') | Some('A'..='F') | Some('_')) {
                let c = self.advance().unwrap();
                if c != '_' { num.push(c); }
            }
            let val = i64::from_str_radix(&num[2..], 16).unwrap_or(0);
            return Ok(TokenKind::Int(val));
        }

        while matches!(self.peek(), Some('0'..='9') | Some('_')) {
            let c = self.advance().unwrap();
            if c != '_' { num.push(c); }
        }
        if self.peek() == Some('.') && !matches!(self.peek_at(1), Some('.')) {
            is_float = true;
            num.push(self.advance().unwrap());
            while matches!(self.peek(), Some('0'..='9') | Some('_')) {
                let c = self.advance().unwrap();
                if c != '_' { num.push(c); }
            }
        }
        // Optional exponent
        if matches!(self.peek(), Some('e') | Some('E')) {
            is_float = true;
            num.push(self.advance().unwrap());
            if matches!(self.peek(), Some('+') | Some('-')) {
                num.push(self.advance().unwrap());
            }
            while matches!(self.peek(), Some('0'..='9')) {
                num.push(self.advance().unwrap());
            }
        }
        // Suffix (f32, f64, i32, u64, etc.)
        if matches!(self.peek(), Some('f') | Some('i') | Some('u')) {
            while matches!(self.peek(), Some('a'..='z') | Some('0'..='9')) {
                self.advance();
            }
        }

        if is_float {
            Ok(TokenKind::Float(num.parse().unwrap_or(0.0)))
        } else {
            Ok(TokenKind::Int(num.parse().unwrap_or(0)))
        }
    }

    fn lex_ident_or_keyword(&mut self, first: char) -> TokenKind {
        let mut ident = String::from(first);
        while matches!(self.peek(), Some('a'..='z') | Some('A'..='Z') | Some('0'..='9') | Some('_')) {
            ident.push(self.advance().unwrap());
        }
        keyword_or_ident(ident)
    }
}

fn keyword_or_ident(s: String) -> TokenKind {
    match s.as_str() {
        "fn"        => TokenKind::Fn,
        "let"       => TokenKind::Let,
        "const"     => TokenKind::Const,
        "return"    => TokenKind::Return,
        "if"        => TokenKind::If,
        "else"      => TokenKind::Else,
        "while"     => TokenKind::While,
        "for"       => TokenKind::For,
        "loop"      => TokenKind::Loop,
        "break"     => TokenKind::Break,
        "continue"  => TokenKind::Continue,
        "struct"    => TokenKind::Struct,
        "enum"      => TokenKind::Enum,
        "impl"      => TokenKind::Impl,
        "trait"     => TokenKind::Trait,
        "import"    => TokenKind::Import,
        "export"    => TokenKind::Export,
        "pub"       => TokenKind::Pub,
        "unsafe"    => TokenKind::Unsafe,
        "async"     => TokenKind::Async,
        "await"     => TokenKind::Await,
        "spawn"     => TokenKind::Spawn,
        "match"     => TokenKind::Match,
        "in"        => TokenKind::In,
        "as"        => TokenKind::As,
        "extern"    => TokenKind::Extern,
        "comptime"  => TokenKind::Comptime,
        "test"      => TokenKind::Test,
        "mut"       => TokenKind::Mut,
        "ref"       => TokenKind::Ref,
        "true"      => TokenKind::Bool(true),
        "false"     => TokenKind::Bool(false),
        "null"      => TokenKind::Null,
        "self"      => TokenKind::Self_,
        "type"      => TokenKind::Type,
        // Primitive types
        "int"    => TokenKind::TyInt,
        "i8"     => TokenKind::TyI8,
        "i16"    => TokenKind::TyI16,
        "i32"    => TokenKind::TyI32,
        "i64"    => TokenKind::TyI64,
        "u8"     => TokenKind::TyU8,
        "u16"    => TokenKind::TyU16,
        "u32"    => TokenKind::TyU32,
        "u64"    => TokenKind::TyU64,
        "f32"    => TokenKind::TyF32,
        "f64"    => TokenKind::TyF64,
        "float"  => TokenKind::TyFloat,
        "bool"   => TokenKind::TyBool,
        "char"   => TokenKind::TyChar,
        "string" => TokenKind::TyString,
        "void"   => TokenKind::TyVoid,
        "usize"  => TokenKind::TyUsize,
        "isize"  => TokenKind::TyIsize,
        _        => TokenKind::Ident(s),
    }
}

// ─── Tests ───────────────────────────────────────────────────────────────────

#[cfg(test)]
mod tests {
    use super::*;

    fn lex(src: &str) -> Vec<TokenKind> {
        Lexer::new(src, "test.cand")
            .tokenize()
            .unwrap()
            .into_iter()
            .map(|t| t.kind)
            .collect()
    }

    #[test]
    fn test_hello_world_tokens() {
        let src = r#"fn main() { println("Hello, C&!"); }"#;
        let tokens = lex(src);
        assert!(tokens.contains(&TokenKind::Fn));
        assert!(tokens.contains(&TokenKind::Ident("main".into())));
        assert!(tokens.contains(&TokenKind::Ident("println".into())));
        assert!(tokens.contains(&TokenKind::StringLit("Hello, C&!".into())));
    }

    #[test]
    fn test_keywords() {
        let src = "let const fn return if else while for struct enum impl";
        let tokens = lex(src);
        assert!(tokens.contains(&TokenKind::Let));
        assert!(tokens.contains(&TokenKind::Const));
        assert!(tokens.contains(&TokenKind::Fn));
        assert!(tokens.contains(&TokenKind::Return));
        assert!(tokens.contains(&TokenKind::If));
        assert!(tokens.contains(&TokenKind::Else));
        assert!(tokens.contains(&TokenKind::While));
        assert!(tokens.contains(&TokenKind::For));
        assert!(tokens.contains(&TokenKind::Struct));
        assert!(tokens.contains(&TokenKind::Enum));
        assert!(tokens.contains(&TokenKind::Impl));
    }

    #[test]
    fn test_integer_literal() {
        let tokens = lex("42");
        assert_eq!(tokens[0], TokenKind::Int(42));
    }

    #[test]
    fn test_float_literal() {
        let tokens = lex("3.14");
        assert_eq!(tokens[0], TokenKind::Float(3.14));
    }

    #[test]
    fn test_hex_literal() {
        let tokens = lex("0xFF");
        assert_eq!(tokens[0], TokenKind::Int(255));
    }

    #[test]
    fn test_operators() {
        let src = "-> => == != <= >= && || += -= *= /=";
        let tokens = lex(src);
        assert!(tokens.contains(&TokenKind::Arrow));
        assert!(tokens.contains(&TokenKind::FatArrow));
        assert!(tokens.contains(&TokenKind::EqEq));
        assert!(tokens.contains(&TokenKind::BangEq));
        assert!(tokens.contains(&TokenKind::LtEq));
        assert!(tokens.contains(&TokenKind::GtEq));
        assert!(tokens.contains(&TokenKind::AmpAmp));
        assert!(tokens.contains(&TokenKind::PipePipe));
        assert!(tokens.contains(&TokenKind::PlusEq));
        assert!(tokens.contains(&TokenKind::MinusEq));
        assert!(tokens.contains(&TokenKind::StarEq));
        assert!(tokens.contains(&TokenKind::SlashEq));
    }

    #[test]
    fn test_string_escape() {
        let tokens = lex(r#""hello\nworld""#);
        assert_eq!(tokens[0], TokenKind::StringLit("hello\nworld".into()));
    }

    #[test]
    fn test_line_comment_skipped() {
        let tokens = lex("42 // this is a comment\n 10");
        assert_eq!(tokens[0], TokenKind::Int(42));
        assert_eq!(tokens[1], TokenKind::Int(10));
    }

    #[test]
    fn test_type_keywords() {
        let tokens = lex("int float bool string void");
        assert!(tokens.contains(&TokenKind::TyInt));
        assert!(tokens.contains(&TokenKind::TyFloat));
        assert!(tokens.contains(&TokenKind::TyBool));
        assert!(tokens.contains(&TokenKind::TyString));
        assert!(tokens.contains(&TokenKind::TyVoid));
    }
}
