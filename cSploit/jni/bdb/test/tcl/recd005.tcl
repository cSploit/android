# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	recd005
# TEST	Verify reuse of file ids works on catastrophic recovery.
# TEST	Test it with blob/log_blob enabled and disabled.
# TEST
# TEST	Make sure that we can do catastrophic recovery even if we open
# TEST	files using the same log file id.
proc recd005 { method args } {
	source ./include.tcl
	global rand_init

	set envargs ""
	set zero_idx [lsearch -exact $args "-zero_log"]
	if { $zero_idx != -1 } {
		set args [lreplace $args $zero_idx $zero_idx]
		set envargs "-zero_log"
	}

	set args [convert_args $method $args]
	set omethod [convert_method $method]
	set orig_args $args

	berkdb srand $rand_init

	#
	# When we ask proc 'populate' to generate big data items, it randomly
	# repeats about a third of the data strings 1000 times. So setting the
	# blob threshold to 1000 will give us a good mix of blob and non-blob
	# items.
	#
	set threshold 1000
	foreach conf [list "" "-blob_threshold $threshold" "-log_blob"] {
		set args $orig_args
		set msg ""
		if { $conf != "" } {
			set msg "with blob"
			if { $conf == "-log_blob" } {
				set msg "$msg -log_blob"
			}
		}
		puts "Recd005:\
		    $method catastrophic recovery ($envargs $msg)"

		if { $conf != "" } {
			# Blob is supported by btree, hash and heap.
			if { [is_btree $omethod] != 1 && \
			    [is_hash $omethod] != 1 && \
			    [is_heap $omethod] != 1} {
				puts "Recd005 skipping method $method for blob"
				return
			}
			# Look for incompatible configurations of blob.
			foreach c { "-encryptaes" "-encrypt" "-compress" \
			    "-dup" "-dupsort" "-read_uncommitted" \
			    "-multiversion" } {
				if { [lsearch -exact $args $c] != -1 } {
					puts "Recd005 skipping $conf for blob"
					return
				}
			}
			if { [lsearch -exact $args "-chksum"] != -1 } {
				set indx [lsearch -exact $args "-chksum"]
				set args [lreplace $args $indx $indx]
				puts "Recd005 ignoring -chksum for blob"
			}
			# Set up the blob argument.
			if { $conf == "-log_blob" } {
				append conf " -blob_threshold $threshold"
			}
		}

		set testfile1 recd005.1.db
		set testfile2 recd005.2.db
		set max_locks 2000
		set eflags "-create -txn wrnosync -lock_max_locks $max_locks \
		    -lock_max_objects $max_locks -home $testdir $envargs $conf"

		set tnum 0
		foreach sizes "{1000 10} {10 1000}" {
			foreach ops "{abort abort} {abort commit} \
			    {commit abort} {commit commit}" {
				env_cleanup $testdir
				incr tnum

				set s1 [lindex $sizes 0]
				set s2 [lindex $sizes 1]
				set op1 [lindex $ops 0]
				set op2 [lindex $ops 1]
				puts "\tRecd005.$tnum: $s1 $s2 $op1 $op2"

				puts "\tRecd005.$tnum.a: creating environment"
				set env_cmd "berkdb_env $eflags"
				set dbenv [eval $env_cmd]
				error_check_bad dbenv $dbenv NULL

				# Create the two databases.
				set oflags "-create -auto_commit -mode 0644\
				    -env $dbenv $args $omethod"
				set db1 [eval {berkdb_open} $oflags $testfile1]
				error_check_bad db_open $db1 NULL
				error_check_good db_open [is_substr $db1 db] 1
				error_check_good db_close [$db1 close] 0

				set db2 [eval {berkdb_open} $oflags $testfile2]
				error_check_bad db_open $db2 NULL
				error_check_good db_open [is_substr $db2 db] 1
				error_check_good db_close [$db2 close] 0
				$dbenv close

				set dbenv [eval $env_cmd]
				puts "\tRecd005.$tnum.b: Populating databases"
				set bigdata 0
				if { $conf != "" } {
					set bigdata 1
				}
				eval {do_one_file $testdir $method $dbenv \
				    $env_cmd $testfile1 $s1 $op1 $bigdata} \
				    $args
				eval {do_one_file $testdir $method $dbenv \
				    $env_cmd $testfile2 $s2 $op2 $bigdata} \
				    $args

				puts "\tRecd005.$tnum.c:\
				    Verifying initial population"
				eval {check_file \
				    $testdir $env_cmd $testfile1 $op1 } $args
				eval {check_file \
				    $testdir $env_cmd $testfile2 $op2 } $args

				# Now, close the environment (so that recovery
				# will work on NT which won't allow delete of
				# an open file).
				reset_env $dbenv

				berkdb debug_check
				puts -nonewline "\tRecd005.$tnum.d:\
				    About to run recovery ... "
				flush stdout

				set stat [catch {exec \
				    $util_path/db_recover -h $testdir -c} \
				    result]
				if { $stat == 1 } {
					error "Recovery error: $result."
				}

				puts "complete"

				# Substitute a file that will need recovery
				# and try running recovery again.
				if { $op1 == "abort" } {
					file copy -force \
					    $testdir/$testfile1.afterop \
					    $testdir/$testfile1
					move_file_extent $testdir $testfile1 \
					    afterop copy
				} else {
					file copy -force \
					    $testdir/$testfile1.init \
					    $testdir/$testfile1
					move_file_extent $testdir $testfile1 \
					    init copy
				}
				if { $op2 == "abort" } {
					file copy -force \
					    $testdir/$testfile2.afterop \
					    $testdir/$testfile2
					move_file_extent $testdir $testfile2 \
					    afterop copy
				} else {
					file copy -force \
					    $testdir/$testfile2.init \
					    $testdir/$testfile2
					move_file_extent $testdir $testfile2 \
					    init copy
				}

				berkdb debug_check
				puts -nonewline "\tRecd005.$tnum.e: About to\
				    run recovery on pre-op database ... "
				flush stdout

				set stat \
				    [catch {exec $util_path/db_recover \
				    -h $testdir -c} result]
				if { $stat == 1 } {
					error "Recovery error: $result."
				}
				puts "complete"

				set dbenv [eval $env_cmd]
				eval {check_file $testdir \
				    $env_cmd $testfile1 $op1 } $args
				eval {check_file $testdir \
				    $env_cmd $testfile2 $op2 } $args
				reset_env $dbenv

				puts "\tRecd005.$tnum.f:\
				    Verify db_printlog can read logfile"
				set tmpfile $testdir/printlog.out
				set stat [catch \
				    {exec $util_path/db_printlog -h $testdir \
				    > $tmpfile} ret]
				error_check_good db_printlog $stat 0
				fileremove $tmpfile
			}
		}
	}
}

proc do_one_file { dir method env env_cmd filename num op bigdata args} {
	source ./include.tcl

	set init_file $dir/$filename.t1
	set afterop_file $dir/$filename.t2
	set final_file $dir/$filename.t3

	# Save the initial file and open the environment and the first file
	file copy -force $dir/$filename $dir/$filename.init
	copy_extent_file $dir $filename init

	# If method is heap, copy other files
	if { [is_heap $method] == 1 } {
		append filename1 $filename "1"
		file copy -force $dir/$filename1 $dir/$filename.init1
		copy_extent_file $dir $filename1 init

		append filename2 $filename "2"
		file copy -force $dir/$filename2 $dir/$filename.init2
		copy_extent_file $dir $filename2 init
	}
	set oflags "-auto_commit -unknown -env $env"
	set db [eval {berkdb_open} $oflags $args $filename]

	# Dump out file contents for initial case
	eval open_and_dump_file $filename $env $init_file nop \
	    dump_file_direction "-first" "-next" $args

	set txn [$env txn]
	error_check_bad txn_begin $txn NULL
	error_check_good txn_begin [is_substr $txn $env] 1

	# Now fill in the db and the txnid in the command
	populate $db $method $txn $num 0 $bigdata

	# Sync the file so that we can capture a snapshot to test
	# recovery.
	error_check_good sync:$db [$db sync] 0
	file copy -force $dir/$filename $dir/$filename.afterop
	copy_extent_file $dir $filename afterop

	#if we are doing heap, we have more files to copy
	if { [is_heap $method] == 1 } {
		file copy -force $dir/$filename1 $dir/$filename.afterop1
		copy_extent_file $dir $filename1 afterop

		file copy -force $dir/$filename2 $dir/$filename.afterop2
		copy_extent_file $dir $filename2 afterop
	}

	eval open_and_dump_file $testdir/$filename.afterop NULL \
	    $afterop_file nop dump_file_direction "-first" "-next" $args
	error_check_good txn_$op:$txn [$txn $op] 0

	if { $op == "commit" } {
		puts "\t\tFile $filename executed and committed."
	} else {
		puts "\t\tFile $filename executed and aborted."
	}

	# Dump out file and save a copy.
	error_check_good sync:$db [$db sync] 0
	eval open_and_dump_file $testdir/$filename NULL $final_file nop \
	    dump_file_direction "-first" "-next" $args
	file copy -force $dir/$filename $dir/$filename.final
	copy_extent_file $dir $filename final

	#if we are doing heap, we have more files to copy
	if { [is_heap $method] == 1 } {
		file copy -force $dir/$filename1 $dir/$filename.final1
		copy_extent_file $dir $filename1 final

		file copy -force $dir/$filename2 $dir/$filename.final2
		copy_extent_file $dir $filename2 final
	}

	# If this is an abort, it should match the original file.
	# If this was a commit, then this file should match the
	# afterop file.
	if { $op == "abort" } {
		filesort $init_file $init_file.sort
		filesort $final_file $final_file.sort
		error_check_good \
		    diff(initial,post-$op):diff($init_file,$final_file) \
		    [filecmp $init_file.sort $final_file.sort] 0
	} else {
		filesort $afterop_file $afterop_file.sort
		filesort $final_file $final_file.sort
		error_check_good \
		    diff(post-$op,pre-commit):diff($afterop_file,$final_file) \
		    [filecmp $afterop_file.sort $final_file.sort] 0
	}

	error_check_good close:$db [$db close] 0
}

proc check_file { dir env_cmd filename op args} {
	source ./include.tcl

	set init_file $dir/$filename.t1
	set afterop_file $dir/$filename.t2
	set final_file $dir/$filename.t3

	eval open_and_dump_file $testdir/$filename NULL $final_file nop \
	    dump_file_direction "-first" "-next" $args
	if { $op == "abort" } {
		filesort $init_file $init_file.sort
		filesort $final_file $final_file.sort
		error_check_good \
		    diff(initial,post-$op):diff($init_file,$final_file) \
		    [filecmp $init_file.sort $final_file.sort] 0
	} else {
		filesort $afterop_file $afterop_file.sort
		filesort $final_file $final_file.sort
		error_check_good \
		    diff(pre-commit,post-$op):diff($afterop_file,$final_file) \
		    [filecmp $afterop_file.sort $final_file.sort] 0
	}
}
