# See the file LICENSE for redistribution information.
#
# Copyright (c) 2012, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#

# TEST	memp006
# TEST	Tests multiple processes accessing and modifying the same files.
# TEST	Attempt to hit the case where we see the mpool file not a
# TEST	multiple of pagesize so that we can make sure we tolerate it.
# TEST	Some file systems don't protect against racing writes and stat
# TEST	so seeing a database not a multiple of pagesize is possible.
# TEST	Use a large pagesize to try to catch the file at a point where
# TEST	it is getting extended and that races with the open.
proc memp006 { } {
	source ./include.tcl
	#
	# Multiple processes not supported by private memory so don't
	# run memp006_body with -private.
	#
	set nm 0
	set count 1
	set max_iter 3
	set start [timestamp]
	while { $nm == 0 && $count <= $max_iter } {
		puts "Memp006: [timestamp] Iteration $count. Started $start"
		set nm [memp006_body]
		incr count
	}
}

proc memp006_body { } {
	source ./include.tcl

	puts "Memp006: Multiprocess mpool pagesize tester"

	set nmpools 4
	set iterations 500

	set iter [expr $iterations / $nmpools]

	# Clean up old stuff and create new.
	env_cleanup $testdir

	for { set i 0 } { $i < $nmpools } { incr i } {
		fileremove -f $testdir/file$i
	}
	set e [eval {berkdb_env -create -lock -home $testdir}]
	error_check_good dbenv [is_valid_env $e] TRUE

	#
	# Start off a STAT process for the env, and then 2 procs
	# for each mpool, an EXTEND and and OPEN process.  So,
	# the total processes are $nmpools * 2 + 1.
	#
	set nprocs [expr $nmpools * 2 + 1]
	set pidlist {}
	puts "Memp006: $tclsh_path\
	    $test_path/memp006script.tcl $testdir 0 STAT $iter > \
	    $testdir/memp006.0.STAT &"
	set p [exec $tclsh_path $test_path/wrap.tcl \
	    memp006script.tcl $testdir/memp006.0.STAT $testdir 0 STAT \
	    $iter &]
	lappend pidlist $p
	for { set i 1 } { $i <= $nmpools } {incr i} {
		puts "Memp006: $tclsh_path\
		    $test_path/memp006script.tcl $testdir $i EXTEND $iter > \
		    $testdir/memp006.$i.EXTEND &"
		set p [exec $tclsh_path $test_path/wrap.tcl \
		    memp006script.tcl $testdir/memp006.$i.EXTEND $testdir $i \
		    EXTEND $iter &]
		lappend pidlist $p
		puts "Memp006: $tclsh_path\
		    $test_path/memp006script.tcl $testdir $i OPEN $iter > \
		    $testdir/memp006.$i.OPEN &"
		set p [exec $tclsh_path $test_path/wrap.tcl \
		    memp006script.tcl $testdir/memp006.$i.OPEN $testdir $i \
		    OPEN $iter &]
		lappend pidlist $p
	}
	puts "Memp006: $nprocs independent processes now running"
	watch_procs $pidlist 15

	# Check for unexpected test failure
	set errstrings [eval findfail [glob $testdir/memp006.*]]
	foreach str $errstrings {
		puts "FAIL: error message in log file: $str"
	}
	set files [glob $testdir/memp006.*]
	#
	# This is the real item we're looking for, whether or not we
	# detected the state where we saw a non-multiple of pagesize
	# in the mpool.
	#
	set nm 0
	foreach f $files {
		set success [findstring "SUCCESS" $f]
		if { $success } {
			set nm 1
			puts "Memp006: Detected non-multiple in $f"
		}
	}
	if { $nm == 0 } {
		puts "Memp006: Never saw non-multiple pages"
	}

	reset_env $e
	return $nm
}
