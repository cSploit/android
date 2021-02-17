Issue: 
======
 If the LocalSystem user is allowed to install the Firebird Service,
 it could make the whole system accessible to a malicious attacker.  

Scope:
======
 Affects Windows NT platforms.

Document author:
=================
 Alex Peshkov (peshkoff@mail.ru)

Document date:  2003/06/22
==============


 Firebird installation kits for Windows NT systems, i.e. those that
 support services, currently provide a route into the host system 
 for any hacker who finds a new security hole in Firebird.  All of 
 the current kits install the Firebird service to run under the 
 LocalSystem account. Through Firebird, the attacker can get 
 LocalSystem access to the system.

The steps to fix things manually are simple:

1)  add the user 'firebird' as a member of the Domain users group,  
    with default rights
 
2)  grant this user write access to all databases, including 
    security2.fdb (isc4.gdb in pre-1.5 versions), and the
    firebird.log file

3)  grant the user 'firebird' rights to "Login as service" 

4)  make the Firebird services (FirebirdServer and FirebirdGuardian,
    if used, log in with username 'firebird'

Solution:
=========
 Alex Peshkov

 People writing installers should note that Firebird's standard routine 
 to install and manage the Firebird Service on WinNT/2000/XP platforms 
 (instsvc.exe) was upgraded in version 1.5 by the addition of an
 optional L[ogin] switch to the {install} command.  It is strongly
 recommended that you employ this switch in the Windows kits, to make
 the 'firebird' user, not LocalSystem, the default account under which
 the Firebird Service logs in.

 For more details, see the document README.instsvc
 switch to (see instsvc.exe).
