# See the file LICENSE for redistribution information.
#
# Copyright (c) 2012, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST  rep107
# TEST
# TEST	Replication and basic view error test.
# TEST	Have a master, a client and a view.
# TEST	Test for various error conditions and restrictions, including
# TEST	having a view call rep_elect; trying to demote a client to a
# TEST	view after opening the env; inconsistent view opening; trying
# TEST	to make it a master, etc.
#
proc rep107 { method { tnum "107" } args } {
	source ./include.tcl
	global env_private
	global repfiles_in_memory

	# Run for btree only.
	if { $checking_valid_methods } {
		set test_methods { btree }
		return $test_methods
	}
	if { [is_btree $method] == 0 } {
		puts "\tRep$tnum: Skipping for method $method."
		return
	}

	# Skip test for HP-UX because we can't open an env twice.
	if { $is_hp_test == 1 } {
		puts "\tRep$tnum: Skipping for HP-UX."
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
	set logsets [create_logsets 3]

	# Run the body of the test with and without recovery,
	# Skip recovery with in-memory logging - it doesn't make sense.
	set views { full none }
	foreach v $views {
		foreach r $test_recopts {
			foreach l $logsets {
				set logindex [lsearch -exact $l "in-memory"]
				if { $r == "-recover" && $logindex != -1 } {
					puts "Skipping rep$tnum for -recover\
				    	with in-memory logs."
					continue
				}
				puts "Rep$tnum ($method $r view($v)):\
	    Replication and views checking error conditions $msg2 $msg3."
				puts "Rep$tnum: Master logs are [lindex $l 0]"
				puts "Rep$tnum: Client logs are [lindex $l 1]"
				puts "Rep$tnum: View logs are [lindex $l 2]"
				rep107_sub $method $tnum $l $r $v $args
			}
		}
	}
}

proc rep107_sub { method tnum logset recargs view largs } {
	source ./include.tcl
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

	set m_logtype [lindex $logset 0]
	set c_logtype [lindex $logset 1]
	set view_logtype [lindex $logset 2]

	# In-memory logs require a large log buffer, and cannot
	# be used with -txn nosync.
	set m_logargs [adjust_logargs $m_logtype]
	set c_logargs [adjust_logargs $c_logtype]
	set view_logargs [adjust_logargs $view_logtype]
	set m_txnargs [adjust_txnargs $m_logtype]
	set c_txnargs [adjust_txnargs $c_logtype]
	set view_txnargs [adjust_txnargs $view_logtype]

	# Set nsites; the view site does not count.
	set nsites 2

	# Open a master.
	repladd 2
	set envcmd(0) "berkdb_env -create $m_txnargs $m_logargs \
	    $verbargs -errpfx MASTER -home $masterdir \
	    -rep_nsites $nsites $repmemargs $privargs \
	    -event -rep_master -rep_transport \[list 2 replsend\]"
	set masterenv [eval $envcmd(0) $recargs]
	error_check_good master_env [is_valid_env $masterenv] TRUE

	# Open two clients, one of which is a view.
	repladd 3
	set envcmd(1) "berkdb_env_noerr -create $c_txnargs $c_logargs \
	    $verbargs -errpfx CLIENT -home $clientdir -rep_nsites $nsites \
	    $repmemargs $privargs -event -rep_client \
	    -rep_transport \[list 3 replsend\]"
	set clientenv [eval $envcmd(1) $recargs]
	error_check_good client_env [is_valid_env $clientenv] TRUE

	#
	# Make this client the view site.  
	#
	# Set the view callback to the BDB default (full) view or the
	# Tcl proc that replicates none, replview_none.
	#
	if { $view == "full" } {
		set viewcb ""
	} else {
		set viewcb replview_none
	}
	repladd 4
	#
	# Omit the role (rep_client or rep_master), rep_view and
	# recovery options from the saved command so that we can
	# try other, illegal combinations later.
	#
	set envcmd(2) "berkdb_env_noerr -create $view_txnargs $view_logargs \
	    $verbargs -errpfx VIEW -home $viewdir -rep_nsites $nsites \
	    $repmemargs $privargs -event -rep_transport \[list 4 replsend\]"
	set viewenv [eval $envcmd(2) -rep_client \
	    -rep_view \[list $viewcb \] $recargs]
	error_check_good view_env [is_valid_env $viewenv] TRUE

	# Bring the clients online by processing the startup messages.
	set envlist "{$masterenv 2} {$clientenv 3} {$viewenv 4}"
	process_msgs $envlist

	#
	# Test situations that will return an error. 
	#
	#	- Try to make the view site a master.
	#	- Call rep_elect from the view site.
	#	- Try to open a 2nd env handle configured as a
	#	  regular client, to a view env.
	#	- Try to open a 2nd env handle configured as a
	#	  view, to a regular client's env.
	#	- Close the view and reopen as a regular client.
	#	- Retry rep_elect, and rep_start(Master) with new handle.
	#	- Make sure non-rep env handles can operate correctly.
	#	(via utility like db_stat, and non-rep local handle)
	#

	#
	# Try to make the view a master.
	#
	puts "\tRep$tnum.a: Try to upgrade in-use view env to a master."
	set res [catch {$viewenv rep_start -master} ret]
	error_check_bad view_master $res 0
	error_check_good view_master1 [is_substr $ret "invalid argument"] 1

	#
	# Try to call rep_elect from view env.  Normally we call rep_elect
	# from a child process but we don't need to do that here because
	# we expect an immediate error return.
	#
	puts "\tRep$tnum.b: Call rep_elect from view env."
	set timeout 1000000
	set mypri 0
	set res [catch {$viewenv rep_elect $nsites $nsites $mypri $timeout} ret]
	error_check_bad view_elect $res 0
	error_check_good view_elect1 [is_substr $ret "invalid argument"] 1

	#
	# Try to open a 2nd env handle to the view env, but make it
	# a regular client.
	#
	puts "\tRep$tnum.c: Open 2nd inconsistent handle to view env."
	set res [catch {eval $envcmd(2) -rep_client} ret]
	error_check_bad view_client $res 0
	error_check_good view_client1 [is_substr $ret "invalid argument"] 1

	error_check_good viewenv_close [$viewenv close] 0
	set viewenv NULL
	if { $repfiles_in_memory == 0 } {
		puts "\tRep$tnum.d: Recover view, try to open as master."
		set res [catch {eval $envcmd(2) -recover -rep_master} ret]
		error_check_bad newview_master $res 0
		error_check_good newview_master1 [is_substr $ret \
		    "invalid argument"] 1

		puts "\tRep$tnum.e: Try to reopen view as regular client."
		set res [catch {eval $envcmd(2) -recover -rep_client} ret]
		error_check_bad newview_client $res 0
		error_check_good newview_client1 [is_substr $ret \
		    "invalid argument"] 1

		#
		# Confirm this site is still known to be a view.  The original
		# view env handle is closed.  We've recovered the env above,
		# because recovery happens before returning the expected errors.
		# But opening the env with DB_INIT_REP should call rep_open and
		# that should figure out we're a view.
		#
		puts "\tRep$tnum.f: Verify non-rep handles can use view env."
		set viewenv [eval $envcmd(2)]
		error_check_good newview_env [is_valid_env $viewenv] TRUE
		set isview [stat_field $viewenv rep_stat "Is view"]
		error_check_good isview $isview 1
	
		# Skip calls to db_stat for env -private -- it can't work.
		if { !$env_private } {
			set stat [catch\
			    {exec $util_path/db_stat -N -RA -h $viewdir} ret]
			error_check_good db_stat $stat 0
			error_check_good db_statout [is_substr $ret \
			    "Environment configured as view site"] 1
		}
	}

	#
	# Try to open a 2nd env handle to the client env, but make it
	# a view.  I.e. try to demote while the other client is open.
	#
	puts "\tRep$tnum.g: Try to reset view status on non-view env."
	set isview [stat_field $clientenv rep_stat "Is view"]
	error_check_good isview $isview 0
	if { !$env_private } {
		puts "\t\tRep$tnum.g1: Open 2nd inconsistent handle on client."
		set res [catch {eval $envcmd(1) -rep_view \[list\]} ret]
		error_check_bad client $res 0
		error_check_good client1 [is_substr $ret "invalid argument"] 1
	}
	puts "\t\tRep$tnum.g2: Check view status via stat."
	set isview [stat_field $clientenv rep_stat "Is view"]
	error_check_good isview_stat $isview 0
	if { !$env_private } {
		set stat [catch\
		    {exec $util_path/db_stat -N -RA -h $clientdir} ret]
		error_check_good db_statout \
		    [is_substr $ret "Environment not configured as view site"] 1
	}

	#
	# Try to open a 2nd env handle to the client env after a clean
	# close, but without running recovery.  We only verify via
	# db_stat since we are closing the environment handle.
	#
	puts "\tRep$tnum.h: Try to reset view status on closed non-view env."
	error_check_good cenv_close [$clientenv close] 0
	if { !$env_private } {
		puts "\t\tRep$tnum.h1: Reopen with inconsistent handle on client."
		set res [catch {eval $envcmd(1) -rep_view \[list\]} ret]
		error_check_bad client $res 0
		error_check_good client1 [is_substr $ret "invalid argument"] 1
		puts "\t\tRep$tnum.h2: Check view status via stat."
		set stat [catch\
		    {exec $util_path/db_stat -N -RA -h $clientdir} ret]
		error_check_good db_statout \
		    [is_substr $ret "Environment not configured as view site"] 1
	}

	#
	# Demote a client by reopening as a view with recovery.
	# This should work.
	#
	puts "\tRep$tnum.i: Demote client to view after recovery."
	set res [catch {eval $envcmd(1) -recover \
	    -rep_view \[list $viewcb \]} v2env]
	error_check_bad client $res 1
	error_check_good v2env [is_valid_env $v2env] TRUE
	puts "\t\tRep$tnum.i1: Check view status via stat."
	if { !$env_private } {
		set stat [catch\
		    {exec $util_path/db_stat -N -RA -h $clientdir} ret]
		error_check_good db_statout \
		    [is_substr $ret "Environment configured as view site"] 1
	}
	set isview [stat_field $v2env rep_stat "Is view"]
	error_check_good isview $isview 1

	error_check_good masterenv_close [$masterenv close] 0
	error_check_good cenv_close [$v2env close] 0

	# Viewenv won't be open for inmem rep at this point.
	if { $viewenv != "NULL" } {
		error_check_good view_close [$viewenv close] 0
	}

	replclose $testdir/MSGQUEUEDIR
}
