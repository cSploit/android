# See the file LICENSE for redistribution information.
#
# Copyright (c) 2001, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	scr###
# TEST	The scr### directories are shell scripts that test a variety of
# TEST	things, including things about the distribution itself.  These
# TEST	tests won't run on most systems, so don't even try to run them.
#
# shelltest.tcl:
#	Code to run shell script tests, to incorporate Java, C++,
#	example compilation, etc. test scripts into the Tcl framework.
proc shelltest {{ run_one 0 } { xml 0 }} {
	source ./include.tcl
	global shelltest_list
	global xmlshelltest_list

	set SH /bin/sh
	if { [file executable $SH] != 1 } {
		puts "Shell tests require valid shell /bin/sh: not found."
		puts "Skipping shell tests."
		return 0
	}

	if { $xml == 1 } {
		set shelltest_list $xmlshelltest_list
	}

	if { $run_one == 0 } {
		puts "Running shell script tests..."

		foreach testpair $shelltest_list {
			set dir [lindex $testpair 0]
			set test [lindex $testpair 1]
			set rundir [lindex $testpair 2]

			env_cleanup $testdir
			file mkdir $testdir/$rundir
			shelltest_copy $test_path/../$dir $testdir/$rundir
			shelltest_run $SH $dir $test $testdir/$rundir
		}
	} else {
		set run_one [expr $run_one - 1];
		set dir [lindex [lindex $shelltest_list $run_one] 0]
		set test [lindex [lindex $shelltest_list $run_one] 1]
		set rundir [lindex [lindex $shelltest_list $run_one] 2]

		env_cleanup $testdir
		file mkdir $testdir/$rundir
		shelltest_copy $test_path/../$dir $testdir/$rundir
		shelltest_run $SH $dir $test $testdir/$rundir
	}
}

proc shelltest_copy { fromdir todir } {
	set globall [glob $fromdir/*]

	foreach f $globall {
		file copy -force $f $todir/
	}
}

proc shelltest_run { sh srcdir test testdir } {
	puts "Running shell script $srcdir ($test)..."

	set ret [catch {exec $sh -c "cd $testdir && sh $test" >&@ stdout} res]

	if { $ret != 0 } {
		puts "FAIL: shell test $srcdir/$test exited abnormally"
	}
}

proc run_c {} { shelltest 1 }
proc run_cxx {} { shelltest 2 }
proc run_junit {} { shelltest 3 }
proc run_java_compat {} { shelltest 4 }
proc run_sql_codegen {} { shelltest 5 }
proc run_xa {} { shelltest 6 }
