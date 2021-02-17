# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	dead010
# TEST
# TEST	Same test as dead003, except the actual youngest and oldest will have
# TEST	higher priorities.  Verify that the oldest/youngest of the lower
# TEST	priority lockers gets killed.  Doesn't apply to 2 procs.
proc dead010 { {procs "4 10"} {tests "ring clump"} {tnum "010"} } {
	source ./include.tcl

	dead003 $procs $tests $tnum 1
}
