; PokerTH Combined (Widget + QML) Windows Installer Script
; NSIS Modern User Interface
;
; Installiert BEIDE Clients in ein gemeinsames Verzeichnis:
;   - pokerth_client.exe       (klassischer Widget-Client)
;   - pokerth_qml-client.exe   (moderner Qt6/QML-Client)
; Beide teilen sich data/, plugins/, qml/ und die Qt6-DLLs im Deploy-Verzeichnis.

!include "MUI2.nsh"
!include "FileFunc.nsh"

; --------------------------------
; General Configuration

!define PRODUCT_NAME "PokerTH"
!define PRODUCT_VERSION "2.1.7"
!define PRODUCT_PUBLISHER "PokerTH Team"
!define PRODUCT_WEB_SITE "http://www.pokerth.net"
!define PRODUCT_EXE_WIDGET "pokerth_client.exe"
!define PRODUCT_EXE_QML "pokerth_qml-client.exe"
!define PRODUCT_DIR_REGKEY "Software\Microsoft\Windows\CurrentVersion\App Paths\${PRODUCT_EXE_QML}"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_UNINST_ROOT_KEY "HKLM"

; Set compression
SetCompressor /SOLID lzma

; Name and file
Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
!ifdef OUTFILE
  OutFile "${OUTFILE}"
!else
  OutFile "PokerTH-${PRODUCT_VERSION}-Setup.exe"
!endif

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

; Finish page - offer to start the modern QML client
!define MUI_FINISHPAGE_RUN "$INSTDIR\${PRODUCT_EXE_QML}"
!define MUI_FINISHPAGE_RUN_TEXT $(RunQmlText)
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

LangString WelcomeText ${LANG_ENGLISH} "This wizard will guide you through the installation of ${PRODUCT_NAME} ${PRODUCT_VERSION}.$\r$\n$\r$\nThis package contains BOTH clients:$\r$\n  - the classic widget client$\r$\n  - the modern Qt6/QML client$\r$\n$\r$\n${PRODUCT_NAME} is a professional Texas Hold'em Poker Simulator.$\r$\n$\r$\nClick Next to continue."
LangString WelcomeText ${LANG_GERMAN} "Dieser Assistent wird Sie durch die Installation von ${PRODUCT_NAME} ${PRODUCT_VERSION} führen.$\r$\n$\r$\nDieses Paket enthält BEIDE Clients:$\r$\n  - den klassischen Widget-Client$\r$\n  - den modernen Qt6/QML-Client$\r$\n$\r$\n${PRODUCT_NAME} ist ein professioneller Texas Hold'em Poker Simulator.$\r$\n$\r$\nKlicken Sie auf Weiter, um fortzufahren."

LangString SecMainName ${LANG_ENGLISH} "Main Program (Widget + QML)"
LangString SecMainName ${LANG_GERMAN} "Hauptprogramm (Widget + QML)"

LangString RunQmlText ${LANG_ENGLISH} "Run ${PRODUCT_NAME} (QML client)"
LangString RunQmlText ${LANG_GERMAN} "${PRODUCT_NAME} starten (QML-Client)"

LangString AlreadyInstalled ${LANG_ENGLISH} "${PRODUCT_NAME} is already installed. $\n$\nClick 'OK' to uninstall the previous version, or 'Cancel' to cancel the installation."
LangString AlreadyInstalled ${LANG_GERMAN} "${PRODUCT_NAME} ist bereits installiert. $\n$\nKlicken Sie auf 'OK', um die vorherige Version zu deinstallieren, oder auf 'Abbrechen', um die Installation abzubrechen."

LangString ReadmeThanks ${LANG_ENGLISH} "Thank you for installing PokerTH!"
LangString ReadmeThanks ${LANG_GERMAN} "Vielen Dank für die Installation von PokerTH!"

LangString ReadmeDesc ${LANG_ENGLISH} "This package installs both the classic widget client (pokerth_client.exe) and the modern Qt6/QML client (pokerth_qml-client.exe)."
LangString ReadmeDesc ${LANG_GERMAN} "Dieses Paket installiert sowohl den klassischen Widget-Client (pokerth_client.exe) als auch den modernen Qt6/QML-Client (pokerth_qml-client.exe)."

LangString ReadmeStart ${LANG_ENGLISH} "To start a client, use the Start Menu entries$\r$\nor the desktop shortcuts."
LangString ReadmeStart ${LANG_GERMAN} "Um einen Client zu starten, verwenden Sie die Startmenü-Einträge$\r$\noder die Desktop-Symbole."

LangString ReadmeEnjoy ${LANG_ENGLISH} "Enjoy playing!"
LangString ReadmeEnjoy ${LANG_GERMAN} "Viel Spaß beim Spielen!"

; --------------------------------
; Installer Sections

Section "Main Program" SecMain
  SectionIn RO

  SetOutPath "$INSTDIR"

  ; Copy all files from deploy directory (both exes + shared DLLs/data/plugins/qml)
  File /r "../../build/deploy\*.*"

  ; Store installation folder (points at the QML client as primary)
  WriteRegStr HKLM "${PRODUCT_DIR_REGKEY}" "" "$INSTDIR\${PRODUCT_EXE_QML}"

  ; Create uninstaller
  WriteUninstaller "$INSTDIR\Uninstall.exe"

  ; Write registry keys for Add/Remove Programs
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayName" "${PRODUCT_NAME} (Widget + QML)"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\Uninstall.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayIcon" "$INSTDIR\pokerth.ico"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"

  ; Get installed size
  ${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
  IntFmt $0 "0x%08X" $0
  WriteRegDWORD ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "EstimatedSize" "$0"

  ; Create Start Menu shortcuts (one per client + uninstaller)
  !insertmacro MUI_STARTMENU_WRITE_BEGIN Application
    CreateDirectory "$SMPROGRAMS\$StartMenuFolder"
    CreateShortcut "$SMPROGRAMS\$StartMenuFolder\${PRODUCT_NAME} (QML).lnk" \
                   "$INSTDIR\${PRODUCT_EXE_QML}" "" "$INSTDIR\pokerth.ico" 0
    CreateShortcut "$SMPROGRAMS\$StartMenuFolder\${PRODUCT_NAME} (Widget).lnk" \
                   "$INSTDIR\${PRODUCT_EXE_WIDGET}" "" "$INSTDIR\pokerth.ico" 0
    CreateShortcut "$SMPROGRAMS\$StartMenuFolder\Uninstall.lnk" "$INSTDIR\Uninstall.exe"

    ; Create shortcut for the dedicated server if it exists
    IfFileExists "$INSTDIR\pokerth_dedicated_server.exe" 0 +2
      CreateShortcut "$SMPROGRAMS\$StartMenuFolder\PokerTH Dedicated Server.lnk" "$INSTDIR\pokerth_dedicated_server.exe" "" "$INSTDIR\pokerth.ico" 0

  !insertmacro MUI_STARTMENU_WRITE_END

  ; Create Desktop shortcuts (one per client)
  CreateShortcut "$DESKTOP\${PRODUCT_NAME} (QML).lnk" \
                 "$INSTDIR\${PRODUCT_EXE_QML}" "" "$INSTDIR\pokerth.ico" 0
  CreateShortcut "$DESKTOP\${PRODUCT_NAME} (Widget).lnk" \
                 "$INSTDIR\${PRODUCT_EXE_WIDGET}" "" "$INSTDIR\pokerth.ico" 0

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

; --------------------------------
; Descriptions

LangString DESC_SecMain ${LANG_GERMAN} "Installiert beide ${PRODUCT_NAME}-Clients (Widget + QML) mit allen Abhängigkeiten."
LangString DESC_SecMain ${LANG_ENGLISH} "Installs both ${PRODUCT_NAME} clients (widget + QML) with all dependencies."

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SecMain} $(DESC_SecMain)
!insertmacro MUI_FUNCTION_DESCRIPTION_END

; --------------------------------
; Uninstaller Section

Section "Uninstall"

  ; Remove Start Menu items
  !insertmacro MUI_STARTMENU_GETFOLDER Application $StartMenuFolder

  Delete "$SMPROGRAMS\$StartMenuFolder\${PRODUCT_NAME} (QML).lnk"
  Delete "$SMPROGRAMS\$StartMenuFolder\${PRODUCT_NAME} (Widget).lnk"
  Delete "$SMPROGRAMS\$StartMenuFolder\PokerTH Dedicated Server.lnk"
  Delete "$SMPROGRAMS\$StartMenuFolder\Uninstall.lnk"
  RMDir "$SMPROGRAMS\$StartMenuFolder"

  ; Remove Desktop shortcuts
  Delete "$DESKTOP\${PRODUCT_NAME} (QML).lnk"
  Delete "$DESKTOP\${PRODUCT_NAME} (Widget).lnk"

  ; Remove installation directory
  RMDir /r "$INSTDIR\data"
  RMDir /r "$INSTDIR\plugins"
  RMDir /r "$INSTDIR\qml"
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
