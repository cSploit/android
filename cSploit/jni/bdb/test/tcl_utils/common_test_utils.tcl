# See the file LICENSE for redistribution information.
#
# Copyright (c) 2011, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# Utility functions for tests in Core and SQL

# Return a list of TCP port numbers that are not currently in use on
# the local system.  Note that this doesn't actually reserve the
# ports, so it's possible that by the time the caller tries to use
# them, another process could have taken one of them.  But for our
# purposes that's unlikely enough that this is still useful: it's
# still better than trying to find hard-coded port numbers that will
# always be available.
#
# Using a starting baseport value that falls in the non-ephemeral port
# range on most platforms.  Can override starting baseport by setting
# environment variable BDBBASEPORT.
#
# Must test explicit 127.0.0.1 host rather than localhost because
# localhost can be configured differently on different platforms or
# machines and that can cause this routine to return ports that are
# actually in use.
#
proc available_ports { n { rangeincr 10 } } {
	global env

	if { [info exists env(BDBBASEPORT)] } {
		set baseport $env(BDBBASEPORT)
	} else {
		set baseport 30100
	}

	# Try sets of contiguous ports ascending from baseport.
	for { set i $baseport } { $i < $baseport + $rangeincr * 100 } \
	    { incr i $rangeincr } {
		set ports {}
		set socks {}
		set numports $n
		set curport $i

		# Try one set of contiguous ports.
		while { [incr numports -1] >= 0 } {
			incr curport
			if [catch { socket -server Unused \
			    -myaddr 127.0.0.1 $curport } sock] {
				# A port is unavailable, try another set.
				break
			}
			lappend socks $sock
			lappend ports $curport
		}
		foreach sock $socks {
			close $sock
		}
		if { $numports == -1 } {
			# We have all the ports we need.
			break
		}
	}
	if { $numports == -1 } {
		return $ports
	} else {
		error "available_ports: could not get ports for $baseport"
	}
}
