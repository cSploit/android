SQL Language Extension: common table expressions

Author:
    Vlad Khorsun <hvlad at users.sourceforge.net>
	based on work by Paul Ruizendaal for Fyracle project


Function:
	Common Table Expressions is like views, locally defined within main query.
From the engine point of view CTE is a derived table so no intermediate 
materialization is performed. 

	Recursive CTEs allow to create recursive queries. It works as below :
engine starts execution from non-recursive members,
for each evaluated row engine starts execution of each recursive member using 
current row values as parameters,
if current instance of recursive member produced no rows engine returns one 
step back and get next row from previous resultset 

	Memory and CPU overhead of recursive CTE is much less than overhead of 
recursive stored procedures. Currently recursion depth is limited by hardcoded 
value of 1024

	
Syntax:

select		: select_expr for_update_clause lock_clause

select_expr	: with_clause select_expr_body order_clause rows_clause
			| select_expr_body order_clause rows_clause
 
with_clause	: WITH RECURSIVE with_list
			| WITH with_list
 
with_list	: with_item 
			| with_item ',' with_list

with_item	: symbol_table_alias_name derived_column_list AS '(' select_expr ')'

select_expr_body	: query_term
		| select_expr_body UNION distinct_noise query_term
		| select_expr_body UNION ALL query_term


	Or, in less formal format :

WITH [RECURSIVE]
	CTE_A [(a1, a2, ...)] 
	AS ( SELECT ...  ),

	CTE_B [(b1, b2, ...)]
	AS ( SELECT ... ),
...
SELECT ...
  FROM CTE_A, CTE_B, TAB1, TAB2 ...
 WHERE ...
 


Rules of non-recursive common table expressions :
 
	Several table expressions can be defined at one query
	Any SELECTs clause can be used in table expressions
	Table expressions can reference each other
	Table expressions can be used within any part of main query or another 
table expression
	The same table expression can be used several times in main query
	Table expressions can be used in INSERT, UPDATE and DELETE statements 
(as subqueries of course)
	Table expressions can be used in procedure language also 
	WITH statements can not be nested 
	References between expressions should not have loops


Example of non-recursive common table expressions :

WITH
  DEPT_YEAR_BUDGET AS (
    SELECT FISCAL_YEAR, DEPT_NO, SUM(PROJECTED_BUDGET) AS BUDGET
      FROM PROJ_DEPT_BUDGET
    GROUP BY FISCAL_YEAR, DEPT_NO
  )
SELECT D.DEPT_NO, D.DEPARTMENT,
       B_1993.BUDGET AS B_1993, B_1994.BUDGET AS B_1994,
       B_1995.BUDGET AS B_1995, B_1996.BUDGET AS B_1996
  FROM DEPARTMENT D
       LEFT JOIN DEPT_YEAR_BUDGET B_1993
    ON D.DEPT_NO = B_1993.DEPT_NO AND B_1993.FISCAL_YEAR = 1993
       LEFT JOIN DEPT_YEAR_BUDGET B_1994
    ON D.DEPT_NO = B_1994.DEPT_NO AND B_1994.FISCAL_YEAR = 1994
       LEFT JOIN DEPT_YEAR_BUDGET B_1995
    ON D.DEPT_NO = B_1995.DEPT_NO AND B_1995.FISCAL_YEAR = 1995
       LEFT JOIN DEPT_YEAR_BUDGET B_1996
    ON D.DEPT_NO = B_1996.DEPT_NO AND B_1996.FISCAL_YEAR = 1996

WHERE EXISTS (SELECT * FROM PROJ_DEPT_BUDGET B WHERE D.DEPT_NO = B.DEPT_NO)


Rules of recursive common table expressions :

	Recursive CTE has reference to itself
	Recursive CTE is an UNION of recursive and non-recursive members	
	At least one non-recursive member (anchor) must be present
	Non-recursive members are placed first in UNION
	Recursive members are separated from anchor members and from each 
other with UNION ALL clause
	References between CTEs should not have loops
	Aggregates (DISTINCT, GROUP BY, HAVING) and aggregate functions (SUM, 
COUNT, MAX etc) are not allowed in recursive members
	Recursive member can have only one reference to itself and only in FROM clause
	Recursive reference can not participate in outer joins


Example of recursive common table expressions :

WITH RECURSIVE
  DEPT_YEAR_BUDGET AS
  (
    SELECT FISCAL_YEAR, DEPT_NO, SUM(PROJECTED_BUDGET) AS BUDGET
      FROM PROJ_DEPT_BUDGET
    GROUP BY FISCAL_YEAR, DEPT_NO
  ),

  DEPT_TREE AS 
  (
    SELECT DEPT_NO, HEAD_DEPT, DEPARTMENT, CAST('' AS VARCHAR(255)) AS INDENT
      FROM DEPARTMENT
     WHERE HEAD_DEPT IS NULL

    UNION ALL

    SELECT D.DEPT_NO, D.HEAD_DEPT, D.DEPARTMENT, H.INDENT || '  '
      FROM DEPARTMENT D JOIN DEPT_TREE H
        ON D.HEAD_DEPT = H.DEPT_NO
  )

SELECT D.DEPT_NO, 
	D.INDENT || D.DEPARTMENT AS DEPARTMENT,
	B_1993.BUDGET AS B_1993, 
	B_1994.BUDGET AS B_1994,
	B_1995.BUDGET AS B_1995, 
	B_1996.BUDGET AS B_1996

  FROM DEPT_TREE D
       LEFT JOIN DEPT_YEAR_BUDGET B_1993
    ON D.DEPT_NO = B_1993.DEPT_NO AND B_1993.FISCAL_YEAR = 1993

       LEFT JOIN DEPT_YEAR_BUDGET B_1994
    ON D.DEPT_NO = B_1994.DEPT_NO AND B_1994.FISCAL_YEAR = 1994

       LEFT JOIN DEPT_YEAR_BUDGET B_1995
    ON D.DEPT_NO = B_1995.DEPT_NO AND B_1995.FISCAL_YEAR = 1995

       LEFT JOIN DEPT_YEAR_BUDGET B_1996
    ON D.DEPT_NO = B_1996.DEPT_NO AND B_1996.FISCAL_YEAR = 1996
