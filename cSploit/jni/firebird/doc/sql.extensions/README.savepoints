SQL Language Extension: support for SQL99 savepoints

Syntax
========

SAVEPOINT <identifier>;

Use the SAVEPOINT statement to identify a point in a transaction
to which you can later roll back. <identifier> specifies the
name of a savepoint to be created. Savepoint names must be 
distinct within a given transaction. If you create a second
savepoint with the same identifer as an earlier savepoint,
the earlier savepoint is erased. After a savepoint has been 
created, you can either continue processing, commit your work,
roll back the entire transaction, or roll back to the savepoint.

ROLLBACK [WORK] TO [SAVEPOINT] <identifier>;

This statement performs the following operations:
 - Rolls back changes performed in the transaction after the savepoint
 - Erases all savepoints created after that savepoint. The named
   savepoint is retained, so you can roll back to the same savepoint
   multiple times. Prior savepoints are also retained.
 - Releases all implicit and explicit record locks acquired since the
   savepoint. Other transactions that have requested access to rows
   locked after the savepoint must continue to wait until the transaction
   is committed or rolled back. Other transactions that have not already
   requested the rows can request and access the rows immediately.
   Note: this behaviour may change in the future product versions.

RELEASE SAVEPOINT <identifier> [ONLY];

Use the RELEASE SAVEPOINT statement to erase savepoint <identifer>
from the transaction context. Unless you specify ONLY clause all savepoints
established since the savepoint <identifier> are erased too.

Author:
   Nickolay Samofatov <skidder@bssys.com>
   (original implementation of user savepoints)
   Dmitry Yemanov <yemanov@yandex.ru>
   (refactored DSQL layer and BLR for better SQL99 and JDBC compliance)

N O T E S
=========

1. Using savepoints (alternate name is "nested transactions") is a very 
  convenient method to handle business logic errors without rolling back 
  the transaction.
  
  Common pattern for this (Java) is:
    stmt.executeUpdate("SAVEPOINT MyClass$do_some_work");
    try {
      MyClass.do_some_work();
    } catch(Exception ex) {
      stmt.executeUpdate("ROLLBACK TO MyClass$do_some_work");
      throw;
    }
    stmt.executeUpdate("RELEASE SAVEPOINT MyClass$do_some_work");

2. User savepoints are not supported in PSQL. Use traditional PSQL 
  exception handling to undo changes performed in stored procedures 
  and triggers. Support of user savepoints in PSQL layer would break 
  the concept of statement atomicity (including procedure call statements).
  Each SQL/PSQL statement is executed under automatic system savepoint
  and either complete successfully or ALL its changes are rolled 
  back and exception is raised. Each PSQL exception handling block is
  also bounded by automatic system savepoints.

3. Savepoint undo log may consume significant amounts of server
  memory exceptionally if you update the same records in the
  same transaction multiple times. Use RELEASE SAVEPOINT statement
  to release system resources required for savepoint maintenance.
  
4. By default, engine uses automatic transaction-level system 
  savepoint to perform transaction rollback.  When you issue
  ROLLBACK statement all changes performed in this transaction
  are backed out via transaction-level savepoint and transaction
  is committed then. This logic is used to reduce amount of garbage 
  collection caused by rolled back transactions. When amount of 
  changes performed under transaction-level savepoint is getting large 
  (10^4-10^6 records affected) engine releases transaction-level 
  savepoint and uses TIP mechanism to roll back the transaction if 
  needed. You can use isc_tpb_no_auto_undo TPB flag to avoid creating 
  transaction-level savepoint if you expect large amount of changes 
  in your transaction.

Example
=======

create table test (id integer);
commit;
insert into test values (1);
commit;
insert into test values (2);
savepoint y;
delete from test;
select * from test; -- returns no rows
rollback to y; 
select * from test; -- returns two rows
rollback;
select * from test; -- returns one row
