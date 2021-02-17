# See the file LICENSE for redistribution information.
#
# Copyright (c) 2005, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	test114
# TEST	Test database compaction with overflow or duplicate pages.
# TEST
# TEST	Populate a database.  Remove a high proportion of entries.
# TEST	Dump and save contents.  Compact the database, dump again,
# TEST	and make sure we still have the same contents.
# TEST	Add back some entries, delete more entries (this time by
# TEST	cursor), dump, compact, and do the before/after check again.

proc test114 { method {nentries 10000} {tnum "114"} args } {
	source ./include.tcl
	global alphabet

	# Compaction is an option for btree, recno, and hash databases.
        if { [is_queue $method] == 1 || [is_heap $method] == 1 } {
		puts "Skipping compaction test$tnum for method $method."
		return
	}

	# Skip for fixed-length methods because we won't encounter
	# overflows or duplicates.
	if { [is_fixed_length $method] == 1 } {
        	puts "Skipping test$tnum for fixed-length method $method."
        	return
	}

	# We run with a small page size to force overflows.  Skip
	# testing for specified page size.
	set pgindex [lsearch -exact $args "-pagesize"]
	if { $pgindex != -1 } {
		puts "Test$tnum: Skipping for specific pagesize."
		return
	}

	set args [convert_args $method $args]
	set omethod [convert_method $method]
	set npart 0
	set nodump 0
	if  { [is_partition_callback $args] == 1 } {
		set nodump 1
		set partindx [lsearch -exact $args "-partition_callback"]
		set npart [lindex $args [expr $partindx + 1]]
	}
	if { $npart == 0 && [is_partitioned $args] == 1 } {
		set partindx [lsearch -exact $args "-partition"]
		incr partindx
		set partkey [lindex $args $partindx]
		set npart [expr [llength $partkey] + 1]
	}

	# If we are using an env, then testfile should just be the db name.
	# Otherwise it is the test directory and the name.
	set txnenv 0
	set eindex [lsearch -exact $args "-env"]
	if { $eindex == -1 } {
		set basename $testdir/test$tnum
		set env NULL
		append args " -cachesize { 0 10000000 0 }"
	} else {
		set basename test$tnum
		incr eindex
		set env [lindex $args $eindex]
		set txnenv [is_txnenv $env]
		if { $txnenv == 1 } {
			append args " -auto_commit"
			#
			# Cut nentries to 1000 for transactional environment
			# to run the test a bit faster.
			#
			if { $nentries > 1000 } {
				set nentries 1000
			}
		}
		set testdir [get_home $env]
	}

	set t1 $testdir/t1
	set t2 $testdir/t2
	set splitopts { "" "-revsplitoff" }
	set pgtype { "overflow" "unsorted duplicate" "sorted duplicate" }
	set txn ""

	foreach pgt $pgtype {
		if { $pgt != "overflow" } {
			# -dup and -dupsort are only supported by btree
			# and hash. And it is an error to specify -recnum
			# and -dup/-dupsort at the same time.
			if { [is_btree $method] != 1 && \
			    [is_hash $method] != 1 } {
				puts "Skipping $method for compaction\
				    with $pgt since it does not\
				    support duplicates."
				continue
			}

			# Compression requires -dupsort.
			if { $pgt != "sorted duplicate" && \
			    [is_compressed $args] == 1 } {
				puts "Skipping compression for\
				    compaction with $pgt."
				continue
			}
		}

		puts "Test$tnum:\
		    ($method $args) Database compaction with $pgt."
		foreach splitopt $splitopts {
			set testfile $basename.db
			if { $npart != 0 } {
				set partpfx $testdir/__dbp.test${tnum}.db.
			}
			if { $splitopt == "-revsplitoff" } {
				set testfile $basename.rev.db
				if { $npart != 0 } {
					set partpfx \
					    $testdir/__dbp.test${tnum}.rev.db.
				}
				if { [is_btree $omethod] != 1 && \
				    [is_hash $omethod] != 1 && \
				    [is_rbtree $omethod] != 1 } {
					puts "Skipping -revsplitoff\
					    option for method $method."
					continue
				}
			}
			set did [open $dict]
			if { $env != "NULL" } {
				set testdir [get_home $env]
			}

			cleanup $testdir $env
			puts "\tTest$tnum.a:\
			    Create and populate database ($splitopt)."
			set flags $args
			if { $pgt == "unsorted duplicate" } {
				append flags " -dup"
			} elseif { $pgt == "sorted duplicate" } {
				append flags " -dupsort"
			}
			set pagesize 512
			set db [eval {berkdb_open -create -pagesize $pagesize \
			    -mode 0644} $splitopt $flags $omethod $testfile]
			error_check_good dbopen [is_valid_db $db] TRUE

			if { $txnenv == 1 } {
				set t [$env txn]
				error_check_good txn \
				    [is_valid_txn $t $env] TRUE
				set txn "-txn $t"
			}
			#
			# For overflow case, repeat the string 100 times to get
			# a big data and then insert it in to the database
			# so that overflow pages are created. For duplicate
			# case, insert 10 duplicates of each key in order to
			# have off-page duplicates.
			#
			if { $pgt == "overflow" } {
				set start 100
				set end 100
			} else {
				set start 1
				set end 10
			}
			set count 0
			set keycnt 0
			while { [gets $did str] != -1 && $count < $nentries } {
				if { [is_record_based $method] == 1 } {
					set key [expr $keycnt + 1]
				} else {
					set key $str
				}
				for { set i $start } \
				    { $i <= $end && $count < $nentries } \
				    { incr i ; incr count} {
					if { $pgt == "overflow" } {
						set str [repeat $alphabet $i]
					} else {
						set str "${i}.$alphabet"
					}
					set ret [eval {$db put} $txn \
					    {$key [chop_data $method $str]}]
					error_check_good put $ret 0
				}
				incr keycnt
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
			#
			# Check that we have the expected type of pages
			# in the database.
			#
			if { $pgt == "overflow" } {
				set ovf [stat_field $db stat "Overflow pages"]
				error_check_good \
				    overflow_pages [expr $ovf > 0] 1
			} else {
				set dup [stat_field $db stat "Duplicate pages"]
				error_check_good \
				    duplicate_pages [expr $dup > 0] 1
			}
				    
			puts "\tTest$tnum.b:\
			    Delete most entries from database."
			set did [open $dict]
			if { $count != $keycnt } {
				set count [expr $keycnt - 1]
			} else {
				set count [expr $nentries - 1]
			}
			set n 57

			# Leave every nth item.  Since rrecno renumbers, we
			# delete starting at nentries and working down to 0.
			if { $txnenv == 1 } {
				set t [$env txn]
				error_check_good txn \
				    [is_valid_txn $t $env] TRUE
				set txn "-txn $t"
			}
			while { [gets $did str] != -1 && $count >= 0 } {
				if { [is_record_based $method] == 1 } {
					set key [expr $count + 1]
				} else {
					set key $str
				}

				if { [expr $count % $n] != 0 } {
					set ret [eval {$db del} $txn {$key}]
					error_check_good del $ret 0
				}
				incr count -1
			}
			if { $txnenv == 1 } {
				error_check_good t_commit [$t commit] 0
			}
			error_check_good db_sync [$db sync] 0

			#
			# Get the db file size. We should look at the
			# partitioned file if it is a partitioned db.
			#
			set size1 [file size $filename]
			if { $npart != 0 } {
				for { set i 0 } { $i < $npart } { incr i } {
					incr size1 [file size ${partpfx}00${i}]
				}
			}
			set count1 [stat_field $db stat "Page count"]

			# Now that the delete is done we ought to have a 
			# lot of pages on the free list.
			if { [is_hash $method] == 1 } { 
				set free1 [stat_field $db stat "Free pages"]
			} else {
				set free1 \
				    [stat_field $db stat "Pages on freelist"]
			}

			puts "\tTest$tnum.c: Do a dump_file on contents."
			if { $txnenv == 1 } {
				set t [$env txn]
				error_check_good txn \
				    [is_valid_txn $t $env] TRUE
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
					error_check_good txn \
					    [is_valid_txn $t $env] TRUE
					set txn "-txn $t"
				}
				if { [catch {eval {$db compact} \
				    $txn {-freespace}} ret] } {
					error "FAIL: db compact: $ret"
				}
				if { $txnenv == 1 } {
					if { $commit == 0 } {
						puts "\tTest$tnum.d: Aborting."
						error_check_good \
						    txn_abort [$t abort] 0
					} else {
						puts "\tTest$tnum.d: Committing."
						error_check_good \
						    txn_commit [$t commit] 0
					}
				}
				error_check_good db_sync [$db sync] 0
				error_check_good verify_dir \
				    [ verify_dir $testdir "" 0 0 $nodump] 0
			}

			set size2 [file size $filename]
			if { $npart != 0 } {
				for { set i 0 } { $i < $npart } { incr i } {
					incr size2 [file size ${partpfx}00${i}]
				}
			}
			set count2 [stat_field $db stat "Page count"]
			if { [is_hash $method] == 1 } { 
				set free2 [stat_field $db stat "Free pages"]
			} else {
				set free2 \
				    [stat_field $db stat "Pages on freelist"]
			}

			#
			# The file size and the number of pages in the database
			# should never increase. Since only the empty pages
			# in the end of the file can be returned to the file
			# system, the file size and the number of pages may
			# remain the same. In this case, the number of pages in
			# the free list should never decrease.
			#
			error_check_good file_size [expr $size2 <= $size1] 1
			error_check_good page_count [expr $count2 <= $count1] 1
			if { $size2 == $size1 } {
				error_check_good page_count $count2 $count1
				error_check_good pages_returned \
				    [expr $free2 >= $free1] 1
			} else {
				error_check_good page_count \
				    [expr $count2 < $count1] 1
			}

			puts "\tTest$tnum.e:\
			    Contents are the same after compaction."
			if { $txnenv == 1 } {
				set t [$env txn]
				error_check_good txn \
				    [is_valid_txn $t $env] TRUE
				set txn "-txn $t"
			}
			dump_file $db $txn $t2
			if { $txnenv == 1 } {
				error_check_good txn_commit [$t commit] 0
			}

			if { [is_hash $method]  != 0 } {
				filesort $t1 $t1.sort
				filesort $t2 $t2.sort
				error_check_good filecmp \
				    [filecmp $t1.sort $t2.sort] 0
			} else {
				error_check_good filecmp [filecmp $t1 $t2] 0
			}

			puts "\tTest$tnum.f: Add more entries to database."
			# Use integers as keys instead of strings, just to mix
			# it up a little.
			if { $txnenv == 1 } {
				set t [$env txn]
				error_check_good txn \
				    [is_valid_txn $t $env] TRUE
				set txn "-txn $t"
			}
			set count 1
			set keycnt 1
			while { $count <= $nentries } {
				set key $keycnt
				for { set i $start } \
				    { $i <= $end && $count <= $nentries } \
				    { incr i ; incr count} {
					if { $pgt == "overflow" } {
						set str [repeat $alphabet $i]
					} else {
						set str "${i}.$alphabet"
					}
					set ret [eval {$db put} $txn \
					    {$key [chop_data $method $str]}]
					error_check_good put $ret 0
				}
				incr keycnt
			}
			if { $txnenv == 1 } {
				error_check_good t_commit [$t commit] 0
			}
			error_check_good db_sync [$db sync] 0
			close $did

			#
			# Check that we have the expected type of pages
			# in the database.
			#
			if { $pgt == "overflow" } {
				set ovf [stat_field $db stat "Overflow pages"]
				error_check_good \
				    overflow_pages [expr $ovf > 0] 1
			} else {
				set dup [stat_field $db stat "Duplicate pages"]
				error_check_good \
				    duplicate_pages [expr $dup > 0] 1
			}

			puts "\tTest$tnum.g:\
			    Remove more entries, this time by cursor."
			set count 0
			if { $txnenv == 1 } {
				set t [$env txn]
				error_check_good txn \
				    [is_valid_txn $t $env] TRUE
				set txn "-txn $t"
			}
			set dbc [eval {$db cursor} $txn]

			# Leave every nth item.
			for { set dbt [$dbc get -first] } \
			    { [llength $dbt] > 0 } \
			    { set dbt [$dbc get -next] ; incr count } {
				if { [expr $count % $n] != 0 } {
					error_check_good dbc_del [$dbc del] 0
				}
			}

			error_check_good cursor_close [$dbc close] 0
			if { $txnenv == 1 } {
				error_check_good t_commit [$t commit] 0
			}
			error_check_good db_sync [$db sync] 0

			set size3 [file size $filename]
			if { $npart != 0 } {
				for { set i 0 } { $i < $npart } { incr i } {
					incr size3 [file size ${partpfx}00${i}]
				}
			}
			set count3 [stat_field $db stat "Page count"]
			if { [is_hash $method] == 1 } { 
				set free3 [stat_field $db stat "Free pages"]
			} else {
				set free3 \
				    [stat_field $db stat "Pages on freelist"]
			}

			puts "\tTest$tnum.h: Save contents."
			if { $txnenv == 1 } {
				set t [$env txn]
				error_check_good txn \
				    [is_valid_txn $t $env] TRUE
				set txn "-txn $t"
			}
			dump_file $db $txn $t1
			if { $txnenv == 1 } {
				error_check_good t_commit [$t commit] 0
			}

			puts "\tTest$tnum.i:\
			    Compact and verify database again."
			for {set commit 0} {$commit <= $txnenv} {incr commit} {
				if { $txnenv == 1 } {
					set t [$env txn]
					error_check_good txn \
					    [is_valid_txn $t $env] TRUE
					set txn "-txn $t"
				}
				if { [catch {eval \
				    {$db compact} $txn {-freespace}} ret] } {
					error "FAIL: db compact: $ret"
				}
				if { $txnenv == 1 } {
					if { $commit == 0 } {
						puts "\tTest$tnum.i: Aborting."
						error_check_good \
						    txn_abort [$t abort] 0
					} else {
						puts "\tTest$tnum.i: Committing."
						error_check_good \
						    txn_commit [$t commit] 0
					}
				}
				error_check_good db_sync [$db sync] 0
				error_check_good verify_dir \
				    [ verify_dir $testdir "" 0 0 $nodump] 0
			}

			set size4 [file size $filename]
			if { $npart != 0 } {
				for { set i 0 } { $i < $npart } { incr i } {
					incr size3 [file size ${partpfx}00${i}]
				}
			}
			set count4 [stat_field $db stat "Page count"]
			if { [is_hash $method] == 1 } { 
				set free4 [stat_field $db stat "Free pages"]
			} else {
				set free4 \
				    [stat_field $db stat "Pages on freelist"]
			}

			error_check_good file_size [expr $size4 <= $size3] 1
			error_check_good page_count [expr $count4 <= $count3] 1
			if { $size4 == $size3 } {
				error_check_good page_count $count4 $count3
				error_check_good pages_returned \
				    [expr $free4 >= $free3] 1
			} else {
				error_check_good page_count \
				    [expr $count4 < $count3] 1
			}

			puts "\tTest$tnum.j:\
			    Contents are the same after compaction."
			if { $txnenv == 1 } {
				set t [$env txn]
				error_check_good txn \
				    [is_valid_txn $t $env] TRUE
				set txn "-txn $t"
			}
			dump_file $db $txn $t2
			if { $txnenv == 1 } {
				error_check_good t_commit [$t commit] 0
			}
			if { [is_hash $method]  != 0 } {
				filesort $t1 $t1.sort
				filesort $t2 $t2.sort
				error_check_good filecmp \
				    [filecmp $t1.sort $t2.sort] 0
			} else {
				error_check_good filecmp [filecmp $t1 $t2] 0
			}

			error_check_good db_close [$db close] 0
		}
	}
}
