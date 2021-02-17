SQL Language Extension: default paremeters in stored procedures

Function:
	allow input parameters of stored procedures to have optional default values

Autor:
	Vlad Khorsun <hvlad at users.sourceforge.net>

Syntax:
	same as default value definition of column or domain, except that
you can use '=' in place of 'DEFAULT' keyword. Parameters with default values 
must be last in parameter list i.e. not allowed to define parameter without 
default value after parameter with default value. Caller must set first few 
parameters, i.e. not allowed set param1, param2, miss param3, set param4...


Default values substitution occurs at run-time. If you define procedure with 
defaults (say P1), call it from another procedure (say P2) and skip some last 
parameters (with default value) then default values for P1 will be substituted 
by the engine at time of the beginning of execution P1. Then if you change 
default values for P1 it is not need to recompile P2. But it is still necessary 
to disconnect all client connections, for more details see IB6 Beta documentation 
"Data Definition Guide", section "Altering and dropping procedures in use"


Examples:

CONNECT ... ;

CREATE PROCEDURE P1 (X INTEGER = 123)
RETURNS (Y INTEGER)
AS
BEGIN
  Y = X;
  SUSPEND;
END;
COMMIT;

 SELECT * FROM P1;

           Y
============

         123

EXECUTE PROCEDURE P1;

           Y
============
         123


CREATE PROCEDURE P2
RETURNS (Y INTEGER)
AS
BEGIN
  FOR SELECT Y FROM P1 INTO :Y
  DO SUSPEND;
END;
COMMIT;


SELECT * FROM P2;

           Y
============

         123


ALTER PROCEDURE P1 (X INTEGER = CURRENT_TRANSACTION)
         RETURNS (Y INTEGER)
AS
BEGIN
  Y = X;
  SUSPEND;
END;
COMMIT;

SELECT * FROM P1;

           Y
============

        5875

SELECT * FROM P2;

           Y
============

         123

COMMIT;

CONNECT  ... ;

SELECT * FROM P2;

           Y
============

        5880


Notes:
	default sources and BLR's are kept in RDB$FIELDS

