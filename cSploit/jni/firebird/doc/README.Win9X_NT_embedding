Issue: 
======
 Embedded systems need to run engine on Windows 98/ME or Windows NT 4.0 hosts. 
 CRT dependency is also not a good thing for such systems.

Rationale:
==========

 Although these systems are out of support by Microsoft we found that as of 2009
 - NT4 and its specialized derivatives remain the platform of choice for 
   closed-circuit environments
 - Win98 is still being used as a platform for secure fund transfer systems
   (that is, sealed appliances using dial-up to transfer financial data)

 We assume the situation will continue for the foreseeable future.

Document author:
=================
 Nikolay Samofatov (skidder at users.sourceforge.net)

Solution:
=========
 MSVC7 build of Firebird 2.5 produces statically linked binaries that can work
 on Windows 98SE, Windows ME and Windows NT 4.0 SP3+ systems as well as the 
 newer platforms all the way to Windows 7.

Known issues:
=============
 You need Comctl32.dll version 5.8 or later installed for Guardian and server
 running as application to display property pages. This library is distributed
 with Internet Explorer 5.
