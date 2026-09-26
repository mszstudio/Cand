#!/usr/bin/env bash
# =============================================================================
# C& Programming Language Toolchain Installer for Linux
# مثبت مترجم لغة البرمجة C& وضبط المسارات البيئية تلقائياً لجميع توزيعات لينكس
# Copyright (c) 2026 MSZ Studio. All rights reserved.
# =============================================================================

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m'

echo -e "${CYAN}${BOLD}=========================================================================${NC}"
echo -e "${CYAN}${BOLD}   C& Programming Language — Native Linux Toolchain Installer${NC}"
echo -e "${CYAN}${BOLD}   تثبيت محرك لغة البرمجة C& وإضافته إلى المتغيرات البيئية تلقائياً${NC}"
echo -e "${CYAN}${BOLD}=========================================================================${NC}\n"

# 1. Detect Linux Distribution
DISTRO="Generic Linux"
PKG_MGR=""
INSTALL_CMD=""

if [ -f /etc/os-release ]; then
    . /etc/os-release
    DISTRO="$NAME $VERSION_ID"
fi

echo -e "[1/6] ${BLUE}Detecting Linux environment...${NC}"
echo -e "      Operating System: ${BOLD}${DISTRO}${NC}"

if command -v dnf >/dev/null 2>&1; then
    PKG_MGR="dnf"
    INSTALL_CMD="sudo dnf install -y gcc make glibc-devel"
elif command -v apt-get >/dev/null 2>&1; then
    PKG_MGR="apt"
    INSTALL_CMD="sudo apt-get update && sudo apt-get install -y build-essential"
elif command -v pacman >/dev/null 2>&1; then
    PKG_MGR="pacman"
    INSTALL_CMD="sudo pacman -S --noconfirm base-devel gcc"
elif command -v zypper >/dev/null 2>&1; then
    PKG_MGR="zypper"
    INSTALL_CMD="sudo zypper install -y gcc make"
elif command -v apk >/dev/null 2>&1; then
    PKG_MGR="apk"
    INSTALL_CMD="sudo apk add build-base gcc make"
elif command -v yum >/dev/null 2>&1; then
    PKG_MGR="yum"
    INSTALL_CMD="sudo yum install -y gcc make"
fi

# 2. Check Compiler Toolchain Dependencies
echo -e "\n[2/6] ${BLUE}Checking build toolchain dependencies...${NC}"
COMPILER=""
if command -v gcc >/dev/null 2>&1; then
    COMPILER="gcc"
elif command -v clang >/dev/null 2>&1; then
    COMPILER="clang"
elif command -v cc >/dev/null 2>&1; then
    COMPILER="cc"
fi

if [ -z "$COMPILER" ]; then
    echo -e "${YELLOW}[!] Host C compiler (gcc or clang) not found on system.${NC}"
    if [ -n "$INSTALL_CMD" ]; then
        echo -e "    Attempting to install build tools via ${PKG_MGR}..."
        echo -e "    Running: ${CYAN}${INSTALL_CMD}${NC}"
        if eval "$INSTALL_CMD"; then
            echo -e "${GREEN}[✓] Toolchain dependencies installed successfully!${NC}"
            if command -v gcc >/dev/null 2>&1; then COMPILER="gcc"; else COMPILER="clang"; fi
        else
            echo -e "${RED}[ERROR] Failed to install compiler automatically.${NC}"
            echo -e "Please install gcc or clang manually using:"
            echo -e "  ${CYAN}${INSTALL_CMD}${NC}"
            exit 1
        fi
    else
        echo -e "${RED}[ERROR] Please install gcc or clang and re-run this installer.${NC}"
        exit 1
    fi
else
    echo -e "      ${GREEN}[✓] Detected C compiler: $COMPILER ($($COMPILER --version | head -n1))${NC}"
fi

# 3. Determine Installation Destination
echo -e "\n[3/6] ${BLUE}Selecting installation target...${NC}"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

IS_ROOT=0
if [ "$(id -u)" -eq 0 ]; then
    IS_ROOT=1
fi

if [ "$IS_ROOT" -eq 1 ]; then
    BIN_DIR="/usr/local/bin"
    SHARE_DIR="/usr/local/share/cand"
else
    # Try writing to /usr/local/bin via sudo if available, else user local directory
    if sudo -n true 2>/dev/null; then
        BIN_DIR="/usr/local/bin"
        SHARE_DIR="/usr/local/share/cand"
        USE_SUDO=1
    else
        BIN_DIR="$HOME/.local/bin"
        SHARE_DIR="$HOME/.local/share/cand"
        USE_SUDO=0
    fi
fi

echo -e "      Binary target:     ${CYAN}${BIN_DIR}/cand${NC}"
echo -e "      Standard library:  ${CYAN}${SHARE_DIR}/std${NC}"

# 4. Compile the Native C& Compiler
echo -e "\n[4/6] ${BLUE}Compiling C& native compiler from source...${NC}"
$COMPILER -O2 -Wall -Wextra -I. -Isrc \
    src/cand_lexer.c \
    src/cand_parser.c \
    src/cand_semantic.c \
    src/cand_codegen.c \
    src/cand_driver.c \
    -lm -o cand

if [ ! -f "cand" ]; then
    echo -e "${RED}[ERROR] Compilation failed! Binary 'cand' was not generated.${NC}"
    exit 1
fi
echo -e "      ${GREEN}[✓] Built native binary 'cand' successfully!${NC}"

# 5. Installing Binary and Standard Libraries
echo -e "\n[5/6] ${BLUE}Installing files to system...${NC}"
if [ "$USE_SUDO" = "1" ]; then
    sudo mkdir -p "$BIN_DIR" "$SHARE_DIR"
    sudo cp -f cand "$BIN_DIR/cand"
    sudo chmod 755 "$BIN_DIR/cand"
    sudo cp -rf std "$SHARE_DIR/"
    sudo cp -f cand.toml "$SHARE_DIR/" 2>/dev/null || true
else
    mkdir -p "$BIN_DIR" "$SHARE_DIR"
    cp -f cand "$BIN_DIR/cand"
    chmod 755 "$BIN_DIR/cand"
    cp -rf std "$SHARE_DIR/"
    cp -f cand.toml "$SHARE_DIR/" 2>/dev/null || true
fi
echo -e "      ${GREEN}[✓] Installed binary and standard library!${NC}"

# 6. Automatic PATH & Environment Registration
echo -e "\n[6/6] ${BLUE}Configuring PATH and environment variables automatically...${NC}"

setup_shell_config() {
    local CONF="$1"
    if [ -f "$CONF" ] || [ -f "$HOME/$(basename "$CONF")" ]; then
        if ! grep -q "$BIN_DIR" "$CONF" 2>/dev/null; then
            echo "" >> "$CONF"
            echo "# Added by C& Language Installer" >> "$CONF"
            echo "export PATH=\"$BIN_DIR:\$PATH\"" >> "$CONF"
            echo "export CAND_PATH=\"$SHARE_DIR\"" >> "$CONF"
            echo -e "      ${GREEN}[✓] Registered in $CONF${NC}"
        else
            echo -e "      ${CYAN}[i] Already present in $CONF${NC}"
        fi
    fi
}

setup_shell_config "$HOME/.bashrc"
setup_shell_config "$HOME/.zshrc"
setup_shell_config "$HOME/.profile"
setup_shell_config "$HOME/.bash_profile"

# Fish shell support
if [ -d "$HOME/.config/fish" ]; then
    mkdir -p "$HOME/.config/fish"
    FISH_CONF="$HOME/.config/fish/config.fish"
    if ! grep -q "$BIN_DIR" "$FISH_CONF" 2>/dev/null; then
        echo "" >> "$FISH_CONF"
        echo "# Added by C& Language Installer" >> "$FISH_CONF"
        echo "set -gx PATH $BIN_DIR \$PATH" >> "$FISH_CONF"
        echo "set -gx CAND_PATH $SHARE_DIR" >> "$FISH_CONF"
        echo -e "      ${GREEN}[✓] Registered in $FISH_CONF${NC}"
    fi
fi

# Export for current installer session
export PATH="$BIN_DIR:$PATH"
export CAND_PATH="$SHARE_DIR"

echo -e "\n${GREEN}${BOLD}=========================================================================${NC}"
echo -e "${GREEN}${BOLD}  [SUCCESS] C& Programming Language installed successfully!${NC}"
echo -e "${GREEN}${BOLD}  تم تثبيت محرك لغة البرمجة C& بنجاح على نظام لينكس!${NC}"
echo -e "${GREEN}${BOLD}=========================================================================${NC}\n"

echo -e "Compiler location: ${CYAN}$BIN_DIR/cand${NC}"
echo -e "Standard Library:  ${CYAN}$SHARE_DIR/std${NC}\n"

echo -e "Quick test commands (يمكنك تجربة الأوامر التالية):"
echo -e "  ${BOLD}cand version${NC}     - Display compiler version"
echo -e "  ${BOLD}cand doctor${NC}      - Verify development environment"
echo -e "  ${BOLD}cand run <file>${NC}  - Compile and run any .cand file directly"
echo -e "  ${BOLD}cand test${NC}        - Execute project unit tests\n"

if [ "$BIN_DIR" != "/usr/local/bin" ] && [[ ":$PATH:" != *":$BIN_DIR:"* ]]; then
    echo -e "${YELLOW}[NOTE] To activate 'cand' in your current terminal session immediately, run:${NC}"
    echo -e "  ${CYAN}source ~/.bashrc${NC}  (or: ${CYAN}export PATH=\"$BIN_DIR:\$PATH\"${NC})\n"
fi
