; Script generated by the HM NIS Edit Script Wizard.

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;                                                                      ;
; GLE - Graphics Layout Engine <http://www.gle-graphics.org/>          ;
;                                                                      ;
; Modified BSD License                                                 ;
;                                                                      ;
; Copyright (C) 2009 GLE.                                              ;
;                                                                      ;
; Redistribution and use in source and binary forms, with or without   ;
; modification, are permitted provided that the following conditions   ;
; are met:                                                             ;
;                                                                      ;
;    1. Redistributions of source code must retain the above copyright ;
; notice, this list of conditions and the following disclaimer.        ;
;                                                                      ;
;    2. Redistributions in binary form must reproduce the above        ;
; copyright notice, this list of conditions and the following          ;
; disclaimer in the documentation and/or other materials provided with ;
; the distribution.                                                    ;
;                                                                      ;
;    3. The name of the author may not be used to endorse or promote   ;
; products derived from this software without specific prior written   ;
; permission.                                                          ;
;                                                                      ;
; THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR   ;
; IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED       ;
; WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE   ;
; ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY       ;
; DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL   ;
; DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE    ;
; GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS        ;
; INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER ;
; IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR      ;
; OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN  ;
; IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.                        ;
;                                                                      ;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; HM NIS Edit Wizard helper defines
!define PRODUCT_NAME "GLE"
!define PRODUCT_VERSION "${GLE_VERSION}"
!define PRODUCT_PUBLISHER "GLE"
!define PRODUCT_WEB_SITE "http://www.gle-graphics.org/"
!define PRODUCT_DIR_REGKEY "Software\Microsoft\Windows\CurrentVersion\App Paths\gle.exe"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_UNINST_ROOT_KEY "HKLM"

SetCompressor lzma

; MUI 1.67 compatible ------
!include "MUI.nsh"
!include "AddToPath.nsi"

; MUI Settings
!define MUI_ABORTWARNING
!define MUI_ICON "${NSISDIR}\Contrib\Graphics\Icons\modern-install.ico"
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\modern-uninstall.ico"
!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_RIGHT
!define MUI_HEADERIMAGE_BITMAP "banner_image.bmp"
!define MUI_HEADERIMAGE_BITMAP_NOSTRETCH
!define MUI_WELCOMEFINISHPAGE_BITMAP_NOSTRETCH
!define MUI_WELCOMEFINISHPAGE_BITMAP "welcome_image.bmp"

; Welcome page
!insertmacro MUI_PAGE_WELCOME
; License page
!insertmacro MUI_PAGE_LICENSE "..\..\..\LICENSE.txt"
; Directory page
!insertmacro MUI_PAGE_DIRECTORY
; Instfiles page
!insertmacro MUI_PAGE_INSTFILES
; Finish page
!define MUI_FINISHPAGE_SHOWREADME "$INSTDIR\readme-win32.txt"
!insertmacro MUI_PAGE_FINISH

; Uninstaller pages
!insertmacro MUI_UNPAGE_INSTFILES

; Language files
!insertmacro MUI_LANGUAGE "English"

; Reserve files
!insertmacro MUI_RESERVEFILE_INSTALLOPTIONS

; MUI end ------

Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
OutFile "${OUTPUT_FILENAME}"
InstallDir "$PROGRAMFILES\Gle4"
InstallDirRegKey HKLM "${PRODUCT_DIR_REGKEY}" ""
ShowInstDetails show
ShowUnInstDetails show
RequestExecutionLevel admin

!define $GLEINST 0

Section "MainSection" SEC01
  SetShellVarContext all
  ; Previous installation of GLE found?
  ReadRegStr $0 HKLM "${PRODUCT_DIR_REGKEY}" ""
  StrCmp $0 "" GLENotRegistered
     CopyFiles /silent "$0/glerc" "$INSTDIR\glerc.old"
  GLENotRegistered:
  ; Is previous installation of GLE in same directory as currunt one?
  Push 0
  Pop $0
  IfFileExists $INSTDIR\glerc 0 GLENotExists
    IntOp $0 $0 + 1
  ; Start with the installation
  GLENotExists:
  SetOutPath "$INSTDIR\bin"
  SetOverwrite try
  File "..\..\..\build\bin\*.*"
  File /nonfatal "C:\cygwin\mingw\bin\mingwm10.dll"
  File "C:\Program Files\Debugging Tools for Windows (x86)\dbghelp.dll"
  SetOutPath "$INSTDIR\font"
  File /r "..\..\..\build\font\*.*"
  SetOutPath "$INSTDIR\doc"
  File "..\..\..\build\doc\*.*"  
  SetOutPath "$INSTDIR\gleinc"
  File /r "..\..\..\build\gleinc\*.*"
  SetOutPath "$INSTDIR\samples"
  File "..\..\..\..\gle-www\examples\2dsimple\*.gle"
  File "..\..\..\..\gle-www\examples\2dsimple\*.dat"
  File "..\..\..\..\gle-www\examples\2dsimple\*.csv"
  File "..\..\..\..\gle-www\examples\diagrams\shapes.gle"
  File "..\..\..\src\samples\qgle_keyboard.gle"
  SetOutPath "$INSTDIR"
  File "..\..\..\build\glerc"
  File "..\..\..\build\inittex.ini"
  File "..\..\..\build\init.tex"  
  File "..\..\..\README*.*"
  File "..\..\..\LICENSE*.*"
  File "www.ico"
  File /oname=readme-win32.txt "..\..\..\platform\readme.win32"
  SetShellVarContext current
  CreateDirectory "$DOCUMENTS\GLE"
  CopyFiles "$INSTDIR\samples" "$DOCUMENTS\GLE\samples"
  SetOutPath "$DOCUMENTS\GLE\samples"
  CreateDirectory "$SMPROGRAMS\GLE"
  CreateShortCut "$SMPROGRAMS\GLE\Command Prompt.lnk" "%SystemRoot%\system32\cmd.exe"
  CreateShortCut "$SMPROGRAMS\GLE\GLE Previewer.lnk" "$INSTDIR\bin\qgle.exe"  
  SetShellVarContext all
  SetOutPath "%SystemRoot%"
  CreateDirectory "$SMPROGRAMS\GLE"
  CreateShortCut "$SMPROGRAMS\GLE\Command Prompt.lnk" "%SystemRoot%\system32\cmd.exe"
  CreateShortCut "$SMPROGRAMS\GLE\GLE Previewer.lnk" "$INSTDIR\bin\qgle.exe"
  SetOutPath "$INSTDIR"
  IntCmp $0 0 NotYetInst DoneIsInst DoneIsInst
  NotYetInst:
    Push "$INSTDIR\bin"
    Call AddToPath
    MessageBox MB_OK "The directory $INSTDIR\bin has been added to your system path."
  DoneIsInst:
  Exec "$INSTDIR\bin\qgle.exe -finddeps '$INSTDIR\glerc.old'"
  !define Index "Line${__LINE__}"
  ReadRegStr $1 HKCR ".gle" ""
  StrCmp $1 "" "${Index}-NoBackup"
  StrCmp $1 "GLEFile" "${Index}-NoBackup"
    WriteRegStr HKCR ".gle" "backup_val" $1
"${Index}-NoBackup:"
  WriteRegStr HKCR ".gle" "" "GLEFile"
  WriteRegStr HKCR "GLEFile" "" "GLE Script"
  WriteRegStr HKCR "GLEFile\DefaultIcon" "" "$INSTDIR\bin\qgle.exe,0"
  WriteRegStr HKCR "GLEFile\shell\open\command" "" '$INSTDIR\bin\qgle.exe "%1"'
  WriteRegStr HKCR "GLEFile\shell\edit" "" "Edit GLE Script"
  WriteRegStr HKCR "GLEFile\shell\edit\command" "" '$INSTDIR\bin\qgle.exe "%1"'
  System::Call 'Shell32::SHChangeNotify(i 0x8000000, i 0, i 0, i 0)'
  !undef Index
SectionEnd

Section -AdditionalIcons
  WriteIniStr "$INSTDIR\${PRODUCT_NAME}.url" "InternetShortcut" "URL" "${PRODUCT_WEB_SITE}"
  WriteIniStr "$INSTDIR\Tutorial.url" "InternetShortcut" "URL" "http://www.gle-graphics.org/tut/windows.html"
  CreateShortCut "$SMPROGRAMS\GLE\Readme.lnk" "$INSTDIR\readme-win32.txt"
  CreateShortCut "$SMPROGRAMS\GLE\Website.lnk" "$INSTDIR\${PRODUCT_NAME}.url" "" "$INSTDIR\www.ico"
  CreateShortCut "$SMPROGRAMS\GLE\Tutorial.lnk" "$INSTDIR\Tutorial.url" "" "$INSTDIR\www.ico"
  CreateShortCut "$SMPROGRAMS\GLE\Uninstall.lnk" "$INSTDIR\uninst.exe"
SectionEnd

Section -Post
  WriteUninstaller "$INSTDIR\uninst.exe"
  WriteRegStr HKLM "${PRODUCT_DIR_REGKEY}" "" "$INSTDIR"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayName" "$(^Name)"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\uninst.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
SectionEnd

Function un.onUninstSuccess
  HideWindow
  MessageBox MB_ICONINFORMATION|MB_OK "$(^Name) was successfully removed from your computer."
FunctionEnd

Function un.onInit
  MessageBox MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2 "Are you sure you want to completely remove $(^Name) and all of its components?" IDYES +2
  Abort
FunctionEnd

Section Uninstall
  Delete "$INSTDIR\*.url"
  Delete "$INSTDIR\*.ico"
  Delete "$INSTDIR\uninst.exe"
  Delete "$INSTDIR\glerc"
  Delete "$INSTDIR\README*.*"
  Delete "$INSTDIR\LICENSE*.*"
  Delete "$INSTDIR\inittex.ini"
  Delete "$INSTDIR\init.tex"  
  Delete "$INSTDIR\font\*.*"
  Delete "$INSTDIR\samples\*.*"
  Delete "$INSTDIR\lib\*.*"
  Delete "$INSTDIR\gleinc\*.*"
  Delete "$INSTDIR\gleinc\contrib\*.*"
  Delete "$INSTDIR\bin\*.*"

  Delete "$SMPROGRAMS\GLE\Uninstall.lnk"
  Delete "$SMPROGRAMS\GLE\Website.lnk"
  Delete "$SMPROGRAMS\GLE\Tutorial.lnk"
  Delete "$SMPROGRAMS\GLE\Readme.lnk"
  Delete "$SMPROGRAMS\GLE\GLE Previewer.lnk"
  Delete "$SMPROGRAMS\GLE\Command Prompt.lnk"

  RMDir "$SMPROGRAMS\GLE"
  RMDir "$INSTDIR\font"
  RMDir "$INSTDIR\bin"
  RMDir "$INSTDIR\samples"
  RMDir "$INSTDIR\lib"
  RMDir "$INSTDIR\gleinc\contrib"
  RMDir "$INSTDIR\gleinc"
  RMDir "$INSTDIR"

  DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}"
  DeleteRegKey HKLM "${PRODUCT_DIR_REGKEY}"

  Push "$INSTDIR\bin"
  Call un.RemoveFromPath

  SetAutoClose true
SectionEnd
