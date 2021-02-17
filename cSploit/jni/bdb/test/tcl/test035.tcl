# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	test035
# TEST	Test033 with off-page non-duplicates and duplicates
# TEST	DB_GET_BOTH functionality with off-page non-duplicates
# TEST	and duplicates.
proc test035 { method {nentries 10000} args} {
	set pgindex [lsearch -exact $args "-pagesize"]
	if { $pgindex != -1 } {
		puts "Test035: Skipping for specific pagesizes"
		return
	}
	# Test with off-page duplicates
	eval {test033 $method $nentries 20 "035" 0 -pagesize 512} $args
	# Test with multiple pages of off-page duplicates
	eval {test033 $method [expr $nentries / 10] 100 "035" 0 -pagesize 512} \
	    $args
	# Test with overflow duplicates
	eval {test033 $method [expr $nentries / 100] 20 "035" 1 -pagesize 512} \
	    $args
	# Test with off-page non-duplicates
	eval {test033 $method $nentries 1 "035" 0 -pagesize 512} $args
	# Test with overflow non-duplicates
	eval {test033 $method [expr $nentries / 100] 1 "035" 1 -pagesize 512}  \
	    $args
}
