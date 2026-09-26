#!/usr/bin/env bash
# =============================================================================
# C& Programming Language Toolchain Uninstaller for Linux
# مزيل تثبيت لغة البرمجة C& من النظام
# =============================================================================

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m'

echo -e "${CYAN}${BOLD}=========================================================================${NC}"
echo -e "${CYAN}${BOLD}   C& Programming Language Toolchain Uninstaller${NC}"
echo -e "${CYAN}${BOLD}=========================================================================${NC}\n"

remove_file() {
    local target="$1"
    if [ -e "$target" ]; then
        if [ -w "$(dirname "$target")" ]; then
            rm -rf "$target"
        elif sudo -n true 2>/dev/null; then
            sudo rm -rf "$target"
        else
            echo "Requesting sudo privileges to remove $target..."
            sudo rm -rf "$target"
        fi
        echo -e "${GREEN}[✓] Removed: $target${NC}"
    fi
}

echo "Removing C& binaries and standard library files..."
remove_file "/usr/local/bin/cand"
remove_file "/usr/bin/cand"
remove_file "/usr/local/share/cand"
remove_file "/usr/share/cand"
remove_file "$HOME/.local/bin/cand"
remove_file "$HOME/.local/share/cand"

echo -e "\nCleaning shell configuration entries..."
for rc in "$HOME/.bashrc" "$HOME/.zshrc" "$HOME/.profile" "$HOME/.bash_profile" "$HOME/.config/fish/config.fish"; do
    if [ -f "$rc" ]; then
        if grep -q "CAND_PATH" "$rc" 2>/dev/null; then
            sed -i '/C& Language Installer/d' "$rc" 2>/dev/null || true
            sed -i '/export CAND_PATH=/d' "$rc" 2>/dev/null || true
            sed -i '/set -gx CAND_PATH/d' "$rc" 2>/dev/null || true
            echo -e "${GREEN}[✓] Cleaned $rc${NC}"
        fi
    fi
done

echo -e "\n${GREEN}${BOLD}[SUCCESS] C& Programming Language toolchain has been uninstalled.${NC}\n"
