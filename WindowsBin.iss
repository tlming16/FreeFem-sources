; Installation of FreeFem++ on Microsoft Windows with Inno Setup
; $Id$

[Setup]
AppName=FreeFem++
AppVerName=FreeFem++ version 1.41
DefaultDirName={pf}\FreeFem++
DefaultGroupName=FreeFem++
Compression=lzma
SolidCompression=yes
ChangesAssociations=yes
OutputBaseFilename=FreeFem++-bin

[Files]
Source: "src\std\FreeFem++.exe"; DestDir: "{app}"
Source: "src\nw\FreeFem++-nw.exe"; DestDir: "{app}"
Source: "examples++"; DestDir: "{app}"
Source: "examples++-eigen"; DestDir: "{app}"
Source: "examples++-tutorial"; DestDir: "{app}"
Source: "DOC\manual.pdf"; DestDir: "{app}"

[Icons]

; Menu
Name: "{group}\FreeFem++"; Filename: "{app}\src\std\FreeFem++.exe"; IconFilename: "{app}\logo.ico"
Name: "{group}\PDF manual"; Filename: "{app}\manual.pdf"
Name: "{group}\Tutorial Examples"; Filename: "{app}\examples++-tutorial"
Name: "{group}\Examples"; Filename: "{app}\examples++"
Name: "{group}\Eigenvalues Examples"; Filename: "{app}\examples++-eigen"
Name: "{group}\Uninstall FreeFem++"; Filename: "{uninstallexe}"

; Desktop
Name: "{userdesktop}\FreeFem++"; Filename: "{app}\src\std\FreeFem++.exe"; IconFilename: "{app}\logo.ico"

[Registry]

; Link .edp file extension to FreeFem++
Root: HKCR; Subkey: ".edp"; ValueType: string; ValueName: ""; ValueData: "FreeFemScript"; Flags: uninsdeletevalue
Root: HKCR; Subkey: "FreeFemScript"; ValueType: string; ValueName: ""; ValueData: "FreeFem++ Script"; Flags: uninsdeletekey
Root: HKCR; Subkey: "FreeFemScript\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\logo.ico"
Root: HKCR; Subkey: "FreeFemScript\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\src\std\FreeFem++.exe"" ""%1"""

