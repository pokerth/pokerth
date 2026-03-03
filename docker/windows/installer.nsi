; PokerTH Windows Installer Script
; NSIS Modern User Interface

!include "MUI2.nsh"
!include "FileFunc.nsh"

; --------------------------------
; General Configuration

!define PRODUCT_NAME "PokerTH"
!define PRODUCT_VERSION "2.0.6"
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

; Welcome page - multilingual via LangStrings
!define MUI_WELCOMEPAGE_TITLE $(WelcomeTitle)
!define MUI_WELCOMEPAGE_TEXT $(WelcomeText)

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
; Language Selection Settings (remember choice in registry)

!define MUI_LANGDLL_REGISTRY_ROOT "HKLM"
!define MUI_LANGDLL_REGISTRY_KEY "${PRODUCT_UNINST_KEY}"
!define MUI_LANGDLL_REGISTRY_VALUENAME "InstallerLanguage"

; --------------------------------
; Languages (English first = default fallback)

!insertmacro MUI_LANGUAGE "English"
!insertmacro MUI_LANGUAGE "German"

; --------------------------------
; Multilingual Strings

LangString WelcomeTitle ${LANG_ENGLISH} "Welcome to ${PRODUCT_NAME} Setup"
LangString WelcomeTitle ${LANG_GERMAN} "Willkommen beim ${PRODUCT_NAME} Setup"

LangString WelcomeText ${LANG_ENGLISH} "This wizard will guide you through the installation of ${PRODUCT_NAME} ${PRODUCT_VERSION}.$\r$\n$\r$\n${PRODUCT_NAME} is a professional Texas Hold'em Poker Simulator.$\r$\n$\r$\nClick Next to continue."
LangString WelcomeText ${LANG_GERMAN} "Dieser Assistent wird Sie durch die Installation von ${PRODUCT_NAME} ${PRODUCT_VERSION} führen.$\r$\n$\r$\n${PRODUCT_NAME} ist ein professioneller Texas Hold'em Poker Simulator.$\r$\n$\r$\nKlicken Sie auf Weiter, um fortzufahren."

LangString SecMainName ${LANG_ENGLISH} "Main Program"
LangString SecMainName ${LANG_GERMAN} "Hauptprogramm"

LangString AlreadyInstalled ${LANG_ENGLISH} "${PRODUCT_NAME} is already installed. $\n$\nClick 'OK' to uninstall the previous version, or 'Cancel' to cancel the installation."
LangString AlreadyInstalled ${LANG_GERMAN} "${PRODUCT_NAME} ist bereits installiert. $\n$\nKlicken Sie auf 'OK', um die vorherige Version zu deinstallieren, oder auf 'Abbrechen', um die Installation abzubrechen."

LangString ReadmeThanks ${LANG_ENGLISH} "Thank you for installing PokerTH!"
LangString ReadmeThanks ${LANG_GERMAN} "Vielen Dank für die Installation von PokerTH!"

LangString ReadmeDesc ${LANG_ENGLISH} "PokerTH is a professional Texas Hold'em Poker Simulator."
LangString ReadmeDesc ${LANG_GERMAN} "PokerTH ist ein professioneller Texas Hold'em Poker Simulator."

LangString ReadmeStart ${LANG_ENGLISH} "To start the game, use the Start Menu entry$\r$\nor the desktop shortcut."
LangString ReadmeStart ${LANG_GERMAN} "Um das Spiel zu starten, verwenden Sie den Startmenü-Eintrag$\r$\noder das Desktop-Symbol."

LangString ReadmeEnjoy ${LANG_ENGLISH} "Enjoy playing!"
LangString ReadmeEnjoy ${LANG_GERMAN} "Viel Spaß beim Spielen!"

; --------------------------------
; Installer Sections

Section "Main Program" SecMain
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
  
  ; Create README (multilingual)
  FileOpen $0 "$INSTDIR\README.txt" w
  FileWrite $0 "${PRODUCT_NAME} ${PRODUCT_VERSION}$\r$\n"
  FileWrite $0 "==============================$\r$\n$\r$\n"
  FileWrite $0 "$(ReadmeThanks)$\r$\n$\r$\n"
  FileWrite $0 "$(ReadmeDesc)$\r$\n$\r$\n"
  FileWrite $0 "Website: ${PRODUCT_WEB_SITE}$\r$\n$\r$\n"
  FileWrite $0 "$(ReadmeStart)$\r$\n$\r$\n"
  FileWrite $0 "$(ReadmeEnjoy)$\r$\n"
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
  ; Show language selection dialog (auto-detects system language)
  !insertmacro MUI_LANGDLL_DISPLAY

  ; Set section name to match selected language
  SectionSetText ${SecMain} $(SecMainName)

  ; Check if already installed
  ReadRegStr $R0 ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString"
  StrCmp $R0 "" done
  
  MessageBox MB_OKCANCEL|MB_ICONEXCLAMATION \
  $(AlreadyInstalled) \
  IDOK uninst
  Abort
  
uninst:
  ClearErrors
  ExecWait '$R0 _?=$INSTDIR'
  
done:
FunctionEnd

Function un.onInit
  ; Restore language selection for uninstaller
  !insertmacro MUI_UNGETLANGUAGE
FunctionEnd
