# See the file LICENSE for redistribution information.
#
# Copyright (c) 2005, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	test111
# TEST	Test database compaction.
# TEST
# TEST	Populate a database.  Remove a high proportion of entries.
# TEST	Dump and save contents.  Compact the database, dump again,
# TEST	and make sure we still have the same contents.
# TEST  Add back some entries, delete more entries (this time by
# TEST	cursor), dump, compact, and do the before/after check again.

proc test111 { method {nentries 10000} {tnum "111"} args } {

	# Compaction is an option for btree, recno, and hash databases.
        if { [is_queue $method] == 1 || [is_heap $method] == 1} {
		puts "Skipping test$tnum for method $method."
		return
	}

	# If a page size was specified, find out what it is.  Pages
	# might not be freed in the case of really large pages (64K)
	# but we still want to run this test just to make sure
	# nothing funny happens.
	set pagesize 0
        set pgindex [lsearch -exact $args "-pagesize"]
        if { $pgindex != -1 } {
                incr pgindex
		set pagesize [lindex $args $pgindex]
        }

	source ./include.tcl
	global rand_init
	error_check_good set_random_seed [berkdb srand $rand_init] 0
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
	set eindex [lsearch -exact $args "-env"]
	if { $eindex == -1 } {
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
	puts "Test$tnum: ($method $args) Database compaction."

	set t1 $testdir/t1
	set t2 $testdir/t2
	set splitopts { "" "-revsplitoff" }
	set txn ""

	if { [is_record_based $method] == 1 } {
		set checkfunc test001_recno.check
	} else {
		set checkfunc test001.check
	}

	foreach splitopt $splitopts {
		set testfile $basename.db
		if { $splitopt == "-revsplitoff" } {
			set testfile $basename.rev.db
	 		if { [is_record_based $method] == 1 } {
				puts "Skipping\
				    -revsplitoff option for method $method."
				continue
			}
		}
		set did [open $dict]
		if { $env != "NULL" } {
			set testdir [get_home $env]
		}
		cleanup $testdir $env

		puts "\tTest$tnum.a: Create and populate database ($splitopt)."
		set db [eval {berkdb_open -create \
		    -mode 0644} $splitopt $args $omethod $testfile]
		error_check_good dbopen [is_valid_db $db] TRUE

		set count 0
		if { $txnenv == 1 } {
			set t [$env txn]
			error_check_good txn [is_valid_txn $t $env] TRUE
			set txn "-txn $t"
		}
		while { [gets $did str] != -1 && $count < $nentries } {
			global kvals

			if { [is_record_based $method] == 1 } {
				set key [expr $count + 1]
				set kvals($key) [pad_data $method $str]
			} else {
				set key $str
				set str [reverse $str]
			}

			set ret [eval \
			    {$db put} $txn {$key [chop_data $method $str]}]
			error_check_good put $ret 0
			incr count

		}
		if { $txnenv == 1 } {
			error_check_good txn_commit [$t commit] 0
		}
		close $did
		error_check_good db_sync [$db sync] 0

		if { $env != "NULL" } {
			set testdir [get_home $env]
			set filename $testdir/$testfile
		} else {
			set filename $testfile
		}

		# Record the file size and page count.  Both will
		# be reduced by compaction.
		set size1 [file size $filename]
		set count1 [stat_field $db stat "Page count"]

		# Delete between 1 and maxdelete items, then skip over between
		# 1 and maxskip items.  This is to make the data bunchy,
		# so we sometimes follow the code path where merging is
		# done record by record, and sometimes the path where
		# the whole page is merged at once.

		puts "\tTest$tnum.b: Delete most entries from database."
		set did [open $dict]
		set count [expr $nentries - 1]
		set maxskip 4
		set maxdelete 48

		# Since rrecno and rbtree renumber, we delete starting at
		# nentries and working down to 0.
		if { $txnenv == 1 } {
			set t [$env txn]
			error_check_good txn [is_valid_txn $t $env] TRUE
			set txn "-txn $t"
		}
		while { [gets $did str] != -1 && $count > 0 } {

			# Delete a random number of successive items.
			set ndeletes [berkdb random_int 1 $maxdelete]
			set target [expr $count - $ndeletes]
			while { [expr $count > $target] && $count > 0 } {
				if { [is_record_based $method] == 1 } {
					set key [expr $count + 1]
				} else {
					set key [gets $did]
				}

				set ret [eval {$db del} $txn {$key}]
				error_check_good del $ret 0
				incr count -1
			}
			# Skip over a random smaller number of items.
			set skip [berkdb random_int 1 [expr $maxskip]]
			set target [expr $count - $skip]
			while { [expr $count > $target] && $count > 0 } {
				incr count -1
			}
		}
		if { $txnenv == 1 } {
			error_check_good t_commit [$t commit] 0
		}
		error_check_good db_sync [$db sync] 0

		puts "\tTest$tnum.c: Do a dump_file on contents."
		if { $txnenv == 1 } {
			set t [$env txn]
			error_check_good txn [is_valid_txn $t $env] TRUE
			set txn "-txn $t"
		}
		dump_file $db $txn $t1
		if { $txnenv == 1 } {
			error_check_good txn_commit [$t commit] 0
		}

		puts "\tTest$tnum.d: Compact and verify database."
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
			    [verify_dir $testdir "" 0 0 $nodump ] 0
		}

		set size2 [file size $filename]
		set count2 [stat_field $db stat "Page count"]

		# Now check for reduction in page count and file size.
		error_check_good pages_freed [expr $count1 > $count2] 1

	#### We should look at the partitioned files #####
	if { [is_partitioned $args] == 0 } {
		set reduction .96
		error_check_good \
		    file_size [expr [expr $size1 * $reduction] > $size2] 1
	}

		puts "\tTest$tnum.e: Contents are the same after compaction."
		if { $txnenv == 1 } {
			set t [$env txn]
			error_check_good txn [is_valid_txn $t $env] TRUE
			set txn "-txn $t"
		}
		dump_file $db $txn $t2
		if { $txnenv == 1 } {
			error_check_good txn_commit [$t commit] 0
		}

		if { [is_hash $method] == 1 } {
			filesort $t1 $t1.sort
			filesort $t2 $t2.sort
			error_check_good filecmp [filecmp $t1.sort $t2.sort] 0
		} else {
			error_check_good filecmp [filecmp $t1 $t2] 0
		}

		puts "\tTest$tnum.f: Add more entries to database."
		# Use integers as keys instead of strings, just to mix it up
		# a little.
		if { $txnenv == 1 } {
			set t [$env txn]
			error_check_good txn [is_valid_txn $t $env] TRUE
			set txn "-txn $t"
		}
		for { set i 1 } { $i < $nentries } { incr i } {
			set key $i
			set str $i
			set ret [eval \
			    {$db put} $txn {$key [chop_data $method $str]}]
			error_check_good put $ret 0
		}
		if { $txnenv == 1 } {
			error_check_good t_commit [$t commit] 0
		}
		error_check_good db_sync [$db sync] 0

		set size3 [file size $filename]
		set count3 [stat_field $db stat "Page count"]

		puts "\tTest$tnum.g: Remove more entries, this time by cursor."
		set count 0
		if { $txnenv == 1 } {
			set t [$env txn]
			error_check_good txn [is_valid_txn $t $env] TRUE
			set txn "-txn $t"
		}
		set dbc [eval {$db cursor} $txn]

		# Delete all items except those evenly divisible by
		# $maxdelete -- so the db is nearly empty.
		for { set dbt [$dbc get -first] } { [llength $dbt] > 0 }\
		    { set dbt [$dbc get -next] ; incr count } {
			if { [expr $count % $maxdelete] != 0 } {
				error_check_good dbc_del [$dbc del] 0
			}
		}

		error_check_good cursor_close [$dbc close] 0
		if { $txnenv == 1 } {
			error_check_good t_commit [$t commit] 0
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
			error_check_good t_commit [$t commit] 0
		}

		puts "\tTest$tnum.i: Compact and verify database again."
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
					puts "\tTest$tnum.i1: Aborting."
					error_check_good txn_abort [$t abort] 0
				} else {
					puts "\tTest$tnum.i2: Committing."
					error_check_good txn_commit [$t commit] 0
				}
			}
			error_check_good db_sync [$db sync] 0
			error_check_good verify_dir \
			    [verify_dir $testdir "" 0 0 $nodump ] 0
		}

		set size4 [file size $filename]
		set count4 [stat_field $db stat "Page count"]

		# Check for page count and file size reduction.
		#
		# Identify cases where we don't expect much reduction in
		# size, for example hash with large pagesizes.
		#
		#### We should look at the partitioned files #####
		set test_filesize 1
		if { [is_partitioned $args] } {
			set test_filesize 0
		}
		if { [is_hash $method] && $pagesize == 65536 } {
			set test_filesize 0
		}

		# Test for reduced file size where expected.  In cases where
		# we don't expect much (if any) reduction, verify that at
		# least the file size hasn't increased. 
		if { $test_filesize == 1 } {
			error_check_good file_size_reduced \
			    [expr [expr $size3 * $reduction] > $size4] 1
			error_check_good pages_freed [expr $count3 > $count4] 1
		} else {
			error_check_good file_size [expr $size3 >= $size4] 1
			error_check_good pages_freed [expr $count3 >= $count4] 1
		}

		puts "\tTest$tnum.j: Contents are the same after compaction."
		if { $txnenv == 1 } {
			set t [$env txn]
			error_check_good txn [is_valid_txn $t $env] TRUE
			set txn "-txn $t"
		}
		dump_file $db $txn $t2
		if { $txnenv == 1 } {
			error_check_good t_commit [$t commit] 0
		}
		if { [is_hash $method] == 1 } {
			filesort $t1 $t1.sort
			filesort $t2 $t2.sort
			error_check_good filecmp [filecmp $t1.sort $t2.sort] 0
		} else {
			error_check_good filecmp [filecmp $t1 $t2] 0
		}

		error_check_good db_close [$db close] 0
		close $did
	}
}
