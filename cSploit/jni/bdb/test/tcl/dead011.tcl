# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	dead011
# TEST	Test out the minlocks, maxlocks, and minwrites options
# TEST	to the deadlock detector when priorities are used.
proc dead011 { { procs "4 6 10" } \
    {tests "maxlocks maxwrites minlocks minwrites" } { tnum "011" } } {
	source ./include.tcl

	dead005 $procs $tests $tnum 1
}
