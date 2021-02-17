SQL Language Extension: WITH LOCK clause in the SELECT statement

Syntax
========

SELECT ... FROM <sometable> [WHERE ...] [FOR UPDATE [OF ...]] WITH LOCK;

   The WITH LOCK clause lets you lock the selected rows so that
   other users cannot lock or update the rows until you end your
   transaction. You can specify this clause only in a top-level 
   SELECT statement (not in subqueries). This clause can be used 
   in DSQL and PSQL.

Restrictions:
 - You cannot specify this clause with the following other constructs:
   the DISTINCT operator, group_by_clause, or aggregate functions.
 - from_clause of SELECT statement must specify single database table
   (views and external tables are not allowed)

Author:
   Nickolay Samofatov <skidder@bssys.com>


N O T E S
=========

When engine processes each record falling under explicit lock 
statement it either returns most current committed record version
or raises exception.

Wait behaviour and conflict reporting depends on the transaction 
parameters specified in the TPB block.

TPB mode                      Behavior
-------------------------------------------------------------
isc_tpb_consistency           Explicit locks are overriden by
                              implicit or explicit table-level locks
							  and are ignored
							  
isc_tpb_concurrency +         If record is modified by any committed
isc_tpb_nowait                transaction since transaction attempting
                              to get explicit lock started or there is 
							  active transaction that performed 
							  modifiction of this record, update 
							  conflict exception is raised immediately
							  
isc_tpb_concurrency +         If record is modified by any committed
isc_tpb_wait                  transaction since transaction attempting
                              to get explicit lock started, update conflict
							  exception is raised immediately.
							  If there is active transaction holding ownership 
							  on this record (via explicit lock or normal update)
							  locking transaction waits for outcome of 
							  blocking transation and when it finishes
							  attempts to get lock on record again.
							  This means that if blocking transaction
							  commits modification of this record,
							  update conflict exception will be raised.
							  
isc_tpb_read_committed +      If there is active transaction holding ownership 
isc_tpb_nowait				  on this record (via explicit lock or normal update),
                              update conflict exception is raised immediately.

isc_tpb_read_committed +      If there is active transaction holding ownership 
isc_tpb_wait				  on this record (via explicit lock or normal update),
							  locking transaction waits for outcome of 
							  blocking transation and when it finishes 
							  attempts to get lock on record again.
							  Update conflict exceptions can never be raised
							  by this kind of explicit lock statement.

When UPDATE statement steps on a record that is locked by another transaction
it either raises update conflict exception or waits for the end of locking 
transaction depending on TPB mode. Engine behaviour here is the same as if this 
record was already modified by locking transaction.

Engine guaranties that all records returned by explicit lock statement
are actually locked and DO fall under search condition specified in WHERE clause
if this search condition depends only on a record to be returned (for example, it 
contains no subqueries reading mutable tables, etc). It also guaranties that
no rows not falling under search condition are locked by the statement. It doesn't 
guaranty that there are no rows falling under search condition, but not locked 
(this may happen if other parallel transactions commit their changes during 
execution of locking statement).

Engine locks rows at fetch time. This has important consequences if you lock
several rows at once. By default, access methods for Firebird databases
fetch results in packs of a few hundred rows. Most access components may not 
give you the rows contained in the last fetched packed where error happened.
You may use FOR UPDATE clause to prevent usage of buffered fetches (and may
use positioned updates feature) or set fetch buffer size to 1 in your access
components in case you need to process locked row before next row is locked
or if you want actually process all rows that can be locked before first row
producing error.

Commit and rollback retaining release explicit and implicit locks
exactly as normal commit and rollback. Be careful when using buffered
fetches and retaining operations. Records contained in the client-side 
buffer may be unlocked even before returned to the application.

Rolling back of implicit or explicit savepoint releases record locks that
were taken under this savepoint, but doesn't notify waiting transactions. 
Applications should not depend on this behaviour as it may get changed in 
the future.

While explicit locks can be used to prevent and/or handle update conflict
errors amount of deadlock errors will grow unless you carefully design your 
locking strategy. Most applications do not need explicit locks at all. The main
purposes of explicit locks are (1) to prevent expensive handling of update 
conflict errors in heavily loaded applications and (2) to maintain integrity
of objects mapped to relational database in clustered environment. 
While solutions for this problems may be very important for web sites 
handling thousands of concurrent users or ERP/CRM solutions
working in large corporations most application programs do not
need to work in such conditions. Explicit locking is an advanced feature,
do not misuse it ! If your use of explicit locking doesn't fall in
one of two categories above think of another way to do the task.

Examples:

A) (simple)
  SELECT * FROM DOCUMENT WHERE ID=? WITH LOCK

B) (multiple rows, one-by-one processing with DSQL cursor)
  SELECT * FROM DOCUMENT WHERE PARENT_ID=? FOR UPDATE WITH LOCK
