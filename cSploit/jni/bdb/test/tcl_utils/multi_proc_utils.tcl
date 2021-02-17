# See the file LICENSE for redistribution information.
#
# Copyright (c) 2011, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# Utility functions for multi process tests in Core and SQL

# The SQL test suite shell (testfixture) lacks some basic Tcl functions
# that are required by do_multi_proc_test and do_sync, like clock.  
# So load the tcl library if it has not already been loaded.
proc load_tcl_library {} {
    global tcl_platform
    set nameexec [info nameofexecutable]
    if { [string match *testfixture* $nameexec] } {
	set loaded [info loaded]
	if { [lsearch $loaded tcl*] == -1 && 
	     [lsearch $loaded libtcl*] == -1 } {
	    set isWindows 0
	    set os $tcl_platform(platform)
	    set tclversion [info tclversion]
	    if { [string  equal -nocase "windows" $os] } {
		# Get the version number and strip the . from it
		for {set x 0} {$x < [string length $tclversion]} {incr x} {
		    set char [string index $tclversion $x]
		    if { [string equal \. $char] } {
		       set tclversion [string replace $tclversion $x $x]
		    }
		}
		load tcl$tclversion[info sharedlibextension] Tcl
	    } else {
		load libtcl$tclversion[info sharedlibextension] Tcl
	    }
	}
    }
}

load_tcl_library

# do_multi_proc_test - Takes a list of scripts and executes them
# as separate processes, and reports any errors or test failures
# to the error log for Core tests, and the error counter for
# SQL tests.  Look at db/test/sql/bdb_multi_proc.test for a
# test example.  Output from the tests are redirected to
# TESTOUTPUT/err_[script number]_[testname].txt
#
# name - Name of the test.
# scripts - A list of scripts that will be writen to separate 
# files then executed by tclsh for Core tests, and testfixture
# for SQL tests.
# args_lists - A list of lists containing arguments to pass
# to the test scripts.
# verbose - Print verbose output to the script error log.
proc do_multi_proc_test { name scripts args_lists {verbose 0} } {
    set working_dir [pwd]
    if [ catch {source ./include.tcl} ] {
	eval cd ..
	if [ catch {source ./include.tcl} ] {
	    eval cd ..
	    source ./include.tcl
	}
    }
    source $test_path/testutils.tcl
    set error_dir [pwd]/TESTOUTPUT

    # May have to create the test directory if in the SQL suite
    if { [file exists $testdir] == 0 } {
	file mkdir $testdir
    }
    # Create the error directory if not already there.
    if { [file exists $error_dir] == 0 } {
	file mkdir $error_dir
    }

    # Write script files as $counter_$name.tcl
    set counter 1
    set fileNames {}
    set errLogs {}
    foreach script $scripts {
	set fileName "${counter}_${name}.tcl"
	lappend fileNames $fileName
	lappend errLogs "err_${counter}_${name}.txt"
	incr counter
	set aFile [open $fileName w]
	puts $aFile $script
	flush $aFile
	close $aFile
    }
    
    # Run scripts
    sentinel_init
    set pidlist {}
    set working_dir [pwd]
    # For core the executable is tclsh, for SQL it is testfixture
    set exec_name [info nameofexecutable]
    foreach fileName $fileNames errLog $errLogs arg_list $args_lists {
	if { $verbose } {
	    puts "Starting script $working_dir/$fileName with arguments $arg_list, and writing to error log $error_dir/$errLog"
	}

	lappend pidlist [exec $exec_name $test_path/wrap.tcl \
			     $working_dir/$fileName \
			     $error_dir/$errLog $arg_list &]
    }

    # Wait for scripts to finish
    watch_procs $pidlist 1 600 0

    # Clean up old script files
    foreach fileName $fileNames {
	catch {file delete -force -- $fileName}
    }

    # Check for errors in the script logs
    foreach errLog $errLogs {
	set fd [open $error_dir/$errLog r]
	# If this is the SQL test suite check for the success
	# message, and if it is not found call fail_test,
	# otherwise we are in the Core test suite so call error
	set procs [info procs fail_test]
	set proc_name [lindex $procs 0]
	if { [string match fail_test $proc_name] } {
	    set success 0
	    while { [gets $fd str] != -1 } {
		if { [string match "0 errors out of * tests" $str] } {
		    set success 1
		    break
		}
	    }
	    if {!$success} {
		fail_test $errLog
	    }
	} else {
	    while { [gets $fd str] != -1 } {
		if { [string match FAIL:* $str] ||
		     [string match Error:* $str] } {
		    close $fd
		    error "FAIL: found message $str"
		}
	    }
	}
	close $fd
    }
    eval cd $working_dir
}

global ::sync_server_results
# do_sync - Synchronizes a set of processes. Works by forcing each process
# to block until it can connect to the servers of each other process,
# and receive a connection on its server from the other processes.
# Returns 0 on successful synchronization, and -1 on failure.
# For an example of how to use this, go to 
# db/test/sql/bdb_multi_proc.test.
#
# myPort - Port that the other processes should connect to this process.
# clientPorts - A list of ports for all other processes to 
# synchronize with.
# timeout - The number of seconds after which the function will abandon
# trying to synchronize with the other processes and will return -1.
# verbose - If set to non-0 prints verbose output.
#
# Note that this procedure is probably not thread safe.  
# It is meant to be used to synchronize processes, not threads with 
# shared memory.  The Thread Tcl library already has functions for 
# synchronizing threads.
proc do_sync { myPort clientPorts timeout {verbose 0} } {  
    package require Thread
    #Get the number of clients
    set numClients [llength $clientPorts]
    unset -nocomplain ::sync_server_results

    # Accept connections to the server until timeout is reached
    set server_thread [thread::create {
	global ::numCon
	global ::numClients
	global ::server_connections
	# Called by the server to keep track of how many connections have
	# occured.
	proc my_connections {sock addr port} {
	    puts $sock "success"
	    close $sock
	    incr ::numCon
	    # Race condition does not matter here since we can set 
	    # server_connections to 0 twice without a problem
	    if { $::numCon >= $::numClients } {
		set ::server_connections 0
	    }
	}
	proc run_server { myPort clients timeout verbose} {
	    set ::numCon 0
	    set ::numClients $clients

	    # Loop until the server connects to all clients or the timeout
	    # is hit.  This is in case one of the client sockets grabed
	    # the server port as its local port.
	    set id [after [expr {int($timeout * 1000)}] \
			set ::server_connections -1]
	    while { [info exists ::server_connections] == 0 } {
		if [catch { socket -server my_connections -myaddr 127.0.0.1 \
				$myPort } server ] {
		    #if {$verbose} {
		    #    puts "Could not create server at $myPort because of: $server. RETRYING"
		    #}
		    catch { close $server }
		} else {
		    vwait ::server_connections
		    after cancel $id
		    close $server
		}
	    }
	    if { $verbose } {
		if { $::server_connections == -1 } {
		    puts "Failure, server at port $myPort reached timeout of $timeout seconds before recieving $::numClients connections."
		} else {
		    puts "Success, server at port $myPort completed connections to $::numClients clients before timeout of $timeout seconds."
		}
	    }
	    catch {close $server}
	    set ::sync_server_results $::server_connections
	}
	thread::wait
    }]
    #start the server thread
    if { $verbose } {
	puts "[timestamp]Starting server at port $myPort."
    }
    thread::send -async $server_thread \
	"run_server $myPort $numClients $timeout $verbose" \
	::sync_server_results

    # Try to connect to each client. If timeout, set an error 
    # return value and quit trying to connect.
    global ::clientconnected
    unset -nocomplain ::clientconnected
    # After the timeout is reached, set ::clientconnected.
    set id [after [expr {int($timeout * 1000)}] \
		set ::clientconnected -1]
    foreach clientPort $clientPorts {
	set returnVal -1
	if { $verbose } {
	    puts "[timestamp] Attempting to contact the server at $clientPort."
	}
	# Loop until the client connects to the server or the timeout
	# is hit.
	while { $returnVal == -1 && [info exists ::clientconnected] == 0 } {
	    update
	    if [ catch { socket 127.0.0.1 $clientPort } s ] {
		#if {$verbose} {
		#    puts "[timestamp] Could not connect to server at $clientPort because of: $s, RETRYING"
		#}
		catch {close $s}
	    } else {
		if { $verbose } {
		    puts "[timestamp] Client connection info: [fconfigure $s -sockname]"
		}
		# Sometimes the client socket will pick the port it is trying
		# to connect to as the port to use on its side, resulting in
		# it connecting to itself
		set portInfo [fconfigure $s -sockname]
		set portOffset [string last " " $portInfo]
		incr portOffset
		set portInfo [string range $portInfo $portOffset end]
		if { $portInfo == $clientPort } {
		    set returnVal -1
		} else {
		    set line "Could not read server"
		    if { ![eof $s] } {
			set line [gets $s]
		    }
		    if { $verbose } {
			puts "[timestamp] Read the following from the server: $line"
		    }
		    if { [string match success* $line] } {
			set returnVal 0
		    } else {
			set returnVal -1
		    }
		}
		catch {close $s}
	    }
	}
	if { $returnVal == -1 } {
	    if { $verbose } {
		puts "[timestamp] Failed to connect to server at port $clientPort before timeout of $timeout"
	    }
	    break
	} 
	if { $verbose } {
	    puts "[timestamp] Succeeded in completing connection to server at $clientPort"
	}
    }
    after cancel $id
    if { $verbose } {
	if { $returnVal == -1 } {
	    puts "[timestamp] Failed to connect ot all servers at ports: $clientPorts"
	} else {
	    puts "[timestamp] Succeeded in connecting to all servers at ports: $clientPorts"
	}
    }

    # wait on the server thread to finish if we have not already
    # timed out
    if {  [eval info exists ::sync_server_results] == 0 } {
	vwait ::sync_server_results    
    }
    thread::release $server_thread
    if { $verbose } {
	if { $::sync_server_results == -1 } {
	    puts "[timestamp] Failed, server at port $myPort failed to connect to all clients by timeout $timeout seconds."
	} else {
	    puts "[timestamp] Succeeded, server at port $myPort suceeded in connecting to al clients."
	}
    }
    if { $::sync_server_results == -1 } {
	set returnVal -1
    }
    after 500
    set returnVal
}

