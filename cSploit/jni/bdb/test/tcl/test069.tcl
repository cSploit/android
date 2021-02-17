# See the file LICENSE for redistribution information.
#
# Copyright (c) 1999, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	test069
# TEST	Test of DB_CURRENT partial puts without duplicates-- test067 w/
# TEST	small ndups to ensure that partial puts to DB_CURRENT work
# TEST	correctly in the absence of duplicate pages.
proc test069 { method {ndups 50} {tnum "069"} args } {
	eval test067 $method $ndups $tnum $args
}
