//! C& Semantic Analyzer & Type Checker

use cand_ast::*;

#[derive(Debug, thiserror::Error)]
pub enum SemanticError {
    #[error("Type mismatch: expected {expected}, found {found}")]
    TypeMismatch { expected: String, found: String },
    #[error("Undeclared identifier: {0}")]
    UndeclaredIdent(String),
}

pub struct SemanticAnalyzer {
    pub errors: Vec<SemanticError>,
}

impl SemanticAnalyzer {
    pub fn new() -> Self {
        SemanticAnalyzer { errors: vec![] }
    }

    pub fn analyze(&mut self, _program: &Program) -> Result<(), SemanticError> {
        Ok(())
    }
}
