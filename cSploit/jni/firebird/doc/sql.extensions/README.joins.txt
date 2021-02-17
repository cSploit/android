--------------
New JOIN types
--------------

Author:
    Adriano dos Santos Fernandes <adrianosf@uol.com.br>

Format:
	<named columns join> ::=
        <table reference> <join type> JOIN <table reference> USING ( <column list> )

    <natural join> ::=
        <table reference> NATURAL <join type> JOIN <table primary>

Notes:
    Named columns join:
        1) All columns specified in <column list> should exist in the tables of both sides.
        2) An equi-join (<left table>.<column> = <right table>.<column>) is automatically
           created for all columns (ANDed).
        3) The USING columns can be accessed without qualifier, and in this case, result of
           it is COALESCE(<left table>.<column>, <right table>.<column>).
        4) In "SELECT *", USING columns are expanded once, using the above rule.

    Natural join:
        1) A "named columns join" is automatically created with all columns common to the
           left and right tables.
        2) If the is no common column, a CROSS JOIN is created.

Examples:
    1) select *
           from employee
           join department
               using (dept_no);

    2) select *
           from employee_project
           natural join employee
           natural join project;
