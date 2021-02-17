#! /usr/bin/perl -w

use strict;
use diagnostics;

use ncpfs;

my ($err,$ctx);
	
print 'NWDSCreateContext() = ';
$ctx = ncpfs::NWDSCreateContext();
if (not defined($ctx)) {
	print "failed\n";
} else {
	my ($nctx);
	print "ok, ctx=$ctx\n";
	print 'NWDSDuplicateContext(ctx) = ';
	$nctx = ncpfs::NWDSDuplicateContext($ctx);
	if (not defined($nctx)) {
		print "failed\n";
	} else {
		print "ok, nctx=$nctx\n";
	}
}
