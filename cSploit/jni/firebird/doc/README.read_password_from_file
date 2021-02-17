Issue:
======
 All command-line utilities which support -password parameter are 
 vulnerable to password sniffing, especially when they're run from 
 scripts. Since 2.1, all Firebird utilities replace argv[PASSWORD] 
 with *, but better solution for hiding password from others in 
 process list should be reading it from file or asking for it on 
 stdin.

Scope:
======
 Security issue.

Document author:
=================
 Alex Peshkov (peshkoff@mail.ru)

Document date:  2008-11-30
==============


 All utilities have new switch 
-fetch_password 
 which may be abbreviated according with utility rules. 
 The exception is QLI, where -F should be used. 
 
 Switch has required parameter - name of file with password. I.e.: 
isql -user sysdba -fet passfile server:employee 
 will load password form file "passfile", using its first line 
 as password. 
 
 One can specify "stdin" as file name to make password be read 
 from stdin. If stdin is terminal, prompt:
Enter password: 
 will be printed. 
 
 For posix users - if you specify '-fetch /dev/tty' you will also 
 be promted. This may be useful if you need to restore from stdin: 
bunzip2 -c emp.fbk.bz2 | gbak -c stdin /db/new.fdb -user sysdba -fetch /dev/tty 
