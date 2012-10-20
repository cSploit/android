# Microsoft Developer Studio Project File - Name="Libnet" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=Libnet - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Libnet.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Libnet.mak" CFG="Libnet - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Libnet - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "Libnet - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Libnet - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LIBNET_EXPORTS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LIBNET_EXPORTS" /FD /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 /nologo /dll /machine:I386 /out:"Release/Libnet-1.1.1.dll"

!ELSEIF  "$(CFG)" == "Libnet - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LIBNET_EXPORTS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LIBNET_EXPORTS" /FD /GZ /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 /nologo /dll /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "Libnet - Win32 Release"
# Name "Libnet - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\src\libnet_advanced.c
# End Source File
# Begin Source File

SOURCE=..\src\libnet_asn1.c
# End Source File
# Begin Source File

SOURCE=..\src\libnet_build_802.1q.c
# End Source File
# Begin Source File

SOURCE=..\src\libnet_build_802.1x.c
# End Source File
# Begin Source File

SOURCE=..\src\libnet_build_802.2.c
# End Source File
# Begin Source File

SOURCE=..\src\libnet_build_802.3.c
# End Source File
# Begin Source File

SOURCE=..\src\libnet_build_arp.c
# End Source File
# Begin Source File

SOURCE=..\src\libnet_build_bgp.c
# End Source File
# Begin Source File

SOURCE=..\src\libnet_build_cdp.c
# End Source File
# Begin Source File

SOURCE=..\src\libnet_build_data.c
# End Source File
# Begin Source File

SOURCE=..\src\libnet_build_dhcp.c
# End Source File
# Begin Source File

SOURCE=..\src\libnet_build_dns.c
# End Source File
# Begin Source File

SOURCE=..\src\libnet_build_ethernet.c
# End Source File
# Begin Source File

SOURCE=..\src\libnet_build_fddi.c
# End Source File
# Begin Source File

SOURCE=..\src\libnet_build_gre.c
# End Source File
# Begin Source File

SOURCE=..\src\libnet_build_icmp.c
# End Source File
# Begin Source File

SOURCE=..\src\libnet_build_igmp.c
# End Source File
# Begin Source File

SOURCE=..\src\libnet_build_ip.c
# End Source File
# Begin Source File

SOURCE=..\src\libnet_build_ipsec.c
# End Source File
# Begin Source File

SOURCE=..\src\libnet_build_isl.c
# End Source File
# Begin Source File

SOURCE=..\src\libnet_build_link.c
# End Source File
# Begin Source File

SOURCE=..\src\libnet_build_mpls.c
# End Source File
# Begin Source File

SOURCE=..\src\libnet_build_ntp.c
# End Source File
# Begin Source File

SOURCE=..\src\libnet_build_ospf.c
# End Source File
# Begin Source File

SOURCE=..\src\libnet_build_rip.c
# End Source File
# Begin Source File

SOURCE=..\src\libnet_build_rpc.c
# End Source File
# Begin Source File

SOURCE=..\src\libnet_build_snmp.c
# End Source File
# Begin Source File

SOURCE=..\src\libnet_build_stp.c
# End Source File
# Begin Source File

SOURCE=..\src\libnet_build_tcp.c
# End Source File
# Begin Source File

SOURCE=..\src\libnet_build_token_ring.c
# End Source File
# Begin Source File

SOURCE=..\src\libnet_build_udp.c
# End Source File
# Begin Source File

SOURCE=..\src\libnet_build_vrrp.c
# End Source File
# Begin Source File

SOURCE=..\src\libnet_checksum.c
# End Source File
# Begin Source File

SOURCE=..\src\libnet_cq.c
# End Source File
# Begin Source File

SOURCE=..\src\libnet_crc.c
# End Source File
# Begin Source File

SOURCE=..\src\libnet_dll.c
# End Source File
# Begin Source File

SOURCE=.\libnet_dll.def
# End Source File
# Begin Source File

SOURCE=..\src\libnet_error.c
# End Source File
# Begin Source File

SOURCE=..\src\libnet_if_addr.c
# End Source File
# Begin Source File

SOURCE=..\src\libnet_init.c
# End Source File
# Begin Source File

SOURCE=..\src\libnet_internal.c
# End Source File
# Begin Source File

SOURCE=..\src\libnet_link_win32.c
# End Source File
# Begin Source File

SOURCE=..\src\libnet_pblock.c
# End Source File
# Begin Source File

SOURCE=..\src\libnet_port_list.c
# End Source File
# Begin Source File

SOURCE=..\src\libnet_prand.c
# End Source File
# Begin Source File

SOURCE=..\src\libnet_raw.c
# End Source File
# Begin Source File

SOURCE=..\src\libnet_resolve.c
# End Source File
# Begin Source File

SOURCE=..\src\libnet_version.c
# End Source File
# Begin Source File

SOURCE=..\src\libnet_write.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\include\win32\config.h
# End Source File
# Begin Source File

SOURCE="..\include\libnet\libnet-asn1.h"
# End Source File
# Begin Source File

SOURCE="..\include\libnet\libnet-functions.h"
# End Source File
# Begin Source File

SOURCE="..\include\libnet\libnet-headers.h"
# End Source File
# Begin Source File

SOURCE="..\include\libnet\libnet-macros.h"
# End Source File
# Begin Source File

SOURCE="..\include\libnet\libnet-structures.h"
# End Source File
# Begin Source File

SOURCE="..\include\libnet\libnet-types.h"
# End Source File
# Begin Source File

SOURCE=..\include\win32\libnet.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# End Target
# End Project
