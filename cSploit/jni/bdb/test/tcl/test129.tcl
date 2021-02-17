# See the file LICENSE for redistribution information.
#
# Copyright (c) 2010, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	test129
# TEST	Test database bulk update for duplicate database, with different
# TEST  configurations.
# TEST
# TEST	This is essentially test127 with the following configurations:
# TEST       * sub database.
# TEST       * bulk buffer pre-sort.

proc test129 {method { nentries 10000 } { ndups 5} args } {
	source ./include.tcl

	if  { [is_partition_callback $args] == 1 } {
		set nodump 1
	} else {
		set nodump 0
	}

	# Test using sub database.
	eval {test127 $method $nentries $ndups "129" 1 0} $args
	eval {verify_dir $testdir "" 1 0 $nodump}
	eval {salvage_dir $testdir "" 1}

	# Test with -sort_multiple.
	eval {test127 $method $nentries $ndups "129" 0 1} $args
	eval {verify_dir $testdir "" 1 0 $nodump}
	eval {salvage_dir $testdir "" 1}

	# Test using sub database, and with -sort_multiple.
	eval {test127 $method $nentries $ndups "129" 1 1} $args

}


