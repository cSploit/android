Firebird Database Server $MAJOR.$MINOR.$RELEASE (Alpha1)
========================================================


############## NOTE ####################################
#                                                      #
# The installer is in an experimental state.           #
# This is not the definitive version for Firebird 3.0. #
#                                                      #
# o Layout and work sequence of the installer is not   #
#    definitive.                                       #
# o Options available are subject to change.           #
# o Method of library deployment has not been          #
#   finalized.                                         #
# o How we will support Legacy Auth is experimental    #
#   - Adding SYSDBA works                              #
#   - Modifying SYSDBA from the installer doesn't      #
# o Don't be surprised to find other anomalies.        #
#                                                      #
############## END NOTE ################################


This document is a guide to installing this package of
Firebird $MAJOR.$MINOR on the Windows platform. These notes refer
to the installation package itself, rather than
Firebird $MAJOR.$MINOR in general. In addition, these notes are
primarily aimed at users of the binary installer.

It is assumed that readers of this document are already
familiar with Firebird 2.5. If you are evaluating
Firebird $MAJOR.$MINOR as part of a migration from Fb 2.5 you are
advised to review the Fb $MAJOR.$MINOR documentation to
understand the changes made between 2.5 and $MAJOR.$MINOR.


Contents
--------

o Before installation
o Problems with installation of MS VC runtime libraries
o Known installation problems
o Uninstallation
o Other Notes
o Installation from a batch file


Before installation
-------------------

It is recommended that you UNINSTALL all previous
versions of Firebird or InterBase before installing
this package. It is especially important to verify that
fbclient.dll and gds32.dll are removed from <system32>.


Problems with installation of MS VC runtime libraries
-----------------------------------------------------

Much work has been done to ensure that the MS Visual
C runtime libraries are correctly installed by the
binary installer. Since v2.1.2 Firebird will work with
locally deployed instances of the runtime libraries.
This especially simplifies deployment of the Firebird
client or embedded dll with your own application.

However, in case of problems it may be necessary to
deploy the official vcredist.exe. The correct versions
for this build of Firebird can be found here:

    http://www.microsoft.com/downloads/details.aspx?familyid=32BC1BEE-A3F9-4C13-9C99-220B62A191EE&displaylang=en

  and x64 here:

    http://www.microsoft.com/downloads/details.aspx?familyid=90548130-4468-4BBC-9673-D6ACABD5D13B&displaylang=en


Other Known installation problems
---------------------------------

o It is only possible to use the binary installer
  to install the default instance of Firebird $MAJOR.$MINOR. If
  you wish to install additional, named instances you
  should manually install them with the zipped install
  image.

o Unfortunately, the installer cannot reliably detect
  if a previous version of Firebird Classic server
  is running.

o There are known areas of overlap between the
  32-bit and 64-bit installs:

  - The service installer (instsvc) uses the same
    default instance name for 32-bit and 64-bit
    installations. This is by design. Services exist
    in a single name space.

  - If the 32-bit and 64-bit control panel applets are
    installed they will both point to the same default
    instance.

o When installing under Vista be sure to install as an
  administrator. ie, if using the binary installer
  right click and choose 'Run as administrator'.
  Otherwise the installer will be unable to start the
  Firebird service at the end of installation.

o Libraries deployed by instclient will fail to load if
  the MS runtime libraries have not been installed
  correctly. In case of problems users should install
  the appropriate version of vcredist.exe mentioned
  above.


Uninstallation
--------------

o It is preferred that this package be uninstalled
  correctly using the uninstallation application
  supplied. This can be called from the Control Panel.
  Alternatively it can be uninstalled by running
  unins000.exe directly from the installation
  directory.

o If Firebird is running as an application (instead of
  as a service) it is recommended that you manually
  stop the server before running the uninstaller. This
  is because the uninstaller cannot stop a running
  application. If a server is running during the
  uninstall the uninstall will complete with errors.
  You will have to delete the remnants by hand.

o Uninstallation leaves five files in the install
  directory:

  - databases.conf
  - firebird.conf
  - fbtrace.conf
  - firebird.log
  - security2.fdb

  This is intentional. These files are all
  potentially modifiable by users and may be required
  if Firebird is re-installed in the future. They can
  be deleted if no longer required.

o A new feature of the uninstaller is an option to
  run it with the /CLEAN parameter. This will check
  the shared file count of each of the above files. If
  possible it will delete them.

o Uninstallation will not remove the MS VC runtime
  libraries from the system directory. These can be
  removed manually via the control panel, but this
  should not be required under normal circumstances.


Other Notes
-----------

  Firebird requires WinSock2. All Win32 platforms
  should have this, except for Win95. A test for the
  Winsock2 library is made during install. If it is
  not found the install will fail. You can visit
  this link:

    http://support.microsoft.com/default.aspx?scid=kb;EN-US;q177719

  to find out how to go about upgrading.


Installation from a batch file
------------------------------

The setup program can be run from a batch file.
Please see this document:

    installation_scripted.txt

for full details.

