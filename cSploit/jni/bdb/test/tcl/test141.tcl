# See the file LICENSE for redistribution information.
#
# Copyright (c) 2011, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	test141
# TEST	Test moving a large part of a hash table to a continguous
# TEST	block of free pages.
# TEST
# TEST	Create a database with a btree and a hash subdb.  Remove
# TEST	all the data from the first (btree) db and then compact
# TEST	to cause the hash db to get moved. 

proc test141 { method {nentries 10000} {tnum "141"} args } {
	source ./include.tcl
	global alphabet
	global passwd

	# This is a hash-only test.
	if { [is_hash $method] != 1} {
		puts "Skipping test$tnum for method $method."
		return
	}

	# Skip for partitioning; this test uses subdatabases.
	if { [is_partitioned $args] == 1 } {
		puts "Test$tnum skipping for partitioned $method"
		return
	}

	# This test creates its own transactional env.
	set eindex [lsearch -exact $args "-env"]
	if { $eindex != -1 } {
		incr eindex
		set env [lindex $args $eindex]
		puts "Test$tnum skipping for env $env"
		return
	}
	set args [convert_args $method $args]
	set omethod [convert_method $method]
	set encargs ""
	set args [split_encargs $args encargs]

	set testfile test$tnum.db
	set data [repeat $alphabet 5]

	puts "Test$tnum: $method ($args $encargs)\
	    Hash testing where we move a large block of entries."

	env_cleanup $testdir
	set env [eval {berkdb_env_noerr} -create -txn -pagesize 512 \
	    {-cachesize { 0 1048576 1 }} $encargs -home $testdir]
	error_check_good env_open [is_valid_env $env] TRUE

	# Populate a btree subdb.
	puts "\tTest$tnum.a: Create and populate a btree subdb."
	set bdb [eval {berkdb_open_noerr -create -btree \
	    -auto_commit -env $env} $args {$testfile BTREE}]    
	error_check_good db_open [is_valid_db $bdb] TRUE

	set t [$env txn]
	set txn "-txn $t"

	for { set i 0 } { $i < $nentries } { incr i } {
		set ret [eval {$bdb put} $txn {$i $i.$data}]	
		error_check_good db_put $ret 0
	}

	error_check_good txn_commit [$t commit] 0
	$bdb sync

	# Now a small hash subdb.
	puts "\tTest$tnum.b: Create and populate a hash subdb."
	set hdb [eval {berkdb_open_noerr -create -hash \
	    -auto_commit -env $env} $args {$testfile HASH}]
	error_check_good dbopen [is_valid_db $hdb] TRUE

	set t [$env txn]
	set txn "-txn $t"

	for { set i 0 } { $i < 100 } { incr i } {
		set ret [eval {$hdb put} $txn {$i $i.$data}]
		error_check_good put $ret 0
	}

	error_check_good txn_commit [$t commit] 0
	$hdb sync

	# Delete the contents of the btree subdatabase. 
	set t [$env txn]
	set txn "-txn $t"
	for { set i 0 } { $i < $nentries } { incr i } {
		set ret [eval {$bdb del} $txn {$i}]
		error_check_good db_del $ret 0
	}
	error_check_good txn_commit [$t commit] 0
	$bdb sync

	# Compact.
	puts "\tTest$tnum.d: Compact the btree."
	set orig_size [file size $testdir/$testfile]

	set t [$env txn]
	set txn "-txn $t"

	if {[catch {eval {$bdb compact} $txn \
	    { -freespace}} ret] } { 
		error "FAIL: db compact: $ret" 
	} 

	error_check_good txn_commit [$t commit] 0
	$bdb sync 

	# The btree compaction is not expected to reduce
	# file size because the last page of the file is hash.
	set after_bt_compact_size [file size $testdir/$testfile]
	error_check_good file_size\
	    [expr $orig_size == $after_bt_compact_size] 1

	puts "\tTest$tnum.d: Compact the hash."
	set t [$env txn]
	set txn "-txn $t"

	if {[catch {eval {$hdb compact} $txn \
	    { -freespace}} ret] } { 
		error "FAIL: db compact: $ret" 
	} 

	error_check_good txn_commit [$t commit] 0
	$hdb sync 

	# Now we expect the file to be much smaller. 
	set after_h_compact_size [file size $testdir/$testfile]
	error_check_good file_size_reduced\
	    [expr $orig_size > [expr $after_h_compact_size * 5]] 1

	# The run_all db_verify will fail since we have a custom
	# hash, so clean up before leaving the test.
	error_check_good bdb_close [$bdb close] 0
	error_check_good hdb_close [$hdb close] 0
	error_check_good env_close [$env close] 0
	env_cleanup $testdir
}

