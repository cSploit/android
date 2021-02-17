# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#

# TEST	memp002
# TEST	Tests multiple processes accessing and modifying the same files.
proc memp002 { } {
	source ./include.tcl
	#
	# Multiple processes not supported by private memory so don't
	# run memp002_body with -private.
	#
	memp002_body ""
	if { $is_qnx_test } {
		puts "Skipping remainder of memp002 for\
		    environments in system memory on QNX"
		return
	}
	set shm_key 20
	memp002_body "-system_mem -shm_key $shm_key"
}

proc memp002_body { flags } {
	source ./include.tcl

	puts "Memp002: {$flags} Multiprocess mpool tester"

	set procs 4
	set psizes "512 1024 2048 4096 8192"
	set iterations 500
	set npages 100

	# Check if this combination of flags is supported by this arch.
	if { [mem_chk $flags] == 1 } {
		return
	}

	set iter [expr $iterations / $procs]

	# Clean up old stuff and create new.
	env_cleanup $testdir

	for { set i 0 } { $i < [llength $psizes] } { incr i } {
		fileremove -f $testdir/file$i
	}
	set e [eval {berkdb_env -create -lock -home $testdir} $flags]
	error_check_good dbenv [is_valid_env $e] TRUE

	set pidlist {}
	for { set i 0 } { $i < $procs } {incr i} {

		puts "$tclsh_path\
		    $test_path/mpoolscript.tcl $testdir $i $procs \
		    $iter $psizes $npages 3 $flags > \
		    $testdir/memp002.$i.out &"
		set p [exec $tclsh_path $test_path/wrap.tcl \
		    mpoolscript.tcl $testdir/memp002.$i.out $testdir $i $procs \
		    $iter $psizes $npages 3 $flags &]
		lappend pidlist $p
	}
	puts "Memp002: $procs independent processes now running"
	watch_procs $pidlist 30

	# Check for test failure
	set errstrings [eval findfail [glob $testdir/memp002.*.out]]
	foreach str $errstrings {
		puts "FAIL: error message in log file: $str"
	}

	reset_env $e
}
