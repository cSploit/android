-------------------
Sequence generators
-------------------

  Function:
    A sequence generator is a mechanism for generating successive exact
    numeric values, one at a time. A sequence generator is a named schema
    object.

  Syntax rules:
    CREATE { SEQUENCE | GENERATOR } <name>
    DROP { SEQUENCE | GENERATOR } <name>
    SET GENERATOR <name> TO <start_value>
    ALTER SEQUENCE <name> RESTART WITH <start_value>
    GEN_ID (<name>, <increment_value>)
    NEXT VALUE FOR <name>

  Type:
    INTEGER in dialect 1, BIGINT in dialect 3

  Example(s):
    1. CREATE SEQUENCE S_EMPLOYEE;
    2. ALTER SEQUENCE S_EMPLOYEE RESTART WITH 0;
    3. SELECT GEN_ID(S_EMPLOYEE, 1) FROM RDB$DATABASE;
    4. INSERT INTO EMPLOYEE (ID, NAME) VALUES (NEXT VALUE FOR S_EMPLOYEE, 'John Smith');

  Note(s):
    1. SEQUENCE is a syntax term declared in the SQL specification, while
       GENERATOR is a legacy InterBase syntax term. It's recommended to use
       the standard SEQUENCE syntax in your applications.
    2. Currently, increment values not equal to 1 (one) could be used only via
       the GEN_ID function. The future versions are expected to provide full
       support for SQL-99 sequence generators (which allows the required
       increment values to be specified at the DDL level), so it's recommended
       to get new sequential values via NEXT VALUE FOR value expression instead
       of the GEN_ID function, unless it's vitally important to use other
       increment values.
    3. GEN_ID(<name>, 0) allows you to retrieve the current sequence value,
       but it should be never used in insert/update statements, as it produces a
       high risk of uniqueness violations in a concurrent environment.
