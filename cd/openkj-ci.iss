; Inno Setup script used by CI to build the self-installing Windows package.
;
; Kept separate from openkj64.iss (the upstream hand-maintained script), which
; expects fonts and a vc_redist that are not in this repository and so cannot
; compile unattended.
;
; Driven entirely by /D defines from the workflow:
;   iscc /DMyAppVersion=3.0.0 /DPayloadDir=..\output /DOutDir=..\dist cd\openkj-ci.iss

#ifndef MyAppVersion
  #define MyAppVersion "3.0.0"
#endif
#ifndef PayloadDir
  #define PayloadDir "..\output"
#endif
#ifndef OutDir
  #define OutDir "..\dist"
#endif

#define MyAppName "OpenKJ"
#define MyAppPublisher "OpenKJ Community"
#define MyAppURL "https://github.com/pkircher29/OpenKJ"
#define MyAppExeName "OpenKJ.exe"
; Same AppId as the upstream installer so this upgrades an existing OpenKJ
; in place rather than leaving two entries in Add/Remove Programs.
#define MyAppId "{474EEC43-B55A-4FCE-8E5A-4ACD90E56103}"

[Setup]
AppId={{#MyAppId}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
; {autopf} resolves to Program Files for an admin install and the user's
; local app data otherwise; {pf} is deprecated and 32-bit-biased.
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
LicenseFile={#PayloadDir}\LICENSE.txt
OutputDir={#OutDir}
OutputBaseFilename=OpenKJ-{#MyAppVersion}-Windows-x64-setup
Compression=lzma2/max
SolidCompression=yes
UninstallDisplayName={#MyAppName} {#MyAppVersion}
UninstallDisplayIcon={app}\{#MyAppExeName}
; This payload is 64-bit only.
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
WizardStyle=modern
PrivilegesRequiredOverridesAllowed=dialog

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "{#PayloadDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\Uninstall {#MyAppName}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
; The MSVC runtime ships alongside; installing it quietly is a no-op when a
; newer one is already present.
Filename: "{app}\vc_redist.x64.exe"; Parameters: "/install /quiet /norestart"; StatusMsg: "Installing Microsoft Visual C++ runtime..."; Flags: waituntilterminated; Check: VCRedistPresent
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent

[Code]
function VCRedistPresent: Boolean;
begin
  Result := FileExists(ExpandConstant('{app}\vc_redist.x64.exe'));
end;
