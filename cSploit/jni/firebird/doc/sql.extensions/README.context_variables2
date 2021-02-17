-----------------------------------------
Generic user and system context variables
-----------------------------------------

Function:
    New built-in functions give access to some information about current
    connection and current transaction. Also they provide means to associate
    and retrieve user context data with transaction or connection.

Author:
    Nickolay Samofatov <nickolay at broadviewsoftware dot com>

Format:
    RDB$SET_CONTEXT( <namespace>, <variable>, <value> )
    RDB$GET_CONTEXT( <namespace>, <variable> )

Declared as:
  DECLARE EXTERNAL FUNCTION RDB$GET_CONTEXT
      VARCHAR(80),
      VARCHAR(80)
  RETURNS VARCHAR(255) FREE_IT;

  DECLARE EXTERNAL FUNCTION RDB$SET_CONTEXT
      VARCHAR(80),
      VARCHAR(80),
      VARCHAR(255)
  RETURNS INTEGER BY VALUE;

Usage:

  RDB$SET_CONTEXT and RDB$GET_CONTEXT set and retrieve current value for the 
  context variables. Namespace name identifies a group of context variables with
  similar properties. Access rules such as the fact if variables may be read and
  written to and by whom are determined by namespace which they belong to.

  Namespace and variable names are case-sensitive.

  RDB$GET_CONTEXT retrieves current value of a variable. If variable does not
  exist in namespace return value for the function is NULL.

  RDB$SET_CONTEXT sets a value for specific variable. Function returns value of
  1 if variable existed before the call and 0 otherwise. To delete variable from
  context set its value to NULL.

  Currently, there is a fixed number of pre-defined namespaces you may use.

  USER_SESSION namespace offers access to session-specific user-defined 
  variables. You can define and set value for variable with any name in this
  context. USER_TRANSACTION namespace offers the same possibilities for
  individual transactions.

  SYSTEM namespace provides read-only access to the following variables.

   Variable name        Value
  ------------------------------------------------------------------------------
   NETWORK_PROTOCOL | The network protocol used by client to connect. Currently
                    | used values: "TCPv4", "WNET", "XNET" and NULL
                    |
   CLIENT_ADDRESS   | The wire protocol address of remote client represented as
                    | string. Value is IP address in form "xxx.xxx.xxx.xxx" for
                    | TCPv4 protocol, local host name for XNET protocol and
                    | NULL for all other protocols
                    |
   DB_NAME          | Canonical name of current database. It is either alias
                    | name if connectivity via file names is not allowed or
                    | fully expanded database file name otherwise.
                    |
   ISOLATION_LEVEL  | Isolation level for current transaction. Returned values
                    | are "READ COMMITTED", "CONSISTENCY", "SNAPSHOT"
                    |
   TRANSACTION_ID   | Numeric ID for current transaction. Returned value is the
                    | same as of CURRENT_TRANSACTION pseudo-variable
                    |
   SESSION_ID       | Numeric ID for current session. Returned value is the
                    | same as of CURRENT_CONNECTION pseudo-variable
                    |
   CURRENT_USER     | Current user for the connection. Returned value is the
                    | same as of CURRENT_USER pseudo-variable
                    |
   CURRENT_ROLE     | Current role for the connection. Returned value is the
                    | same as of CURRENT_ROLE pseudo-variable
                    |
   ENGINE_VERSION   | Engine version number, e.g. "2.1.0" (since V2.1)

Notes:
   To prevent DoS attacks against Firebird Server you are not allowed to have
   more than 1000 variables stored in each transaction or session context.

Example(s):

create procedure set_context(User_ID varchar(40), Trn_ID integer) as
begin
  RDB$SET_CONTEXT('USER_TRANSACTION', 'Trn_ID', Trn_ID);
  RDB$SET_CONTEXT('USER_TRANSACTION', 'User_ID', User_ID);
end;

create table journal (
   jrn_id integer not null primary key,
   jrn_lastuser varchar(40),
   jrn_lastaddr varchar(255),
   jrn_lasttransaction integer
);

CREATE TRIGGER UI_JOURNAL FOR JOURNAL BEFORE INSERT OR UPDATE
as 
begin
  new.jrn_lastuser = rdb$get_context('USER_TRANSACTION', 'User_ID');
  new.jrn_lastaddr = rdb$get_context('SYSTEM', 'CLIENT_ADDRESS');
  new.jrn_lasttransaction = rdb$get_context('USER_TRANSACTION', 'Trn_ID');
end;

execute procedure set_context('skidder', 1);

insert into journal(jrn_id) values(0);

commit;
