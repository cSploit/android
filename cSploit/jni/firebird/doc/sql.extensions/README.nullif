SQL Language Extension: NULLIF

Function:
    Return a NULL value for a sub-expression if it has a specific value 
    otherwise return the value of the sub-expression

Author:
    Arno Brinkman <firebird@abvisie.nl>

Format:

  <case abbreviation> ::=
      NULLIF <left paren> <value expression> <comma> <value expression> <right paren>

Syntax Rules:
    1) NULLIF (V1, V2) is equivalent to the following <case specification>:
         CASE WHEN V1 = V2 THEN NULL ELSE V1 END

Notes:
    See also README.data_type_results_of_aggregations.txt

Examples:

A)
  UPDATE PRODUCTS
    SET STOCK = NULLIF(STOCK,0)

