# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# Deadlock Test 5.
# Test out the minlocks, maxlocks, and minwrites options
# to the deadlock detector.
proc dead005 { { procs "4 6 10" } \
    {tests "maxlocks maxwrites minlocks minwrites" } { tnum "005" } { pri 0 } } {
	source ./include.tcl

	set msg ""
	if { $pri == 1 } {
		set msg " with priority"
	}
	
	foreach t $tests {
		puts "Dead$tnum.$t: deadlock detection tests"
		env_cleanup $testdir

		# Create the environment.
		set env [berkdb_env -create -mode 0644 -lock -home $testdir]
		error_check_good lock_env:open [is_valid_env $env] TRUE
		case $t {
			maxlocks { set to m }
			maxwrites { set to W }
			minlocks { set to n }
			minwrites { set to w }
		}
		foreach n $procs {
			set dpid [exec $util_path/db_deadlock -v -t 0.100000 \
			    -h $testdir -a $to >& $testdir/dd.out &]
			sentinel_init
			set pidlist ""

			# Fire off the tests
			puts "\tDead$tnum: $t test with $n procs $msg"
			for { set i 0 } { $i < $n } { incr i } {
				set locker [$env lock_id]
				# Configure priorities, if necessary, such that
				# the absolute max or min is a higher priority.
				# The number of locks for each locker is set by
				# countlocks in testutils.tcl.
				if {$pri == 1} {
					if {$t == "maxlocks"} {
						set half [expr $n / 2]
						if {$i < $half} {
							set lk_pri 0
						} else {
							set lk_pri 1
						}
					} elseif {$t == "maxwrites"} {
						if {$i == 0 || $i == 1} {
							set lk_pri 0
						} else {
							set lk_pri 1
						}
					} elseif {$t == "minlocks"} {
						set half [expr $n / 2]
						if {$i >= $half} {
							set lk_pri 0
						} else {
							set lk_pri 1
						}
					} elseif {$t == "minwrites"} {
						if {$i == 0 || $i == 2} {
							set lk_pri 0
						} else {
							set lk_pri 1
						}
					}
					$env lock_set_priority $locker $lk_pri
				}
				puts "$tclsh_path $test_path/wrap.tcl \
				    $testdir/dead$tnum.log.$i \
				    ddscript.tcl $testdir $t $locker $i $n"
				set p [exec $tclsh_path \
					$test_path/wrap.tcl \
					ddscript.tcl $testdir/dead$tnum.log.$i \
					$testdir $t $locker $i $n &]
				lappend pidlist $p
			}
			watch_procs $pidlist 5

			# Now check output
			set dead 0
			set clean 0
			set other 0
			for { set i 0 } { $i < $n } { incr i } {
				set did [open $testdir/dead$tnum.log.$i]
				while { [gets $did val] != -1 } {
					# If the line comes from the 
					# profiling tool, ignore it. 
					if { [string first \
					    "profiling:" $val] == 0 } { 
						continue
					}
					switch $val {
						DEADLOCK { incr dead }
						1 { incr clean }
						default { incr other }
					}
				}
				close $did
			}
			tclkill $dpid
			puts "\tDead$tnum: dead check..."
			dead_check $t $n 0 $dead $clean $other
			# Now verify that the correct participant
			# got deadlocked.
			if {$pri == 0} {
				switch $t {
					maxlocks {set f [expr $n - 1]}
					maxwrites {set f 2}
					minlocks {set f 0}
					minwrites {set f 1}
				}
			} else {
				switch $t {
					maxlocks {set f [expr [expr $n / 2] - 1]}
					maxwrites {set f 0}
					minlocks {set f [expr $n / 2]}
					minwrites {set f 0}
				}
			}

			set did [open $testdir/dead$tnum.log.$f]
			error_check_bad file:$t [gets $did val] -1
			error_check_good read($f):$t $val DEADLOCK
			close $did
		}
		error_check_good lock_env:close [$env close] 0
		# Windows needs files closed before deleting them, so pause
		tclsleep 2
		fileremove -f $testdir/dd.out
		# Remove log files
		for { set i 0 } { $i < $n } { incr i } {
			fileremove -f $testdir/dead$tnum.log.$i
		}
	}
}
