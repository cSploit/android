# See the file LICENSE for redistribution information.
#
# Copyright (c) 2011, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	fop010
# TEST	Test file system operations in child transactions. 
# TEST	Two ops, each in its own child txn.
proc fop010 { method args } {

	# Run for btree only to cut down on redundant testing. 
	if { [is_btree $method] == 0 } {
		puts "Skipping fop010 for method $method"
		return
	}

	eval {fop006 $method 0 1} $args
}



