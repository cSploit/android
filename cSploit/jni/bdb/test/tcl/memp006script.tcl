# See the file LICENSE for redistribution information.
#
# Copyright (c) 2012, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# Multiple process mpool page extension tester.
# Usage: mpoolscript dir id op numiters
# dir: home directory.
# id: Unique identifier for this process.
# op: Operation to perform.  EXTEND, OPEN, STAT
# numiters: Total number of iterations.

source ./include.tcl
source $test_path/test.tcl
source $test_path/testutils.tcl

set usage "mpoolscript dir id op numiters"

#
# We have 3 different types of procs that will run this script:
# STAT: runs mpool stats trying to detect non-multiple of
# pagesize situations.
# EXTEND: runs in a loop continually extending the file by
# writing a dirty page to the end.
# OPEN: runs in a loop continually opening/closing the mpool file.
# It is the open path that checks for non-multiple pagesize and
# sets the stat field.
#

# STAT proc:
# Runs in a loop that looks at the "Odd file size detected" stat.
# There is one of these procs running during the test.  This proc
# controls the creation of the MARKER file for the other procs.
# This proc will run until it either reaches a maximum number of
# iterations or it detects the non-multiple pagesize situation.
proc mp6_stat { menv dir niters } {
	set done 0
	set count 1
	while { $done == 0 } {
		set odd [stat_field $menv mpool_stat "Odd file size detected"]
		puts \
"STAT: $count of $niters iterations: Odd file size found $odd time(s)."
		if { $odd != 0 || $count >= $niters } {
			set done 1
			puts "STAT: [timestamp] Open $dir/MARKER"
			set marker [open $dir/MARKER a]
			puts $marker DONE
			if { $odd != 0 } {
				puts "SUCCESS"
			}
			puts "STAT: [timestamp] close MARKER"
			close $marker
		} else {
			tclsleep 1
			incr count
		}
	}
	puts \
"STAT: After $count of $niters iterations: Odd file size found $odd time(s)."
}

# EXTEND proc:
# This proc creates the mpool file and will run in a loop creating a new
# dirty page at the end of the mpool file, extending it each time.
# It runs until it sees the MARKER file created by the STAT proc.
proc mp6_extend { menv dir id pgsize } {
	puts "EXTEND: Create file$id"
	set mp [$menv mpool -create -mode 0644 -pagesize $pgsize file$id]
	error_check_good memp_fopen [is_valid_mpool $mp $menv] TRUE
	set pgno 0
	while { [file exists $dir/MARKER] == 0 } {
		set pg [$mp get -create -dirty $pgno]
		$pg put
		$mp fsync
		incr pgno
		set e [file exists $dir/MARKER]
		if { [expr $pgno % 10] == 0 } {
			puts "[timestamp] Wrote $pgno pages, $dir/MARKER $e"
		}
	}
	puts "EXTEND: Done: Created $pgno pages"
	$mp close
}

# OPEN proc:
# This proc open the mpool file and will run in a loop opening and closing
# the mpool file.  The BDB open code detects the situation we're looking for.
# It runs until it sees the MARKER file created by the STAT proc.
proc mp6_open { menv dir id pgsize } {
	set mark [file exists $dir/MARKER]
	set myfile "file$id"
	#
	# First wait for the EXTEND process to create the file
	#
	puts "OPEN: Wait for $myfile to be created from EXTEND proc."
	while { [file exists $dir/$myfile] == 0 } {
		tclsleep 1
	}
	puts "OPEN: Open and close in a loop"
	while { [file exists $dir/MARKER] == 0 } {
		set mp [$menv mpool -pagesize $pgsize file$id]
		$mp close
	}
	puts "OPEN: Done"
}

# Verify usage
if { $argc != 4 } {
	puts stderr "FAIL:[timestamp] Usage: $usage"
	puts $argc
	exit
}

# Initialize arguments
set dir [lindex $argv 0]
set id [lindex $argv 1]
set op [lindex $argv 2]
set numiters [ lindex $argv 3 ]

# Give time for all processes to start up.
tclsleep 3

puts -nonewline "Beginning execution for $id: $op $dir $numiters"
flush stdout


set env_cmd {berkdb_env -lock -home $dir}
set menv [eval $env_cmd]
error_check_good env_open [is_valid_env $menv] TRUE

set pgsize [expr 64 * 1024]
if { $op == "STAT" } {
	mp6_stat $menv $dir $numiters
} elseif { $op == "EXTEND" } {
	mp6_extend $menv $dir $id $pgsize
} elseif { $op == "OPEN" } {
	mp6_open $menv $dir $id $pgsize
} else {
	error "Unknown op: $op"
}
# Close environment system
set r [$menv close]
error_check_good env_close $r 0

puts "[timestamp] $id Complete"
flush stdout
