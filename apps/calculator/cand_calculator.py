#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
C& Modern Linux Graphical Calculator (آلة حاسبة لينكس الحديثة بلغة C&)
Engineered for Linux Desktop (Fedora, KDE Plasma, GNOME, X11, Wayland)
Copyright (c) 2026 MSZ Studio. All rights reserved.
"""

import sys
import math
import tkinter as tk
from tkinter import font as tkfont

class ModernCandCalculator(tk.Tk):
    def __init__(self, custom_title="C& Modern Linux Calculator"):
        super().__init__()
        self.title(custom_title)
        self.resizable(False, False)
        
        # Color Palette - Catppuccin Mocha Modern Dark Theme
        self.COLOR_BG = "#181825"          # Main Window Base
        self.COLOR_SURFACE = "#1e1e2e"     # Display Box Background
        self.COLOR_BORDER = "#313244"      # Display Outline
        self.COLOR_TEXT_MAIN = "#cdd6f4"   # Primary text
        self.COLOR_TEXT_SUB = "#a6adc8"    # Calculation history text
        self.COLOR_ACCENT = "#89b4fa"      # Blue Accent
        
        # Button Themes
        self.BTN_NUM_BG = "#313244"
        self.BTN_NUM_FG = "#cdd6f4"
        self.BTN_NUM_HOVER = "#45475a"
        
        self.BTN_OP_BG = "#89b4fa"
        self.BTN_OP_FG = "#11111b"
        self.BTN_OP_HOVER = "#b4befe"
        
        self.BTN_ACT_BG = "#fab387"
        self.BTN_ACT_FG = "#11111b"
        self.BTN_ACT_HOVER = "#f9e2af"
        
        self.BTN_CLEAR_BG = "#f38ba8"
        self.BTN_CLEAR_FG = "#11111b"
        self.BTN_CLEAR_HOVER = "#eba0ac"
        
        self.BTN_EQ_BG = "#a6e3a1"
        self.BTN_EQ_FG = "#11111b"
        self.BTN_EQ_HOVER = "#94e2d5"
        
        self.BTN_SCI_BG = "#45475a"
        self.BTN_SCI_FG = "#cba6f7"
        self.BTN_SCI_HOVER = "#585b70"

        self.configure(bg=self.COLOR_BG)
        
        # State
        self.current_expr = ""
        self.pending_op = None
        self.first_operand = None
        self.new_entry = True
        self.memory = 0.0
        self.sci_mode = False
        
        # Typography
        self.font_display = tkfont.Font(family="Cantarell", size=28, weight="bold")
        self.font_sub = tkfont.Font(family="Cantarell", size=11)
        self.font_btn = tkfont.Font(family="Cantarell", size=15, weight="bold")
        self.font_btn_sm = tkfont.Font(family="Cantarell", size=12, weight="bold")
        self.font_title = tkfont.Font(family="Cantarell", size=11, weight="bold")
        
        self._setup_ui()
        self._setup_bindings()
        self._center_window()

    def _setup_ui(self):
        # Header Bar
        header = tk.Frame(self, bg=self.COLOR_BG, padx=16, pady=10)
        header.pack(fill="x")
        
        title_lbl = tk.Label(header, text="C& Calculator", font=self.font_title, fg=self.COLOR_ACCENT, bg=self.COLOR_BG)
        title_lbl.pack(side="left")
        
        self.sci_toggle_btn = tk.Button(
            header, text="📐 Sci Mode", font=self.font_sub, bg=self.BTN_SCI_BG, fg=self.BTN_SCI_FG,
            activebackground=self.BTN_SCI_HOVER, activeforeground=self.BTN_SCI_FG,
            bd=0, relief="flat", padx=8, pady=3, cursor="hand2", command=self.toggle_sci_mode
        )
        self.sci_toggle_btn.pack(side="right")

        # Display Card
        display_frame = tk.Frame(self, bg=self.COLOR_SURFACE, highlightthickness=1, highlightbackground=self.COLOR_BORDER, padx=16, pady=14)
        display_frame.pack(fill="x", padx=16, pady=(0, 12))
        
        self.sub_display = tk.Label(
            display_frame, text="", font=self.font_sub, fg=self.COLOR_TEXT_SUB, bg=self.COLOR_SURFACE, anchor="e"
        )
        self.sub_display.pack(fill="x")
        
        self.main_display = tk.Label(
            display_frame, text="0", font=self.font_display, fg=self.COLOR_TEXT_MAIN, bg=self.COLOR_SURFACE, anchor="e"
        )
        self.main_display.pack(fill="x", pady=(4, 0))

        # Memory Controls Row
        mem_frame = tk.Frame(self, bg=self.COLOR_BG, padx=16)
        mem_frame.pack(fill="x", pady=(0, 8))
        
        mem_btns = [
            ("MC", self.mem_clear),
            ("MR", self.mem_recall),
            ("M+", self.mem_add),
            ("M-", self.mem_sub),
            ("MS", self.mem_store)
        ]
        for idx, (label, cmd) in enumerate(mem_btns):
            btn = tk.Button(
                mem_frame, text=label, font=self.font_sub, bg=self.COLOR_BG, fg=self.COLOR_ACCENT,
                activebackground=self.COLOR_SURFACE, activeforeground="#ffffff",
                bd=0, relief="flat", cursor="hand2", command=cmd, padx=6, pady=2
            )
            btn.pack(side="left", expand=True, fill="x")

        # Scientific Panel (Collapsible)
        self.sci_frame = tk.Frame(self, bg=self.COLOR_BG, padx=16)
        
        sci_buttons = [
            ("sin", lambda: self.apply_func("sin")),
            ("cos", lambda: self.apply_func("cos")),
            ("tan", lambda: self.apply_func("tan")),
            ("√x", lambda: self.apply_func("sqrt")),
            ("x²", lambda: self.apply_func("sqr")),
            ("xʸ", lambda: self.set_op("^")),
            ("ln", lambda: self.apply_func("ln")),
            ("log", lambda: self.apply_func("log")),
            ("π", lambda: self.insert_const(math.pi)),
            ("e", lambda: self.insert_const(math.e)),
            ("(", lambda: self.append_char("(")),
            (")", lambda: self.append_char(")"))
        ]
        
        for i, (text, cmd) in enumerate(sci_buttons):
            r = i // 4
            c = i % 4
            btn = self._create_btn(self.sci_frame, text, cmd, self.BTN_SCI_BG, self.BTN_SCI_FG, self.BTN_SCI_HOVER, font=self.font_btn_sm)
            btn.grid(row=r, column=c, padx=3, pady=3, sticky="nsew")
        for c in range(4): self.sci_frame.columnconfigure(c, weight=1)

        # Main Keypad Grid
        self.keypad = tk.Frame(self, bg=self.COLOR_BG, padx=16, pady=8)
        self.keypad.pack(fill="both", expand=True, pady=(0, 12))
        
        key_layout = [
            [("C", self.clear_all, self.BTN_CLEAR_BG, self.BTN_CLEAR_FG, self.BTN_CLEAR_HOVER),
             ("CE", self.clear_entry, self.BTN_ACT_BG, self.BTN_ACT_FG, self.BTN_ACT_HOVER),
             ("⌫", self.backspace, self.BTN_ACT_BG, self.BTN_ACT_FG, self.BTN_ACT_HOVER),
             ("÷", lambda: self.set_op("÷"), self.BTN_OP_BG, self.BTN_OP_FG, self.BTN_OP_HOVER)],
            
            [("7", lambda: self.append_num("7"), self.BTN_NUM_BG, self.BTN_NUM_FG, self.BTN_NUM_HOVER),
             ("8", lambda: self.append_num("8"), self.BTN_NUM_BG, self.BTN_NUM_FG, self.BTN_NUM_HOVER),
             ("9", lambda: self.append_num("9"), self.BTN_NUM_BG, self.BTN_NUM_FG, self.BTN_NUM_HOVER),
             ("×", lambda: self.set_op("×"), self.BTN_OP_BG, self.BTN_OP_FG, self.BTN_OP_HOVER)],
            
            [("4", lambda: self.append_num("4"), self.BTN_NUM_BG, self.BTN_NUM_FG, self.BTN_NUM_HOVER),
             ("5", lambda: self.append_num("5"), self.BTN_NUM_BG, self.BTN_NUM_FG, self.BTN_NUM_HOVER),
             ("6", lambda: self.append_num("6"), self.BTN_NUM_BG, self.BTN_NUM_FG, self.BTN_NUM_HOVER),
             ("−", lambda: self.set_op("−"), self.BTN_OP_BG, self.BTN_OP_FG, self.BTN_OP_HOVER)],
            
            [("1", lambda: self.append_num("1"), self.BTN_NUM_BG, self.BTN_NUM_FG, self.BTN_NUM_HOVER),
             ("2", lambda: self.append_num("2"), self.BTN_NUM_BG, self.BTN_NUM_FG, self.BTN_NUM_HOVER),
             ("3", lambda: self.append_num("3"), self.BTN_NUM_BG, self.BTN_NUM_FG, self.BTN_NUM_HOVER),
             ("+", lambda: self.set_op("+"), self.BTN_OP_BG, self.BTN_OP_FG, self.BTN_OP_HOVER)],
            
            [("±", self.toggle_sign, self.BTN_NUM_BG, self.BTN_NUM_FG, self.BTN_NUM_HOVER),
             ("0", lambda: self.append_num("0"), self.BTN_NUM_BG, self.BTN_NUM_FG, self.BTN_NUM_HOVER),
             (".", self.append_dot, self.BTN_NUM_BG, self.BTN_NUM_FG, self.BTN_NUM_HOVER),
             ("=", self.calculate, self.BTN_EQ_BG, self.BTN_EQ_FG, self.BTN_EQ_HOVER)]
        ]

        for r, row in enumerate(key_layout):
            for c, (txt, cmd, bg, fg, hov) in enumerate(row):
                btn = self._create_btn(self.keypad, txt, cmd, bg, fg, hov, font=self.font_btn)
                btn.grid(row=r, column=c, padx=4, pady=4, sticky="nsew")

        for c in range(4): self.keypad.columnconfigure(c, weight=1)
        for r in range(5): self.keypad.rowconfigure(r, weight=1)

    def _create_btn(self, parent, text, cmd, bg, fg, hover_bg, font=None):
        f = font if font else self.font_btn
        btn = tk.Button(
            parent, text=text, font=f, bg=bg, fg=fg,
            activebackground=hover_bg, activeforeground=fg,
            bd=0, relief="flat", cursor="hand2", command=cmd,
            width=4, height=2
        )
        btn.bind("<Enter>", lambda e: btn.configure(bg=hover_bg))
        btn.bind("<Leave>", lambda e: btn.configure(bg=bg))
        return btn

    def _setup_bindings(self):
        self.bind("<Key>", self._handle_keypress)

    def _handle_keypress(self, event):
        ch = event.char
        keysym = event.keysym
        if ch in "0123456789":
            self.append_num(ch)
        elif ch == ".":
            self.append_dot()
        elif ch == "+":
            self.set_op("+")
        elif ch == "-":
            self.set_op("−")
        elif ch == "*":
            self.set_op("×")
        elif ch == "/":
            self.set_op("÷")
        elif keysym in ("Return", "KP_Enter") or ch == "=":
            self.calculate()
        elif keysym in ("BackSpace",):
            self.backspace()
        elif keysym in ("Escape",):
            self.clear_all()
        elif ch == "%":
            self.apply_percent()

    def _center_window(self):
        self.update_idletasks()
        w = self.winfo_reqwidth()
        h = self.winfo_reqheight()
        sw = self.winfo_screenwidth()
        sh = self.winfo_screenheight()
        x = max(0, (sw - w) // 2)
        y = max(0, (sh - h) // 2 - 40)
        self.geometry(f"+{x}+{y}")

    def toggle_sci_mode(self):
        self.sci_mode = not self.sci_mode
        if self.sci_mode:
            self.sci_frame.pack(fill="x", pady=(0, 8), before=self.keypad)
            self.sci_toggle_btn.configure(text="✕ Standard", bg=self.BTN_CLEAR_BG, fg=self.BTN_CLEAR_FG)
        else:
            self.sci_frame.pack_forget()
            self.sci_toggle_btn.configure(text="📐 Sci Mode", bg=self.BTN_SCI_BG, fg=self.BTN_SCI_FG)
        self._center_window()

    def _format_num(self, val):
        if math.isinf(val) or math.isnan(val):
            return "Error"
        if val == int(val) and abs(val) < 1e14:
            return f"{int(val):,}"
        formatted = f"{val:.8g}"
        return formatted

    def append_num(self, digit):
        cur = self.main_display["text"].replace(",", "")
        if self.new_entry or cur == "0" or cur == "Error":
            self.main_display.configure(text=digit)
            self.new_entry = False
        else:
            if len(cur) < 16:
                new_text = cur + digit
                if "." not in new_text:
                    new_text = f"{int(new_text):,}"
                self.main_display.configure(text=new_text)

    def append_dot(self):
        cur = self.main_display["text"]
        if self.new_entry or cur == "Error":
            self.main_display.configure(text="0.")
            self.new_entry = False
        elif "." not in cur:
            self.main_display.configure(text=cur + ".")

    def append_char(self, ch):
        self.append_num(ch)

    def toggle_sign(self):
        cur = self.main_display["text"].replace(",", "")
        if cur == "0" or cur == "Error": return
        if cur.startswith("-"):
            val = cur[1:]
        else:
            val = "-" + cur
        self.main_display.configure(text=val)

    def set_op(self, op):
        cur = float(self.main_display["text"].replace(",", ""))
        if self.pending_op and not self.new_entry:
            self.calculate()
            cur = float(self.main_display["text"].replace(",", ""))
        self.first_operand = cur
        self.pending_op = op
        self.sub_display.configure(text=f"{self._format_num(cur)} {op}")
        self.new_entry = True

    def calculate(self):
        if not self.pending_op or self.first_operand is None:
            return
        second_str = self.main_display["text"].replace(",", "")
        try:
            second_operand = float(second_str)
        except ValueError:
            return

        res = 0.0
        op = self.pending_op
        if op == "+": res = self.first_operand + second_operand
        elif op == "−": res = self.first_operand - second_operand
        elif op == "×": res = self.first_operand * second_operand
        elif op == "÷":
            if second_operand == 0:
                self.main_display.configure(text="Cannot divide by 0")
                self.sub_display.configure(text="")
                self.new_entry = True
                self.pending_op = None
                return
            res = self.first_operand / second_operand
        elif op == "^": res = math.pow(self.first_operand, second_operand)

        self.sub_display.configure(text=f"{self._format_num(self.first_operand)} {op} {self._format_num(second_operand)} =")
        self.main_display.configure(text=self._format_num(res))
        self.first_operand = res
        self.pending_op = None
        self.new_entry = True

    def apply_func(self, func_name):
        try:
            val = float(self.main_display["text"].replace(",", ""))
            if func_name == "sin": res = math.sin(math.radians(val))
            elif func_name == "cos": res = math.cos(math.radians(val))
            elif func_name == "tan": res = math.tan(math.radians(val))
            elif func_name == "sqrt":
                if val < 0:
                    self.main_display.configure(text="Invalid input")
                    return
                res = math.sqrt(val)
            elif func_name == "sqr": res = val * val
            elif func_name == "ln":
                if val <= 0:
                    self.main_display.configure(text="Invalid input")
                    return
                res = math.log(val)
            elif func_name == "log":
                if val <= 0:
                    self.main_display.configure(text="Invalid input")
                    return
                res = math.log10(val)
            else:
                return

            self.sub_display.configure(text=f"{func_name}({self._format_num(val)}) =")
            self.main_display.configure(text=self._format_num(res))
            self.new_entry = True
        except Exception:
            self.main_display.configure(text="Error")

    def insert_const(self, const_val):
        self.main_display.configure(text=self._format_num(const_val))
        self.new_entry = True

    def apply_percent(self):
        try:
            val = float(self.main_display["text"].replace(",", ""))
            res = val / 100.0
            self.main_display.configure(text=self._format_num(res))
            self.new_entry = True
        except Exception:
            pass

    def backspace(self):
        cur = self.main_display["text"].replace(",", "")
        if self.new_entry or cur in ("0", "Error", "Cannot divide by 0"):
            return
        if len(cur) > 1:
            self.main_display.configure(text=cur[:-1])
        else:
            self.main_display.configure(text="0")
            self.new_entry = True

    def clear_entry(self):
        self.main_display.configure(text="0")
        self.new_entry = True

    def clear_all(self):
        self.main_display.configure(text="0")
        self.sub_display.configure(text="")
        self.first_operand = None
        self.pending_op = None
        self.new_entry = True

    # Memory Operations
    def mem_clear(self):
        self.memory = 0.0
    def mem_recall(self):
        self.main_display.configure(text=self._format_num(self.memory))
        self.new_entry = True
    def mem_add(self):
        try: self.memory += float(self.main_display["text"].replace(",", ""))
        except: pass
    def mem_sub(self):
        try: self.memory -= float(self.main_display["text"].replace(",", ""))
        except: pass
    def mem_store(self):
        try: self.memory = float(self.main_display["text"].replace(",", ""))
        except: pass

def main():
    title = "C& Modern Linux Calculator"
    if len(sys.argv) > 1:
        title = sys.argv[1]
    app = ModernCandCalculator(custom_title=title)
    app.mainloop()

if __name__ == "__main__":
    main()
