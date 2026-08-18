@echo off
setlocal enabledelayedexpansion
title C& Programming Language Toolchain Installer

echo =========================================================================
echo   C& Programming Language Toolchain Installer (Automatic PATH Setup)
echo   تثبيت محرك لغة البرمجة C& وإضافته إلى المتغيرات البيئية تلقائياً
echo =========================================================================

set "TARGET_DIR=%LOCALAPPDATA%\CandLanguage"

echo.
echo [1/4] Creating installation directory at %TARGET_DIR%...
if not exist "%TARGET_DIR%" mkdir "%TARGET_DIR%"
if not exist "%TARGET_DIR%\std" mkdir "%TARGET_DIR%\std"
if not exist "%TARGET_DIR%\examples" mkdir "%TARGET_DIR%\examples"

echo [2/4] Copying C& compiler binary and standard libraries...
copy /Y "cand.exe" "%TARGET_DIR%\cand.exe" >nul
copy /Y "cand.toml" "%TARGET_DIR%\cand.toml" >nul
xcopy /Y /S /E "std\*" "%TARGET_DIR%\std\" >nul
xcopy /Y /S /E "examples\*" "%TARGET_DIR%\examples\" >nul

echo [3/4] Registering C& (%TARGET_DIR%) to User PATH environment variable...
powershell -NoProfile -Command "$oldPath = [Environment]::GetEnvironmentVariable('Path', 'User'); if ($oldPath -notlike '*CandLanguage*') { [Environment]::SetEnvironmentVariable('Path', $oldPath + ';%TARGET_DIR%', 'User'); Write-Host '[SUCCESS] Added to PATH!' } else { Write-Host '[INFO] Path already exists.' }"

echo [4/4] Verifying installation...
if exist "%TARGET_DIR%\cand.exe" (
    echo.
    echo =========================================================================
    echo   [SUCCESS] C& Programming Language installed successfully!
    echo   Location: %TARGET_DIR%
    echo.
    echo   You can now open any new CMD or PowerShell window and run:
    echo     cand build main.cand
    echo     cand run main.cand
    echo     cand test
    echo =========================================================================
) else (
    echo [ERROR] Installation failed!
)

pause
