# See the file LICENSE for redistribution information.
#
# Copyright (c) 2010, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# TEST	rep093
# TEST	Egen changes during election.
#
proc rep093 { method { niter 20 } { tnum "093" } args } {
	source ./include.tcl

	# Skip for all methods except btree.
	if { $checking_valid_methods } {
		set test_methods { btree }
		return $test_methods
	}
	if { [is_btree $method] == 0 } {
		puts "Rep$tnum: Skipping for method $method."
		return
	}

	rep093_sub $method $niter $tnum yes $args
	rep093_sub $method $niter $tnum no $args
}

# Start an election at site A, at a time when site B already has a higher egen.
# When site B sees the obsolete VOTE1, it responds with an ALIVE, and that causes
# site A recognize an egen update (producing HOLDELECTION, causing us to start
# another election process, and to abandon the first one).
#
# When $lost is true, we discard site A's initial VOTE1 message, and manually
# start an election at site B as well.  In this case it's the VOTE1 message that
# causes site A to realize it needs an egen update.  The result is similar,
# though the code path is completely different.
# 
proc rep093_sub { method niter tnum lost largs } {
	global rep_verbose
	global testdir
	global verbose_type
	global repfiles_in_memory
	global queuedbs
	global elect_serial

	if { $lost } {
		set msg " with lost VOTE1 message"
	} else {
		set msg ""
	}
	puts "Rep$tnum: Egen change during election$msg."

	set verbargs ""
	if { $rep_verbose == 1 } {
		set verbargs " -verbose {$verbose_type on} "
	}

	set repmemargs ""
	if { $repfiles_in_memory } {
		set repmemargs "-rep_inmem_files "
	}

	env_cleanup $testdir

	replsetup [set qdir $testdir/MSGQUEUEDIR]

	set dira $testdir/SITEA
	set dirb $testdir/SITEB

	file mkdir $dira
	file mkdir $dirb

	puts "\tRep$tnum.a: Create a small group."
	repladd 1
	set envcmda "berkdb_env_noerr -create -txn -errpfx SITEA \
	    $repmemargs -event \
	    $verbargs -home $dira -rep_transport \[list 1 replsend\]"

	# Site A will be initial master, just so as to create some initial
	# data.
	# 
	set enva [eval $envcmda -rep_master]
	set masterenv $enva

	repladd 2
	set envcmdb "berkdb_env_noerr -create -txn -errpfx SITEB \
	    $repmemargs -event \
	    $verbargs -home $dirb -rep_transport \[list 2 replsend\]"
	set envb [eval $envcmdb -rep_client]

	set envlist "{$enva 1} {$envb 2}"
	process_msgs $envlist

	set start 0
	eval rep_test $method $masterenv NULL $niter $start $start 0 $largs
	incr start $niter

	# Level the playing field, by making site A a client.
	puts "\tRep$tnum.b: Make site A a client."
	$enva rep_start -client
	process_msgs $envlist

	# Arrange for site B to have a higher egen than site A.  We do this by
	# running a couple of quick election attempts from B in which A does not
	# participate.
	# 
	puts "\tRep$tnum.c: Adjust site B's egen."
	set pri 100
	set timeout 10000
	set nsites 2
	set nvotes 2
	error_check_bad solitaire1 [catch {$envb rep_elect \
		$nsites $nvotes $pri $timeout} result] 0
	error_check_good solitaire1a [is_substr $result DB_REP_UNAVAIL] 1
	error_check_bad solitaire2 [catch {$envb rep_elect \
		$nsites $nvotes $pri $timeout} result] 0
	error_check_good solitaire2a [is_substr $result DB_REP_UNAVAIL] 1

	set egena [stat_field $enva rep_stat "Election generation number"]
	set egenb [stat_field $envb rep_stat "Election generation number"]
	error_check_good starting_egen [expr $egenb > $egena] 1
	replclear 1

	# Start an election at site A, using a timeout longer than we should
	# ever need.
	# 
	puts "\tRep$tnum.d: Start an election at site A."
	set envid 1
	set timeout [expr 60 * 1000000]
	set elect_serial 1
	set pfx "A.1"
	start_election $pfx $qdir $dira $envid $nsites $nvotes $pri $timeout
	set elect_pipe($envid) $elect_serial

	set wait_limit 20
	if { $lost } {
		# Wait until the child process has gotten as far as sending its
		# vote1 message to site B (eid 2).  We want to "lose" that
		# message, but it won't do any good to replclear before the
		# message is actually in the message queue.
		#
		set envid 2
		set voted false
		for { set count 0 } { $count < $wait_limit } { incr count } {
			if {[rep093_find_vote1 $envid]} {
				set voted true
				break;
			}
			tclsleep 1
		}
		error_check_good voted $voted true
		replclear $envid
		
		# Start an election at site B.  We expect site A to react to
		# this by indicating that we should start another rep_elect()
		# call immediately.  We'll check this later, when we examine the
		# $got_egenchg flag.
		#
		puts "\tRep$tnum.e: Start election at site B."
		incr elect_serial
		set pfx "B.$elect_serial"
		start_election $pfx $qdir $dirb $envid $nsites $nvotes $pri $timeout
		set elect_pipe($envid) $elect_serial
	}

	set got_egenchg false
	set got_master false
	set done false
	for { set count 0 } { $count < $wait_limit && !$done} { incr count } {
		foreach pair $envlist {
			set env [lindex $pair 0]
			set envid [lindex $pair 1]
			if { [info exists elect_pipe($envid)] } {
				check_election $elect_pipe($envid) \
				    unavail child_elected
			} else {
				set child_elected false
			}
			set parent_elected [is_elected $env]
			if { ( $child_elected || $parent_elected ) && \
				 !$got_master } {
				set got_master true
				puts "\tRep$tnum.f: Env [$env get_errpfx]\
				    won the election."
				$env rep_start -master
				if { $env eq $enva } {
					set client $envb
				} else {
					set client $enva
				}
			}
			replprocessqueue $env $envid 0 he
			if { $he } {
				# In the "lost msg case" the only HOLDELECTION
				# indication we should be getting is at site A
				# (EID 1).
				#
				if { $lost } {
					error_check_good siteA $envid 1
				}
				if { $env eq $enva } {
					set got_egenchg true
				}
				incr elect_serial
				set envpfx [$env get_errpfx]
				set pfx "$envpfx.$elect_serial"
				puts "\tRep$tnum.g: Starting another\
				    election $pfx at $envpfx."
				set dir [$env get_home]
				start_election $pfx $qdir $dir $envid \
				    $nsites $nvotes $pri $timeout
				set elect_pipe($envid) $elect_serial
			}
		}
		if { $got_master && \
		    [stat_field $client rep_stat "Startup complete"] } {
			puts "\tRep$tnum.h: Env [$client get_errpfx]\
			    has STARTUPDONE."
			set done true
		} else {
			tclsleep 1
		}
	}
	error_check_good done $done true
	error_check_good got_egenchg $got_egenchg true
	cleanup_elections

	$enva close
	$envb close
	replclose $qdir
}


proc rep093_find_vote1 { envid } {
	global queuedbs

	set dbc [$queuedbs($envid) cursor]

	set result no
	for { set records [$dbc get -first] } \
	    { [llength $records] > 0 } \
	    { set records [$dbc get -next] } {
		    set dbt_pair [lindex $records 0]
		    set recno [lindex $dbt_pair 0]
		    set msg [lindex $dbt_pair 1]
		    set ctrl [lindex $msg 0]
		    if {[berkdb msgtype $ctrl] eq "vote1"} {
			    set result yes
			    break
		    }
	    }
	
	$dbc close
	return $result
}
