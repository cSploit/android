SQL Language Extension: ALTER DATABASE SET/DROP LINGER

   Implements capability to manage database linger.

Author:
   Alex Peshkoff <peshkoff@mail.ru>

Syntax is:

   ALTER DATABASE SET LINGER TO {seconds};
   ALTER DATABASE DROP LINGER;

Description:

Makes it possible to set and clear linger value for database.

Database linger makes engine (when running in SS mode) do not close database immediately after
last attachment is closed. This helps to increase performance when database is often opened/closed
with almost zero price.

To set linger for database do:
   ALTER DATABASE SET LINGER TO 30;		-- will set linger interval to 30 seconds

To reset linger for database do:
   ALTER DATABASE DROP LINGER;			-- will make engine do not delay closing given database
   ALTER DATABASE SET LINGER TO 0;		-- another way to clean linger settings

Notice.
Sometimes it may be useful to turn off linger once to force server to close some database not
shutting it down. Dropping linger for it is not good solution - you will have to turn it on
manually later. To perform this task it's better to use GFIX utility with new switch 'NOLinger' -
it makes database be closed immediately when last attachment is gone no matter of linger interval
set in database. Next time linger will work normally. Services API is also available for it -
see details in README.services_extension).
