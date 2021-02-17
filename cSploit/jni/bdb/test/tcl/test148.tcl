# See the file LICENSE for redistribution information.
#
# Copyright (c) 2013, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	test148
# TEST	Test database compaction with -freeonly, -start/-stop.
# TEST
# TEST	Populate a database. Remove a high proportion of entries.
# TEST	Dump and save contents.  Compact the database with -freeonly,
# TEST	-start, -stop, or -start/-stop, dump again, and make sure
# TEST	we still have the same contents.

proc test148 { method {nentries 10000} {tnum "148"} args } {
	source ./include.tcl

	global rand_init
	error_check_good set_random_seed [berkdb srand $rand_init] 0

	# Compaction is supported by btree, recno and hash.
	if { [is_heap $method] == 1 || [is_queue $method] == 1 } {
		puts "Skipping test$tnum for method $method."
		return
	}

	set args [convert_args $method $args]
	set omethod [convert_method $method]
	set npart 0
	set nodump 0
	if  { [is_partition_callback $args] == 1 } {
		set nodump 1
		if { [is_hash $method] == 1 } {
			set indx [lsearch -exact $args "-partition_callback"]
			incr indx
			set npart [lindex $args $indx]
		}
	}

	if { [is_partitioned $args] ==1 && $npart == 0 } {
		set indx [lsearch -exact $args "-partition"]
		incr indx
		set partkey [lindex $args $indx]
		set npart [expr [llength $partkey] + 1]
	}

	#
	# If we are using an env, then testfile should just be the db name.
	# Otherwise it is the test directory and the name.
	#
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

	puts "Test$tnum: ($method $args) Database compaction with\
	    -freeonly, -start/-stop."

	set t1 $testdir/t1
	set t2 $testdir/t2
	set opts { "-freeonly" "-start" "-stop" "-start/stop" "-stop/start" }
	set splitopts { "" "-revsplitoff" }
	set txn ""

	foreach opt $opts {
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

			puts -nonewline "\tTest$tnum.a:\
			    Create and populate database"
			puts " using opt $opt and splitopt $splitopt."

			set db [eval {berkdb_open -create \
			    -mode 0644} $splitopt $args $omethod $testfile]
			error_check_good dbopen [is_valid_db $db] TRUE

			set count 0
			if { $txnenv == 1 } {
				set t [$env txn]
				error_check_good txn \
				    [is_valid_txn $t $env] TRUE
				set txn "-txn $t"
			}
			while { [gets $did str] != -1 && $count < $nentries } {
				if { [is_record_based $method] == 1 } {
					set key [expr $count + 1]
				} else {
					set key $str
					set str [reverse $str]
				}

				set ret [eval {$db put} $txn \
				    {$key [chop_data $method $str]}]
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

			puts "\tTest$tnum.b:\
			    Delete most entries from database."
			set did [open $dict]
			set count [expr $nentries - 1]
			set n 17

			if { $txnenv == 1 } {
				set t [$env txn]
				error_check_good txn \
				    [is_valid_txn $t $env] TRUE
				set txn "-txn $t"
			}

			#
			# Pick 2 numbers between 0 and $count.
			# For rrecno, deleting a record will shift the records
			# with recno bigger than it forward. So pick the
			# random numbers after delete.
			#
			if { [is_rrecno $method] != 1 } {
				set startcnt [berkdb random_int 1 $count]
				set stopcnt [berkdb random_int 1 $count]
			} else {
				set delcnt 0
				set origcnt $count
			}
			set startkey ""
			set stopkey ""
			#
			# Since rrecno renumbers, we delete starting at
			# nentries and working down to 0.
			#
			while { [gets $did str] != -1 && $count > 0 } {
				if { [is_record_based $method] == 1 } {
					set key [expr $count + 1]
				} else {
					set key $str
				}

				if { [expr $count % $n] != 0 } {
					set ret [eval {$db del} $txn {$key}]
					error_check_good del $ret 0
					if { [is_rrecno $method] == 1 } {
						incr delcnt
					}
				}

				#
				# Set the option value of -start/stop for
				# btree.
				#
				if { [is_btree $method] == 1 || \
				    [is_rbtree $method] == 1} {
					if { $startkey == "" || \
					    $count == $startcnt } {
						set startkey $key
					}
					if { $stopkey == "" || \
					    $count == $stopcnt } {
						set stopkey $key
					}
				}

				incr count -1
			}
			if { $txnenv == 1 } {
				error_check_good t_commit [$t commit] 0
			}
			error_check_good db_sync [$db sync] 0
			close $did

			#
			# Make sure startkey <= stopkey since we will reverse
			# the option value of -start/-stop for testing.
			#
			if { [is_record_based $method] == 1 } {
				if { [is_rrecno $method] == 1 } {
					set startcnt [berkdb random_int 1 \
					    [expr $origcnt - $delcnt]]
					set stopcnt [berkdb random_int 1 \
					    [expr $origcnt - $delcnt]]
				}
				set startkey $startcnt
				set stopkey $stopcnt
				if { $startkey > $stopkey } {
					set key $startkey
					set startkey $stopkey
					set stopkey $key
				}
			} elseif { [is_hash $method] != 1 } {
				if { [string compare $startkey $stopkey] > 0} {
					set key $startkey
					set startkey $stopkey
					set stopkey $key
				}
			}

			#
			# For hash method, pick 2 random numbers between
			# the smallest and biggest hash bucket numbers as
			# the option value passed to -start/-stop.
			#
			if { [is_hash $method] == 1} {
				set startkey 0
				set stopkey [expr \
				    [stat_field $db stat "Buckets"] - 1]
				#
				# For partitioned database, each partition
				# maintains the hash bucket number on its own.
				#
				if { [is_partition_callback $args] == 1 } {
					set stopkey \
					    [expr $stopkey / $npart / 2]
					if { $stopkey == 0 } {
						set stopkey 1
					}
				}

				set startcnt \
				    [berkdb random_int $startkey $stopkey]
				set stopcnt \
				    [berkdb random_int $startkey $stopkey]
				#
				# Make sure startkey <= stopkey since we will
				# reverse the option value of -start/-stop
				# for testing.
				#
				if { $startcnt < $stopcnt } {
					set startkey $startcnt
					set stopkey $stopcnt
				} else {
					set startkey $stopcnt
					set stopkey $startcnt
				}
			}

			# Get the file size after deleting the items.
			set size1 [file size $filename]
			if { $npart != 0 } {
				for { set i 0 } { $i < $npart } { incr i } {
					incr size1 [file size ${partpfx}00${i}]
				}
			}
			set count1 [stat_field $db stat "Page count"]
			set internal1 [stat_field $db stat "Internal pages"]
			set leaf1 [stat_field $db stat "Leaf pages"]
			set in_use1 [expr $internal1 + $leaf1]

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

			if { $opt == "-freeonly" } {
				set flags $opt
			} elseif { $opt == "-start" } {
				set flags "-start $startkey"
			} elseif { $opt == "-stop" } {
				set flags "-stop $stopkey"
			} elseif { $opt == "-start/stop" } {
				set flags "-start $startkey -stop $stopkey"
			} else {
				set flags "-start $stopkey -stop $startkey"
			}

			puts "\tTest$tnum.d: Compact database $flags."

			if {[catch {eval {$db compact} $flags} ret] } {
				error "FAIL: db compact $flags: $ret"
			}

			error_check_good db_sync [$db sync] 0
			set size2 [file size $filename]
			if { $npart != 0} {
				for { set i 0 } { $i < $npart } { incr i } {
					incr size2 [file size ${partpfx}00${i}]
				}
			}
			error_check_good verify_dir \
			     [verify_dir $testdir "" 0 0 $nodump] 0
			set count2 [stat_field $db stat "Page count"]
			set internal2 [stat_field $db stat "Internal pages"]
			set leaf2 [stat_field $db stat "Leaf pages"]

			# The page count and file size should never increase.
			error_check_good page_count [expr $count2 <= $count1] 1
			error_check_good file_size [expr $size2 <= $size1] 1

			# Pages in use (leaf + internal) should never increase.
			set in_use2 [expr $internal2 + $leaf2]
			error_check_good pages_in_use \
			    [expr $in_use2 <= $in_use1] 1

			#
			# When -freeonly is used, no page compaction is done
			# but only the freed pages in the end of file are
			# returned to the file system.
			#
			set examined \
			    [stat_field $db compact_stat "Pages examined"]
			if { $opt == "-freeonly" } {
				error_check_good pages_examined $examined 0
			} elseif { $opt == "-stop/start" } {
				error_check_good pages_examined \
				    [expr $examined >= 0] 1
			} else {
				error_check_good pages_examined \
				    [expr $examined > 0] 1
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
			if { [is_hash $method] == 1 } {
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
