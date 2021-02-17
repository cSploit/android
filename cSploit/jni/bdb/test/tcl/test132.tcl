# See the file LICENSE for redistribution information.
#
# Copyright (c) 2011, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	test132
# TEST  Test foreign database operations on sub databases and 
# TEST  in-memory databases.

proc test132 {method {nentries 1000} {ndups 5} args } {
	source ./include.tcl

	if  { [is_partition_callback $args] == 1 } {
		set nodump 1
	} else {
		set nodump 0
	}

	# Test using on-disk sub databases.
	eval {test131 $method $nentries "132" $ndups 1 0} $args
	eval {verify_dir $testdir "" 1 0 $nodump}
	eval {salvage_dir $testdir "" 1}

	# Test using in-memory databases.
	eval {test131 $method $nentries "132" $ndups 0 1} $args

}

