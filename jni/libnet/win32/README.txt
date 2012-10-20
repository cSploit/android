Libnet 1.1.1 VC++ Build instructions.
Kirby Kuehl vacuum@users.sourceforge.net


Tested Platforms:
Windows 2000 SP4, Windows XP SP1

Tested IDEs:
VC++ 6.0 Service Pack 5
Visual Studio .NET 2003
Visual Studio .NET 2002

Winpcap 3.01 Developers pack: http://winpcap.polito.it/install/default.htm
Platform SDK : http://www.microsoft.com/msdownload/platformsdk/sdkupdate/


1. Extract Libnet tarball. (Probably unnecessary since you are reading this.)

2. Download and extract Winpcap 3.x developer's pack from http://winpcap.polito.it/install/default.htm
   NOTE: Must download 3.x developer's pack. Do not use earlier versions.

3. Open Libnet-1.1.1/win32/Libnet.dsw (If you are using VC++6.0)
        Libnet-1.1.1/win32/Libnet-1.1.1-2002.sln (If youa are using Visual Studio .NET 20002)
        Libnet-1.1.1/win32/Libnet-1.1.1-2003.sln (If youa are using Visual Studio .NET 20003)

4. To properly setup the winpcap/libpcap dependencies.
   
   Visual Studio .NET Instructions.
   Select Tools/Options
      Under the options dialog, select Projects and then VC++ Directories
      Select Include files.
      Add the following path
      <path>wpdpack\Include
      Select Library paths.
      Add the following path
      <path>wpdpack\Lib

   Visual C++ 6.0 Instructions
   You will need to also install the Microsoft Platform SDK in order to have iphlpapi.h
   The platform SDK is available here: http://www.microsoft.com/msdownload/platformsdk/sdkupdate/
   NOTE: The include order is important, or you will get redefinition errors. Put the platform sdk's directory first.
   Select Tools/Options
   Select the Directories Tab
      Select Include files.
      Add the following path
      <path>wpdpack\Include
      <path>Program Files\Microsoft SDK\include
      Select Library paths.
      Add the following path
      <path>wpdpack\Lib
      <path>Program Files\Microsoft SDK\Lib
   
 

   
 
 
