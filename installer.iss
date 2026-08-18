; Inno Setup Compiler Script for C& Programming Language Toolchain
; Script Version: 1.0.0
; Windows Installer for C& Compiler and Standard Libraries

#define MyAppName "C& Programming Language"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "C& Engine Core Team"
#define MyAppURL "https://github.com/mszstudio/Cand"
#define MyAppExeName "cand.exe"

[Setup]
AppId={{C8E2D77A-93B4-4B22-9F1F-C3D4E5F6A7B8}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={autopf}\CandLanguage
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
OutputBaseFilename=CandLanguage_Setup_v1.0.0
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
ArchitecturesInstallIn64BitMode=x64

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "addtopath"; Description: "Add C& Language compiler (cand.exe) to User PATH environment variable"; Flags: checkedonce
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; Flags: unchecked

[Files]
Source: "cand.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "cand.toml"; DestDir: "{app}"; Flags: ignoreversion
Source: "std\*"; DestDir: "{app}\std"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "examples\*"; DestDir: "{app}\examples"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "README.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "LANGUAGE.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "COMPILER.md"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\{#MyAppName} Documentation"; Filename: "{app}\README.md"
Name: "{group}\{#MyAppName} Language Guide"; Filename: "{app}\LANGUAGE.md"
Name: "{group}\Uninstall {#MyAppName}"; Filename: "{uninstallexe}"

[Registry]
; Automatically add C& bin path to User PATH
Root: HKCU; Subkey: "Environment"; ValueType: expandsz; ValueName: "Path"; ValueData: "{olddata};{app}"; Tasks: addtopath; Check: NeedsAddPath('{app}')

[Code]
function NeedsAddPath(Param: string): boolean;
var
  OrigPath: string;
begin
  if not RegQueryStringValue(HKEY_CURRENT_USER, 'Environment', 'Path', OrigPath) then
  begin
    Result := True;
    exit;
  end;
  Result := Pos(';' + UpperCase(Param) + ';', ';' + UpperCase(OrigPath) + ';') = 0;
  if Result then
    Result := Pos(';' + UpperCase(Param), ';' + UpperCase(OrigPath)) = 0;
end;
