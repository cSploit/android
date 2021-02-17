SQL Language Extension: ORDER BY clause can specify expressions and nulls placement

Syntax
========

SELECT ... FROM .... ORDER BY order_list ....;

order_list : order_item [, order_list];

order_item : <expression> [order_direction] [nulls_placement]

order_direction : ASC | DESC;

nulls_placement : NULLS FIRST | NULLS LAST;

   The ORDER BY clause lets you specify any valid expressions to 
   sort query results. If expression is consisted of a single number
   it is interpreted as column number. The nulls_placement clause
   controls ordering of nulls in result set. They can be sorted
   either above (NULLS FIRST) or below (NULLS LAST) of all other values.
   Behaviour when nulls_placement is unspecified is NULLS FIRST.   

Author:
   Nickolay Samofatov <skidder@bssys.com>


N O T E S
=========

 - If you override the default nulls placement, no index can be used
   for sorting. That is, no index will be used for an ASCENDING sort
   if NULLS LAST is specified, nor for a DESCENDING sort
   if NULLS FIRST is specified.
 - Results are undefined if you ask engine to sort results using 
   non-deterministic UDF or stored procedure.
 - Amount of procedure invocations is undefined if you ask engine to 
   sort results using UDF or stored procedure in any case even
   if you reference column calling procedure by number
 - You can use only numbers to reference columns to sort unions

Examples
========


A)
  SELECT * FROM MSG
  ORDER BY PROCESS_TIME DESC NULLS FIRST

B)
  SELECT FIRST 10 * FROM DOCUMENT
  ORDER BY STRLEN(DESCRIPTION) DESC

C)
  SELECT DOC_NUMBER, DOC_DATE FROM PAYORDER
  UNION ALL
  SELECT DOC_NUMBER, DOC_DATA FROM BUDGORDER
  ORDER BY 2 DESC NULLS LAST, 1 ASC NULLS FIRST

