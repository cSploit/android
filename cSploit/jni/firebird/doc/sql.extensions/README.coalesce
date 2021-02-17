SQL Language Extension: COALESCE

Function:
    Allow a column value to be calculated by a number of expressions, 
    the first expression returning a non NULL value is returned as the 
    column value

Author:
    Arno Brinkman <firebird@abvisie.nl>

Format:

  <case abbreviation> ::=
    | COALESCE <left paren> <value expression> { <comma> <value expression> }... <right paren>


Syntax Rules:
    1) COALESCE (V1, V2) is equivalent to the following <case specification>:
         CASE WHEN V1 IS NOT NULL THEN V1 ELSE V2 END
    2) COALESCE (V1, V2,..., Vn), for n >= 3, is equivalent to the following
         <case specification>:
         CASE WHEN V1 IS NOT NULL THEN V1 ELSE COALESCE (V2,...,Vn) END

Notes:
    See also README.data_type_results_of_aggregations.txt

Examples:
A)

  SELECT
    PROJ_NAME AS Projectname,
    COALESCE(e.FULL_NAME,'[> not assigned <]') AS Employeename
  FROM
    PROJECT p
    LEFT JOIN EMPLOYEE e ON (e.EMP_NO = p.TEAM_LEADER)

B)
  SELECT
    COALESCE(Phone,MobilePhone,'Unknown') AS "Phonenumber"
  FROM
    Relations

