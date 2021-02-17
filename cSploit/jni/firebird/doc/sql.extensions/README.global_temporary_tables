SQL Language Extension: global temporary tables

Author:
    Vlad Khorsun <hvlad at users.sourceforge.net>


Function:
	Global temporary tables (GTTs) are tables with permanent metadata, stored 
in the system catalogue, but with temporary data. GTT's may be of two kinds - 
with data, persistent within lifetime of connection in which the given GTT 
was referenced, and with data, persistent within only during lifetime of 
referencing transaction. The data from different connections (transactions) are 
isolated from each other, but metadata of the global temporary table are shared 
between all connections and transactions.


	Syntax and rules :
	
CREATE GLOBAL TEMPORARY TABLE 
		...
	[ON COMMIT <DELETE | PRESERVE> ROWS]
	
	Creates metadata of the temporary table in the system catalogue. 
Clause ON COMMIT sets the kind of temporary table:

	ON COMMIT PRESERVE ROWS : data of the given table after end of transaction 
		remain in database until end of connection
		
	ON COMMIT DELETE ROWS : data of the given table are deleted from database 
		immediately after end of transaction 
		
	If optional clause ON COMMIT is not specified ON COMMIT DELETE ROWS is 
used by default. 

	CREATE GLOBAL TEMPORARY TABLE - usual DDL statement and processed by the 
engine the same way as operator CREATE TABLE. Therefore it's impossible to create or 
drop GTT within stored procedure or trigger.

	GTT differs from permanent tables by value of RDB$RELATIONS.RDB$RELATION_TYPE :
GTT with ON COMMIT PRESERVE ROWS option has value 4 in RDB$RELATION_TYPE field
whereas GTT with ON COMMIT DELETE ROWS option has value of 5. See full list of 
values in RDB$TYPES

	GTT may have indexes, triggers, field level and table level constraints - as 
well as usual tables. 

	All kinds of constraints between temporary and persistent tables follow 
the rules below:

	a) references between persistent and temporary tables are forbidden 
	b) GTT with ON COMMIT PRESERVE ROWS can't have reference on GTT with 
		ON COMMIT DELETE ROWS 
	c) Domain constraints can't have reference on GTT.


	Implementation details:

	GTT instance (set of data rows created by and visible within given connection 
or transaction) is created when referenced for the first time, usually at statement prepare 
time. Each instance has its own private set of pages on which data and indexes 
are stored. Data rows and indexes have the same physical storage layout as 
permanent tables. 

	When connection or transaction ends all pages of a GTT instance are released 
immediately (this is similar as when you do DROP TABLE but metatada remains in 
database of course). This is much quicker than traditional row by row delete + 
garbage collection of deleted record versions. DELETE triggers are not fired in 
this case.

	Data and index pages of all of the GTTs instances are placed in separate temporary 
files. Each connection has its own temporary file created when this connection 
first referenced some GTT. Also these temporary files are always opened with "Forced 
Writes = OFF" setting despite of database setting. 

	There's no limit on number of GTT instances. If you have N transactions 
active simultaneously and each transaction has referenced some GTT then you'll 
have N GTTs instances.
