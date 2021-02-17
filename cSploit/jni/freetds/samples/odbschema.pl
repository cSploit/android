#! /usr/bin/perl -w

# List DDL for an ODBC data source

# Contributed to the public domain by James K. Lowden
# Inspired by the much more sophisticated dbschema by Michael Peppler, et al.
# $Id: odbschema.pl,v 1.1 2008-12-08 22:44:26 jklowden Exp $

use Getopt::Std;
use File::Basename;
use DBI;
use DBI::Const::GetInfoType;

getopts( 'S:v:X', \%opts );

$opts{v} = 0 unless defined($opts{v});

# print environment information, to help know what can be connected to
if( $opts{v} == 3 ) {
	print "Available drivers:\n", join($/, DBI->available_drivers, $/);
	print "Available ODBC Data Sources:\n", join($/, DBI->data_sources('ODBC', undef), $/);
	print "Available ODBC Constants:\n", join($/, keys %GetInfoType), $/;
}

$server = $opts{S} 
	or die basename($0) . qq(: need -S server (DSN) name\n);

$dsn = qq(dbi:ODBC:$server) 
	or die qq(could not connect to ODBC Data Source "$server"\n);

my ($username, $auth);

$dbh = DBI->connect($dsn, $username, $auth, undef);

# get tables
$sth = $dbh->table_info('%', '', '');
while ( @row = $sth->fetchrow_array ) {
	if( $opts{v} == 2 ) {
		print '[', join('], [', @row), ']', $/;
	}
	next unless $row[3] eq 'TABLE';
	push @tables, $row[2];
	$catalogs{$row[0]}++;
}

die if 1 < keys %catalogs;

($catalog) = keys %catalogs;
$schema = '';
$fGetKeys = 1;

# set LongReadLen to something more than 80 if extracting data. 
if( $opts{X} ) {
	printf STDERR qq("LongReadLen" is %d\n), $dbh->{LongReadLen};
	$dbh->{LongReadLen} = 640;
	printf STDERR qq("LongReadLen" is %d\n), $dbh->{LongReadLen};
}

foreach my $table (@tables) {
	if( $fGetKeys ) {
		my $ksth = $dbh->primary_key_info( $catalog, $schema, $table ); 
		if( !defined($ksth) ) {
			warn "warning: primary_key_info failed for $catalog.$schema.$table, "
				 . "no primary keys will be extracted for this schema.\n";
			$fGetKeys = 0;
			goto GetColumns;
		}

		# get primary key columns
		my $c = 0;
		my @key = ();
		while ( $rrow = $ksth->fetchrow_hashref ) {
			my $pkname = defined($rrow->{PK_NAME})? $rrow->{PK_NAME} : '';
			$rrow->{KEY_SEQ} == ++$c 
				or die "KEY_SEQ $rrow->{KEY_SEQ} != $c\n";
			push @key, $rrow->{COLUMN_NAME};
		}
	}

	#
	GetColumns:
	#
	my $column = '%';
	$sth = $dbh->column_info( $catalog, $schema, $table, $column ) 
		or die "error: column_info failed\n";

	printf "create table [%s]\n", $table;

	# get columns
	my $comma = '(';
	while ( $rrow = $sth->fetchrow_hashref ) {
		my $nullable = $rrow->{NULLABLE}? '    NULL' : 'not NULL';
		my $type = $rrow->{TYPE_NAME};
		$type .= "($rrow->{CHAR_OCTET_LENGTH})" if $rrow->{CHAR_OCTET_LENGTH};
		printf "\t$comma [%-30s] %-15s %s -- %3d\n", 
			$rrow->{COLUMN_NAME}, $type, $nullable, $rrow->{ORDINAL_POSITION};
		$comma = ',';
	}
	if( $key ) {
		printf "\t, PRIMARY KEY (%s)\n", join ', ', @key;
	}
	printf "\t);\n\n";

	if( $opts{X} ) {
		extract_data($table);
	}
}

sub extract_data
{ my $table = $_[0];

	my $filename = $table;
	$filename =~ s/[<>\ ]//g;
	open DAT, ">$filename.dat" 
		or die qq(error: could not open "$filename.dat" (table: $table)\n);

	my $statement = "select * from [$table]";

	my $sth = $dbh->prepare($statement) 
		or die qq(error: could not prepare "$statement"\n);

	$sth->execute() or die;

	local $^W;
	$^W = 0;
	my @row;
	while ( @row = $sth->fetchrow_array ) {
			die if grep /\t/, @row;
			print DAT join("\t", @row), $/;
	}
}
	


__END__
