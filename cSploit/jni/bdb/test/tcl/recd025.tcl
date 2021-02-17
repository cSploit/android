# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	recd025
# TEST	Basic tests for transaction bulk loading and recovery with
# TEST	blob/log_blob enabled and disabled.
# TEST	In particular, verify that the tricky hot backup protocol works.

# These tests check the following conditions

# 1. We can abort a TXN_BULK transaction, both online and during recovery.
# 2. Tricky hot backup protocol works correctly
#     - start bulk txn; populate, adding pages
#     - set hotbackup_in_progress (forces chkpt, as bulk txn is in progress)
#     - copy database file
#     - populate more, adding more pages (with bulk optimization disabled)
#     - commit
#     - move copy back into test dir
#     - roll forward, verify rolled-forward backup matches committed database

# A more straightforward test of bulk transactions and hot backup is
# in the backup test.
    
proc recd025 { method args } {
	global fixed_len
	global is_hp_test
	source ./include.tcl

	# Skip test for specified page sizes -- we want to
	# specify our own page size.
	set pgindex [lsearch -exact $args "-pagesize"]
	if { $pgindex != -1 } {
		puts "Recd025: Skipping for specific pagesizes"
		return
	}

	# Skip test for heap, as heap does not have BULK ops
	if { [is_heap $method] == 1 } {
		puts "Recd025 skipping for heap method."
		return
	}

	# Increase size of fixed-length records to match other methods.
        # We pick an arbitrarily larger record size to ensure that we
        # allocate several pages.
	set orig_fixed_len $fixed_len
	set fixed_len 53
	set opts [convert_args $method $args]
	set omethod [convert_method $method]

	set eindex [lsearch -exact $args "-env"]
	if { $eindex != -1 } {
		puts "Recd025 skipping -env since it needs its own,"
		return
	}

	# Set the blob threshold as the length of the data items.	
	set threshold 53
	set orig_opts $opts
	foreach conf [list "" "-blob_threshold $threshold" "-log_blob"] {
		set opts $orig_opts
		set msg ""
		if { $conf != "" } {
			set msg "with blob"
			if { $conf == "-log_blob" } {
				set msg "$msg -log_blob"
			}
		}
		puts "Recd025: TXN_BULK page allocation and recovery ($msg)."

		if { $conf != "" } {
			# Blob is supported by btree, hash and heap.
			if { [is_btree $omethod] != 1 && \
			    [is_hash $omethod] != 1 } {
				puts "Recd025 skipping method $method for blob"
				return
			}
			# Look for incompatible configurations of blob.
			foreach c { "-encryptaes" "-encrypt" "-compress" \
			    "-dup" "-dupsort" "-read_uncommitted" \
			    "-multiversion" } {
				if { [lsearch -exact $opts $c] != -1 } {
					puts "Recd025 skipping $conf for blob"
					return
				}
			}
			if { [lsearch -exact $opts "-chksum"] != -1 } {
				set indx [lsearch -exact $opts "-chksum"]
				set opts [lreplace $opts $indx $indx]
				puts "Recd025 ignoring -chksum for blob"
			}
			# Set up the blob argument.
			if { $conf == "-log_blob" } {
				append conf " -blob_threshold $threshold"
			}
		}

		# Create the database and environment.
		env_cleanup $testdir
		set testfile recd025.db

		puts "\tRecd025.1a: creating environment"
		set env_cmd "berkdb_env -create -txn -home $testdir $conf"
		set dbenv [eval $env_cmd]
		error_check_good dbenv [is_valid_env $dbenv] TRUE

		# Open database with small pages.
		puts "\tRecd025.1b:\
		    creating and populating database with small pages"
		set pagesize 512
		set oflags "-create $omethod -mode 0644 -pagesize $pagesize \
		    -env $dbenv -auto_commit $opts $testfile"
		set db [eval {berkdb_open} $oflags]

		error_check_good db_open [is_valid_db $db] TRUE

		set batchsize 20
		set lim 0
		set iter 1
		set datasize 53
		set data [repeat "a" $datasize]

		set t [$dbenv txn]

		for {set lim [expr $lim + $batchsize]} \
		    {$iter <= $lim } {incr iter} {
			eval {$db put} -txn $t $iter $data
		}
		error_check_good txn_commit [$t commit] 0

		error_check_good sync:$db [$db sync] 0

		# Make a copy of the database now, for comparison

		catch {
		    file copy -force $testdir/$testfile $testdir/$testfile.orig
		} res
		copy_extent_file $testdir $testfile orig
		eval open_and_dump_file $testdir/$testfile.orig NULL \
		    $testdir/dump.orig nop dump_file_direction \
		    "-first" "-next" $opts

		puts "\tRecd025.1c:\
		    start bulk transaction, put data, allocating pages"
	
		set t [$dbenv txn -txn_bulk]

		for {set lim [expr $lim + $batchsize]} \
		    {$iter <= $lim } {incr iter} {
			eval {$db put} -txn $t $iter $data
		}

		# A copy before aborting
		error_check_good sync:$db [$db sync] 0
		catch {
		    file copy -force \
		    $testdir/$testfile $testdir/$testfile.preabort
		} res
		copy_extent_file $testdir $testfile preabort

		puts "\tRecd025.1d:\
		    abort bulk transaction; verify undo of puts"

		error_check_good txn_abort [$t abort] 0

		error_check_good sync:$db [$db sync] 0

		eval open_and_dump_file $testdir/$testfile NULL \
		    $testdir/dump.postabort nop dump_file_direction \
		    "-first" "-next" $opts

		filesort $testdir/dump.orig $testdir/dump.orig.sort
		filesort $testdir/dump.postabort $testdir/dump.postabort.sort

		error_check_good verify_abort_diff [filecmp \
		    $testdir/dump.orig.sort $testdir/dump.postabort.sort] 0

		error_check_good db_close [$db close] 0
		reset_env $dbenv

		puts "\tRecd025.1e:\
		    recovery with allocations rolled back"

		# Move the preabort file into place, and run recovery

		catch {
		    file copy -force \
		    $testdir/$testfile.preabort $testdir/$testfile
		} res

		set stat [catch \
		    {eval exec $util_path/db_recover -h $testdir -c } res]
		if { $stat == 1 } {
			error "FAIL: Recovery error: $res."
		}

		eval open_and_dump_file $testdir/$testfile NULL \
		    $testdir/dump.postrecovery nop dump_file_direction \
		    "-first" "-next" $opts
		filesort $testdir/dump.postrecovery \
		    $testdir/dump.postrecovery.sort

		error_check_good verify_abort_diff [filecmp \
		    $testdir/dump.orig.sort $testdir/dump.postabort.sort] 0


		# Now for the really tricky hot backup test.

		puts "\tRecd025.3a: opening environment"
		set env_cmd "berkdb_env -create -txn -home $testdir $conf"
		set dbenv [eval $env_cmd]
		error_check_good dbenv [is_valid_env $dbenv] TRUE

		# Open database
		puts "\tRecd025.3b: opening database with small pages"
		set oflags "$omethod -pagesize $pagesize \
		    -env $dbenv -auto_commit $opts $testfile"
		set db [eval {berkdb_open} $oflags]

		error_check_good db_open [is_valid_db $db] TRUE
	
		puts "\tRecd025.3c: start bulk transaction and add pages"
		set t [$dbenv txn -txn_bulk]

		for {set lim [expr $lim + $batchsize]} \
		    {$iter <= $lim } {incr iter} {
			eval {$db put} -txn $t $iter $data
		}
	
		puts "\tRecd025.3d:\
		    Set hotbackup_in_progress, and copy the database"

		$dbenv set_flags -hotbackup_in_progress on

		catch {
		    file copy -force \
		    $testdir/$testfile $testdir/$testfile.hotcopy
		} res

		puts "\tRecd025.3e: add more pages and commit"


		for {set lim [expr $lim + $batchsize] } \
		    {$iter <= $lim } {incr iter} {
			eval {$db put} -txn $t $iter $data
		}

		error_check_good txn_commit [$t commit] 0

		$dbenv set_flags -hotbackup_in_progress off

		error_check_good db_close [$db close] 0
		reset_env $dbenv

		# dump the finished product

		eval open_and_dump_file $testdir/$testfile NULL \
		    $testdir/dump.final nop dump_file_direction \
		    "-first" "-next" $opts

		filesort \
		    $testdir/dump.final $testdir/dump.final.sort

		puts "\tRecd025.3f: roll forward the hot copy and compare"

		catch {
		    file copy -force \
		    $testdir/$testfile.hotcopy $testdir/$testfile
		} res

		#
        	# Perform catastrophic recovery, to simulate hot
		# backup behavior.
		#
		set stat [catch \
		    {eval exec $util_path/db_recover -h $testdir -c } res]
		if { $stat == 1 } {
			error "FAIL: Recovery error: $res."
		}

		eval open_and_dump_file $testdir/$testfile NULL \
		    $testdir/dump.recovered_copy nop dump_file_direction \
		    "-first" "-next" $opts
		filesort $testdir/dump.recovered_copy \
		    $testdir/dump.recovered_copy.sort

		error_check_good verify_abort_diff \
		    [filecmp $testdir/dump.final.sort \
		    $testdir/dump.recovered_copy.sort] 0

		# Set fixed_len back to the global value so we don't
		# mess up other tests.
		set fixed_len $orig_fixed_len
	}
}

	
	
	

	

	
	


	
