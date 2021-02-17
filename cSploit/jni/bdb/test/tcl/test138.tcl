# See the file LICENSE for redistribution information.
#
# Copyright (c) 2011, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	test138
# TEST  Test Automatic Resource Management. [#16188][#20281]
# TEST  Here, we test the following cases:
# TEST    Non-encrypt for cds
# TEST    Non-encrypt for tds
# TEST    Encrypt for ds
# TEST    Encrypt for cds
# TEST    Encrypt for tds

proc test138 { method {nentries 1000} {start 0} {skip 0} args } {
	source ./include.tcl

	eval {test137 $method $nentries $start $skip 0 "138" "cds"} $args
	eval {test137 $method $nentries $start $skip 0 "138" "tds"} $args
	eval {test137 $method $nentries $start $skip 1 "138" "ds"} $args
	eval {test137 $method $nentries $start $skip 1 "138" "cds"} $args
	eval {test137 $method $nentries $start $skip 1 "138" "tds"} $args
}
