# See the file LICENSE for redistribution information.
#
# Copyright (c) 2005, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	fop008
# TEST	Test file system operations on named in-memory databases.
# TEST	Combine two ops in one transaction.
proc fop008 { method args } {
	eval {fop006 $method 1 0} $args
}



