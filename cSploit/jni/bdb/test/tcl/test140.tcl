# See the file LICENSE for redistribution information.
#
# Copyright (c) 2011, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	test140
# TEST	Test expansion and contraction of hash chains with
# TEST	deletion, compaction, and recovery.
# TEST
# TEST	On each cycle, populate a database to create reasonably
# TEST	long hash chains, say 8 - 10 entries.  Delete some 
# TEST	entries, check for consistency, compact, and check 
# TEST	for consistency again.  We both commit and abort the 
# TEST	compact, and in the case of the abort, compare 
# TEST	the recovered database to the pre-compact database.

proc test140 { method {tnum "140"} args } {
	source ./include.tcl
	global alphabet
	global passwd
	global util_path

	# This is a hash-only test.
	if { [is_hash $method] != 1} {
		puts "Skipping test$tnum for method $method."
		return
	}

	# Skip for specified pagesizes.  This test uses a small
	# pagesize to generate long chains. 
	set pgindex [lsearch -exact $args "-pagesize"]
	if { $pgindex != -1 } {
		puts "Test$tnum: Skipping for specific pagesizes"
		return
	}

	# This test uses recovery, so it sets up its own
	# transactional environment.  
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
	set dumpargs ""
	set args [split_encargs $args encargs]
	if { $encargs != "" } {
		set dumpargs " -P $passwd "
	}
	set nonoverflow [repeat $alphabet 2]
	set overflow [repeat $alphabet 250]
	set testfile test$tnum.db

	puts "Test$tnum: $method ($args $encargs)\
	    Hash testing with deletion, compaction, and recovery."

	foreach testtype { "two gaps" "one gap" "one overflow"\
	    "before overflow" "after overflow" } {
		foreach txnend { "commit" "abort" } {

			env_cleanup $testdir
			set env [eval {berkdb_env_noerr} -create -txn \
			    $encargs -mode 0644 -home $testdir]
			error_check_good env_open [is_valid_env $env] TRUE

			# Populate.
			set msg "Populate: test $testtype with $txnend"
			puts "\tTest$tnum.a: $msg."
			set db [eval {berkdb_open_noerr -create -hash -ffactor 1 \
			    -auto_commit -env $env -nelem 10 -pagesize 512 \
			    -hashproc ident_test140} $args {$testfile}]
			error_check_good dbopen [is_valid_db $db] TRUE

			set t [$env txn]
			set txn "-txn $t"

			for { set i 0 } { $i < 8 } { incr i } {
				for { set j 0 } { $j < 32 } { incr j } {
					set ret [eval {$db put} $txn \
					    {$i.$j.XXX \
					    $i.$j.$nonoverflow}]
					error_check_good put $ret 0
				}
			}

			if { [is_substr $testtype "overflow"] == 1 } {
				puts "\t\tTest$tnum.a.1: Adding overflow entry."
				set i 3
				set j 3
				set ret [eval {$db put} $txn \
				    {$i.$j.$overflow $i.$j.$overflow}]
				error_check_good put $ret 0 
			}

			error_check_good txn_commit [$t commit] 0
			$db sync

			# Delete a bunch of entries.  We do different 
			# patterns of deletions on each pass. 
			puts "\tTest$tnum.b: Delete with $testtype."
			set t [$env txn]
			set txn "-txn $t"

			# Delete several contiguous entries, creating 
			# one large hole in the chain. 
			if { $testtype == "one gap" } {
				for { set i 0 } { $i < 8 } { incr i } {
					for { set j 8 } { $j < 24 } { incr j } {
						set ret [eval {$db del} \
						    $txn {$i.$j.XXX}]
						error_check_good del $ret 0
					}
				}
			}
	
			# Knock two holes in the chain, so there's a piece 
			# that not "connected" to anything. 
			if { $testtype == "two gaps" } {
				for { set i 0 } { $i < 8 } { incr i } {
					for { set j 8 } { $j < 14 } { incr j } {
						set ret [eval {$db del} \
						    $txn {$i.$j.XXX}]
						error_check_good del $ret 0
					}
				}
				for { set i 0 } { $i < 8 } { incr i } {
					for { set j 20 } { $j < 26 } { incr j } {
						set ret [eval {$db del} \
						    $txn {$i.$j.XXX}]
						error_check_good del $ret 0
					}
				}
			}
	
			# Delete the overflow entry.
			if { $testtype == "one overflow" } {
				set ret [eval {$db del} $txn {3.3.$overflow}]
				error_check_good del_overflow $ret 0
			}
	
			# Delete the entry before the overflow entry.
			if { $testtype == "before overflow" } {
				set ret [eval {$db del} $txn {3.2.XXX}]
				error_check_good del_overflow $ret 0
			}
	
			# Delete the entry after the overflow entry.
			if { $testtype == "after overflow" } {
				set ret [eval {$db del} $txn {3.4.XXX}]
				error_check_good del_overflow $ret 0
			}
	
			error_check_good txn_commit [$t commit] 0
			$db sync 

			# Verify.
			puts "\tTest$tnum.c: Verify."
			set ret [eval {berkdb dbverify -noorderchk} \
			    -env $env {$testfile}]

			# Compact. Save a copy of the pre-compaction file.
			set ret [eval {exec $util_path/db_dump} $dumpargs \
			    {-p -f $testdir/file.init $testdir/$testfile} ]	
			filesort $testdir/file.init \
			    $testdir/file.init.sorted
			puts "\tTest$tnum.d: Compact and $txnend."

			set t [$env txn]
			set txn "-txn $t"

			if {[catch {eval {$db compact} $txn \
			    { -freespace}} ret] } { 
				error "FAIL: db compact: $ret" 
			} 
			$db sync 

			# End the transaction.  For an abort, check
			# file consistency between a copy made before
			# the txn started, the file after the abort, 
			# and the recovered file.
			if { $txnend == "abort" } {
				error_check_good abort [$t abort] 0
				$db sync

				set ret [eval {exec $util_path/db_dump -p} \
				    $dumpargs {-f $testdir/file.afterabort \
				    $testdir/$testfile} ]	
				filesort $testdir/file.afterabort \
				    $testdir/file.afterabort.sorted

				puts -nonewline "\t\tTest$tnum.d1:\
				    About to run recovery ... "
				flush stdout

				set stat [catch {eval {exec\
				    $util_path/db_recover} $dumpargs \
				    {-h $testdir}} result]
				if { $stat == 1 } {
					error "Recovery error: $result."
				}
				puts "complete"

				puts "\t\tTest$tnum.d2: Dump\
				    the recovered file."
				# Dump the recovered file.
				set ret [eval {exec $util_path/db_dump -p} \
				    $dumpargs {-f $testdir/file.recovered \
				    $testdir/$testfile} ]	
				filesort $testdir/file.recovered \
				    $testdir/file.recovered.sorted

				puts "\t\tTest$tnum.d3: Compare\
				    initial file to aborted file."
				error_check_good filecmp_orig_abort \
				    [filecmp $testdir/file.init.sorted \
				    $testdir/file.afterabort.sorted] 0
				puts "\t\tTest$tnum.d3: Compare\
				    initial file to recovered file."
				error_check_good filecmp_orig_abort \
				    [filecmp $testdir/file.init.sorted \
				    $testdir/file.recovered.sorted] 0

				# Clean up.
				catch {$db close} dbret
				catch {$env close} envret
			} else {
				error_check_good commit [$t commit] 0
				$db close
				$env close
			}
		}
	}

	# The run_all db_verify will fail since we have a custom
	# hash, so clean up before leaving the test.
	env_cleanup $testdir
}

# This proc is used for identifying hash buckets; it requires
# a key that starts with an integer, then has a dot ".", 
# then anything else.
proc ident_test140 { key } {
	set idx [string first . $key]

	# This handles the case where we are saving a 
	# hash on the initial db create. 
	if { $idx == -1 } {
		return 9999
	}

	set item [string range $key 0 [expr $idx - 1]]
	set bucket [format "%04d" $item]
	return $bucket
}

