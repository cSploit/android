# See the file LICENSE for redistribution information.
#
# Copyright (c) 2010, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST  rep096
# TEST	Replication and db_replicate utility.
# TEST
# TEST	Create a master and client environment.  Open them.
# TEST	Start a db_replicate process on each.  Create a database on
# TEST	the master and write some data.  Then verify contents.
proc rep096 { method { niter 100 } { tnum "096" } args } {

	source ./include.tcl
	global databases_in_memory 
	global repfiles_in_memory
	global util_path
	global EXE

	# All access methods are allowed.
	if { $checking_valid_methods } {
		return "ALL"
	}

	# QNX does not support fork() in a multi-threaded environment.
	if { $is_qnx_test } {
		puts "Skipping Rep$tnum on QNX."
		return
	}

	if { [file exists $util_path/db_replicate$EXE] == 0 } {
		puts "Skipping Rep$tnum with db_replicate.  Is it built?"
		return
	}

	set logsets [create_logsets 2]

	set args [convert_args $method $args]

	set msg "using on-disk databases"
	if { $databases_in_memory } {
		set msg "using named in-memory databases"
		if { [is_queueext $method] } { 
			puts -nonewline "Skipping rep$tnum for method "
			puts "$method with named in-memory databases."
			return
		}
	}

	set msg2 "and on-disk replication files"
	if { $repfiles_in_memory } {
		set msg2 "and in-memory replication files"
	}

	# Run the body of the test with and without recovery.
	foreach r $test_recopts {
		foreach l $logsets {
			set logindex [lsearch -exact $l "in-memory"]
			if { $r == "-recover" && $logindex != -1 } {
				puts "Rep$tnum: Skipping for in-memory logs\
				    with -recover."
				continue
			}
			puts "Rep$tnum ($method $r): Db_replicate and\
			    non-rep env handles $msg $msg2."
			puts "Rep$tnum: Master logs are [lindex $l 0]"
			puts "Rep$tnum: Client logs are [lindex $l 1]"
			rep096_sub $method $niter $tnum $l $r $args
		}
	}
}

proc rep096_sub { method niter tnum logset recargs largs } {
	source ./include.tcl
	global testdir
	global is_hp_test
	global databases_in_memory
	global repfiles_in_memory
	global rep_verbose
	global verbose_type

	set verbargs ""
	set varg ""
	if { $rep_verbose == 1 } {
		set verbargs " -verbose {$verbose_type on} "
		set varg " -v "
	}

	set repmemargs ""
	if { $repfiles_in_memory } {
		set repmemargs "-rep_inmem_files "
	}

	env_cleanup $testdir

	set masterdir $testdir/MASTERDIR
	set clientdir $testdir/CLIENTDIR

	file mkdir $masterdir
	file mkdir $clientdir

	set m_logtype [lindex $logset 0]
	set c_logtype [lindex $logset 1]

	# In-memory logs require a large log buffer, and cannot
	# be used with -txn nosync.
	set m_logargs [adjust_logargs $m_logtype]
	set c_logargs [adjust_logargs $c_logtype]
	set m_txnargs [adjust_txnargs $m_logtype]
	set c_txnargs [adjust_txnargs $c_logtype]

	if { $databases_in_memory } {
		set dbname { "" "test.db" }
	} else { 
		set dbname "test.db"
	} 

	#
	# Make DB_CONFIG files in each directory.  Set a client priority
	# of 1 so that it sends acks and the master expects acks.
	#
	puts "\tRep$tnum.a: Creating initial environments."
	set nsites 2
	replicate_make_config $masterdir 0 100
	replicate_make_config $clientdir 1 1

	#
	# Open a master and a client, but only with -rep.  Note that we need
	# to specify -thread also.
	#
	set max_locks 2500
	set env_cmd(M) "berkdb_env_noerr -create -log_max 1000000 \
	    -lock_max_objects $max_locks -lock_max_locks $max_locks \
	    -home $masterdir -errpfx MASTER $verbargs $repmemargs \
	    $m_txnargs $m_logargs -rep -thread"
	set masterenv [eval $env_cmd(M) $recargs]

	set env_cmd(C) "berkdb_env_noerr -create $c_txnargs $c_logargs \
	    -lock_max_objects $max_locks -lock_max_locks $max_locks \
	    -home $clientdir -errpfx CLIENT $verbargs $repmemargs -rep -thread"
	set clientenv [eval $env_cmd(C) $recargs]

	#
	# Now start db_replicate on each site.
	#
	puts "\tRep$tnum.b: Start db_replicate on each env."
	set dpid(M) [eval {exec $util_path/db_replicate -h $masterdir} \
	    -M -t 5 $varg &]
	set dpid(C) [eval {exec $util_path/db_replicate -h $clientdir} $varg &]

	await_startup_done $clientenv

	#
	# Force a checkpoint from a subordinate connection.  However,
	# the checkpoint log records will likely get lost prior to the
	# connection getting established.
	#
	puts "\tRep$tnum.c: Force checkpoint from non-rep process."
	set cid [exec $util_path/db_checkpoint -h $masterdir -1]

	#
	# Wait for the master and client LSNs to match after this
	# checkpoint.  That might mean waiting for a rerequest
	# to run or db_replicate to call rep_flush.
	#
	await_condition \
	    {[stat_field $masterenv rep_stat "Next LSN expected"] == \
	    [stat_field $clientenv rep_stat "Next LSN expected"]}

	if { $is_freebsd_test == 0 } {
		#
		# Now perform operations using this Tcl process with
		# subordinate connections.  This does not work with FreeBSD.
		#
		set omethod [convert_method $method]
		set db [eval berkdb_open_noerr -create -env $masterenv \
		    -auto_commit -mode 0644 $largs $omethod $dbname]
		error_check_good db_open [is_valid_db $db] TRUE

		await_condition \
		    {[stat_field $masterenv rep_stat "Next LSN expected"] == \
		    [stat_field $clientenv rep_stat "Next LSN expected"]}

		if { !$databases_in_memory } {
			error_check_good client_db \
			    [file exists $clientdir/$dbname] 1
		} 

		# Run a modified test001 in the master (and update client).
		puts "\tRep$tnum.d: Running rep_test in replicated env."
		eval rep_test $method $masterenv $db $niter 0 0 0 $largs
	
		await_condition \
		    {[stat_field $masterenv rep_stat "Next LSN expected"] == \
		    [stat_field $clientenv rep_stat "Next LSN expected"]}

		# Check that databases are in-memory or on-disk as expected.
		check_db_location $masterenv
		check_db_location $clientenv
	
		$db close
		rep_verify $masterdir $masterenv $clientdir $clientenv 1 1 1
	} else {
		puts "\tRep$tnum.d: Force 2nd checkpoint from non-rep process."
		set cid [exec $util_path/db_checkpoint -h $masterdir -1]
	}

	puts "\tRep$tnum.e: Await final processing."
	await_condition \
	    {[stat_field $masterenv rep_stat "Next LSN expected"] == \
	    [stat_field $clientenv rep_stat "Next LSN expected"]}

	tclkill $dpid(C)
	tclkill $dpid(M)

	error_check_good masterenv_close [$masterenv close] 0
	error_check_good clientenv_close [$clientenv close] 0
}
