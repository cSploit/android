SQL Language Extension: EXECUTE STATEMENT

   Implements capability to take a string which is a valid dynamic SQL
   statement and execute it as if it had been submitted to DSQL.  
   Available in triggers and stored procedures. 

Author:
   Alex Peshkoff <pes@insi.yaroslavl.ru>

Syntax may have three forms.

Syntax 1
========

EXECUTE STATEMENT <string>;

Description

Executes <string> as SQL operation. It should not return any data rows.
Following types of SQL operators may be executed:

   * Insert, Delete and Update.
   * Execute Procedure.
   * Any DDL (except Create/Drop Database).

Sample:

CREATE PROCEDURE DynamicSampleOne (Pname VARCHAR(100))
AS
DECLARE VARIABLE Sql VARCHAR(1024);
DECLARE VARIABLE Par INT;

BEGIN
SELECT MIN(SomeField) FROM SomeTable INTO :Par;
Sql = 'EXECUTE PROCEDURE ' || Pname || '(';
Sql = Sql || CAST(Par AS VARCHAR(20)) || ')';
EXECUTE STATEMENT Sql;
END


Syntax 2
=========

EXECUTE STATEMENT <string> INTO :var1, ., :varn;

Description

Executes <string> as SQL operation, returning single data row. Only
singleton SELECT operators may be executed with this form of EXECUTE
STATEMENT.

Sample:

CREATE PROCEDURE DynamicSampleTwo (TableName VARCHAR(100))
AS
DECLARE VARIABLE Par INT;

BEGIN
EXECUTE STATEMENT 'SELECT MAX(CheckField) FROM ' || TableName INTO :Par;
IF (Par > 100) THEN
  EXCEPTION Ex_Overflow 'Overflow in ' || TableName;
END


Syntax 3
========

FOR EXECUTE STATEMENT <string> INTO :var1, ., :varn DO
<compound-statement>;

Description

Executes <string> as SQL operation, returning multiple data rows. Any SELECT
operator may be executed with this form of EXECUTE STATEMENT.

Sample:

CREATE PROCEDURE DynamicSampleThree (TextField VARCHAR(100), TableName VARCHAR(100))
	RETURNING_VALUES (Line VARCHAR(32000))
AS
DECLARE VARIABLE OneLine VARCHAR(100);

BEGIN
Line = '';
FOR EXECUTE STATEMENT 'SELECT ' || TextField || ' FROM ' || TableName
	INTO :OneLine
DO
  IF (OneLine IS NOT NULL) THEN
    Line = Line || OneLine || ' ';
SUSPEND;
END


N O T E S
=========

I. For all forms of EXECUTE STATEMENT SQL, the DSQL string can not contain 
any parameters. All variable substitution into the static part of the SQL 
statement should be performed before EXECUTE STATEMENT.

EXECUTE STATEMENT is potentially dangerous, because:

  1. At compile time there is no checking for the correctness of the SQL 
     statement.  No checking of returned values (in syntax forms 2 & 3 )
     can be done.
  2. There can be no dependency checks to ensure that objects referred to in
     the SQL statement string are not dropped from the database or modified 
     in a manner that would break your statement. For example, a DROP TABLE
     request for the table used in the compiled EXECUTE PROCEDURE statement
     will be granted.
  3. In general, EXECUTE STATEMENT operations are rather slow, because the
     statement to be executed has to be prepared each time it is executed
     by this method.

These don't mean that you should never use this feature.  But, please, 
take into account the given facts and apply a rule of thumb to use
EXECUTE STATEMENT only when other methods are impossible, or perform even
worse than EXECUTE STATEMENT. 

To help (a little) with bugfixing, returned values are strictly checked for 
correct datatype. This helps to avoid some errors where unpredictable 
type-casting would otherwise cause exceptions in some conditions but not
in others.  For example, the string '1234' would convert to an int 1234, 
but 'abc' would give a conversion error.

II. If the stored procedure has special privileges on some objects, the 
dynamic statement submitted in the EXECUTE STATEMENT string does not 
inherit them. Privileges are restricted to those granted to the user who 
is executing the procedure.

III. Even though EXECUTE STATEMENT is available in triggers and stored procedures
only, it's possible to call it directly from DSQL statements, using EXECUTE BLOCK
facility. Example:

set term ^;
execute block returns(i bigint) as
begin
	execute statement 'select avg(rdb$relation_id)
	from rdb$database' into :i;
	suspend;
end^
set term ;^

However, for such contrived code, it's better to call EXECUTE BLOCK in a stored
procedure or to construct the query dynamically in the client side.
For information on EXECUTE BLOCK, see the README.execute_block document.

