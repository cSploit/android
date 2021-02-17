# See the file LICENSE for redistribution information.
#
# Copyright (c) 2012, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# repmgr035script - procs to use at each replication site in the
# replication manager upgrade test.
#

proc repmgr035scr_starttest { role oplist envid mydir markerdir local_port remote_ports } {
	global util_path
	global repfiles_in_memory

	set repmemargs ""
	if { $repfiles_in_memory } {
		set repmemargs "-rep_inmem_files "
	}

	puts "set up env cmd"
	set lockmax 40000
	set logbuf [expr 16 * 1024]
	set logmax [expr $logbuf * 4]
	if { $role == "MASTER" } {
		set rolearg master
	} elseif { $role == "CLIENT" } {
		set rolearg client
	} else {
		puts "FAIL: unrecognized replication role $role"
		return
	}
	set rep_env_cmd "berkdb_env_noerr -create -home $mydir \
	    -log_max $logmax -log_buffer $logbuf $repmemargs \
	    -lock_max_objects $lockmax -lock_max_locks $lockmax \
	    -errpfx $role -txn -data_dir DATADIR \
	    -verbose {rep on} -errfile /dev/stderr -rep -thread"

	# Change directories to where this will run.
	# !!!
	# mydir is an absolute path of the form
	# <path>/build_unix/TESTDIR/MASTERDIR or
	# <path>/build_unix/TESTDIR/CLIENTDIR.0
	#
	# So we want to run relative to the build_unix directory
	cd $mydir/../..

	puts "open repenv $rep_env_cmd"
	set repenv [eval $rep_env_cmd]
	error_check_good repenv_open [is_valid_env $repenv] TRUE

	set legacy_str ""
	set nsites_str ""
	if { [have_group_membership] } {
		# With group membership, use the legacy option to start the
		# sites in the replication group because this will work when
		# some sites are still at an older, pre-group-membership 
		# version of Berkeley DB.
		set legacy_str "legacy"
	} else {
		# When running an earlier version of Berkeley DB before
		# group membership, we must supply an nsites value.
		set nsites_str " -nsites [expr [llength $remote_ports] + 1]"
	}
	set repmgr_conf " -start $rolearg $nsites_str \
	    -local { 127.0.0.1 $local_port $legacy_str }"
	# Append each remote site.  This is required for group membership
	# legacy startups, and doesn't hurt the other cases.
	foreach rmport $remote_ports {
		append repmgr_conf " -remote { 127.0.0.1 $rmport $legacy_str }"
	}
	# Turn off elections so that clients still running at the end of the
	# test after the master shuts down do not create extra log records.
	$repenv rep_config {mgrelections off}
	eval $repenv repmgr $repmgr_conf

	if { $role == "CLIENT" } {
		await_startup_done $repenv
	}

	puts "repenv is $repenv"
	#
	# Indicate that we're done starting up.  Sleep to let
	# others do the same.
	#
	puts "create START$envid marker file"
	upgrade_create_markerfile $markerdir/START$envid
	puts "sleeping after marker"
	tclsleep 3

	# Here is where the real test starts.
	#
	# Different operations may have different args in their list.
	# REPTEST: Args are method, niter, nloops.
	# REPTEST_GET: Does not use args.
	set op [lindex $oplist 0]
	if { $op == "REPTEST" } {
		upgradescr_reptest $repenv $oplist $markerdir
	}
	if { $op == "REPTEST_GET" } {
		upgradescr_repget $repenv $oplist $mydir $markerdir
	}
	puts "Closing env"
	$repenv mpool_sync
	error_check_good envclose [$repenv close] 0

}

proc repmgr035scr_verify { oplist mydir } {
	global util_path

	set rep_env_cmd "berkdb_env_noerr -home $mydir -txn \
	    -data_dir DATADIR"

	upgradescr_verify $oplist $mydir $rep_env_cmd
}

#
# Arguments:
# type: START, VERIFY
# 	START starts up a replication site and performs an operation.
#		the operations are:
#		REPTEST runs the rep_test_upg procedure on the master.
#		REPTEST_GET run a read-only test on a client.
#	VERIFY dumps the log and database contents.
# role: master or client
# op: operation to perform
# envid: environment id number for use in replsend
# ctldir: controlling directory
# mydir: directory where this participant runs
# reputils_path: location of reputils.tcl
# local_port: port for local repmgr site
# remote_ports: ports for remote repmgr sites
#
set usage "upgradescript type role op envid ctldir mydir reputils_path local_port remote_ports"

# Verify usage
if { $argc != 9 } {
	puts stderr "Argc $argc, argv $argv"
	puts stderr "FAIL:[timestamp] Usage: $usage"
	exit
}

# Initialize arguments
set type [ lindex $argv 0 ]
set role [ lindex $argv 1 ]
set op [ lindex $argv 2 ]
set envid [ lindex $argv 3 ]
set ctldir [ lindex $argv 4 ]
set mydir [ lindex $argv 5 ]
set reputils_path [ lindex $argv 6 ]
set local_port [ lindex $argv 7 ]
set remote_ports [ lindex $argv 8 ]

set histdir $mydir/../..
puts "Histdir $histdir"

global env
cd $histdir
set stat [catch {eval exec ./db_printlog -V} result]
if { $stat != 0 } {
	set env(LD_LIBRARY_PATH) ":$histdir:$histdir/.libs:$env(LD_LIBRARY_PATH)"
}
source ./include.tcl
source $test_path/test.tcl

set is_repchild 1
puts "Did args. now source reputils"
source $reputils_path/reputils.tcl

set markerdir $ctldir/TESTDIR/MARKER

puts "Calling proc for type $type"
if { $type == "START" } {
	repmgr035scr_starttest $role $op $envid $mydir $markerdir $local_port $remote_ports
} elseif { $type == "VERIFY" } {
	file mkdir $mydir/VERIFY
	repmgr035scr_verify $op $mydir
} else {
	puts "FAIL: unknown type $type"
	return
}
