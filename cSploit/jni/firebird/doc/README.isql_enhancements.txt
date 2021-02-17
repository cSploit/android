Isql enhancements in Firebird v2.
---------------------------------

Author: Claudio Valderrama C. <cvalde at usa.net>

1) Command line switch -b to bail out on error when used in non-interactive mode.

When using scripts as input in the command line, it may be totally unappropriate
to let isql continue executing a batch of commands after an error has happened.
Therefore, the "-b[ail]" option was created. It will stop at the first error it
can detect. Most cases have been covered, but if you find some error that's not
recognized by isql, you should inform the project, as this is a feature in progress.
When isql stops, it means it will no longer execute any command in the input script
and will return an error code to the operating system. At this time there aren't
different error codes. A return non-zero return code should be interpreted as failure.
Depending on other options (like -o, -m and -m2) , isql will show the error message
on screen or will send it to a file.

Some features:

- Even if isql is executing nested scripts, it will cease all execution and will
return to the operating system when it detects an error. Nested scripts happen
when a script A is used as isql input but in turn A contains an INPUT command to
load script B an so on. Isql doesn't check for direct or indirect recursion, thus
if the programmer makes a mistake and script A loads itself or loads script B that
in turn loads script A again, isql will run until it exhaust memory or an error
is returned from the database, at whose point -bail if activated will stop all
activity.

- The line number of the failure is not yet known. It has been a private test feature
for some years but needs more work to be included in the official isql.

- DML errors will be caught when being prepared or executed, depending on the type
of error.

- DDL errors will be caught when being prepared or executed by default, since isql
uses AUTODDL ON by default. However, if AUTO DLL is OFF, the server only complains
when the script does an explicit COMMIT and this may involve several SQL statements.

- The feature can be enabled/disabled interactively or from a script by means of the
SET BAIL [ON | OFF]
command. As it's the case with other SET commands, simply using SET BAIL will toggle
the state between activated and deactivated. Using SET will display the state of
the switch among many others.

- Even if BAIL is activated, it doesn't mean it will change isql behavior. An
additional requirement should be met: the session should be non-interactive. A
non-interactive session happens when the user calls isql in batch mode, giving it
a script as input. Example:
isql -b -i my_fb.sql -o results.log -m -m2
However, if the user loads isql interactively and later executes a script with the
input command, this is considered an interactive session even though isql knows it's
executing a script. Example:
isql
Use CONNECT or CREATE DATABASE to specify a database
SQL> set bail;
SQL> input my_fb.sql;
SQL> ^Z
Whatever contents the script has, it will be executed completely even with errors
and despite the BAIL option was enabled.


2) SET HEADING ON/OFF option.

Some people consider useful the idea of doing a SELECT inside isql and have the
output sent to a file, for additional processing later, specially if the number
of columns make isql display unpractical. However, isql by default prints column
headers and in this scenario, they are a nuisance. Therefore, the feature (that was
previously the fixed default) can be enabled/disabled interactively or from a script
by means of the
SET HEADing [ON | OFF]
command. As it's the case with other SET commands, simply using SET HEAD will toggle
the state between activated and deactivated. Using SET will display the state of
the switch among many others.
Note: this switch cannot be deactivated with a command line parameter.


3) Command line switch -m2 to send output of statistics and plans to the same file
than the rest of the output.

When the user specifies that the output should be sent to a file, two possibilities
have existed for years: either the command line switch -o followed by a file name
is used or once inside isql, the command OUTput followed by a file name is used at
any time, be it an interactive or a batch session. To return the output to the
console, simply typing OUTput; is enough. So far so good, but error messages
don't go to that file. They are shown in the console. Then isql developed the -m
command line switch to melt the error messages with the normal output to whatever
place the output was being redirected. This left still another case: statistics
about operations (SET STATs command) and SQL plans as the server returns them
(SET PLAN and SET PLANONLY commands) are considered diagnostic messages and thus
were sent always to the console. What the -m2 command line switch does is to
ensure that such information about stats and plans goes to the same file the output
has been redirected to.
Note: neither -m nor -m2 have interactive counterparts through the SET command.
They only can be specified in the command line switches for isql.

4) Ability to show the line number where an error happened in a script.

In previous versions, the only reasonable way to know where a script had caused
an error was using the switched -e for echoing commands, -o to send the output
to a file and -m to merge the error output to the same file. This way, you could
observe the commands isql executed and the errors if they exist. The script continued
executing to the end. The server only gives a line number related to the single command
(statement) that it's executing, for some DSQL failures. For other errors, you
only know the statement caused problems.

With the addition of -b for bail as described in (1), the user is given the power
to tell isql to stop executing scripts when an error happens, but you still need to
echo the commands to the output file to discover which statement caused the failure.

Now, the ability to signal a script-related line number of a failure enables the
user to go to the script directly and find the offending statement. When the server
provides line and column information, you will be told the exact line in the script
that caused the problem. When the server only indicates a failure, you will be told
the starting line of the statement that caused the failure, related to the whole script.

This feature works even if there are nested scripts, namely, if script SA includes
script SB and SB causes a failure, the line number is related to SB. When SB is
read completely, isql continues executing SA and then isql continues counting lines
related to SA, since each file gets a separate line counter. A script SA includes
SB when SA uses the INPUT command to load SB.

Lines are counted according to what the underlying IO layer considers separate lines.
For ports using EDITLINE, a line is what readline() provides in a single call.
Line length is limited to 32767 bytes, but this has been always the limit.

5) SHOW SYSTEM command shows predefined UDFs.

The SHOW <object_type> command is meant to show user objects of that type.
The SHOW SYSTEM commmand is meant to show system objects, but until now it
only showed system tables. Now it lists the predefined, system UDFs incorporated
into FB 2. It may be enhanced to list system views if we create some of them
in the future.

6) -r2 command line parameter.

The sole objective of this parameter is to specify a case-sensitive role name.
With -r, the default switch, roles provided in the command line are uppercased.
With -r2, the role is passed to the engine exactly as typed in the command line.

7) Binary text is shown in hex.

This feature was contributed before the firt FB2 alpha. It will show content
from CHAR/VARCHAR columns in hex when the character set is binary (octets).
This feature is currently hardcoded: it can't be disabled.

8) SET SQLDA_DISPLAY ON/OFF option.

This option exists long before FB1 and it was available previously in DEBUG builds
only. Now this has been made public. It shows the raw SQLVARs information.
Each SQLVAR represents a field in the XSQLDA, the main structure used in the FB API
to talk to clients, transferring data into and out of the server. This option
is not accounted for when you type
SET;
in isql to see the state for most options.

