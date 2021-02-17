-------------------
Column type in PSQL
-------------------

Function:
    Allow usage of table or view column type in PSQL.

Author:
    Adriano dos Santos Fernandes <adrianosf@uol.com.br>

Syntax rules:
    data_type ::=
         <builtin_data_type>
       | <domain_name>
       | TYPE OF <domain_name>
       | TYPE OF COLUMN <table or view>.<column>

Examples:
    CREATE TABLE PERSON (ID INTEGER, NAME VARCHAR(40));

    CREATE PROCEDURE SP_INS_PERSON
        (ID TYPE OF COLUMN PERSON.ID, NAME TYPE OF COLUMN PERSON.NAME)
    AS
        DECLARE VARIABLE NEW_ID TYPE OF COLUMN PERSON.ID;
    BEGIN
        INSERT INTO PERSON (ID, NAME)
            VALUES (:ID, :NAME)
            RETURNING ID INTO :NEW_ID;
    END

Notes:
    1. TYPE OF COLUMN gets only the type of the column. It doesn't use constraints and default values.

See also:
    README.domains_psql.txt
