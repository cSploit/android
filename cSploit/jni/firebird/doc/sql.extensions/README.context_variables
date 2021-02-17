------------------------
System context variables
------------------------


CURRENT_CONNECTION / CURRENT_TRANSACTION (FB 1.5)
-------------------------------------------------

  Function:
    Returns system identifier of the active connection/transaction,
    i.e. a connection/transaction, in which context the given SQL
    statement is executed.

  Author:
    Dmitry Yemanov <yemanov@yandex.ru>

  Syntax rules:
    CURRENT_CONNECTION / CURRENT_TRANSACTION

  Type:
    INTEGER

  Scope:
    DSQL, PSQL

  Example(s):
    1. SELECT CURRENT_CONNECTION FROM RDB$DATABASE;
    2. NEW.CONN_ID = CURRENT_TRANSACTION;
    3. EXECUTE PROCEDURE P_LOGIN(CURRENT_CONNECTION);

  Note(s):
    These values are stored on the database header page,
    so they will be reset after a database restore.


ROW_COUNT (FB 1.5)
------------------

  Function:
    Returns number of rows, affected by the last SQL statement.

  Author:
    Dmitry Yemanov <yemanov@yandex.ru>

  Syntax rules:
    ROW_COUNT

  Type:
    INTEGER

  Scope:
    PSQL, context of the given procedure/trigger.

  Example(s):
    UPDATE TABLE1 SET FIELD1 = 0 WHERE ID = :ID;
    IF (ROW_COUNT = 0) THEN
      INSERT INTO TABLE1 (ID, FIELD1) VALUES (:ID, 0);

  Note(s):
    1. It can be used to check whether a SELECT statement returned
       any rows or not. Also it can be used to exit a fetch loop
       on an explicit PSQL cursor.
    2. ROW_COUNT contains zero after EXECUTE STATEMENT call. That's
       a design limitation, because dynamic SQL statements are
       executed as nested requests (i.e. in another context).

  See also:
    README.cursors


SQLCODE / GDSCODE (FB 1.5)
--------------------------

  Function:
    Returns numeric error code for the active exception.

  Author:
    Dmitry Yemanov <yemanov@yandex.ru>

  Syntax rules:
    SQLCODE / GDSCODE

  Type:
    INTEGER

  Scope:
    PSQL, context of the exception handling block.

  Example(s):
    BEGIN
      ...
    WHEN SQLCODE -802 DO
      EXCEPTION E_EXCEPTION_1;
    WHEN SQLCODE -803 DO
      EXCEPTION E_EXCEPTION_2;
    WHEN ANY DO
      EXECUTE PROCEDURE P_ANY_EXCEPTION(SQLCODE);
    END

  Note(s):
    1. GDSCODE variable returns a numeric representation of the
       appropriate Firebird error code.
    2. Both SQLCODE and GDSCODE always evaluate to zero outside
       the exception handling block.
    3. If you catch exceptions with 'WHEN SQLCODE' block, then only
       SQLCODE variable contains the error code inside this block,
       whilst GDSCODE contains zero. Obviously, this situation is
       opposite for 'WHEN GDSCODE' block.
    4. For 'WHEN ANY' block, the error code is set in SQLCODE
       variable only.
    5. If user-defined exception is thrown, both SQLCODE and GDSCODE
       variables contain zero, regardless of the exception handling
       block type.

  See also:
    README.exception_handling


INSERTING / UPDATING / DELETING (FB 1.5)
----------------------------------------

  Function:
    Determines type of row operation being executed.

  Author:
    Dmitry Yemanov <yemanov@yandex.ru>

  Syntax rules:
    INSERTING / UPDATING / DELETING

  Type:
    BOOLEAN (emulated via pseudo-expression in FB 1.5)

  Scope:
    PSQL, triggers only.

  Example(s):
    IF (INSERTING OR DELETING) THEN
      NEW.ID = GEN_ID(G_GENERATOR_1, 1);

  See also:
    README.universal_triggers
