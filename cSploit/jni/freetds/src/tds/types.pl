#!/usr/bin/perl

use strict;

sub readLine()
{
	my $line;
	while ($line = <>) {
		chomp $line;
		# remove comments
		$line =~ s/#.*//;
		# removed unused spaces at the end
		$line =~ s/\s+$//;
		# if not empty return it
		return $line if $line;
	}
	return $line;
}

sub fixField($)
{
	shift;
	return $_ if substr($_,0,1) ne '"';
	$_ = substr($_, 1, length($_) - 2);
	$_ =~ s/""/"/g;
	return $_;
}

sub splitRow($)
{
	my $row = "$_[0];";
	return map { fixField($_) } ($row =~ /\G("(?:[^"]|"")*"|[^";]*);/sg);
}

# read header
my $hdr = lc(readLine());
$hdr =~ s/ /_/g;
my @fields = splitRow($hdr);

# read all files
my %types;
my $line;
while ($line = readLine()) {
	my %type;
	@type{@fields} = splitRow($line);
	next if !$type{'name'};
	$types{$type{'name'}} = \%type;
}

print qq|/*
 * This file produced from $0
 */

|;

sub unique($)
{
	return keys %{{ map { $_ => 1 } @_ }};
}

sub switchValues()
{
	my ($indent, $column, @list) = @_;
	foreach my $value (sort { $a <=> $b } &unique(map { $_->{$column} } @list)) {
		print $indent.qq|case $_:\n| for (sort map { $_->{'name'} } grep { $_->{$column} eq $value } @list);
		print $indent.qq|	return $value;\n|;
	}
}

# generate tds_get_size_by_type function
print q|/**
 * Return the number of bytes needed by specified type.
 */
int
tds_get_size_by_type(int servertype)
{
	switch (servertype) {
|;
my @list = grep { $_->{'size'} != -1 } values %types;
&switchValues("\t", 'size', @list);
print q|	default:
		return -1;
	}
}

|;

# generate tds_get_varint_size
print q|/**
 * tds_get_varint_size() returns the size of a variable length integer
 * returned in a TDS 7.0 result string
 */
int
tds_get_varint_size(TDSCONNECTION * conn, int datatype)
{
	switch (datatype) {
|;
@list = grep { $_->{'varint'} != 1 && $_->{'varint'} ne '??' && uc($_->{'vendor'}) ne 'MS' && uc($_->{'vendor'}) ne 'SYB' } values %types;
&switchValues("\t", 'varint', @list);
print q|	}

	if (IS_TDS7_PLUS(conn)) {
		switch (datatype) {
|;
@list = grep { $_->{'varint'} != 1 && $_->{'varint'} ne '??' && uc($_->{'vendor'}) eq 'MS' } values %types;
&switchValues("\t\t", 'varint', @list);
print q|		}
	} else if (IS_TDS50(conn)) {
		switch (datatype) {
|;
@list = grep { $_->{'varint'} != 1 && $_->{'varint'} ne '??' && uc($_->{'vendor'}) eq 'SYB' } values %types;
&switchValues("\t\t", 'varint', @list);
print q|		}
	}
	return 1;
}

|;

#generate

print q|/**
 * Return type suitable for conversions (convert all nullable types to fixed type)
 * @param srctype type to convert
 * @param colsize size of type
 * @result type for conversion
 */
int
tds_get_conversion_type(int srctype, int colsize)
{
	switch (srctype) {
|;
# extract all types that have nullable representation
# exclude SYB5INT8 cause it collide with SYBINT8
@list = grep { $_->{'nullable_type'} ne "0" && $_->{name} ne 'SYB5INT8' } values %types;
foreach my $type (@list)
{
	die("$type->{name} should be not nullable") if $type->{nullable};
	die("$type->{name} has invalid nullable type") if !exists($types{$type->{nullable_type}});
}
foreach my $type (sort &unique(map { $_->{nullable_type} } @list)) {
	my @list2 = grep { $_->{nullable_type} eq $type } @list;
	print qq|	case $type:\n|;
	if ($#list2 == 0) {
		print qq|		return $list2[0]->{name};\n|;
	} else {
		print qq|		switch (colsize) {\n|;
		foreach my $item (sort { $b->{size} <=> $a->{size} } @list2) {
			print qq|		case $item->{size}:
			return $item->{name};\n|;
		}
		print qq|		}
		break;\n|;
	}
}
print q|	case SYB5INT8:
		return SYBINT8;
	}
	return srctype;
}

|;
