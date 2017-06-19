; glurp.nsi
;
; Glurp installation script for NSIS installer compiler
; By: Daniel Lindenaar <daniel-glurp@lindenaar.org>

;--------------------------------

Function .onInit
  Push $R0
  ReadRegStr $R0 HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Glurp" "UninstallString"
  StrCmp $R0 "" Glurp_NOT_PRESENT
  MessageBox MB_OK "Glurp is already installed. Installation will be aborted."
  Pop $R0
  Abort
  Glurp_NOT_PRESENT:
  Pop $R0
FunctionEnd

; The name of the installer
Name "Glurp"

; The file to write
OutFile "glurp_win32_installer.exe"

; The default installation directory
InstallDir $PROGRAMFILES\glurp
InstallDirRegKey HKLM "Software\glurp" "InstallationDirectory"
;--------------------------------

; Pages

Page directory
Page instfiles
UninstPage uninstConfirm
UninstPage instfiles
;--------------------------------

; The stuff to install
Section "Install" 
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Glurp" "DisplayName" "Glurp simulation tool"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Glurp" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Glurp" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Glurp" "NoRepair" 1

  ; Set output path to the installation directory.
  SetOutPath $INSTDIR
  
  ; Put file there
  File src\glurp.exe
  SetOutPath $INSTDIR\data
  File pixmaps\*.png
  File glurp.glade

  WriteRegStr HKLM "Software\glurp" "InstallationDirectory" $INSTDIR
  WriteUninstaller "$INSTDIR\Uninstall.exe" 

SectionEnd ; end the section

Section "Uninstall"
  RMDir /r $INSTDIR
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Glurp"
  DeleteRegKey HKLM "Software\glurp" 
SectionEnd
