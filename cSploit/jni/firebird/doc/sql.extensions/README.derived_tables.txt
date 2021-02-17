SQL Language Extension: derived tables

Function:
    A derived table is a table derived from a select statement.
    Derived tables can also be nested to build complex queries.
    They can be joined the same as normal tables and views.

Author:
    Arno Brinkman <firebird@abvisie.nl>

Format:
    SELECT
      <select list>
    FROM
      <table reference list>

    <table reference list> ::= <table reference> [{<comma> <table reference>}...]

    <table reference> ::= 
        <table primary>
      | <joined table>

    <table primary> ::=
        <table> [[AS] <correlation name>]
      | <derived table>

    <derived table> ::=
        <query expression> [[AS] <correlation name>] 
          [<left paren> <derived column list> <right paren>]

    <derived column list> ::= <column name> [{<comma> <column name>}...]

Notes:
    Every column in the derived table must have a name. Unnamed expressions like 
    constants should be added with an alias or the column list should be used.
    The number of columns in the column list should be the same as the number of 
    columns from the query expression.
    The optimizer can handle a derived table very efficiently, but if the 
    derived table contains a sub-select then no join order can be made (if the 
    derived table is included in an inner join).

Examples:

a) Simple derived table:

  SELECT
    *
  FROM
    (SELECT
       RDB$RELATION_NAME, RDB$RELATION_ID
     FROM
       RDB$RELATIONS) AS R (RELATION_NAME, RELATION_ID)


b) Aggregate on a derived table which also contains an aggregate

  SELECT
    DT.FIELDS,
    Count(*)
  FROM
    (SELECT
       R.RDB$RELATION_NAME,
       Count(*)
     FROM
       RDB$RELATIONS R
       JOIN RDB$RELATION_FIELDS RF ON (RF.RDB$RELATION_NAME = R.RDB$RELATION_NAME)
     GROUP BY
       R.RDB$RELATION_NAME) AS DT (RELATION_NAME, FIELDS)
  GROUP BY
    DT.FIELDS


c) UNION and ORDER BY example:

  SELECT
    DT.*
  FROM
    (SELECT
       R.RDB$RELATION_NAME,
       R.RDB$RELATION_ID
     FROM
       RDB$RELATIONS R
     UNION ALL
     SELECT
       R.RDB$OWNER_NAME,
       R.RDB$RELATION_ID
     FROM
       RDB$RELATIONS R
     ORDER BY
       2) AS DT
  WHERE
    DT.RDB$RELATION_ID <= 4

