Name:           cand
Version:        1.0.0
Release:        1%{?dist}
Summary:        C& Programming Language Native Compiler Toolchain
License:        MIT
URL:            https://github.com/mszstudio/Cand
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  gcc
BuildRequires:  make
BuildRequires:  glibc-devel
Requires:       gcc
Requires:       glibc

%description
C& (Cand) is an independent native programming language designed for
high performance, AI computation, direct C interop, and clean expressive syntax.
This package provides the complete standalone compiler binary (cand) and standard libraries.

%prep
%setup -q

%build
make CC=gcc CFLAGS="%{optflags} -I. -Isrc" LDFLAGS="-lm"

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}%{_bindir}
mkdir -p %{buildroot}%{_datadir}/cand/std

install -m 755 cand %{buildroot}%{_bindir}/cand
cp -rf std/* %{buildroot}%{_datadir}/cand/std/
install -m 644 cand.toml %{buildroot}%{_datadir}/cand/cand.toml 2>/dev/null || true

%files
%license LICENSE
%doc README.md LANGUAGE.md ARCHITECTURE.md
%{_bindir}/cand
%{_datadir}/cand

%changelog
* Mon Sep 21 2026 MSZ Studio <contact@mszstudio.com> - 1.0.0-1
- Initial release of C& Programming Language native compiler package for Fedora/RHEL.
