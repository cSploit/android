# See the file LICENSE for redistribution information.
#
# Copyright (c) 2000, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	recd018
# TEST	Test recover of closely interspersed checkpoints and commits.
# TEST	Test with blob/log_blob enabled and disabled.
#
# This test is from the error case from #4230.
#
proc recd018 { method {ndbs 10} args } {
	source ./include.tcl

	set args [convert_args $method $args]
	set omethod [convert_method $method]
	set orig_args $args
	set tnum "018"
	set tname recd$tnum.db

	# The structure of a data item is "$i.data", so set the blob
	# threshold to 6 and all items will be stored as blobs. 
	set threshold 6
	foreach conf [list "" "-blob_threshold $threshold" "-log_blob"] {
		set args $orig_args
		set msg ""
		if { $conf != "" } {
			set msg "with blob"
			if { $conf == "-log_blob" } {
				set msg "$msg -log_blob"
			}
		}
		puts "Recd$tnum ($args):\
		    $method recovery of checkpoints and commits ($msg)."

		if { $conf != "" } {
			# Blob is supported by btree, hash and heap.
			if { [is_btree $omethod] != 1 && \
			    [is_hash $omethod] != 1 && \
			    [is_heap $omethod] != 1} {
				puts "Recd018 skipping method $method for blob"
				return
			}
			# Look for incompatible configurations of blob.
			foreach c { "-encryptaes" "-encrypt" "-compress" \
			    "-dup" "-dupsort" "-read_uncommitted" \
			    "-multiversion" } {
				if { [lsearch -exact $args $c] != -1 } {
					puts "Recd018 skipping $conf for blob"
					return
				}
			}
			if { [lsearch -exact $args "-chksum"] != -1 } {
				set indx [lsearch -exact $args "-chksum"]
				set args [lreplace $args $indx $indx]
				puts "Recd018 ignoring -chksum for blob"
			}
			# Set up the blob argument.
			if { $conf == "-log_blob" } {
				append conf " -blob_threshold $threshold"
			}
		}

		env_cleanup $testdir

		set i 0
		if { [is_record_based $method] == 1 } {
			set key 1
			set key2 2
		} else {
			set key KEY
			set key2 KEY2
		}

		puts "\tRecd$tnum.a: Create environment and database."
		set flags "-create -txn wrnosync -home $testdir $conf"

		set env_cmd "berkdb_env $flags"
		set dbenv [eval $env_cmd]
		error_check_good dbenv [is_valid_env $dbenv] TRUE

		set oflags "-auto_commit\
		    -env $dbenv -create -mode 0644 $args $omethod"
		for { set i 0 } { $i < $ndbs } { incr i } {
			set testfile $tname.$i
			set db($i) [eval {berkdb_open} $oflags $testfile]
			error_check_good dbopen [is_valid_db $db($i)] TRUE
			set file $testdir/$testfile.init
			catch { file copy -force $testdir/$testfile $file} res
			copy_extent_file $testdir $testfile init
		}

		# Main loop:  Write a record or two to each database. Do a
		# commit immediately followed by a checkpoint after each one.
		error_check_good "Initial Checkpoint" [$dbenv txn_checkpoint] 0

		puts "\tRecd$tnum.b Put/Commit/Checkpoint to $ndbs databases"
		for { set i 0 } { $i < $ndbs } { incr i } {
			set testfile $tname.$i
			set data $i
			if { $conf != "" } {
				set data $i.data
			}

			# Put, in a txn.
			set txn [$dbenv txn]
			error_check_good txn_begin \
			    [is_valid_txn $txn $dbenv] TRUE
			error_check_good db_put [$db($i) put \
			    -txn $txn $key [chop_data $method $data]] 0
			error_check_good txn_commit [$txn commit] 0
			error_check_good txn_checkpt [$dbenv txn_checkpoint] 0
			if { [expr $i % 2] == 0 } {
				set txn [$dbenv txn]
				error_check_good txn2 \
				    [is_valid_txn $txn $dbenv] TRUE
				error_check_good db_put [$db($i) put -txn \
				    $txn $key2 [chop_data $method $data]] 0
				error_check_good txn_commit [$txn commit] 0
				error_check_good txn_checkpt \
				    [$dbenv txn_checkpoint] 0
			}
			error_check_good db_close [$db($i) close] 0
			set file $testdir/$testfile.afterop
			catch { file copy -force $testdir/$testfile $file} res
			copy_extent_file $testdir $testfile afterop
		}
		error_check_good env_close [$dbenv close] 0

		# Now, loop through and recover to each timestamp,
		# verifying the expected increment.
		puts "\tRecd$tnum.c: Run recovery (no-op)"
		set ret [catch {exec $util_path/db_recover -h $testdir} r]
		error_check_good db_recover $ret 0

		puts "\tRecd$tnum.d: Run recovery (initial file)"
		for { set i 0 } {$i < $ndbs } { incr i } {
			set testfile $tname.$i
		set file $testdir/$testfile.init
		catch { file copy -force $file $testdir/$testfile } res
		move_file_extent $testdir $testfile init copy
		}

		set ret [catch {exec $util_path/db_recover -h $testdir} r]
		error_check_good db_recover $ret 0

		puts "\tRecd$tnum.e: Run recovery (after file)"
		for { set i 0 } {$i < $ndbs } { incr i } {
			set testfile $tname.$i
			set file $testdir/$testfile.afterop
			catch { file copy -force $file $testdir/$testfile } res
			move_file_extent $testdir $testfile afterop copy
		}

		set ret [catch {exec $util_path/db_recover -h $testdir} r]
		error_check_good db_recover $ret 0
	}
}
