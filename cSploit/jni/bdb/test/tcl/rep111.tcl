# See the file LICENSE for redistribution information.
#
# Copyright (c) 2012, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST  rep111
# TEST
# TEST	Replication and partial view and client-to-client synchronization.
# TEST	Start up master and view.  Create files and make sure
# TEST	the correct files appear on the view.  Start client site and
# TEST	confirm the view serves client-to-client.
#
proc rep111 { method { niter 100 } { tnum "111" } args } {
	source ./include.tcl
	global databases_in_memory
	global env_private
	global has_crypto
	global passwd
	global repfiles_in_memory

	# Valid for all access methods.
	if { $checking_valid_methods } {
		return "btree"
	}

        if { [is_btree $method] == 0 } {
		puts "Rep$tnum: skipping for non-btree method $method."
		return
	}

	set msg "using on-disk databases"
	#
	# Partial replication does not support in-memory databases.
	#
	if { $databases_in_memory } {
		puts -nonewline "Skipping rep$tnum for method "
		puts "$method for named in-memory databases."
		return
	}

	set msg2 "and on-disk replication files"
	if { $repfiles_in_memory } {
		set msg2 "and in-memory replication files"
	}

	set msg3 ""
	if { $env_private } {
		set msg3 "with private env"
	}

	set args [convert_args $method $args]
	set saved_args $args
	set logsets [create_logsets 3]

	set views { none odd full }
	foreach v $views {
		foreach l $logsets {
			set envargs ""
			set args $saved_args
			puts "Rep$tnum ($method $envargs $args view($v)):\
	Replication, views and client-to-client sync $msg $msg2 $msg3."
			puts "Rep$tnum: Master logs are [lindex $l 0]"
			puts "Rep$tnum: Client logs are [lindex $l 1]"
			puts "Rep$tnum: View logs are [lindex $l 2]"
			rep111_sub $method $niter $tnum $envargs $l $v $args

			# Skip encrypted tests if not supported
			if { $has_crypto == 0 } {
				continue
			}

			# Run same tests with security.
			append envargs " -encryptaes $passwd "
			append args " -encrypt "
			puts "Rep$tnum ($method $envargs $args view($v)):\
	Replication, views and client-to-client sync $msg $msg2 $msg3."
			puts "Rep$tnum: Master logs are [lindex $l 0]"
			puts "Rep$tnum: Client logs are [lindex $l 1]"
			puts "Rep$tnum: View logs are [lindex $l 2]"
			rep111_sub $method $niter $tnum $envargs $l $v $args
		}
	}
}

proc rep111_sub { method niter tnum envargs logset view largs } {
	source ./include.tcl
	global anywhere
	global env_private
	global rep_verbose
	global repfiles_in_memory
	global verbose_type

	set verbargs ""
	if { $rep_verbose == 1 } {
		set verbargs " -verbose {$verbose_type on} "
	}

	set repmemargs ""
	if { $repfiles_in_memory == 1 } {
		set repmemargs " -rep_inmem_files "
	}

	set privargs ""
	if { $env_private } {
		set privargs " -private "
	}

	env_cleanup $testdir

	set qdir $testdir/MSGQUEUEDIR
	replsetup $qdir

	set masterdir $testdir/MASTERDIR
	set clientdir $testdir/CLIENTDIR
	set viewdir $testdir/VIEWDIR

	file mkdir $masterdir
	file mkdir $clientdir
	file mkdir $viewdir

	#
	# Log size is small so we quickly create more than one.
	# The documentation says that the log file must be at least
	# four times the size of the in-memory log buffer.
	set maxpg 16384
	set log_max [expr $maxpg * 8]

	set m_logtype [lindex $logset 0]
	set c_logtype [lindex $logset 1]
	set v_logtype [lindex $logset 2]

	# In-memory logs require a large log buffer, and cannot
	# be used with -txn nosync.
	set m_logargs [adjust_logargs $m_logtype]
	set c_logargs [adjust_logargs $c_logtype]
	set v_logargs [adjust_logargs $v_logtype]
	set m_txnargs [adjust_txnargs $m_logtype]
	set c_txnargs [adjust_txnargs $c_logtype]
	set v_txnargs [adjust_txnargs $v_logtype]

	# Open a master.
	repladd 2
	set ma_envcmd "berkdb_env -create $m_txnargs $m_logargs \
	    $verbargs -errpfx MASTER -home $masterdir $envargs \
	    $repmemargs $privargs -log_max $log_max \
	    -rep_master -rep_transport \[list 2 replsend\]"
	set masterenv [eval $ma_envcmd]
	error_check_good master_env [is_valid_env $masterenv] TRUE

	# Open two clients, one of which is a view.
	# Set up a client command but don't eval until later.
	# !!! Do NOT put the 'repladd' call here because we don't
	# want this client to already have a backlog of records
	# when it starts.
	#
	set cl_envcmd "berkdb_env_noerr -create $c_txnargs $c_logargs \
	    $verbargs -errpfx CLIENT -home $clientdir -log_max $log_max \
	    $repmemargs $privargs -rep_client $envargs \
	    -rep_transport \[list 3 replsend\]"

	#
	# Make 2nd client the view site.  Set the view callback.  
	#
	switch $view {
		"full" { set viewcb "" }
		"none" { set viewcb replview_none }
		"odd" { set viewcb replview_odd }
	}
	repladd 4
	set v_envcmd "berkdb_env_noerr -create $v_txnargs $v_logargs \
	    $verbargs -errpfx VIEW -home $viewdir -log_max $log_max \
	    $repmemargs $privargs -rep_client $envargs \
	    -rep_view \[list $viewcb \] -rep_transport \[list 4 replsend\]"

	set viewenv [eval $v_envcmd]
	error_check_good view_env [is_valid_env $viewenv] TRUE
	set envlist "{$masterenv 2} {$viewenv 4}"
	process_msgs $envlist

	# 
	# Run rep_test several times, each time through, using
	# a different database name.
	#
	set nfiles 5
	puts "\tRep$tnum.a: Running rep_test $nfiles times in replicated env."
	set omethod [convert_method $method]
	#
	# Create the files and write some data to them.
	#
	for { set i 0 } { $i < $nfiles } { incr i } {
		set testfile "test.$i.db"
		puts "\t\tRep$tnum.$i: Creating $testfile."
		set db [eval {berkdb_open_noerr} -env $masterenv -auto_commit \
		    -create -mode 0644 $omethod $largs $testfile]
		error_check_good reptest_db [is_valid_db $db] TRUE
		set mult [expr $i * 10]
		set nentries [expr $niter + $mult]
		eval rep_test $method $masterenv $db $nentries $mult $mult \
		    0 $largs
		error_check_good dbclose [$db close] 0
		process_msgs $envlist
	}

	# Verify the logs on the view.  For now we'll assume
	# the right thing happened with the databases, as we'll
	# do a full verification later.
	rep_verify $masterdir $masterenv $viewdir $viewenv \
	    1 1 1 NULL

	#
	# Now that the view is initialized, open the client.  Turn
	# on client-to-client sync.
	#
	puts "\tRep$tnum.b: Start client.  Sync from view."
	set anywhere 1
	repladd 3
	set clientenv [eval $cl_envcmd]
	error_check_good client_env [is_valid_env $clientenv] TRUE
	set envlist "{$masterenv 2} {$clientenv 3} {$viewenv 4}"
	process_msgs $envlist

	set req [stat_field $viewenv rep_stat "Client service requests"]
	set miss [stat_field $viewenv rep_stat "Client service req misses"]
	set rereq [stat_field $clientenv rep_stat "Client rerequests"]
	puts "\tRep$tnum.c: Verify sync-up from view (req $req, miss $miss)."

	#
	# Confirm the number of rerequests received, and the number
	# of misses based on the databases available at this view.
	# The number of requests received should be at least:
	# number of databases (PAGE_REQs for $nfiles) + 1 (PAGE_REQ
	# for system db) + 2 (LOG_REQ for init and ALL_REQ after init).
	#
	# The number of misses on the viewenv should equal the number
	# of rerequests on the client.  Also a viewenv will record a miss
	# for any database it does not have. 
	#
	# Any client serving log records for an internal init will always
	# record at least one miss.  The reason is that if it gets
	# a LOG_REQ with an end-of-range that points to the very end of the log
	# file, the serving site gets NOTFOUND in its log cursor reading loop
	# and can't tell whether it simply hit the end, or is really missing
	# sufficient log records to fulfill the request.
	# Additionally, for each type of view it should be at most:
	# full: 1 (per explanation above)
	# odd: 1 + ($nfiles / 2 + 1)
	# none: 1 + $nfiles
	#
	error_check_good miss_rereq $miss $rereq
	set expect_req [expr $nfiles + 3]
	error_check_good req [expr $req >= $expect_req] 1
	switch $view {
		"full" { set max_miss 1 }
		"none" { set max_miss [expr $nfiles + 1] }
		"odd" { set max_miss [expr [expr $nfiles / 2] + 2] }
	}

	error_check_good miss [expr $miss <= $max_miss] 1
	#
	# Verify the right files are replicated.
	#
	puts "\tRep$tnum.d.2:  Verify logs and databases."

	# On the client everything should be there.  First compare just the
	# logs and no databases.
	#
	rep_verify $masterdir $masterenv $clientdir $clientenv\
	    1 1 1 NULL
	for { set i 0 } { $i < $nfiles } { incr i } {
		set dbname "test.$i.db"
		rep_verify $masterdir $masterenv $clientdir $clientenv \
		    1 1 0 $dbname
	}

	# On the view everything should be there in the "full" case.
	# In all cases, the 2 non-durable databases should exist.
	# No test databases should exist in the "none" case.
	# Only databases with odd digits should be there for the "odd" case.
	# No matter what the logs should be there and match.
	# We let rep_verify compare the logs, then compare the dbs
	# if they are expected to exist.  Send in NULL to just compare logs.

	rep_verify $masterdir $masterenv $viewdir $viewenv \
	    1 1 1 NULL
	for { set i 0 } { $i < $nfiles } { incr i } {
		set dbname "test.$i.db"
		if { $view == "full" } {
			rep_verify $masterdir $masterenv $viewdir $viewenv \
			    1 1 0 $dbname
		} elseif { $view == "none" } {
			puts "\t\tRep$tnum: $viewdir ($dbname) should not exist"
			error_check_good db$i [file exists $viewdir/$dbname] 0
		} else {
			# odd digit case
			set replicated [string match "*\[13579\]*" $dbname]
			if { $replicated } {
				rep_verify $masterdir $masterenv \
				    $viewdir $viewenv 1 1 0 $dbname
			} else {
				puts \
			"\t\tRep$tnum: $viewdir ($dbname) should not exist"
				error_check_good db$i \
				    [file exists $viewdir/$dbname] 0
			}
		}
	}

	set anywhere 0
	error_check_good masterenv_close [$masterenv close] 0
	error_check_good cenv_close [$clientenv close] 0
	error_check_good view_close [$viewenv close] 0

	replclose $testdir/MSGQUEUEDIR
}
