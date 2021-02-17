# See the file LICENSE for redistribution information.
#
# Copyright (c) 2004, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	rep103
# TEST	Test of multiple data dirs and databases, with different
# TEST	directory structure on master and client.
# TEST
# TEST	One master, two clients using several data_dirs.
# TEST	Create databases in different data_dirs.  Replicate to client
# TEST	that doesn't have the same data_dirs.
# TEST	Add 2nd client later to require it to catch up via internal init.
#
proc rep103 { method { niter 500 } { tnum "103" } args } {
	source ./include.tcl
	global databases_in_memory
	global repfiles_in_memory
	global env_private

	# Valid for all access methods.
	if { $checking_valid_methods } {
		return "ALL"
	}

	set args [convert_args $method $args]
	set logsets [create_logsets 2]

        # This test needs to set its own pagesize.
	set pgindex [lsearch -exact $args "-pagesize"]
	if { $pgindex != -1 } {
		puts "Rep$tnum: skipping for specific pagesizes"
		return
	}

	# This test needs on-disk databases.
	if { $databases_in_memory } {
		puts "Rep$tnum: skipping for in-memory databases"
		return
	}
	set msg "using on-disk databases"

	set msg2 "and on-disk replication files"
	if { $repfiles_in_memory } {
		set msg2 "and in-memory replication files"
	}

	set msg3 ""
	if { $env_private } {
		set msg3 "with private env"
	}

	#
	# Run the body of the test with and without recovery,
	# and with varying directory configurations:
	#   datadir: master databases in data0/data1 client in env_home
	#   overlap0: master databases in data0/data1 client in data0
	#   overlap1: master databases in data0/data1 client in cdata/data1
	#   nooverlap: master databases in data0/data1 client in cdata
	#
	set opts { datadir overlap0 overlap1 nooverlap }
	foreach r $test_recopts {
		foreach c $opts {
			foreach l $logsets {
				set logindex [lsearch -exact $l "in-memory"]
				if { $r == "-recover" && $logindex != -1 } {
					puts "Skipping rep$tnum for -recover\
					    with in-memory logs."
					continue
				}
				puts "Rep$tnum ($method $r $c):\
				    Multiple databases in multiple data_dirs \
				    $msg $msg2 $msg3."
				puts "Rep$tnum: Master logs are [lindex $l 0]"
				puts "Rep$tnum: Client logs are [lindex $l 1]"
				rep103_sub $method $niter $tnum $l $r $c $args
			}
		}
	}
}

proc rep103_sub { method niter tnum logset recargs opts largs } {
	global testdir
	global util_path
	global repfiles_in_memory
	global env_private
	global rep_verbose
	global verbose_type

	set verbargs ""
	if { $rep_verbose == 1 } {
		set verbargs " -verbose {$verbose_type on} "
	}

	set repmemargs ""
	if { $repfiles_in_memory } {
		set repmemargs "-rep_inmem_files "
	}

	set privargs ""
	if { $env_private == 1 } {
		set privargs " -private "
	}

	env_cleanup $testdir

	replsetup $testdir/MSGQUEUEDIR

	set masterdir $testdir/MASTERDIR
	set clientdir $testdir/CLIENTDIR
	set clientdir2 $testdir/CLIENTDIR2

	file mkdir $masterdir
	file mkdir $clientdir
	file mkdir $clientdir2

	# Log size is small so we quickly create more than one.
	# The documentation says that the log file must be at least
	# four times the size of the in-memory log buffer.
	set pagesize 4096
	append largs " -pagesize $pagesize "
	set log_max [expr $pagesize * 8]
	set omethod [convert_method $method]

	set m_logtype [lindex $logset 0]
	set c_logtype [lindex $logset 1]

	# In-memory logs cannot be used with -txn nosync.
	set m_logargs [adjust_logargs $m_logtype]
	set c_logargs [adjust_logargs $c_logtype]
	set m_txnargs [adjust_txnargs $m_logtype]
	set c_txnargs [adjust_txnargs $c_logtype]

	# Set up data directories for the various configurations.
	set nfiles 2
	set m_create_dirs {data0 data1}
	file mkdir $masterdir/data0
	file mkdir $masterdir/data1
	set data_diropts {-data_dir data0 -data_dir data1}

	#
	# Now set up a different client data_dir structure based on
	# which option we're running this time.  We will create two
	# databases, each in a different data_dir on the master.
	# Figure out where each db will be on the client.
	#
	# We use 2 identical clients.  One we use at creation time to
	# make sure processing creation records works correctly.  The
	# 2nd client is started after archiving the creation records to
	# make sure internal init creates the files correctly.
	#
	set expect_dir {}
	set expect2_dir {}
	set data_c_diropts ""
	switch $opts {
		"datadir" {
			# Client has no data_dirs.  Both dbs in env home.
			set expect_dir [list $clientdir $clientdir]
			set expect2_dir [list $clientdir2 $clientdir2]
		}
		"overlap0" {
			#
			# Client only has data0.  Both dbs in data0.
			#
			file mkdir $clientdir/data0
			file mkdir $clientdir2/data0
			set expect_dir [list $clientdir/data0 $clientdir/data0]
			set expect2_dir \
			    [list $clientdir2/data0 $clientdir2/data0]
			lappend data_c_diropts -data_dir
			lappend data_c_diropts data0
		}
		"overlap1" {
			#
			# Client has cdata.  Specify that as first data_dir
			# and db0 should get created there.
			# Client has data1.  Db1 should get created there.
			#
			file mkdir $clientdir/cdata
			file mkdir $clientdir/data1
			file mkdir $clientdir2/cdata
			file mkdir $clientdir2/data1
			set expect_dir [list $clientdir/cdata $clientdir/data1]
			set expect2_dir \
			    [list $clientdir2/cdata $clientdir2/data1]
			lappend data_c_diropts -data_dir
			lappend data_c_diropts cdata
			lappend data_c_diropts -data_dir
			lappend data_c_diropts data1
		}
		"nooverlap" {
			#
			# Client has cdata.  Specify that as only data_dir.
			# Both dbs should get created there.
			#
			file mkdir $clientdir/cdata
			file mkdir $clientdir2/cdata
			set expect_dir [list $clientdir/cdata $clientdir/cdata]
			set expect2_dir \
			    [list $clientdir2/cdata $clientdir2/cdata]
			lappend data_c_diropts -data_dir
			lappend data_c_diropts cdata
			lappend data_c_diropts -create_dir
			lappend data_c_diropts cdata
		}
	}

	# Open a master.
	repladd 1
	set ma_envcmd "berkdb_env_noerr -create $m_txnargs \
	    $repmemargs $privargs \
	    $m_logargs -log_max $log_max -errpfx MASTER \
	    $data_diropts $verbargs \
	    -home $masterdir -rep_transport \[list 1 replsend\]"
	set masterenv [eval $ma_envcmd $recargs -rep_master]

	#
	# Create a database in each data_dir and add some data to it.
	# Run rep_test in the master.
	# This is broken up into two loops because we want all of the
	# file creations done first so that they're all in the first
	# log file.  Later archiving will then remove all creation records.
	#
	puts "\tRep$tnum.a.0: Running rep_test $nfiles times in replicated env."
	set dbopen ""
	for { set i 0 } { $i < $nfiles } { incr i } {
		set crdir [lindex $m_create_dirs $i]
		set dbname "db$crdir.db"
		set db($i) [eval {berkdb_open_noerr -env $masterenv \
		    -auto_commit -create -create_dir $crdir \
		    -mode 0644} $largs $omethod $dbname]
	}
	for { set i 0 } { $i < $nfiles } { incr i } {
		set mult [expr $i * 10]
		set nentries [expr $niter + $mult]
		eval rep_test $method $masterenv $db($i) $nentries $mult $mult \
		    0 $largs
	}

	# Open a client
	repladd 2
	set cl_envcmd "berkdb_env_noerr -create $c_txnargs \
	    $repmemargs $privargs \
	    $c_logargs -log_max $log_max -errpfx CLIENT \
	    $data_c_diropts $verbargs \
	    -home $clientdir -rep_transport \[list 2 replsend\]"
	set clientenv [eval $cl_envcmd $recargs -rep_client]

	set envlist "{$masterenv 1} {$clientenv 2}"
	process_msgs $envlist

	# Clobber replication's 30-second anti-archive timer, which will have
	# been started by client sync-up internal init, so that we can do a
	# log_archive in a moment.
	#
	$masterenv test force noarchive_timeout

	#
	# Check that the database creation replicated to the correct data_dir.
	#
	puts "\tRep$tnum.b: Check databases in expected locations."
	rep103_dir_verify $nfiles $m_create_dirs $expect_dir

	#
	# Now check that a client that is initialized via internal init
	# correctly recreates the data_dir structure.
	#
	puts "\tRep$tnum.c: Initialize client2 via internal init."
	#
	# First make sure the master moves beyond log file 1.
	#
	set firstlf [get_logfile $masterenv first]
	$masterenv log_archive -arch_remove
	while { [get_logfile $masterenv first] <= $firstlf } {
		eval rep_test $method $masterenv $db(0) $nentries $mult $mult \
		    0 $largs
		process_msgs $envlist
		incr mult $nentries
		$masterenv log_archive -arch_remove
		set lf [get_logfile $masterenv first]
	}

	#
	# Now that we've archived, start up the 2nd client.
	#
	repladd 3
	set cl2_envcmd "berkdb_env_noerr -create $c_txnargs \
	    $repmemargs $privargs \
	    $c_logargs -log_max $log_max -errpfx CLIENT2 \
	    $data_c_diropts $verbargs \
	    -home $clientdir2 -rep_transport \[list 3 replsend\]"
	set client2env [eval $cl2_envcmd $recargs -rep_client]

	# Bring the client online by processing the startup messages.
	set envlist "{$masterenv 1} {$clientenv 2} {$client2env 3}"
	process_msgs $envlist

	# Now that internal init is complete, check the file locations.
	rep103_dir_verify $nfiles $m_create_dirs $expect2_dir

	for { set i 0 } { $i < $nfiles } { incr i } {
		error_check_good db_close [$db($i) close] 0
	}

	error_check_good masterenv_close [$masterenv close] 0
	error_check_good clientenv_close [$clientenv close] 0
	error_check_good clientenv2_close [$client2env close] 0
	replclose $testdir/MSGQUEUEDIR
}

proc rep103_dir_verify { nfiles m_dirs c_dirs} {
	for { set i 0 } { $i < $nfiles } { incr i } {
		set mdir [lindex $m_dirs $i]
		set cdir [lindex $c_dirs $i]
		set dbname "db$mdir.db"
		error_check_good db_$mdir \
		    [file exists $cdir/$dbname] 1
	}
}
