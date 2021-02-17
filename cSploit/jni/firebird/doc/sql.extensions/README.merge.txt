-----------------
MERGE statement
-----------------

  Function:
    Read data from the source and INSERT or UPDATE in the target table depending on a
    condition.

  Author:
    Adriano dos Santos Fernandes <adrianosf@uol.com.br>

  Format:
	<merge statement> ::=
		MERGE
			INTO <table or view> [ [AS] <correlation name> ]
			USING <table or view or derived table> [ [AS] <correlation name> ]
			ON <condition>
			<merge when>...
			<returning clause>

	<merge when> ::=
		<merge when matched> |
		<merge when not matched>

	<merge when matched> ::=
		WHEN MATCHED [ AND <condition> ] THEN
			{ UPDATE SET <assignment list> | DELETE }

	<merge when not matched> ::=
		WHEN NOT MATCHED [ AND <condition> ] THEN
			INSERT [ <left paren> <column list> <right paren> ]
				VALUES <left paren> <value list> <right paren>

  Syntax rules:
	1. At least one of <merge when matched> or <merge when not matched> should be specified.

  Scope:
    DSQL, PSQL

  Examples:
	1.
		MERGE
			INTO customers c
			USING (SELECT * FROM customers_delta WHERE id > 10) cd
			ON (c.id = cd.id)
			WHEN MATCHED THEN
				UPDATE SET
					name = cd.name
			WHEN NOT MATCHED THEN
				INSERT (id, name)
					VALUES (cd.id, cd.name)

  Notes:
	A right join is made between INTO and USING tables using the condition.
	UPDATE is called when a record exist in the left table (INTO), otherwise
	INSERT is called.

	As soon it's decided if the source matched or not a record in the target, the set of the
	corresponding (WHEN MATCHED / WHEN NOT MATCHED) statements is evaluated in the order specified,
	to check their optional conditions. The first statement which has its condition evaluated to true
	is the one which will be executed, and the subsequent ones will be ignored.

	If no record is returned in the join, INSERT is not called.
