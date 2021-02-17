SQL Language Extension: Improved aggregate handling

Syntax
========


GROUP BY
--------
SELECT ... FROM .... [GROUP BY group_by_list]

group_by_list : group_by_item [, group_by_list];

group_by_item : column_name
		| ordinal
		| udf
		| group_by_function;

group_by_function	: numeric_value_function
		| string_value_function
		| case_expression
		;

numeric_value_function	: EXTRACT '(' timestamp_part FROM value ')';

string_value_function	:  SUBSTRING '(' value FROM pos_short_integer ')'
			| SUBSTRING '(' value FROM pos_short_integer FOR nonneg_short_integer ')'
			| KW_UPPER '(' value ')'
			;

(FB1.0 only allowed GROUP BY column_name and udf)
Ordinal references to a n-th select-item from the select-list (same as ORDER BY).
The group_by_item cannot reference to any aggregate-function (also not burried 
inside a expression) from the same context.


HAVING
--------
The having clause only allows aggregate functions or expressions 
that are part of the GROUP BY clause. Previously it was allowed
to use columns that were not part of the GROUP BY clause.


ORDER BY
--------
When the context is an aggregate statement then the ORDER BY 
clause only allows valid expressions. That are aggregate 
functions or expression part of the GROUP BY clause.
Previously it was allowed to use non-valid expressions.


Aggregate functions inside sub-selects
--------
It is now possible to use a aggregate function or expression
contained in the GROUP BY clause inside a sub-select.
(See Examples A)


Mixing aggregate functions from different contexts
--------
You can use aggregate functions from different contexts inside 
a expression.
(See Example B)


Sub-selects are supported inside a aggregate function
--------
Using a singleton select expression inside a aggregate function is
supported.
(See Example C)


Nested aggregate functions
--------
Using a aggregate function inside a other aggregate function is
possible if the aggregate function inside is from a lower context.
(See Example C)


Author:
    Arno Brinkman <firebird@abvisie.nl>


N O T E S
=========

 - Not all expressions are currently alowed inside the GROUP BY list.
   (concatenation for example) 
 - ORDER BY clause only accepts valid expressions (this was not for FB1.0)
 - HAVING clause only accepts valid expressions (this was not for FB1.0)
 - Using ordinal 'copies' the expression from the select list as does the
   order by clause. This means when a ordinal references to a sub-select
   the sub-select is at least executed twice.


Examples
========


A)
  SELECT
    r.RDB$RELATION_NAME,
    MAX(r.RDB$FIELD_POSITION),
    (SELECT 
       r2.RDB$FIELD_NAME 
     FROM 
       RDB$RELATION_FIELDS r2
     WHERE 
       r2.RDB$RELATION_NAME = r.RDB$RELATION_NAME and
       r2.RDB$FIELD_POSITION = MAX(r.RDB$FIELD_POSITION))
  FROM
    RDB$RELATION_FIELDS r
  GROUP BY
    1

  SELECT
    rf.RDB$RELATION_NAME AS "Relationname",
    (SELECT
       r.RDB$RELATION_ID
     FROM
       RDB$RELATIONS r
     WHERE
       r.RDB$RELATION_NAME = rf.RDB$RELATION_NAME) AS "ID",
    COUNT(*) AS "Fields"
  FROM
    RDB$RELATION_FIELDS rf
  GROUP BY
    rf.RDB$RELATION_NAME


B)
  SELECT
    r.RDB$RELATION_NAME,
    MAX(i.RDB$STATISTICS) AS "Max1",
    (SELECT
       COUNT(*) || ' - ' || MAX(i.RDB$STATISTICS)
     FROM
       RDB$RELATION_FIELDS rf
     WHERE
       rf.RDB$RELATION_NAME = r.RDB$RELATION_NAME) AS "Max2"
  FROM
    RDB$RELATIONS r
    JOIN RDB$INDICES i on (i.RDB$RELATION_NAME = r.RDB$RELATION_NAME)
  GROUP BY
    r.RDB$RELATION_NAME
  HAVING
    MIN(i.RDB$STATISTICS) <> MAX(i.RDB$STATISTICS)

Note! This query gives results in FB1.0, but they are WRONG!


C)
 SELECT
    r.RDB$RELATION_NAME,
    SUM((SELECT
           COUNT(*)
         FROM
           RDB$RELATION_FIELDS rf
         WHERE
           rf.RDB$RELATION_NAME = r.RDB$RELATION_NAME))
  FROM
    RDB$RELATIONS r
    JOIN RDB$INDICES i on (i.RDB$RELATION_NAME = r.RDB$RELATION_NAME)
  GROUP BY
    r.RDB$RELATION_NAME


