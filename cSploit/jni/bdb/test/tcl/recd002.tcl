# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	recd002
# TEST	Split recovery tests with blob/log_blob enabled and disabled.
# TEST	For every known split log message, makes sure
# TEST	that we exercise redo, undo, and do-nothing condition.
proc recd002 { method {select 0} args} {
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

	set pgindex [lsearch -exact $args "-pagesize"]
	if { $pgindex != -1 } {
		puts "Recd002: skipping for specific pagesizes"
		return
	}
	berkdb srand $rand_init

	# Queues don't do splits, so we don't really need the small page
	# size and the small page size is smaller than the record, so it's
	# a problem.
	if { [string compare $omethod "-queue"] == 0 } {
		set pagesize 4096
	} else {
		set pagesize 512
	}

	set testfile recd002.db
	set testfile2 recd002-2.db
	set eflags "-create -txn wrnosync\
	    -lock_max_locks 2000 -home $testdir $envargs"

	# Set up the env cmd used in op_recover.
	set env_cmd_rec "berkdb_env $eflags"

	#
	# When we ask proc 'populate' to generate big data items, it randomly
	# repeats about a third of the data strings 1000 times. So setting the
	# blob threshold to 1000 will give us a good mix of blob and non-blob
	# items.
	#
	set threshold 1000
	set orig_args $args
	foreach conf [list "" "-blob_threshold $threshold" "-log_blob"] {
		set args $orig_args
		set msg ""
		if { $conf != "" } {
			set msg "with blob"
			if { $conf == "-log_blob" } {
				set msg "$msg -log_blob"
			}
		}

		puts "Recd002: $method split recovery tests ($envargs $msg)"

		if { $conf != "" } {
			# Blob is supported by btree, hash and heap.
			if { [is_btree $omethod] != 1 && \
			    [is_hash $omethod] != 1 && \
			    [is_heap $omethod] != 1} {
				puts "Recd002 skipping method $method for blob"
				return
			}
			# Look for incompatible configurations of blob.
			foreach c { "-encryptaes" "-encrypt" "-compress" \
			    "-dup" "-dupsort" "-read_uncommitted" \
			    "-multiversion" } {
				if { [lsearch -exact $args $c] != -1 } {
					puts "Recd002 skipping $conf for blob"
					return
				}
			}
			if { [lsearch -exact $args "-chksum"] != -1 } {
				set indx [lsearch -exact $args "-chksum"]
				set args [lreplace $args $indx $indx]
				puts "Recd002 ignoring -chksum for blob"
			}
			# Set up the blob argument.
			if { $conf == "-log_blob" } {
				append conf " -blob_threshold $threshold"
				append env_cmd_rec " -log_blob"
			}
		}

		env_cleanup $testdir

		puts "\tRecd002.a: creating environment"
		set env_cmd "berkdb_env $eflags $conf"
		set dbenv [eval $env_cmd]
		error_check_bad dbenv $dbenv NULL

		# Create the databases. We will use a small page size so that
		# splits happen fairly quickly.
		set oflags "-create -auto_commit $args $omethod -mode 0644 \
		    -env $dbenv -pagesize $pagesize $testfile"
		set db [eval {berkdb_open} $oflags]
		error_check_bad db_open $db NULL
		error_check_good db_open [is_substr $db db] 1
		error_check_good db_close [$db close] 0
		set oflags "-create -auto_commit $args $omethod -mode 0644 \
		    -env $dbenv -pagesize $pagesize $testfile2"
		set db [eval {berkdb_open} $oflags]
		error_check_bad db_open $db NULL
		error_check_good db_open [is_substr $db db] 1
		error_check_good db_close [$db close] 0
		reset_env $dbenv

		# List of recovery tests: {CMD MSG} pairs
		set bigdata 0
		if { $conf != "" } {
			set bigdata 1
		}
		set slist {
			{ {populate DB $omethod TXNID $n 0 $bigdata}
			   "Recd002.b: splits"}
			{ {unpopulate DB TXNID $r}
			   "Recd002.c: Remove keys"}
		}

		# If pages are 512 bytes, then adding 512 key/data pairs
		# should be more than sufficient.
		set n 512
		set r [expr $n / 2 ]
		foreach pair $slist {
			set cmd [subst [lindex $pair 0]]
			set msg [lindex $pair 1]
			if { $select != 0 } {
				set tag [lindex $msg 0]
				set tail [expr [string length $tag] - 2]
				set tag [string range $tag $tail $tail]
				if { [lsearch $select $tag] == -1 } {
				continue
				}
			}
			op_recover abort \
			    $testdir $env_cmd_rec $testfile $cmd $msg $args
			op_recover commit \
			    $testdir $env_cmd_rec $testfile $cmd $msg $args
			#
			# Test prepare only when blobs are not enabled: they
			# require support for file removal in prepared txns.
			# Note that since prepare-discard ultimately aborts
			# the txn, it must come before prepare-commit.
			#
			if { $conf == "" } {
				op_recover prepare-abort $testdir \
				    $env_cmd_rec $testfile2 $cmd $msg $args
				op_recover prepare-discard $testdir \
				    $env_cmd_rec $testfile2 $cmd $msg $args
				op_recover prepare-commit $testdir \
				    $env_cmd_rec $testfile2 $cmd $msg $args
			}
		}

		puts "\tRecd002.d: Verify db_printlog can read logfile"
		set tmpfile $testdir/printlog.out
		set stat [catch {exec $util_path/db_printlog -h $testdir \
		    > $tmpfile} ret]
		error_check_good db_printlog $stat 0
		fileremove $tmpfile
	}
}
