#!/usr/bin/perl

# $Id: order_coverage.pl,v 1.2 2009-04-30 06:48:50 freddy77 Exp $
# This utility create some sorted version of GCOV results

%processed = ();
@to_process = ();

$sorted = 'index.sort.html';

sub perc($)
{
	my $row = shift;
	my ($perc) = ($row =~ m/<td class="coverPer.*?">([0-9\.]+)/);
	return $perc;
}

sub read_file($)
{
	open(FILE, '<', shift) or die('error opening');
	my @all = <FILE>;
	my $all = "@all";
	undef @all;
	close(FILE);
	return $all;
}
sub order_file($)
{
	my $fn = shift;

	my $all = read_file($fn);

	# mark as processed
	$processed{$fn} = 1;

	# discover other files
	while ($all =~ m{<td class="coverFile"><a href="(.*?index.html)">.*</a></td>}g) {
		push(@to_process, $1) if !exists($processed{$1});
	}

	# extract parts (before, table, after)
	die ('format error') if $all !~ m{(.*<table.*?<td class="tableHead" colspan=3>(?:<a href="$sorted">)?Coverage(?:</a>)?</td>.*?</tr>)(.*?)(\n\s*</table.*)}is;
	my ($before, $table, $after) = ($1, $2, $3);

	# extract rows
	my @rows = ();
	while ($table =~ m{([ \t]*<tr>.*?<td class="coverFile">.*?<td class="coverNum.*?">.*?line.*?</tr>\s*)}sg) {
		push @rows, $1;
	}

	# make sorted index
	my $ofn = $fn;
	$ofn =~ s/index\.html/$sorted/;
	my $output = $before . join('', sort { perc($a) <=> perc($b) } @rows) . $after;
	$output =~ s{(<td class="tableHead">)(Directory\&nbsp;name|Filename)(</td>)}{$1<a href="index.html">$2</a>$3};
	$output =~ s{(<td class="tableHead" colspan=3>)<a href="$sorted">(Coverage)</a>(</td>)}{$1$2$3};
	$output =~ s{(<td class="coverFile"><a href=".*?)index\.html(">.*</a></td>)}{$1$sorted$2}g;
	open(OUT, '>', $ofn) or die('opening output');
	print OUT $output;
	close(OUT);

	# add link to normal index to sorted version
	$output = $all;
	$output =~ s{(<td class="tableHead" colspan=3>)(Coverage)(</td>)}{$1<a href="$sorted">$2</a>$3};
	open(OUT, '>', $fn) or die('opening output');
	print OUT $output;
	close(OUT);
}

# process all files starting from index.html
push @to_process, 'index.html';
while (@to_process) {
	my $fn = pop(@to_process);
	print "processing $fn\n";
	order_file($fn);
}

