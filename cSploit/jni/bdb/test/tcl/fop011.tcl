# See the file LICENSE for redistribution information.
#
# Copyright (c) 2011, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	fop011
# TEST	Test file system operations in child transactions. 
# TEST	Combine two ops in one child transaction, with in-emory
# TEST	databases.
proc fop011 { method args } {

	# Run for btree only to cut down on redundant testing. 
	if { [is_btree $method] == 0 } {
		puts "Skipping fop011 for method $method"
		return
	}

	eval {fop001 $method 1 1} $args
}



