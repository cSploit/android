# See the file LICENSE for redistribution information.
#
# Copyright (c) 2005, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	test112
# TEST	Test database compaction with a deep tree.
# TEST
# TEST	This is a lot like test111, but with a large number of
# TEST 	entries and a small page size to make the tree deep.
# TEST	To make it simple we use numerical keys all the time.
# TEST
# TEST	Dump and save contents.  Compact the database, dump again,
# TEST	and make sure we still have the same contents.
# TEST  Add back some entries, delete more entries (this time by
# TEST	cursor), dump, compact, and do the before/after check again.

proc test112 { method {nentries 80000} {tnum "112"} args } {
	source ./include.tcl
	global alphabet

	# Compaction is an option for btree, recno, and hash databases.
        if { [is_queue $method] == 1 || [is_heap $method] == 1} {
		puts "Skipping test$tnum for method $method."
		return
	}

        # Skip for specified pagesizes.  This test uses a small
	# pagesize to generate a deep tree.
	set pgindex [lsearch -exact $args "-pagesize"]
	if { $pgindex != -1 } {
		puts "Test$tnum: Skipping for specific pagesizes"
		return
	}

	set args [convert_args $method $args]
	set omethod [convert_method $method]
	if  { [is_partition_callback $args] == 1 } {
		set nodump 1
	} else {
		set nodump 0
	}

	# If we are using an env, then testfile should just be the db name.
	# Otherwise it is the test directory and the name.
	set txnenv 0
	set txn ""
	set eindex [lsearch -exact $args "-env"]
	if { $eindex == -1 } {
		set testfile $testdir/test$tnum.db
		set env NULL
	} else {
		set testfile test$tnum.db
		incr eindex
		set env [lindex $args $eindex]
		set txnenv [is_txnenv $env]
		if { $txnenv == 1 } {
			append args " -auto_commit "
		}
		set testdir [get_home $env]
	}
	puts "Test$tnum: $method ($args) Database compaction with deep tree."

	set t1 $testdir/t1
	set t2 $testdir/t2
	cleanup $testdir $env

	set db [eval {berkdb_open -create\
	    -pagesize 512 -mode 0644} $args $omethod $testfile]
	error_check_good dbopen [is_valid_db $db] TRUE

	if { [is_record_based $method] == 1 } {
		set checkfunc test001_recno.check
	} else {
		set checkfunc test001.check
	}

	puts "\tTest$tnum.a: Populate database."
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	for { set i 1 } { $i <= $nentries } { incr i } {
		set key $i
		set str $i.$alphabet

		set ret [eval \
		    {$db put} $txn {$key [chop_data $method $str]}]
		error_check_good put $ret 0
	}
	if { $txnenv == 1 } {
		error_check_good txn_commit [$t commit] 0
	}
	error_check_good db_sync [$db sync] 0

	if { $env != "NULL" } {
		set testdir [get_home $env]
		set filename $testdir/$testfile
	} else {
		set filename $testfile
	}

	# Record file size, page count, and levels.  They will 
	# be reduced by compaction.
	set size1 [file size $filename]
	set count1 [stat_field $db stat "Page count"]
	set levels [stat_field $db stat "Levels"]
#	error_check_good enough_levels [expr $levels >= 4] 1

	puts "\tTest$tnum.b: Delete most entries from database."
	# Leave every nth item.  Since rrecno renumbers, we
	# delete starting at nentries and working down to 0.
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	for { set i $nentries } { $i > 0 } { incr i -1 } {
		set key $i

		# Leave every n'th item.
		set n 121
		if { [expr $i % $n] != 0 } {
			set ret [eval {$db del} $txn {$key}]
			error_check_good del $ret 0
		}
	}
	if { $txnenv == 1 } {
		error_check_good txn_commit [$t commit] 0
	}
	error_check_good db_sync [$db sync] 0

	puts "\tTest$tnum.c: Do a dump_file on contents."
	dump_file $db "" $t1

	puts "\tTest$tnum.d: Compact database."
	for {set commit 0} {$commit <= $txnenv} {incr commit} {
		if { $txnenv == 1 } {
			set t [$env txn]
			error_check_good txn [is_valid_txn $t $env] TRUE
			set txn "-txn $t"
		}
		if {[catch {eval {$db compact} $txn {-freespace}} ret] } {
			error "FAIL: db compact: $ret"
		}
		if { $txnenv == 1 } {
			if { $commit == 0 } {
				puts "\tTest$tnum.d: Aborting."
				error_check_good txn_abort [$t abort] 0
			} else {
				puts "\tTest$tnum.d: Committing."
				error_check_good txn_commit [$t commit] 0
			}
		}
		error_check_good db_sync [$db sync] 0
		error_check_good verify_dir \
		    [ verify_dir $testdir "" 0 0 $nodump] 0
	}

	set size2 [file size $filename]
	set count2 [stat_field $db stat "Page count"]

	# The on-disk file size should be significantly smaller.
#### We should look at the partitioned files #####
if { [is_partitioned $args] == 0 } {
	set reduction .80
	error_check_good file_size [expr [expr $size1 * $reduction] > $size2] 1
}

	# Also, we should have reduced the number of pages and levels.
	error_check_good page_count_reduced [expr $count1 > $count2] 1
	if { [is_hash $method] == 0 } {
		set newlevels [stat_field $db stat "Levels"]
		error_check_good fewer_levels [expr $newlevels < $levels ] 1
	}

	puts "\tTest$tnum.e: Check that contents are the same after compaction."
	dump_file $db "" $t2
	if { [is_hash $method]  != 0 } {
		filesort $t1 $t1.sort
		filesort $t2 $t2.sort
		error_check_good filecmp [filecmp $t1.sort $t2.sort] 0
	} else {
		error_check_good filecmp [filecmp $t1 $t2] 0
	}

	puts "\tTest$tnum.f: Add more entries to database."
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	for { set i 1 } { $i < $nentries } { incr i } {
		set key $i
		set str $i.$alphabet
		set ret [eval \
		    {$db put} $txn {$key [chop_data $method $str]}]
		error_check_good put $ret 0
	}
	if { $txnenv == 1 } {
		error_check_good txn_commit [$t commit] 0
	}
	error_check_good db_sync [$db sync] 0

	set size3 [file size $filename]
	set count3 [stat_field $db stat "Page count"]

	puts "\tTest$tnum.g: Remove more entries, this time by cursor."
	set i 0
	set n 11
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	set dbc [eval {$db cursor} $txn]

	for { set dbt [$dbc get -first] } { [llength $dbt] > 0 }\
	    { set dbt [$dbc get -next] ; incr i } {
		if { [expr $i % $n] != 0 } {
			error_check_good dbc_del [$dbc del] 0
		}
	}
	error_check_good cursor_close [$dbc close] 0
	if { $txnenv == 1 } {
		error_check_good txn_commit [$t commit] 0
	}
	error_check_good db_sync [$db sync] 0

	puts "\tTest$tnum.h: Save contents."
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	dump_file $db $txn $t1
	if { $txnenv == 1 } {
		error_check_good txn_commit [$t commit] 0
	}

	puts "\tTest$tnum.i: Compact database again."
	for {set commit 0} {$commit <= $txnenv} {incr commit} {
		if { $txnenv == 1 } {
			set t [$env txn]
			error_check_good txn [is_valid_txn $t $env] TRUE
			set txn "-txn $t"
		}
		if {[catch {eval {$db compact} $txn {-freespace}} ret] } {
			error "FAIL: db compact: $ret"
		}
		if { $txnenv == 1 } {
			if { $commit == 0 } {
				puts "\tTest$tnum.d: Aborting."
				error_check_good txn_abort [$t abort] 0 
			} else {
				puts "\tTest$tnum.d: Committing."
				error_check_good txn_commit [$t commit] 0
			}
		}
		error_check_good db_sync [$db sync] 0
		error_check_good verify_dir \
		    [ verify_dir $testdir "" 0 0 $nodump] 0
	}

	set size4 [file size $filename]
	set count4 [stat_field $db stat "Page count"]

#### We should look at the partitioned files #####
if { [is_partitioned $args] == 0 } {
set reduction .9
# puts "$size3 $size4"
	error_check_good file_size [expr [expr $size3 * $reduction] > $size4] 1
}
	
	error_check_good page_count_reduced [expr $count3 > $count4] 1

	puts "\tTest$tnum.j: Check that contents are the same after compaction."
	dump_file $db "" $t2
	if { [is_hash $method]  != 0 } {
		filesort $t1 $t1.sort
		filesort $t2 $t2.sort
		error_check_good filecmp [filecmp $t1.sort $t2.sort] 0
	} else {
		error_check_good filecmp [filecmp $t1 $t2] 0
	}

	error_check_good db_close [$db close] 0
}

