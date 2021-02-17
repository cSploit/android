# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	dead003
# TEST
# TEST	Same test as dead002, but explicitly specify DB_LOCK_OLDEST and
# TEST	DB_LOCK_YOUNGEST.  Verify the correct lock was aborted/granted.
proc dead003 { {procs "2 4 10"} {tests "ring clump"} {tnum "003"} {pri 0} } {
	source ./include.tcl
	global lock_curid
	global lock_maxid

	set detects { oldest youngest }
	set msg ""
	if { $pri == 1 } {
		set msg " with priority"
	}
	puts "Dead$tnum: Deadlock detector tests: $detects"

	# Create the environment.
	foreach d $detects {
		env_cleanup $testdir
		puts "\tDead$tnum.a: creating environment for $d"
		set env [berkdb_env \
		    -create -mode 0644 -home $testdir -lock -lock_detect $d]
		error_check_good lock_env:open [is_valid_env $env] TRUE

		foreach t $tests {
			foreach n $procs {
				if {$pri == 1 && $n == 2} {
					puts "Skipping test for $n procs \
					    with priority."
				}
				set pidlist ""
				sentinel_init
				set ret [$env lock_id_set \
				     $lock_curid $lock_maxid]
				error_check_good lock_id_set $ret 0

				# Fire off the tests
				puts "\tDead$tnum: $n procs of test $t $msg"
				for { set i 0 } { $i < $n } { incr i } {
					set locker [$env lock_id]
					# If testing priorities, set the oldest
					# and youngest lockers to a higher
					# priority, meaning the second oldest
					# or second youngest locker will be
					# aborted.
					if {$pri == 1} {
						if {$i == 0 || \
						    $i == [expr $n - 1]} {
							$env lock_set_priority \
							    $locker 1
						} else {
							$env lock_set_priority \
							    $locker 0
						}
						
					}
					puts "$tclsh_path\
					    test_path/ddscript.tcl $testdir \
					    $t $locker $i $n >& \
					    $testdir/dead$tnum.log.$i"
					set p [exec $tclsh_path \
					    $test_path/wrap.tcl \
					    ddscript.tcl \
					    $testdir/dead$tnum.log.$i $testdir \
					    $t $locker $i $n &]
					lappend pidlist $p
				}
				watch_procs $pidlist 5

				# Now check output
				# dead: the number of aborted lockers
				# clean: the number of non-aborted lockers
				# killed: the highest aborted locker
				# kept: the highest non-aborted locker
				# In a ring, only one locker is aborted.
				# In a clump, only one locker is not aborted.
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
				puts "\tDead$tnum: dead check..."
				dead_check $t $n 0 $dead $clean $other
				#
				# If we get here we know we have the
				# correct number of dead/clean procs, as
				# checked by dead_check above.  Now verify
				# that the right process was the one.
				puts "\tDead$tnum: Verify $d locks were aborted"
				if {$pri == 0} {
					set l ""
					if { $d == "oldest" } {
						set l [expr $n - 1]
					}
					if { $d == "youngest" } {
						set l 0
					}
					set did [open $testdir/dead$tnum.log.$l]
					while { [gets $did val] != -1 } {
						# If the line comes from the 
						# profiling tool, ignore it. 
						if { [string first \
						    "profiling:" $val] == 0 } { 
							continue
						}
						error_check_good check_abort \
						    $val 1
					}
					close $did
				} else {
					if {$d == "oldest" && $t == "clump"} {
						error_check_good check_abort \
						    $kept [expr $n - 1]
					}
					if {$d == "oldest" && $t == "ring"} {
						error_check_good check_abort \
						    $killed 1
					}
					if {$d == "youngest" && $t == "clump"} {
						error_check_good check_abort \
						    $kept 0
					}
					if {$d == "youngest" && $t == "ring"} {
						error_check_good check_abort \
						    $killed [expr $n - 2]
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
}
