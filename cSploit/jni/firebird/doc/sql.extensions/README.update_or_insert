-----------------
UPDATE OR INSERT statement
-----------------

  Function:
    Allow to update or insert a record based on the existence (checked with IS NOT DISTINCT) or not of it.

  Author:
    Adriano dos Santos Fernandes <adrianosf@uol.com.br>

  Syntax rules:
    UPDATE OR INSERT INTO <table or view> [(<column_list>)]
        VALUES (<value_list>)
        [MATCHING (<column_list>)]
        [RETURNING <value_list> [INTO <variable_list>]]

  Scope:
    DSQL, PSQL

  Examples:
    1. UPDATE OR INSERT INTO T1 (F1, F2) VALUES (:F1, :F2);
    2. UPDATE OR INSERT INTO EMPLOYEE (ID, NAME) VALUES (:ID, :NAME) RETURNING ID;
    3. UPDATE OR INSERT INTO T1 (F1, F2) VALUES (:F1, :F2) MATCHING (F1);
    4. UPDATE OR INSERT INTO EMPLOYEE (ID, NAME) VALUES (:ID, :NAME) RETURNING OLD.NAME;

  Notes:
    1. When MATCHING is omitted, the existence of a primary key is required.
    2. INSERT and UPDATE permissions are needed on <table or view>.
    3. If the RETURNING clause is present, then the statement is described as
       isc_info_sql_stmt_exec_procedure by the API. Otherwise it is described
       as isc_info_sql_stmt_insert.

  Limitation:
    1. A singleton error will be raised if the RETURNING clause is present and more than
       one record match the condition.
    2. There is no "UPDATE OR INSERT ... SELECT ..." as "INSERT ... SELECT". Use MERGE for
       this type of functionality.
