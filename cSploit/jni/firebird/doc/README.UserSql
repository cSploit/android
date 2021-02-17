Issue: 
======
 New SQL operators to maintain user accounts.

Scope:
======
 Affects firebird versions starting with 2.5.

Document author:
=================
 Alex Peshkov (peshkoff@mail.ru)

Document date:  Fri Dec 28 2007
==============


 Added following DSQL operators:

 CREATE USER name PASSWORD 'pass' [FIRSTNAME 'text'] [MIDDLENAME 'text'] [LASTNAME 'text']; 
 ALTER USER name [SET] [PASSWORD 'pass'] [FIRSTNAME 'text'] [MIDDLENAME 'text'] [LASTNAME 'text']; 
 DROP USER name; 

 At least one of PASSWORD / FIRSTNAME / MIDDLENAME / LASTNAME is required in ALTER USER.
 Non-privileged (non-SYSDBA) user can use only ALTER USER operator with his own name 
 as name parameter in it.

 WARNING! These operators do not support 2 phase commit - changes in secure DB
 take place at first (prepare) phase, and there is no limbo state in it.
