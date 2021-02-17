# See the file LICENSE for redistribution information.
#
# Copyright (c) 2005, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	test117
# TEST	Test database compaction with requested fill percent or specified
# TEST	number of pages to free.
# TEST
# TEST	Populate a database.  Remove a high proportion of entries.
# TEST	Dump and save contents. Compact the database with the following
# TEST	configurations.
# TEST	1) Compact with requested fill percentages, starting at 10% and
# TEST	working our way up to 100.
# TEST	2) Compact the database 4 times with -pages option and each time
# TEST	try to compact 1/4 of the original database pages.
# TEST
# TEST	On each compaction, make sure we still have the same contents.
# TEST
# TEST	Unlike the other compaction tests, this one does not
# TEST	use -freespace.

proc test117 { method {nentries 10000} {tnum "117"} args } {
	source ./include.tcl

	# Compaction is supported by btree, hash and recno.
        if { [is_queue $method] == 1 || [is_heap $method] == 1 } {
		puts "Skipping test$tnum for method $method."
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
	puts "Test$tnum: ($method $args) Database compaction with\
	    with specified fillpercent or number of pages."
	set t1 $testdir/t1
	set t2 $testdir/t2
	set splitopts { "" "-revsplitoff" }
	set txn ""

	set compactopts { "-fillpercent" "-pages" }
	foreach compactopt $compactopts {
		#
		# Compaction using a requested fill percentage is
		# an option for btree and recno databases only.
		#
		if { [is_hash $method] == 1 && $compactopt == "-fillpercent"} {
			puts "Skipping -fillpercent option for method $method."
			continue
		}

		foreach splitopt $splitopts {
			set testfile $basename.db
			if { $splitopt == "-revsplitoff" } {
				set testfile $basename.rev.db
		 		if { [is_record_based $method] == 1 } {
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

			puts "\tTest$tnum.a: Create and\
			    populate database ($splitopt)."
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
				global kvals

				if { [is_record_based $method] == 1 } {
					set key [expr $count + 1]
					set kvals($key) [pad_data $method $str]
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
				set txn ""
			}
			if { [is_record_based $method] == 1 } {
				set pkey [expr $count + 1]
				set kvals($pkey) [pad_data $method $str]
			} else {
				set pkey $str
			}
			set pdata [chop_data $method $str]
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

			#
			# Leave every nth item.  Since rrecno renumbers, we
			# delete starting at nentries and working down to 0.
			#
			if { $txnenv == 1 } {
				set t [$env txn]
				error_check_good txn \
				    [is_valid_txn $t $env] TRUE
				set txn "-txn $t"
			}
			while { [gets $did str] != -1 && $count > 0 } {
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
				set txn ""
			}
			error_check_good db_sync [$db sync] 0

			#
			# Get the file size after deleting the items.
			# In some cases with compression enabled, the file may
			# grow somewhat while the deletes are performed.
			# The file will still shrink overall after compacting.
			# [#17402]
			#
			set size1 [file size $filename]
			set count1 [stat_field $db stat "Page count"]
			set internal1 [stat_field $db stat "Internal pages"]
			set leaf1 [stat_field $db stat "Leaf pages"]
			set in_use1 [expr $internal1 + $leaf1]
			set free1 [stat_field $db stat "Pages on freelist"]

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
				set txn ""
			}

			# Set up the compaction option value.
			if { $compactopt == "-fillpercent" } {
				#
				# For fill percentages, start at 10% and
				# work up to 100%.
				#
				set start 10
				set end 100
				set inc 10
			} elseif { $compactopt == "-pages" } {
				#
				# For the number of pages to free, compact
				# the database 4 times and each time try
				# to compact 1/4 of the original database
				# pages.
				#
				if { $count1 < 4 } {
					set start 1
				} else {
					set start [expr $count1 / 4]
				}
				set end $start
				set inc 0
				set count 0
			} else {
				error "FAIL:\
				    unrecognized compact option $compactopt"
			}

			for { set optval $start } { $optval <= $end }\
			    { incr optval $inc } {

				puts "\tTest$tnum.d: Compact and verify\
				    database $compactopt $optval."

				if { [catch {eval {$db compact} \
				    $compactopt $optval} ret] } {
					error "FAIL: db compact\
					    $compactopt $optval: $ret"
				}

				error_check_good db_sync [$db sync] 0
				set size2 [file size $filename]
				error_check_good verify_dir \
				     [verify_dir $testdir "" 0 0 $nodump] 0
				set count2 [stat_field $db stat "Page count"]
				set internal2 \
				    [stat_field $db stat "Internal pages"]
				set leaf2 [stat_field $db stat "Leaf pages"]
				set free2 \
				    [stat_field $db stat "Pages on freelist"]

				#
				# The page count and file size should never
				# increase.
				#
				error_check_good page_count \
				    [expr $count2 <= $count1] 1
				error_check_good file_size \
				    [expr $size2 <= $size1] 1

				#
				# Pages in use (leaf + internal) should never
				# increase; pages on free list should never
				# decrease.
				#
				set in_use2 [expr $internal2 + $leaf2]
				error_check_good pages_in_use \
				    [expr $in_use2 <= $in_use1] 1
				error_check_good pages_on_freelist \
				    [expr $free2 >= $free1] 1

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
					error_check_good txn_commit \
					    [$t commit] 0
				}
				if { [is_hash $method] == 1 } {
					filesort $t1 $t1.sort
					filesort $t2 $t2.sort
					error_check_good filecmp \
					    [filecmp $t1.sort $t2.sort] 0
				} else {
					error_check_good filecmp \
					    [filecmp $t1 $t2] 0
				}

				if { $compactopt == "-pages" } {
					incr count
					if { $count >= 4 } {
						break
					}
				}

				#
				# Reset original values to the post-compaction
				# number for the next pass.
				#
				set count1 $count2
				set free1 $free2
				set size1 $size2
				set in_use1 $in_use2
			}
			error_check_good db_close [$db close] 0
			close $did
		}
	}
}
