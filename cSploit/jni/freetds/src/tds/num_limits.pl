#!/usr/bin/perl

#
# $Id: num_limits.pl,v 1.2 2005-08-02 12:09:57 freddy77 Exp $
# Compute limits table to check numeric typs
#

use strict;

# This function convert a number given in textual form ("1000") to a
# packet form (binary packet in groups of selected bit)
sub to_pack($$)
{
	my ($num, $bit) = @_;
	my @digits;

	my ($carry, $n, $i, $pos, $bin);
	my $out = "\x00" x 32;
	$pos = 0;
	$bin = '';
	while($num ne '') {
		@digits = split('', $num);
		$carry = 0;
		for $i (0..$#digits) {
			$n = $carry * 10 + $digits[$i];
			$digits[$i] = int($n / 2);
			$carry = $n & 1;
		}
		$bin = "$carry$bin";
		vec($out, $pos++, 1) = $carry;
		shift @digits if ($digits[0] == 0);
		$num = join('', @digits);
	}
	return reverse unpack($bit == 32 ? 'V' x 8 : 'v' x 16, $out);
}

# Print a tables for given bits
# This function produce two arrays
#  one (limits) store not 0 packet numbers
#  the other (limit_indexes) store indexed in first array
sub print_all()
{
	my ($bit) = @_;
	my @limits = ();
	my @indexes = ();
	my $i;

	$indexes[0] = 0;
	# compute packet numbers and indexes
	# we use 1000... number to reduce number length (lower packet are 
	# zero for big numbers)
	for $i (0..77) {
		my @packet = &to_pack("1".('0'x$i), $bit);
		$indexes[$i] = $#limits + 1;
		while($packet[0] == 0) {
			shift @packet;
		}
		while($packet[0] != 0) {
			push @limits, shift @packet;
		}
		$indexes[$i+1] = $#limits + 1;
	}

	# fix indexes to use signed char numbers instead of int
	my $adjust = $bit == 32 ? 4 : 6;
	printf("#define LIMIT_INDEXES_ADJUST %d\n\n", $adjust);
	for $i (0..78) {
		my $idx = $indexes[$i];
		$idx = $idx - $adjust * $i;
		die ('error compacting indexes') if ($idx < -128 || $idx > 127);
		$indexes[$i] = $idx;
	}

	# print all
	print "static const signed char limit_indexes[79]= {\n";
	for $i (0..78) {
		printf("\t%d,\t/* %2d */\n", $indexes[$i], $i);
	}
	print "};\n\n";

	print "static const TDS_WORD limits[]= {\n";
	my $format = sprintf("\t0x%%0%dxu,\t/* %%3d */\n", $bit / 4);
	for $i (0..$#limits) {
		printf($format, $limits[$i], $i);
		die ('limit if zero ??') if ($limits[$i] == 0);
	}
	print "};\n";
}

&print_all(32);

