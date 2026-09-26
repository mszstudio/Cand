# C& Programming Language — Linux Ecosystem & Packaging Guide
دليل حزم وتعريفات لغة البرمجة C& لجميع توزيعات لينكس

This directory provides comprehensive native installation scripts and official package definitions for all major Linux distributions.

---

## 🚀 1. Universal One-Command Quick Install (جميع التوزيعات)

To automatically compile, install, and configure `cand` in your PATH on any Linux distribution, run:

```bash
chmod +x install.sh
./install.sh
```

### What `install.sh` does automatically:
1. **Detects your Linux distribution**: (Fedora, Ubuntu, Debian, Arch Linux, Alpine, openSUSE, etc.).
2. **Checks toolchain dependencies**: Ensures `gcc`/`clang` and standard C libraries are installed, with automatic installation instructions via the detected package manager (`dnf`, `apt`, `pacman`, `zypper`, `apk`).
3. **Builds native compiler binary**: Compiles `cand` using `-O2` optimization and native C backend.
4. **Installs compiler & standard library**:
   - Binary: `/usr/local/bin/cand` (or `~/.local/bin/cand`)
   - Standard Library: `/usr/local/share/cand/std` (or `~/.local/share/cand/std`)
5. **Configures PATH automatically**: Appends clean export entries to `~/.bashrc`, `~/.zshrc`, `~/.profile`, and Fish shell.
6. **Configures `CAND_PATH`**: Ensures Cand resolves `std/math`, `std/sys`, `std/net`, `std/ai`, etc. from any directory on the system.

---

## 📦 2. Distribution Packages & Build Definitions

### A. Fedora / Red Hat Enterprise Linux / CentOS / Rocky / Alma (`.rpm`)
Use the official RPM spec file: [`cand.spec`](cand.spec).

```bash
# Install rpm build tools
sudo dnf install -y rpm-build gcc make

# Build RPM package
rpmbuild -ba linux/cand.spec
```

### B. Arch Linux / Manjaro / EndeavourOS (`PKGBUILD`)
Use the official Arch packaging script: [`PKGBUILD`](PKGBUILD).

```bash
cd linux
makepkg -si
```

### C. Debian / Ubuntu / Linux Mint / Pop!_OS (`.deb`)
Use the Debian definitions in [`debian/`](debian/).

```bash
sudo apt-get install -y build-essential debhelper devscripts
debuild -b -uc -us
```

### D. Generic POSIX / Alpine Linux
Using the universal Makefile:

```bash
make
sudo make install
```

---

## 🧪 3. Verification & Testing

Verify that your installation is working:

```bash
cand version
cand doctor
cand test
```

---

## 🗑️ 4. Uninstallation

To cleanly remove `cand` and restore shell PATH configurations:

```bash
chmod +x linux/uninstall.sh
./linux/uninstall.sh
```
Or via Makefile:
```bash
sudo make uninstall
```
