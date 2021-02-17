--------
Packages
--------

Author:
    Adriano dos Santos Fernandes <adrianosf@uol.com.br>
    (This feature was sponsored with donations gathered in the "5th Brazilian Firebird Developers Day")

Description:
    A package is a group of procedures and functions managed as one entity.

Syntax:
    <package_header> ::=
        { CREATE [OR ALTER] | ALTER | RECREATE } PACKAGE <name>
        AS
        BEGIN
            [ <package_item> ... ]
        END

    <package_item> ::=
        <function_decl> ; |
        <procedure_decl> ;

    <function_decl> ::=
        FUNCTION <name> [( <parameters> )] RETURNS <type>

    <procedure_decl> ::=
        PROCEDURE <name> [( <parameters> ) [RETURNS ( <parameters> )]]

    <package_body> ::=
        { CREATE | RECREATE } PACKAGE BODY <name>
        AS
        BEGIN
            [ <package_item> ... ]
            [ <package_body_item> ... ]
        END

    <package_body_item> ::=
        <function_impl> |
        <procedure_impl>

    <function_impl> ::=
        FUNCTION <name> [( <parameters> )] RETURNS <type>
        AS
        BEGIN
           ...
        END
        |
        FUNCTION <name>  [( <parameters> )] RETURNS <type>
            EXTERNAL NAME '<name>' ENGINE <engine>

    <procedure_impl> ::=
        PROCEDURE <name> [( <parameters> ) [RETURNS ( <parameters> )]]
        AS
        BEGIN
           ...
        END
        |
        PROCEDURE <name> [( <parameters> ) [RETURNS ( <parameters> )]]
            EXTERNAL NAME '<name>' ENGINE <engine>

    <drop_package_header> ::=
        DROP PACKAGE <name>

    <drop_package_body> ::=
        DROP PACKAGE BODY <name>

Objectives:
    - Make functional dependent code separated in logical modules like programming languages do.

      It's well known in programming world that having code grouped in some way (for example in
      namespaces, units or classes) is a good thing. With standard procedures and functions in the
      database this is not possible. It's possible to group them in different scripts files, but
      two problems remain:
      1) The grouping is not represented in the database metadata.
      2) They all participate in a flat namespace and all routines are callable by everyone (not
         talking about security permissions here).
    
    - Facilitate dependency tracking between its internal routines and between other packaged and
      unpackaged routines.

      Firebird packages are divided in two pieces: a header (aka PACKAGE) and a body (aka
      PACKAGE BODY). This division is very similar to a Delphi unit. The header corresponds to the
      interface part, and the body corresponds to the implementation part.

      The user needs first to create the header (CREATE PACKAGE) and after it the body (CREATE
      PACKAGE BODY).

      When a packaged routine uses a determined database object, it's registered on Firebird system
      tables that the package body depends on that object. If you want to, for example, drop that
      object, you first need to remove who depends on it. As who depends on it is a package body,
      you can just drop it even if some other database object depends on this package. When the body
      is dropped, the header remains, allowing you to create its body again after changing it based 
      on the object removal.

    - Facilitate permission management.

      It's generally a good practice to create routines with a privileged database user and grant
      usage to them for users or roles. As Firebird runs the routines with the caller privileges,
      it's also necessary to grant resources usage to each routine, when these resources would not
      be directly accessible to the callers, and grant usage of each routine to users and/or roles.

      Packaged routines do not have individual privileges. The privileges act on the package.
      Privileges granted to packages are valid for all (including private) package body routines,
      but are stored for the package header. Example usage:
        GRANT SELECT ON TABLE secret TO PACKAGE pk_secret;
        GRANT EXECUTE ON PACKAGE pk_secret TO ROLE role_secret;

    - Introduce private scope to routines making them available only for internal usage in the
      defining package.

      All programming languages have the notion of routine scope. But without some form of grouping,
      this is not possible. Firebird packages also work as Delphi units in this regard. If a
      routine is not declared on the package header (interface) and is implemented in the body
      (implementation), it becomes a private routine. A private routine can only be called from
      inside its package.

Syntax rules:
    - A package body should implement all routines declared in the header and in the body start,
      with the same signature.
    - Default value for procedure parameters could not be redefined (be informed in <package_item>
      and <package_body_item>). That means, they can be in <package_body_item> only for private
      procedures not declared.

Notes:
    - DROP PACKAGE drops the package body before dropping its header.
    - UDFs (DECLARE EXTERNAL FUNCTION) are currently not supported inside packages.

Examples:
    - See examples/package.
