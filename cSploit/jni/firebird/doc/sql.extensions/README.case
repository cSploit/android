SQL Language Extension: CASE

Function:
    Allow the result of a column to be determined by a the results of a 
    case expression.

Author:
    Arno Brinkman <firebird@abvisie.nl>

Format:
  <case expression> ::=
      <case abbreviation>
    | <case specification>

  <case abbreviation> ::=
      NULLIF <left paren> <value expression> <comma> <value expression> <right paren>
    | COALESCE <left paren> <value expression> { <comma> <value expression> }... <right paren>

  <case specification> ::=
      <simple case>
    | <searched case>

  <simple case> ::=
    CASE <value expression>
      <simple when clause>...
      [ <else clause> ]
    END

  <searched case> ::=
    CASE
      <searched when clause>...
      [ <else clause> ]
    END

  <simple when clause> ::= WHEN <when operand> THEN <result>

  <searched when clause> ::= WHEN <search condition> THEN <result>

  <when operand> ::= <value expression>

  <else clause> ::= ELSE <result>

  <result> ::=
      <result expression>
    | NULL

  <result expression> ::= <value expression>

Notes:
    See also README.data_type_results_of_aggregations.txt

Examples:

A) (simple)
  SELECT
    o.ID,
    o.Description,
    CASE o.Status
      WHEN 1 THEN 'confirmed'
      WHEN 2 THEN 'in production'
      WHEN 3 THEN 'ready'
      WHEN 4 THEN 'shipped'
      ELSE 'unknown status ''' || o.Status || ''''
    END
  FROM
    Orders o

B) (searched)
  SELECT
    o.ID,
    o.Description,
    CASE
      WHEN (o.Status IS NULL) THEN 'new'
      WHEN (o.Status = 1) THEN 'confirmed'
      WHEN (o.Status = 3) THEN 'in production'
      WHEN (o.Status = 4) THEN 'ready'
      WHEN (o.Status = 5) THEN 'shipped'
      ELSE 'unknown status ''' || o.Status || ''''
    END
  FROM
    Orders o

