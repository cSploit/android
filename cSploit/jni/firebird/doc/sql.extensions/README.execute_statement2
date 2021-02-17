SQL Language Extension: EXECUTE STATEMENT

   Extends already implemented EXECUTE STATEMENT with ability to query
external Firebird's databases. Introduced support for input parameters.

Authors:
   Vlad Khorsun <hvlad@users.sourceforge.net>
   Alex Peshkoff <pes@insi.yaroslavl.ru>


Syntax and notes :

[FOR] EXECUTE STATEMENT <query_text> [(<input_parameters>)]
    [ON EXTERNAL [DATA SOURCE] <connection_string>]
    [WITH AUTONOMOUS | COMMON TRANSACTION]
    [AS USER <user_name>]
    [PASSWORD <password>]
    [ROLE <role_name>]
    [WITH CALLER PRIVILEGES]
    [INTO <variables>]
    
- Order of clauses below is not fixed :
    [ON EXTERNAL [DATA SOURCE] <connection_string>]
    [WITH AUTONOMOUS TRANSACTION | COMMON TRANSACTION]
    [AS USER <user_name>]
    [PASSWORD <password>]
    [ROLE <role_name>]
    [WITH CALLER PRIVILEGES]
	
- Duplicate clauses are not allowed

- if you use both <query_text> and <input_parameters> then you must
    enclose <query_text> into round brackets, for example :
    EXECUTE STATEMENT (:sql) (p1 := 'abc', p2 := :second_param) ...
    
- both named and not named input parameters are supported. Mix of named and not
	named input parameters in the same statement is not allowed.

- syntax of named input parameters 
	<input_parameters> ::= 
		  <named_parameter> 
		| <input_parameters>, <named_parameter>
		
	<named_parameter> ::= 
		  <parameter name> := <expression>

	Syntax above introduced new parameter value binding operator ":=" to avoid 
	clashes with future boolean expressions.
	This syntax may be changed in release version.

- if ON EXTERNAL DATA SOURCE clause is omitted then
	a) statement will be executed against current (local) database
	b) if AS USER clause is omitted or <user_name> equal to CURRENT_USER
	and if ROLE clause is omitted or <role_name> equal to CURRENT_ROLE
	then the statement is executed in current connection context
	c) if <user_name> is not equal to CURRENT_USER or <role_name> not equal to CURRENT_ROLE
	then the statement is executed in separate connection established inside the same
	engine instance (i.e. created new internal connection without Y-Valve and remote layers).

- <connection_string> is usual connection string accepted by isc_attach_database, 
    i.e. [<host_name><protocol_delimiter>]database_path.

- connection to the external data source is made using the same character set as
	current (local) connection is used.

- AUTONOMOUS TRANSACTION started new transaction with the same parameters as current 
	transaction. This transaction will be committed if the statement is executed ok or rolled 
	back if the statement is executed with errors.

- COMMON TRANSACTION 
	a) started new transaction with the same parameters as current transaction, or 
	b) used already started transaction in this connection, or 
	c) used current transaction if current connection is used.
	This transaction lifetime is bound to the lifetime of current (local) transaction 
	and commits\rolled back the same way as current transaction.

- by default COMMON TRANSACTION is used

- if PASSWORD clause is omitted then 
	a) if <user_name> is omitted, NULL or equal to CURRENT_USER value and
		if <role_name> is omitted, NULL or equal to CURRENT_ROLE value
		then trusted autentication is performed, and
		a1) for current connection (ON EXTERNAL DATA SOURCE is omitted) -
			CURRENT_USER/CURRENT_ROLE is effective user account and role
		a2) for local database (<connection_string> refers to the current database) -
			CURRENT_USER/CURRENT_ROLE is effective user account and role
		a3) for remote database - operating system account under which engine
			process is currently run is effective user account.
	b) else isc_dpb_user_name (if <user_name> is present and not empty) and isc_dpb_sql_role_name
	   (if <role_name> is present and not empty) will be filled in DPB and native autentication
	   is performed.

- if WITH CALLER PRIVILEGES is specified and ON EXTERNAL DATA SOURCE is omitted, then
	the statement is prepared using additional privileges of caller stored procedure or trigger
	(if EXECUTE STATEMENT is inside SP\trigger). This causes the same effect as if statement 
	is executed by SP\trigger directly.

- Exceptions handling
	a) if ON EXTERNAL DATA SOURCE clause is present then error information is interpreted 
		by the Firebird itself and wrapped into Firebird own error (isc_eds_connection or 
		isc_eds_statement). This is necessary as in general user application can't interprete
		or understand error codes provided by (unknown) external data source. Text of 
		interpreted remote error contains both error codes and corresponding messages.
		
		a1) format of isc_eds_connection error :
			Template string 
				Execute statement error at @1 :\n@2Data source : @3
			Status-vector tags
				isc_eds_connection,
				isc_arg_string, <failed API function name>,
				isc_arg_string, <text of interpreted external error>,
				isc_arg_string, <data source name>
		
		a2) format of isc_eds_statement error :
			Template string 
				Execute statement error at @1 :\n@2Statement : @3\nData source : @4
			Status-vector tags
				isc_eds_statement,
				isc_arg_string, <failed API function name>,
				isc_arg_string, <text of interpreted external error>,
				isc_arg_string, <query>,
				isc_arg_string, <data source name>
				
		At PSQL level these errors could be handled using appropriate GDS code, for example

			WHEN GDSCODE eds_statement
			
		Note, that original error codes are not accessible in WHEN statement. This could be
		improved in the future.
			
	b) if ON EXTERNAL DATA SOURCE clause is omitted then original status-vector with 
		error is passed to the caller PSQL code as is. For example, if dynamic statement
		raised isc_lock_conflict error, this error will be passed to the caller and
		could be handled using following handler
		
			WHEN GDSCODE lock_conflict
			


    Examples :

1. insert speed test

RECREATE TABLE TTT (TRAN INT, CONN INT, ID INT);

a) direct inserts

EXECUTE BLOCK AS
DECLARE N INT = 100000;
BEGIN
  WHILE (N > 0) DO
  BEGIN
    INSERT INTO TTT VALUES (CURRENT_TRANSACTION, CURRENT_CONNECTION, CURRENT_TRANSACTION);
    N = N - 1;
  END
END


b) inserts via prepared dynamic statement using named input parameters

EXECUTE BLOCK AS
DECLARE S VARCHAR(255);
DECLARE N INT = 100000;
BEGIN
  S = 'INSERT INTO TTT VALUES (:a, :b, :a)';

  WHILE (N > 0) DO
  BEGIN
    EXECUTE STATEMENT (:S) (a := CURRENT_TRANSACTION, b := CURRENT_CONNECTION)
    WITH COMMON TRANSACTION;
    N = N - 1;
  END
END


c) inserts via prepared dynamic statement using not named input parameters
EXECUTE BLOCK AS
DECLARE S VARCHAR(255);
DECLARE N INT = 100000;
BEGIN
  S = 'INSERT INTO TTT VALUES (?, ?, ?)';

  WHILE (N > 0) DO
  BEGIN
    EXECUTE STATEMENT (:S) (CURRENT_TRANSACTION, CURRENT_CONNECTION, CURRENT_TRANSACTION);
    N = N - 1;
  END
END



2. connections and transactions test

a) Execute this block few times in the same transaction - it will
   create three new connections to the current database and reuse
   it in every call. Transactions are also reused.

EXECUTE BLOCK RETURNS (CONN INT, TRAN INT, DB VARCHAR(255))
AS
DECLARE I INT = 0;
DECLARE N INT = 3;
DECLARE S VARCHAR(255);
BEGIN
  SELECT A.MON$ATTACHMENT_NAME FROM MON$ATTACHMENTS A
   WHERE A.MON$ATTACHMENT_ID = CURRENT_CONNECTION
    INTO :S;

  WHILE (i < N) DO
  BEGIN
    DB = TRIM(CASE i - 3 * (I / 3) WHEN 0 THEN '\\.\' WHEN 1 THEN 'localhost:' ELSE '' END) || :S;

    FOR EXECUTE STATEMENT 'SELECT CURRENT_CONNECTION, CURRENT_TRANSACTION FROM RDB$DATABASE'
      ON EXTERNAL :DB 
      AS USER CURRENT_USER PASSWORD 'masterkey' -- just for example
      WITH COMMON TRANSACTION
      INTO :CONN, :TRAN
    DO SUSPEND;

    i = i + 1;
  END
END

b) Execute this block few times in the same transaction - it will
   create three new connections to the current database on every call. 

EXECUTE BLOCK RETURNS (CONN INT, TRAN INT, DB VARCHAR(255))
AS
DECLARE I INT = 0;
DECLARE N INT = 3;
DECLARE S VARCHAR(255);
BEGIN
  SELECT A.MON$ATTACHMENT_NAME FROM MON$ATTACHMENTS A
   WHERE A.MON$ATTACHMENT_ID = CURRENT_CONNECTION
    INTO :S;

  WHILE (i < N) DO
  BEGIN
    DB = TRIM(CASE i - 3 * (I / 3) WHEN 0 THEN '\\.\' WHEN 1 THEN 'localhost:' ELSE '' END) || :S;

    FOR EXECUTE STATEMENT 'SELECT CURRENT_CONNECTION, CURRENT_TRANSACTION FROM RDB$DATABASE'
      ON EXTERNAL :DB 
      WITH AUTONOMOUS TRANSACTION -- note autonomous transaction
      INTO :CONN, :TRAN
    DO SUSPEND;

    i = i + 1;
  END
END


3. input expressions evaluated only once

EXECUTE BLOCK RETURNS (A INT, B INT, C INT)
AS
BEGIN
  EXECUTE STATEMENT ('SELECT CAST(:X AS INT), CAST(:X AS INT), CAST(:X AS INT) FROM RDB$DATABASE')
          (x := GEN_ID(G, 1))
    INTO :A, :B, :C;

  SUSPEND;
END
