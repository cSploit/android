/*
 * The contents of this file are subject to the Interbase Public
 * License Version 1.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy
 * of the License at http://www.Inprise.com/IPL.html
 *
 * Software distributed under the License is distributed on an
 * "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either express
 * or implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code was created by Inprise Corporation
 * and its predecessors. Portions created by Inprise Corporation are
 * Copyright (C) Inprise Corporation.
 *
 * All Rights Reserved.
 * Contributor(s): ______________________________________.
 */
The following example programs demonstrate a number of useful features of
the Firebird programming interface.

Note that the following environment variables need to be set before running
the examples:

ISC_USER     - A valid username on the server.
ISC_PASSWORD - The password for the above user.
ISC_DATABASE - The path to the employee.gdb example database, including
	       server name.  For example, to connect to the NT server
	       named "NTserver" via NetBEUI:
		ISC_DATABASE=\\NTserver\c:\firebird\examples

	       This assumes that Firebird was installed to the
	       firebird directory on the c: drive.

In addition, a guest account should be created with the username "guest"
and password "guest" before running api15 and winevent.

Embedded Static SQL

Program     Description
---------   ------------------------------------------------------------------
stat1.e     Illustrates a simple update to an existing table, commit, rollback.

stat2.e     Illustrates singleton select.

stat3.e     Illustrates a simple cursor -- declare/open/close/fetch.

stat4.e     Show 'declare table' and 'create table'.

stat5.e     Demonstrate 'update where current of'.

stat6.e     Select an array.

stat7.e     Illustrate blob cursor for select.

stat8.e     Illustrate blob cursor for insert.

stat9.e     Execute and select from a stored procedure.

stat10.e    Demonstrate 'set database', 'connect' and 'set transaction'.

stat11.e    Demonstrate 'set transaction' with various isolation options.

stat12.e    Event wait and signaling.
stat12t.e

WHENEVER SQLERROR and BASED_ON clause are illustrated by many programs.
^L

Embedded Dynamic SQL

Program     Description
---------   ------------------------------------------------------------------
dyn1.e      Execute 'create database' statement as a static string.

dyn2.e      'Execute immediate', and 'prepare' and 'execute'.

dyn3.e      Dynamic cursor for select with output SQLDA allocated.

dyn4.e      Execute an update query with parameter markers and input SQLDA.

dyn5.e      Demonstrate dynamic reallocation of SQLDA and 'describe' statement.

dynfull.e   A full_dsql program (process unknown statements).

VARY struct is used by dyn3.e, dynfull.e.
^L

API Interface

Program     Description
---------   ------------------------------------------------------------------
api1.c      Execute 'create dabatabase' statement as a static string.
            Demonstrates zero database handle.

api2.c      'Execute immediate', and 'prepare' and 'execute'.

api3.c      Dynamic cursor for select with output SQLDA allocated.

api4.c      Execute an update query with parameter markers and input SQLDA.

api5.c      Demonstrate dynamic reallocation of SQLDA and 'describe' statement.

apifull.c   A full_dsql program (process unknown statements).
	    Demonstrates stmt_info calls and numeric scale.

api6.c      Assemble an update current of statement, based on a dynamic
            cursor name.  Free a statement handle and re-use it as the cursor.

api7.c      Demonstrate blob_open, get_segment.

api8.c      Demonstrate create_blob, put_segment.

api9.c      Demonstrate blob_open2 (using blob filter).
api9f.c     Filter for api9.c.

api10.c     Update an array using get_slice/put_slice.

api11.c     Execute and select from a stored procedure.

api12.c     A program with several active transactions.

api13.c     A multi-database transaction with 2-phase commit.

api14.e     Combine the three programming styles in one program.

api15.c	    Construct a database parameter buffer.  db_info calls.

api16.c	    Demonstrate asynchronous event trapping.            

winevent.c  Demonstrate asynchronous event trapping
            (Replacement for api16.c on Windows Client)

api16t.c    Identical to stat12t, this triggers the event for api16.

VARY struct is used by api3.c, apifull.c, and api14.e.

SQLCODE extraction from status is covered by several programs.

Zero transaction handle is covered in several programs, ex. api14.e.


