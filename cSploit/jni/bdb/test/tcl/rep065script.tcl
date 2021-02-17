# See the file LICENSE for redistribution information.
#
# Copyright (c) 2006, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# rep065script - procs to use at each replication site in the
# replication upgrade test.
#

proc rep065scr_starttest { role oplist envid msgdir mydir allids markerdir } {
	global qtestdir
	global util_path
	global repfiles_in_memory

	puts "repladd_noenv $allids"
	set qtestdir $msgdir
	foreach id $allids {
		repladd_noenv $id
	}

	set repmemargs ""
	if { $repfiles_in_memory } {
		set repmemargs "-rep_inmem_files "
	}

	puts "set up env cmd"
	set lockmax 40000
	set logbuf [expr 16 * 1024]
	set logmax [expr $logbuf * 4]
	if { $role == "MASTER" } {
		set rolearg "-rep_master"
	} elseif { $role == "CLIENT" } {
		set rolearg "-rep_client"
	} else {
		puts "FAIL: unrecognized replication role $role"
		return
	}
	set rep_env_cmd "berkdb_env_noerr -create -home $mydir \
	    -log_max $logmax -log_buffer $logbuf $repmemargs \
	    -lock_max_objects $lockmax -lock_max_locks $lockmax \
	    -errpfx $role -txn $rolearg -data_dir DATADIR \
	    -verbose {rep on} -errfile /dev/stderr \
	    -rep_transport \[list $envid replsend_noenv\]"

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

proc rep065scr_msgs { role envid msgdir mydir allids markerdir } {
	global qtestdir
	global repfiles_in_memory

	set repmemargs ""
	if { $repfiles_in_memory } {
		set repmemargs "-rep_inmem_files "
	}

	#
	# The main test process will write a START marker file when it has
	# started and a DONE marker file when it has completed.  Wait here
	# for the expected START marker file.
	#
	while { [file exists $markerdir/START$envid] == 0 } {
		tclsleep 1
	}

	puts "repladd_noenv $allids"
	set qtestdir $msgdir
	foreach id $allids {
		repladd_noenv $id
	}

	puts "set up env cmd"
	if { $role == "MASTER" } {
		set rolearg "-rep_master"
	} elseif { $role == "CLIENT" } {
		set rolearg "-rep_client"
	} else {
		puts "FAIL: unrecognized replication role $role"
		return
	}
	set rep_env_cmd "berkdb_env_noerr -home $mydir \
	    -errpfx $role -txn $rolearg $repmemargs \
	    -verbose {rep on} -errfile /dev/stderr \
	    -data_dir DATADIR \
	    -rep_transport \[list $envid replsend_noenv\]"

	# Change directories to where this will run.
	cd $mydir

	puts "open repenv $rep_env_cmd"
	set repenv [eval $rep_env_cmd]
	error_check_good repenv_open [is_valid_env $repenv] TRUE

	set envlist "{$repenv $envid}"
	puts "repenv is $repenv"
	while { [file exists $markerdir/DONE] == 0 } {
		process_msgs $envlist 0 NONE NONE 1
		tclsleep 1
	}
	#
	# Process messages in case there are a few more stragglers.
	# Just because the main test is done doesn't mean that all
	# the messaging is done.  Loop for messages as long as
	# progress is being made.
	#
	set nummsg 1
	while { $nummsg != 0 } {
		process_msgs $envlist 0 NONE NONE 1
		tclsleep 1
		# First look at messages from us
		set nummsg [replmsglen_noenv $envid from]
		puts "Still have $nummsg not yet processed by others"
	}
	replclear_noenv $envid from
	tclsleep 1
	replclear_noenv $envid
	$repenv mpool_sync
	error_check_good envclose [$repenv close] 0
}

proc rep065scr_verify { oplist mydir id } {
	global util_path

	set rep_env_cmd "berkdb_env_noerr -home $mydir -txn \
	    -data_dir DATADIR \
	    -rep_transport \[list $id replnoop\]"

	upgradescr_verify $oplist $mydir $rep_env_cmd
}

#
# Arguments:
# type: START, PROCMSGS, VERIFY
# 	START starts up a replication site and performs an operation.
#		the operations are:
#		REPTEST runs the rep_test_upg procedure on the master.
#		REPTEST_GET run a read-only test on a client.
# 	PROCMSGS processes messages until none are left.
#	VERIFY dumps the log and database contents.
# role: master or client
# op: operation to perform
# envid: environment id number for use in replsend
# allids: all env ids we need for sending
# ctldir: controlling directory
# mydir: directory where this participant runs
# reputils_path: location of reputils.tcl

set usage "upgradescript type role op envid allids ctldir mydir reputils_path"

# Verify usage
if { $argc != 8 } {
	puts stderr "Argc $argc, argv $argv"
	puts stderr "FAIL:[timestamp] Usage: $usage"
	exit
}

# Initialize arguments
set type [ lindex $argv 0 ]
set role [ lindex $argv 1 ]
set op [ lindex $argv 2 ]
set envid [ lindex $argv 3 ]
set allids [ lindex $argv 4 ]
set ctldir [ lindex $argv 5 ]
set mydir [ lindex $argv 6 ]
set reputils_path [ lindex $argv 7 ]

set histdir $mydir/../..
puts "Histdir $histdir"

set msgtestdir $ctldir/TESTDIR

global env
cd $histdir
set stat [catch {eval exec ./db_printlog -V} result]
if { $stat != 0 } {
	set env(LD_LIBRARY_PATH) ":$histdir:$histdir/.libs:$env(LD_LIBRARY_PATH)"
}
source ./include.tcl
source $test_path/test.tcl

# The global variable noenv_messaging must be set after sourcing
# test.tcl or its value will be wrong.
global noenv_messaging
set noenv_messaging 1

set is_repchild 1
puts "Did args. now source reputils"
source $reputils_path/reputils.tcl
source $reputils_path/reputilsnoenv.tcl

set markerdir $msgtestdir/MARKER

puts "Calling proc for type $type"
if { $type == "START" } {
	rep065scr_starttest $role $op $envid $msgtestdir $mydir $allids $markerdir
} elseif { $type == "PROCMSGS" } {
	rep065scr_msgs $role $envid $msgtestdir $mydir $allids $markerdir
} elseif { $type == "VERIFY" } {
	file mkdir $mydir/VERIFY
	rep065scr_verify $op $mydir $envid
} else {
	puts "FAIL: unknown type $type"
	return
}
