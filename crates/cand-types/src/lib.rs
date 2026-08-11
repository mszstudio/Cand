//! C& Type System definitions

use cand_ast::*;

pub fn is_assignable(target: &Type, source: &Type) -> bool {
    target == source
}
