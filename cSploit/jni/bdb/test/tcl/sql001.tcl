# See the file LICENSE for redistribution information.
#
# Copyright (c) 2010, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	sql001
# TEST	Test db_replicate using a simple SQL app.
# TEST
# TEST  Start db_replicate on master side and client side,
# TEST	and do various operations using dbsql on master side.
# TEST  After every operation, we will check the records on both sides,
# TEST  to make sure we get same results from both sides.
# TEST  Also try an insert operation on client side; it should fail.

proc sql001 { {nentries 1000} {tnum "001"} args} {
	source ./include.tcl
	global EXE

	if { [file exists $util_path/dbsql$EXE] == 0 } {
		puts "Skipping Sql$tnum with dbsql.  Is it built?"
		return
	}

	env_cleanup $testdir

	puts "Sql$tnum: Test db_replicate with a simple SQL application."

	# Set up the directories and configuration files.
	set dbname sql001.db
	set envname $dbname-journal

	set masterdir $testdir/MASTERDIR
	set clientdir $testdir/CLIENTDIR.1

	file mkdir $masterdir
	file mkdir $masterdir/$envname
	file mkdir $clientdir
	file mkdir $clientdir/$envname

	foreach {porta portb} [available_ports 2] {}

	set confa [open $masterdir/$envname/DB_CONFIG w]
	set confb [open $clientdir/$envname/DB_CONFIG w]

	puts $confa "add_data_dir .."
	puts $confa "set_create_dir .."
	puts $confa "set_open_flags db_init_rep"
	puts $confa "repmgr_site 127.0.0.1 $porta db_local_site on db_group_creator on "
	puts $confa "repmgr_site 127.0.0.1 $portb"
	puts $confa "set_open_flags db_thread"
	puts $confa "set_open_flags db_register"

	puts $confb "add_data_dir .."
	puts $confb "set_create_dir .."
	puts $confb "set_open_flags db_init_rep"
	puts $confb "repmgr_site 127.0.0.1 $portb db_local_site on"
	puts $confb "repmgr_site 127.0.0.1 $porta db_bootstrap_helper on"
	puts $confb "set_open_flags db_thread"
	puts $confb "set_open_flags db_register"

	close $confa
	close $confb	

	set tmpsql "$testdir/sql001.sql"
	set masterfile "$testdir/master.sqlresults"
	set clientfile "$testdir/client.sqlresults"

	# For dbsql, to create the environment, we must first create a table.
	# And it is fine if we drop it right after creation.
	puts "\tSql$tnum.a:\
	    Create the environment on master and client side."
	build_sql_file $tmpsql [list \
	    "create table if not exists tempdb(f1, f2, f3);" \
	    "drop table if exists tempdb;"]

	set ret [catch {eval {exec \
	    $util_path/dbsql $masterdir/$dbname < $tmpsql > $masterfile}} res]
	error_check_good dbsql.a.1 $ret 0
	set ret [catch {eval {exec \
	    $util_path/dbsql $clientdir/$dbname < $tmpsql > $clientfile}} res]
	error_check_good dbsql.a.2 $ret 0

	set env_cmd(M) "berkdb_env_noerr -home $masterdir/$envname \
	    -errpfx MASTER -lock -log -txn"
	set masterenv [eval $env_cmd(M)]

	set env_cmd(C) "berkdb_env_noerr -home $clientdir/$envname \
	    -errpfx CLIENT -lock -log -txn"
	set clientenv [eval $env_cmd(C)]

	# Start the db_replicate program.
	set dpid(M) [eval {exec $util_path/db_replicate \
	    -h $masterdir/$envname -M -t 5 >& $testdir/master.log &}]
	set dpid(C) [eval {exec $util_path/db_replicate \
	    -h $clientdir/$envname -t 5 >& $testdir/client.log &}]

	await_condition \
	    {[stat_field $masterenv rep_stat "Next LSN expected"] == \
	    [stat_field $clientenv rep_stat "Next LSN expected"]}

	# We create two tables on master side and sleep a while. 
	# It is supposed that they will be on client side after replication.
	puts "\tSql$tnum.b: Create two tables on master side."
	set tab_fields {
		{"t1" {f11 f12 f13} }
		{"t2" {f21 f22 f23 f24} }
	}
	set sql_list {}
	for {set i 0} {$i < [llength $tab_fields]} {incr i} {
		set tablename [lindex [lindex $tab_fields $i] 0]
		set fields [lindex [lindex $tab_fields $i] 1]
		set fields_cnt [llength $fields]
		set sql_str "create table if not exists $tablename ("
		for {set j 0} {$j < $fields_cnt} {incr j} {
			set field [lindex $fields $j]
			if {$j < $fields_cnt - 1} {
				set sql_str "${sql_str}$field,"
			} else {
				set sql_str "${sql_str}$field);"
			}
		}
		lappend sql_list $sql_str
	}
	build_sql_file $tmpsql $sql_list

	set ret [catch {eval {exec \
	    $util_path/dbsql $masterdir/$dbname < $tmpsql >$masterfile}} res]
	error_check_good dbsql.b $ret 0

	await_condition \
	    {[stat_field $masterenv rep_stat "Next LSN expected"] == \
	    [stat_field $clientenv rep_stat "Next LSN expected"]}

	puts "\tSql$tnum.c: Query table list on both sides."
	build_sql_file $tmpsql ".tables"

	set ret [catch {eval {exec \
	    $util_path/dbsql $masterdir/$dbname < $tmpsql >$masterfile}} res]
	error_check_good dbsql.c.1 $ret 0
	set ret [catch {eval {exec \
	    $util_path/dbsql $clientdir/$dbname < $tmpsql >$clientfile}} res]
	error_check_good dbsql.c.2 $ret 0
	error_check_good result_compare.1 [filecmp $masterfile $clientfile] 0

	# Insert some records on master side and check them on both sides.
	puts "\tSql$tnum.d: Insert records on master side."
	set sql_list {}
	for {set i 1} {$i <= $nentries} {incr i} {
		lappend sql_list "insert into t1 \
		    values(\"$i\", \"vf12_$i\", \"vf13_$i\");"
		lappend sql_list "insert into t2 \
		    values(\"$i\", \"vf22_$i\", \"vf23_$i\", \"vf24_$i\");"
	}
	build_sql_file $tmpsql $sql_list

	set ret [catch {eval {exec \
	    $util_path/dbsql $masterdir/$dbname < $tmpsql >& masterfile}} res]
	error_check_good dbsql.d.1 $ret 0

	await_condition \
	    {[stat_field $masterenv rep_stat "Next LSN expected"] == \
	    [stat_field $clientenv rep_stat "Next LSN expected"]} 100

	puts "\tSql$tnum.e: Query the records on both sides."
	build_sql_file $tmpsql [list "select * from t1;" "select * from t2;"]

	set ret [catch {eval {exec \
	    $util_path/dbsql $masterdir/$dbname < $tmpsql >$masterfile}} res]
	error_check_good dbsql.e.1 $ret 0
	set ret [catch {eval {exec \
	    $util_path/dbsql $clientdir/$dbname < $tmpsql >$clientfile}} res]
	error_check_good dbsql.e.2 $ret 0
	error_check_good result_compare.2 [filecmp $masterfile $clientfile] 0
	
	# Update first-half of the records on master side and check the records
        # on both sides.
	puts "\tSql$tnum.f: Update some records on master side."
	set sql_list {}
	for {set i 1} {$i < $nentries / 2} {incr i 4} {
		lappend sql_list "update t1 set f12=\"vf12_${i}_$i\" , \
		    f13=\"vf13_${i}_$i\" where f11=\"$i\";"
		lappend sql_list "update t2 set f22=\"vf22_${i}_new\", \
		    f23=\"vf23_${i}_new\", f24=\"vf24_${i}_new\" \
		    where f21=\"$i\";"
	}
	build_sql_file $tmpsql $sql_list

	set ret [catch {eval {exec \
	    $util_path/dbsql $masterdir/$dbname < $tmpsql > $masterfile}} res]
	error_check_good dbsql.f.1 $ret 0

	await_condition \
	    {[stat_field $masterenv rep_stat "Next LSN expected"] == \
	    [stat_field $clientenv rep_stat "Next LSN expected"]} 100

	puts "\tSql$tnum.g: Query the records on both sides."
	build_sql_file $tmpsql [list "select * from t1;" "select * from t2;"]

	set ret [catch {eval {exec \
	    $util_path/dbsql $masterdir/$dbname < $tmpsql >$masterfile}} res]
	error_check_good dbsql.g.1 $ret 0
	set ret [catch {eval {exec \
	    $util_path/dbsql $clientdir/$dbname < $tmpsql >$clientfile}} res]
	error_check_good dbsql.g.2 $ret 0
	error_check_good result_compare.3 [filecmp $masterfile $clientfile] 0

	# Delete second-half of the records on master side, and then check the
       	# records on both sides.
	puts "\tSql$tnum.h: Delete some records on master side."
	set sql_list {}
	for {set i [expr $nentries / 2]} {$i <= $nentries} {incr i 4} {
		lappend sql_list "delete from t1 where f11=\"$i\";"
		lappend sql_list "delete from t2 where f21=\"$i\";"
	}
	build_sql_file $tmpsql $sql_list

	set ret [catch {eval {exec \
	    $util_path/dbsql $masterdir/$dbname < $tmpsql > $masterfile}} res]
	error_check_good dbsql.h.1 $ret 0

	await_condition \
	    {[stat_field $masterenv rep_stat "Next LSN expected"] == \
	    [stat_field $clientenv rep_stat "Next LSN expected"]} 100

	puts "\tSql$tnum.i: Query the records on both sides."
	build_sql_file $tmpsql [list "select * from t1;" "select * from t2;"]

	set ret [catch {eval {exec \
	    $util_path/dbsql $masterdir/$dbname < $tmpsql >$masterfile}} res]
	error_check_good dbsql.i.1 $ret 0
	set ret [catch {eval {exec \
	    $util_path/dbsql $clientdir/$dbname < $tmpsql >$clientfile}} res]
	error_check_good dbsql.i.2 $ret 0
	error_check_good result_compare.3 [filecmp $masterfile $clientfile] 0

	# Data changes should be forbidden on client side.
	# So here, we are trying to insert some records on client side, 
	# and we suppose dbsql return a non-zero code. 
	# The records on client sides should keep unchanged.
	puts "\tSql$tnum.j: Insert records on client side."
	set sql_list {}
	for {set i [expr $nentries + 1]} {$i <= [expr $nentries + 500]} \
	    {incr i} {	
		lappend sql_list "insert into t1 \
		    values(\"$i\", \"vf12_$i\", \"vf13_$i\");"	
		lappend sql_list "insert into t2 \
		    values(\"$i\", \"vf22_$i\", \"vf23_$i\", \"vf24_$i\");"
	}
	build_sql_file $tmpsql $sql_list

	set ret [catch {eval {exec \
	    $util_path/dbsql $clientdir/$dbname < $tmpsql > $masterfile.1 }} res]
	error_check_bad dbsql.j.1 $ret 0

	await_condition \
	    {[stat_field $masterenv rep_stat "Next LSN expected"] == \
	    [stat_field $clientenv rep_stat "Next LSN expected"]}

	puts "\tSql$tnum.k: Query the records on both sides."
	build_sql_file $tmpsql [list "select * from t1;" "select * from t2;"]

	set ret [catch {eval {exec \
	    $util_path/dbsql $masterdir/$dbname < $tmpsql >$masterfile.1}} res]
	error_check_good dbsql.k.1 $ret 0
	set ret [catch {eval {exec \
	    $util_path/dbsql $clientdir/$dbname < $tmpsql >$clientfile.1}} res]
	error_check_good dbsql.k.2 $ret 0
	error_check_good result_compare.4.1 \
	    [filecmp $masterfile.1 $clientfile.1] 0
	error_check_good result_compare.4.2 \
	    [filecmp $masterfile.1 $masterfile] 0

	# We execute some queries on both sides, and we suppose we will get 
	# same results on both sides for these queries
	puts "\tSql$tnum.l: Execute some queries on both sides."
	set queries [list \
		"select * from t1 where f11 >= [expr $nentries / 2];" \
		"select * from t2 where f21 >= [expr $nentries / 2];" \
		"select * from t1 where f11 <= [expr $nentries / 3];" \
		"select * from t2 where f21 <= [expr $nentries / 3];"]

	set query_len [llength $queries]
	for {set i 0} {$i < $query_len} {incr i} {
		build_sql_file $tmpsql [lindex $queries $i]
		set ret [catch {eval {exec $util_path/dbsql \
		    $masterdir/$dbname < $tmpsql >$masterfile}} res]
		error_check_good dbsql.l.$i.1 $ret 0
		set ret [catch {eval {exec $util_path/dbsql \
		    $clientdir/$dbname < $tmpsql >$clientfile}} res]
		error_check_good dbsql.l.$i.2 $ret 0
		error_check_good result_compare.5.$i \
		    [filecmp $masterfile $clientfile] 0
	}

	# Here, we create two views, and suppose they will be on client side.
	# Also, we try to do query on these views on both sides, and we 
	# suppose they will return same records.
	puts "\tSql$tnum.m: Create two views on master side."
	build_sql_file $tmpsql [list \
	    "create view if not exists view_t1 as select f11, f12 from t1;" \
	    "create view if not exists view_t2 as select f22, f23 from t2;"]

	set ret [catch {eval {exec \
	    $util_path/dbsql $masterdir/$dbname < $tmpsql >$masterfile}} res]
	error_check_good dbsql.m.1 $ret 0

	await_condition \
	    {[stat_field $masterenv rep_stat "Next LSN expected"] == \
	    [stat_field $clientenv rep_stat "Next LSN expected"]}

	puts "\tSql$tnum.n: Query the table list on both sides."
	build_sql_file $tmpsql ".tables"

	set ret [catch {eval {exec \
	    $util_path/dbsql $masterdir/$dbname < $tmpsql >$masterfile}} res]
	error_check_good dbsql.n.1 $ret 0
	set ret [catch {eval {exec \
	    $util_path/dbsql $clientdir/$dbname < $tmpsql >$clientfile}} res]	
	error_check_good dbsql.n.2 $ret 0

	puts "\tSql$tnum.o: Query the records from views on both sides."
	build_sql_file $tmpsql [list \
	    "select * from view_t1;" "select * from view_t2;"]

	set ret [catch {eval {exec \
	    $util_path/dbsql $masterdir/$dbname < $tmpsql >$masterfile}} res]
	error_check_good dbsql.o.1 $ret 0
	set ret [catch {eval {exec \
	    $util_path/dbsql $clientdir/$dbname < $tmpsql >$clientfile}} res]
	error_check_good dbsql.o.2 $ret 0
	error_check_good result_compare.6 [filecmp $masterfile $clientfile] 0

	# Drop all the tables, views from master side, and we suppose
	# They will disappear on client side too.
	# After that, running .tables should return nothing.
	puts "\tSql$tnum.p: Drop all the tables and views on master side."
	build_sql_file $tmpsql [list \
	    "drop view if exists view_t1;" "drop view if exists view_t2;" \
	    "drop table if exists t1;" "drop table if exists t2;"]

	set ret [catch {eval {exec \
	    $util_path/dbsql $masterdir/$dbname < $tmpsql >$masterfile}} res]
	error_check_good dbsql.p.1 $ret 0

	await_condition \
	    {[stat_field $masterenv rep_stat "Next LSN expected"] == \
	    [stat_field $clientenv rep_stat "Next LSN expected"]}

	puts "\tSql$tnum.q: Query the table list on both sides."
	set outf [open $masterfile.1 w]
	close $outf
	build_sql_file $tmpsql ".tables"

	set ret [catch {eval {exec \
	    $util_path/dbsql $masterdir/$dbname < $tmpsql >$masterfile}} res]
	error_check_good dbsql.q.1 $ret 0
	set ret [catch {eval {exec \
	    $util_path/dbsql $clientdir/$dbname < $tmpsql >$clientfile}} res]
	error_check_good dbsql.q.2 $ret 0
	error_check_good result_compare.7.1 [filecmp $masterfile $clientfile] 0
	error_check_good result_compare.7.2 \
	    [filecmp $masterfile $masterfile.1] 0
	
	# Kill the processes for db_replicate.
	tclkill $dpid(C)
	tclkill $dpid(M)

	error_check_good masterenv_close [$masterenv close] 0
	error_check_good clientenv_close [$clientenv close] 0
}

proc build_sql_file {sqlfile stmt_list} {
	set sqlf [open $sqlfile w]
	foreach stmt $stmt_list {
		puts $sqlf $stmt
	}
	close $sqlf
}
