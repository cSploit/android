PSQL stack trace

Function: 
	Send to client within status-vector simple stack trace with names of stored procedures
	and/or triggers. Status-vector appends with following items :

	isc_stack_trace, isc_arg_string, <string length>, <string>

	isc_stack_trace is a new error code with value of 335544842L

	Stack trace is represented by one string and consists from all the sp/trigger names 
	starting from point where exception occurred up to most outer caller. Maximum length of 
	that string is 2048 bytes. If actual trace is longer then it will be truncated to this limit.

Author:
	Vlad Khorsun <hvlad at users.sourceforge.net>

Examples:

0. Create metadata:

CREATE TABLE ERR (ID INT NOT NULL PRIMARY KEY, NAME VARCHAR(16));

CREATE EXCEPTION EX '!';

CREATE OR ALTER PROCEDURE ERR_1 AS
BEGIN
  EXCEPTION EX 'ID = 3';
END;

CREATE OR ALTER TRIGGER ERR_BI FOR ERR BEFORE INSERT AS
BEGIN
  IF (NEW.ID = 2)
  THEN EXCEPTION EX 'ID = 2';

  IF (NEW.ID = 3)
  THEN EXECUTE PROCEDURE ERR_1;

  IF (NEW.ID = 4)
  THEN NEW.ID = 1 / 0;
END;

CREATE OR ALTER PROCEDURE ERR_2 AS
BEGIN
  INSERT INTO ERR VALUES (3, '333');
END;



1. User exception from trigger:

SQL> INSERT INTO ERR VALUES (2, '2');
Statement failed, SQLCODE = -836
exception 3
-ID = 2
-At trigger 'ERR_BI'


2. User exception from procedure called by trigger:

SQL> INSERT INTO ERR VALUES (3, '3');
Statement failed, SQLCODE = -836
exception 3
-ID = 3
-At procedure 'ERR_1'
At trigger 'ERR_BI'


3. Division by zero occurred in trigger:

SQL> INSERT INTO ERR VALUES (4, '4');
Statement failed, SQLCODE = -802
arithmetic exception, numeric overflow, or string truncation
-At trigger 'ERR_BI'


4. User exception from procedure:

SQL> EXECUTE PROCEDURE ERR_1;
Statement failed, SQLCODE = -836
exception 3
-ID = 3
-At procedure 'ERR_1'


5. User exception from procedure with more deep call stack:

SQL> EXECUTE PROCEDURE ERR_2;
Statement failed, SQLCODE = -836
exception 3
-ID = 3
-At procedure 'ERR_1'
At trigger 'ERR_BI'
At procedure 'ERR_2'
