@echo off
setlocal
title Build C& Language Installer Package

echo =========================================================================
echo   Building C& Language Installer Package
echo =========================================================================

call build.bat

if exist "%ProgramFiles(x86)%\Inno Setup 6\ISCC.exe" (
    echo Building Inno Setup EXE Package...
    "%ProgramFiles(x86)%\Inno Setup 6\ISCC.exe" installer.iss
    echo [SUCCESS] Standalone Installer created: Output\CandLanguage_Setup_v1.0.0.exe
) else if exist "%ProgramFiles%\Inno Setup 6\ISCC.exe" (
    echo Building Inno Setup EXE Package...
    "%ProgramFiles%\Inno Setup 6\ISCC.exe" installer.iss
    echo [SUCCESS] Standalone Installer created: Output\CandLanguage_Setup_v1.0.0.exe
) else (
    echo [NOTE] Inno Setup compiler ISCC.exe not found on system.
    echo [OK] Standard one-click installer script 'install.bat' is ready for distribution!
)
