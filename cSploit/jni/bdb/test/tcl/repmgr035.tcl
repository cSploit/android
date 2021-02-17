# See the file LICENSE for redistribution information.
#
# Copyright (c) 2012, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	repmgr035
# TEST	Tests replication manager running with different versions.
# TEST	This capability is introduced with 4.5, but this test can only
# TEST	go back to 5.0 because it requires the ability to turn off
# TEST	elections.
# TEST
# TEST	Start a replication group of 1 master and N sites, all
# TEST	running some historical version greater than or equal to 5.0.
# TEST	Take down a client and bring it up again running current.
# TEST	Run some upgrades, make sure everything works.
# TEST
# TEST	Each site runs the tcllib of its own version, but uses
# TEST	the current tcl code (e.g. test.tcl).
proc repmgr035 { { nsites 3 } args } {
	source ./include.tcl
	global repfiles_in_memory
	global is_windows_test

	if { $is_windows_test } {
		puts "Skipping repmgr035 for Windows platform"
		return
	}

	set method "btree"
	set msg2 "and on-disk replication files"
	if { $repfiles_in_memory } {
		set msg2 "and in-memory replication files"
	}

	# Make the list of {method version} pairs to test.
	#
	set mvlist [repmgr035_method_version]
	set mvlen [llength $mvlist]
	puts "Repmgr035: Testing the following $mvlen method/version pairs:"
	puts "Repmgr035: $mvlist"
	puts "Repmgr035: $msg2"
	set count 1
	set total [llength $mvlist]
	set slist [upgrade_setup_sites $nsites]
	foreach i $mvlist {
		puts "Repmgr035: Test iteration $count of $total: $i"
		repmgr035_sub $count $i $nsites $slist
		incr count
	}
}

proc repmgr035_sub { iter mv nsites slist } {
	source ./include.tcl
	set method [lindex $mv 0]
	set vers [lindex $mv 1]

	puts "\tRepmgr035.$iter.a: Set up."
	# Whatever directory we started this process from is referred
	# to as the controlling directory.  It will start all the child
	# processes.
	set controldir [pwd]
	env_cleanup $controldir/$testdir

	# Set up the historical build directory.  The master will start
	# running with historical code.
	#
	# This test presumes we are running in the current build
	# directory and that the expected historical builds are
	# set up in a similar fashion.  If they are not, quit gracefully.

	set pwd [pwd]
	set homedir [file dirname [file dirname $pwd]]
	#
	# Cannot use test_path because that is relative to the current
	# directory (which will often be the old release directory).
	# We need to send in the pathname to the reputils path to the
	# current directory and that will be an absolute pathname.
	#
	set reputils_path $pwd/../test/tcl
	set histdir $homedir/$vers/build_unix
	if { [file exists $histdir] == 0 } {
		puts -nonewline "Skipping iteration $iter: cannot find"
		puts " historical version $vers."
		return
	}
	if { [file exists $histdir/db_verify] == 0 } {
		puts -nonewline "Skipping iteration $iter: historical version"
		puts " $vers is missing some executables.  Is it built?"
		return
	}

	set histtestdir $histdir/TESTDIR

	env_cleanup $histtestdir
	set markerdir $controldir/$testdir/MARKER
	file delete -force $markerdir

	# Create site directories.  They start running in the historical
	# directory, too.  They will be upgraded to the current version
	# first.
	for { set i 0 } { $i < $nsites } { incr i } {
		set siteid($i) [expr $i + 1]
		set sid $siteid($i)
		set histdirs($sid) $histtestdir/SITE.$i
		set upgdir($sid) $controldir/$testdir/SITE.$i
		file mkdir $histdirs($sid)
		file mkdir $histdirs($sid)/DATADIR
		file mkdir $upgdir($sid)
		file mkdir $upgdir($sid)/DATADIR
	}

	#
	# We know that slist has all sites starting in the histdir.
	# So if we encounter an upgrade value, we upgrade that client
	# from the hist dir.
	#
	set count 1
	foreach siteupg $slist {
		puts "\tRepmgr035.b.$iter.$count: Run with sitelist $siteupg."
		#
		# Delete the marker directory each iteration so that
		# we don't find old data in there.
		#
		file delete -force $markerdir
		file mkdir $markerdir
		#
		# Get the chosen master index from the list of sites.
		#
		set mindex [upgrade_get_master $nsites $siteupg]
		set meid [expr $mindex + 1]
		set ports [available_ports $nsites]

		#
		# Kick off the test processes.  We need 1 test process
		# per site.
		#
		set pids {}
		for { set i 0 } { $i < $nsites } { incr i } {
			set upg [lindex $siteupg $i]
			set sid $siteid($i)
			#
			# If we are running "old" set up an array
			# saying if this site has run old/new yet.
			# The reason is that we want to "upgrade"
			# only the first time we go from old to new,
			# not every iteration through this loop.
			#
			if { $upg == 0 } {
				puts -nonewline "\t\tRepmgr035.b: Test: Old site $i"
				set sitedir($i) $histdirs($sid)
				set already_upgraded($i) 0
			} else {
				puts -nonewline "\t\tRepmgr035.b: Test: Upgraded site $i"
				set sitedir($i) $upgdir($sid)
				if { $already_upgraded($i) == 0 } {
					upgrade_one_site $histdirs($sid) \
					    $sitedir($i)
				}
				set already_upgraded($i) 1
			}
			if { $sid == $meid } {
				set role MASTER
				set op [list REPTEST $method 15 10]
				puts " (MASTER)"
			} else {
				set role CLIENT
				set op {REPTEST_GET}
				puts " (CLIENT)"
			}

			# Construct remote port list for start up.
			set remote_ports {}
			foreach port $ports {
			    if { $port != [lindex $ports $i] } {
					lappend remote_ports $port
				}
			}
			lappend pids [exec $tclsh_path $test_path/wrap.tcl \
			    repmgr035script.tcl \
			    $controldir/$testdir/$count.S$i.log \
		      	    SKIP \
			    START $role \
			    $op $sid $controldir \
			    $sitedir($i) $reputils_path \
			    [lindex $ports $i] $remote_ports &]
		}

		watch_procs $pids 20

		#
		# Kick off the verification processes.  These walk the logs
		# and databases.  We need separate processes because old
		# sites need to use old utilities.
		#
		set pids {}
		puts "\tRepmgr035.c.$iter.$count: Verify all sites."
		for { set i 0 } { $i < $nsites } { incr i } {
			if { $siteid($i) == $meid } {
				set role MASTER
			} else {
				set role CLIENT
			}
			lappend pids [exec $tclsh_path $test_path/wrap.tcl \
			    repmgr035script.tcl \
			    $controldir/$testdir/$count.S$i.ver \
		      	    SKIP \
			    VERIFY $role \
		    	    {LOG DB} $siteid($i) $controldir \
			    $sitedir($i) $reputils_path \
			    [lindex $ports $i] $remote_ports &]
		}

		watch_procs $pids 10
		#
		# Now that each site created its verification files,
		# we can now verify everyone.
		#
		for { set i 0 } { $i < $nsites } { incr i } {
			if { $i == $mindex } {
				continue
			}
			puts \
	"\t\tRepmgr035.c: Verify: Compare databases master and client $i"
			error_check_good db_cmp \
			    [filecmp $sitedir($mindex)/VERIFY/dbdump \
			    $sitedir($i)/VERIFY/dbdump] 0
			set upg [lindex $siteupg $i]
			# !!!
			# Although db_printlog works and can read old logs,
			# there have been some changes to the output text that
			# makes comparing difficult.  One possible solution
			# is to run db_printlog here, from the current directory
			# instead of from the historical directory.
			#
			if { $upg == 0 } {
				puts \
	"\t\tRepmgr035.c: Verify: Compare logs master and client $i"
				error_check_good log_cmp \
				    [filecmp $sitedir($mindex)/VERIFY/prlog \
				    $sitedir($i)/VERIFY/prlog] 0
			} else {
				puts \
	"\t\tRepmgr035.c: Verify: Compare LSNs master and client $i"
				error_check_good log_cmp \
				    [filecmp $sitedir($mindex)/VERIFY/loglsn \
				    $sitedir($i)/VERIFY/loglsn] 0
			}
		}

		#
		# At this point we have a master and sites all up to date
		# with each other.  Now, one at a time, upgrade the sites
		# to the current version and start everyone up again.
		incr count
	}
}

proc repmgr035_method_version { } {

	set mv {}
	set versions {db-5.0.32 db-5.1.29 db-5.2.42 db-5.3.21}
	set versions_len [expr [llength $versions] - 1]

	# Walk through the list of versions and pair each with btree method.
	while { $versions_len >= 0 } {
		set version [lindex $versions $versions_len]
		incr versions_len -1
		lappend mv [list btree $version]
	}
	return $mv
}
