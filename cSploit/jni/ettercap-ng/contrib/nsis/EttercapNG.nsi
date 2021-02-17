;
;   ettercap -- NSIS script for the windows installer
;
;   Copyright (C) ALoR 
;
;   This program is free software; you can redistribute it and/or modify
;   it under the terms of the GNU General Public License as published by
;   the Free Software Foundation; either version 2 of the License, or
;   (at your option) any later version.
;
;   This program is distributed in the hope that it will be useful,
;   but WITHOUT ANY WARRANTY; without even the implied warranty of
;   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.   See the
;   GNU General Public License for more details.
;
;   You should have received a copy of the GNU General Public License
;   along with this program; if not, write to the Free Software
;   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
;
;   $Id: EttercapNG.nsi,v 1.10 2005/01/04 14:30:49 alor Exp $
;
; NOTE: this .NSI script is designed for NSIS v2.0+

;--------------------------------
;Version Informations

!define VER_MAJOR 0
!define VER_MINOR 7
!define VER_REVISION 3

!define VER_DISPLAY "0.7.3"

;--------------------------------
;Include Modern UI

   !include "MUI.nsh"

;--------------------------------
;General

   ;Name and file
   Name "Ettercap NG"
   Caption "Ettercap ${VER_DISPLAY} Setup"
   OutFile "ettercap-NG-${VER_DISPLAY}-win32.exe"

   ;Default installation folder
   InstallDir "$PROGRAMFILES\EttercapNG"

   ;Get installation folder from registry if available
   InstallDirRegKey HKCU "Software\ettercap_ng" ""

;--------------------------------
;Compiler settings

   SetCompressor lzma
   SetDatablockOptimize on 
   CRCCheck on 
   ShowInstDetails show ; (can be show to have them shown, or nevershow to disable)
   ShowUnInstDetails show ; (can be show to have them shown, or nevershow to disable)
   SetDateSave on ; (can be on to have files restored to their orginal date)

;--------------------------------
;Variables

   Var MUI_TEMP
   Var STARTMENU_FOLDER
   Var CHECKFAILED
   Var DOCUMENTATION
   ; window handlers
   Var NEXTBUTTON
   Var BACKBUTTON
   Var CANCELBUTTON

;--------------------------------
;Interface Configuration

   !define MUI_ICON "eNG.ico"
   !define MUI_HEADERIMAGE
   !define MUI_HEADERIMAGE_BITMAP "${NSISDIR}\Contrib\Graphics\Header\win.bmp"
   !define MUI_CUSTOMFUNCTION_ABORT UserAbort
   !define MUI_ABORTWARNING_TEXT "Do you want to exit the Ettercap NG setup ?"
   !define MUI_COMPONENTSPAGE_SMALLDESC

;--------------------------------
;Reserve Files

   ;These files should be inserted before other files in the data block

   ReserveFile "eNG-radiobuttons.ini"
   ReserveFile "eNG-message.ini"
   !insertmacro MUI_RESERVEFILE_INSTALLOPTIONS

;--------------------------------
;Pages

   ; welcome
   !define MUI_WELCOMEPAGE_TITLE "Welcome to the Ettercap NG ${VER_DISPLAY} Setup Wizard"
   !define MUI_WELCOMEPAGE_TEXT "This wizard will guide you through the installation of Ettercap NG ${VER_DISPLAY}, the next generation of the popular ettercap sniffer.\r\n\r\nEttercap NG includes a new Modern GTK User Interface, plugin support, support for multiple MITM attack at the same time and a more structured core.\r\n\r\n"
   !insertmacro MUI_PAGE_WELCOME

   ; check for the correct windows version
   Page custom CheckWindowsVersion

   ; license
   !insertmacro MUI_PAGE_LICENSE "LICENSE.txt"

   ; check for the presence of winpcap
   Page custom CheckWinpcap

   ; already installed, what to do ?
   Page custom PageReinstall PageLeaveReinstall

   ; components
   !insertmacro MUI_PAGE_COMPONENTS

   ; directory
   !insertmacro MUI_PAGE_DIRECTORY

   ; start menu
   !define MUI_STARTMENUPAGE_REGISTRY_ROOT "HKCU"
   !define MUI_STARTMENUPAGE_REGISTRY_KEY "Software\ettercap_ng"
   !define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "Start Menu Folder"
   !insertmacro MUI_PAGE_STARTMENU Application $STARTMENU_FOLDER

   ; install
   !insertmacro MUI_PAGE_INSTFILES

   ; finish
   !define MUI_FINISHPAGE_NOAUTOCLOSE
   !define MUI_FINISHPAGE_LINK "Visit the Ettercap website for the latest news, FAQs and support"
   !define MUI_FINISHPAGE_LINK_LOCATION "http://ettercap.sf.net/"

   !define MUI_FINISHPAGE_NOREBOOTSUPPORT
   !insertmacro MUI_PAGE_FINISH

   ; uninstall
   !insertmacro MUI_UNPAGE_CONFIRM
   !insertmacro MUI_UNPAGE_INSTFILES

;--------------------------------
;Languages

   !insertmacro MUI_LANGUAGE "English"



;--------------------------------
;Installer Sections

Section "Ettercap NG core" SecCore

   ; this is required, make it unchangeable by the user
   SectionIn RO

   SetOverwrite on

   SetOutPath "$INSTDIR"
   File ..\..\ettercap.exe
   File ..\..\*.dll
   
   SetOutPath "$INSTDIR\share"
   File ..\..\share\*
   
   SetOutPath "$INSTDIR\etc\pango"
   File ..\..\etc\pango\*

   SetOutPath "$INSTDIR\etc\gtk-2.0"
   File ..\..\etc\gtk-2.0\*

   SetOutPath "$INSTDIR\etc\fonts"
   File ..\..\etc\fonts\*

   SetOutPath "$INSTDIR\lib\pango\1.4.0\modules"
   File ..\..\lib\pango\1.4.0\modules\*

   SetOutPath "$INSTDIR\\lib\gtk-2.0\2.4.0\engines"
   File ..\..\lib\gtk-2.0\2.4.0\engines\*

   SetOutPath "$INSTDIR\\lib\gtk-2.0\2.4.0\loaders"
   File ..\..\lib\gtk-2.0\2.4.0\loaders\*

SectionEnd

Section "Etterlog" SecEtterlog

   SetOutPath "$INSTDIR"

   SetOverwrite on
   File ..\..\etterlog.exe

SectionEnd

Section "Etterfilter" SecEtterfilter

   SetOutPath "$INSTDIR"

   SetOverwrite on
   File ..\..\etterfilter.exe

SectionEnd

Section "Plugins" SecPlugins

   SetOutPath "$INSTDIR\lib"

   SetOverwrite on
   File ..\..\lib\*.dll


SectionEnd

Section "Documentation" SecDocs

   SetOutPath "$INSTDIR\doc"

   File ..\..\doc\*.pdf

   IntOp $DOCUMENTATION 0 + 1	; remember this for further use

SectionEnd

; Hidden section for the uninstaller
; it needs to be created whichever options the user choose
Section "-Make the uninstaller"

   SetOutPath "$INSTDIR"

   ;Store installation folder
   WriteRegStr HKLM "Software\ettercap_ng" "" $INSTDIR
   WriteRegDword HKLM "Software\ettercap_ng" "VersionMajor" "${VER_MAJOR}"
   WriteRegDword HKLM "Software\ettercap_ng" "VersionMinor" "${VER_MINOR}"
   WriteRegDword HKLM "Software\ettercap_ng" "VersionRevision" "${VER_REVISION}"

   ;Create uninstaller
   WriteUninstaller "$INSTDIR\Uninstall.exe"

   ; make the unisntall appear in the control panel "add/remove programs"
   WriteRegExpandStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\ettercap_ng" "UninstallString" '"$INSTDIR\uninstall.exe"'
   WriteRegExpandStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\ettercap_ng" "InstallLocation" "$INSTDIR"
   WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\ettercap_ng" "DisplayName" "Ettercap NG ${VER_DISPLAY}"
   WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\ettercap_ng" "DisplayIcon" "$INSTDIR\ettercap.exe,0"
   WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\ettercap_ng" "DisplayVersion" "${VER_DISPLAY}"
   WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\ettercap_ng" "VersionMajor" "${VER_MAJOR}"
   WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\ettercap_ng" "VersionMinor" "${VER_MINOR}"
   WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\ettercap_ng" "VersionRevision" "${VER_REVISION}"
   WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\ettercap_ng" "NoModify" "1"
   WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\ettercap_ng" "NoRepair" "1"
   WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\ettercap_ng" "URLInfoAbout" "http://ettercap.sourceforge.net"
   WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\ettercap_ng" "Publisher" "Ettercap developers"
   WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\ettercap_ng" "URLUpdateInfo" "http://ettercap.sf.net/download"

   ;start menu
   !insertmacro MUI_STARTMENU_WRITE_BEGIN Application

      ;Create shortcuts
      CreateDirectory "$SMPROGRAMS\$STARTMENU_FOLDER"
      CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\ettercap.lnk" "$INSTDIR\ettercap.exe" "-G"
      
      ; the command propmt to use etterfilter and etterlog
      ReadRegStr $R1 HKEY_LOCAL_MACHINE "SOFTWARE\Microsoft\Windows NT\CurrentVersion" "CurrentVersion"
      StrCpy $9 $R1 1
      StrCmp $9 '4' 0 win_2000_XP
      ; Windows NT 4.0
         CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\ettercap prompt.lnk" "$SYSDIR\command.com" "" 
         Goto end
      win_2000_XP:
         CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\ettercap prompt.lnk" "$SYSDIR\cmd.exe" "" 
      end:
      
      ; links to the documenation
      IntCmp $DOCUMENTATION 1 0 no_doc
      CreateDirectory "$SMPROGRAMS\$STARTMENU_FOLDER\docs"
         CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\docs\man-ettercap.lnk" "$INSTDIR\doc\ettercap.pdf" 
         CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\docs\man-ettercap_curses.lnk" "$INSTDIR\doc\ettercap_curses.pdf" 
         CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\docs\man-ettercap_plugins.lnk" "$INSTDIR\doc\ettercap_plugins.pdf" 
         CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\docs\man-etterfilter.lnk" "$INSTDIR\doc\etterfilter.pdf" 
         CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\docs\man-etterlog.lnk" "$INSTDIR\doc\etterlog.pdf" 
         CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\docs\man-etter.conf.lnk" "$INSTDIR\doc\etter.conf.pdf" 
      no_doc:
      
      CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\Uninstall.lnk" "$INSTDIR\Uninstall.exe"

   !insertmacro MUI_STARTMENU_WRITE_END

SectionEnd

;--------------------------------
;Descriptions

   ;Assign language strings to sections
   !insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
      !insertmacro MUI_DESCRIPTION_TEXT ${SecCore} "The ettercap NG core components."
      !insertmacro MUI_DESCRIPTION_TEXT ${SecEtterlog} "Etterlog: the utility to parse the ettercap NG logs"
      !insertmacro MUI_DESCRIPTION_TEXT ${SecEtterfilter} "Etterfilter: the utility to compile content filter scripts to be used within ettercap NG"
      !insertmacro MUI_DESCRIPTION_TEXT ${SecPlugins} "Various plugins"
      !insertmacro MUI_DESCRIPTION_TEXT ${SecDocs} "The most important part: the documentation !"
   !insertmacro MUI_FUNCTION_DESCRIPTION_END


;--------------------------------
;Functions

Function .onInit

   ; extract the ini for the custom UI
   !insertmacro MUI_INSTALLOPTIONS_EXTRACT "eNG-radiobuttons.ini"
   !insertmacro MUI_INSTALLOPTIONS_EXTRACT "eNG-message.ini"

FunctionEnd

Function UserAbort

   StrCmp $CHECKFAILED "1" exit 0
      !insertmacro MUI_ABORTWARNING

	exit:

FunctionEnd


Function CheckWindowsVersion

   ReadRegStr $R1 HKEY_LOCAL_MACHINE "SOFTWARE\Microsoft\Windows NT\CurrentVersion" "CurrentVersion"
   StrCmp $R1 "" 0 GoodVersion
      ; we are not 2000/XP/2003

      ; disable the 'back' and 'next' buttons
      GetDlgItem $NEXTBUTTON $HWNDPARENT 1
      GetDlgItem $BACKBUTTON $HWNDPARENT 3
      GetDlgItem $CANCELBUTTON $HWNDPARENT 2
      ShowWindow $NEXTBUTTON 0
      ShowWindow $BACKBUTTON 0
      SendMessage $CANCELBUTTON ${WM_SETTEXT} 0 "STR:Exit"

      StrCpy $CHECKFAILED "1"

      !insertmacro MUI_INSTALLOPTIONS_WRITE "eNG-message.ini" "Field 1" "Text" "Ettercap NG was compiled to run only on Windows 2000 / XP or greater\r\n\r\nYou can download the source code and try to recompile it for your platform."
      !insertmacro MUI_HEADER_TEXT "Incorrect Windows version" ""
      !insertmacro MUI_INSTALLOPTIONS_DISPLAY "eNG-message.ini"

   GoodVersion:

FunctionEnd


Function CheckWinpcap

   IfFileExists "$SYSDIR/packet.dll" WinpcapOK

      !insertmacro MUI_INSTALLOPTIONS_WRITE "eNG-message.ini" "Field 1" "Text" "The installer didn't find the file packet.dll in $SYSDIR.\r\n\r\nYou have to install winpcap in order to be able to execute ettercap."
      !insertmacro MUI_HEADER_TEXT "Missing Winpcap installation" ""
      !insertmacro MUI_INSTALLOPTIONS_DISPLAY "eNG-message.ini"

   WinpcapOK:

   FileClose $1

FunctionEnd


Function PageReinstall

   ReadRegStr $R0 HKLM "Software\ettercap_ng" ""

   StrCmp $R0 "" 0 +2
      Abort

   ;Detect version
      ReadRegDWORD $R0 HKLM "Software\ettercap_ng" "VersionMajor"
      IntCmp $R0 ${VER_MAJOR} minor_check new_version older_version
   minor_check:
      ReadRegDWORD $R0 HKLM "Software\ettercap_ng" "VersionMinor"
      IntCmp $R0 ${VER_MINOR} revision_check new_version older_version
   revision_check:
      ReadRegDWORD $R0 HKLM "Software\ettercap_ng" "VersionRevision"
      IntCmp $R0 ${VER_REVISION} same_version new_version older_version

   new_version:

    !insertmacro MUI_INSTALLOPTIONS_WRITE "eNG-radiobuttons.ini" "Field 1" "Text" "An older version of Ettercap NG is installed on your system. It's recommended that you uninstall the current version before installing. Select the operation you want to perform and click Next to continue."
    !insertmacro MUI_INSTALLOPTIONS_WRITE "eNG-radiobuttons.ini" "Field 2" "Text" "Uninstall before installing"
    !insertmacro MUI_INSTALLOPTIONS_WRITE "eNG-radiobuttons.ini" "Field 3" "Text" "Upgrade to version ${VER_DISPLAY}"
    !insertmacro MUI_HEADER_TEXT "Already Installed" "Choose how you want to install Ettercap NG."
    StrCpy $R0 "1"
    Goto reinst_start

   older_version:

    !insertmacro MUI_INSTALLOPTIONS_WRITE "eNG-radiobuttons.ini" "Field 1" "Text" "A newer version of Ettercap NG is already installed! It is not recommended that you install an older version. If you really want to install this older version, it's better to uninstall the current version first. Select the operation you want to perform and click Next to continue."
    !insertmacro MUI_INSTALLOPTIONS_WRITE "eNG-radiobuttons.ini" "Field 2" "Text" "Uninstall before installing"
    !insertmacro MUI_INSTALLOPTIONS_WRITE "eNG-radiobuttons.ini" "Field 3" "Text" "Downgrade to version ${VER_DISPLAY}"
    !insertmacro MUI_HEADER_TEXT "Already Installed" "Choose how you want to install Ettercap NG."
    StrCpy $R0 "1"
    Goto reinst_start

   same_version:

    !insertmacro MUI_INSTALLOPTIONS_WRITE "eNG-radiobuttons.ini" "Field 1" "Text" "Ettercap NG ${VER_DISPLAY} is already installed. Select the operation you want to perform and click Next to continue."
    !insertmacro MUI_INSTALLOPTIONS_WRITE "eNG-radiobuttons.ini" "Field 2" "Text" "Add/Reinstall components"
    !insertmacro MUI_INSTALLOPTIONS_WRITE "eNG-radiobuttons.ini" "Field 3" "Text" "Uninstall Ettercap NG"
    !insertmacro MUI_HEADER_TEXT "Already Installed" "Choose the maintenance option to perform."
    StrCpy $R0 "2"

   reinst_start:

   !insertmacro MUI_INSTALLOPTIONS_DISPLAY "eNG-radiobuttons.ini"

FunctionEnd

Function PageLeaveReinstall

   ; check the selected option
   !insertmacro MUI_INSTALLOPTIONS_READ $R1 "eNG-radiobuttons.ini" "Field 2" "State"

   StrCmp $R0 "1" 0 +2
      StrCmp $R1 "1" reinst_uninstall reinst_done

   StrCmp $R0 "2" 0 +3
      StrCmp $R1 "1" reinst_done reinst_uninstall

   reinst_uninstall:
   ReadRegStr $R1 HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\ettercap_ng" "UninstallString"

   ;Run uninstaller
   HideWindow

      ClearErrors
      ExecWait '$R1 _?=$INSTDIR'

      IfErrors no_remove_uninstaller
      IfFileExists "$INSTDIR\ettercap.exe" no_remove_uninstaller

         Delete $R1
         RMDir $INSTDIR

      no_remove_uninstaller:

   StrCmp $R0 "2" 0 +2
      Quit

   BringToFront

   reinst_done:

FunctionEnd



;--------------------------------
;Uninstaller Section

Section "Uninstall"

   Delete "$INSTDIR\Uninstall.exe"

   Delete "$INSTDIR\etter*.exe"
   Delete "$INSTDIR\*.dll"
   Delete "$INSTDIR\ettercapNG-${VER_DISPLAY}_cvs_debug.log"

   RMDir /r "$INSTDIR\doc"
   RMDir /r "$INSTDIR\lib"
   RMDir /r "$INSTDIR\etc"
   RMDir /r "$INSTDIR\share"
   RMDir "$INSTDIR"

   !insertmacro MUI_STARTMENU_GETFOLDER Application $MUI_TEMP

   Delete "$SMPROGRAMS\$MUI_TEMP\ettercap.lnk"
   Delete "$SMPROGRAMS\$MUI_TEMP\ettercap prompt.lnk"
   Delete "$SMPROGRAMS\$MUI_TEMP\Uninstall.lnk"
   Delete "$SMPROGRAMS\$MUI_TEMP\docs\*"
   RMDir /r "$SMPROGRAMS\$MUI_TEMP\docs"

   ;Delete empty start menu parent diretories
   StrCpy $MUI_TEMP "$SMPROGRAMS\$MUI_TEMP"

   startMenuDeleteLoop:
      RMDir $MUI_TEMP
      GetFullPathName $MUI_TEMP "$MUI_TEMP\.."

      IfErrors startMenuDeleteLoopDone

      StrCmp $MUI_TEMP $SMPROGRAMS startMenuDeleteLoopDone startMenuDeleteLoop
   startMenuDeleteLoopDone:


   DeleteRegKey /ifempty HKLM "Software\ettercap_ng"
   DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\ettercap_ng"

SectionEnd

; vim:ts=3:expandtab
