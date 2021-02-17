# See the file LICENSE for redistribution information.
#
# Copyright (c) 2010, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	test128
# TEST	Test database bulk update for non-duplicate databases, with different
# TEST  configurations.
# TEST
# TEST	This is essentially test126 with the following configurations:
# TEST      * sub database.
# TEST      * secondary database.
# TEST      * bulk buffer pre-sort.

proc test128 {method { nentries 10000 } {callback 1} args } {
	source ./include.tcl

	if  { [is_partition_callback $args] == 1 } {
		set nodump 1
	} else {
		set nodump 0
	}	
	# Test using sub database
	eval {test126 $method $nentries "128" $callback 1 0 0} $args
	eval {verify_dir $testdir "" 1 0 $nodump}
	eval {salvage_dir $testdir "" 1}

	# Test using secondary database
	eval {test126 $method $nentries "128" $callback 0 1 0} $args
	eval {verify_dir $testdir "" 1 0 $nodump}
	eval {salvage_dir $testdir "" 1}

	# Test with -sort_multiple
	eval {test126 $method $nentries "128" $callback 0 0 1} $args
	eval {verify_dir $testdir "" 1 0 $nodump}
	eval {salvage_dir $testdir "" 1}

	# Test using both sub database and secondary database,
	# and with -sort_multiple
	eval {test126 $method $nentries "128" $callback 1 1 1} $args

}


