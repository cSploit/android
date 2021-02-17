----------------------------------------
Select statements and select expressions
----------------------------------------

Semantics:

  A select statement is used to return data to the caller (PSQL module or the
  client program). Select expressions retrieve parts of data that construct both
  the final the result set or any of the intermediate sets. Select expressions
  are also known as subqueries.

Syntax rules:

  <select statement> ::=
    <select expression> [FOR UPDATE] [WITH LOCK]

  <select expression> ::=
    <query specification> [UNION [{ALL | DISTINCT}] <query specification>]

  <query specification> ::=
    SELECT [FIRST <value>] [SKIP <value>] <select list>
    FROM <table expression list>
    WHERE <search condition>
    GROUP BY <group value list>
    HAVING <group condition>
    PLAN <plan item list>
    ORDER BY <sort value list>
    ROWS <value> [TO <value>]

  <table expression> ::=
    <table name> | <joined table> | <derived table>

  <joined table> ::=
    {<cross join> | <qualified join>}

  <cross join> ::=
    <table expression> CROSS JOIN <table expression>

  <qualified join> ::=
    <table expression> [{INNER | {LEFT | RIGHT | FULL} [OUTER]}] JOIN <table expression>
    ON <join condition>

  <derived table> ::=
    '(' <select expression> ')'

Conclusions:

  * FOR UPDATE mode and row locking can only be performed for a final dataset,
    they cannot be applied to a subquery
  * Unions are allowed inside any subquery
  * Clauses FIRST, SKIP, PLAN, ORDER BY, ROWS are allowed for any subquery

Notes:

  * Either FIRST/SKIP or ROWS is allowed, they cannot be mixed together
    (a syntax error is thrown).

  * INSERT statement accepts a select expression to create a dataset to be
    inserted into a table. So its SELECT part supports all the features defined
    above.

  * UPDATE and DELETE statements are always based on an implicit cursor
    iterating through its target table and limited with the WHERE clause. You
    may also specify the final parts of the select expression syntax to limit
    the number of affected rows or optimize the statement. The following clauses
    are allowed at the end of the UPDATE/DELETE statements: PLAN, ORDER BY and
    ROWS.
