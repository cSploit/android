# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	dead002
# TEST	Same test as dead001, but use "detect on every collision" instead
# TEST	of separate deadlock detector.
proc dead002 { { procs "2 4 10" } {tests "ring clump" } \
    {timeout 0} {tnum 002} {pri 0} } {
	source ./include.tcl

	if {$timeout > 0 && $pri > 0} {
		puts "Dead$tnum: Both timeout and priority cannot be set."
		return
	}
	set msg ""
	if { $pri == 1 } {
		set msg " with priority"
	}
	puts "Dead$tnum: Deadlock detector tests (detect on every collision)"

	env_cleanup $testdir

	# Create the environment.
	puts "\tDead$tnum.a: creating environment"
	set lmode "default"
	if { $timeout != 0 } {
		set lmode "expire"
	}
	set env [berkdb_env \
	    -create -mode 0644 -home $testdir \
	    -lock -lock_timeout $timeout -lock_detect $lmode]
	error_check_good lock_env:open [is_valid_env $env] TRUE

	foreach t $tests {
		foreach n $procs {
			set pidlist ""
			sentinel_init

			# Fire off the tests
			puts "\tDead$tnum: $n procs of test $t $msg"
			for { set i 0 } { $i < $n } { incr i } {
				set locker [$env lock_id]
				if {$pri == 1} {
					$env lock_set_priority $locker $i
				}      
				puts "$tclsh_path $test_path/wrap.tcl \
				    ddscript.tcl $testdir/dead$tnum.log.$i \
				    $testdir $t $locker $i $n"
				set p [exec $tclsh_path $test_path/wrap.tcl \
					ddscript.tcl $testdir/dead$tnum.log.$i \
					$testdir $t $locker $i $n &]
				lappend pidlist $p
				# If we're running with timeouts, pause so that
				# locks will have a chance to time out.
				if { $timeout != 0 } {
					tclsleep 2
				}
			}
			watch_procs $pidlist 5

			# Now check output
			# dead: the number of aborted lockers
			# clean: the number of non-aborted lockers
			# killed: the highest aborted locker
			# kept: the highest non-aborted locker
			# In a ring, only one locker is aborted.  If testing
			# priorities, it should be 0, the lowest priority.
			# In a clump, only one locker is not aborted. If testing
			# priorities, it should be n, the highest priority.
			set dead 0
			set clean 0
			set other 0
			set killed $n
		 	set kept $n
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
						DEADLOCK { 
							incr dead
							set killed $i
						}
						1 {
							incr clean
							set kept $i
						}
						default { incr other }
					}
				}
				close $did
			}

			puts "\tDead$tnum: dead check ..."
			dead_check $t $n $timeout $dead $clean $other
			if { $pri == 1 } {
				if { $t == "ring" } {
					# Only the lowest priority killed in a
	       				# ring
	       				error_check_good low_priority_killed \
			       		    $killed 0
				} elseif { $t == "clump" } {
					# All but the highest priority killed in a
	       				# clump
			       		error_check_good high_priority_kept \
			       		    $kept [expr $n - 1]
				}
			}
		}
	}

	fileremove -f $testdir/dd.out
	# Remove log files
	for { set i 0 } { $i < $n } { incr i } {
		fileremove -f $testdir/dead$tnum.log.$i
	}
	error_check_good lock_env:close [$env close] 0
}
