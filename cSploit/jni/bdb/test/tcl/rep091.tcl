# See the file LICENSE for redistribution information.
#
# Copyright (c) 2010, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# TEST	rep091
# TEST	Read-your-writes consistency.
# TEST	Write transactions at the master, and then call the txn_applied()
# TEST	method to see whether the client has received and applied them yet.
#
proc rep091 { method { niter 20 } { tnum "091" } args } {
	source ./include.tcl
	global databases_in_memory
	global repfiles_in_memory

	# Skip for all methods except btree.
	if { $checking_valid_methods } {
		set test_methods { btree }
		return $test_methods
	}
	if { [is_btree $method] == 0 } {
		puts "Rep091: Skipping for method $method."
		return
	}

	# Set up for on-disk or in-memory databases and make dbname match 
	# the one assigned in rep_test.
	if { $databases_in_memory == 1 } {
		set dbname { "" "test.db" }
		set msg "using named in-memory databases"
	} else {
		set dbname "test.db"
		set msg "using on-disk databases"
	}

	# For this test it's important to test both ways, so for now run both
	# under our control.  Later, when the containing test infrastructure for
	# doing this automatically is more fully developed, we can remove this
	# loop and just let the infrastructure handle it.
	# 
	set orig $repfiles_in_memory
	foreach repfiles_in_memory {0 1} {

		if { $repfiles_in_memory } {
			set msg2 "and in-memory replication files"
		} else {
			set msg2 "and on-disk replication files"
		}
		puts "Rep$tnum ($method $args): Test of\
		    read-your-writes consistency $msg $msg2."
		
		rep091a_sub $method $niter $tnum no $dbname $args
		rep091a_sub $method $niter $tnum yes $dbname $args
		rep091b_sub $method $niter $tnum $dbname $args
		rep091c_sub $method $niter $tnum no $dbname $args
		rep091c_sub $method $niter $tnum yes $dbname $args
		rep091d_sub $method $niter $tnum no $dbname $args
		rep091d_sub $method $niter $tnum yes $dbname $args
		rep091e_sub $method $niter $tnum $dbname $args
		if { $repfiles_in_memory } {
			rep091f_sub $method $niter $tnum $dbname $args
		}
	}

	# Restore original setting, in case any other tests are going to be run
	# after this one.
	# 
	set repfiles_in_memory $orig
}

proc rep091a_sub { method niter tnum future_gen dbname largs } {
	global rep_verbose
	global testdir
	global verbose_type
	global repfiles_in_memory
	global tcl_platform

	puts -nonewline "Rep$tnum: read-your-writes consistency, basic test"
	if { $future_gen } {
		puts ", with extra gen cycle."
	} else {
		puts "."
	}

	set verbargs ""
	if { $rep_verbose == 1 } {
		set verbargs " -verbose {$verbose_type on} "
	}

	set repmemargs ""
	if { $repfiles_in_memory } {
		set repmemargs "-rep_inmem_files "
	}

	env_cleanup $testdir

	replsetup $testdir/MSGQUEUEDIR

	set masterdir $testdir/MASTERDIR
	set clientdir $testdir/CLIENTDIR

	file mkdir $masterdir
	file mkdir $clientdir

	puts "\tRep$tnum.a: Create master and client."
	repladd 1
	set ma_envcmd "berkdb_env_noerr -create -txn -errpfx MASTER \
	    $repmemargs \
	    $verbargs -home $masterdir -rep_transport \[list 1 replsend\]"
	set masterenv [eval $ma_envcmd -rep_master]

	repladd 2
	set cl_envcmd "berkdb_env_noerr -create -txn -errpfx CLIENT \
	    $repmemargs \
	    $verbargs -home $clientdir -rep_transport \[list 2 replsend\]"
	set clientenv [eval $cl_envcmd -rep_client]

	set envlist "{$masterenv 1} {$clientenv 2}"
	process_msgs $envlist

	if { $future_gen } {
		# Cycle the gen an extra time, so that the client sees not only
		# a future transaction, but a future gen.  Restart the master as
		# a client first, and communicate with the surviving other
		# client, to make sure the master realizes the correct gen.  We
		# can get away without doing this in the normal case, but when
		# repfiles are in memory the master otherwise starts over from
		# gen 0, leading to quite a mess.
		# 
		$masterenv close
		set masterenv [eval $ma_envcmd -rep_client -recover]
		set envlist "{$masterenv 1} {$clientenv 2}"
		process_msgs $envlist
		
		$masterenv rep_start -master
	}

	set start 0
	eval rep_test $method $masterenv NULL $niter $start $start 0 $largs
	incr start $niter

	puts "\tRep$tnum.b: Write txn and get its commit token."
	set omethod [convert_method $method]
	set dbargs [convert_args $method $largs]
	set db [eval berkdb_open_noerr -env $masterenv -auto_commit \
		-create $omethod $dbargs $dbname ]
	set txn [$masterenv txn -token]
	$db put -txn $txn "key1" "data1"
	set token [$txn commit]

# binary scan $token IIIII r1 r2 r3 r4 r5
# puts "the token value is $r1 $r2 $r3 $r4 $r5"

	# For the most part, we're interested in checking txn_applied at
	# clients.  But it's also supposed to work in a degenerate, simple
	# manner at the master.
	#
	error_check_good applied_at_master [$masterenv txn_applied $token] 0

	# While we're at it, verify that if we don't ask for the commit token,
	# the result of [$env commit] is just a plain "0".  (This is more of a
	# sanity check on the Tcl API, to help us believe that the rest of this
	# test is valid.)
	# 
	set txn [$masterenv txn]
	$db put -txn $txn "other key" "any data"
	set result [$txn commit]
	error_check_good commit_no_token $result 0

	$db close

	puts "\tRep$tnum.c: Check the applied status of the transaction at the client."

	# Before processing messages, the client will not have heard of the
	# transaction yet.  But afterwards it should.  Check it once with no
	# timeout (default 0), and again with a timeout specified to do a simple
	# test of waiting.  (Multi-thread waiting tests will be done
	# separately.)
	#
	set result [$clientenv txn_applied $token]
	error_check_good not_applied [is_substr $result DB_TIMEOUT] 1

	# Tcl timer behavior is abnormal on Windows 2003; decrease
	# the expected duration by 1 second. 
	set exp_dur 10
	set os_name $tcl_platform(os)
	set os_version $tcl_platform(osVersion)
	if { [string match "Windows NT" $os_name] && 
	     [string match "5.2" $os_version] } {
		set exp_dur [expr $exp_dur - 1]
	}
	set start [clock seconds]
	set result [$clientenv txn_applied -timeout 10000000 $token]
	set duration [expr [clock seconds] - $start]
	error_check_good not_yet_applied [expr $duration >= $exp_dur] 1

	process_msgs $envlist
	error_check_good txn_applied [$clientenv txn_applied $token] 0

	# "Empty" transactions are not really interesting either, but again
	# they're supposed to be allowed:
	#
	set txn [$masterenv txn -token]
	set token [$txn commit]
	set result [$masterenv txn_applied $token]
	error_check_good empty_txn [is_substr $result DB_KEYEMPTY] 1
	set result [$clientenv txn_applied $token]
	error_check_good empty_txn2 [is_substr $result DB_KEYEMPTY] 1

	# Check a few simple invalid cases.
	#
	error_check_bad client_token [catch {$clientenv txn -token} result] 0
	error_check_good cl_tok_msg [is_substr $result "invalid arg"] 1
	set txn [$masterenv txn]
	error_check_bad child_token \
	    [catch {$masterenv txn -token -parent $txn} result] 0
	error_check_good parent_tok_msg [is_substr $result "invalid arg"] 1

	$txn abort
	$clientenv close
	$masterenv close

	replclose $testdir/MSGQUEUEDIR
}

# Verify that an LSN history database appears correctly at a client even if the
# client is created via a hot backup (instead of internal init).  Sites "A" and
# "B" alternate the master role, in order to facilitate accumulating a history
# of gen changes (especially in the in-mem case, where otherwise our LSN history
# database would be lost each time we restarted the master).  The "CLIENT2" site
# will be the one to be initialized via hot backup.
# 
proc rep091b_sub { method niter tnum dbname largs } {
	global rep_verbose
	global testdir
	global verbose_type
	global util_path
	global repfiles_in_memory

	puts "Rep$tnum: read-your-writes consistency, hot backup."

	set verbargs ""
	if { $rep_verbose == 1 } {
		set verbargs " -verbose {$verbose_type on} "
	}

	set repmemargs ""
	if { $repfiles_in_memory } {
		set repmemargs "-rep_inmem_files "
	}

	env_cleanup $testdir

	replsetup $testdir/MSGQUEUEDIR

	set dira $testdir/DIRA
	set dirb $testdir/DIRB
	set clientdir2 $testdir/CLIENTDIR2

	file mkdir $dira
	file mkdir $dirb
	file mkdir $clientdir2

	repladd 1
	set envcmd_a "berkdb_env_noerr -create -txn -errpfx SITE_A \
	    $repmemargs \
	    $verbargs -home $dira -rep_transport \[list 1 replsend\]"
	set masterenv [eval $envcmd_a -rep_master]

	repladd 2
	set envcmd_b "berkdb_env_noerr -create -txn -errpfx SITE_B \
	    $repmemargs \
	    $verbargs -home $dirb -rep_transport \[list 2 replsend\]"
	set clientenv [eval $envcmd_b -rep_client]

	set envlist "{$masterenv 1} {$clientenv 2}"
	process_msgs $envlist

	set start 0

	# Swap master and client role, some arbitrary number of times, in order
	# to change the generation number, before writing our transaction of
	# interest.
	# 
	set count 2
	for { set i 0 } { $i < $count } { incr i } {
		eval rep_test $method $masterenv NULL $niter $start $start 0 $largs
		incr start $niter
		process_msgs $envlist

		set tmp $masterenv
		set masterenv $clientenv
		set clientenv $tmp
		$clientenv rep_start -client
		$masterenv rep_start -master
		process_msgs $envlist
	}

	set omethod [convert_method $method]
	set dbargs [convert_args $method $largs]
	set db [eval berkdb_open_noerr -env $masterenv -auto_commit \
		 $omethod $dbargs $dbname ]
	set txn [$masterenv txn -token]
	$db put -txn $txn "some key" "some data"
	set token [$txn commit]

	eval rep_test $method $masterenv NULL $niter $start $start 0 $largs
	incr start $niter
	process_msgs $envlist

	# Cycle the master gen a few more times, in order to force the token gen
	# not to be at the top of the history.  Again, the count is arbitrary.
	# 
	$db close
	set count 3
	for { set i 0 } { $i < $count } { incr i } {
		set tmp $masterenv
		set masterenv $clientenv
		set clientenv $tmp
		$clientenv rep_start -client
		$masterenv rep_start -master
		process_msgs $envlist

		eval rep_test $method $masterenv NULL $niter $start $start 0 $largs
		incr start $niter
		process_msgs $envlist
	}

	# Note that the choice of which existing directory ("a" or "b") we use
	# as the source of the copy doesn't matter.
	# 
	exec $util_path/db_hotbackup -h $dira -b $clientdir2

	set count 2
	for { set i 0 } { $i < $count } { incr i } {
		set tmp $masterenv
		set masterenv $clientenv
		set clientenv $tmp
		$clientenv rep_start -client
		$masterenv rep_start -master
		process_msgs $envlist

		eval rep_test $method $masterenv NULL $niter $start $start 0 $largs
		incr start $niter
		process_msgs $envlist
	}

	# Create a second transaction, after a few more master gen changes, and
	# get its token too.
	# 
	set db [eval berkdb_open_noerr -env $masterenv -auto_commit \
		 $omethod $dbargs $dbname ]
	set txn [$masterenv txn -token]
	$db put -txn $txn "other key" "different data"
	set token2 [$txn commit]

	# Start the new client, by running "catastrophic" recovery on the
	# hot-backup files.
	# 
	exec $util_path/db_recover -c -h $clientdir2
	repladd 3
	set cl2_envcmd "berkdb_env_noerr -create -txn -errpfx CLIENT2 \
	    $repmemargs \
	    $verbargs -home $clientdir2 -rep_transport \[list 3 replsend\]"
	set clientenv2 [eval $cl2_envcmd -rep_client]
	$clientenv2 rep_config {autoinit off}

	# Before syncing with the master, we should be able to confirm the first
	# transaction, but not the second one, because the hot backup should
	# include an (old) copy of the LSN history database.
	#
	if {$repfiles_in_memory} {
		set result [$clientenv2 txn_applied $token]
		error_check_good still_waiting [is_substr $result DB_TIMEOUT] 1
		set result2 [$clientenv2 txn_applied $token2]
		error_check_good not_yet_applied [is_substr $result2 DB_TIMEOUT] 1
	} else {
		error_check_good txn_applied [$clientenv2 txn_applied $token] 0
		set result2 [$clientenv2 txn_applied $token2]
		error_check_good not_yet_applied [is_substr $result2 DB_NOTFOUND] 1
	}

	# Sync with master, and this time token2 should be there.
	# 
	lappend envlist [list $clientenv2 3]
	process_msgs $envlist
	error_check_good txn_applied2 [$clientenv2 txn_applied $token2] 0

	$db close
	$clientenv2 close
	$clientenv close
	$masterenv close

	replclose $testdir/MSGQUEUEDIR
}

# Test detection of rollbacks.
# 
proc rep091c_sub { method niter tnum future_gen dbname largs } {
	global rep_verbose
	global testdir
	global verbose_type
	global repfiles_in_memory

	puts "Rep$tnum: read-your-writes consistency, rollbacks."

	set verbargs ""
	if { $rep_verbose == 1 } {
		set verbargs " -verbose {$verbose_type on} "
	}

	set repmemargs ""
	if { $repfiles_in_memory } {
		set repmemargs "-rep_inmem_files "
	}

	env_cleanup $testdir

	replsetup $testdir/MSGQUEUEDIR

	set masterdir $testdir/MASTERDIR
	set clientdir $testdir/CLIENTDIR
	set clientdir2 $testdir/CLIENTDIR2

	file mkdir $masterdir
	file mkdir $clientdir
	file mkdir $clientdir2

	puts "\tRep$tnum.a: Create a group of three."
	repladd 1
	set ma_envcmd "berkdb_env_noerr -create -txn -errpfx MASTER \
	    $repmemargs \
	    $verbargs -home $masterdir -rep_transport \[list 1 replsend\]"
	set masterenv [eval $ma_envcmd -rep_master]

	repladd 2
	set cl_envcmd "berkdb_env_noerr -create -txn -errpfx CLIENT \
	    $repmemargs \
	    $verbargs -home $clientdir -rep_transport \[list 2 replsend\]"
	set clientenv [eval $cl_envcmd -rep_client]

	repladd 3
	set cl2_envcmd "berkdb_env_noerr -create -txn -errpfx CLIENT2 \
	    $repmemargs \
	    $verbargs -home $clientdir2 -rep_transport \[list 3 replsend\]"
	set clientenv2 [eval $cl2_envcmd -rep_client]

	set envlist "{$masterenv 1} {$clientenv 2} {$clientenv2 3}"
	process_msgs $envlist

	set start 0
	eval rep_test $method $masterenv NULL $niter $start $start 0 $largs
	incr start $niter
	process_msgs $envlist

	# If desired, test the case where the token has a "future" gen, by
	# bumping of the gen a few times after cutting off the disconnected
	# client.
	# 
	if { $future_gen } {
		for { set count 3 } { $count > 0 } { incr count -1 } {
			$masterenv close
			set masterenv [eval $ma_envcmd -rep_master -recover]
			eval rep_test $method $masterenv NULL \
			    $niter $start $start 0 $largs
			incr start $niter
		}
		set envlist "{$masterenv 1} {$clientenv 2} {$clientenv2 3}"
	}

	# Write some more transactions, taking a token for one of them, but
	# prevent one of the clients from seeing any of them.
	# 
	puts "\tRep$tnum.b: Write transactions, and get token."
	eval rep_test $method $masterenv NULL $niter $start $start 0 $largs
	incr start $niter
	set omethod [convert_method $method]
	set dbargs [convert_args $method $largs]
	set db [eval berkdb_open_noerr -env $masterenv -auto_commit \
		 $omethod $dbargs $dbname ]
	set txn [$masterenv txn -token]
	$db put -txn $txn "some key" "some data"
	set token [$txn commit]
	$db close
	eval rep_test $method $masterenv NULL $niter $start $start 0 $largs
	incr start $niter
	replclear 2
	process_msgs $envlist

	puts "\tRep$tnum.c: Switch master, forcing a rollback."
	$masterenv close
# 	$clientenv rep_start -master
	$clientenv rep_start -client; # 
	set envlist "{$clientenv 2} {$clientenv2 3}"; # 
	process_msgs $envlist;			      # 
	$clientenv rep_start -master;		      # 
	eval rep_test $method $clientenv NULL $niter $start $start 0 $largs
	incr start $niter
	replclear 1
# 	set envlist "{$clientenv 2} {$clientenv2 3}"
	process_msgs $envlist

	puts "\tRep$tnum.d: Check txn_applied at client and at (new) master."
	set result [$clientenv2 txn_applied $token]
	error_check_good rolled_back [is_substr $result DB_NOTFOUND] 1

	set result [$clientenv txn_applied $token]
	error_check_good rolled_back2 [is_substr $result DB_NOTFOUND] 1

	$clientenv close
	$clientenv2 close
	replclose $testdir/MSGQUEUEDIR
}

# Test envid check.  Simulate a network partition: two masters proceed at the
# same gen, in the same LSN range.  The envid would be the only way we would
# know that a transaction from the disconnected master is not correctly applied
# at a client in the other partition.
# 
proc rep091d_sub { method niter tnum extra_gen dbname largs } {
	global rep_verbose
	global testdir
	global verbose_type
	global repfiles_in_memory

	puts -nonewline "Rep$tnum: read-your-writes consistency, partition"
	if { $extra_gen } {
		puts ", with extra gen."
	} else {
		puts "."
	}

	set verbargs ""
	if { $rep_verbose == 1 } {
		set verbargs " -verbose {$verbose_type on} "
	}

	set repmemargs ""
	if { $repfiles_in_memory } {
		set repmemargs "-rep_inmem_files "
	}

	env_cleanup $testdir

	replsetup $testdir/MSGQUEUEDIR

	set masterdir $testdir/MASTERDIR
	set clientdir $testdir/CLIENTDIR
	set clientdir2 $testdir/CLIENTDIR2

	file mkdir $masterdir
	file mkdir $clientdir
	file mkdir $clientdir2

	puts "\tRep$tnum.a: Create a group of three."
	repladd 1
	set ma_envcmd "berkdb_env_noerr -create -txn -errpfx MASTER \
	    $repmemargs \
	    $verbargs -home $masterdir -rep_transport \[list 1 replsend\]"
	set masterenv [eval $ma_envcmd -rep_master]

	repladd 2
	set cl_envcmd "berkdb_env_noerr -create -txn -errpfx CLIENT \
	    $repmemargs \
	    $verbargs -home $clientdir -rep_transport \[list 2 replsend\]"
	set clientenv [eval $cl_envcmd -rep_client]

	repladd 3
	set cl2_envcmd "berkdb_env_noerr -create -txn -errpfx CLIENT2 \
	    $repmemargs \
	    $verbargs -home $clientdir2 -rep_transport \[list 3 replsend\]"
	set clientenv2 [eval $cl2_envcmd -rep_client]

	set envlist "{$masterenv 1} {$clientenv 2} {$clientenv2 3}"
	process_msgs $envlist

	set start 0
	eval rep_test $method $masterenv NULL $niter $start $start 0 $largs
	incr start $niter
	process_msgs $envlist

	# Bounce master and client1.  Then when they come back up, pretend
	# client1 is disconnected, and somehow decides to act as a master (an
	# application not using elections, obviously).  Note that we must
	# replclear appropriately in both directions.
	# 
	$masterenv close
	$clientenv close
	set masterenv [eval $ma_envcmd -rep_master -recover]
	replclear 2
	set clientenv [eval $cl_envcmd -rep_master -recover]
	replclear 1
	replclear 3
	set envlist "{$masterenv 1} {$clientenv2 3}"

	puts "\tRep$tnum.b: Run identical series of txns at two masters."
	set orig_start $start
	eval rep_test $method $masterenv NULL $niter $start $start 0 $largs
	incr start $niter
	set omethod [convert_method $method]
	set dbargs [convert_args $method $largs]
	set db [eval berkdb_open_noerr -env $masterenv -auto_commit \
		 $omethod $dbargs $dbname ]
	set txn [$masterenv txn]
	$db put -txn $txn "some key" "some data"
	$txn commit
	$db close
	eval rep_test $method $masterenv NULL $niter $start $start 0 $largs
	replclear 2
	process_msgs $envlist

	set start $orig_start
	eval rep_test $method $clientenv NULL $niter $start $start 0 $largs
	incr start $niter
	set db [eval berkdb_open_noerr -env $clientenv -auto_commit \
		 $omethod $dbargs $dbname ]
	set txn [$clientenv txn -token]
	$db put -txn $txn "some key" "some data"
	set token [$txn commit]
	$db close
	eval rep_test $method $clientenv NULL $niter $start $start 0 $largs
	incr start $niter
	replclear 1
	replclear 3

	if { $extra_gen } {
		$masterenv close
		set masterenv [eval $ma_envcmd -rep_master -recover]
		set envlist "{$masterenv 1} {$clientenv2 3}"
		eval rep_test $method $masterenv NULL $niter $start $start 0 $largs
		replclear 2
		process_msgs $envlist
	}

	puts "\tRep$tnum.c: Check txn_applied."
	set result [$clientenv2 txn_applied $token]
	error_check_good not_found [is_substr $result DB_NOTFOUND] 1

	$clientenv close
	$clientenv2 close
	$masterenv close
	replclose $testdir/MSGQUEUEDIR
}

# Test the simplified, degenerate behavior of a non-replication environment: a
# valid transaction is always considered applied, unless it disappears in a
# backup/restore.
# 
proc rep091e_sub { method niter tnum dbname largs } {
	global testdir
	global util_path

	set backupdir "$testdir/backup"
	file mkdir $backupdir

	puts "Rep$tnum: read-your-writes consistency, non-rep env."
	env_cleanup $testdir

	set dir $testdir
	set envcmd "berkdb_env_noerr -create -txn -home $dir"
	set env [eval $envcmd]

	set start 0
	eval rep_test $method $env NULL $niter $start $start 0 $largs
	incr start $niter

	exec $util_path/db_hotbackup -h $testdir -b $backupdir

	set omethod [convert_method $method]
	set dbargs [convert_args $method $largs]
	set db [eval berkdb_open_noerr -env $env -auto_commit \
		 $omethod $dbargs $dbname ]
	set txn [$env txn -token]
	$db put -txn $txn "some key" "some data"
	set token [$txn commit]
	$db close

	eval rep_test $method $env NULL $niter $start $start 0 $largs
	incr start $niter

	error_check_good applied [$env txn_applied $token] 0
	$env close
	
	set envcmd "berkdb_env_noerr -create -txn -home $backupdir -recover_fatal"
	set env [eval $envcmd]
	set result [$env txn_applied $token]
	error_check_good restored [is_substr $result DB_NOTFOUND] 1
	$env close
}

# Check proper behavior of txn_applied() at a client before in-mem DB's have
# been materialized.  This is only relevant when repfiles_in_memory, so skip it
# in the default case.  Sites "A" and "B" will take turns being master, and
# CLIENT2 will be the one whose behavior is under test.
# 
proc rep091f_sub { method niter tnum dbname largs } {
	global rep_verbose
	global testdir
	global verbose_type
	global util_path
	global repfiles_in_memory

	if { $repfiles_in_memory } {
		set repmemargs "-rep_inmem_files "
		puts "Rep$tnum: read-your-writes consistency, missing in-mem DB."
	} else {
		puts "Rep$tnum: skipping missing in-mem DB test."
		return
	}

	set verbargs ""
	if { $rep_verbose == 1 } {
		set verbargs " -verbose {$verbose_type on} "
	}

	env_cleanup $testdir

	replsetup $testdir/MSGQUEUEDIR

	set dira $testdir/DIRA
	set dirb $testdir/DIRB
	set clientdir2 $testdir/CLIENTDIR2

	file mkdir $dira
	file mkdir $dirb
	file mkdir $clientdir2

	repladd 1
	set envcmd_a "berkdb_env_noerr -create -txn -errpfx SITE_A \
	    $repmemargs \
	    $verbargs -home $dira -rep_transport \[list 1 replsend\]"
	set masterenv [eval $envcmd_a -rep_master]

	repladd 2
	set envcmd_b "berkdb_env_noerr -create -txn -errpfx SITE_B \
	    $repmemargs \
	    $verbargs -home $dirb -rep_transport \[list 2 replsend\]"
	set clientenv [eval $envcmd_b -rep_client]

	repladd 3
	set cl2_envcmd "berkdb_env_noerr -create -txn -errpfx CLIENT2 \
	    $repmemargs \
	    $verbargs -home $clientdir2 -rep_transport \[list 3 replsend\]"
	set clientenv2 [eval $cl2_envcmd -rep_client]

	set envlist "{$masterenv 1} {$clientenv 2} {$clientenv2 3}"
	process_msgs $envlist

	set start 0
	eval rep_test $method $masterenv NULL $niter $start $start 0 $largs
	incr start $niter

	# Write the transaction of interest in gen 1.
	# 
	set omethod [convert_method $method]
	set dbargs [convert_args $method $largs]
	set db [eval berkdb_open_noerr -env $masterenv -auto_commit \
		 $omethod $dbargs $dbname ]
	set txn [$masterenv txn -token]
	$db put -txn $txn "some key" "some data"
	set token [$txn commit]
	$db close

	eval rep_test $method $masterenv NULL $niter $start $start 0 $largs
	incr start $niter
	process_msgs $envlist

	set tmp $masterenv
	set masterenv $clientenv
	set clientenv $tmp
	$clientenv rep_start -client
	$masterenv rep_start -master
	process_msgs $envlist

	eval rep_test $method $masterenv NULL $niter $start $start 0 $largs
	incr start $niter
	process_msgs $envlist
	
	# Bounce the client.  Even though it has the transaction of interest,
	# and had observed the change to gen 2, when we restart it it won't have
	# its LSN history database.
	#
	$clientenv2 close
	set clientenv2 [eval $cl2_envcmd -rep_client -recover]

	set result [$clientenv2 txn_applied $token]
	error_check_good not_yet [is_substr $result DB_TIMEOUT] 1

	# Sync with master, and this time token should be confirmed.
	# 
	set envlist [lreplace $envlist end end [list $clientenv2 3]]
	process_msgs $envlist
	error_check_good txn_applied [$clientenv2 txn_applied $token] 0

	$clientenv2 close
	$clientenv close
	$masterenv close

	replclose $testdir/MSGQUEUEDIR
}
