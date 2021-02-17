# See the file LICENSE for redistribution information.
#
# Copyright (c) 2010, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	test130
# TEST	Test moving of subdatabase metadata pages.
# TEST
# TEST	Populate num_db sub-database. Open multiple handles on each.
# TEST  Remove a high proportion of entries.
# TEST	Dump and save contents.  Compact the database, dump again,
# TEST	and make sure we still have the same contents.
# TEST  Make sure handles and cursors still work after compaction.

proc test130 { method {nentries 10000} {num_db 3} {tnum "130"} args } {

	# Compaction is an option for btree, recno, and hash databases.
	if { [is_queue $method] == 1 } {
		puts "Skipping test$tnum for method $method."
		return
	}

	# Heap cannot have subdatabases
	if { [is_heap $method] == 1 } {
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

	# This test must have an environment because of sub-databases.
	set txnenv 0
	set eindex [lsearch -exact $args "-env"]
	if { $eindex == -1 } {
		puts "Skipping test$tnum, needs environment"
		return
	} else {
		set basename test$tnum
		incr eindex
		set env [lindex $args $eindex]
		set txnenv [is_txnenv $env]
		if { $txnenv == 1 } {
			append args " -auto_commit "
		}
		set testdir [get_home $env]
		set nhandles 3
	}
	puts "Test$tnum: ($method $args) Database compaction."

	set t1 $testdir/t1
	if { [is_record_based $method] == 1 } {
		set splitopt ""
		set special [expr $nentries + 10]
	} else {
		set splitopt "-revsplitoff"
		set special "special"
	}
	set txn ""

	cleanup $testdir $env

	# Create num_db sub databases and fill them with data.
	for {set i 0} {$i < $num_db} {incr i} {
		set testfile $basename.db
		set did [open $dict]
		if { $env != "NULL" } {
			set testdir [get_home $env]
		}

		puts "\tTest$tnum.a: Create and populate database sub$i ($splitopt)."
		set db(0,$i) [eval {berkdb_open -create \
		    -mode 0644} $splitopt $args $omethod {$testfile sub$i}]
		error_check_good dbopen [is_valid_db $db(0,$i)] TRUE

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
			    {$db(0,$i) put} $txn {$key [chop_data $method $str]}]
			error_check_good put $ret 0
			incr count
		}

		if { $txnenv == 1 } {
			error_check_good txn_commit [$t commit] 0
		}
		close $did
		set ret [eval {$db(0,$i) put $special [chop_data $method "data"]} ]
		error_check_good put $ret 0
		error_check_good db_sync [$db(0,$i) sync] 0
		
		# Open a couple of cursors on this handle.
		set dbc(0,$i,0) [eval {$db(0,$i) cursor } ]
		error_check_good dbc \
		     [is_valid_cursor $dbc(0,$i,0) $db(0,$i)] TRUE
		set dbc(0,$i,1) [eval {$db(0,$i) cursor } ]
		error_check_good dbc \
		     [is_valid_cursor $dbc(0,$i,1) $db(0,$i)] TRUE

		# Open nhandles on each sub database. Each with some cursors
		for {set j 1 } {$j < $nhandles} {incr j} {
			set db($j,$i) [eval berkdb_open $args $testfile sub$i]
			error_check_good db [is_valid_db $db($j,$i)] TRUE
			set dbc($j,$i,0) [eval {$db($j,$i) cursor } ]
			error_check_good dbc \
			     [is_valid_cursor $dbc($j,$i,0) $db($j,$i)] TRUE
			set dbc($j,$i,1) [eval {$db($j,$i) cursor } ]
			error_check_good dbc \
			     [is_valid_cursor $dbc($j,$i,1) $db($j,$i)] TRUE
		}

	}

	set testdir [get_home $env]
	set filename $testdir/$testfile

	# Delete between 1 and maxdelete items, then skip over between
	# 1 and maxskip items.  This is to make the data bunchy,
	# so we sometimes follow the code path where merging is
	# done record by record, and sometimes the path where
	# the whole page is merged at once.

	puts "\tTest$tnum.b: Delete most entries from each database."
	set did [open $dict]
	set count [expr $nentries - 1]
	set maxskip 4
	set maxdelete 48

	# Since rrecno and rbtree renumber, we delete starting at
	# nentries and working down to 0.
	for {set i 0} {$i < $num_db} {incr i} {
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

				set ret [eval {$db(0,$i) del} $txn {$key}]
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
		error_check_good db_sync [$db(0,$i) sync] 0

		puts "\tTest$tnum.c: Do a dump_file on contents."
		if { $txnenv == 1 } {
			set t [$env txn]
			error_check_good txn [is_valid_txn $t $env] TRUE
			set txn "-txn $t"
		}
		dump_file $db(0,$i) $txn $t1
		if { $txnenv == 1 } {
			error_check_good txn_commit [$t commit] 0
		}

		puts "\tTest$tnum.d: Compact and verify databases"
		for {set commit 0} {$commit <= $txnenv} {incr commit} {
			if { $txnenv == 1 } {
				set t [$env txn]
				error_check_good txn [is_valid_txn $t $env] TRUE
				set txn "-txn $t"

				# Open a cursor in this transaction -- compact 
				# should complain.
				set txncursor(0,$i) \
				    [eval {$db(0,$i) cursor} $txn]
				set ret [catch {eval {$db(0,$i) compact} \
				    $txn {-freespace}} res]
				error_check_good no_txn_cursor $ret 1
				error_check_good txn_cursor_close \
				    [$txncursor(0,$i) close] 0
			}

			# Now compact for real.
			set orig_size [file size $filename]
			if {[catch {eval {$db(0,$i) compact} \
			    $txn {-freespace}} ret] } {
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
			error_check_good db_sync [$db(0,$i) sync] 0
			error_check_good verify_dir \
			    [verify_dir $testdir "" 0 0 $nodump ] 0
			#
			# The compaction of subdb$i with i < (numdb - 1) is not
			# expected to reduce the file size because the last
			# page of the file is owned by subdb${num_db-1}.
			#
			set after_compact_size [file size $filename]
			if { $i < [expr $num_db - 1] || 
			    ($txnenv == 1 && $commit == 0) } {
				error_check_good file_size \
				    [expr $orig_size == $after_compact_size] 1
			} else {
				error_check_good file_size \
				    [expr $orig_size >= $after_compact_size] 1
			}
		}
	}

	# See that the handles and cursors can get data.
	# Brute force check that the handle locks are right, try to remove
	# the database without first closing each handle.  Skip the 
	# cursor and db handle checks for rrecno -- the deletes will 
	# have changed the value of $special.
	set paddeddata [pad_data $method "data"]
	for {set i 0} {$i < $num_db} {incr i} {
		puts "\tTest$tnum.e: \
		     See that the handle $i and cursors still work."
		for {set j 0} {$j < $nhandles} {incr j} {
			if { ![is_rrecno $method] } {
				set ret [eval {$db($j,$i) get $special } ]
				error_check_good handle$i \
				     [lindex [lindex $ret 0] 0] $special
				error_check_good handle$i \
				     [lindex [lindex $ret 0] 1] $paddeddata
				set ret [eval {$dbc($j,$i,0) get -set $special } ]
				error_check_good handle$i \
				     [lindex [lindex $ret 0] 0] $special
				error_check_good handle$i \
				     [lindex [lindex $ret 0] 1] $paddeddata
				set ret [eval {$dbc($j,$i,1) get -set $special } ]
				error_check_good handle$i \
				     [lindex [lindex $ret 0] 0] $special
				error_check_good handle$i \
			     [lindex [lindex $ret 0] 1] $paddeddata
			}
			puts "\tTest$tnum.f: Try to remove and then close."
			# We only try this if it's transactional -- otherwise
			# there are no locks to prevent the removal.
			if { $txnenv == 1 } {
				set t [eval $env txn -nowait]
				catch {$env dbremove -txn $t \
				     $testfile sub$i} ret
				error_check_bad dbremove $ret 0
				$t commit
			}
			error_check_good dbc_close [eval $dbc($j,$i,0) close] 0
			error_check_good dbc_close [eval $dbc($j,$i,1) close] 0
			error_check_good db_close [eval $db($j,$i) close] 0
		}

		# Now remove the db and make sure that there are no lingering
		# handle locks that have hung around.
		if { $txnenv == 1 } {
		    error_check_good dbremove \
			 [eval $env dbremove -auto_commit $testfile sub$i] 0
		} else {
		   error_check_good dbremove \
			[eval $env dbremove $testfile sub$i ] 0
		}
	}
}
