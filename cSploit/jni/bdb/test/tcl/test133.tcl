# See the file LICENSE for redistribution information.
#
# Copyright (c) 2011, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	test133
# TEST  Test Cursor Cleanup.
# TEST  Open a primary database and a secondary database,
# TEST  then open 3 cursors on the secondary database, and
# TEST  point them at the first item.
# TEST  Do the following operations in loops:
# TEST   * The 1st cursor will delete the current item.
# TEST   * The 2nd cursor will also try to delete the current item.
# TEST   * Move all the 3 cursors to get the next item and check the returns.
# TEST  Finally, move the 3rd cursor once.

proc test133 {method {nentries 1000} {tnum "133"} {subdb 0} args} {
	source ./include.tcl

	# For rrecno, when keys are deleted, the ones after will move forward,
	# and the keys change, which is not good to verify after delete.
	# Therefore we skip rrecno method.
	if {[is_rrecno $method]} {
		puts "Skipping test$tnum for $method test."
		return
	}
	
	set sub_msg ""

	# Check if we use sub database.
	if { $subdb } {
		if {[is_queue $method]} {
			puts "Skipping test$tnum with sub database for $method."
			    return
		}
		if {[is_partitioned $args]} {
			puts "Skipping test$tnum with sub database\
		       	    for partitioned $method test."
			    return		
		}
		if {[is_heap $method]} {
			puts "Skipping test$tnum with sub database\
			     for $method."
			return
		}
		set sub_msg "using sub databases"
	}	

	# If we are using an env, then testfile should just be the db name.
	# Otherwise it is the test directory and the name.
	set eindex [lsearch -exact $args "-env"]
	set txnenv 0
	if { $eindex == -1 } {
		if {$subdb} {
			puts "Skipping test$tnum $sub_msg for non-env test."
			return
		}		
		set basename $testdir/test$tnum
		set env NULL
	} else {
		set basename test$tnum
		incr eindex
		set env [lindex $args $eindex]
		set txnenv [is_txnenv $env]
		if { $txnenv == 1 } {
			append args " -auto_commit "
		}
		set testdir [get_home $env]
	}

	cleanup $testdir $env
	set orig_args $args
	set args [convert_args $method $args]

	puts "Test$tnum: $method ($args)\
	    Cursor Cleanup Test $sub_msg."	

	# Hash does not have compression support.
	if {[is_compressed $orig_args]} {
		set secdb_types {"-btree"}
	} else {
		set secdb_types {"-btree" "-hash"}
	}
	set i 0
	foreach sec_method $secdb_types {
		set sec_args [convert_args $sec_method $orig_args]
		test133_sub "\tTest$tnum.$i" $basename $subdb $method $args \
		    $sec_method $sec_args $i
		incr i
	}
}

proc test133_sub { prefix basename use_subdb pri_method pri_args 
    sec_method sec_args indx } {
	global alphabet
	upvar txnenv txnenv
	upvar env env
	upvar nentries nentries
	
	# We can not set partition keys to hash.
	if {[is_partitioned $sec_args] && ![is_partition_callback $sec_args] \
	    && [is_hash $sec_method]} {
		puts "Skipping for $sec_method with $sec_args."
		return
	}

	if { $use_subdb } {
		set pri_testfile $basename.$indx.db
		set pri_subname "primary"
		set sec_testfile $basename.$indx.db
		set sec_subname "secondary"
	} else {
		set pri_testfile $basename.$indx-primary.db
		set pri_subname ""
		set sec_testfile $basename.$indx-secondary.db
		set sec_subname ""
	}

	puts "$prefix.a: Open the primary database."
	set pri_omethod [convert_method $pri_method]	
	set db [eval {berkdb_open_noerr -create -mode 0644} \
	    $pri_args $pri_omethod $pri_testfile $pri_subname]
	error_check_good dbopen [is_valid_db $db] TRUE	

	# Open a simple dupsort database.
	# In order to be consistent, we need to use all the passed-in 
	# am-unrelated flags.
	puts "$prefix.b: Open the secondary ($sec_method) database."
	set sec_db [eval {berkdb_open_noerr -create -mode 0644} $sec_args \
	    -dup -dupsort $sec_method $sec_testfile $sec_subname]
	error_check_good secdb_open [is_valid_db $sec_db] TRUE
	set ret [$db associate -create [callback_n 1] $sec_db]
	error_check_good db_associate $ret 0

	set txn ""
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	
	puts "$prefix.c: Putting data into the primary database."

	for {set i 1} {$i <= $nentries} {incr i} {
		error_check_good "put_$i" [eval $db put $txn \
		    $i [make_fixed_length $pri_method $i.$alphabet]] 0
	}

	puts "$prefix.d: Opening three cursors on secondary database."

	set cursor1 [eval $sec_db cursor $txn]
	set cursor2 [eval $sec_db cursor $txn]
	set cursor3 [eval $sec_db cursor $txn]

	puts "$prefix.e: Deleting records using the 1st cursor."	
	set delcnt [expr $nentries / 2]
	for {set i 0} {$i < $delcnt} {incr i} {
		set ret1 [$cursor1 get -next]
		set ret2 [$cursor2 get -next]
		set ret3 [$cursor3 get -next]
		error_check_good cmp_1_2 $ret1 $ret2
		error_check_good cmp_1_3 $ret1 $ret3
		
		error_check_good cursor1_del [eval $cursor1 del] 0
		set ret [catch {eval $cursor2 del} res]
		error_check_good {Check DB_NOTFOUND/DB_KEYEMPTY} \
		    [expr [is_substr $res DB_NOTFOUND] || \
		    [is_substr $res DB_KEYEMPTY]] 1
	}

	error_check_good cursor1_close [$cursor1 close] 0
	error_check_good cursor2_close [$cursor2 close] 0

	# The 3rd cursor is the final cursor pointing to the deleted item.
	# Usually, when the last cursor moves after all the deleted items,
	# these deleted items will be deleted physically. So here, we move
	# to next(it is after all the deleted items), then close the cursor.
	puts "$prefix.f: Moving the 3rd cursor."
	set ret3 [$cursor3 get -next]
	error_check_bad cursor3_get [llength $ret3] 0
	error_check_good cursor3_close [$cursor3 close] 0
	if { $txnenv == 1 } {
		error_check_good txn_commit [$t commit] 0
	}
	error_check_good secdb_close [$sec_db close] 0	
	error_check_good db_close [$db close] 0
}
