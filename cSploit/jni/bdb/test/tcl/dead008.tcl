# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	dead008
# TEST	Run dead001 deadlock test using priorities
proc dead008 { {tnum "008"} } {
	source ./include.tcl
	dead001 "2 4 10" "ring clump" "0" $tnum 1

}
