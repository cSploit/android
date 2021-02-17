# See the file LICENSE for redistribution information.
#
# Copyright (c) 1999, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	db_reptest
# TEST	Wrapper to configure and run the db_reptest program.

#
# TODO:
# late client start.
#

global last_nsites
set last_nsites 0

global os_tbase
set os_tbase 1

#
# There are several user-level procs that the user may invoke.
# 1. db_reptest - Runs randomized configurations in a loop.
# 2. basic_db_reptest - Runs a simple set configuration once,
#	as a smoke test.
# 3. restore_db_reptest 'dir' - Runs the configuration given in 'dir'
#	in a loop.  The purpose is either to reproduce a problem
#	that some configuration encountered, or test a fix.
# 4. db_reptest_prof - Runs a single randomized configuration
#	and generates gprof profiling information for that run.
# 5. basic_db_reptest_prof - Runs a simple set configuration and
#	generates gprof profiling information for that run.
# 6. restore_db_reptest_prof - Runs the configuration given in 'dir' and
#	generates gprof profiling information for one run.
#

#
# db_reptest - Run a randomized configuration.  Run the test
# 'count' times in a loop, or until 'stopstr' is seen in the OUTPUT
# files or if no count or string is given, it is an infinite loop.
#
proc db_reptest { { stopstr "" } {count -1} } {
	berkdb srand [pid]
	set cmd "db_reptest_int random"
	db_reptest_loop $cmd $stopstr $count
}

#
# Run a basic reptest.  The types are:
# Basic 0 - Two sites, start with site 1 as master, 5 worker threads, btree,
#	run 100 seconds, onesite remote knowledge.
# Basic 1 - Three sites, all sites start as client, 5 worker threads, btree
#	run 150 seconds, full remote knowledge.
#
proc basic_db_reptest { { basic 0 } } {
	global util_path

	if { [file exists $util_path/db_reptest] == 0 } {
		puts "Skipping db_reptest.  Is it built?"
		return
	}
	if { $basic == 0 } {
		db_reptest_int basic0
	}
	if { $basic == 1 } {
		db_reptest_int basic1
	}
}

proc basic_db_reptest_prof { { basic 0 } } {
	basic_db_reptest $basic
	generate_profiles
}

#
# Restore a configuration from the given directory and
# run that configuration in a loop 'count' times or until
# 'stopstr' is seen in the OUTPUT files or if no count or
# string is given, it is an infinite loop.
#
proc restore_db_reptest { restoredir { stopstr "" } { count -1 } } {
	set cmd "db_reptest_int restore $restoredir/SAVE_RUN"
	db_reptest_loop $cmd $stopstr $count
}

proc restore_db_reptest_prof { restoredir } {
	restore_db_reptest $restoredir "" 1
	generate_profiles
}

#
# Run a single randomized iteration and then generate the profile
# information for each site.
#
proc db_reptest_prof { } {
	berkdb srand [pid]
	set cmd "db_reptest_int random"
	db_reptest_loop $cmd "" 1
	generate_profiles
}

proc generate_profiles {} {
	global dirs
	global use
	global util_path

	#
	# Once it is complete, generate profile information.
	#
	for { set i 1 } { $i <= $use(nsites) } { incr i } {
		set gmon NULL
		set known_gmons \
		    { $dirs(env.$i)/db_reptest.gmon $dirs(env.$i)/gmon.out }
		foreach gfile $known_gmons {
			if { [file exists $gfile] } {
				set gmon $gfile
				break
			}
		}
		if { $gmon == "NULL" } {
			puts "No gmon file.  Was it built with profiling?"
			return
		}
		set prof_out db_reptest.$i.OUT
		set stat [catch {exec gprof $util_path/db_reptest \
		    $gmon >>& $prof_out} ret]
		if { $stat != 0 } {
			puts "FAIL: gprof: $ret"
		}
		error_check_good gprof $stat 0
		puts "Profiled output for site $i: $prof_out"
	}
}

proc db_reptest_profile { } {
	db_reptest_prof
}

#
# Wrapper to run the command in a loop, 'count' times.
#
proc db_reptest_loop { cmd stopstr count } {
	global util_path

	if { [file exists $util_path/db_reptest] == 0 } {
		puts "Skipping db_reptest.  Is it built?"
		return
	}
	set iteration 1
	set start_time [clock format [clock seconds] -format "%H:%M %D"]
	while { 1 } {
		puts -nonewline "ITERATION $iteration: "
		puts -nonewline \
		    [clock format [clock seconds] -format "%H:%M %D"]
		puts " (Started: $start_time)"

		#
		eval $cmd

		puts -nonewline "COMPLETED $iteration: "
		puts [clock format [clock seconds] -format "%H:%M %D"]
		incr iteration
		#
		# If we've been given a string to look for, run until we
		# see it.  Or if not, skip to the count check.
		#
		if { [string length $stopstr] > 0 } {
			set found [search_output $stopstr]
			if { $found } {
				break
			}
		}
		if { $count > 0 && $iteration > $count } {
			break
		}
	}
}

#
# Internal version of db_reptest that all user-level procs
# eventually call.  It will configure a single run of
# db_reptest based on the configuration type specified
# in 'cfgtype'.  This proc will:
# Configure a run of db_reptest
# Run db_reptest
# Verify the sites after db_reptest completes.
#
proc db_reptest_int { cfgtype { restoredir NULL } } {
	source ./include.tcl
	global dirs
	global os_tbase
	global use

	env_cleanup $testdir

	set dirs(save) TESTDIR/SAVE_RUN
	set dirs(restore) $restoredir
	reptest_cleanup $dirs(save)

	#
	# Set up the array to indicate if we are going to use
	# a metadata dir or database data dir.  If we are using
	# datadirs, decide which one gets the created database.
	#
	get_datadirs $cfgtype use dirs

	#
	# Get all the default or random values needed for the test
	# and its args first.
	#
	set runtime 0
	
	#
	# Set the time basis for each platform.  It is used to calculate
	# timeouts and waiting times. Some slower platforms need to use
	# longer time values for this test to succeed.
	#
	if { $is_windows_test == 1 } {
		set os_tbase 3
	}
		
	#
	# Get number of sites first because pretty much everything else
	# after here depends on how many sites there are.
	#
	set use(nsites) [get_nsites $cfgtype $dirs(restore)]
	set use(lease) [get_lease $cfgtype $dirs(restore)]
	set use(peers) [get_peers $cfgtype]
	set use(view) 0
	set use(view_site) 0
	#
	# Get port information in case it needs to be converted for this
	# run.  A conversion will happen for a restored run if the current
	# baseport is different than the one used in restoredir.
	#
	set portlist [available_ports $use(nsites)]
	set baseport(curr) [expr [lindex $portlist 0] - 1]
	set baseport(orig) [get_orig_baseport $cfgtype $dirs(restore)]
	#
	# Only use kill if we have > 2 sites.
	# Returns a list.  An empty list means this will not be a kill test.
	# Otherwise the list has 3 values, the kill type and 2 kill sites.
	# See the 'get_kill' proc for a description of kill types.
	#
	set use(kill) ""
	set kill_type "NONE"
	set kill_site 0
	set kill_remover 0
	set site_remove 0
	set kill_self 0
	if { $use(nsites) > 2 } {
		set use(kill) [get_kill $cfgtype \
		    $dirs(restore) $use(nsites) baseport]
		if { [llength $use(kill)] > 0 } {
			set kill_type [lindex $use(kill) 0]
			set kill_site [lindex $use(kill) 1]
			set kill_remover [lindex $use(kill) 2]
			# Remember a site that is supposed to kill itself.
			if { $kill_type == "DIE" || $kill_type == "REMOVE" } {
				set kill_self $kill_site
			}
		} else {
			# If we are not doing a kill test, determine if
			# we are doing a remove test.
			set site_remove [get_remove $cfgtype $dirs(restore) \
			    $use(nsites)]
		}
		if { $cfgtype == "restore" } {
			set use(view) [get_view $cfgtype $dirs(restore) \
			    NULL NULL $use(nsites) $kill_self]
			set use(view_site) [expr {abs($use(view))}]
		}
	}
	if { $cfgtype != "restore" } {
		if { $use(lease) } {
			set use(master) 0
		} else {
			set use(master) [get_usemaster $cfgtype]
			if { $site_remove == $use(master) } {
				set site_remove 0
			}
		}
		set master_site [get_mastersite $cfgtype $use(master) $use(nsites)]
		set noelect [get_noelect $use(master)]
		set master2_site [get_secondary_master \
		    $noelect $master_site $kill_site $use(nsites)]
		set use(view) [get_view $cfgtype NULL $master_site \
		    $master2_site $use(nsites) $kill_self]
		set use(view_site) [expr {abs($use(view))}]
		set autotakeover_site [get_autotakeover $use(kill) \
		    $site_remove $use(view) $use(view_site) $use(nsites)]
		set workers [get_workers $cfgtype $use(lease)]
		set dbtype [get_dbtype $cfgtype]
		set runtime [get_runtime $cfgtype]
		puts "Running: $use(nsites) sites, $runtime seconds."
		puts -nonewline "Running: "
		if { $use(createdir) } {
			puts -nonewline \
    "$use(datadir) datadirs, createdir DATA.$use(createdir), "
		}
		if { $use(metadir) } {
			puts -nonewline "METADIR, "
		}
		if { $kill_type == "DIE" || $kill_type == "REMOVE" } {
			puts -nonewline "kill site $kill_site, "
		}
		if { $kill_type == "LIVE_REM" } {
			puts -nonewline \
			    "live removal of site $kill_site by $kill_remover, "
		} elseif { $site_remove } {
			puts -nonewline "remove site $site_remove, "
		}
		if { $autotakeover_site } {
			puts -nonewline "autotakeover site $autotakeover_site, "
		}
		if { $use(view_site) } {
			if { $use(view) < 0 } {
				set vstat "empty"
			} else {
				set vstat "full"
			}
			puts -nonewline "$vstat view site $use(view_site), "
		}
		if { $use(lease) } {
			puts "with leases."
		} elseif { $use(master) } {
			set master_text "master site $master_site"
			if { $noelect } {
				set master_text [concat $master_text \
				    "no elections"]
			}
			if { $master2_site } {
				set master_text [concat $master_text \
				    "secondary master site $master2_site"]
			}
			puts "$master_text."
		} else {
			puts "no master."
		}
	}
	#
	# This loop sets up the args to the invocation of db_reptest
	# for each site.
	#
	for { set i 1 } {$i <= $use(nsites) } { incr i } {
		set dirs(env.$i) TESTDIR/ENV$i
		set dirs(home.$i) ../ENV$i
		reptest_cleanup $dirs(env.$i)
		#
		# If we are restoring the args, just read them from the
		# saved location for this sites.  Otherwise build up
		# the args for each piece we need.
		#
		if { $cfgtype == "restore" } {
			set cid [open $dirs(restore)/DB_REPTEST_ARGS.$i r]
			set prog_args($i) [read $cid]
			close $cid
			#
			# Convert -K port number arguments to current
			# baseport if needed.  regsub -all substitutes
			# all occurrences of pattern, which is "-K "
			# and a number.  The result of regsub contains a tcl
			# expression with the number (\2, the second part of
			# the pattern), operators and variable names, e.g.:
			#   -K [expr 30104 - $baseport(orig) + $baseport(curr)]
			# and then subst evalutes the tcl expression.
			#
			if { $baseport(curr) != $baseport(orig) } {
				regsub -all {(-K )([0-9]+)} $prog_args($i) \
				    {-K [expr \2 - $baseport(orig) + \
				    $baseport(curr)]} prog_args($i)
				set prog_args($i) [subst $prog_args($i)]
			}
			if { $runtime == 0 } {
				set runtime [parse_runtime $prog_args($i)]
				puts "Runtime: $runtime"
			}
		} else {
			set nmsg [berkdb random_int 1 [expr $use(nsites) * 2]]
			set prog_args($i) \
			    "-v -c $workers -t $dbtype -T $runtime -m $nmsg "
			set prog_args($i) \
			    [concat $prog_args($i) "-h $dirs(home.$i)"]
			set prog_args($i) \
			    [concat $prog_args($i) "-o $use(nsites)"]
			#
			# Add in if this site should remove itself.
			#
			if { $site_remove == $i } {
				set prog_args($i) [concat $prog_args($i) "-r"]
			}
			#
			# Add in if this site should kill itself.
			#
			if { ($kill_type == "DIE" || $kill_type == "REMOVE") && \
			    $kill_site == $i} {
				set prog_args($i) [concat $prog_args($i) "-k"]
			}
			#
			# Add in if this site should remove a killed site.
			#
			if { $kill_remover == $i } {
				set kport [lindex $portlist \
				    [expr $kill_site - 1]]
				set prog_args($i) [concat $prog_args($i) \
				    "-K $kport"]
			}
			#
			# Add in if this site starts as master, client or view.
			#
			if { $i == $master_site } {
				set state($i) MASTER
				set prog_args($i) [concat $prog_args($i) "-M"]
			} else {
				set state($i) CLIENT
				#
				# If we have a master, then we just want to
				# start as a client.  Otherwise start with
				# elections.
				#
				if { $use(view_site) != 0 && \
				    $use(view_site) == $i } {
					if { $use(view) < 0 } {
						set prog_args($i) \
						    [concat $prog_args($i) "-V 0"]
					} else {
						set prog_args($i) \
						    [concat $prog_args($i) "-V 1"]
					}
				} else {
					if { $use(master) } {
						set prog_args($i) \
						    [concat $prog_args($i) "-C"]
					} else {
						set prog_args($i) \
						    [concat $prog_args($i) "-E"]
					}
				}
			}
			#
			# Add in if we are in no elections mode and if we are 
			# the secondary master.
			#
			if { $noelect } {
				set prog_args($i) [concat $prog_args($i) "-n"]
				if { $i == $master2_site } {
					set prog_args($i) \
					    [concat $prog_args($i) "-s"]
				}
			}
			#
			# Add in if this site should do an autotakeover.
			#
			if { $autotakeover_site == $i } {
				set prog_args($i) [concat $prog_args($i) "-a"]
			}
		}
		save_db_reptest $dirs(save) ARGS $i $prog_args($i)
	}

	# Now make the DB_CONFIG file for each site.
	reptest_make_config $cfgtype dirs state use $portlist baseport

	# Run the test
	run_db_reptest dirs $use(nsites) $runtime $use(lease)
	puts "Test run complete.  Verify."

	# Verify the test run.
	verify_db_reptest $use(nsites) dirs use $kill_site $site_remove

	# Show the summary files
	print_summary

}

#
# Make a DB_CONFIG file for all sites in the group
#
proc reptest_make_config { cfgtype dirsarr starr usearr portlist baseptarr } {
	upvar $dirsarr dirs
	upvar $starr state
	upvar $baseptarr baseport
	upvar $usearr use
	global os_tbase
	global rporttype

	#
	# Generate global config values that should be the same
	# across all sites, such as number of sites and log size, etc.
	#
	set rporttype NULL
	set default_cfglist {
	{ "set_flags" "DB_TXN_NOSYNC" }
	{ "rep_set_request" "150000 2400000" }
	{ "rep_set_timeout" "db_rep_checkpoint_delay 0" }
	{ "rep_set_timeout" "db_rep_connection_retry 2000000" }
	{ "rep_set_timeout" "db_rep_heartbeat_send 500000" }
	{ "set_cachesize"  "0 4194304 1" }
	{ "set_lg_max" "131072" }
	{ "set_lk_detect" "db_lock_default" }
	{ "set_verbose" "db_verb_recovery" }
	{ "set_verbose" "db_verb_replication" }
	}

	set acks { db_repmgr_acks_all db_repmgr_acks_all_peers \
	    db_repmgr_acks_none db_repmgr_acks_one db_repmgr_acks_one_peer \
	    db_repmgr_acks_quorum }

	#
	# 2site strict and ack policy must be the same on all sites.
	#
	if { $cfgtype == "random" } {
		if { $use(nsites) == 2 } {
			set strict [berkdb random_int 0 1]
		} else {
			set strict 0
		}
		if { $use(lease) } {
			#
			# 2site strict with leases must have ack policy of
			# one because quorum acks are ignored in this case,
			# resulting in lease expired panics on some platforms.
			#
			if { $strict } {
				set ackpolicy db_repmgr_acks_one
			} else {
				set ackpolicy db_repmgr_acks_quorum
			}
		} else {
			set done 0
			while { $done == 0 } {
				set acksz [expr [llength $acks] - 1]
				set myack [berkdb random_int 0 $acksz]
				set ackpolicy [lindex $acks $myack]
				#
				# Only allow the "none" policy with 2 sites
				# otherwise it can overwhelm the system and
				# it is a rarely used option.
				#
				if { $ackpolicy == "db_repmgr_acks_none" && \
				    $use(nsites) > 2 } {
					continue
				}
				#
				# Only allow "all" or "all_peers" policies
				# if not killing a site, otherwise the
				# unavailable site will cause the master
				# to ignore acks and blast the clients with
				# log records.
				#
				if { [llength $use(kill)] > 0 && \
				    ($ackpolicy == "db_repmgr_acks_all" || \
				    $ackpolicy == 
				    "db_repmgr_acks_all_peers") } {
					continue
				}
				set done 1
			}
		}
	} else {
		set ackpolicy db_repmgr_acks_one
	}
	#
	# Set known_master to the initial master or if one is not
	# assigned, randomly assign the group creator.
	#
	set known_master 0
	if { $cfgtype != "restore" } {
		for { set i 1 } { $i <= $use(nsites) } { incr i } {
			if { $state($i) == "MASTER" } {
				set known_master $i
			}
		}
		if { $known_master == 0 } {
			set known_master [berkdb random_int 1 $use(nsites)]
			while { $known_master == $use(view_site) } {
				set known_master [berkdb random_int 1 $use(nsites)]
			}
		}
	}
	for { set i 1 } { $i <= $use(nsites) } { incr i } {
		#
		# If we're restoring we just need to copy it.
		#
		if { $cfgtype == "restore" } {
			#
			# Convert DB_CONFIG port numbers to current baseport
			# if needed.
			#
			set restore_cfile $dirs(restore)/DB_CONFIG.$i
			set new_cfile $dirs(env.$i)/DB_CONFIG
			set new_save_cfile $dirs(save)/DB_CONFIG.$i
			if { $baseport(curr) != $baseport(orig) } {
				convert_config_ports $restore_cfile \
				    $new_cfile baseport
				file copy $new_cfile $new_save_cfile
			} else {
				file copy $restore_cfile $new_cfile
				file copy $restore_cfile $new_save_cfile
			}
			if { $use(metadir) } {
				file mkdir $dirs(env.$i)/METADIR
			}
			if { $use(datadir) } {
				for { set diri 1 } { $diri <= $use(datadir) } \
				    { incr diri } {
					file mkdir $dirs(env.$i)/DATA.$diri
				}
			}
			continue
		}
		#
		# Otherwise set up per-site config information
		#
		set cfglist $default_cfglist

		#
		# Add lease configuration if needed.  We're running all
		# locally, so there is no clock skew.
		#
		set allist [get_ack_lease_timeouts $use(lease)]
		if { $use(lease) } {
			#
			# We need to have an ack timeout > lease timeout.
			# Otherwise txns can get committed without waiting
			# long enough for leases to get granted.
			#
			lappend cfglist { "rep_set_config" "db_rep_conf_lease" }
			lappend cfglist { "rep_set_timeout" \
			    "db_rep_lease_timeout [lindex $allist 1]" }
			lappend cfglist { "rep_set_timeout" \
			    "db_rep_ack_timeout [lindex $allist 0]" }
		} else {
			lappend cfglist { "rep_set_timeout" \
			    "db_rep_ack_timeout [lindex $allist 0]" }
		}

		#
		# Add heartbeat_monitor.  Calculate value based on number of
		# sites to reduce spurious heartbeat expirations.
		#
		lappend cfglist { "rep_set_timeout" \
		    "db_rep_heartbeat_monitor \
		    [expr $use(nsites) * 500000 * $os_tbase]" }

		#
		# Add datadirs and the metadir, if needed.  If we are using
		# datadirs, then set which one is the create dir.
		#
		if { $use(metadir) } {
			file mkdir $dirs(env.$i)/METADIR
			lappend cfglist { "set_metadata_dir" "METADIR" }
		}
		if { $use(datadir) } {
			for { set diri 1 } { $diri <= $use(datadir) } \
			    { incr diri } {
				file mkdir $dirs(env.$i)/DATA.$diri
				#
				# Need to add to list in 2 steps otherwise
				# $diri in the list will not get evaluated
				# until later.
				#
				set litem [list add_data_dir DATA.$diri]
				lappend cfglist $litem
			}
			lappend cfglist { "set_create_dir" \
			    "DATA.$use(createdir)" }
		}

		#
		# Priority
		#
		if { $state($i) == "MASTER" } {
			lappend cfglist { "rep_set_priority" 100 }
		} else {
			if { $cfgtype == "random" } {
				set pri [berkdb random_int 10 25]
			} else {
				set pri 20
			}
			set litem [list rep_set_priority $pri]
			lappend cfglist $litem
		}
		#
		# Others: limit size, bulk, 2site strict
		#
		if { $cfgtype == "random" } {
			set limit_sz [berkdb random_int 15000 1000000]
			set bulk [berkdb random_int 0 1]
			if { $bulk } {
				lappend cfglist \
				    { "rep_set_config" "db_rep_conf_bulk" }
			}
			#
			# 2site strict was set above for all sites but
			# should only be used for sites in random configs.
			#
			if { $strict } {
				lappend cfglist { "rep_set_config" \
				    "db_repmgr_conf_2site_strict" }
			} else {
				lappend cfglist { "rep_set_config" \
				    "db_repmgr_conf_2site_strict off" }
			}
		} else {
			set limit_sz 100000
		}
		set litem [list rep_set_limit "0 $limit_sz"]
		lappend cfglist $litem
		set litem [list repmgr_set_ack_policy $ackpolicy]
		lappend cfglist $litem
		#
		# Now set up the local and remote ports.  If we are the
		# known_master (either master or group creator) set the
		# group creator flag on.
		#
		# Must use explicit 127.0.0.1 rather than localhost because
		# localhost can be configured differently on different
		# machines or platforms.  Use of localhost can cause 
		# available_ports to return ports that are actually in use.
		#
		set lport($i) [lindex $portlist [expr $i - 1]]
		if { $i == $known_master } {
			#
			# Any change to this generated syntax will probably
			# require a change to get_orig_baseport, which relies
			# on this ordering and these embedded spaces.
			#
			set litem [list repmgr_site \
			    "127.0.0.1 $lport($i) db_local_site on \
			    db_group_creator on"]
		} else {
			set litem [list repmgr_site \
			    "127.0.0.1 $lport($i) db_local_site on"]
		}
		lappend cfglist $litem
		set rport($i) [get_rport $portlist $i $use(nsites) \
		    $known_master $cfgtype]
		#
		# Declare all sites bootstrap helpers.
		#
		foreach p $rport($i) {
			if { $use(peers) } {
				set litem [list repmgr_site "127.0.0.1 $p \
				    db_bootstrap_helper on db_repmgr_peer on"]
			} else {
				set litem [list repmgr_site "127.0.0.1 $p \
				    db_bootstrap_helper on"]
			}
			#
			# If we have full knowledge, assume a legacy system.
			#
			if { $cfgtype == "full" } {
				lappend litem "db_legacy on"
			}
			lappend cfglist $litem
		}
		#
		# Now write out the DB_CONFIG file.
		#
		set cid [open $dirs(env.$i)/DB_CONFIG a]
		foreach c $cfglist {
			set carg [subst [lindex $c 0]]
			set cval [subst [lindex $c 1]]
			puts $cid "$carg $cval"
		}
		close $cid
		set cid [open $dirs(env.$i)/DB_CONFIG r]
		set cfg [read $cid]
		close $cid
	
		save_db_reptest $dirs(save) CONFIG $i $cfg
	}

}

proc reptest_cleanup { dir } {
	#
	# For now, just completely remove it all.  We might want
	# to use env_cleanup at some point in the future.
	#
	fileremove -f $dir
	file mkdir $dir
}


proc save_db_reptest { savedir op site savelist } {
	#
	# Save a copy of the configuration and args used to run this
	# instance of the test.
	#
	if { $op == "CONFIG" } {
		set outfile $savedir/DB_CONFIG.$site
	} else {
		set outfile $savedir/DB_REPTEST_ARGS.$site
	}
	set cid [open $outfile a]
	puts -nonewline $cid $savelist
	close $cid
}

proc run_db_reptest { dirsarr numsites runtime use_lease } {
	source ./include.tcl
	upvar $dirsarr dirs
	global killed_procs

	set pids {}
	#
	# Wait three times workload run time plus an ack_timeout for each site
	# to kill a run.  The ack_timeout is especially significant for runs
	# where leases are in use because they take much longer to get started.
	#
	set ack_timeout [lindex [get_ack_lease_timeouts $use_lease] 0]
	set watch_time [expr $runtime * 3 + \
	    [expr $ack_timeout / 1000000] * $numsites]
	for { set i 1 } { $i <= $numsites } { incr i } {
		lappend pids [exec $tclsh_path $test_path/wrap_reptest.tcl \
		    $dirs(save)/DB_REPTEST_ARGS.$i $dirs(env.$i) \
		    $dirs(save)/site$i.log &]
		tclsleep 1
	}
	watch_procs $pids 15 $watch_time
	set killed [llength $killed_procs]
	if { $killed > 0 } {
		puts \
"Processes $killed_procs never finished, saving db_stat -E for all envs in $dirs(save)/site#.dbstatE"
		for { set i 1 } { $i <= $numsites } { incr i } {
			set statout $dirs(save)/site$i.dbstatE
			set stat [catch {exec $util_path/db_stat \
			    -N -E -h $dirs(env.$i) >& $statout} result] 
		}
		error "Processes $killed_procs never finished"
	}
}

proc verify_db_reptest { num_sites dirsarr usearr kill site_rem } {
	upvar $dirsarr dirs
	upvar $usearr use

	for { set startenv 1 } { $startenv <= $num_sites } { incr startenv } {
		#
		# Find the first full, real copy of the run.
		# We skip an environment that was killed in the middle
		# of the test, or a site that was removed from the group
		# in the middle of the test, or an empty view site.
		#
		if { $kill == $startenv || $site_rem == $startenv || 
		    $startenv == $use(view_site) && $use(view) < 0 } {
			#
			# If it is an empty view, verify it is empty since
			# we won't visit this one again later.
			#
			if { $startenv == $use(view_site) && $use(view) < 0 } {
				puts "View $startenv: Verify am1.db doesn't exist"
				error_check_good am1db [file exists {eval \
				    $dirs(env.$startenv)/$datadir/am1.db}] 0
			}
			continue
		}
		#
		# If it is a real site, we have a winner.  Stop now.
		#
		break
	}
	set cmpeid [expr $startenv + 1]
	set envbase [berkdb_env_noerr -home $dirs(env.$startenv)]
	set datadir ""
	if { $use(createdir) } {
		set datadir DATA.$use(createdir)
	}
	for { set i $cmpeid } { $i <= $num_sites } { incr i } {
		if { $i == $kill || $i == $site_rem } {
			continue
		}
		if { $i == $use(view_site) && $use(view) < 0 } {
			#
			# If this is an empty view, make sure that the db
			# does not exist on this site.
			#
			puts "View $i: Verify am1.db does not exist"
			error_check_good am1db [file exists \
			    {eval $dirs(env.$i)/$datadir/am1.db}] 0
			continue
		}
		set cmpenv [berkdb_env_noerr -home $dirs(env.$i)]
		puts "Compare $dirs(env.$startenv) with $dirs(env.$i)"
		#
		# Compare 2 envs.  We assume the name of the database that
		# db_reptest creates and know it is 'am1.db'.
		# We want as other args:
		# 0 - compare_shared_portion
		# 1 - match databases
		# 0 - don't compare logs (for now)
		rep_verify $dirs(env.$startenv) $envbase $dirs(env.$i) $cmpenv \
		    0 1 0 am1.db $datadir
		$cmpenv close
	}
	$envbase close
}

proc get_nsites { cfgtype restoredir } {
	global last_nsites

	#
	# Figure out the number of sites.  We use 'glob' to get all of
	# the valid DB_CONFIG files in the restoredir.  That command uses
	# a single digit match, so the maximum number of sites must be <= 9.
	# Match DB_CONFIG.# so that it does not consider anything like an
	# emacs save file.
	#
	set maxsites 5
	#
	# If someone changes maxsites to be too big, it will break the
	# 'glob' below.  Catch that now.
	#
	if { $maxsites > 9 } {
		error "Max sites too large."
	}
	if { $cfgtype == "restore" } {
		set ret [catch {glob $restoredir/DB_CONFIG.\[1-$maxsites\]} \
		    result]
		if { $ret != 0 } {
			error "Could not get config list: $result"
		}
		return [llength $result]
	}
	if { $cfgtype == "random" } {
		#
		# Sometimes 'random' doesn't seem to do a good job.  I have
		# seen on all iterations after the first one, nsites is
		# always 2, 100% of the time.  Add this bit to make sure
		# this nsites values is different from the last iteration.
		#
		set n [berkdb random_int 2 $maxsites]
		while { $n == $last_nsites } {
			set n [berkdb random_int 2 $maxsites]
puts "Getting random nsites between 2 and $maxsites.  Got $n, last_nsites $last_nsites"
		}
		set last_nsites $n
		return $n
	}
	if { $cfgtype == "basic0" } {
		return 2
	}
	if { $cfgtype == "basic1" } {
		return 3
	}
	return -1
}

#
# Run with master leases?  25%/75% (use a master lease 25% of the time).
#
proc get_lease { cfgtype restoredir } {
	#
	# The number of sites must be the same for all.  Read the
	# first site's saved DB_CONFIG file if we're restoring since
	# we only know we have at least 1 site.
	#
	if { $cfgtype == "restore" } {
		set uselease 0
		set cid [open $restoredir/DB_CONFIG.1 r]
		while { [gets $cid cfglist] } {
#			puts "Read in: $cfglist"
			if { [llength $cfglist] == 0 } {
				break;
			}
			set cfg [lindex $cfglist 0]
			if { $cfg == "rep_set_config" } {
				set lease [lindex $cfglist 1]
				if { $lease == "db_rep_conf_lease" } {
					set uselease 1
					break;
				}
			}
		}
		close $cid
		return $uselease
	}
	if { $cfgtype == "random" } {
		set leases { 1 0 0 0 }
		set len [expr [llength $leases] - 1]
		set i [berkdb random_int 0 $len]
		return [lindex $leases $i]
	}
	if { $cfgtype == "basic0" } {
		return 0
	}
	if { $cfgtype == "basic1" } {
		return 0
	}
}

#
# Do a kill test about half the time.  We randomly choose a
# site number to kill, it could be a master or a client.  If
# we want to remove the site from the group, randomly choose
# a site to do the removal.
#
# We return a list with the kill type and the sites.  Return
# an empty list if we don't kill any site.  There are a few variants:
#
# 1: Die - A site just kills itself but remains part of the group.
# Return a list {DIE deadsite# 0}.
# 2: Removal - A site kills itself, and some site will also remove
# the dead site from the group. (Could be the same site that is dying,
# in which case the removal is done right before it exits.)
# {REMOVE deadsite# removalsite#}.
# 3. Live removal - Some site removes another live site from the group.
# (Could be itself.)
# {LIVE_REM killsite# removalsite#}.
#
proc get_kill { cfgtype restoredir num_sites basept } {
	upvar $basept baseport

	set nokill ""
	if { $cfgtype == "restore" } {
		set ksite 0
		set localkill 0
		set rkill 0
		set rsite 0
		set kport 0
		set ktype NONE
		for { set i 1 } { $i <= $num_sites } { incr i } {
			set cid [open $restoredir/DB_REPTEST_ARGS.$i r]
			# !!!
			# We currently assume the args file is 1 line.
			#
			gets $cid arglist
			close $cid
#			puts "Read in: $arglist"
			set dokill [lsearch $arglist "-k"]
			set dorem [lsearch $arglist "-K"]
			#
			# Only 1 of those args should ever be set for a given
			# input line.  We need to look at all sites in order
			# to determine the kill type.  If we find both -k and
			# -K, the site will be the same, so overwriting it
			# no matter what order the sites, is okay.
			#
			if { $dokill >= 0 } {
				set ksite $i
				set localkill 1
			}
			#
			# If it is a remote removal kill type, we are
			# the site doing the removing and we need to get
			# the site to remove from the arg.  $dorem is the
			# index of the arg, so + 1 is the site number.
			# The site in the arg is the port number so grab
			# the site number out of it.
			#
			if { $dorem >= 0 } {
				set rkill 1
				set kport [lindex $arglist [expr $dorem + 1]]
				set ksite [expr $kport - $baseport(orig)]
				# Convert kport to current baseport if needed.
				if { $baseport(curr) != $baseport(orig) } {
					set kport [expr $kport - \
					    $baseport(orig) + $baseport(curr)]
				}
				set rsite $i
			}
		}
		#
		# If we have a remote kill, then we decide the kill type
		# based on whether the killed site will be dead or alive.
		# If we found no site to kill/remove, we know it is not
		# a kill test.
		#
		if { $ksite == 0 } {
			return $nokill
		} else {
			#
			# See proc comment for a definition of each kill type.
			#
			if { $localkill == 1 && $rkill == 0 } {
				set ktype DIE
			}
			if { $localkill == 1 && $rkill == 1 } {
				set ktype REMOVE
			}
			if { $localkill == 0 && $rkill == 2 } {
				set ktype LIVE_REM
			}
			return [list $ktype $ksite $rsite]
		}
	}
	if { $cfgtype == "random" } {
		# Do a kill and/or removal test 40% of the time.
		set k { 0 0 0 1 0 1 0 1 1 0 }
		set len [expr [llength $k] - 1]
		set i [berkdb random_int 0 $len]
		set dokill [lindex $k $i]
		set i [berkdb random_int 0 $len]
		set dorem [lindex $k $i]
		#
		# Set up for the possibilities listed above.
		#
		if { $dokill == 0 && $dorem == 0 } {
			return $nokill
		}
		#
		# Choose which sites to kill and do removal.
		#
		set ksite [berkdb random_int 1 $num_sites]
		set rsite [berkdb random_int 1 $num_sites]
		if { $dokill == 1 && $dorem == 0 } {
			set ktype DIE
			set rsite 0
		}
		if { $dokill == 1 && $dorem == 1 } {
			set ktype REMOVE
		}
		if { $dokill == 0 && $dorem == 1 } {
			set ktype LIVE_REM
		}
		return [list $ktype $ksite $rsite]
	}
	if { $cfgtype == "basic0" || $cfgtype == "basic1" } {
		return $nokill
	} else {
		error "Get_kill: Invalid config type $cfgtype"
	}
}

#
# If we want to run a remove/rejoin, which site?  This proc
# will return a site number of a site to remove/rejoin or
# it will return 0 if no removal test.  Sites are numbered
# starting at 1.
#
proc get_remove { cfgtype restoredir nsites } {
	set rsite 0
	if { $cfgtype == "random" } {
		# Do a remove test half the time we're called.
		set k { 0 0 0 1 1 1 0 1 1 0 }
		set len [expr [llength $k] - 1]
		set i [berkdb random_int 0 $len]
		if { [lindex $k $i] == 1 } {
			set rsite [berkdb random_int 1 $nsites]
		}
	} elseif { $cfgtype == "restore" } {
		#
		# If we're restoring we still need to know if a site is
		# running its own removal test so we know to skip it for verify.
		#
		for { set i 1 } { $i <= $nsites } { incr i } {
			set cid [open $restoredir/DB_REPTEST_ARGS.$i r]
			# !!!
			# We currently assume the args file is 1 line.
			# This code also assumes only 1 site ever does removal.
			#
			gets $cid arglist
			close $cid
#			puts "Read in: $arglist"
			set dorem [lsearch $arglist "-r"]
			if { $dorem >= 0 } {
				set rsite $i
				#
				# If we find one, we know no other site will
				# be doing removal.  So stop now.
				#
				break
			}
		}
	}
	return $rsite
}

#
# Use peers or only the master for requests? 25%/75% (use a peer 25%
# of the time and master 75%)
#
proc get_peers { cfgtype } {
	if { $cfgtype == "random" } {
		set peer { 0 0 0 1 }
		set len [expr [llength $peer] - 1]
		set i [berkdb random_int 0 $len]
		return [lindex $peer $i]
	} else {
		return 0
	}
}

#
# Start with a master or all clients?  25%/75% (use a master 25%
# of the time and have all clients 75%)
#
proc get_usemaster { cfgtype } {
	if { $cfgtype == "random" } {
		set mst { 1 0 0 0 }
		set len [expr [llength $mst] - 1]
		set i [berkdb random_int 0 $len]
		return [lindex $mst $i]
	}
	if { $cfgtype == "basic0" } {
		return 1
	}
	if { $cfgtype == "basic1" } {
		return 0
	}
}

#
# If we use a master, which site?  This proc will return
# the site number of the mastersite, or it will return
# 0 if no site should start as master.  Sites are numbered
# starting at 1.
#
proc get_mastersite { cfgtype usemaster nsites } {
	if { $usemaster == 0 } {
		return 0
	}
	if { $cfgtype == "random" } {
		return [berkdb random_int 1 $nsites]
	}
	if { $cfgtype == "basic0" } {
		return 1
	}
	if { $cfgtype == "basic1" } {
		return 0
	}
}

#
# If we are using a master, use no elections 20% of the time.
#
proc get_noelect { usemaster } {
	if { $usemaster } {
		set noelect { 0 0 1 0 0 }
		set len [expr [llength $noelect] - 1]
		set i [berkdb random_int 0 $len]
		return [lindex $noelect $i]
	} else {
		return 0
	}
}

#
# If we are using no elections mode and we are going to kill the initial
# master, select a different site to start up as master after the initial
# master is killed.
#
proc get_secondary_master { noelect master_site kill nsites } {
	if { $noelect == 0 || $kill != $master_site} {
		return 0
	}
	set master2_site [berkdb random_int 1 $nsites]
	while { $master2_site == $master_site } {
		set master2_site [berkdb random_int 1 $nsites]		
	}
	return $master2_site
}

#
# Determine if we are using view/partial site.  A site cannot
# be a view if it is the intended master or secondary master.
#
# Return 0 if not using a view.  Return Site# if using a full view.
# Return -Site# if an empty view.
#
proc get_view { cfgtype restoredir master_site second_master nsites kill_self} {
	if { $cfgtype == "restore" } {
		set viewsite 0
		for { set i 1 } { $i <= $nsites } { incr i } {
			set cid [open $restoredir/DB_REPTEST_ARGS.$i r]
			# !!!
			# We currently assume the args file is 1 line.
			#
			gets $cid arglist
			close $cid
#			puts "Read in: $arglist"
			set view [lsearch $arglist "-V"]
			if { $view >= 0 } {
				set viewsite $i
				set vtype [lindex $arglist [expr $view + 1]]
				if { $vtype == 0 } {
					set viewsite [expr -$viewsite]
				}
			}
		}
		return $viewsite
	}
	if { $cfgtype == "basic0" } {
		return 0
	}
	if { $cfgtype == "basic1" } {
		return 3
	}
	if { $cfgtype == "random" } {
		if { $nsites == 2 } {
			return 0
		}
		#
		# Use views 25% of the time.  Of those, 50% will be
		# an empty view if the configuration is otherwise
		# compatible with an empty view.
		#
		set useview [berkdb random_int 0 3]
		if { $useview != 1 } {
			return 0
		}
		set viewsite [berkdb random_int 1 $nsites]
		while { $viewsite == $master_site || \
		    $viewsite == $second_master} {
			set viewsite [berkdb random_int 1 $nsites]
		}
		set empty [berkdb random_int 0 1]
		#
		# If a site is supposed to kill itself, it can't be an empty
		# view because an empty view cannot execute the mechanism to
		# kill itself in the access method thread.  In this case,
		# just leave the view as a full view.  Note that an empty
		# view can remove itself or other sites from the repgroup
		# because this is done in the event thread.
		#
		if { $empty == 1 && $viewsite != $kill_self } {
			set viewsite [expr -$viewsite]
		}
		return $viewsite
	}
}

#
# Return a site number for autotakeover or 0 for no autotakeover.
#
proc get_autotakeover { kill remove view viewsite nsites } {
	set autotakeover 0
	#
	# Do not combine autotakeover with a kill or remove test because that
	# would be too much disruption during a possibly short test run.
	#
	if { [llength $kill] == 0 && $remove == 0 } {
		set at { 0 1 0 1 1 0 0 1 0 1 }
		set len [expr [llength $at] - 1]
		set i [berkdb random_int 0 $len]
		if { [lindex $at $i] == 1 } {
			set autotakeover [berkdb random_int 1 $nsites]
			#
			# An empty view site cannot do autotakeover because it
			# does not truly run access method threads which are
			# used to determine when the autotakeover should occur.
			#
			while { $autotakeover == $viewsite && $view < 0 } {
				set autotakeover [berkdb random_int 1 $nsites]
			}
		}
	}
	return $autotakeover
}

#
# This is the number of worker threads performing the workload.
# This is not the number of message processing threads.
#
# Scale back the number of worker threads if leases are in use.
# The timing with leases can be fairly sensitive and since all sites
# run on the local machine, too many workers on every site can
# overwhelm the system, causing lost messages and delays that make
# the tests fail.  Rather than try to tweak timeouts, just reduce
# the workloads a bit.
#
proc get_workers { cfgtype lease } {
	if { $cfgtype == "random" } {
		if { $lease } {
			return [berkdb random_int 2 4]
		} else {
			return [berkdb random_int 2 8]
		}
	}
	if { $cfgtype == "basic0" || $cfgtype == "basic1" } {
		return 5
	}
}

proc get_dbtype { cfgtype } {
	if { $cfgtype == "random" } {
		#
		# 50% btree, 25% queue 12.5% hash 12.5% recno
		# We favor queue only because there is special handling
		# for queue in internal init.
		#
#		set methods {btree btree btree btree queue queue hash recno}
		set methods {btree btree btree btree hash recno}
		set len [expr [llength $methods] - 1]
		set i [berkdb random_int 0 $len]
		return [lindex $methods $i]
	}
	if { $cfgtype == "basic0" || $cfgtype == "basic1" } {
		return btree
	}
}

proc get_runtime { cfgtype } {
	global os_tbase

	if { $cfgtype == "random" } {
		return [expr [berkdb random_int 100 500] * $os_tbase]
	}
	if { $cfgtype == "basic0" } {
		return [expr 100 * $os_tbase]
	}
	if { $cfgtype == "basic1" } {
		return [expr 150 * $os_tbase]
	}
}

proc get_rport { portlist i num_sites known_master cfgtype} {
	global rporttype

	if { $cfgtype == "random" && $rporttype == "NULL" } {
		set types {backcirc forwcirc full onesite}
		set len [expr [llength $types] - 1]
		set rindex [berkdb random_int 0 $len]
		set rporttype [lindex $types $rindex]
	}
	if { $cfgtype == "basic0" } {
		set rporttype onesite
	}
	if { $cfgtype == "basic1" } {
		set rporttype full
	}
	#
	# This produces a circular knowledge ring.  Either forward
	# or backward.  In the forwcirc, ENV1 knows (via -r) about
	# ENV2, ENV2 knows about ENV3, ..., ENVX knows about ENV1.
	#
	if { $rporttype == "forwcirc" } {
		if { $i != $num_sites } {
			return [list [lindex $portlist $i]]
		} else {
			return [list [lindex $portlist 0]]
		}
	}
	if { $rporttype == "backcirc" } {
		if { $i != 1 } {
			return [list [lindex $portlist [expr $i - 2]]]
		} else {
			return [list [lindex $portlist [expr $num_sites - 1]]]
		}
	}
	#
	# This produces a configuration where site N does not know
	# about any other site and every other site knows about site N.
	# Site N must either be the master or group creator.
	# NOTE: Help_site_i subtracts one because site numbers
	# are 1-based and list indices are 0-based.
	#
	if { $rporttype == "onesite" } {
		set helper_site [expr $known_master - 1]
		if { $i == $known_master } {
			return {}
		}
		return [lindex $portlist $helper_site]
	}
	#
	# This produces a fully connected configuration
	#
	if { $rporttype == "full" } {
		set rlist {}
		for { set site 1 } { $site <= $num_sites } { incr site } {
			if { $site != $i } {
				lappend rlist \
				    [lindex $portlist [expr $site - 1]]
			}
		}
		return $rlist
	}
}

#
# We need to have an ack timeout > lease timeout. Otherwise txns can get 
# committed without waiting long enough for leases to get granted.  We
# return a list {acktimeout# leasetimeout#}, with leasetimeout#=0 if leases
# are not in use.
#
proc get_ack_lease_timeouts { useleases } {
	global os_tbase

	if { $useleases } {
		return [list [expr 20000000 * $os_tbase] \
		    [expr 10000000 * $os_tbase]]
	} else {
		return [list [expr 5000000 * $os_tbase]  0]
	}
}

#
# Use datadir half the time.  Then pick how many and which datadir
# the database should reside in.  Use a metadata dir 25% of the time.
#
proc get_datadirs { cfgtype usearr dirarr } {
	upvar $usearr use
	upvar $dirarr dir

	set use(datadir) 0
	set use(createdir) 0
	set use(metadir) 0
	if { $cfgtype == "random" } {
		set meta { 0 0 0 1 }
		#
		# Randomly pick if we use datadirs, and if so, how many, up to 4.
		# Although we may create several datadirs, we only choose one
		# of them in which to create the database.
		#
		set data { 0 0 0 0 1 2 3 4 }
		set mlen [expr [llength $meta] - 1]
		set dlen [expr [llength $data] - 1]
		set im [berkdb random_int 0 $mlen]
		set id [berkdb random_int 0 $dlen]
		set use(datadir) [lindex $data $id]
		set use(metadir) [lindex $meta $im]
		#
		# If we're using datadirs, then randomly pick the creation dir.
		#
		if { $use(datadir) != 0 } {
			set use(createdir) [berkdb random_int 1 $use(datadir)]
		}
	} elseif { $cfgtype == "restore" } {
		set cid [open $dir(restore)/DB_CONFIG.1 r]
		set cfg [read $cid]
		# Look for metadata_dir, add_data_dir and set_create_dir.
		set use(metadir) [regexp -all {(set_metadata_dir)} $cfg]
		set use(datadir) [regexp -all {(add_data_dir)} $cfg]
		if { $use(datadir) } {
			set c [regexp {(set_create_dir )(DATA.[0-9])} $cfg m cr]
			#
			# We need to extract the directory number from the
			# createdir directory name.  I.e., DATA.2 needs '2'.
			#
			regexp {(DATA.)([0-9])} $m match d use(createdir)
		}
		close $cid
	}
	return 0
}

#
# Get the original baseport for a configuration to be restored by using
# the local site port number for its first site because every configuration
# will have a first site.
#
proc get_orig_baseport { cfgtype { restoredir NULL } } {
	if { $cfgtype != "restore" } {
		return 0
	} else {
		set cid [open $restoredir/DB_CONFIG.1 r]
		set cfg [read $cid]
		# Look for a number between "127.0.0.1" and "db_local_site on".
		# The spaces after 127.0.0.1 and before db_local_site are
		# significant in the pattern match.  Also accept localhost as
		# input so that old configs can be run.
		regexp {(127.0.0.1 |localhost )([0-9]+)( db_local_site on)} \
		    $cfg match p1 pnum
		close $cid
		return [expr $pnum - 1]
	}
}

#
# Convert DB_CONFIG file port numbers following "127.0.0.1 " to use a 
# different baseport.  regsub -all substitutes all occurrences of pattern,
# which is "127.0.0.1 " and a number.  The result of regsub contains a tcl
# expression with the number (\2, the second part of the pattern), operators
# and variable names, e.g.:
#     -K [expr 30104 - $baseport(orig) + $baseport(curr)]
# and then subst evalutes the tcl expression.  Also accept localhost as
# input so that old configs can be run.
#
# Writes a converted copy of orig_file to new_file.
#
proc convert_config_ports { orig_file new_file basept } {
	upvar $basept baseport

	set cid [open $orig_file r]
	set cfg [read $cid]
	regsub -all {(127.0.0.1 |localhost )([0-9]+)} $cfg \
	    {127.0.0.1 [expr \2 - $baseport(orig) + $baseport(curr)]} cfg
	set cfg [subst $cfg]
	close $cid
	set cid [open $new_file a]
	puts -nonewline $cid $cfg
	close $cid
	return 0
}

proc parse_runtime { progargs } {
	set i [lsearch $progargs "-T"]
	set val [lindex $progargs [expr $i + 1]]
	return $val
}

proc print_summary { } {
	source ./include.tcl

	set ret [catch {glob $testdir/summary.*} result]
	if { $ret == 0 } {
		set sumfiles $result
	} else {
		puts "Could not get summary list: $result"
		return 1
	}
	foreach f $sumfiles {
		puts "====   $f   ===="
		set ret [catch {open $f} fd]
		if { $ret != 0 } {
			puts "Error opening $f: $fd"
			continue
		}
		while { [gets $fd line] >= 0 } {
			puts "$line"
		}
		close $fd
	}
	return 0
}

proc search_output { stopstr } {
	source ./include.tcl

	set ret [catch {glob $testdir/E*/OUTPUT} result]
	if { $ret == 0 } {
		set outfiles $result
	} else {
		puts "Could not find any OUTPUT files: $result"
		return 0
	}
	set found 0
	foreach f $outfiles {
		set ret [catch {exec grep $stopstr $f > /dev/null} result]
		if { $ret == 0 } {
			puts "$f: Match found: $stopstr"
			set found 1
		}
	}
	return $found
}
