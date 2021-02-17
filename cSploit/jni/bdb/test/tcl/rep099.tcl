# See the file LICENSE for redistribution information.
#
# Copyright (c) 2004, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	rep099
# TEST	Test of multiple data dirs and databases, both active replication
# TEST	and internal init.
# TEST
# TEST	One master, two clients using several data_dirs.
# TEST	Create databases in different data_dirs.  Replicate to client.
# TEST	Add 2nd client later to require it to catch up via internal init.
#
proc rep099 { method { niter 500 } { tnum "099" } args } {
	source ./include.tcl
	global databases_in_memory
	global repfiles_in_memory
	global env_private

	# Valid for all access methods.
	if { $checking_valid_methods } {
		return "ALL"
	}

	set args [convert_args $method $args]
	set logsets [create_logsets 3]

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
	#   datadir: rep files in db_home, databases in data0/data1/data2
	#   repdir: rep files in meta, databases in db_home
	#   both: rep files in meta, databases in data0/data1/data2
	#   overlap0: rep files in data0, databases in data0/data1/data2
	#   overlap1: rep files in data1, databases in data0/data1/data2
	#   overlap2: rep files in data2, databases in data0/data1/data2
	#
	set opts { datadir repdir both overlap0 overlap1 overlap2 }
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
				puts "Rep$tnum: Client2 logs are [lindex $l 2]"
				rep099_sub $method $niter $tnum $l $r $c $args
			}
		}
	}
}

proc rep099_sub { method niter tnum logset recargs opts largs } {
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
	set newdir $testdir/NEWDIR

	file mkdir $masterdir
	file mkdir $newdir
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
	set c2_logtype [lindex $logset 2]

	# In-memory logs cannot be used with -txn nosync.
	set m_logargs [adjust_logargs $m_logtype]
	set c_logargs [adjust_logargs $c_logtype]
	set c2_logargs [adjust_logargs $c2_logtype]
	set m_txnargs [adjust_txnargs $m_logtype]
	set c_txnargs [adjust_txnargs $c_logtype]
	set c2_txnargs [adjust_txnargs $c2_logtype]

	# Set up data directories for the various configurations.
	set create_dirs {}
	set nfiles 0
	set data_diropts ""
	if { $opts == "repdir" } {
		set nfiles 1
	} else {
		set create_dirs {data0 data1 data2}
		foreach d $create_dirs {
			incr nfiles
			file mkdir $masterdir/$d
			file mkdir $clientdir/$d
			file mkdir $clientdir2/$d
			file mkdir $newdir/$d
			lappend data_diropts -data_dir
			lappend data_diropts $d
		}
	}

	# Set up metadata directory for all configurations except "datadir",
	# which doesn't have one.
	set meta_diropts ""
	set meta_dir ""
	switch $opts {
		"repdir" -
		"both" {
			set meta_dir "meta"
			file mkdir $masterdir/$meta_dir
			file mkdir $clientdir/$meta_dir
			file mkdir $clientdir2/$meta_dir
			file mkdir $newdir/$meta_dir
		}
		"overlap0" { set meta_dir "data0" }
		"overlap1" { set meta_dir "data1" }
		"overlap2" { set meta_dir "data2" }
	}
	if { $opts != "datadir" } {
		set meta_diropts "-metadata_dir $meta_dir"
	}

	# Open a master.
	repladd 1
	set ma_envcmd "berkdb_env_noerr -create $m_txnargs \
	    $repmemargs $privargs $meta_diropts \
	    $m_logargs -log_max $log_max -errpfx MASTER \
	    $data_diropts $verbargs \
	    -home $masterdir -rep_transport \[list 1 replsend\]"
	set masterenv [eval $ma_envcmd $recargs -rep_master]

	# Open a client
	repladd 2
	set cl_envcmd "berkdb_env_noerr -create $c_txnargs \
	    $repmemargs $privargs $meta_diropts \
	    $c_logargs -log_max $log_max -errpfx CLIENT \
	    $data_diropts $verbargs \
	    -home $clientdir -rep_transport \[list 2 replsend\]"
	set clientenv [eval $cl_envcmd $recargs -rep_client]

	# Bring the client online by processing the startup messages.
	set envlist "{$masterenv 1} {$clientenv 2}"
	process_msgs $envlist

	# Clobber replication's 30-second anti-archive timer, which will have
	# been started by client sync-up internal init, so that we can do a
	# log_archive in a moment.
	#
	$masterenv test force noarchive_timeout

	#
	# Create a database in each data_dir and add some data to it.
	# Run rep_test in the master (and update client).
	# This is broken up into two loops because we want all of the
	# file creations done first so that they're all in the first
	# log file.  Later archiving will then remove all creation records.
	#
	puts "\tRep$tnum.a.0: Running rep_test $nfiles times in replicated env."
	set dbopen ""
	if { $opts == "repdir" } {
		set dbname "dbdata.db"
		set db(0) [eval {berkdb_open_noerr -env $masterenv \
		    -auto_commit -create -mode 0644} $largs $omethod $dbname]
	} else {
		for { set i 0 } { $i < $nfiles } { incr i } {
			set crdir [lindex $create_dirs $i]
			set dbname "db$crdir.db"
			set db($i) [eval {berkdb_open_noerr -env $masterenv \
			    -auto_commit -create -create_dir $crdir \
			    -mode 0644} $largs $omethod $dbname]
		}
	}
	for { set i 0 } { $i < $nfiles } { incr i } {
		set mult [expr $i * 10]
		set nentries [expr $niter + $mult]
		eval rep_test $method $masterenv $db($i) $nentries $mult $mult \
		    0 $largs
		process_msgs $envlist
	}

	#
	# Check that the database creation replicated to the correct data_dir.
	#
	puts "\tRep$tnum.b: Check create_dirs properly replicated."
	rep099_dir_verify $nfiles $clientdir $create_dirs $opts

	#
	# Check that running recovery from full log files recreates 
	# correctly.  This can only be done from on-disk log files. 
	# NOTE:  This part of the test has nothing to do with replication
	# but is a good thing to test and is easy to do here.
	#
	if { $m_logtype == "on-disk" } {
		puts "\tRep$tnum.c: Check create_dirs properly recovered."
		set lfiles [glob -nocomplain $masterdir/log.*]
		foreach lf $lfiles {
			set fname [file tail $lf]
			file copy $lf $newdir/$fname
		}
		set stat [catch {exec $util_path/db_recover -c -h $newdir} result]
		rep099_dir_verify $nfiles $newdir $create_dirs $opts
	}

	#
	# Now check that a client that is initialized via internal init
	# correctly recreates the data_dir structure.
	#
	puts "\tRep$tnum.d: Initialize client2 via internal init."
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
	}

	#
	# Now that we've archived, start up the 2nd client.
	#
	repladd 3
	set cl2_envcmd "berkdb_env_noerr -create $c2_txnargs \
	    $repmemargs $privargs $meta_diropts \
	    $c2_logargs -log_max $log_max -errpfx CLIENT2 \
	    $data_diropts $verbargs \
	    -home $clientdir2 -rep_transport \[list 3 replsend\]"
	set client2env [eval $cl2_envcmd $recargs -rep_client]

	# Bring the client online by processing the startup messages.
	set envlist "{$masterenv 1} {$clientenv 2} {$client2env 3}"
	process_msgs $envlist

	# Now that internal init is complete, check the file locations.
	rep099_dir_verify $nfiles $clientdir2 $create_dirs $opts

	for { set i 0 } { $i < $nfiles } { incr i } {
		error_check_good db_close [$db($i) close] 0
	}

	puts "\tRep$tnum.e: Location check for all envs."
	check_log_location $masterenv
	check_log_location $clientenv
	check_log_location $client2env

	puts "\tRep$tnum.f: Check location of persistent rep files."
	if { !$repfiles_in_memory } {
		check_persistent_rep_files $masterdir $meta_dir
		check_persistent_rep_files $clientdir $meta_dir
		check_persistent_rep_files $clientdir2 $meta_dir
	}
	if { $opts != "datadir" } {
		error_check_good getmdd [$masterenv get_metadata_dir] $meta_dir
	}

	# Make sure there are no rep files in the data
	# directories.  Even when rep files are on disk,
	# they should be in the env's home directory.
	foreach d $create_dirs {
	        # Check that databases are in-memory or on-disk as expected.
		check_db_location $masterenv $d
		check_db_location $clientenv $d
		check_db_location $client2env $d

	        # Check no rep files in data dirs in non-overlapping configs.
		if { $opts == "datadir" || $opts == "both" } {
			no_rep_files_on_disk $masterdir/$d
			no_rep_files_on_disk $clientdir/$d
			no_rep_files_on_disk $clientdir2/$d
		}
	}
	# create_dirs is empty for repdir, so check repdir separately.  Can't
	# perform no_rep_files_on_disk because database location (db_home)
	# also contains non-persistent metadata files.
	if { $opts == "repdir" } {
		check_db_location $masterenv ""
		check_db_location $clientenv ""
		check_db_location $client2env ""
	}
	error_check_good masterenv_close [$masterenv close] 0
	error_check_good clientenv_close [$clientenv close] 0
	error_check_good clientenv2_close [$client2env close] 0
	replclose $testdir/MSGQUEUEDIR
}

proc rep099_dir_verify { nfiles checkdir create_dirs opts } {
	if { $opts == "repdir" } {
		set dbname "dbdata.db"
		error_check_good db_0 [file exists $checkdir/$dbname] 1
	} else {
		for { set i 0 } { $i < $nfiles } { incr i } {
			set crdir [lindex $create_dirs $i]
			set dbname "db$crdir.db"
			error_check_good db_$crdir \
			    [file exists $checkdir/$crdir/$dbname] 1
		}
	}
}

proc check_persistent_rep_files { dir metadir } {
	if { $metadir == "" } {
		set checkdir "$dir"
	} else {
		set checkdir "$dir/$metadir"
	}
	#
	# Cannot check for persistent file __db.rep.init because it is not
	# there most of the time.
	#
	error_check_good gen [file exists "$checkdir/__db.rep.gen"] 1
	error_check_good egen [file exists "$checkdir/__db.rep.egen"] 1
	error_check_good sysdb [file exists "$checkdir/__db.rep.system"] 1
}
