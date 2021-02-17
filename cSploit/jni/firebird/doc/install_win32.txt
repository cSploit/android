-----------------------------------------------------------
Firebird 1.5 installation notes
-----------------------------------------------------------

1. GENERIC INFORMATION

  The distribution of the version 1.5 has a number of
  changes as compared with 1.0.

1.1. Some files were renamed, namely:

  ibserver.exe -> fbserver.exe (SS architecture)
  ibguard.exe -> fbguard.exe
  interbase.msg -> firebird.msg
  interbase.log -> firebird.log
  isc4.gdb -> security.fdb

  Regarding the last item, it should be mentioned that
  now the recommended extension for database files is
  ".fdb" to avoid possible conflicts with "system restore"
  feature of Windows ME/XP operating systems.

1.2. Changes in the client library

  Now the client library is named "fbclient.dll". To provide a
  compatibility with existing applications, a gateway library
  "gds32.dll" can be installed to the Windows System Directory.
  See instclient.exe utility for this. This tool can also properly
  install fbclient.dll to Windows System Directory, should you like or
  need so.
  It's recommended to use native fbclient.dll library
  in newly developed applications. All server utilities (gbak,
  gfix, etc) uses only fbclient.dll and do not require gds32.dll.

1.3. New configuration file

  ibconfig file is no longer used and it has been replaced with
  firebird.conf, which is a part of new configuration manager and
  allows you to use both old and some new options. You can see
  the list of all supported configuration options (as well as
  their default values) in the distributed version of this file.
  
1.4. Classic Server (CS)

  Since Firebird 1.5, Classic engine is included in win32
  distributions. The file of this server version is named
  fb_inet_server.exe and support TCP/IP and NetBEUI network
  protocols (local protocol is not supported). The usage of
  the Classic engine doesn't differ from its SS variant,
  except of the architecture specifics (one server process
  is running per client connection).

1.5. Ability to install and work with existent IB/FB1 server

  There were some changes done in the system object names
  to allow FB 1.5 to be installed and used on the computer
  which already has IB/FB1 installed. FB 1.5 also uses
  another registry keys. If you setup the server to use
  different network ports, you also can run a few server
  instances simultaneously or run FB 1.5 in the same time
  with IB/FB1.

1.6. Compatibility with previous versions

  Name of the local IPC port is no longer compatible, i.e.
  with default server settings you cannot connect to it from
  applications using old client library (gds32.dll). If
  necessary, you can setup the server to use old name of the
  IPC map via the configuration file.

  New version uses updated ODS (10.1). It doens't cause any
  incompatibilities with previous versions, but you should be
  aware of this fact. Engine doesn't upgrade ODS automatically
  and Firebird 1.0 and 1.5 can use both ODS 10.0 and 10.1 databases.
  Regardless of the above, backup/restore is still the
  recommended procedure of migrating databases to the different
  version of the server.

  Since a number of bugs has been fixed, the behaviour of the
  database may change after downgrading from v1.5 to v1.0.
  The datailed information of all such issues (as well as
  appropriate recommendations) will be published separately.

  Some specifics of the server work have been changed in v1.5.
  For more detailed information see configuration file
  (firebird.conf) and release notes (WhatsNew.txt).

2. INSTALLATION

  The installation of FB 1.5 doesn't practically differ from
  previous versions.

2.1. Required steps

  If you don't have a special setup program (it's distributed
  separately) the steps are the following:

  - unzip the archive into the separate directory (since a few 
    file names were changed, it doesn't make sense to unzip
    v1.5 files into the directory with IB/FB1)
  - change the current directory to <root>\bin (here and below 
    <root> is the directory where v1.5 files are located)
  - run instreg.exe:
      instreg.exe install
    it causes the installation path to be written into the registry
    (HKLM\Software\Firebird Project\Firebird Server\Instances)
  - if you want to register a service, run also instsvc.exe:
      instsvc.exe install
    (This is a Windows NT/2K/XP specific step.)
  - optionally, you can copy both fbclient.dll and gds32.dll
    to the OS system directory. To do so, use the provided instclient.exe
    tool.
    
    Usage:
      instclient i[nstall] [ -f[orce] ] library
                 q[uery] library
                 r[emove] library

      where library is:  f[bclient] | g[ds32]

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
  
2.2. Installation of CS

  To install the CS engine, the only difference is the additional
  option for instsvc.exe:
      instsvc.exe install -classic

  It means that you may have only one copy of the engine (either
  fbserver.exe or fb_inet_server.exe) to be installed as a service.

2.3. Simplified setup

  If you don't need a registered service, then you may avoid running
  both instreg.exe and instsvc.exe. In this case you should just unzip
  the archive into a separate directory and run the server:
    fbserver.exe -a
  It should treat its parent directory as a root directory in this
  case.
  You can also use instclient.exe to copy client library to System.

2.4. Uninstallation

  To remove FB 1.5 you should:

  - stop the server (running "instsvc.exe stop" for instance)
  - run "instsvc.exe remove"
  - run "instreg.exe remove"
  - run "instclient.exe remove fbclient" and/or "insclient.exe remove gds32"
    if you used that tool to install those libraries the OS system directory.
  - delete installation directory

3. INFORMATION ABOUT THIS VERSION

  All changes included in FB 1.5 are briefly described in file:
    <root>\doc\WhatsNew.txt
  Full documentation about new features and bugfixes will be
  published in the official Release Notes, which will be available
  in the final release.
