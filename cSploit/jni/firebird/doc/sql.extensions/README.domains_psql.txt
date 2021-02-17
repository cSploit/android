---------------
Domains in PSQL
---------------

Function:
    Allow usage of domains in PSQL.

Author:
    Adriano dos Santos Fernandes <adrianosf@uol.com.br>

Syntax rules:
    data_type ::=
         <builtin_data_type>
       | <domain_name>
       | TYPE OF <domain_name>
       | TYPE OF COLUMN <table or view>.<column>

Examples:
    CREATE DOMAIN DOM AS INTEGER;

    CREATE PROCEDURE SP (I1 TYPE OF DOM, I2 DOM) RETURNS (O1 TYPE OF DOM, O2 DOM)
    AS
        DECLARE VARIABLE V1 TYPE OF DOM;
        DECLARE VARIABLE V2 DOM;
    BEGIN
    END

Notes:
    1. TYPE OF gets only the type of the domain. It doesn't use constraints and default values.
    2. A new field RDB$VALID_BLR was added in RDB$RELATIONS and RDB$TRIGGERS to store if the procedure/trigger is valid or not after an ALTER DOMAIN.
    3. The value of RDB$VALID_BLR is shown in ISQL commands SHOW PROCEDURE/TRIGGER.

See also:
    README.column_type_psql.txt
