; NSIS Config (http://nsis.sf.net)
; for Win64 drivers
;
; Run it with
;
;    .../makensis [-DWITH_SQLITE_DLLS] this-file.nsi
;
; to create the installer sqliteodbc_w64.exe
;
; If -DWITH_SQLITE_DLLS is specified, separate SQLite DLLs
; are packaged which allows to exchange these independently
; of the ODBC drivers in the Win64 system folder.

; -------------------------------
; Start

BrandingText " "
!ifdef WITH_SEE
Name "SQLite3 ODBC Driver (SEE) for Win64"
!else
Name "SQLite3 ODBC Driver for Win64"
!endif

!ifdef WITH_SEE
!define PROD_NAME  "SQLite ODBC Driver (SEE) for Win64"
!define PROD_NAME0 "SQLite ODBC Driver (SEE) for Win64"
!else
!define PROD_NAME  "SQLite ODBC Driver for Win64"
!define PROD_NAME0 "SQLite ODBC Driver for Win64"
!endif
CRCCheck On
!include "MUI.nsh"
!include "Sections.nsh"
 
;--------------------------------
; General
 
OutFile "sqliteodbc_w64.exe"
 
;--------------------------------
; Folder selection page
 
InstallDir "$PROGRAMFILES64\${PROD_NAME0}"
 
;--------------------------------
; Modern UI Configuration

!define MUI_ICON "sqliteodbc.ico"
!define MUI_UNICON "sqliteodbc.ico" 
!define MUI_WELCOMEPAGE_TITLE "SQLite ODBC for Win64 Installation"
!define MUI_WELCOMEPAGE_TEXT "This program will guide you through the \
installation of SQLite ODBC Driver.\r\n\r\n$_CLICK"
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "license.txt"
!insertmacro MUI_PAGE_DIRECTORY
!ifndef WITHOUT_SQLITE2
!insertmacro MUI_PAGE_COMPONENTS
!endif
!insertmacro MUI_PAGE_INSTFILES

!define MUI_FINISHPAGE_TITLE "SQLite ODBC for Win64 Installation"  
!define MUI_FINISHPAGE_TEXT "The installation of SQLite ODBC Driver is complete.\
\r\n\r\n$_CLICK"

!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
 
;--------------------------------
; Language
 
!insertmacro MUI_LANGUAGE "English"
 
;--------------------------------
; Installer Sections

Section "-Main (required)" InstallationInfo
 
; Add files
 SetOutPath "$INSTDIR"
!ifdef WITH_SEE
 File "sqlite3odbc${WITH_SEE}.dll"
!else
 File "sqlite3odbc.dll"
!endif
; unsupported non-WCHAR driver for SQLite3
!ifdef WITH_SEE
 File "sqlite3odbc${WITH_SEE}nw.dll"
!else
 File "sqlite3odbcnw.dll"
!endif
!ifndef WITHOUT_SQLITE3_EXE
 File "sqlite3.exe"
!endif
 File "inst.exe"
 File "instq.exe"
 File "uninst.exe"
 File "uninstq.exe"
 File "adddsn.exe"
 File "remdsn.exe"
 File "addsysdsn.exe"
 File "remsysdsn.exe"
 File "sqlite3_mod_fts3.dll"
 File "sqlite3_mod_blobtoxy.dll"
 File "sqlite3_mod_impexp.dll"
 File "sqlite3_mod_csvtable.dll"
 File "sqlite3_mod_rtree.dll"
 File "sqlite3_mod_extfunc.dll"
 File "sqlite3_mod_zipfile.dll"
 File "license.terms"
 File "license.txt"
 File "README"
 File "readme.txt"
!ifdef WITH_SQLITE_DLLS
 File "sqlite3.dll"
!endif

; Shortcuts
 SetOutPath "$SMPROGRAMS\${PROD_NAME0}"
 CreateShortCut "$SMPROGRAMS\${PROD_NAME0}\Re-install ODBC Drivers.lnk" \
   "$INSTDIR\inst.exe"
 CreateShortCut "$SMPROGRAMS\${PROD_NAME0}\Remove ODBC Drivers.lnk" \
   "$INSTDIR\uninst.exe"
 CreateShortCut "$SMPROGRAMS\${PROD_NAME0}\Uninstall.lnk" \
   "$INSTDIR\uninstall.exe"
 CreateShortCut "$SMPROGRAMS\${PROD_NAME0}\View README.lnk" \
   "$INSTDIR\readme.txt"
 SetOutPath "$SMPROGRAMS\${PROD_NAME0}\Shells"
!ifndef WITHOUT_SQLITE3_EXE
 CreateShortCut "$SMPROGRAMS\${PROD_NAME0}\Shells\SQLite 3.lnk" \
   "$INSTDIR\sqlite3.exe"
!endif
 
; Write uninstall information to the registry
 WriteRegStr HKLM \
  "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PROD_NAME0}" \
  "DisplayName" "${PROD_NAME} (remove only)"
 WriteRegStr HKLM \
  "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PROD_NAME0}" \
  "UninstallString" "$INSTDIR\Uninstall.exe"

 SetOutPath "$INSTDIR"
 WriteUninstaller "$INSTDIR\Uninstall.exe"

 ExecWait '"$INSTDIR\instq.exe"'

SectionEnd

!ifndef WITHOUT_SQLITE2
Section /o "SQLite 2 Drivers" Sqlite2Install
 SetOutPath "$INSTDIR"
 File "sqliteodbc.dll"
 File "sqliteodbcu.dll"
 File "sqlite.exe"
 File "sqliteu.exe"
!ifdef WITH_SQLITE_DLLS
 File "sqlite.dll"
 File "sqliteu.dll"
!endif

 CreateShortCut "$SMPROGRAMS\${PROD_NAME0}\Shells\SQLite 2.lnk" \
   "$INSTDIR\sqlite.exe"
 CreateShortCut "$SMPROGRAMS\${PROD_NAME0}\Shells\SQLite 2 (UTF-8).lnk" \
   "$INSTDIR\sqliteu.exe"

 ExecWait '"$INSTDIR\instq.exe"'
SectionEnd
!endif

;--------------------------------
; Uninstaller Section

Section "Uninstall"

ExecWait '"$INSTDIR\uninstq.exe"'
 
; Delete Files 
RMDir /r "$INSTDIR\*" 
RMDir /r "$INSTDIR\*.*" 
 
; Remove the installation directory
RMDir /r "$INSTDIR"

; Remove start menu/program files subdirectory

RMDir /r "$SMPROGRAMS\${PROD_NAME0}"
  
; Delete Uninstaller And Unistall Registry Entries
DeleteRegKey HKEY_LOCAL_MACHINE "SOFTWARE\${PROD_NAME0}"
DeleteRegKey HKEY_LOCAL_MACHINE \
    "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\${PROD_NAME0}"
  
SectionEnd
 
;--------------------------------
; EOF
