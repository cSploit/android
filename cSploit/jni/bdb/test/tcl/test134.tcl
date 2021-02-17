# See the file LICENSE for redistribution information.
#
# Copyright (c) 2011, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	test134
# TEST  Test cursor cleanup for sub databases.

proc test134 {method {nentries 1000} args} {
	source ./include.tcl

	eval {test133 $method $nentries "134" 1} $args

}
