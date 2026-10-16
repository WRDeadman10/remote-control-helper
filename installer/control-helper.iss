; Inno Setup script for the LAN Camera Remote Desktop Control Helper
; Output: control-helper-setup.exe (standalone native helper installer)

#ifndef AppVersion
  #define AppVersion "1.0.0"
#endif
#ifndef BuildDir
  #define BuildDir "..\build\Release"
#endif
#ifndef OutDir
  #define OutDir "..\build\installer"
#endif

[Setup]
AppId={{E68A91F2-4C3D-4A7F-8B52-92D0315891CD}}
AppName=LAN Camera Remote Desktop Control Helper
AppVersion={#AppVersion}
AppVerName=LAN Camera Remote Desktop Control Helper {#AppVersion}
AppPublisher=LAN Camera Streaming
DefaultDirName={autopf}\LanCameraRemoteControl
DisableDirPage=yes
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0
OutputDir={#OutDir}
OutputBaseFilename=control-helper-setup
Compression=lzma2/ultra
SolidCompression=yes
WizardStyle=modern
UninstallDisplayName=LAN Camera Remote Desktop Control Helper
CloseApplications=no
RestartIfNeededByRun=no

[Registry]
; Register lancam-control:// protocol handler in HKCU (runs without admin elevation)
Root: HKCU; Subkey: "Software\Classes\lancam-control"; ValueType: string; ValueName: ""; ValueData: "URL:LAN Camera Remote Control Protocol"; Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\Classes\lancam-control"; ValueType: string; ValueName: "URL Protocol"; ValueData: ""; Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\Classes\lancam-control\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\remote-control-helper.exe"" ""%1"""; Flags: uninsdeletekey

[Files]
Source: "{#BuildDir}\remote-control-helper.exe"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{autoprograms}\LAN Camera Remote Control Helper"; Filename: "{app}\remote-control-helper.exe"

[Run]
Filename: "{app}\remote-control-helper.exe"; Parameters: "--register"; Flags: runhidden waituntilterminated
Filename: "{app}\remote-control-helper.exe"; Description: "Launch Remote Desktop Control Helper"; Flags: nowait postinstall skipifsilent
