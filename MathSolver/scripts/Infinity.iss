; Build only on Windows after native Qt/CAS deployment has passed its tests.
; Command-line defines: AppVersion, BuildDir, OutputDir.
#ifndef AppVersion
  #error AppVersion must be supplied by package-windows.ps1
#endif
#ifndef BuildDir
  #error BuildDir must point to the deployed Windows application
#endif
#ifndef OutputDir
  #error OutputDir must point to the installer output folder
#endif
[Setup]
AppId={{4C5BC517-C9ED-4B32-A394-C230C17D3F39}
AppName=Infinity
AppVersion={#AppVersion}
AppPublisher=Lysandra
AppCopyright=Lysandra
DefaultDirName={localappdata}\Programs\Infinity
DefaultGroupName=Infinity
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0
OutputDir={#OutputDir}
OutputBaseFilename=Infinity-{#AppVersion}-Windows-x64-Setup
SetupIconFile=..\resources\icons\Infinity.ico
UninstallDisplayIcon={app}\Infinity.exe
LicenseFile=..\LICENSE
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; Flags: unchecked
[Files]
Source: "{#BuildDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
[Icons]
Name: "{group}\Infinity"; Filename: "{app}\Infinity.exe"; WorkingDir: "{app}"
Name: "{autodesktop}\Infinity"; Filename: "{app}\Infinity.exe"; WorkingDir: "{app}"; Tasks: desktopicon
Name: "{group}\Uninstall Infinity"; Filename: "{uninstallexe}"
[Run]
Filename: "{app}\Infinity.exe"; Description: "Launch Infinity"; Flags: nowait postinstall skipifsilent
