-----------
Subroutines
-----------

Author:
    Adriano dos Santos Fernandes <adrianosf at gmail.com>

Description:
    Support for PSQL subroutines (functions and procedures) inside functions, procedures, triggers
    and EXECUTE BLOCK. Subroutines are declared in the main routine and may be used from there.

Syntax:
    <declaration item> ::=
        DECLARE [VARIABLE] <variable name> <data type> [ := <value> ];
        |
        DECLARE [VARIABLE] CURSOR <cursor name> FOR (<query>);
        |
        DECLARE FUNCTION <function name> RETURNS <data type>
        AS
            ...
        BEGIN
            ...
        END
        |
        DECLARE PROCEDURE <procedure name> [ (<input parameters>) ] [ RETURNS (<output parameters>) ]
        AS
            ...
        BEGIN
            ...
        END

Limitations:
    1) Subroutines may not be nested in another subroutine. They are only supported in the main
       routine.
    2) Currently, a subroutine may not directly access or use variables, cursors or another
       subroutines of the main statements. This may be allowed in the future.

Examples:
    set term !;

    -- 1) Sub-procedures in execute block.

    execute block returns (name varchar(31))
    as
        declare procedure get_tables returns (table_name varchar(31))
        as
        begin
            for select rdb$relation_name
                  from rdb$relations
                  where rdb$view_blr is null
                  into table_name
            do
                suspend;
        end

        declare procedure get_views returns (view_name varchar(31))
        as
        begin
            for select rdb$relation_name
                  from rdb$relations
                  where rdb$view_blr is not null
                  into view_name
            do
                suspend;
        end
    begin
        for select table_name
              from get_tables
            union all
            select view_name
              from get_views
              into name
        do
            suspend;
    end!


    -- 2) Sub-function in a stored function.

    create or alter function func1 (n1 integer, n2 integer) returns integer
    as
        declare function subfunc (n1 integer, n2 integer) returns integer
        as
        begin
            return n1 + n2;
        end
    begin
        return subfunc(n1, n2);
    end!

    select func1(5, 6) from rdb$database!
