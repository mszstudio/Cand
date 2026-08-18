@echo off
echo Building Independent C^& Native Compiler (cand.exe)...

set CLANG_BIN="C:\Program Files\LLVM\bin\clang.exe"
set INC_FLAGS=-I. -D_CRT_SECURE_NO_WARNINGS -I"C:\BuildTools\VC\Tools\MSVC\14.44.35207\include" -I"C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\ucrt" -I"C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\um" -I"C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\shared"
set LIB_FLAGS=-L"C:\BuildTools\VC\Tools\MSVC\14.44.35207\lib\x64" -L"C:\Program Files (x86)\Windows Kits\10\Lib\10.0.22621.0\ucrt\x64" -L"C:\Program Files (x86)\Windows Kits\10\Lib\10.0.22621.0\um\x64"

if exist %CLANG_BIN% (
    echo Using compiler: %CLANG_BIN%
    %CLANG_BIN% -O2 %INC_FLAGS% %LIB_FLAGS% src/cand_lexer.c src/cand_parser.c src/cand_semantic.c src/cand_codegen.c src/cand_driver.c -o cand.exe
    if %ERRORLEVEL% EQU 0 (
        echo [SUCCESS] Built cand.exe successfully!
        exit /b 0
    )
)

gcc -O2 -I. src/cand_lexer.c src/cand_parser.c src/cand_semantic.c src/cand_codegen.c src/cand_driver.c -o cand.exe 2>nul
if %ERRORLEVEL% EQU 0 (
    echo [SUCCESS] Built cand.exe successfully with GCC!
    exit /b 0
)

clang -O2 -I. src/cand_lexer.c src/cand_parser.c src/cand_semantic.c src/cand_codegen.c src/cand_driver.c -o cand.exe 2>nul
if %ERRORLEVEL% EQU 0 (
    echo [SUCCESS] Built cand.exe successfully with Clang!
    exit /b 0
)

echo [ERROR] Failed to compile cand.exe.
exit /b 1
