; PokerTH Windows Installer Script
; NSIS Modern User Interface

!include "MUI2.nsh"
!include "FileFunc.nsh"

; --------------------------------
; General Configuration

!define PRODUCT_NAME "PokerTH"
!define PRODUCT_VERSION "2.0"
!define PRODUCT_PUBLISHER "PokerTH Team"
!define PRODUCT_WEB_SITE "http://www.pokerth.net"
!define PRODUCT_DIR_REGKEY "Software\Microsoft\Windows\CurrentVersion\App Paths\pokerth_client.exe"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_UNINST_ROOT_KEY "HKLM"

; Set compression
SetCompressor /SOLID lzma

; Name and file
Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
OutFile "PokerTH-${PRODUCT_VERSION}-Setup.exe"

; Default installation directory
InstallDir "$PROGRAMFILES64\PokerTH"

; Get installation folder from registry if available
InstallDirRegKey HKLM "${PRODUCT_DIR_REGKEY}" ""

; Request application privileges for Windows Vista and higher
RequestExecutionLevel admin

; --------------------------------
; Variables

Var StartMenuFolder

; --------------------------------
; Interface Settings

!define MUI_ABORTWARNING
!define MUI_ICON "pokerth.ico"
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\modern-uninstall.ico"

; Welcome page
!define MUI_WELCOMEPAGE_TITLE "Willkommen beim ${PRODUCT_NAME} Setup"
!define MUI_WELCOMEPAGE_TEXT "Dieser Assistent wird Sie durch die Installation von ${PRODUCT_NAME} ${PRODUCT_VERSION} führen.$\r$\n$\r$\n${PRODUCT_NAME} ist ein professioneller Texas Hold'em Poker Simulator.$\r$\n$\r$\nKlicken Sie auf Weiter, um fortzufahren."

; --------------------------------
; Pages

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "../../COPYING"
!insertmacro MUI_PAGE_DIRECTORY

; Start Menu Folder Page Configuration
!define MUI_STARTMENUPAGE_REGISTRY_ROOT "HKLM" 
!define MUI_STARTMENUPAGE_REGISTRY_KEY "${PRODUCT_UNINST_KEY}" 
!define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "StartMenuFolder"
!define MUI_STARTMENUPAGE_DEFAULTFOLDER "${PRODUCT_NAME}"

!insertmacro MUI_PAGE_STARTMENU Application $StartMenuFolder
!insertmacro MUI_PAGE_INSTFILES

; Finish page
!define MUI_FINISHPAGE_RUN "$INSTDIR\pokerth_client.exe"
!define MUI_FINISHPAGE_SHOWREADME "$INSTDIR\README.txt"
!insertmacro MUI_PAGE_FINISH

; Uninstaller pages
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

; --------------------------------
; Languages

!insertmacro MUI_LANGUAGE "German"
!insertmacro MUI_LANGUAGE "English"

; --------------------------------
; Installer Sections

Section "Hauptprogramm" SecMain
  SectionIn RO
  
  SetOutPath "$INSTDIR"
  
  ; Copy all files from deploy directory
  File /r "../../build/deploy\*.*"
  
  ; Store installation folder
  WriteRegStr HKLM "${PRODUCT_DIR_REGKEY}" "" "$INSTDIR\pokerth_client.exe"
  
  ; Create uninstaller
  WriteUninstaller "$INSTDIR\Uninstall.exe"
  
  ; Write registry keys for Add/Remove Programs
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayName" "${PRODUCT_NAME}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\Uninstall.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayIcon" "$INSTDIR\pokerth.ico"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
  
  ; Get installed size
  ${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
  IntFmt $0 "0x%08X" $0
  WriteRegDWORD ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "EstimatedSize" "$0"
  
  ; Create Start Menu shortcuts
  !insertmacro MUI_STARTMENU_WRITE_BEGIN Application
    CreateDirectory "$SMPROGRAMS\$StartMenuFolder"
    CreateShortcut "$SMPROGRAMS\$StartMenuFolder\${PRODUCT_NAME}.lnk" "$INSTDIR\pokerth_client.exe" "" "$INSTDIR\pokerth.ico" 0
    CreateShortcut "$SMPROGRAMS\$StartMenuFolder\Uninstall.lnk" "$INSTDIR\Uninstall.exe"
    
    ; Create shortcuts for other executables if they exist
    IfFileExists "$INSTDIR\pokerth_dedicated_server.exe" 0 +2
      CreateShortcut "$SMPROGRAMS\$StartMenuFolder\PokerTH Dedicated Server.lnk" "$INSTDIR\pokerth_dedicated_server.exe" "" "$INSTDIR\pokerth.ico" 0
    
  !insertmacro MUI_STARTMENU_WRITE_END
  
  ; Create Desktop shortcut
  CreateShortcut "$DESKTOP\${PRODUCT_NAME}.lnk" "$INSTDIR\pokerth_client.exe" "" "$INSTDIR\pokerth.ico" 0
  
  ; Create README
  FileOpen $0 "$INSTDIR\README.txt" w
  FileWrite $0 "${PRODUCT_NAME} ${PRODUCT_VERSION}$\r$\n"
  FileWrite $0 "==============================$\r$\n$\r$\n"
  FileWrite $0 "Vielen Dank für die Installation von PokerTH!$\r$\n$\r$\n"
  FileWrite $0 "PokerTH ist ein professioneller Texas Hold'em Poker Simulator.$\r$\n$\r$\n"
  FileWrite $0 "Website: ${PRODUCT_WEB_SITE}$\r$\n$\r$\n"
  FileWrite $0 "Um das Spiel zu starten, verwenden Sie den Startmenü-Eintrag$\r$\n"
  FileWrite $0 "oder das Desktop-Symbol.$\r$\n$\r$\n"
  FileWrite $0 "Viel Spaß beim Spielen!$\r$\n"
  FileClose $0
  
SectionEnd

Section "Visual C++ Redistributable" SecVCRedist
  ; This section would download and install VC++ Redistributable if needed
  ; For now, MinGW static linking makes this unnecessary
SectionEnd

; --------------------------------
; Descriptions

LangString DESC_SecMain ${LANG_GERMAN} "Installiert die Hauptdateien von ${PRODUCT_NAME}."
LangString DESC_SecMain ${LANG_ENGLISH} "Installs the main ${PRODUCT_NAME} files."

LangString DESC_SecVCRedist ${LANG_GERMAN} "Visual C++ Laufzeitbibliotheken (wird für MinGW nicht benötigt)."
LangString DESC_SecVCRedist ${LANG_ENGLISH} "Visual C++ Runtime Libraries (not needed for MinGW build)."

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SecMain} $(DESC_SecMain)
  !insertmacro MUI_DESCRIPTION_TEXT ${SecVCRedist} $(DESC_SecVCRedist)
!insertmacro MUI_FUNCTION_DESCRIPTION_END

; --------------------------------
; Uninstaller Section

Section "Uninstall"
  
  ; Remove Start Menu items
  !insertmacro MUI_STARTMENU_GETFOLDER Application $StartMenuFolder
  
  Delete "$SMPROGRAMS\$StartMenuFolder\${PRODUCT_NAME}.lnk"
  Delete "$SMPROGRAMS\$StartMenuFolder\PokerTH Dedicated Server.lnk"
  Delete "$SMPROGRAMS\$StartMenuFolder\Uninstall.lnk"
  RMDir "$SMPROGRAMS\$StartMenuFolder"
  
  ; Remove Desktop shortcut
  Delete "$DESKTOP\${PRODUCT_NAME}.lnk"
  
  ; Remove installation directory
  RMDir /r "$INSTDIR\data"
  RMDir /r "$INSTDIR\plugins"
  Delete "$INSTDIR\*.exe"
  Delete "$INSTDIR\*.dll"
  Delete "$INSTDIR\qt.conf"
  Delete "$INSTDIR\*.bat"
  Delete "$INSTDIR\*.sh"
  Delete "$INSTDIR\*.ico"
  Delete "$INSTDIR\README.txt"
  Delete "$INSTDIR\Uninstall.exe"
  RMDir "$INSTDIR"
  
  ; Remove registry keys
  DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}"
  DeleteRegKey HKLM "${PRODUCT_DIR_REGKEY}"
  
  SetAutoClose true
  
SectionEnd

; --------------------------------
; Installer Functions

Function .onInit
  ; Check if already installed
  ReadRegStr $R0 ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString"
  StrCmp $R0 "" done
  
  MessageBox MB_OKCANCEL|MB_ICONEXCLAMATION \
  "${PRODUCT_NAME} ist bereits installiert. $\n$\nKlicken Sie auf 'OK', um die vorherige Version zu deinstallieren, oder auf 'Abbrechen', um die Installation abzubrechen." \
  IDOK uninst
  Abort
  
uninst:
  ClearErrors
  ExecWait '$R0 _?=$INSTDIR'
  
done:
FunctionEnd
