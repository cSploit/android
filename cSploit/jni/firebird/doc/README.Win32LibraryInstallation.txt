
One of the design goals of Firebird 1.5 is to prepare the way for multiple 
installs of the server. This will allow users to run different versions 
side by side. Firebird 1.5 does support this, although it is not well 
documented and very much requires intervention from a skilled user. Future
versions of Firebird will make this process far less complicated. In the 
meantime Firebird 1.5 needs to prepare the ground. This forces us to 
confront the issue of library installation. At the same time, Microsoft 
have taken their own steps to manage installation of different library 
versions. Taken together these two separate issues mean a new approach to 
library installation for Firebird 1.5 and beyond.

1/ Installation of Microsoft system libraries. 

The problems associated with installing different versions of Microsoft 
system libraries are so notorious that it has acquired the name
'DLL Hell'. 

From the release of Windows 2000 onwards Microsoft have made it almost 
impossible to upgrade system dll's. To resolve this Microsoft now 
recommend that each application installs local copies of any system 
libraries that are required. 

Firebird 1.5 follows this practice for all Microsoft platforms from 
Windows 2000 onwards and places the required libraries in the \bin directory 
along with the server. When installed on older systems (Win 9x, NT) it follows 
the standard practice and attempts to upgrade older versions of the system 
libraries.


2/ Installation of fbclient.dll.

Firebird 1.5 and beyond no longer use gds32.dll as the client library. 
It is now called fbclient.dll. Given the problems that Microsoft have 
had with DLL hell it wouldn't make much sense if we continued to store the 
Firebird client library in the <system> directory. And as we want to allow 
installation of multiple engines simultaneously we would be creating our 
own DLL hell if we continued the practice of using the <system> directory 
for the client library. So, from Firebird 1.5 on, the client library resides 
in the \bin directory along with all the other binaries.

A new registry key has been added and all Firebird compliant applications
should now use this to locate the correct version of Firebird that they wish 
to use. The new key is:

  HKEY_LOCAL_MACHINE\SOFTWARE\Firebird Project\Firebird Server\Instances

Firebird will guarantee that one entry under this key always exists. It will 
be known as

  "DefaultInstance"
  
and will store the path to the root directory of (yes, you've guessed it) 
the default installation. Those that don't care about particular 
installations can alway use the default instance to locate the fbclient.dll.

Future versions of Firebird will see other entries under Instances. 
Applications will be able to enumerate the registry entries to determine
which library instance the wish to load. 


3/ Supporting legacy applications and drivers.

Traditionally, applications that use InterBase or Firebird have expected to
load the gds32.dll client library from the <system> directory. Firebird 1.5
ships a tool named 'instclient.exe' that can install a clone of fbclient.dll
to the Windows System directory. This clone gets patched on the fly so that
its file version information starts in "6.3". This is done so because some
old applications do extra checks on the GDS32.DLL file version. Based on a
lot of experiments, it has been determined that 6.3 is a safe "compatible"
version number to use, much better than "1.5".

During the installation process the installer checks to see if an 
installation of InterBase or Firebird exists. If nothing is installed 
it will write a patched gds32.dll into the <system> directory. If it detects
that any possible version of Firebird or InterBase may already be installed
it will not install the gds32.dll in the <system> directory. However
you can always do so later by using the new 'instclient.exe' tool.

It is intended that future versions of Firebird will not attempt to 
install gds32.dll into the <system> directory and ultimately it will
be completely removed from the distribution.

This 'instclient.exe' tool can also install the FBCLIENT.DLL itself in the
Windows system directory. This can be preferred by some tools or
applications and we don't want to miss any use case.

instclient
Usage:
  instclient i[nstall] [ -f[orce] ] library
             q[uery] library
             r[emove] library

  where library is:  fbclient | gds32

  This utility should be located and run from the 'bin' directory
  of your Firebird installation.
  '-z' can be used with any other option, prints version

Purpose:
  This utility manages deployment of the Firebird client library
  into the Windows system directory. It caters for two installation
  scenarios:

    Deployment of the native fbclient.dll.
    Deployment of gds32.dll to support legacy applications.

  Version information and shared library counts are handled
  automatically. You may provide the -f[orce] option to override
  version checks.

  Please, note that if you -f[orce] the installation, you might have
  to reboot the machine in order to finalize the copy and you might
  break some other Firebird or InterBase(R) version on the system.



