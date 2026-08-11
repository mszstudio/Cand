//! C& Diagnostics — Rich, beautiful compiler diagnostics and error reporting.

use colored::*;

#[derive(Debug, Clone)]
pub enum DiagnosticLevel {
    Error,
    Warning,
    Note,
    Help,
}

#[derive(Debug, Clone)]
pub struct Diagnostic {
    pub code: String,
    pub level: DiagnosticLevel,
    pub message: String,
    pub file: String,
    pub line: u32,
    pub col: u32,
    pub end_col: u32,
    pub source_line: String,
    pub expected: Option<String>,
    pub found: Option<String>,
    pub help: Option<String>,
}

impl Diagnostic {
    pub fn new(
        code: impl Into<String>,
        level: DiagnosticLevel,
        message: impl Into<String>,
        file: impl Into<String>,
        line: u32,
        col: u32,
    ) -> Self {
        Diagnostic {
            code: code.into(),
            level,
            message: message.into(),
            file: file.into(),
            line,
            col,
            end_col: col + 1,
            source_line: String::new(),
            expected: None,
            found: None,
            help: None,
        }
    }

    pub fn with_source(mut self, source_line: impl Into<String>) -> Self {
        self.source_line = source_line.into();
        self
    }

    pub fn with_expected(mut self, expected: impl Into<String>) -> Self {
        self.expected = Some(expected.into());
        self
    }

    pub fn with_found(mut self, found: impl Into<String>) -> Self {
        self.found = Some(found.into());
        self
    }

    pub fn with_help(mut self, help: impl Into<String>) -> Self {
        self.help = Some(help.into());
        self
    }

    pub fn emit(&self) {
        let (level_str, level_color) = match self.level {
            DiagnosticLevel::Error => ("error", "red"),
            DiagnosticLevel::Warning => ("warning", "yellow"),
            DiagnosticLevel::Note => ("note", "cyan"),
            DiagnosticLevel::Help => ("help", "green"),
        };

        eprintln!(
            "{}[{}]: {}",
            level_str.color(level_color).bold(),
            self.code.bold(),
            self.message.bold()
        );

        eprintln!(
            "  --> {}:{}:{}",
            self.file, self.line, self.col
        );

        if !self.source_line.is_empty() {
            let line_num_str = format!("{}", self.line);
            let padding = " ".repeat(line_num_str.len());

            eprintln!(" {} |", padding);
            eprintln!(" {} | {}", line_num_str.blue().bold(), self.source_line);

            let underline_len = (self.end_col - self.col).max(1) as usize;
            let underline_pad = " ".repeat((self.col.saturating_sub(1)) as usize);
            let underline = "^".repeat(underline_len).color(level_color).bold();

            eprintln!(" {} | {}{}", padding, underline_pad, underline);
        }

        if let Some(ref expected) = self.expected {
            eprintln!("expected:\n    {}", expected.green());
        }

        if let Some(ref found) = self.found {
            eprintln!("found:\n    {}", found.red());
        }

        if let Some(ref help) = self.help {
            eprintln!("help:\n    {}", help.cyan());
        }

        eprintln!();
    }
}
