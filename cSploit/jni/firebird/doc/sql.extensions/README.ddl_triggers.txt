------------
DDL triggers
------------

Author:
	Adriano dos Santos Fernandes <adrianosf@uol.com.br>
	(This feature was sponsored with donations gathered in the "5th Brazilian Firebird Developers Day")

Syntax:
	<database-trigger> ::=
		{CREATE | RECREATE | CREATE OR ALTER}
			TRIGGER <name>
			[ACTIVE | INACTIVE]
			{BEFORE | AFTER} <ddl event>
			[POSITION <n>]
		AS
		BEGIN
			...
		END

	<ddl event> ::=
		ANY DDL STATEMENT
	  | <ddl event item> [{OR <ddl event item>}...]

	<ddl event item> ::=
		CREATE TABLE
	  | ALTER TABLE
	  | DROP TABLE
	  | CREATE PROCEDURE
	  | ALTER PROCEDURE
	  | DROP PROCEDURE
	  | CREATE FUNCTION
	  | ALTER FUNCTION
	  | DROP FUNCTION
	  | CREATE TRIGGER
	  | ALTER TRIGGER
	  | DROP TRIGGER
	  | CREATE EXCEPTION
	  | ALTER EXCEPTION
	  | DROP EXCEPTION
	  | CREATE VIEW
	  | ALTER VIEW
	  | DROP VIEW
	  | CREATE DOMAIN
	  | ALTER DOMAIN
	  | DROP DOMAIN
	  | CREATE ROLE
	  | ALTER ROLE
	  | DROP ROLE
	  | CREATE SEQUENCE
	  | ALTER SEQUENCE
	  | DROP SEQUENCE
	  | CREATE USER
	  | ALTER USER
	  | DROP USER
	  | CREATE INDEX
	  | ALTER INDEX
	  | DROP INDEX
	  | CREATE COLLATION
	  | DROP COLLATION
	  | ALTER CHARACTER SET
	  | CREATE PACKAGE
	  | ALTER PACKAGE
	  | DROP PACKAGE
	  | CREATE PACKAGE BODY
	  | DROP PACKAGE BODY


Syntax rules:
	1) DDL triggers type can't be changed.

Semantics:
	1) BEFORE triggers are fired before changes to system tables and AFTER triggers are fired after
	   system table changes.
	2) It's possible to cancel the command raising an exception in BEFORE or in AFTER triggers.
	3) Firebird really does DDL actions only when committing the transaction that run DDL commands.
	   You should pay attention to that. What you can do in AFTER triggers are exactly what you can
	   do after a DDL command without autocommit. For example, you can't create a table and use it
	   on the trigger.
	4) With CREATE OR ALTER statements, trigger is fired one time using the CREATE or ALTER event
	   based on the previous existence of the object. With RECREATE statements, trigger is fired for
	   DROP (when the object exists) and for CREATE events.
	5) ALTER and DROP events are generally not fired when the object name does not exist.
	6) As exception to rule 5, BEFORE ALTER/DROP USER triggers fires even when the user name does
	   not exist. This is because these commands run on the security database and the verification
	   is not done before run the command on it. This is likely to be different with embedded users,
	   so do not write your code depending on this.
	7) If some exception is raised after the DDL command starts its execution and before AFTER
	   triggers are fired, AFTER triggers will not be fired.
	8) Packaged procedures and triggers do not fire individual {CREATE | ALTER | DROP} {PROCEDURE |
	   FUNCTION} triggers.

Notes:
	1) COMMENT ON, GRANT, REVOKE and ALTER DATABASE do not fire DDL triggers.

Utilities support:
	DDL triggers is a type of database triggers, so the parameters -nodbtriggers (GBAK and ISQL)
	and -T (NBACKUP) also works for them.
	These parameters could only be used by database owner and SYSDBA.

Permissions:
	Only database owner and SYSDBA can create/alter/drop DDL triggers.

DDL_TRIGGER context namespace:
	It has been introduced the DDL_TRIGGER context for usage with RDB$GET_CONTEXT. Usage of this
	namespace is valid only when DDL triggers are running. It's valid to use it in stored
	procedures and functions called by DDL triggers.
	The DDL_TRIGGER context works like a stack. Before a DDL trigger is fired, it's pushed on that
	stack the values relative to the executed command. After the trigger finishes, the value is
	popped. So in the case of cascade DDL statements, when an user DDL command fires a DDL trigger
	and this trigger executes another DDL command with EXECUTE STATEMENT, the values of DDL_TRIGGER
	namespace are the ones relative to the command that fired the last DDL trigger on the call
	stack.

	The context elements are:
	- EVENT_TYPE: event type (CREATE, ALTER, DROP)
	- OBJECT_TYPE: object type (TABLE, VIEW, etc)
	- DDL_EVENT: event name (<ddl event item>), where <ddl_event_item> is EVENT_TYPE || ' ' || OBJECT_TYPE
	- OBJECT_NAME: metadata object name
	- SQL_TEXT: sql statement text


Example usages:
===============


1) Enforce a name consistense scheme. All procedure names should start with the prefix "SP_".

create exception e_invalid_sp_name 'Invalid SP name (should start with SP_)';

set term !;

create trigger trig_ddl_sp before create procedure
as
begin
    if (rdb$get_context('DDL_TRIGGER', 'OBJECT_NAME') not starting 'SP_') then
        exception e_invalid_sp_name;
end!

-- Test

create procedure sp_test
as
begin
end!

create procedure test
as
begin
end!

-- The last command raises this exception and procedure TEST is not created
-- Statement failed, SQLSTATE = 42000
-- exception 1
-- -E_INVALID_SP_NAME
-- -Invalid SP name (should start with SP_)
-- -At trigger 'TRIG_DDL_SP' line: 4, col: 5

set term ;!


----------------------------------------


2) Implement hand-made DDL security. Only certainly users could run DDL commands.

create exception e_access_denied 'Access denied';

set term !;

create trigger trig_ddl before any ddl statement
as
begin
    if (current_user <> 'SUPER_USER') then
        exception e_access_denied;
end!

-- Test

create procedure sp_test
as
begin
end!

-- The last command raises this exception and procedure SP_TEST is not created
-- Statement failed, SQLSTATE = 42000
-- exception 1
-- -E_ACCESS_DENIED
-- -Access denied
-- -At trigger 'TRIG_DDL' line: 4, col: 5

set term ;!


----------------------------------------


3) Log DDL actions and attempts.

create sequence ddl_seq;

create table ddl_log (
    id bigint not null primary key,
    moment timestamp not null,
    user_name varchar(31) not null,
    event_type varchar(25) not null,
    object_type varchar(25) not null,
    ddl_event varchar(25) not null,
    object_name varchar(31) not null,
    sql_text blob sub_type text not null,
    ok char(1) not null
);

set term !;

create trigger trig_ddl_log_before before any ddl statement
as
    declare id type of column ddl_log.id;
begin
    -- We do the changes in an AUTONOMOUS TRANSACTION, so if an exception happens and the command
    -- didn't run, the log will survive.
    in autonomous transaction do
    begin
        insert into ddl_log (id, moment, user_name, event_type, object_type, ddl_event, object_name,
                             sql_text, ok)
            values (next value for ddl_seq, current_timestamp, current_user,
                    rdb$get_context('DDL_TRIGGER', 'EVENT_TYPE'),
                    rdb$get_context('DDL_TRIGGER', 'OBJECT_TYPE'),
                    rdb$get_context('DDL_TRIGGER', 'DDL_EVENT'),
                    rdb$get_context('DDL_TRIGGER', 'OBJECT_NAME'),
                    rdb$get_context('DDL_TRIGGER', 'SQL_TEXT'),
                    'N')
            returning id into id;
        rdb$set_context('USER_SESSION', 'trig_ddl_log_id', id);
    end
end!

-- Note: the above trigger will fire for this DDL command. It's good idea to use -nodbtriggers
-- when working with them!
create trigger trig_ddl_log_after after any ddl statement
as
begin
    -- Here we need an AUTONOMOUS TRANSACTION because the original transaction will not see the
    -- record inserted on the BEFORE trigger autonomous transaction if user transaction is not
    -- READ COMMITTED.
    in autonomous transaction do
       update ddl_log set ok = 'Y' where id = rdb$get_context('USER_SESSION', 'trig_ddl_log_id');
end!

commit!

set term ;!

-- So lets delete the record about trig_ddl_log_after creation.
delete from ddl_log;
commit;

-- Test

-- This will be logged one time (as T1 did not exist, RECREATE acts as CREATE) with OK = Y.
recreate table t1 (
    n1 integer,
    n2 integer
);

-- This will fail as T1 already exists, so OK will be N.
create table t1 (
    n1 integer,
    n2 integer
);

-- T2 does not exists. There will be no log.
drop table t2;

-- This will be logged two times (as T1 exists, RECREATE acts as DROP and CREATE) with OK = Y.
recreate table t1 (
    n integer
);

commit;

select id, ddl_event, object_name, sql_text, ok from ddl_log order by id;

                   ID DDL_EVENT                 OBJECT_NAME                              SQL_TEXT OK     
===================== ========================= =============================== ================= ====== 
                    2 CREATE TABLE              T1                                           80:3 Y      
==============================================================================
SQL_TEXT:  
recreate table t1 (
    n1 integer,
    n2 integer
)
==============================================================================
                    3 CREATE TABLE              T1                                           80:2 N      
==============================================================================
SQL_TEXT:  
create table t1 (
    n1 integer,
    n2 integer
)
==============================================================================
                    4 DROP TABLE                T1                                           80:6 Y      
==============================================================================
SQL_TEXT:  
recreate table t1 (
    n integer
)
==============================================================================
                    5 CREATE TABLE              T1                                           80:9 Y      
==============================================================================
SQL_TEXT:  
recreate table t1 (
    n integer
)
==============================================================================

