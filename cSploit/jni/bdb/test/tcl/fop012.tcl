# See the file LICENSE for redistribution information.
#
# Copyright (c) 2011, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	fop012
# TEST	Test file system operations in child transactions. 
# TEST	Two ops, each in its own child txn, with in-memory dbs.
proc fop012 { method args } {

	# Run for btree only to cut down on redundant testing. 
	if { [is_btree $method] == 0 } {
		puts "Skipping fop012 for method $method"
		return
	}

	eval {fop006 $method 1 1} $args
}



