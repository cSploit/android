README to instreg.exe and instsvc.exe
-------------------------------------
Authors:   
  Original:    Alex Peshkov (peshkoff at mail.ru)
  Revised by:  Olivier Mascia (om at tipgroup.com)

NOTE :: To solve any potential issues with long paths containing spaces 
  or special (non ascii) characters, these tools no longer take the 
  RootDirectory as a command-line argument.  Both binaries must be installed 
  in (or copied to) the /bin directory beneath your Firebird root directory.  

  (Root directory == directory root where firebird.conf and security2.fdb are
                     installed.)  

  For example, if they are located in C:\FB20\bin, the root directory will
  be deduced as C:\FB20.

============
INSTREG.EXE
============
The purpose of this utility is to install the Registry keys for Firebird. 
Because its task is to deduce the RootDirectory path as the directory 
immediately above its own location and to write this to the Registry as an 
absolute path, instreg.exe must be installed in the /bin sub-directory of the
Firebird root directory (see note above).  

The Registry key is *not* required for the engine to run;  however, many 
third-party tools expect to read it in order to work.  The key stored is 
    HKLM\SOFTWARE\Firebird Project\Firebird Server\Instances
and contains one string value:  DefaultInstance.

This 'DefaultInstance' string value contains the full path (backslash
terminated) to the root directory of the installation. This setup is built
so that management of named multi-instances of the server will be possible in
a next version. Currently, the one and only instance receives the hard-coded
name 'DefaultInstance'. There is nothing else stored in the registry. Any
version or configuration information is to be found by third-parties tools
in the relevant firebird.conf file. This file can be located thanks to the
root path stored in the ...\Instances\DefaultInstance value.

Usage:
  instreg i[nstall]
          r[emove]

  '-z' can be used with any other option, prints the Firebird software 
       version as the first line of output.
  To get brief text help, just invoke instreg.exe with no arguments.

============
INSTSVC.EXE
============
Firebird provides a standard routine to manage the Firebird Service on 
WinNT/2000/XP platforms - instsvc.exe.

Console messages
----------------
Output of Windows error messages (if any) has been enhanced so that any 
required transliteration of ansi strings to the OEM charset of the console 
window is done. This means people using a non-English Windows will get 
Windows system error messages in their language with correct accents and 
so on.

Security related features
-------------------------
The '-login' (or -l) option previously implemented has been extensively 
revised and enhanced. It can now be used to set up the service(s) to run 
under a specific user account (presumably with limited privileges on the 
system) instead of the default "LocalSystem".

Usage
------
  instsvc i[nstall] [ -s[uperserver]* | -c[lassic] | -m[ultithreaded] ]
                    [ -a[uto]* | -d[emand] ]
                    [ -g[uardian] ]
                    [ -l[ogin] username [password] ]
                    [ -n[ame] instance ]
                    [ -i[nteractive] ]

          sta[rt]   [ -b[oostpriority] ]
                    [ -n[ame] instance ]
          sto[p]    [ -n[ame] instance ]
          q[uery]
          r[emove]  [ -n[ame] instance ]

  This utility should be located and run from the 'bin' directory
  of your Firebird installation.

  '*' denotes the default values
  '-z' can be used with any other option, prints version
  'username' refers by default to a local account on this machine.
  Use the format 'domain\username' or 'server\username' if appropriate.


Examples
========
assuming Firebird is installed (copied to its installed location).

  Superserver and auto-startup are now the default.

instsvc i OR instsvc install
  sets up a Superserver as a service
  in auto-startup (started at boot-up) mode
  under the default system identity, named "LocalSystem".

instsvc i -g OR instsvc install -guardian 
  as above, but starts the Guardian service first.

The options "-demand" (-d) and "-classic" (-c) allow the defaults to be 
changed, so the classic server (instead of the superserver) can be 
configured as a service and either service can be configured to start 
on demand instead of starting at boot time.

To remove the service(s)
------------------------
Services must be stopped before being removed.
  This could be automated with 3 lines of code 
  but it is better to leave it as is for safety. 

instsvc r OR instsvc remove
  removes the service definition. 
  If Guardian was configured, both the guardian and 
  server services will be removed. 

To start and stop the installed service(s)
------------------------------------------
instsvc sta or instsvc start
  starts the server or Guardian + server, 
  according to what was configured by the install command.
instsvc sto or instsvc stop
  stops the server or Guardian + server, 
  according to what was configured by the install command.

To get a server condition report
--------------------------------
This is a new command:

instsvc q or instsvc query
  Prints a report of exactly *what* is configured and *how*, 
  as well as whether the services are running or stopped.

To use the secure login feature
-------------------------------
Option "-login" requires a username and a password:

instsvc i -g -login user pass
  This would configure FB superserver as auto-start, with Guardian, 
  to run under the identity "user" whose password is pass. 
 
  * If the password is included, the password should not begin 
    with '-'.
 
  * If the password is omitted from the command-line, instsvc 
    will prompt for it.  In that case, the password will be masked 
    with '*' as it is typed.

While configuring the services as above, instsvc automatically grants 
the Windows operating system privilege named "Log on as a Service" to 
the specified user. This is required, otherwise the service would not 
be allowed to start.  Instsvc is silent about it unless it fails.

To take care of the possibility that the user logging in via the -login 
switch might be an ordinary user with no administrator privileges on the 
machine, instsvc adjusts the default security descriptor of the service(s) 
so that this user can start/stop/query them.  This is necessary to enable 
the Guardian (also running as this user) to start the FB server. The user 
will not be able to uninstall the service(s) nor change their 
configuration. 

NOTE
----
Instsvc does not CREATE the "user" on the Windows machine. That would not 
be a good idea. The admin might want to create the user as part of the 
local machine or as part of a domain and instsvc has no particular need to 
usurp the admin's responsibility for creating users. 

The user needs to have read/write access to all databases and the 
firebird.log file.  For security reasons, write access to firebird.conf 
and Firebird executables should NOT be given.

On Windows NT/2K/XP it is common to include the domain or server prefix for a
user defined on a domain or locally on another server.  In the above examples,
"user" might take the form of:
   DOMAIN\user or
   SERVER\user
With no prefix, the user is implicitly a local user on the current machine.

/* ends */
