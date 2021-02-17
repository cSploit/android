SQL Language Extension: EXECUTE BLOCK

Function:
	Allow execute PL-SQL block as if it is stored procedure. 
	Supports input and output parameters

Autor:
	Vlad Khorsun <hvlad at users.sourceforge.net>

Syntax:
	EXECUTE BLOCK [ (param datatype = ?, param datatype = ?, ...) ]
		[ RETURNS (param datatype, param datatype, ...) }
	AS
	[DECLARE VARIABLE var datatype; ...]
	BEGIN
	...
	END

Client-side:
	The call isc_dsql_sql_info with parameter isc_info_sql_stmt_type returns
	- isc_info_sql_stmt_select, if block has output parameters. 
	Semantics of a call is similar to SELECT query - client has open cursor, 
can fetch data from it, and must close it after use.

	- isc_info_sql_stmt_exec_procedure, if block has no output parameters.
	Semantics of a call is similar to EXECUTE query - client has no cursor,
execution runs until first SUSPEND or end of block

	The client should preprocess only head of the SQL statement or use '?' 
instead of ':' as parameter indicator because in a body of the block may be links 
to local variables and \ or parameters with a colon ahead.

Example:
	User SQL is
		EXECUTE BLOCK (X INTEGER = :X) RETURNS (Y VARCHAR)
		AS
		DECLARE V INTEGER;
		BEGIN
		  INSERT INTO T(...) VALUES (... :X ...);

		  SELECT ... FROM T INTO :Y;
		  SUSPEND;
		END
	
	Preprocessed SQL is
		EXECUTE BLOCK (X INTEGER = ?) RETURNS (Y VARCHAR)
		AS
		DECLARE V INTEGER;
		BEGIN
		  INSERT INTO T(...) VALUES (... :X ...);

		  SELECT ... FROM T INTO :Y;
		  SUSPEND;
		END

