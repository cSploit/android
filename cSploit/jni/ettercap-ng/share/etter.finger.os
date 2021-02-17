############################################################################
#                                                                          #
#  ettercap -- etter.finger.os -- passive OS fingerprint database          #
#                                                                          #
#  Copyright (C) 2001-2004  ALoR & NaGA                                    #
#                                                                          #
#  This program is free software; you can redistribute it and/or modify    #
#  it under the terms of the GNU General Public License as published by    #
#  the Free Software Foundation; either version 2 of the License, or       #
#  (at your option) any later version.                                     #
#                                                                          #
############################################################################
#                                                                          #
#  Version         : $Revision: 1.21 $                                     # 
#  Last updated on : $Date: 2004/11/05 10:21:03 $                          #
#  Total entries   : 1697                                                  #
#                                                                          #
############################################################################
#                                                                          #
# The fingerprint database has the following structure:                    #
#                                                                          #
# WWWW:MSS:TTL:WS:S:N:D:T:F:LEN:OS                                         #
#                                                                          #
# WWWW: 4 digit hex field indicating the TCP Window Size                   #
# MSS : 4 digit hex field indicating the TCP Option Maximum Segment Size   #
#       if omitted in the packet or unknown it is "_MSS"                   #
# TTL : 2 digit hex field indicating the IP Time To Live                   #
# WS  : 2 digit hex field indicating the TCP Option Window Scale           #
#       if omitted in the packet or unknown it is "WS"                     #
# S   : 1 digit field indicating if the TCP Option SACK permitted is true  #
# N   : 1 digit field indicating if the TCP Options contain a NOP          #
# D   : 1 digit field indicating if the IP Don't Fragment flag is set      #
# T   : 1 digit field indicating if the TCP Timestamp is present           #
# F   : 1 digit ascii field indicating the flag of the packet              #
#       S = SYN                                                            #
#       A = SYN + ACK                                                      #
# LEN : 2 digit hex field indicating the length of the packet              #
#       if irrilevant or unknown it is "LT"                                #
# OS  : an ascii string representing the OS                                #
#                                                                          #
# IF YOU FIND A NEW FINGERPRING, PLEASE MAIL IT US WITH THE RESPECTIVE OS  #
# or use the appropriate form at:                                          #
#    http://ettercap.sourceforge.net/index.php?s=stuff&p=fingerprint       #
#                                                                          #
# TO GET THE LATEST DATABASE:                                              #
#                                                                          #
#    ettercap -U                                                           #
#                                                                          #
############################################################################

0000:05B4:80:WS:0:0:1:0:S:2C:Novell netware 5.00
0000:_MSS:80:WS:0:0:0:0:A:LT:3Com Access Builder 4000 7.2
0000:_MSS:FF:WS:0:0:0:0:A:28:Windows XP
0008:_MSS:40:WS:0:0:0:0:S:28:Red Hat Linux 7.2 Kernel 2.4.7-10
0010:_MSS:40:WS:0:0:0:0:S:28:Gentoo Linux (Kernel 2.6.6-rc1)
0017:05B4:40:00:0:1:0:1:A:3C:Gestetner printer 
0040:_MSS:80:WS:0:0:0:0:A:LT:Gold Card Ethernet Interface Firm. Ver. 3.19 (95.01.16)
0046:_MSS:80:WS:0:0:0:0:A:LT:Cyclades PathRouter
0096:_MSS:80:WS:0:0:0:0:A:LT:Cyclades PathRouter V 1.2.4
0100:0218:FF:WS:0:0:0:0:A:2C:Allied Hub
0100:_MSS:80:WS:0:0:0:0:A:LT:Allied Telesyn AT-S10 version 3.0 on an AT-TS24TR hub
0100:_MSS:80:WS:0:0:1:0:A:LT:Xyplex Network9000
0109:0109:40:00:0:1:0:1:A:3C:Debian Potato (2.2), Linux 2.2.17
0200:0000:40:WS:0:0:0:0:S:LT:Linux 2.0.35 - 2.0.37
0200:0200:40:00:0:1:0:0:A:30:Linux 
0200:0200:40:WS:0:0:0:0:A:2C:Ascend MAX 1800
0200:05B4:40:00:0:0:0:0:S:2C:Linux 2.0.35 - 2.0.38
0200:05B4:40:00:0:0:0:0:S:LT:Linux 2.0.38
0200:05B4:40:34:0:0:0:0:S:LT:Linux 2.0.33
0200:05B4:40:WS:0:0:0:0:S:2C:Linux 2.0.34-38
0200:05B4:40:WS:0:0:0:0:S:LT:Linux 2.0.36
0200:05B4:FF:WS:0:0:0:0:A:2C:Router 3Com 812 ADSL
0200:_MSS:40:WS:0:0:0:0:S:28:Windows XP
0200:_MSS:40:WS:0:0:1:0:A:28:ESESA TCP/IP Stack
0200:_MSS:80:00:0:0:0:0:A:LT:Linux 2.0.32-34
0200:_MSS:80:00:0:1:0:0:A:LT:Bay Networks BLN-2 Network Router or ASN Processor rev 9
0200:_MSS:80:00:0:1:0:1:A:LT:Bay Networks BLN-2 Network Router or ASN Processor rev 9
0200:_MSS:80:WS:0:0:0:0:A:LT:3COM / USR TotalSwitch Firmware: 02.02.00R
0212:_MSS:80:00:0:0:0:0:A:LT:Linux 2.0.32-34
0212:_MSS:80:00:0:1:1:0:A:LT:Solaris 2.6 - 2.7
0212:_MSS:80:00:0:1:1:1:A:LT:Solaris 2.6 - 2.7
0212:_MSS:80:WS:0:0:0:0:A:LT:CacheOS (CacheFlow 2000 proxy cache)
0212:_MSS:80:WS:0:0:1:0:A:LT:Linux 2.2.5 - 2.2.13 SMP
0212:_MSS:80:WS:0:1:0:0:A:LT:NetBSD 1.4 running on a SPARC IPX
0212:_MSS:80:WS:0:1:0:1:A:LT:NetBSD 1.4 running on a SPARC IPX
0212:_MSS:80:WS:0:1:1:0:A:LT:NetBSD 1.4 / Generic mac68k (Quadra 610)
0212:_MSS:80:WS:0:1:1:1:A:LT:NetBSD 1.4 / Generic mac68k (Quadra 610)
0218:0218:40:00:0:1:0:1:A:LT:NetBSD
0218:0218:40:WS:0:0:0:0:A:2C:Windows 98
0244:_MSS:80:WS:0:0:0:0:A:LT:Cyclades PathRouter/PC
03CA:_MSS:80:WS:0:0:0:0:A:LT:MPE/iX 5.5
03E0:0550:40:05:1:1:1:1:S:LT:Windows 2000
03E0:05B4:20:05:1:1:1:1:S:40:Windows 2000 server
03F2:_MSS:80:00:0:0:0:0:A:LT:Lexmark Optra S Printer
03F2:_MSS:80:WS:0:0:0:0:A:LT:Lexmark Optra S Printer
03F6:0218:FF:WS:0:0:0:0:A:2C:Lexmark Optra SC Printer
03F6:_MSS:80:WS:0:0:0:0:A:LT:Lexmark Optra S Printer
0400:0109:40:10:0:1:0:1:S:3C:Linux
0400:0200:FF:WS:0:0:0:0:A:2C:Zyxel Prestige 10
0400:0218:FF:WS:0:0:0:0:A:2C:3Com Superstack II
0400:0400:20:WS:0:0:0:0:A:2C:Windows 2000
0400:0400:40:00:1:1:0:1:A:3C:ElsaLanCom (SoHo ISDN Router)
0400:0400:80:WS:0:0:0:0:A:2C:Prime SharePH-1UNW (hardware printserver)
0400:0400:80:WS:0:0:0:0:S:2C:NCSA 2.3.07
0400:0500:80:WS:0:0:0:0:A:2C:ITC Version i7.13 of 02-07-99 (embedded device)
0400:0594:FF:WS:0:0:0:0:A:2C:3Com SuperStack 3 Switch 4400
0400:059C:FF:WS:0:0:0:0:A:18:3Com SuperStack 3300xm version 2.71 prom 1.00 
0400:059C:FF:WS:0:0:0:0:A:2C:3Com SuperStack II 3300
0400:05A4:FF:WS:0:0:0:0:A:2C:3Com SuperStack II 3000 
0400:05B4:40:00:0:1:0:1:S:3C:Windows 44
0400:05B4:40:WS:0:0:0:0:A:2C:FORE ES-2810 Switch
0400:05B4:FF:WS:0:0:0:0:A:2C:3Com 812 ADSL ROUTER
0400:05C8:20:WS:0:0:0:0:A:2C:Nortel BayStack Access Node
0400:0901:40:10:0:1:0:1:S:3C:Mac OS X 102.4
0400:_MSS:40:WS:0:0:0:0:S:28:Linux 2.4.18
0400:_MSS:80:00:0:1:0:0:A:LT:Bay Networks BLN-2 Network Router or ASN Processor rev 9
0400:_MSS:80:00:0:1:0:1:A:LT:Bay Networks BLN-2 Network Router or ASN Processor rev 9
0400:_MSS:80:WS:0:0:0:0:A:LT:3com Office Connect Router 810
0400:_MSS:80:WS:0:0:1:0:A:LT:Aironet 630-2400 V3.3P Wireless LAN bridge
0420:0114:40:00:1:1:1:1:A:3C:Linux 2.4.18-rc4
0424:_MSS:80:WS:0:0:0:0:A:LT:Intel InBusiness Print Station
0430:_MSS:80:WS:0:0:0:0:A:LT:PacketShaper 4000 v4.1.3b2 2000-04-05
052A:052A:40:00:1:1:0:1:A:3C:Debian Potato (2.2), Linux 2.2.20
0534:0534:80:WS:0:0:1:0:A:2C:Netware
0550:0550:40:WS:0:0:0:0:A:2C:Linux 2.4
0564:0564:40:WS:0:0:0:0:A:2C:Linux
0564:0564:40:WS:1:0:1:1:A:38:Linux 2.4 
0564:0564:80:WS:1:1:1:0:A:LT:Windows 2000
0564:6405:40:00:0:1:1:1:A:3C:Solaris
0564:6405:40:WS:0:1:1:1:A:38:Mac OS
0578:0578:08:WS:0:0:0:0:A:2C:Minix 16-bit/Intel 2.0.3
0578:0578:40:WS:0:0:0:0:A:2C:CVP Telsey
0578:0578:40:WS:0:0:0:0:S:2C:Windows 2000
0578:_MSS:40:WS:0:0:0:0:A:28:Elsa Router 
0578:_MSS:80:WS:0:0:0:0:A:LT:Minix 32-bit/Intel 2.0.0
0584:0584:80:00:1:1:1:1:A:3C:Red Hat Linux 8.0 (Psyche)
0584:0584:FF:00:0:1:1:1:A:3C:OpenBSD
05AC:0218:40:WS:0:0:0:0:A:2C:AXIS Printer
05B4:052A:40:00:0:1:0:1:A:3C:NetBSD 1.6.1
05B4:052A:40:00:1:1:0:1:A:3C:Debian Sarge, Linux 2.4.24-1
05B4:0564:40:01:0:1:1:1:A:3C:FreeBSD 5.1-RELEASE 
05B4:0564:40:WS:1:1:0:0:A:30:Red Hat Linux 9.0 
05B4:0564:80:WS:1:1:0:0:A:30:Windows 2000 
05B4:05B4:20:WS:0:0:0:0:A:2C:Windows XP
05B4:05B4:40:00:0:1:0:1:S:3C:HP LaserJet 4550 Printer 
05B4:05B4:40:00:0:1:1:1:A:3C:FreeBSD
05B4:05B4:40:00:1:1:1:1:A:3C:Red Hat Linux release 6.2 (Zoot) Kernel 2.2.14-5.0
05B4:05B4:40:00:1:1:1:1:A:40:SunOS 5.8 
05B4:05B4:40:01:1:1:1:1:A:40:Solaris 8 
05B4:05B4:40:WS:1:1:1:0:A:30:Windows 2000 Server
05B4:05B4:40:WS:1:1:1:0:S:30:Windows XP SP1
05B4:05B4:80:WS:0:0:0:0:A:2C:Zebra ZPL 
05B4:05B4:80:WS:1:1:1:0:A:30:Windows 2000 Server 
05B4:05B4:FF:00:1:1:1:1:A:40:Solaris 7 
05B4:05B4:FF:WS:0:0:0:0:A:2C:FreeBSD/i386
05B4:B405:40:00:0:1:1:1:A:LT:Red Hat 7.1  (kernel 2.4.3)
05B4:B405:80:00:0:1:1:1:A:LT:Windows 2000 Server
05B4:_MSS:80:00:0:1:0:0:A:LT:HP JetDirect Card (J4169A) in an HP LaserJet 8150
05B4:_MSS:80:WS:0:0:0:0:A:LT:TOPS-20 Monitor 7(102540)-1,TD-1
05B4:_MSS:80:WS:0:1:1:0:A:LT:Network Appliance NetCache 5.1D4
05B4:_MSS:80:WS:0:1:1:1:A:LT:Network Appliance NetCache 5.1D4
05DC:0FD8:40:WS:0:0:0:0:A:2C:D-Link dsl604 wireless router 
05DC:_MSS:80:WS:0:0:0:0:A:LT:Gandalf LanLine Router
0600:0300:20:WS:0:0:0:0:S:2C:Chase IOLAN Terminal Server v3.3.09 - TCP 
0600:_MSS:80:WS:0:0:0:0:A:LT:Chase IOLAN Terminal Server v3.5.02 CDi
0640:05B4:40:WS:0:0:0:0:A:2C:APC MasterSwitch Network Power Controller
0640:_MSS:80:WS:0:0:0:0:A:LT:APC MasterSwitch Network Power Controller
0648:0218:40:WS:0:0:0:0:A:2C:HP Printer
0648:_MSS:80:WS:0:0:0:0:A:LT:FastComm FRAD F9200-DS-DNI -- Ver. 4.2.3A
06C2:_MSS:80:WS:0:0:0:0:A:LT:Cyclades PathRAS Remote Access Server v1.1.8 - 1.3.12
0700:_MSS:80:00:0:0:0:0:A:LT:Lantronix ETS16P Version V3.5/2(970721)
0700:_MSS:80:WS:0:0:0:0:A:LT:Lantronix ETS16P Version V3.5/2(970721)
073F:_MSS:80:00:0:0:0:0:A:LT:Novell NetWare 3.12 or 386 TCP/IP
073F:_MSS:80:WS:0:0:0:0:A:LT:CLIX R3.1 Vr.7.6.20 6480
07D0:_MSS:80:00:0:0:1:0:A:LT:Novell NetWare 3.12 - 5.00
07D0:_MSS:80:WS:0:0:1:0:A:LT:Novell NetWare 3.12 - 5.00
0800:0109:40:10:0:1:0:1:S:3C:Linux
0800:0200:FF:WS:0:0:0:0:A:2C:Cisco
0800:0578:FF:WS:0:0:0:0:A:2C:NetGear Hardware Router
0800:_MSS:40:WS:0:0:0:0:A:28:Connexa 
0800:_MSS:40:WS:0:0:0:0:S:28:Window 2000 SP3
0800:_MSS:80:00:0:0:0:0:A:LT:KA9Q
0800:_MSS:80:00:0:0:0:1:A:LT:KA9Q
0800:_MSS:80:WS:0:0:0:0:A:LT:3Com Access Builder 4000 7.2
0800:_MSS:80:WS:0:0:1:0:A:LT:HP Procurve Routing Switch 9304M
0808:_MSS:80:WS:0:0:0:0:A:LT:Siemens HICOM 300 Phone switch (WAML LAN card)
0834:0578:FF:WS:0:0:0:0:A:2C:BeWan 2.3.6 
0834:7805:FF:WS:0:0:0:0:A:2C:Vigor 2900G 
0848:_MSS:80:WS:0:0:0:0:A:LT:Intergraph Workstation (2000 Series) running CLiX R3.1
0860:0218:40:00:1:1:1:0:S:30:Windows 9x
0860:0218:40:00:1:1:1:0:S:3C:Windows 9x
0860:0218:40:00:1:1:1:0:S:LT:Windows 9x
0860:0218:40:WS:0:0:0:0:A:2C:Windows 98
0860:0218:80:WS:0:0:1:0:A:2C:Windows 98
0860:0218:FF:WS:0:0:0:0:S:LT:Cisco IGS 3000 IOS 11.x(16), 2500 IOS 11.2(3)P
0860:05B4:40:WS:0:0:1:0:A:2C:Windows 98
0860:05B4:40:WS:1:1:1:0:S:30:Windows 98
0860:05B4:80:00:0:1:1:1:A:3C:Windows ME
0860:05B4:FF:WS:0:0:0:0:A:LT:IOS Version 10.3(15) - 11.1(20)
0860:_MSS:80:00:0:0:0:0:A:LT:HP JetDirect  Firmware Rev. H.06.00
0860:_MSS:80:00:0:1:1:0:A:LT:Windows NT4 / Win95 / Win98
0860:_MSS:80:00:0:1:1:1:A:LT:Windows NT4 / Win95 / Win98
0860:_MSS:80:WS:0:0:0:0:A:LT:Chase IOLan Terminal Server
09C8:02F8:FF:WS:0:0:0:0:A:2C:Windows
0A28:_MSS:80:WS:0:0:0:0:A:LT:Apple Color LaserWrite 600 Printer
0AF0:0578:80:WS:1:1:1:0:A:30:Windows 2000
0B18:058C:40:WS:0:0:0:0:S:2C:HNC 91849
0B63:_MSS:80:00:0:1:1:0:A:LT:Linux 2.1.19 - 2.2.17
0B63:_MSS:80:00:0:1:1:1:A:LT:Linux 2.1.19 - 2.2.17
0B68:05B4:40:00:1:1:1:1:S:3C:Linux 2.4.21 
0B68:05B4:40:WS:0:0:0:0:A:2C:DLink DP-300 Printserver
0B68:05B4:40:WS:1:1:1:0:A:30:Linux 2.2.19
0B68:05B4:FF:WS:1:1:1:0:A:LT:Lexmark T520 Network Printer
0B68:_MSS:40:WS:0:0:1:0:A:LT:Linux 2.0.32 - 2.0.34
0B68:_MSS:80:00:0:1:1:0:A:LT:Sun Solaris 8 early acces beta through actual release
0B68:_MSS:80:00:0:1:1:1:A:LT:Sun Solaris 8 early acces beta through actual release
0B68:_MSS:80:WS:0:0:0:0:A:LT:D-Link Print Server
0BB8:05B0:20:WS:0:0:0:0:A:2C:VMS/VAX 5.5
0BB8:_MSS:80:00:0:1:0:0:A:LT:OpenVMS 7.1 Alpha running Digital's UCX v4.1ECO2
0C00:0109:40:10:0:1:0:1:S:3C:Linux Mandrake 9.1
0C00:052A:40:WS:0:0:0:0:A:2C:NetGear Router
0C00:05A0:40:WS:0:0:0:0:A:2C:Trendnet TEW 431 BRP
0C00:_MSS:40:WS:0:0:0:0:S:28:Slackware 8.0
0C00:_MSS:40:WS:0:0:0:0:S:LT:Linux Slakware 8.0
0C00:_MSS:80:WS:0:0:0:0:A:LT:Canon photocopier/fax/scanner/printer GP30F
0C1C:05B4:FF:WS:0:0:0:0:A:2C:Nokia M1122 Adsl Router
0C90:0218:40:WS:0:0:0:0:A:2C:HP Printer
0C90:05B4:40:00:1:1:1:0:S:34:Windows 
0C90:05B4:40:WS:1:1:1:0:S:30:Windows XP
0C90:_MSS:80:WS:0:0:0:0:A:LT:HP JetDirect Print Server
0E00:_MSS:80:WS:0:0:0:0:A:LT:Lantronix EPS1 Version V3.5/1(970325)
0F87:_MSS:80:00:0:0:0:0:A:LT:Novell NetWare 3.12 or 386 TCP/IP
0F87:_MSS:80:WS:0:0:0:0:A:LT:A/UX 3.1.1 SVR2 or OpenStep 4.2
0F87:_MSS:80:WS:0:0:1:0:A:LT:AIX 4.3
0FA0:_MSS:80:WS:0:0:0:0:A:LT:MultiTech CommPlete (modem server) RAScard
0FFF:0400:20:WS:0:0:0:0:A:2C:ASMAX Broadband router
1000:0002:40:WS:0:0:0:0:A:2C:Alcatel Modem
1000:0004:FF:WS:0:0:0:0:A:2C:Cisco PIX
1000:0109:40:10:0:1:0:1:S:LT:FreeBSD 4.5
1000:0200:40:00:0:1:0:1:A:3C:Alcatel Router ADSL Speed Touch Pro
1000:0200:40:WS:0:0:0:0:A:2C:Alcatel Speedtouch Pro ADSL modem
1000:0200:40:WS:0:0:0:0:S:LT:CISCO IOS
1000:0200:FF:WS:0:0:0:0:A:2C:Alcatel Speed Touch Home/Pro
1000:0400:1E:F5:0:0:0:0:S:LT:Alcatel (Xylan) OmniStack 5024 v3.4.5
1000:0400:1E:WS:0:0:0:0:S:LT:Chorus MiX V.3.2 r4.1.5 COMP-386
1000:0400:20:F5:0:0:0:0:S:LT:Alcatel (Xylan) OmniStack 5024
1000:0400:20:WS:0:0:0:0:A:2C:Alcatel LSS 210-Stack Version 3.4.8
1000:0400:40:WS:0:0:0:0:A:2C:HP J2603A Ethernet SNMP Module
1000:0400:40:WS:0:0:0:0:S:2C:Trumpet TCP 2.01 / DOS
1000:05AC:FF:WS:0:0:0:0:A:2C:QMS4060
1000:05B0:80:00:0:1:0:0:S:30:VMS
1000:05B4:10:WS:0:0:0:0:A:2C:Extreme Gigabit switch
1000:05B4:20:WS:0:0:0:0:A:2C:StorageWorks SAN switch (FabricOS v2.1.7)
1000:05B4:40:WS:0:0:0:0:A:18:DWL1000 802.11b Access Pointrt.com
1000:05B4:40:WS:0:0:0:0:A:2C:iPath Cable Media Access Hub
1000:05B4:40:WS:0:0:0:0:S:2C:Solaris
1000:05B4:FF:WS:0:0:0:0:A:2C:Cisco Pix 515
1000:0901:40:10:0:1:0:1:S:LT:Mac os X 10.1
1000:AC05:40:WS:0:0:0:0:A:2C:HP Procurve Switch
1000:_MSS:20:WS:0:0:0:0:A:28:Motorola SurfBoard SB4100 CableModem
1000:_MSS:20:WS:0:0:0:0:S:28:Linux RedHat 9 (kernel 2.4.20)
1000:_MSS:40:WS:0:0:0:0:A:LT:SCO UnixWare 2.1.2
1000:_MSS:40:WS:0:0:0:0:S:28:SunOS 4.1.4
1000:_MSS:40:WS:0:0:0:0:S:LT:Linux
1000:_MSS:80:00:0:0:1:0:A:LT:OpenVMS/Alpha 7.1 using Process Software's TCPWare V5.3-4
1000:_MSS:80:00:0:1:0:0:A:LT:Alcatel 1000 ADSL (modem)
1000:_MSS:80:00:0:1:0:1:A:LT:Alcatel 1000 ADSL (modem)
1000:_MSS:80:WS:0:0:0:0:A:28:Aironet AP4800E v8.07 11 Mbps wireless access poinit
1000:_MSS:80:WS:0:0:1:0:A:LT:VirtualAccess LinxpeedPro 120 running Software 7.4.33CM
1020:0218:FF:WS:0:0:0:0:A:2C:Cisco 2600 IOS 12.0
1020:0218:FF:WS:0:0:0:0:S:2C:Cisco 3660 IOS 12.2(x)
1020:022C:FF:00:0:0:0:0:S:LT:Cisco 1750 IOS 12.0(5), Cisco 2500 IOS 11.3(1)
1020:022C:FF:WS:0:0:0:0:A:2C:Cisco IOS 12.0(5)
1020:0564:FF:WS:0:0:0:0:A:2C:IOS (tm) C2600 Software (C2600-IS-M), Version 12.2(8)T4,R 
1020:05B4:FF:WS:0:0:0:0:A:2C:Cisco IOS 12.1.5-12.2.13a
1020:05B4:FF:WS:0:0:0:0:S:LT:Cisco 2611 IOS 11.3(2)XA4
1020:6405:FF:WS:0:0:0:0:A:2C:Cisco IOS 
1020:B405:FF:WS:0:0:0:0:A:2C:AIRONET1200 
1020:_MSS:80:WS:0:0:0:0:A:LT:AS5200
1020:_MSS:FF:WS:0:0:1:0:A:LT:Cisco IOS
10C0:0218:80:WS:1:1:1:0:S:30:Windows 2000 Advanced Server SP2
10C0:0218:FF:WS:0:0:0:0:A:2C:Cisco IOS
10C0:0218:FF:WS:0:0:0:0:S:LT:Cisco 1600 IOS 11.2(15)P
10C0:055E:80:WS:1:1:1:0:S:30:Windows 98
10C0:05B4:40:00:1:1:1:1:S:40:Windows XP
10C0:05B4:80:WS:1:1:1:0:S:30:Windows NT SP3
10C0:05B4:FF:WS:0:0:0:0:A:2C:Cisco IOS 11.2
10C0:05B4:FF:WS:0:0:0:0:S:LT:Cisco 3620 IOS 11.2(17)P
10C0:_MSS:80:WS:0:0:0:0:A:LT:Cisco 1600/3640/7513 Router (IOS 11.2(14)P)
111C:0218:40:WS:0:0:0:0:A:2C:Ascend MAX 1800
111C:05B4:20:WS:0:0:0:0:A:2C:Linksys PSUS4 Printserver
111C:05B4:40:WS:0:0:0:0:A:2C:OpenVMS
111C:05B4:40:WS:0:0:1:0:A:2C:Ascend MAX6000
111C:05B4:40:WS:0:0:1:0:A:LT:SCO Openserver 502
111C:_MSS:80:WS:0:0:0:0:A:LT:Ascend/Lucent Max (HP,4000-6000) version 6.1.3 - 7.0.2+
111C:_MSS:80:WS:0:0:1:0:A:LT:Apple LaserWriter 16/600 PS, HP 6P, or HP 5 Printer
1140:05CC:40:00:1:1:1:1:A:3C:Linux 2.4.8
1146:05C2:40:00:1:1:1:1:S:3C:Debian 3.0 woody (2.4.18)
1164:05B4:40:01:1:1:1:1:S:3C:Fedora Red Hat
1234:_MSS:FF:WS:0:0:0:0:S:28:Knoppix based L.A.S 
12CC:0650:40:00:1:1:1:1:A:3C:Debian Woody
12D8:1802:FF:WS:0:0:0:0:S:2C:Palm OS 
1490:0524:40:00:1:1:1:1:S:3C:Linux Suse 8.1
14B8:052E:40:00:1:1:1:1:S:3C:Linux RedHat 7
14F0:0218:80:WS:1:1:1:0:A:LT:Windows 2000 Professional
14F0:05B4:80:WS:1:1:1:0:S:LT:Windows ME 
1520:0548:40:00:1:1:1:1:S:3C:Linux Red Hat
1540:0550:40:00:1:1:1:1:S:3C:Linux
1540:0550:40:WS:0:0:1:0:A:2C:Linux RedHat - 2.4.18
1540:0550:40:WS:1:1:1:0:A:30:Linux 2.4.x
159F:05B4:40:00:0:1:1:0:S:3C:FreeBSD 2.2.1 - 4.1
15B8:057A:40:00:1:1:1:1:A:3C:Linux 2.4.10
15E0:0584:40:00:0:1:1:1:A:3C:Linux 2.4.20
1608:0582:40:WS:1:1:1:0:A:30:Linux
1610:0584:40:00:1:1:1:1:S:3C:Linux 2.4.20
1618:0578:40:00:1:1:1:1:S:3C:Linux 2.4.13
1640:0590:40:00:1:1:1:1:S:3C:Linux 2.4.23
165C:_MSS:80:WS:0:0:1:0:A:LT:SCO Release 5
1678:05AA:40:00:1:1:1:1:A:3C:Linux 2.6.1
1680:0584:40:00:1:1:1:1:A:3C:Linux 2.4.18 2.4.19
1680:05AC:40:00:1:1:1:1:A:3C:Linux 2.4.20 (X86)
1680:_MSS:80:00:0:1:1:1:A:LT:Linux 2.4.7 (X86)
16A0:01F4:40:00:1:1:1:1:A:3C:Linux 
16A0:0564:40:00:0:1:1:1:A:3C:Linux 2.6.x
16A0:0564:40:00:1:1:0:1:A:3C:Linux Redhat 7.2 (Enigma) - Linux
16A0:0564:40:00:1:1:1:1:A:3C:Linux 2.4.xx
16A0:0564:40:WS:0:1:1:1:A:38:Linux
16A0:0564:40:WS:1:0:1:1:A:38:Linux 2.4.21 
16A0:0578:40:00:1:1:1:1:A:3C:Linux 2.4.7
16A0:057A:40:00:1:1:1:1:A:3C:Debian Linux
16A0:057A:40:02:1:1:1:1:A:3C:Debian GNU/Linux
16A0:0584:40:00:1:1:1:1:A:3C:Linux 
16A0:0586:40:00:1:1:1:1:A:3C:Linux
16A0:059C:40:00:1:1:1:1:A:3C:Linux fw 2.4.7-10
16A0:05AC:20:00:1:1:1:1:A:3C:Linux
16A0:05AC:40:00:0:1:1:1:A:3C:Linux 2.4.20 RedHat 9 
16A0:05AC:40:00:1:1:0:1:A:3C:Linux
16A0:05AC:40:00:1:1:1:1:A:3C:Linux 2.4.18-686 (Debia GNU/Linux)
16A0:05AC:80:00:1:1:1:1:A:3C:Linux 2.4.12 
16A0:05AC:FF:00:1:1:1:1:A:3C:Linux 2.4.12 
16A0:05B4:40:00:0:1:1:1:A:3C:Linux 2.4.4-4GB
16A0:05B4:40:00:1:1:0:1:A:3C:Windows XP
16A0:05B4:40:00:1:1:1:0:A:LT:Linux 2.4.2
16A0:05B4:40:00:1:1:1:1:A:3C:Linux 2.4.xx
16A0:05B4:40:01:1:1:1:1:A:LT:Linux Kernel 2.4.17 (with MOSIX patch)
16A0:05B4:40:03:1:1:0:1:A:3C:Linux Kernel 2.4.xx
16A0:05B4:40:07:1:1:1:1:A:3C:Linux.2.4.20-web100
16A0:05B4:40:WS:0:1:1:1:A:38:Windows
16A0:05B4:40:WS:1:0:1:1:A:38:Linux 
16A0:05B4:80:00:0:1:1:0:A:3C:Linux Kernel 2.4.xx
16A0:05B4:80:00:0:1:1:1:A:3C:Linux Kernel 2.4.xx
16A0:05B4:80:00:1:1:1:1:A:3C:Linux Kernel 2.4.12
16A0:05B4:FF:00:1:1:0:1:A:3C:Linux Kernel 2.4.18
16A0:05B4:FF:00:1:1:1:1:A:3C:Linux 2.4.12
16A0:2A05:40:00:1:1:0:1:A:3C:Linux 2.4.22
16A0:5C05:40:00:0:1:1:1:A:3C:Linux 2.4.19 Knoppix
16A0:6405:40:02:0:1:1:1:A:3C:RedHat Enterprise 3.0 ES
16A0:7A05:40:00:0:1:1:1:A:3C:Redhat Linux
16A0:8C05:40:00:0:1:1:1:A:3C:Linux RedHat 9 
16A0:B405:40:00:0:1:1:1:A:3C:Linux version 2.4.2-2 (Red Hat Linux 7.1)
16A0:B405:40:00:0:1:1:1:A:LT:Linux Kernel 2.4.24 (ppc)
16A0:B405:40:00:1:1:1:1:A:3C:Linux Debian Unstable 2.4.26
16A8:05AA:40:WS:1:1:1:0:A:30:Debian GNU/Linux (unstable) - 2.4.xx Series Kernel
16B0:051E:40:WS:1:1:1:0:A:30:Linux 2.4.19
16B0:0584:40:00:1:1:1:1:S:3C:Linux 2.2.x 
16B0:0584:40:WS:0:0:1:0:A:2C:Slackware 8.0 Linux 2.2.20
16B0:0584:40:WS:1:1:1:0:A:30:Redhat 7.0 (linux 2.2.16)
16B0:05AC:40:00:1:1:1:0:S:3C:Linux 2.4.10
16B0:05AC:40:00:1:1:1:1:S:3C:Linux 2.4.18-6mdk
16B0:05AC:40:6F:1:1:1:0:S:3C:Linux 2.4.10
16D0:0218:80:00:1:1:1:0:S:30:Windows 95
16D0:0218:80:WS:1:1:1:0:A:30:Windows 2000
16D0:052A:40:00:1:1:0:1:S:3C:Linux 2.4.23
16D0:0550:80:WS:1:1:1:0:S:30:Windows XP
16D0:055C:40:00:1:1:1:1:S:3C:Linux 2.6.7-gentoo-r7
16D0:0564:40:00:0:1:1:0:A:30:Phlak 0.2 
16D0:0564:40:00:1:1:1:0:A:34:Linux 2.4.2
16D0:0564:40:00:1:1:1:1:S:3C:Linux RedHat 
16D0:0564:40:WS:0:0:1:0:A:2C:Linux 2.4
16D0:0564:40:WS:1:1:1:0:A:30:Linux 2.4.19
16D0:0564:40:WS:1:1:1:1:S:3C:Slackware Linux 8.1
16D0:0584:40:00:1:1:1:1:S:3C:Linux 2.4.8
16D0:0584:40:WS:0:0:1:0:A:2C:Linux 2.4.18 
16D0:0584:40:WS:1:1:1:0:A:30:Linux 2.4.17
16D0:0584:80:WS:0:0:1:0:A:2C:RedHat Linux
16D0:0594:40:WS:0:0:0:0:A:2C:Linux 
16D0:0598:40:WS:0:0:1:0:A:2C:Linux
16D0:0598:40:WS:1:1:0:0:A:30:Linux
16D0:059C:40:WS:1:1:1:0:A:30:Red Hat Linux
16D0:05AC:40:00:1:1:1:1:S:3C:Debian - Linux 2.4.18 
16D0:05AC:40:WS:0:0:1:0:A:2C:Linux 2.4.xx 
16D0:05AC:40:WS:1:1:0:0:A:30:Windows .NET 
16D0:05AC:40:WS:1:1:1:0:A:30:Linux 2.4.18-3 (IServ/RedHat)
16D0:05B4:01:WS:1:1:1:0:A:30:Linux
16D0:05B4:20:00:1:1:1:1:S:3C:Linux 2.4.20
16D0:05B4:20:00:1:1:1:1:S:LT:Linux 2.4.10-GR Security Patch 1.8.1
16D0:05B4:40:00:0:1:0:0:A:30:Slackware 2.4.17
16D0:05B4:40:00:0:1:0:1:A:3C:HP psc 2500 network Printer 
16D0:05B4:40:00:0:1:1:0:A:30:Linux Slackware 8 - kernel 2.4.17
16D0:05B4:40:00:0:1:1:0:S:30:Mandrake 8.2
16D0:05B4:40:00:0:1:1:0:S:3C:Linux 2.4.13-ac7
16D0:05B4:40:00:0:1:1:1:S:3C:Linux 2.4.19-pre10-ac2
16D0:05B4:40:00:1:1:0:0:S:34:Linux 2.4.xx
16D0:05B4:40:00:1:1:0:1:S:3C:Linux 2.6.0
16D0:05B4:40:00:1:1:1:0:A:34:Linux 2.4.xx
16D0:05B4:40:00:1:1:1:0:S:30:Linux 2.4.1-14
16D0:05B4:40:00:1:1:1:0:S:34:Linux 2.4.23-grsec 
16D0:05B4:40:00:1:1:1:0:S:34:Linux Debian Woody
16D0:05B4:40:00:1:1:1:0:S:3C:Linux 2.4.xx
16D0:05B4:40:00:1:1:1:1:S:3C:Linux 2.4.xx
16D0:05B4:40:01:1:1:1:1:S:3C:Linux 2.4.10 - 2.4.16
16D0:05B4:40:01:1:1:1:1:S:LT:Linux Red Hat 9 
16D0:05B4:40:03:1:1:1:0:A:34:Windows 
16D0:05B4:40:07:1:1:1:1:S:3C:Debian Linux 
16D0:05B4:40:07:1:1:1:1:S:3C:Linux Fedora Core 2 2.6.8-1.521 
16D0:05B4:40:WS:0:0:0:0:A:2C:HP LaserJet 2100 Series
16D0:05B4:40:WS:0:0:0:0:A:LT:Intel PRO/Wireless LAN Acess Point Version 02.00-04
16D0:05B4:40:WS:0:0:0:0:S:2C:Linux
16D0:05B4:40:WS:0:0:1:0:A:2C:Linux 2.4.xx
16D0:05B4:40:WS:0:0:1:0:A:LT:Linux
16D0:05B4:40:WS:0:0:1:0:A:LT:Linux 
16D0:05B4:40:WS:0:0:1:0:S:2C:Linux 2.4.18
16D0:05B4:40:WS:0:1:1:1:S:38:SuSE Linux 8.0 Kernel 2.4.18-4GB (i686)
16D0:05B4:40:WS:1:1:0:0:A:30:Linux
16D0:05B4:40:WS:1:1:1:0:A:1C:Linux 2.4.22-gentoo-r5 
16D0:05B4:40:WS:1:1:1:0:A:30:Linux 2.4.xx
16D0:05B4:40:WS:1:1:1:0:A:30:Linux Fedora Core 1 
16D0:05B4:40:WS:1:1:1:0:A:30:Mandrake 9.2 (Linux 2.4.xx) 
16D0:05B4:80:00:1:1:1:1:S:3C:Linux 2.4.14 - 2.4.22
16D0:05B4:80:WS:0:0:0:0:A:2C:Linux
16D0:05B4:80:WS:0:0:1:0:A:2C:Windows XP / 2000
16D0:05B4:80:WS:0:0:1:0:S:2C:FreeBSD
16D0:05B4:80:WS:1:1:0:0:A:30:Linux
16D0:05B4:80:WS:1:1:1:0:A:30:Windows 95
16D0:05B4:80:WS:1:1:1:0:S:30:Windows 95
16D0:05B4:80:WS:1:1:1:0:S:LT:Windows 98 / 2000
16D0:05B4:FF:00:1:1:1:1:S:3C:Slackware Linux
16D0:05B4:FF:WS:0:0:0:0:A:2C:Linksys BEFSR11 1 Port Router/HUB
16D0:05B4:FF:WS:0:0:1:0:S:2C:Linux 2.4 
16D0:05B4:FF:WS:1:1:0:0:S:30:Windows 98
16D0:05B4:FF:WS:1:1:1:0:A:LT:Linux 2.4.10
16D0:05B4:FF:WS:1:1:1:0:S:30:Windows 98
16D0:9C05:40:WS:1:1:1:0:A:30:Linux 2.4.18
16D0:B405:40:00:1:1:1:1:S:3C:Linux 2.4.18
16D0:B405:40:00:1:1:1:1:S:LT:Redhat Linux 7.1 (Kernel 2.4.2)
16D0:B405:40:01:1:1:1:1:S:LT:SuSe 8.0 Linux 2.4.18
16D0:B405:40:WS:0:0:0:0:A:2C:HP LaserJet 4050N
16D0:B405:40:WS:0:0:0:0:A:LT:SMC Broadband / MacSense Router
16D0:B405:40:WS:0:0:1:0:A:2C:RedHat Linux 7.3 (2.4.18)
16D0:B405:FF:WS:0:0:0:0:A:2C:LinkSys Router
16D0:_MSS:80:00:0:0:0:0:A:LT:HP Color LaserJet 4500N, Jet Direct J3113A/2100
16D0:_MSS:80:00:0:1:1:0:A:LT:Windows NT4 / Win95 / Win98
16D0:_MSS:80:00:0:1:1:1:A:LT:Windows NT4 / Win95 / Win98
16D0:_MSS:80:WS:0:0:0:0:A:LT:HP Color LaserJet 4500N, Jet Direct J3113A/2100
16D0:_MSS:80:WS:0:0:1:0:A:LT:Linux 2.4.7 (X86)
1770:0578:40:WS:0:0:1:0:A:2C:SMC Router SMC7004VBR 
1800:04EC:80:WS:1:1:0:0:A:30:NetWare 6 SP3 
1800:0558:80:WS:0:0:1:0:A:LT:Novell Netware 5.1 SP3
1800:0558:80:WS:0:0:1:0:S:2C:Novel Netware 4.0
1800:05B4:40:00:0:1:1:1:S:3C:VMS
1800:05B4:80:00:0:1:1:0:A:30:Novel Netware 5.1 
1800:05B4:80:00:1:1:1:0:A:34:Novell Netware 5.1
1800:05B4:80:00:1:1:1:0:S:34:Novell Netware 6.0
1800:05B4:80:WS:0:0:1:0:A:2C:Novell Netware 4.0 / 5.0
1800:05B4:80:WS:0:0:1:0:S:2C:Novell Netware 5.1
1800:05B4:80:WS:1:1:1:0:A:30:Netware 5.1 SP5
1800:5805:80:WS:0:0:1:0:A:LT:Novell Netware 5.1
1800:_MSS:40:WS:0:0:1:0:A:LT:VMS MultiNet V4.2(16) / OpenVMS V7.1-2
1800:_MSS:80:00:0:1:0:0:A:LT:OpenVMS 6.2 - 7.2-1 on VAX or AXP
1800:_MSS:80:00:0:1:0:1:A:LT:OpenVMS 6.2 - 7.2-1 on VAX or AXP
1800:_MSS:80:00:0:1:1:0:A:LT:VMS MultiNet V4.2(16)/ OpenVMS V7.1-2
1800:_MSS:80:00:0:1:1:1:A:LT:VMS MultiNet V4.2(16)/ OpenVMS V7.1-2
1800:_MSS:80:WS:0:0:0:0:A:LT:IPAD Model 5000 or V.1.52
1800:_MSS:80:WS:0:0:1:0:A:LT:Novell Netware 5.0 SP5
1860:05B4:80:WS:1:1:1:0:A:30:Windows XP Pro
1920:05B4:40:WS:1:1:0:0:S:30:Windows 98
192F:_MSS:80:00:0:0:0:0:A:LT:Mac OS 7.0-7.1 With MacTCP 1.1.1 - 2.0.6
192F:_MSS:80:WS:0:0:0:0:A:LT:Mac OS 7.0-7.1 With MacTCP 1.1.1 - 2.0.6
1AB8:0564:40:WS:1:1:1:0:A:LT:IRIX
1C52:05AA:40:00:1:1:1:1:S:40:Windows XP Pro
1C84:05B4:40:WS:0:0:1:0:A:2C:Linux
1C84:_MSS:80:WS:0:0:0:0:A:LT:Instant Internet box
1D4C:_MSS:80:WS:0:0:0:0:A:LT:Sega Dreamcast
1F0E:_MSS:80:WS:0:0:0:0:A:LT:AmigaOS AmiTCP/IP 4.3
1FB0:0FD8:40:WS:0:0:0:0:A:2C:Actiontec ADSL modem 
1FB0:0FD8:40:WS:0:0:0:0:A:2C:Actiontec ADSL mom 
1FE0:0550:80:WS:1:1:1:0:S:30:Windows
1FFE:0218:FF:WS:0:0:0:0:A:2C:Linux Debian 
1FFE:0546:FF:WS:0:0:0:0:A:2C:Linux Debian 2.4
1FFE:055C:FF:WS:0:0:0:0:A:2C:Linux Debian woody
1FFE:0584:FF:WS:0:0:0:0:A:2C:Linux Debian 
1FFE:0596:FF:WS:0:0:0:0:A:2C:Windows
1FFE:05B4:FF:WS:0:0:0:0:A:2C:Linux Debian
1FFF:_MSS:80:00:0:0:1:0:A:LT:Novell NetWare 3.12 - 5.00
1FFF:_MSS:80:WS:0:0:1:0:A:LT:NetWare 4.11 SP7- 5 SP3A BorderManager 3.5
2000:0002:40:00:0:1:0:0:A:LT:Mac OS 9/Apple ShareIP
2000:0109:40:WS:0:0:0:0:A:LT:Cisco CacheOS 1.1.0
2000:0200:40:WS:0:0:0:0:A:2C:ForeThought ASX200BX
2000:0200:40:WS:0:0:0:0:A:LT:QNX / Amiga OS
2000:0200:80:WS:1:1:1:0:S:30:Windows XP - Windows 98
2000:020C:40:00:0:1:0:1:A:LT:OS/400
2000:0218:20:WS:1:1:0:0:S:30:Windows XP
2000:0218:40:WS:0:0:0:0:A:LT:OS/400
2000:0218:80:00:1:1:1:0:S:30:Windows 9x
2000:0218:80:WS:1:1:1:0:S:30:Windows 9x or 2000
2000:052A:80:WS:1:1:1:0:S:30:Windows 98
2000:0534:80:WS:1:1:1:0:S:30:Windows 98
2000:0550:80:WS:1:1:1:0:S:30:Windows 9x
2000:0556:80:WS:1:1:1:0:S:30:Windows 98 SE
2000:0564:40:WS:0:0:0:0:A:2C:Linux
2000:0564:80:WS:1:1:1:0:S:30:Windows 98
2000:056C:40:00:0:1:0:1:A:3C:IBM AS400
2000:0584:40:WS:0:0:0:0:A:2C:Linux (debian 2.0)
2000:0586:80:WS:1:1:1:0:S:30:Windows 9x or NT4
2000:0588:80:WS:1:1:1:0:S:30:Windows 98SE
2000:0598:80:WS:1:1:1:0:S:30:Windows 2000
2000:059C:40:00:0:1:0:1:A:3C:Windows NT
2000:059C:80:WS:1:1:1:0:S:30:Windows 98
2000:05AC:40:00:0:1:0:0:A:30:OS 400
2000:05AC:40:WS:0:0:0:0:A:2C:Windows
2000:05AC:40:WS:0:0:1:0:A:2C:AS400 
2000:05AC:80:WS:0:0:1:0:S:2C:Windows NT 4.0 
2000:05AC:80:WS:1:1:1:0:S:30:Windows 98 SE
2000:05B0:20:WS:0:0:1:0:S:2C:Windows 95
2000:05B0:80:00:1:1:1:0:S:40:Linux 2.2.13
2000:05B0:80:00:1:1:1:1:S:LT:Windows 95
2000:05B0:80:WS:0:0:0:0:A:2C:D-Link DI 614v2.2
2000:05B0:80:WS:1:1:1:0:S:30:Windows NT SP3
2000:05B4:08:WS:1:1:0:0:S:30:Windows 2000 
2000:05B4:20:00:0:0:1:0:S:2C:Windows NT 4.0
2000:05B4:20:WS:0:0:1:0:S:2C:Windows 95
2000:05B4:20:WS:1:1:0:0:S:LT:Slackware Linux 7.1 Kernel 2.2.16
2000:05B4:20:WS:1:1:1:0:S:30:Windows 98
2000:05B4:40:00:0:1:0:0:A:30:SMC Barricade Wireless router
2000:05B4:40:00:0:1:1:0:S:3C:BSDI BSD/OS 3.1
2000:05B4:40:00:0:1:1:0:S:LT:BSDI BSD/OS 3.1
2000:05B4:40:00:0:1:1:1:A:3C:FreebSD 5.0 RELEASE (x86)
2000:05B4:40:00:1:1:1:0:S:3C:BSDI BSD/OS 3.0-3.1 (or MacOS, NetBSD)
2000:05B4:40:00:1:1:1:0:S:40:WebTV netcache engine (BSDI)
2000:05B4:40:WS:0:0:0:0:A:2C:Linux
2000:05B4:40:WS:0:0:0:0:S:2C:CacheFlow 500x CacheOS 2.1.08 - 2.2.1
2000:05B4:40:WS:0:0:1:0:S:2C:AXCENT Raptor Firewall Windows NT 4.0/SP3
2000:05B4:80:00:0:0:1:0:S:2C:Windows NT 4.0
2000:05B4:80:00:0:0:1:0:S:LT:Windows NT 4.0
2000:05B4:80:00:1:1:1:0:S:2C:Windows 9x
2000:05B4:80:00:1:1:1:0:S:30:Windows 9x
2000:05B4:80:00:1:1:1:0:S:40:Windows 9x
2000:05B4:80:00:1:1:1:1:S:40:Windows 95
2000:05B4:80:WS:0:0:0:0:A:2C:D-Link DWL-900AP
2000:05B4:80:WS:0:0:0:0:S:2C:Windows NT 4.0
2000:05B4:80:WS:0:0:1:0:A:2C:Windows 2000
2000:05B4:80:WS:0:0:1:0:S:2C:Windows NT 4.0 SP6a / Windows 2000
2000:05B4:80:WS:1:0:1:0:S:2C:Windows NT
2000:05B4:80:WS:1:0:1:0:S:LT:Windows NT
2000:05B4:80:WS:1:1:0:0:S:30:Windows 95
2000:05B4:80:WS:1:1:1:0:A:30:Windows 2000
2000:05B4:80:WS:1:1:1:0:S:30:Windows 98 / 2000
2000:05B4:80:WS:1:1:1:0:S:3C:Linux 2.2.19
2000:05B4:80:WS:1:1:1:0:S:LT:Windows 98
2000:05B4:FF:WS:1:1:0:0:S:30:Windows XP SP1
2000:05B4:FF:WS:1:1:1:0:S:30:Windows 98SE 
2000:6363:80:WS:1:1:1:0:S:LT:Microsoft NT 4.0 Server SP5
2000:6D70:40:WS:0:0:0:0:A:2C:Linux
2000:B405:20:WS:0:0:0:0:A:LT:HP Ux 9.x
2000:B405:20:WS:0:0:1:0:S:LT:Windows 2000
2000:B405:40:00:0:1:0:1:A:3C:Apple AirPort Base Station
2000:B405:40:00:0:1:1:1:A:3C:windows 98
2000:B405:80:00:0:1:0:1:A:3C:Cisco VPN3002 HW Client
2000:B405:80:WS:0:0:1:0:S:LT:Windows 98 / NT
2000:B405:80:WS:1:1:1:0:S:30:Windows 98 / XP
2000:B405:FF:WS:1:1:1:0:S:30:Windows 98se
2000:_MSS:40:WS:0:0:0:0:S:LT:Mac OS 8.6
2000:_MSS:40:WS:0:0:1:0:A:LT:BSDI BSD/OS
2000:_MSS:80:00:0:0:0:0:A:LT:IBM VM/ESA 2.2.0 CMS Mainframe System
2000:_MSS:80:00:0:0:1:0:A:LT:Novell NetWare 3.12 - 5.00
2000:_MSS:80:00:0:1:0:0:A:LT:Accelerated Networks - High Speed Integrated Access VoDSL
2000:_MSS:80:00:0:1:0:1:A:LT:Tandem NSK D40
2000:_MSS:80:00:0:1:1:0:A:LT:AS/400e 720 running OS/400 R4.4
2000:_MSS:80:00:0:1:1:1:A:LT:AS/400e 720 running OS/400 R4.4
2000:_MSS:80:WS:0:0:0:0:A:LT:AGE Logic, Inc. IBM XStation
2010:_MSS:80:WS:0:0:1:0:A:LT:Windows NT / Win9x
2017:0534:80:WS:0:0:1:0:A:2C:Windows 98 SE
2017:05B4:40:00:0:1:1:1:A:LT:BSDI BSD/OS 3.0-3.1 (or possibly MacOS, NetBSD)
2017:05B4:80:WS:0:0:1:0:A:2C:Windows 98 SE / Windows NT 4.0
2017:1802:80:WS:0:0:1:0:A:LT:Windows NT 4.0
2017:_MSS:80:00:0:1:0:0:A:LT:Ascend GRF Router running Ascend Embedded/OS 2.1
2017:_MSS:80:00:0:1:1:0:A:LT:BSDI 4.0-4.0.1
2017:_MSS:80:00:0:1:1:1:A:LT:BSDI 4.0-4.0.1
2017:_MSS:80:WS:0:0:0:0:A:LT:CacheOS (CacheFlow 500-5000 webcache) CFOS 2.1.08 - 2.2.1
2017:_MSS:80:WS:0:0:1:0:A:LT:3Com NetBuilder & NetBuilder II OS v 9.3
2058:0564:40:WS:0:0:1:0:A:2C:BSDi BSD/OS 4.0.1
2058:0564:80:WS:0:0:0:0:A:2C:Windows NT
2058:0564:80:WS:0:0:1:0:A:2C:Windows 2000 / NT / Win9x
2058:05B4:80:WS:1:1:1:0:A:LT:Windows 98 SE
2058:6405:80:WS:0:0:1:0:A:LT:Windows
20D0:0578:40:00:1:1:1:1:A:40:Windows
20D0:0578:80:WS:0:0:1:0:A:2C:Windows NT 4.0 Server
2118:0584:80:WS:0:0:0:0:A:2C:Checkpoint FW-1 4.1 on Solaris 2.6 
2120:_MSS:80:WS:0:0:1:0:A:LT:Gauntlet 4.0a firewall on Solaris 2.5.1
2130:0588:80:WS:1:1:1:0:A:30:Windows 98 SE
2142:058B:40:00:1:1:1:1:A:40:Windows NT
2180:0218:80:WS:1:1:1:0:A:30:Windows 98
2180:05B0:80:WS:0:0:1:0:A:2C:Windows NT - Windows 9x
2180:05B4:20:WS:0:0:1:0:A:2C:Windows XP
2180:05B4:80:00:1:1:1:1:S:40:Windows 9x 
2180:05B4:80:WS:0:0:1:0:A:2C:Windows NT / Win9x
2180:_MSS:20:WS:0:0:1:0:A:LT:Windows NT / Win9x
2180:_MSS:40:WS:0:0:1:0:A:LT:BSDI BSD/OS
2190:05B4:20:WS:0:0:1:0:A:LT:Windows NT / Win9x
2190:05B4:80:WS:0:0:1:0:A:LT:Windows NT / Win9x
21D2:05B4:80:WS:0:0:1:0:A:LT:Windows NT / Win9x
21F0:05B4:80:00:1:1:1:1:A:40:vXWorks
21F0:05B4:80:WS:0:0:1:0:A:LT:Windows NT 4.0
2200:_MSS:80:00:0:1:0:0:A:LT:Stock OpenVMS 7.1
2200:_MSS:80:00:0:1:0:1:A:LT:Stock OpenVMS 7.1
2200:_MSS:80:00:0:1:1:0:A:LT:OpenVMS 6.2/Alpha
2200:_MSS:80:00:0:1:1:1:A:LT:OpenVMS 6.2/Alpha
2200:_MSS:80:WS:0:0:0:0:A:LT:Linux 2.0.34-38
2208:05AC:40:WS:1:1:1:0:A:30:Linux 
2208:05B4:80:WS:0:0:1:0:A:2C:Windows NT
2208:05B4:80:WS:1:1:1:0:A:30:Windows 98 SE
2220:05B0:80:WS:1:1:1:0:A:30:Windows NT
2220:_MSS:40:WS:0:0:1:0:A:LT:BSDI BSD/OS
2220:_MSS:80:WS:0:0:1:0:A:LT:Windows NT / Win9x
2229:_MSS:80:00:0:0:1:0:A:LT:Solaris 2.5, 2.5.1
2229:_MSS:80:WS:0:0:0:0:A:LT:DG/UX Release R4.11MU02
2229:_MSS:80:WS:0:0:1:0:A:LT:Solaris 2.3 - 2.4
2238:0218:40:WS:1:1:1:0:A:30:Windows 98
2238:0218:80:WS:1:1:1:0:S:30:Windows 2000 Pro
2238:0550:80:WS:1:1:1:0:S:LT:Linux
2238:0564:80:WS:0:0:1:0:A:30:Solaris 2.7
2238:0564:80:WS:1:1:1:0:S:30:Solaris 
2238:0564:FF:00:0:0:1:0:S:2C:Solaris 2.7
2238:0584:80:WS:0:0:1:0:A:30:Linux Red Hat
2238:05AC:40:WS:0:0:0:0:A:2C:Windows 98
2238:05B4:20:WS:0:0:0:0:A:LT:Snap Server (Quantum)
2238:05B4:20:WS:0:0:1:0:A:2C:Microsoft Windows 95
2238:05B4:20:WS:1:1:1:0:S:30:Windows 98
2238:05B4:40:00:0:0:1:0:S:LT:Solaris 2.6
2238:05B4:40:00:0:1:0:0:A:30:Hp jetDirect
2238:05B4:40:00:0:1:0:1:A:3C:SMC Barricade SMC7004VWBR
2238:05B4:40:00:0:1:1:1:A:3C:IRIX
2238:05B4:40:00:0:1:1:1:A:LT:BSDI BSD/OS 3.0
2238:05B4:40:00:1:1:1:1:A:40:Cisco IOS 
2238:05B4:40:WS:0:0:0:0:A:2C:Linksys WAP11 
2238:05B4:40:WS:0:0:1:0:A:2C:Cisco IOS
2238:05B4:40:WS:0:0:1:0:A:LT:BSDI BSD/OS 3.0-3.1 (or possibly MacOS, NetBSD)
2238:05B4:40:WS:1:1:1:0:A:30:Windows NT4
2238:05B4:40:WS:1:1:1:0:S:30:Windows XP SP1 + Sygate Personal Firewall 
2238:05B4:80:00:1:1:1:1:A:40:Windows 98
2238:05B4:80:WS:0:0:0:0:A:2C:Windows NT 4.x
2238:05B4:80:WS:0:0:1:0:A:2C:Windows NT 4.x / Win9x
2238:05B4:80:WS:0:0:1:0:A:30:Windows
2238:05B4:80:WS:1:1:0:0:A:30:Windows 98
2238:05B4:80:WS:1:1:1:0:A:30:Windows 98 / 2000 / XP
2238:05B4:80:WS:1:1:1:0:S:30:Windows 2000 - XP SP1
2238:05B4:FF:00:0:0:1:0:S:2C:Solaris 2.6 or 2.7
2238:05B4:FF:00:0:1:0:0:A:30:SunOS 5.7 
2238:05B4:FF:00:0:1:1:0:A:30:SunOS 5.7 
2238:05B4:FF:WS:0:0:1:0:A:2C:SunOS 5.7
2238:05B4:FF:WS:0:0:1:0:A:LT:OpenBSD
2238:05B4:FF:WS:0:0:1:0:S:2C:SunOS 5.7 Generic sun4u sparc
2238:05B4:FF:WS:0:1:1:0:S:2C:Solaris 2.6 or 2.7
2238:05B4:FF:WS:1:0:1:0:S:2C:Solaris 2.6 - 2.7
2238:05B4:FF:WS:1:1:1:0:A:30:SunOS 5.7
2238:1128:80:WS:0:0:1:0:A:2C:Novell Netware
2238:1144:80:WS:0:0:1:0:A:2C:Windows NT 4.0
2238:9805:80:00:0:1:0:1:A:3C:Windows XP Professional
2238:B405:20:WS:0:0:1:0:A:2C:Windows 95 
2238:B405:40:00:0:1:1:1:A:3C:Windows 2000 / XP
2238:B405:80:00:0:1:1:1:A:3C:Windows 95
2238:B405:80:WS:0:0:1:0:A:2C:Windows 2000
2238:B405:FF:00:0:1:1:0:A:LT:Solaris
2238:B405:FF:WS:0:0:0:0:A:2C:Solaris 2.5.1
2238:B405:FF:WS:0:0:1:0:A:LT:Solaris 2.5.1
2238:B405:FF:WS:0:0:1:0:S:LT:Solaris 7
2238:_MSS:40:WS:0:0:1:0:A:LT:BSDI BSD/OS
2238:_MSS:80:WS:0:0:0:0:A:LT:HP printer w/JetDirect card
223F:05AC:FF:WS:0:0:0:0:A:2C:Windows 98
223F:05B4:FF:WS:0:0:0:0:A:2C:Solaris 2.6
223F:7805:FF:WS:0:0:0:0:A:LT:Solaris
2274:04EC:80:WS:1:1:1:0:A:30:Windows 98
2274:04EC:FF:WS:1:1:1:0:A:30:Symantec Raptor Firewall
2274:0564:80:WS:0:0:1:0:A:2C:PIX FireWall
2284:114E:40:00:1:1:1:1:A:3C:Solaris 
2297:0109:FF:00:0:1:1:1:A:3C:Solaris 2.6 -7 (SPARC)
2297:_MSS:80:00:0:1:1:0:A:LT:Raptor Firewall 6 on Solaris 2.6
2297:_MSS:80:00:0:1:1:1:A:LT:Raptor Firewall 6 on Solaris 2.6
2328:_MSS:FF:WS:0:0:1:0:A:LT:Solaris 2.6 - 2.7
2332:_MSS:80:00:0:0:1:0:A:LT:Solaris 2.3 - 2.4
2332:_MSS:80:WS:0:0:0:0:A:LT:Solaris 2.4 w/most Sun patches
2332:_MSS:80:WS:0:0:1:0:A:LT:Solaris 2.3 - 2.4
2398:0218:FF:WS:0:0:1:0:A:2C:SunOS 5.6
239C:_MSS:80:WS:0:0:0:0:A:LT:Apollo Domain/OS SR10.4
23B4:23B4:FF:00:0:0:1:0:S:LT:Solaris 2.6
2400:_MSS:FF:WS:0:0:1:0:A:LT:Solaris 2.6 - 2.7
246C:0534:80:WS:0:0:1:0:A:2C:Windows NT 4.0
2491:_MSS:80:00:0:1:1:0:A:LT:Solaris 2.6 - 2.7 with tcp_strong_iss=0
2491:_MSS:80:00:0:1:1:1:A:LT:Solaris 2.6 - 2.7 with tcp_strong_iss=0
2530:0550:80:WS:1:1:1:0:A:30:windows 98
2530:05B4:80:WS:0:0:1:0:A:LT:Windows 2000
2544:_MSS:80:00:0:0:1:0:A:LT:Solaris 2.3 - 2.4
2568:0564:FF:00:1:1:1:1:A:40:Solaris 8 
2568:0564:FF:WS:0:1:0:1:A:38:Solaris
2568:0564:FF:WS:1:1:1:1:A:3C:Solaris
2580:05AC:80:WS:1:1:1:0:A:30:Windows 2000 Sp3 (Build2195)
25BC:0564:FF:WS:0:0:1:0:A:2C:Windows
25BC:0564:FF:WS:0:0:1:0:A:LT:Solaris
25BC:0564:FF:WS:1:1:1:0:A:30:Solaris
2648:0584:FF:00:1:1:1:1:A:40:Windows 2000 NT 
2648:8405:FF:00:0:1:1:1:A:LT:Solaris
26E2:058E:FF:WS:1:1:1:0:A:30:SunOS 5.7
2756:_MSS:80:WS:0:0:0:0:A:LT:AmigaOS AmiTCP/IP Genesis 4.6
2760:05AC:FF:00:1:1:1:1:A:40:Solaris 7 
2788:_MSS:80:00:0:1:1:0:A:LT:Solaris 2.6 - 2.7
2788:_MSS:80:00:0:1:1:1:A:LT:Solaris 2.6 - 2.7
2798:05B4:FF:00:0:1:1:0:A:LT:Solaris
2798:05B4:FF:00:0:1:1:1:A:3C:Solaris 2.6 / SunOS 5.6
2798:05B4:FF:00:1:1:1:1:A:40:SunOS 5.7
2798:B405:FF:00:0:1:0:1:A:3C:Solaris 6
2798:B405:FF:00:0:1:1:1:A:3C:Solaris 7
2798:_MSS:FF:WS:0:0:1:0:A:LT:Solaris 2.6 - 2.7
2800:05B4:80:WS:0:0:0:0:S:2C:Tiptel Innovaphone IP200 V4.00 sr4
2800:05B4:80:WS:1:1:1:0:S:30:Windows XP Professional SP1
2A20:0550:FF:00:0:1:1:1:A:3C:SunOS 5.6 sum4m sparc SUNW,SPARCstation-20
2D24:_MSS:80:00:0:1:1:1:A:LT:Linux Kernel 2.4.xx (X86)
2D25:_MSS:80:WS:0:0:0:0:A:LT:Mac OS 7.0-7.1 With MacTCP 1.1.1 - 2.0.6
2DA0:B405:40:00:0:1:0:0:A:30:HPirect J6039A
2DA0:_MSS:80:WS:0:0:0:0:A:LT:Windows 98SE + IE5.5sp1
3000:05B4:FF:WS:0:0:0:0:A:2C:BeOS 
3000:05B4:FF:WS:0:0:0:0:S:2C:BeOS 5.0
3000:05B4:FF:WS:0:1:0:0:S:2C:BeOS 5.0
3000:_MSS:80:WS:0:0:0:0:A:LT:Acorn Risc OS 3.6 (Acorn TCP/IP Stack 4.07)
37FF:_MSS:80:WS:0:0:0:0:A:LT:Linux 1.2.13
3840:0564:40:WS:0:0:0:0:S:2C:Windows 98 
3908:05B4:40:00:1:1:0:1:S:40:HPUX 11.11
3908:05B4:40:00:1:1:1:1:A:3C:IPCop v1.2.0
3908:05B4:40:WS:1:1:0:0:S:30:Windows 98 SE
3908:05B4:40:WS:1:1:1:0:A:30:Linux Debian
3908:05B4:40:WS:1:1:1:0:S:30:windows 2000 Professional
3908:05B4:80:WS:1:1:1:0:A:30:Linux 2.0.3 
3C00:_MSS:80:WS:0:0:0:0:A:LT:Linux 2.0.27 - 2.0.30
3C0A:_MSS:80:00:0:1:1:0:A:LT:Linux 2.1.19 - 2.2.17
3C0A:_MSS:80:00:0:1:1:1:A:LT:Linux 2.1.19 - 2.2.17
3CAC:0584:40:00:1:1:1:1:A:3C:Linux 2.2.16
3E43:_MSS:80:00:0:1:0:0:A:LT:AIX 4.1-4.1.5
3E43:_MSS:80:00:0:1:0:1:A:LT:AIX 4.1-4.1.5
3E64:05AC:40:00:0:1:1:1:A:3C:Debian 3.0
3E64:05AC:40:00:1:1:1:0:S:3C:Windows 98
3E64:05AC:40:00:1:1:1:1:A:4C:Linux 2.2.x 2.4.x
3E64:05AC:40:00:1:1:1:1:S:3C:Linux Debian 3.0 (kernel 2.2)
3E80:05A6:80:WS:0:0:0:0:A:2C:Windows xp
3E80:05B4:FF:WS:0:0:0:0:A:2C:CiscoATA-186
3E80:_MSS:80:WS:0:0:0:0:A:LT:Alcatel Advanced Reflexes IP Phone, Version: E/AT400/46.8
3E80:_MSS:80:WS:0:0:1:0:A:LT:VersaNet ISP-Accelerator(TM) Remote Access Server
3EBC:05B4:40:00:1:1:0:1:A:3C:Linux 2.2.17 GNU Debian/Potato
3EBC:05B4:40:00:1:1:1:0:A:34:Linux Slackware 8.0
3EBC:05B4:40:00:1:1:1:0:S:3C:Debian/Caldera Linux 2.2.x
3EBC:05B4:40:00:1:1:1:1:A:3C:Linux 2.2.19 or 2.4.17
3EBC:05B4:40:00:1:1:1:1:S:3C:Linux 2.2.16 - 2.2.19
3EBC:05B4:40:WS:0:0:0:0:A:2C:AIX 4.3.2
3EBC:05B4:40:WS:0:0:1:0:A:2C:AIX 4.3
3EBC:05B4:40:WS:0:0:1:0:A:LT:Debian GNU/Linux
3EBC:05B4:40:WS:1:1:1:0:A:30:Linux 2.2.19
3EBC:05B4:40:WS:1:1:1:0:A:LT:Linux 2.2.19
3EBC:05B4:80:WS:0:0:0:0:A:2C:Novell Netware
3EBC:B405:40:00:1:1:1:1:S:LT:Slackware Linux v7.1 - Linux Kernel 2.2.16
3ED0:0218:40:WS:0:0:1:0:A:LT:Linux Slackware 8.0
3F20:0650:40:00:1:1:1:1:S:3C:Linux 2.2.19
3F25:0109:40:00:0:1:1:1:A:3C:Linux 2.2.17 - 2.2.19
3F25:_MSS:80:00:0:1:0:0:A:LT:AIX 4.3.2.0-4.3.3.0 on an IBM RS/*
3F25:_MSS:80:00:0:1:0:1:A:LT:AIX 4.3.2.0-4.3.3.0 on an IBM RS/*
3F25:_MSS:80:00:0:1:1:0:A:LT:Linux 2.1.19 - 2.2.17
3F25:_MSS:80:00:0:1:1:1:A:LT:Linux 2.1.19 - 2.2.17
3F25:_MSS:80:WS:0:0:0:0:A:LT:AIX 3.2
3F25:_MSS:80:WS:0:0:1:0:A:LT:Linux 2.2.19
3FE0:05B4:40:WS:0:0:0:0:A:2C:Caldera OpenLinux(TM) 1.3 / RedHat 7.2 / FreeSCO 0.2.7
3FE0:_MSS:80:00:0:0:0:0:A:LT:Linux 2.0.32-34
3FF0:_MSS:80:00:0:0:0:0:A:LT:Linux 2.0.34-38
3FF0:_MSS:80:WS:0:0:0:0:A:LT:AtheOS ( www.atheos.cx )
3FFF:_MSS:80:WS:0:0:0:0:A:LT:IBM MVS (unknown version)
4000:0000:40:WS:0:0:0:0:S:LT:ULTRIX V4.5 (Rev. 47)
4000:01F4:80:WS:1:1:0:0:A:30:Windows 
4000:0200:40:00:0:0:0:0:S:2C:AIX 3.2, 4.2 - 4.3
4000:0200:40:00:0:1:0:0:S:3C:OpenBSD 2.6-2.8
4000:0200:40:00:0:1:0:1:A:3C:IPSO 3.3 / Net BSD 1.5.2
4000:0200:40:00:0:1:0:1:S:LT:OpenBSD
4000:0200:40:00:1:1:0:1:S:LT:FreeBSD
4000:0200:40:WS:0:0:0:0:A:2C:AIX 4.3
4000:0200:40:WS:0:0:0:0:S:LT:BorderWare 5.2
4000:0200:FF:WS:0:0:0:0:A:2C:Windows 2000
4000:0218:80:00:1:1:1:1:S:LT:Windows XP Home
4000:023C:40:00:1:1:1:0:S:40:OpenBSD 3.0
4000:023C:80:WS:1:1:1:0:S:30:Windows NT SP4+
4000:0400:40:00:0:1:0:1:A:3C:IPSO 3.7
4000:0400:40:WS:0:0:0:0:A:2C:Xerox 440 Document center
4000:04EC:80:WS:1:1:1:0:S:30:Windows 2000
4000:04F8:80:WS:1:1:1:0:S:30:Windows NT SP3
4000:0528:80:WS:1:1:1:0:S:30:Windows XP pro
4000:052A:80:WS:1:1:0:0:S:30:Windows 2000 Server 
4000:0530:80:WS:1:1:1:0:S:30:Windows 2000
4000:0534:80:WS:1:1:1:0:S:30:Windows 2000
4000:0542:80:WS:1:1:1:0:S:30:Windows ME
4000:0550:40:WS:0:0:0:0:A:2C:Windows 2000
4000:0550:40:WS:0:0:1:0:A:2C:NetBSD
4000:0550:80:WS:0:0:0:0:A:2C:Mac OS 
4000:0550:80:WS:0:0:1:0:A:2C:Windows 2000
4000:0550:80:WS:1:1:1:0:S:30:Windows 2000
4000:0556:80:WS:1:1:1:0:S:30:Windows XP
4000:0562:80:WS:1:1:1:0:S:30:Windows 2000 
4000:0564:80:WS:1:1:0:0:S:30:Windows 2000
4000:0564:80:WS:1:1:1:0:S:LT:Windows 2000
4000:0578:40:00:0:1:0:1:A:3C:NetBSD 1.6
4000:0578:40:00:1:1:1:1:S:40:OpenBSD 3.0
4000:0578:80:WS:1:1:1:0:S:30:Windows 2000
4000:0578:FF:00:1:1:0:1:A:40:Windows 2000 / 2003
4000:057A:80:WS:1:1:1:0:S:30:Windows 2000
4000:057E:80:WS:1:1:1:0:S:30:Windows XP (Home) 
4000:0582:80:WS:1:1:1:0:S:30:Microsoft Windows 2000 Professional SP4 
4000:0584:20:WS:1:1:0:0:S:30:Windows ME
4000:0584:80:00:1:1:0:1:A:40:Windows 
4000:0584:80:WS:1:1:1:0:S:30:Windows 2000 
4000:0586:80:WS:1:1:1:0:S:30:Windows 2000
4000:0588:80:WS:1:1:1:0:S:30:Windows ME
4000:058A:80:WS:1:1:1:0:S:30:Windows 2000
4000:058C:80:WS:1:1:1:0:S:30:Free BSD 
4000:0596:80:WS:1:1:1:0:S:30:Linux Red Hat
4000:0598:40:WS:0:0:0:0:A:2C:Linux Debian 3.0 
4000:0598:40:WS:1:1:0:0:A:30:Windows 
4000:0598:80:00:1:1:1:1:S:40:Windows XP
4000:0598:80:WS:1:1:1:0:S:30:Windows 2000 
4000:05A0:80:WS:1:1:1:0:S:30:Windows XP Pro
4000:05A4:80:WS:1:1:1:0:S:30:Windows
4000:05AC:40:00:1:1:1:1:S:40:OpenBSD 3.1
4000:05AC:80:WS:1:1:0:0:A:30:Windows 2000
4000:05AC:80:WS:1:1:1:0:S:30:Windows 2000 / XP
4000:05B0:80:WS:1:1:1:0:S:30:Window 2000 pro. SP2
4000:05B4:01:WS:1:1:1:0:S:30:Windows 2003 
4000:05B4:20:WS:1:1:1:0:S:30:Windows
4000:05B4:40:00:0:0:1:0:S:2C:FreeBSD 4.0-STABLE, 3.2-RELEASE
4000:05B4:40:00:0:1:0:0:S:3C:NetBSD 1.3/i386
4000:05B4:40:00:0:1:0:1:A:3C:NetBSD 1.5.2 (GENERIC)
4000:05B4:40:00:0:1:0:1:S:3C:NetBSD 1.5 (x86)
4000:05B4:40:00:0:1:1:0:S:2C:FreeBSD 2.2.8-RELEASE
4000:05B4:40:00:0:1:1:0:S:3C:Linux 2.4.2 - 2.4.14
4000:05B4:40:00:0:1:1:0:S:44:FreeBSD 4.3 - 4.4PRERELEASE
4000:05B4:40:00:0:1:1:0:S:LT:FreeBSD 2.2.8-RELEASE
4000:05B4:40:00:0:1:1:1:A:3C:NetBSD 1.6U
4000:05B4:40:00:0:1:1:1:S:3C:FreeBSD 4.4
4000:05B4:40:00:1:1:0:1:S:40:OpenBSD 3.2 
4000:05B4:40:00:1:1:1:1:A:40:OpenBSD 3.5 
4000:05B4:40:00:1:1:1:1:S:40:OpenBSD 3.0
4000:05B4:40:00:1:1:1:1:S:48:Amiga OS / Miami Deluxe 1.0c
4000:05B4:40:5E:0:1:1:0:S:2C:FreeBSD 4.0-STABLE, 3.2-RELEASE
4000:05B4:40:62:0:0:1:0:S:2C:FreeBSD 4.0-STABLE, 3.2-RELEASE
4000:05B4:40:70:0:0:1:0:S:2C:FreeBSD 4.0-STABLE, 3.2-RELEASE
4000:05B4:40:WS:0:0:0:0:A:2C:NetBSD 1.3 - 1.33 / AIX 4.3.X
4000:05B4:40:WS:0:0:0:0:S:2C:AIX 4.3 - 4.3.3
4000:05B4:40:WS:0:0:1:0:A:2C:NetBSD
4000:05B4:40:WS:0:0:1:0:S:2C:FreeBSD 4.2 - 4.3
4000:05B4:40:WS:1:1:0:0:A:30:Windows
4000:05B4:40:WS:1:1:1:0:S:30:Windows 2000 sp3
4000:05B4:80:00:0:1:1:1:A:LT:Linux Kernel 2.4.xx (X86)
4000:05B4:80:00:1:1:0:1:A:40:Windows Server 2003
4000:05B4:80:00:1:1:1:0:S:30:Windows 2000
4000:05B4:80:00:1:1:1:1:A:40:Windows 2000
4000:05B4:80:00:1:1:1:1:S:40:Windows 2000 Pro
4000:05B4:80:4B:1:1:1:0:S:30:Windows ME
4000:05B4:80:WS:0:0:0:0:A:2C:Windows 2000 
4000:05B4:80:WS:0:0:0:0:S:2C:Windows Longhorn
4000:05B4:80:WS:0:0:1:0:A:2C:Windows 2000
4000:05B4:80:WS:0:1:1:0:S:30:Windows XP 
4000:05B4:80:WS:1:1:0:0:A:30:Windows Server 2003
4000:05B4:80:WS:1:1:0:0:S:30:Windows 2000
4000:05B4:80:WS:1:1:0:0:S:LT:BeOS
4000:05B4:80:WS:1:1:1:0:A:30:Windows XP
4000:05B4:80:WS:1:1:1:0:S:30:Windows ME / 2000 / XP
4000:05B4:80:WS:1:1:1:0:S:3C:Linux RedHat 7.2 (kernel 2.4.9)
4000:05B4:80:WS:1:1:1:0:S:LT:Windows XP / 2000 / ME
4000:05B4:FF:00:0:1:1:0:S:LT:FreeBSD 2.2.6-RELEASE
4000:05B4:FF:WS:0:0:0:0:A:LT:Cisco Systems IOS 11.3
4000:05B4:FF:WS:1:1:1:0:S:30:Windows 2000
4000:0650:40:00:0:1:1:1:A:3C:NetBSD 1.6
4000:0FB0:80:WS:1:1:1:0:S:LT:Windows 2000 Professional 
4000:3605:80:WS:1:1:1:0:S:30:Windows XP
4000:5005:40:WS:0:0:0:0:A:2C:Mac OS X
4000:5005:80:WS:0:0:0:0:A:2C:Windows 2000
4000:62BB:80:WS:1:1:1:0:S:LT:Windows 2000
4000:B405:40:00:0:1:0:1:A:3C:NetBSD 1.5 
4000:B405:40:00:0:1:1:1:S:LT:FreeBSD
4000:B405:80:00:0:1:0:1:A:3C:Windows Server 2003
4000:B405:80:WS:1:1:1:0:S:30:Windows XP / 2000 / ME
4000:B405:80:WS:1:1:1:0:S:LT:Windows XP - 2000
4000:D84A:80:WS:1:1:1:0:S:30:Windows 2000
4000:_MSS:80:00:0:0:0:0:A:LT:IBM MVS (unknown version)
4000:_MSS:80:00:0:0:1:0:A:LT:OpenVMS 7.1 using Process Software's TCPWare 5.3
4000:_MSS:80:00:0:1:0:0:A:LT:Check Point FireWall-1 4.0 SP-5 (IPSO build)
4000:_MSS:80:00:0:1:0:1:A:LT:Check Point FireWall-1 4.0 SP-5 (IPSO build)
4000:_MSS:80:WS:0:0:0:0:A:LT:Auspex Fileserver (AuspexOS 1.9.1/SunOS 4.1.4)
4000:_MSS:80:WS:0:0:0:0:S:28:Windows XP 
4000:_MSS:80:WS:0:0:1:0:A:LT:AmigaOS Miami 3.0
4020:0564:40:00:0:1:0:1:A:3C:OpenBSD
402E:0218:80:00:0:1:1:1:A:3C:Windows Millenium
402E:05B4:80:00:0:1:0:1:A:3C:Linux
402E:05B4:80:00:0:1:1:1:A:3C:Windows 2000 Professional / Windows XP
402E:5005:80:00:0:1:1:1:A:3C:Windows XP / 2000 / ME
402E:B405:40:WS:0:0:1:0:A:LT:Free BSD 4.1.1 - 4.3 X86
402E:_MSS:80:00:0:1:0:0:A:LT:FreeBSD 2.1.0 - 2.1.5
402E:_MSS:80:00:0:1:0:1:A:LT:FreeBSD 2.1.0 - 2.1.5
402E:_MSS:80:00:0:1:1:0:A:LT:D-Link DI-701, Version 2.22
402E:_MSS:80:00:0:1:1:1:A:LT:D-Link DI-701, Version 2.22
402E:_MSS:80:WS:0:0:0:0:A:LT:OpenBSD 2.1/X86
402E:_MSS:80:WS:0:0:1:0:A:LT:AmigaOS Miami 2.1-3.0
402E:_MSS:80:WS:0:1:1:0:A:LT:Windows XP Professional Release
403D:_MSS:80:00:0:1:0:0:A:LT:FreeBSD 2.1.0 - 2.1.5
403D:_MSS:80:00:0:1:0:1:A:LT:FreeBSD 2.1.0 - 2.1.5
403D:_MSS:80:00:0:1:1:0:A:LT:Acorn RiscOS 3.7 using AcornNet TCP/IP stack
403D:_MSS:80:00:0:1:1:1:A:LT:Acorn RiscOS 3.7 using AcornNet TCP/IP stack
4074:01F4:40:00:0:1:0:1:A:3C:OpenBSD 3.2
4074:_MSS:40:WS:0:0:0:0:A:LT:OpenBSD 2.x
4088:05B4:40:WS:0:0:1:0:A:LT:FreeBSD
40B0:0564:80:WS:1:1:1:0:A:30:Windows 2000 (firewalled)
40B0:05B4:40:WS:0:0:1:0:A:2C:FreeBSD 4.3
40E8:0218:80:WS:1:1:1:0:A:LT:Windows ME
40E8:04EC:80:WS:0:0:1:0:A:2C:Windows 2000 Pro
40E8:0584:80:WS:0:0:1:0:A:2C:Windows 2000
40E8:05B4:40:00:1:1:0:1:A:40:Windows 
40E8:05B4:80:WS:0:0:1:0:A:2C:Windows 2000 Server
40E8:05B4:80:WS:1:1:1:0:A:30:Windows XP 
40E8:05B4:FF:00:0:0:1:0:S:30:Mac OS 7.x-9.x
40E8:B405:80:WS:0:0:1:0:A:2C:Windows 2000
40E8:B405:FF:00:0:0:1:0:S:LT:Mac OS 8.6
4150:_MSS:40:WS:0:0:1:0:A:LT:Cisco Localdirector 430, running OS 2.1
41A0:0578:80:00:1:1:1:1:A:40:Windows XP SP1
41A0:0584:40:00:1:1:1:1:A:40:Windos XP
41B8:05B4:80:WS:1:1:1:0:A:30:Windows 2000 server
41E8:057E:80:00:1:1:1:1:A:40:Windows 2000
4230:0584:80:00:1:1:0:1:A:40:Linux
4230:0584:80:00:1:1:1:1:A:40:Windows 2000 Server
4230:0584:80:WS:1:1:0:0:A:30:Windows 2000 Server
4230:0584:80:WS:1:1:1:0:A:30:Windows 2000 Server
4230:05B4:80:WS:1:1:1:0:A:30:Windows XP
4238:0518:40:WS:0:0:0:0:S:2C:Xbox
4240:05B4:40:04:1:1:1:0:S:34:Windows XP
4240:05B4:80:04:1:1:1:0:S:34:Windows 2000 Pro
4240:B405:80:04:1:1:1:0:S:34:Windows XP
4240:_MSS:80:00:0:0:1:0:A:LT:MacOS 8.1
4248:0586:80:00:1:1:1:1:A:40:GNU / Olli OS
4248:0586:80:WS:1:1:1:0:A:30:Windows 2000 server
4290:058C:40:WS:0:0:1:0:A:2C:Windows 2000
4290:05B4:80:WS:1:1:1:0:A:30:Windows 2000 Pro SP3
4322:052A:40:WS:0:0:1:0:A:2C:Windows 
4322:05B4:80:WS:1:1:1:0:A:30:Linksys Router 
4350:9C05:80:WS:1:1:1:0:A:30:Windows Server 2003
4380:05A0:80:WS:1:1:1:0:A:30:Openbsd
4380:05B4:80:WS:1:1:1:0:A:30:Windows 2003 Server
4380:A005:80:WS:1:1:1:0:A:30:Open BSD
43B0:05A4:80:WS:1:1:1:0:A:30:OpenBSD 2.8 GENERIC 
43E0:05B4:40:00:0:1:0:1:A:LT:OpenBSD 2.8 GENERIC
43E0:05B4:40:00:0:1:1:0:A:LT:FreeBSD 4.x
43E0:05B4:40:00:0:1:1:1:A:3C:FreeBSD 4.4-Release
43E0:05B4:40:00:1:1:0:1:A:40:OpenBSD 3.4
43E0:05B4:40:00:1:1:1:1:A:40:OpenBSD 2.9 3.0
43E0:A805:40:00:0:1:0:1:A:LT:OpenBSD 2.6
43E0:B405:40:00:0:1:0:1:A:LT:OpenBSD 2.9
43E0:B405:40:00:0:1:1:1:A:3C:FreeBSD 4.4 / OpenBSD 3.1
43E0:_MSS:40:WS:0:0:0:0:A:LT:OpenBSD 2.x
43E0:_MSS:40:WS:0:0:1:0:A:LT:FreeBSD 2.2.1 - 4.0
43F8:AA05:40:WS:0:0:1:0:A:2C:Free BSD
4410:05AC:40:WS:1:1:1:0:A:30:OpenBSD 3.0 
4410:05AC:80:00:1:1:1:1:A:LT:Windows 2000 
4410:05AC:80:WS:1:1:1:0:A:30:Windows 2000 Workstation / Windows 98 SE
4410:05B4:80:00:1:1:1:0:A:34:Windows 2000
4410:05B4:80:00:1:1:1:1:A:40:Windows 2000
4410:05B4:80:WS:1:1:1:0:A:30:Windows 2000
4431:_MSS:80:00:0:1:1:0:A:LT:Solaris 2.6 - 2.7
4431:_MSS:80:00:0:1:1:1:A:LT:Solaris 2.6 - 2.7
4440:05B0:80:00:1:1:1:1:A:40:Windows 2000 (Advanced Server)
4440:05B0:80:WS:1:1:1:0:A:30:Solaris 7
4440:05B4:80:WS:1:1:1:0:A:30:Windows 2000 Terminal Server
4440:05B4:80:WS:1:1:1:0:A:LT:Solaris 7
4452:_MSS:80:00:0:0:1:0:A:LT:Solaris 2.5, 2.5.1
4470:05B4:40:00:0:1:0:1:A:3C:Windows 2000
4470:05B4:40:00:0:1:1:1:A:3C:WINDOWS 2000
4470:05B4:40:00:1:1:0:0:A:LT:Windows 2000
4470:05B4:40:00:1:1:0:1:A:40:Windows NT
4470:05B4:40:00:1:1:1:0:A:34:OpenBSD 3.0
4470:05B4:40:00:1:1:1:1:A:3C:Linux
4470:05B4:40:00:1:1:1:1:A:40:Windows NT
4470:05B4:40:WS:0:0:0:0:A:LT:FreeBSD NFR Network Flight Recorder
4470:05B4:40:WS:0:0:1:0:A:2C:FreeBSD 4.2 / FreeBSD 3.4-STABLE
4470:05B4:40:WS:1:1:0:0:A:30:OpenBSD 2.8
4470:05B4:40:WS:1:1:1:0:A:30:OpenBSD 3.0 - 3.1
4470:05B4:80:00:0:1:1:0:A:LT:Windows 2000
4470:05B4:80:00:0:1:1:1:A:3C:Windows NT4 / Win95 / Win98
4470:05B4:80:00:1:1:0:1:A:40:Linux 2.2.14 - 2.2.20
4470:05B4:80:00:1:1:0:1:A:LT:Windows 2000 Professional
4470:05B4:80:00:1:1:1:0:A:34:Windows 2000 Server
4470:05B4:80:00:1:1:1:1:A:40:Windows 2000 Pro / XP Pro / 2003 Server
4470:05B4:80:WS:0:0:1:0:A:2C:Windows 2000
4470:05B4:80:WS:1:1:0:0:A:30:Windows 2000 pro
4470:05B4:80:WS:1:1:1:0:A:30:Windows 2000 Server / XP Pro / NT 4.0 server
4470:05B4:80:WS:1:1:1:0:A:3C:FreeBSD 4.x
4470:05B4:80:WS:1:1:1:0:S:30:Windows Server 2003 Enterprise Edition
4470:05B4:80:WS:1:1:1:1:S:3C:Windows 2000 Advance Server
4470:05B4:FF:WS:0:0:1:0:A:2C:Windows 2000
4470:05B4:FF:WS:1:1:1:0:A:30:Windows XP
4470:B405:40:00:0:1:1:1:A:3C:Windows NT 2000/5.0
4470:B405:80:00:0:1:0:1:A:3C:Windows ME
4470:B405:80:00:0:1:1:1:A:3C:Windows 2000 Workstation / XP Home
4470:B405:80:00:0:1:1:1:A:LT:Windows Server 2003 
4470:B405:80:00:1:1:1:0:A:34:Windows XP SP1
4470:B405:80:WS:1:1:0:0:A:30:Mac OS 8.6
4470:B405:80:WS:1:1:1:0:A:30:Windows XP
4470:B405:FF:00:0:0:1:0:A:LT:Mac OS 8.6
4470:_MSS:40:WS:0:0:0:0:A:LT:FreeBSD 2.2.1 - 4.0
4470:_MSS:40:WS:0:0:1:0:A:LT:FreeBSD 2.2.1 - 4.0
4470:_MSS:80:WS:0:0:0:0:A:LT:Snap Network Box
44E8:04EC:80:00:1:1:1:1:A:40:Windows 2000 pro
44E8:04EC:80:WS:1:1:1:0:A:30:Windows 2000 Pro
4510:0550:80:00:1:1:1:1:A:40:Windows 2000
4510:0550:80:WS:0:0:1:0:A:LT:Windows 2000 Professional 5.0.2195 Service Pack 2
4510:0550:80:WS:1:1:1:0:A:30:Windows 2000 Professional 5.0.2195 Service Pack 2
4510:05B4:FF:WS:1:1:1:0:A:30:Windows XP Pro 
455B:_MSS:80:00:0:0:0:0:A:LT:MacOS 8.1 running on a PowerPC G3 (iMac)
455B:_MSS:80:00:0:0:1:0:A:LT:Mac OS 8.6
462B:_MSS:80:00:0:1:1:0:A:LT:Solaris 2.6 - 7 X86
462B:_MSS:80:00:0:1:1:1:A:LT:Solaris 2.6 - 7 X86
4764:23B2:40:00:1:1:1:1:S:3C:Linux 2.4.xx
4860:0218:80:WS:1:1:1:0:S:30:Windows XP
4860:058E:10:WS:1:1:1:0:S:30:Linux 
4860:05B4:80:WS:1:1:1:0:S:30:Windows 2000
4EC0:05B4:80:00:0:1:1:1:A:3C:Windows XP Pro SP1
4F68:05AC:80:WS:1:1:1:0:S:30:Windows 2000 Professional 
4FD8:05B4:40:00:1:1:1:0:S:34:Windows XP Professional
5B40:_MSS:80:WS:0:0:0:0:A:LT:Polycom ViewStation 512K videoconferencing system
6000:_MSS:80:00:0:0:0:0:A:LT:SCO OpenServer(TM) Release 5
6000:_MSS:80:00:0:0:1:0:A:LT:OpenVMS v7.1 VAX
6000:_MSS:80:00:0:1:1:0:A:LT:Sequent DYNIX/ptx(R) V4.4.6
6028:05B4:40:00:0:1:1:1:A:3C:Solaris 8
6028:05B4:40:00:1:1:1:1:A:40:Solaris 7 / 8
6028:B405:40:00:0:1:1:1:A:3C:Solaris 8
6030:0564:40:00:1:1:1:1:A:40:Solaris 8 
6030:6405:40:00:0:1:1:1:A:3C:Solaris 8
6050:05B4:40:00:1:1:1:0:A:34:Windows
6050:05B4:40:WS:0:0:1:0:A:2C:Solaris 9
6050:05B4:FF:WS:0:0:0:0:A:2C:Solaris 8
606C:00D8:40:00:1:1:1:1:A:40:Solaris 9
60DA:0584:40:00:0:1:1:1:A:3C:Solaris 8
60DA:_MSS:80:00:0:1:1:0:A:LT:Sun Solaris 8 early acces beta through actual release
60DA:_MSS:80:00:0:1:1:1:A:LT:Sun Solaris 8 early acces beta through actual release
60F4:05B4:40:00:0:0:1:0:S:LT:SCO UnixWare 7.0.1
60F4:05B4:40:00:0:1:1:0:A:30:SunOS 5.8 
60F4:05B4:40:00:0:1:1:0:S:LT:SCO UnixWare 7.1.0 x86
60F4:05B4:40:00:0:1:1:1:A:3C:SCO UnixWare 7.1.1
60F4:05B4:40:00:1:1:1:0:A:34:SunOS 5.8
60F4:05B4:40:WS:0:0:0:0:A:2C:SCO openserver 5.0.5
60F4:05B4:40:WS:0:0:0:0:S:2C:SCO openserver 5.0.6
60F4:05B4:40:WS:0:0:1:0:A:2C:Windows 2000
60F4:05B4:40:WS:0:0:1:0:S:2C:SCO Openserver 5
60F4:05B4:40:WS:1:1:0:0:S:30:SunOS 5.8
60F4:05B4:40:WS:1:1:1:0:A:30:SunOS 5.8
60F4:05B4:40:WS:1:1:1:0:S:30:Solaris 8
60F4:05B4:40:WS:1:1:1:0:S:LT:Solaris 8
60F4:B405:40:WS:1:1:1:0:S:LT:Sun
60F4:_MSS:40:WS:0:0:1:0:A:LT:SCO OpenServer 5.0.5
60F4:_MSS:80:00:0:1:0:0:A:LT:NCR MP-RAS SVR4 UNIX System Version 3
60F4:_MSS:80:00:0:1:0:1:A:LT:NCR MP-RAS SVR4 UNIX System Version 3
60F4:_MSS:80:00:0:1:1:0:A:LT:NCR MP-RAS 3.01
60F4:_MSS:80:00:0:1:1:1:A:LT:NCR MP-RAS 3.01
60F4:_MSS:80:WS:0:0:0:0:A:LT:SCO UnixWare 7.0.0 or OpenServer 5.0.4-5
6108:0564:40:WS:1:1:0:0:A:30:Windows 95 / 98 
61A8:0200:40:00:1:1:1:1:A:LT:Windows NT
61A8:_MSS:80:00:0:1:1:0:A:LT:Windows NT4 / Win95 / Win98
61A8:_MSS:80:00:0:1:1:1:A:LT:Windows NT4 / Win95 / Win98
6270:04EC:80:00:1:1:1:0:S:34:Windows 2000
6270:04EC:80:WS:1:1:1:0:A:30:Linux Red Hat 
6270:04EC:80:WS:1:1:1:0:S:30:Windows XP
6270:0564:40:WS:1:1:1:0:A:30:Windows
6270:0584:40:00:1:1:1:1:A:40:SunOS 5.8
6270:05AC:80:00:1:1:1:1:A:40:Solaris
6270:05B4:40:00:1:1:1:0:S:34:Windows 2000
6270:05B4:80:WS:1:1:1:0:S:30:Windows XP
6348:0584:40:WS:1:1:1:0:A:30:Solaris/Ultra sparc 
6420:_MSS:40:WS:0:1:1:0:A:3C:SunOS 5.8 
64F0:055C:40:WS:1:1:1:1:A:3C:SunOS
6540:05B4:40:00:1:1:1:1:A:40:Windows XP 
6978:05B4:80:WS:1:1:1:0:S:30:Windows XP
6C68:05B4:40:02:1:1:1:0:S:34:Windows XP
6FCC:_MSS:80:WS:0:0:0:0:A:LT:IBM OS/2 V 2.1
7000:05B4:40:WS:0:0:0:0:A:2C:IBM OS/2
7000:05B4:40:WS:0:0:0:0:S:2C:IBM OS/2 V.3
7000:_MSS:80:WS:0:0:0:0:A:LT:IBM OS/2 V.3
70D5:_MSS:80:00:0:1:1:0:A:LT:Digital UNIX OSF1 V 4.0-4.0F
7210:04EC:40:00:1:1:1:1:S:40:Windows 98
7210:05B4:80:WS:0:0:1:0:A:2C:Windows NT server 4.0 sp6a
77C4:05B4:40:00:0:1:1:0:A:30:Linux 2.2.19
77C4:05B4:40:00:1:1:0:1:A:3C:Linux 2.2.22 (PLD)
77C4:05B4:40:00:1:1:1:1:A:3C:Linux SuSE 7.3
77C4:05B4:40:00:1:1:1:1:A:LT:Linux SuSE 7.x
77C4:05B4:40:WS:0:0:1:0:A:2C:SuSE Linux 7.1
77C4:05B4:40:WS:1:1:1:0:A:30:Linux Mandrake 7.1 / Debian 3.0
77C4:B405:40:00:0:1:1:1:A:3C:Linux
77C4:_MSS:40:WS:0:0:1:0:A:LT:Linux 2.1.122 - 2.2.14
7850:0578:40:WS:1:1:1:0:A:30:Suse Linux 7.0
7900:_MSS:80:00:0:1:1:0:A:LT:Atari Mega STE running JIS-68k 3.0
7900:_MSS:80:00:0:1:1:1:A:LT:Atari Mega STE running JIS-68k 3.0
7958:0584:40:00:1:1:1:0:A:34:Linux Mandrake 7.2
7958:0584:40:00:1:1:1:1:A:3C:Windows NT 4
7958:0584:40:WS:1:1:1:0:A:30:Linux 2.2.16-3 (RH 6.2)
7960:0F2C:40:00:1:1:1:0:S:LT:Linux 2.2.12-20
7B10:0598:40:00:1:1:1:1:A:3C:Linux 2.2.18pre21
7B2F:_MSS:80:00:0:1:1:0:A:LT:Linux 2.1.19 - 2.2.17
7B2F:_MSS:80:00:0:1:1:1:A:LT:Linux 2.1.19 - 2.2.17
7B44:055C:40:WS:1:1:1:0:A:30:Debian/GNU Linux
7B88:0218:40:WS:0:0:1:0:A:2C:Reliant Unix from Siemens-Nixdorf
7BC0:_MSS:40:WS:0:0:1:0:A:LT:Linux 2.1.122 - 2.2.14
7BF0:052A:40:WS:1:1:1:0:A:30:PLD Linux
7BF0:2A05:40:WS:1:1:1:0:A:30:Linux 2.2.19
7BF0:_MSS:40:WS:0:0:1:0:A:LT:Linux 2.1.122 - 2.2.14
7BFC:0564:40:00:1:1:1:1:A:3C:Slackware Linux 8.0
7BFC:0564:40:00:1:1:1:1:A:LT:CISCO PIX 6.1
7BFC:0564:40:WS:0:0:1:0:A:2C:Linux
7BFC:0564:40:WS:1:0:1:1:A:38:Linux
7BFC:0564:40:WS:1:1:1:0:A:LT:Cisco PIX
7BFC:6405:40:00:0:1:1:1:A:3C:Linux
7C00:_MSS:80:00:0:0:0:0:A:LT:Linux 2.0.27 - 2.0.30
7C00:_MSS:80:WS:0:0:0:0:A:LT:Convex OS Release 10.1
7C38:_MSS:80:00:0:1:1:0:A:LT:Linux 2.1.19 - 2.2.17
7C38:_MSS:80:00:0:1:1:1:A:LT:Linux 2.1.19 - 2.2.17
7C70:05B4:40:00:1:1:1:0:S:LT:Linux 2.3.99-ac - 2.4.0-test1
7C70:_MSS:80:00:0:1:1:0:A:LT:Linux 2.3.28-33
7C70:_MSS:80:00:0:1:1:1:A:LT:Linux 2.3.28-33
7C9C:05AA:40:WS:0:0:1:0:A:2C:SuSE Linux 7.0
7C9C:AA05:40:00:0:1:1:1:A:LT:Linux RedHat 7.1
7CC8:0584:40:00:1:1:1:0:S:3C:Linux 2.2
7D00:05A0:80:00:1:1:1:0:S:34:Windows
7D00:05A0:80:WS:1:1:1:0:S:30:OpenBSD
7D00:05B4:80:WS:1:1:1:0:S:30:Windows XP Pro
7D78:0109:80:10:0:1:0:1:S:LT:OpenBSD 2.9 generic
7D78:0564:40:00:1:1:1:1:S:3C:Linux
7D78:0564:40:WS:1:1:1:1:S:3C:Slware Linux 7.1
7D78:05AC:40:00:1:1:1:0:A:34:OpenBSD 2.9
7D78:05B4:01:00:1:1:1:1:S:3C:Linux 
7D78:05B4:20:00:1:1:1:0:S:LT:Linux 2.2.13
7D78:05B4:3A:WS:0:0:0:0:S:LT:Linux 2.0.38
7D78:05B4:40:00:0:1:1:0:A:30:RedHat 6.2
7D78:05B4:40:00:0:1:1:0:S:LT:Linux 2.2.19
7D78:05B4:40:00:0:1:1:1:A:3C:Linux 2.2.17 - 2.2.20
7D78:05B4:40:00:1:1:0:0:S:LT:Linux 2.2.19
7D78:05B4:40:00:1:1:0:1:A:3C:Linux pld 2.4.22
7D78:05B4:40:00:1:1:1:0:A:34:Linux 2.2.19 (Mandrake Secure)
7D78:05B4:40:00:1:1:1:0:A:LT:Linux 2.2.19 - 2.2.20
7D78:05B4:40:00:1:1:1:0:S:34:Linux 2.2.19 (Mandrake Secure)
7D78:05B4:40:00:1:1:1:0:S:3C:Linux 2.2.9 - 2.2.18
7D78:05B4:40:00:1:1:1:0:S:LT:Linux 2.2.14 - 2.2.20
7D78:05B4:40:00:1:1:1:1:A:3C:Linux 2.2.14 - 2.2.20
7D78:05B4:40:00:1:1:1:1:S:3C:Linux 2.2.14 - 2.2.20
7D78:05B4:40:09:1:1:1:0:S:3C:Linux 2.2.x
7D78:05B4:40:BE:1:1:1:0:S:3C:Linux 2.2.16
7D78:05B4:40:WS:0:0:0:0:S:2C:Linux 2.0.33
7D78:05B4:40:WS:0:0:1:0:A:2C:Cobalt Linux RaQ 4 (2.2.19)
7D78:05B4:40:WS:0:0:1:0:A:LT:Linux 2.2.19 - 2.2.20
7D78:05B4:40:WS:1:0:1:1:A:38:Linux 2.2.16-22
7D78:05B4:40:WS:1:1:0:0:A:30:Linux 2.2.(PLD) 
7D78:05B4:40:WS:1:1:1:0:A:30:Linux 2.2.12 - 2.2.20
7D78:05B4:80:WS:0:0:1:0:A:LT:Windows XP
7D78:05B4:80:WS:1:1:1:0:A:30:Windows 2000 sp2
7D78:05B4:80:WS:1:1:1:0:S:30:Windows XP Professional
7D78:05B4:FF:00:1:1:1:1:A:3C:Linux 2.2.18
7D78:B405:40:00:0:1:1:0:A:30:Yellow Dog Linux 2.2
7D78:B405:40:00:0:1:1:1:A:3C:Cobalt
7D78:B405:40:00:1:1:1:1:A:3C:Linux Redhat 6.2 Zoot (Kernel 2.2.14-5)
7D78:B405:40:00:1:1:1:1:S:LT:Linux 2.2.14
7D78:_MSS:80:WS:0:0:0:0:A:28:Linux 
7D78:_MSS:80:WS:0:0:0:0:S:28:Linux 2.4.19 crypto gentoo
7DC8:0578:40:WS:1:1:1:0:A:30:Suse Linux
7E18:_MSS:80:00:0:1:1:0:A:LT:Linux Kernel 2.4.0-test5
7EA0:3F5C:40:00:1:1:1:1:A:3C:Linux Kernel 2.4.10
7EB8:3F5C:40:00:1:1:1:1:S:3C:Suse 7.3 (2.4.16)
7EDC:0584:40:WS:1:1:1:0:A:LT:Linux 2.2.18
7EF4:0514:40:00:1:1:1:1:A:3C:Linux 2.2.14
7F53:0109:40:00:0:1:1:1:A:LT:Linux 2.2.14
7F53:_MSS:80:00:0:0:0:0:A:LT:AIX 4.0 - 4.2
7F53:_MSS:80:00:0:1:1:0:A:LT:Linux 2.1.19 - 2.2.17
7F53:_MSS:80:00:0:1:1:1:A:LT:Linux 2.1.19 - 2.2.17
7F53:_MSS:80:WS:0:0:0:0:A:LT:AIX 3.2
7F53:_MSS:80:WS:0:0:1:0:A:LT:Linux Kernel 2.1.88
7F7D:_MSS:80:00:0:1:1:0:A:LT:Linux 2.1.91 - 2.1.103
7F7D:_MSS:80:00:0:1:1:1:A:LT:Linux 2.1.91 - 2.1.103
7F80:0550:40:00:1:1:1:1:A:LT:Linux 2.2.19
7FB6:0218:FF:00:0:0:0:0:S:LT:3Com HiPer ARC, System V4.2.32
7FB8:0218:40:00:1:1:0:0:S:3C:SCO UnixWare 7.1.0 x86
7FB8:0218:40:00:1:1:0:1:S:3C:Linux
7FB8:0218:40:00:1:1:1:1:A:LT:Linux 2.2.19
7FB8:0218:40:WS:0:0:1:0:A:2C:Linux 2.2.16
7FE0:05B4:40:WS:0:0:0:0:A:2C:Linux 2.0.38
7FE0:_MSS:40:WS:0:0:0:0:A:LT:Linux 2.0.34 - 2.0.38
7FE0:_MSS:80:00:0:0:0:0:A:LT:Linux 2.0.32-34
7FE0:_MSS:80:WS:0:0:0:0:A:LT:Cobalt Linux 4.0 (Fargo) Kernel 2.0.34C52_SK on MIPS
7FF0:_MSS:80:00:0:0:0:0:A:LT:Linux 2.0.34-38
7FFF:04EC:80:WS:1:1:1:0:S:30:Windows 2000 
7FFF:0550:80:00:1:1:1:0:S:34:Windows 2000
7FFF:0550:80:WS:1:1:1:0:A:30:Windows 2000
7FFF:0550:80:WS:1:1:1:0:S:30:Suse
7FFF:0564:80:WS:1:1:1:0:A:30:Windows 98 SE 
7FFF:0578:80:WS:1:1:1:0:S:30:Linux Suse
7FFF:057A:80:00:1:1:1:0:S:34:Linux 
7FFF:0586:40:WS:1:1:1:0:S:30:Windows 2000
7FFF:058E:40:00:1:1:1:0:S:34:OpenBSD 3.1
7FFF:05A0:80:00:1:1:1:0:S:34:Windows XP
7FFF:05A0:80:WS:1:1:1:0:A:30:Windows 98 SE
7FFF:05A0:80:WS:1:1:1:0:S:30:Windows 2000 Server 
7FFF:05AC:80:00:0:1:1:0:A:30:Windows 2000 / XP / ME
7FFF:05AC:80:00:1:1:1:0:S:34:Windows 2000 
7FFF:05B4:40:00:0:1:0:0:A:30:Sinix 5.4x
7FFF:05B4:40:00:0:1:1:0:A:30:Windows XP
7FFF:05B4:40:00:1:1:1:1:S:3C:SuSe Linux
7FFF:05B4:40:03:1:1:1:1:S:3C:Linux
7FFF:05B4:40:WS:0:0:0:0:S:2C:Windows 98 SE
7FFF:05B4:80:00:0:1:1:1:A:3C:Windows XP
7FFF:05B4:80:00:1:1:1:0:S:34:Windows XP / 2000
7FFF:05B4:80:00:1:1:1:1:A:40:Windows 2000
7FFF:05B4:80:00:1:1:1:1:S:40:Windows XP Service Pack 1 
7FFF:05B4:80:WS:0:0:1:0:A:2C:Windows NT
7FFF:05B4:80:WS:0:0:1:0:S:2C:Windows NT 4.0 SP6a
7FFF:05B4:80:WS:1:1:1:0:A:30:Windows 98 SE
7FFF:05B4:80:WS:1:1:1:0:S:30:Windows 2000 Server
7FFF:7E05:80:00:0:1:1:0:A:30:BeOS
7FFF:_MSS:80:00:0:0:1:0:A:LT:Linux 2.4.7 (X86)
7FFF:_MSS:80:00:0:1:0:0:A:LT:ReliantUNIX-Y 5.44 B0033 RM600 1/256 R10000
7FFF:_MSS:80:00:0:1:1:0:A:LT:Linux Kernel 2.4.xx (X86)
7FFF:_MSS:80:00:0:1:1:1:A:LT:Linux Kernel 2.4.xx (X86)
7FFF:_MSS:80:WS:0:0:0:0:A:LT:SINIX-Y 5.43B0045
7FFF:_MSS:80:WS:0:0:1:0:A:LT:Linux 2.1.76
8000:0200:40:00:0:1:0:0:A:LT:Mac OS X Server
8000:0218:40:WS:0:0:1:0:A:2C:HP-UX
8000:0564:40:00:0:1:0:1:A:3C:solaris 8
8000:0564:40:00:1:1:1:1:A:40:Solaris 8 
8000:0578:40:00:0:1:1:1:S:3C:Linux 
8000:0584:40:00:0:1:1:1:A:3C:HP-UX
8000:0584:40:WS:0:0:1:0:A:2C:HP-UX B.10.20
8000:05A0:40:WS:1:1:1:0:A:30:Windows 2000
8000:05AC:40:00:0:1:1:1:A:3C:FreeBSD 4.9 
8000:05AC:40:00:0:1:1:1:S:LT:Mac OS X 10.1.5
8000:05AC:40:WS:0:0:1:0:A:2C:OS400 V5R2
8000:05AC:FF:WS:0:0:1:0:A:2C:Mac OS X
8000:05B4:01:WS:0:0:1:0:A:2C:HP-UX 
8000:05B4:20:WS:0:0:1:0:S:2C:Windows CE 3.0 (Ipaq 3670)
8000:05B4:20:WS:0:1:1:0:S:2C:Windows CE 3.0 (Ipaq 3670)
8000:05B4:40:00:0:0:1:0:S:2C:HP-UX B.10.01 A 9000/712
8000:05B4:40:00:0:1:0:0:A:LT:NetBSD
8000:05B4:40:00:0:1:0:0:S:LT:OpenVMS
8000:05B4:40:00:0:1:0:1:A:3C:HP-UX B.11.00
8000:05B4:40:00:0:1:0:1:S:3C:NetBSD
8000:05B4:40:00:0:1:1:0:A:LT:HP-UX
8000:05B4:40:00:0:1:1:0:S:30:Digital UNIX V4.0E, Mac OS X
8000:05B4:40:00:0:1:1:1:A:3C:Windows 
8000:05B4:40:00:0:1:1:1:S:3C:Mac OS X 10.1.[23]
8000:05B4:40:00:1:1:1:1:S:40:OpenBSD 3.1 
8000:05B4:40:WS:0:0:0:0:A:2C:FreeBSD
8000:05B4:40:WS:0:0:0:0:S:2C:AIX 
8000:05B4:40:WS:0:0:1:0:A:2C:Mac OS X Darwin 1.4 / HP-UX 10.20
8000:05B4:40:WS:0:0:1:0:S:2C:Mac OS X
8000:05B4:40:WS:1:1:1:0:A:30:Windows 2000
8000:05B4:80:00:0:1:1:0:S:30:Dec V4.0 OSF1
8000:05B4:80:00:1:1:1:0:A:34:Windows XP Pro
8000:05B4:80:WS:0:0:1:0:S:LT:Novell NetWare 4.11
8000:05B4:FF:00:0:1:0:0:S:30:MAC OS 8.6
8000:05B4:FF:00:0:1:1:0:S:30:Mac OS 9
8000:05B4:FF:00:0:1:1:1:A:3C:Mac OS
8000:05B4:FF:WS:0:0:1:0:A:LT:OpenBSD
8000:05B4:FF:WS:1:1:1:0:A:30:Wire Home Portal DSL Routerw/nat
8000:1000:40:WS:0:0:1:0:A:2C:HP-UX
8000:114C:40:WS:0:0:0:0:A:2C:IBM MVS
8000:6405:FF:00:0:1:1:1:A:3C:Mac OS 9.2.2
8000:6405:FF:WS:0:0:1:0:A:2C:Mac OS 9.2.2
8000:9405:FF:00:0:1:1:1:A:3C:Mac Os 9.1
8000:B405:40:00:0:1:1:1:S:3C:Mac OS X 10.x (Darwin 1.3.x 1.4.x 5.x)
8000:B405:40:00:0:1:1:1:S:LT:Mac OS X 10.x.x -  Darwin 6.1
8000:B405:40:00:0:1:1:1:S:LT:Mac Ox X 10.2.8 Jaguar
8000:B405:40:WS:0:0:1:0:S:2C:MacOS X
8000:B405:40:WS:0:0:1:0:S:LT:Mac Os X 
8000:B405:80:00:1:1:1:1:S:40:Windows CE 3.0
8000:B405:80:WS:1:1:1:0:S:LT:Pocket PC
8000:B405:FF:00:0:1:0:0:S:LT:Mac OS 9.1
8000:B405:FF:00:0:1:1:0:A:30:Mac OS 9.1
8000:B405:FF:00:0:1:1:0:S:30:Mac OS 9.2
8000:B405:FF:00:0:1:1:1:A:3C:Mac OS 9.1
8000:_MSS:80:00:0:0:1:0:A:LT:Novell NetWare 3.12 - 5.00
8000:_MSS:80:00:0:1:0:0:A:LT:Cray UNICOS 9.0.1ai - 10.0.0.2
8000:_MSS:80:00:0:1:0:1:A:LT:Cray UNICOS 9.0.1ai - 10.0.0.2
8000:_MSS:80:00:0:1:1:0:A:LT:Apple MacOS 9.04 (Powermac or G4)
8000:_MSS:80:00:0:1:1:1:A:LT:Apple MacOS 9.04 (Powermac or G4)
8000:_MSS:80:WS:0:0:0:0:A:LT:DECNIS 600 V4.1.3B System
8000:_MSS:80:WS:0:0:1:0:A:LT:Cisco IOS 12.0(3.3)S  (perhaps a 7200)
804C:A005:40:00:0:1:1:1:A:3C:Mac OS X
8052:05B4:40:01:1:1:1:1:S:40:Solaris 5.8
805C:_MSS:80:00:0:1:0:0:A:LT:BSDI BSD/OS 2.0 - 2.1
805C:_MSS:80:00:0:1:0:1:A:LT:BSDI BSD/OS 2.0 - 2.1
805C:_MSS:80:00:0:1:1:0:A:LT:Compaq Tru64 UNIX (formerly Digital UNIX) 4.0e
807A:_MSS:80:00:0:1:0:0:A:LT:OpenBSD 2.6-2.8
807A:_MSS:80:00:0:1:0:1:A:LT:OpenBSD 2.6-2.8
807A:_MSS:80:00:0:1:1:0:A:LT:AmigaOS 3.1 running Miami Deluxe 0.9m
807A:_MSS:80:00:0:1:1:1:A:LT:AmigaOS 3.1 running Miami Deluxe 0.9m
8084:0584:40:01:1:1:1:1:A:40:SunOS 5.8 / Solaris 8 
8160:0564:40:01:1:1:1:0:A:34:Solaris 8
8160:05B4:40:01:0:1:1:1:A:3C:Solaris 8 - X86
81D0:0218:40:00:0:1:0:0:A:LT:OSF1 4.0
81D0:_MSS:40:WS:0:0:1:0:A:LT:Compaq Tru64 UNIX 5.0
8218:05B4:40:00:0:1:1:1:A:3C:Mac OS X 10.1.5
8218:05B4:40:00:1:1:1:1:A:40:Solaris 
8218:05B4:40:01:1:1:1:0:A:LT:Solaris 8
8218:05B4:40:01:1:1:1:1:A:40:Gauntlet Firewall
8218:B405:40:00:0:1:0:1:A:3C:MAC OS X 10.3.4
8218:B405:40:00:0:1:1:1:A:3C:Mac OS X 10.1.4
8218:B405:40:00:0:1:1:1:A:LT:MacOS X Server Release
8235:0488:FF:WS:0:0:0:0:S:2C:Mac OS X 
8246:AA05:40:00:0:1:0:1:A:3C:X server
8274:05B4:40:WS:0:0:0:0:A:2C:Digital Unix 4.0b 
832C:05B4:40:00:0:1:1:0:A:30:Compaq Tru64 UNIX 5.0 / Digital UNIX V5.60
832C:05B4:40:WS:0:0:0:0:A:2C:Digital Unix
832C:05B4:40:WS:0:0:1:0:A:2C:Freebsd 4.3
832C:05B4:40:WS:1:1:1:0:A:30:Windows 2000 pro
832C:05B4:40:WS:1:1:1:0:S:30:Solaris 8
832C:05B4:FF:00:1:1:0:0:A:34:Windows 2000 Server 
832C:05B4:FF:WS:0:0:1:0:S:2C:Solaris 7
832C:05B4:FF:WS:1:1:0:0:A:30:MAC OS X
832C:10D8:40:00:0:1:0:0:A:30:Unix
832C:B405:40:00:0:1:1:0:A:30:Digital UNIX V4.0F
832C:B405:40:WS:0:0:1:0:A:2C:Mac OS X 10.1
832C:B405:40:WS:0:0:1:0:A:LT:Darwin
832C:B405:FF:00:0:1:1:0:A:LT:Mac OS X
832C:_MSS:40:WS:0:0:1:0:A:LT:Linux 2.0.34 - 2.0.38
8340:0578:40:WS:1:1:1:0:A:30:Solaris 8 
8340:0584:40:00:0:1:1:1:A:3C:Mac OS X/10
8340:05B4:40:00:0:1:1:1:A:3C:MacOS X
8371:_MSS:80:00:0:1:1:0:A:LT:Solaris 2.6 - 2.7
8371:_MSS:80:00:0:1:1:1:A:LT:Solaris 2.6 - 2.7
8377:_MSS:80:00:0:0:1:0:A:LT:Solaris 2.5, 2.5.1
8400:8C05:40:00:0:1:1:1:A:3C:MacOS X 10.3
8520:B405:40:WS:0:0:1:0:A:LT:MacOS X 10.2.1
869F:_MSS:80:00:0:1:1:0:A:LT:Windows NT4 / Win95 / Win98
869F:_MSS:80:00:0:1:1:1:A:LT:Windows NT4 / Win95 / Win98
8700:05AC:FF:00:1:1:1:1:A:40:Solaris 2.6
8765:_MSS:80:00:0:1:1:0:A:LT:Solaris 2.6 - 2.7 with tcp_strong_iss=0
8765:_MSS:80:00:0:1:1:1:A:LT:Solaris 2.6 - 2.7 with tcp_strong_iss=0
879B:_MSS:80:WS:0:0:1:0:A:LT:Solaris 2.5, 2.5.1
87C0:05B4:FF:00:1:1:0:1:A:40:Windows 
87C0:05B4:FF:00:1:1:1:1:A:40:Solaris 2.6 2.7
87C0:_MSS:FF:WS:0:0:1:0:A:LT:Solaris 2.6 - 2.7
88E0:05B4:80:WS:1:1:1:0:S:30:Windows 2000
8EDA:_MSS:80:00:0:0:1:0:A:LT:Solaris 2.5, 2.5.1
8F4D:_MSS:80:00:0:1:1:0:A:LT:Solaris 2.6 - 2.7
8F4D:_MSS:80:00:0:1:1:1:A:LT:Solaris 2.6 - 2.7
91B4:05AC:40:00:1:1:1:0:A:34:Windows NT 5.1 (Windows XP)
91B4:05B4:40:00:1:1:1:0:S:34:Windows XP
94C0:05B4:80:WS:1:1:1:0:S:30:Windows 2000 
95C2:B405:80:WS:1:1:1:0:S:30:Windows ME 
9820:05B4:40:WS:1:1:1:0:S:30:Windows 2000
9C40:05B4:80:WS:1:1:1:0:S:30:Windows Xp
AA3C:05B4:80:WS:1:1:1:0:S:30:Windows 2000 Pro
ABCD:_MSS:80:00:0:1:1:0:A:LT:Solaris 2.6 - 2.7
ABCD:_MSS:80:00:0:1:1:1:A:LT:Solaris 2.6 - 2.7
AC00:0550:80:WS:0:0:1:0:S:2C:Windows NT 4.0 SP6a
AC00:05AC:80:WS:1:1:1:0:S:30:Windows 2000 SP2
AC00:FA3B:80:WS:1:1:1:0:S:LT:Windows 2000 SP2
AE4C:0594:80:WS:1:1:1:0:S:30:OpenBSD 3.5 
AE4C:05B4:40:03:1:1:1:1:S:40:Windows XP
AE4C:05B4:40:WS:0:0:0:0:S:2C:Windows 95
AE4C:05B4:80:03:1:1:1:0:S:34:Windows 2000
AE60:0586:80:WS:1:1:1:0:S:30:Windows ME
AE60:05B4:80:03:1:1:1:1:S:40:Linux
AE60:05B4:80:WS:1:1:1:0:S:30:Windows XP
B270:05B4:80:WS:1:1:1:0:S:LT:Windows ME
B400:05AC:20:03:1:1:1:1:S:40:windows 200
B400:05B4:20:03:1:1:0:1:S:LT:Windows 2000 SP2
B400:05B4:20:03:1:1:1:1:S:40:Windows 2000
B400:05B4:40:03:1:1:1:1:S:40:Windows XP Pro SP1
B400:05B4:80:03:1:1:1:1:S:40:Windows XP 
B400:B405:20:03:1:1:1:0:S:34:Windows 2000 SP3
B580:05AC:40:02:1:1:1:1:A:40:Windows XP
B580:05AC:80:03:1:1:1:1:A:40:Windows XP
B580:05B4:80:03:1:1:1:1:A:40:Windows XP
B5A4:05B4:80:00:1:1:0:1:A:40:Linux 2.4.18
B5A4:05B4:80:00:1:1:1:0:S:34:Windows 2000 Server
B5A4:05B4:80:00:1:1:1:1:A:40:Windows XP Corp. SP1 (Ger)
B5A4:05B4:80:WS:0:0:0:0:A:2C:Windows XP Corp. SP1 (Ger)
B5A4:05B4:80:WS:1:1:1:0:S:30:Windows XP SP2 german 
B5C9:_MSS:80:00:0:1:1:0:A:LT:Windows Millenium Edition v4.90.3000
B5C9:_MSS:80:00:0:1:1:1:A:LT:Windows Millenium Edition v4.90.3000
B680:05B4:FF:WS:0:0:0:0:A:2C:ZyXEL ZyNOS 3.50
B9F0:0578:FF:WS:0:0:0:0:A:2C:Windows 2000
BB80:05B4:80:WS:1:1:1:0:S:30:Windows 2000
BB80:_MSS:80:WS:0:0:1:0:A:LT:Windows 98
C000:057A:40:WS:1:1:1:0:A:30:Linux
C000:05A0:80:00:1:1:1:1:S:40:Windows XP
C000:05B4:40:00:0:0:0:0:S:2C:IRIX 6.5 / 6.4
C000:05B4:40:00:1:1:1:1:A:40:IRIX64 6.5 IP27
C000:05B4:40:WS:0:0:0:0:S:LT:Irix 6.5
C000:05B4:40:WS:1:1:1:0:A:30:Linux
C000:05B4:80:00:0:1:0:0:A:LT:IRIX 6.2 - 6.5
C000:05B4:80:00:0:1:0:1:A:LT:IRIX 6.2 - 6.5
C000:_MSS:80:WS:0:0:0:0:A:LT:OS-9/68K V2.4 (Quanterra Q4124 - 68030)
C050:05B4:40:00:0:1:1:1:A:3C:Solaris 9 SPARC ULTRA 10
C050:05B4:40:00:1:1:1:1:A:40:SunOS 5.9 (sun4u)
C08A:_MSS:80:00:0:1:1:0:A:LT:FreeBSD 2.2.1 - 4.1
C08A:_MSS:80:00:0:1:1:1:A:LT:FreeBSD 2.2.1 - 4.1
C0B7:_MSS:80:00:0:1:1:0:A:LT:FreeBSD 2.2.1 - 4.1
C0B7:_MSS:80:00:0:1:1:1:A:LT:FreeBSD 2.2.1 - 4.1
C1E8:05B4:40:WS:0:0:1:0:A:2C:Solaris 9
C1E8:05B4:40:WS:1:1:1:0:A:30:Windows NT
C1E8:05B4:40:WS:1:1:1:0:S:30:Solaris 9
C210:0564:40:WS:1:1:1:0:A:30:Linux 
C377:05AC:80:02:1:1:1:1:S:40:Windows 2000 advanced server SP2
C3C8:0598:40:WS:1:1:1:0:A:30:Unix
C79C:05B4:80:WS:1:1:1:0:S:30:Windows XP Home
CDFF:_MSS:80:00:0:0:1:0:A:LT:SONY NEWS-OS 6.1.2
D600:05AC:80:02:1:1:1:1:S:40:Windows XP Professional
D780:05B4:40:03:1:1:1:0:S:34:Winblows XP
D780:B405:40:03:1:1:1:0:S:LT:Windows XP
DA00:05B4:80:02:1:1:0:0:S:34:Windows 2000
DA00:05B4:80:02:1:1:1:0:S:34:Windows XP
E000:0564:40:00:0:1:0:1:A:3C:FreeBSD 4.8
E000:0564:40:00:0:1:1:1:A:3C:BSD
E000:0564:40:WS:0:0:1:0:A:2C:Free BSD 4.7
E000:0578:40:00:0:1:1:1:A:3C:FreeBSD 4.9-stable 
E000:0578:40:WS:0:0:1:0:A:2C:FreeBSD 
E000:0584:40:00:0:1:1:1:A:3C:Linux/FreeBSD 
E000:0586:40:WS:0:0:0:0:A:2C:FreeBSD4.6.2
E000:05A0:40:00:0:1:1:1:A:3C:FreeBSD 4.7-RELEASE / 4.9
E000:05AA:40:WS:0:0:1:0:A:2C:Windows2000
E000:05AC:40:00:0:1:1:1:A:3C:free BSD
E000:05AC:40:WS:0:0:1:0:A:2C:FreeBSD 4.7
E000:05B4:40:00:0:1:0:1:A:3C:FreeBSD 4.6-RELEASE-p1
E000:05B4:40:00:0:1:1:0:A:30:Free BSD 4.8-STABLE
E000:05B4:40:00:0:1:1:1:A:3C:FreeBSD 4.7-PRERELEASE
E000:05B4:40:00:0:1:1:1:S:3C:FreeBSD 4.6-RELEASE
E000:05B4:40:00:0:1:1:1:S:LT:Free BSD 4.7-Stable
E000:05B4:40:WS:0:0:0:0:A:2C:FreeBSD 4.6-RELEASE-p1
E000:05B4:40:WS:0:0:1:0:A:2C:FreeBSD 4.7
E000:05B4:40:WS:0:0:1:0:S:2C:FreeBSD
E000:05B4:80:00:0:1:1:1:S:3C:FreeBSD 4.6-RC2
E000:6405:40:00:0:1:0:1:A:3C:FreeBSD 4.6
E000:B405:40:00:0:1:0:1:A:3C:FreeBSD 4.6
E000:B405:40:00:0:1:1:1:A:3C:Free BSD 4.8 
E000:B405:40:00:0:1:1:1:S:3C:Free BSD
E000:B405:40:00:0:1:1:1:S:LT:Free BSD 
E000:C003:40:00:0:1:1:1:A:3C:Linux Debian 
E640:05AC:40:02:1:1:1:0:S:34:Windows 2000 Server - Windows XP SP1
E640:05B4:40:02:1:1:0:0:S:LT:Windows 2000 SP2
E640:05B4:40:02:1:1:1:0:S:34:Windows 2000 Professional
E8C0:05A0:40:01:1:1:1:0:S:34:Windows 98
E920:_MSS:80:00:0:1:1:1:A:LT:Windows ME
EA60:05AC:40:WS:0:0:1:0:A:2C:Router Cisco 677 
EA60:05DC:40:WS:0:0:1:0:A:2C:Windows XP
EA60:05DC:40:WS:0:0:1:0:A:LT:Cisco 667i-DIR DSL router -- cbos 2.4.2
EA60:_MSS:80:WS:0:0:1:0:A:LT:Cisco 675 DSL router -- cbos 2.1
EB28:0578:40:WS:1:1:1:0:A:30:IRIX 6.5.x
EBC0:05AC:40:02:0:1:0:0:S:30:Windows 2000
EBC0:05AC:40:02:1:1:1:0:S:34:Windows XP
EBC0:05B4:40:02:1:1:0:0:S:LT:Windows ME
EBC0:05B4:40:02:1:1:1:0:S:34:Windows 2000 Pro SP2 / XP Pro
EBC0:05B4:40:02:1:1:1:1:S:40:Windows XP
EBC0:05B4:40:WS:1:1:1:0:S:30:Windown XP
EBC0:05B4:80:02:1:1:1:0:S:34:Windows XP
EBC0:05B4:80:02:1:1:1:1:S:40:Windows XP
EBC0:05B4:80:WS:1:1:1:0:S:30:Windows ME
ED90:05B4:40:00:0:1:1:1:A:3C:Irix 6.5.8
ED90:05B4:40:00:1:1:1:1:A:40:IRIX 6.5
ED90:_MSS:40:WS:0:0:1:0:A:LT:IRIX 6.2 - 6.5
EE48:_MSS:40:WS:0:0:1:0:A:LT:IRIX 5.1 - 5.3
EF2A:_MSS:80:00:0:1:0:0:A:LT:IRIX 5.2
EF2A:_MSS:80:00:0:1:0:1:A:LT:IRIX 5.2
EF88:_MSS:40:WS:0:0:1:0:A:LT:IRIX 6.2 - 6.5
F000:0200:40:WS:0:0:0:0:S:LT:IRIX 5.3 / 4.0.5F
F000:05B4:40:00:0:1:0:0:A:30:OSF1 5.1 732 alpha
F000:05B4:40:00:0:1:0:0:S:30:OSF1 5.1
F000:05B4:40:WS:0:0:0:0:A:2C:OSF1
F000:05B4:40:WS:0:0:0:0:S:LT:IRIX 6.3
F000:05B4:40:WS:1:1:0:0:S:LT:IRIX 6.5.10
F000:05B4:80:00:0:1:0:0:A:LT:IRIX 5.3
F000:05B4:80:00:0:1:0:1:A:LT:IRIX 5.3
F000:B405:40:WS:1:1:0:0:S:30:SGI Irix 6.5.17m
F400:05B4:80:01:1:1:1:0:S:34:Windows XP
F424:05B4:80:04:1:1:1:0:A:34:Windows XP SP1 - english 
F5E0:05B4:80:01:1:1:1:0:S:34:Windows 2000
F990:05B4:40:WS:1:1:1:0:S:30:Windows 2000
F99F:0000:80:WS:0:0:0:0:S:28:Linux 2.2.x or 2.4.x
FAD8:0574:80:WS:1:1:1:0:S:30:Windows XP
FAF0:0200:80:WS:1:1:1:0:S:30:Windows 2000 
FAF0:04EC:80:WS:1:1:1:0:S:30:Windows XP Professional 
FAF0:0514:80:WS:1:1:1:0:S:30:Windows XP
FAF0:0518:80:WS:1:1:1:0:S:30:Windows 2000 server
FAF0:0528:80:WS:1:1:1:0:S:30:Windows
FAF0:0550:80:WS:1:1:1:0:A:30:Windows XP Pro 
FAF0:055C:80:WS:1:1:1:0:S:30:Windows XP SP2
FAF0:0564:80:00:1:1:1:1:A:40:Windows XP
FAF0:0564:80:WS:0:0:1:0:A:2C:Windows XP Professional
FAF0:0564:80:WS:1:1:1:0:A:30:Windows 2K
FAF0:0564:80:WS:1:1:1:0:S:30:Windows XP Professional
FAF0:0572:80:WS:1:1:1:0:S:30:Windows XP
FAF0:0578:80:WS:1:1:1:0:A:30:Windows XP
FAF0:0578:80:WS:1:1:1:0:S:30:Windows 2000
FAF0:057A:80:WS:1:1:1:0:S:30:Windows XP
FAF0:0584:40:WS:1:1:1:0:A:30:Unix 
FAF0:0584:80:WS:0:1:1:0:S:30:Windows XP 
FAF0:0584:80:WS:1:1:1:0:A:30:Windows 2000 / XP
FAF0:0584:80:WS:1:1:1:0:S:30:Windows XP
FAF0:0586:80:WS:1:1:1:0:S:30:Windows XP pro
FAF0:0598:80:WS:1:1:1:0:A:30:Windows XP
FAF0:05A0:80:00:1:1:1:1:A:40:Windows XP
FAF0:05AC:40:03:1:1:1:0:A:34:Windows 2000 
FAF0:05AC:80:00:1:1:1:1:A:40:Windows XP 
FAF0:05AC:80:WS:1:1:1:0:A:30:Windows 2000
FAF0:05AC:80:WS:1:1:1:0:S:30:Windows 2000 Pro
FAF0:05B0:80:WS:1:1:1:0:A:30:Windows 2000
FAF0:05B4:40:00:1:1:1:0:S:34:Windows XP
FAF0:05B4:40:00:1:1:1:0:S:LT:Windows 98
FAF0:05B4:40:00:1:1:1:1:A:40:Windows 2000
FAF0:05B4:40:00:1:1:1:1:S:40:Windows 2000
FAF0:05B4:40:02:1:1:0:0:A:LT:Windows ME
FAF0:05B4:40:02:1:1:1:0:A:34:Windows 2000
FAF0:05B4:40:WS:0:0:0:0:A:2C:IBM AIX (JG)
FAF0:05B4:40:WS:0:0:1:0:A:2C:SunOS 5.9
FAF0:05B4:40:WS:0:0:1:0:S:2C:Windows NT Server 4.0 
FAF0:05B4:40:WS:1:1:1:0:A:30:Unix
FAF0:05B4:40:WS:1:1:1:0:S:30:Windows XP Professional, Build 2600
FAF0:05B4:80:00:0:1:1:0:A:LT:Windows 2000 Professional, Build 2183 (RC3)
FAF0:05B4:80:00:0:1:1:1:A:3C:Windows 2000 Professional / Windows XP Pro
FAF0:05B4:80:00:1:1:1:0:A:34:Windows XP
FAF0:05B4:80:00:1:1:1:0:S:34:Windows 2000
FAF0:05B4:80:00:1:1:1:1:A:40:Windows 2000 Version 5.0 (Build 2195)
FAF0:05B4:80:00:1:1:1:1:S:40:Windows XP
FAF0:05B4:80:WS:0:0:0:0:A:2C:Windows NT 4.0
FAF0:05B4:80:WS:0:0:0:0:S:2C:Windows XP Proffesional Release
FAF0:05B4:80:WS:0:0:1:0:A:2C:Linux Slackware 8.0
FAF0:05B4:80:WS:0:1:1:0:A:LT:Windows XP Professional Release
FAF0:05B4:80:WS:0:1:1:0:S:30:Windows XP
FAF0:05B4:80:WS:1:1:0:0:A:30:Linux
FAF0:05B4:80:WS:1:1:0:0:S:30:Windows 2000 Pro
FAF0:05B4:80:WS:1:1:1:0:A:30:Windows XP
FAF0:05B4:80:WS:1:1:1:0:S:30:Windows XP Pro, Windows 2000 Pro
FAF0:05B4:80:WS:1:1:1:0:S:LT:Windows 98 SE / 2000 / XP Professional
FAF0:05B4:FF:WS:0:0:1:0:S:2C:Linux 2.1.xx
FAF0:05B4:FF:WS:1:1:1:0:A:LT:Windows 2000 Pro
FAF0:05B4:FF:WS:1:1:1:0:S:30:Windows 2000 Professional SP 3
FAF0:B405:40:00:0:1:1:0:A:LT:Mac OS X Server 10.1
FAF0:B405:80:00:0:1:1:1:A:3C:Windows XP professional
FAF0:B405:80:00:0:1:1:1:A:LT:Windows XP
FAF0:B405:80:00:1:1:1:1:A:LT:Pocket Pc 2002
FAF0:B405:80:WS:0:0:1:0:A:2C:Windows XP Pro
FAF0:B405:80:WS:1:1:1:0:A:30:Windows XP
FAF0:B405:80:WS:1:1:1:0:S:30:Windows XP Home
FAF0:_MSS:FF:WS:0:0:0:0:A:LT:Solaris 2.6 - 2.7
FB04:0594:80:WS:1:1:1:0:S:30:Windows XP
FB40:0218:40:00:1:1:1:0:S:34:Windows 2000 Pro
FB40:05B4:40:00:1:1:1:0:S:34:Windows XP
FC00:04EC:80:WS:1:1:1:0:S:30:Windows XP
FC00:0518:40:WS:0:0:0:0:A:2C:Xbox - Avalaunch 0.48.64 
FC00:0546:80:WS:1:1:1:0:S:30:Windows XP Pro 
FC00:0564:80:WS:1:1:1:0:A:30:Cisco PIX FireWall
FC00:0578:40:WS:0:0:0:0:S:2C:Microsoft XBox 
FC00:057E:80:WS:1:1:1:0:S:30:Windows XP
FC00:05B4:80:00:0:1:1:1:A:3C:Windows 2000
FC00:05B4:80:00:1:1:0:1:A:40:Windows 2000 Running IIS Version 5
FC00:05B4:80:00:1:1:1:0:S:34:Windows XP SP1 
FC00:05B4:80:00:1:1:1:1:A:40:Windows 2000 - XP
FC00:05B4:80:WS:0:0:1:0:A:2C:Windows XP
FC00:05B4:80:WS:1:1:1:0:A:30:Windows X
FC00:05B4:80:WS:1:1:1:0:S:30:Windows 2000 Pro
FC00:7E05:80:WS:1:1:1:0:S:30:Windows XP Pro
FC00:B405:80:00:0:1:1:1:A:3C:Windows 2000 server
FC00:B405:80:00:1:1:1:0:A:34:Windows XP Professional v2002 SP1
FC00:B405:80:WS:1:1:1:0:S:30:Windows XP 
FC6C:05B4:80:WS:1:1:1:0:S:LT:Windows XP 
FCA4:057E:80:WS:1:1:1:0:S:30:Windows XP
FD20:05A0:80:00:1:1:1:1:A:40:Windows XP Home v2002 
FD20:05A0:80:WS:1:1:1:0:A:30:Windows XP home 
FD20:05A0:80:WS:1:1:1:0:S:30:Windows XP Home v2002
FDB8:0584:FF:WS:1:1:1:0:S:30:Windows XP
FDD4:05A4:80:WS:1:1:1:0:S:30:Windows XP SP1
FE70:0588:80:WS:1:1:0:0:A:30:Windows 98 SE
FE70:0588:80:WS:1:1:0:0:S:30:Windows XP Home
FE88:05B4:FF:00:0:1:1:1:A:3C:Solaris
FE88:05B4:FF:00:1:1:1:1:A:40:Mac OS X Server
FE88:B405:40:00:0:1:1:1:A:3C:Mac OS X Server 10.x
FE88:_MSS:FF:WS:0:0:1:0:A:LT:Solaris 2.6 - 2.7
FEF4:0534:80:00:1:1:1:1:A:40:Windows 2000
FEF4:0534:80:WS:1:1:1:0:S:30:Windows XP
FEFA:_MSS:80:00:0:1:0:0:A:LT:AIX v4.2
FEFA:_MSS:80:00:0:1:0:1:A:LT:AIX v4.2
FF00:0550:80:00:1:1:1:1:A:40:Windows XP
FF00:0550:80:WS:1:1:1:0:S:30:Windows XP
FF3C:0584:80:WS:1:1:1:0:A:30:Windows 2000 
FF3C:05AC:40:WS:1:1:1:0:A:30:Windows XP
FF3C:05AC:80:00:1:1:1:0:A:34:Windows XP Pro
FF3C:05AC:80:WS:1:1:0:0:S:30:Windows 2000 Pro SP3
FF3C:05AC:80:WS:1:1:1:0:S:30:Windows 2000
FF70:0218:80:WS:1:1:1:0:A:30:Windows 2000 Professional
FF70:0218:80:WS:1:1:1:0:S:1C:Windows XP Professional Build 2600.xpsp2.030422
FFA3:05B4:40:WS:1:1:1:0:S:30:Windown 2000 
FFAF:_MSS:80:00:0:0:1:0:A:LT:Solaris 2.3 - 2.4
FFAF:_MSS:80:00:0:1:0:0:A:LT:Hitachi HI-UX/MPP (don't know version)
FFAF:_MSS:80:00:0:1:0:1:A:LT:Hitachi HI-UX/MPP (don't know version)
FFAF:_MSS:80:WS:0:0:0:0:A:LT:AIX 3.2.5 (Bull HardWare)
FFDC:0200:40:00:0:1:0:1:A:3C:Windows XP
FFF0:04EC:80:00:1:1:1:0:A:34:Windos XP 
FFF0:04EC:80:00:1:1:1:1:A:40:Windows 2000
FFF0:04EC:80:WS:1:1:1:0:S:30:Windows 2000
FFF0:057A:80:00:1:1:1:1:A:40:Windows XP
FFF0:05B0:80:WS:1:1:1:0:A:30:Windows XP
FFF0:05B0:80:WS:1:1:1:0:S:30:Windows XP
FFF0:B005:80:00:0:1:1:1:A:3C:Windows XP
FFF0:EC04:80:WS:1:1:1:0:S:30:Windows XP
FFF7:_MSS:80:00:0:1:1:0:A:LT:Solaris 2.6 - 2.7
FFF7:_MSS:80:00:0:1:1:1:A:LT:Solaris 2.6 - 2.7
FFFF:0200:40:01:0:1:0:0:A:30:FreeBSD 4.8
FFFF:0200:40:WS:0:0:0:0:A:2C:AIX
FFFF:0200:80:WS:1:1:1:0:S:30:Windows
FFFF:0218:80:00:1:1:1:0:S:34:Windows XP
FFFF:04EC:40:00:1:1:1:1:S:40:Windows XP 
FFFF:04EC:80:00:1:1:1:1:A:40:Windows 2000
FFFF:04EC:80:WS:0:0:1:0:A:2C:Windows 2000
FFFF:04EC:80:WS:1:1:1:0:S:30:Windows 2000 Professional
FFFF:052A:40:WS:0:0:0:0:A:2C:Windows* [aol client]
FFFF:052A:80:00:1:1:0:1:A:40:Windows 2000 Server 
FFFF:0534:40:WS:1:1:1:0:A:30:Windows XP Pro SP1
FFFF:0548:80:WS:1:1:0:0:S:30:Windows 2000 Server
FFFF:0550:40:WS:1:1:1:0:A:30:Windows 2000 Server
FFFF:0558:80:WS:0:0:1:0:S:LT:BorderManager 3.5
FFFF:0564:40:00:0:1:1:1:A:3C:FreeBSD 
FFFF:0564:40:01:0:1:1:1:S:3C:FreeBSD 5.1-RELEASE 
FFFF:0564:40:WS:0:0:0:0:A:LT:AIX
FFFF:0564:40:WS:0:0:1:0:A:2C:Linux
FFFF:0564:40:WS:1:1:1:0:S:30:Windows 98 Second Edition
FFFF:0564:80:00:1:1:1:1:A:40:Windows 2000 server
FFFF:0564:80:WS:1:1:1:0:A:30:Windows 2000 / NT 
FFFF:0564:80:WS:1:1:1:0:S:30:Windows
FFFF:0564:80:WS:1:1:1:1:A:3C:Windows 2000
FFFF:0578:40:WS:0:0:1:0:S:2C:FreeBSD 4.5 
FFFF:057E:80:00:1:1:1:1:S:40:Windows 2000
FFFF:0584:40:00:1:1:1:1:A:40:Windows XP
FFFF:0584:40:WS:0:0:0:0:A:2C:Solaris
FFFF:0584:80:WS:1:1:1:0:A:30:Cisco-louche1 
FFFF:0584:80:WS:1:1:1:0:S:30:Linux Debian
FFFF:0586:40:00:0:1:1:1:A:3C:Unix
FFFF:0586:40:01:0:1:1:1:S:3C:Mac OS X 10.2
FFFF:0586:40:WS:1:1:1:0:S:30:Windows 98 SE 
FFFF:058C:40:00:1:1:1:1:A:40:Windows XP Pro
FFFF:058C:40:WS:0:0:0:0:A:2C:FreeBSD 4.5
FFFF:058C:40:WS:1:1:1:0:A:30:Windows 2000 Pro SP2
FFFF:058C:80:00:1:1:1:1:A:40:FreeBSD
FFFF:058C:80:WS:1:1:1:0:A:30:NetBSD
FFFF:0598:40:WS:0:0:0:0:A:2C:FreeBSD 4.5
FFFF:0598:40:WS:0:0:0:0:S:2C:Cisco webcache
FFFF:05A0:80:WS:1:1:1:0:A:30:Windows XP
FFFF:05A0:80:WS:1:1:1:0:S:30:Windows XP Pro
FFFF:05A0:FF:WS:1:1:1:0:S:30:Windows 98 
FFFF:05AC:40:00:1:1:1:0:S:34:MS Windows XP SP1
FFFF:05AC:40:01:0:1:0:1:A:3C:FreeBSD 4.5-STABLE
FFFF:05AC:40:01:0:1:1:1:S:3C:Mac OS X 10.2.x 
FFFF:05AC:40:WS:1:1:1:0:S:30:Windows 98 Second Edition
FFFF:05AC:40:WS:1:1:1:0:S:LT:Windows 98
FFFF:05AC:80:00:0:1:0:1:S:3C:Windows XP
FFFF:05AC:80:00:1:1:1:1:A:40:Windows 2000 Server
FFFF:05AC:80:WS:0:0:1:0:S:LT:Windows 98
FFFF:05AC:80:WS:1:1:1:0:A:30:BSD
FFFF:05AC:80:WS:1:1:1:0:S:30:Windows 95
FFFF:05B0:80:WS:1:1:1:0:S:30:Windows 2000 Professional 
FFFF:05B4:40:00:0:1:0:0:S:3C:CacheOS 3.1 on a CacheFlow 6000
FFFF:05B4:40:00:0:1:1:1:A:3C:Mac OS X (Panther) ver. 10.3.3 (7F44)
FFFF:05B4:40:00:0:1:1:1:S:3C:FreeBSD 4.4 / 4.5 / 4.7
FFFF:05B4:40:00:1:1:1:1:S:40:Openbsd 2.9
FFFF:05B4:40:01:0:1:0:0:A:30:FreeBSD 4.5
FFFF:05B4:40:01:0:1:0:0:S:30:AOL proxy
FFFF:05B4:40:01:0:1:0:1:A:3C:FreeBSD 4.4 - 4.5
FFFF:05B4:40:01:0:1:0:1:A:LT:AIX
FFFF:05B4:40:01:0:1:1:1:A:3C:FreeBSD 4.3 - 4.4 - 5.1
FFFF:05B4:40:01:0:1:1:1:A:LT:Linux 
FFFF:05B4:40:01:0:1:1:1:S:3C:FreeBSD 4.5-STABLE
FFFF:05B4:40:01:0:1:1:1:S:44:FreeBSD
FFFF:05B4:40:02:0:1:0:1:A:3C:AIX 4.3 
FFFF:05B4:40:02:1:1:1:0:S:34:Windows 2003 
FFFF:05B4:40:03:0:1:1:1:A:3C:FreeBSD 4.7-RELEASE 
FFFF:05B4:40:WS:0:0:0:0:A:2C:FreeBSD 4.5
FFFF:05B4:40:WS:0:0:0:0:A:LT:Win NT 4.0 SP4
FFFF:05B4:40:WS:0:0:1:0:A:2C:FreeBSD 4.3 - 4.4
FFFF:05B4:40:WS:0:0:1:0:S:2C:FreeBSD 4.5-RELEASE
FFFF:05B4:40:WS:1:1:0:0:S:30:Windows 98
FFFF:05B4:40:WS:1:1:1:0:A:30:Windows 2000 Pro SP2
FFFF:05B4:40:WS:1:1:1:0:S:30:Windows 98 Second Edition
FFFF:05B4:80:00:0:0:1:0:A:LT:MacOS 8.1
FFFF:05B4:80:00:0:1:0:0:A:LT:AIX 3.2 running on RS/6000
FFFF:05B4:80:00:0:1:0:1:A:LT:AIX 3.2 running on RS/6000
FFFF:05B4:80:00:0:1:1:1:A:3C:Windows 2000 SP4 
FFFF:05B4:80:00:1:1:0:1:A:40:Windows 2000 Server 
FFFF:05B4:80:00:1:1:1:0:A:34:Windows 2000 Server SP4 
FFFF:05B4:80:00:1:1:1:1:A:40:Windows 2000 Advanced Server
FFFF:05B4:80:01:0:1:1:1:S:3C:FreeBSD 5.0 dp-1
FFFF:05B4:80:03:1:1:1:0:S:34:Windows 2000
FFFF:05B4:80:03:1:1:1:1:S:40:Windows 2000 SP 3
FFFF:05B4:80:WS:0:0:1:0:A:2C:BSD
FFFF:05B4:80:WS:1:1:0:0:A:30:Windows 2000 Server SP4 
FFFF:05B4:80:WS:1:1:0:0:S:30:Windows 98 SE
FFFF:05B4:80:WS:1:1:1:0:A:30:NetBSD
FFFF:05B4:80:WS:1:1:1:0:S:30:Windows 2000 Professional SP4
FFFF:05B4:80:WS:1:1:1:0:S:LT:MacOS X
FFFF:05B4:FF:01:0:1:1:0:A:30:Mac OS 9
FFFF:05B4:FF:01:0:1:1:0:S:30:Mac OS 9
FFFF:05B4:FF:WS:0:0:1:0:A:2C:FreeBSD 
FFFF:05B4:FF:WS:1:1:1:0:S:30:Windows 2000 
FFFF:7805:40:01:0:1:0:1:S:3C:Mac OS X 10.3 
FFFF:7E05:80:00:1:1:1:1:S:40:Windows 2000
FFFF:8C05:FF:01:0:1:0:1:A:LT:MacOS X
FFFF:A005:40:01:0:1:1:1:S:LT:Linux (Embedded) Router
FFFF:AC05:80:00:1:1:1:1:S:40:Windows XP
FFFF:AC05:80:WS:1:1:1:0:S:30:Windows XP
FFFF:B405:40:00:0:1:1:1:A:3C:Mac OS X (Panther) ver. 10.3.x 
FFFF:B405:40:00:0:1:1:1:S:3C:Mac OS X 10.3 Panther
FFFF:B405:40:00:0:1:1:1:S:LT:Darwin
FFFF:B405:40:00:0:1:1:1:S:LT:Darwin 1.4
FFFF:B405:40:00:0:1:1:1:S:LT:Mac OS X 10.3.2 (ppc) 
FFFF:B405:40:00:0:1:1:1:S:LT:Mac Os X 10.3.4
FFFF:B405:40:01:0:1:0:1:A:3C:FreeBSD 4.4 / 4.5
FFFF:B405:40:01:0:1:1:0:A:LT:Mac OS X 
FFFF:B405:40:01:0:1:1:1:A:3C:Mac OS X 10.1.3 / 10.2.x
FFFF:B405:40:01:0:1:1:1:S:3C:Mac OS X 10.1.x
FFFF:B405:40:02:0:1:1:1:S:3C:Mac OS X 10.x.x
FFFF:B405:40:WS:0:0:1:0:A:2C:Mac OS X 
FFFF:B405:40:WS:0:0:1:0:A:LT:Mac OS X 
FFFF:B405:40:WS:0:0:1:0:S:2C:Mac OS X 10.3
FFFF:B405:40:WS:0:0:1:0:S:LT:Mac OS X (10.3)
FFFF:B405:40:WS:0:0:1:0:S:LT:Mac OS X (10.3) 
FFFF:B405:40:WS:0:0:1:0:S:LT:Mac OS X 10.3
FFFF:B405:80:00:0:1:1:1:A:3C:Windows 2000 
FFFF:B405:80:WS:0:0:0:0:A:2C:Windows 2000 Advanced Server
FFFF:B405:80:WS:1:1:1:0:A:30:Windows XP
FFFF:B405:80:WS:1:1:1:0:S:30:Windows
FFFF:B405:FF:02:0:1:0:1:A:LT:AIX
FFFF:B405:FF:02:0:1:1:0:S:LT:Mac OS 9.2
FFFF:_MSS:80:00:0:0:0:0:A:LT:IBM MVS TCP/IP stack V. 3.2 or AIX 4.3.2
FFFF:_MSS:80:00:0:1:1:0:A:LT:Cray Unicos 9.0 - 10.0 or Unicos/mk 1.5.1
FFFF:_MSS:80:00:0:1:1:1:A:LT:Cray Unicos 9.0 - 10.0 or Unicos/mk 1.5.1
FFFF:_MSS:80:WS:0:0:0:0:A:LT:IBM MVS TCP/IP stack V. 3.2 or AIX 4.3.2
FFFF:_MSS:80:WS:0:0:1:0:A:LT:Novell NetWare 3.12 - 5.00
FFFF:_MSS:FF:WS:0:0:1:0:A:LT:Solaris 2.6 - 2.7
