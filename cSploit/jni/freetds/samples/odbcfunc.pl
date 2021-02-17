#! /usr/bin/perl -w

# List supported/unsupported functions for an ODBC data source

# Contributed to the public domain by James K. Lowden
# $Id: odbcfunc.pl,v 1.2 2008-12-08 22:47:07 jklowden Exp $

use Getopt::Std;
use File::Basename;
use DBI;
use DBI::Const::GetInfoType;

getopts( 'I:P:S:U:v:', \%opts );

$opts{v} = 0 unless defined($opts{v});

$server = $opts{S} 
	or die basename($0) . qq(: need -S server (DSN) name\n);

$dsn = qq(dbi:ODBC:$server) 
	or die qq(could not connect to ODBC Data Source "$server"\n);

my ($username, $passwd) = ($opts{U}, $opts{P});

$dbh = DBI->connect($dsn, $username, $passwd, undef);

print "Available ODBC Constants:\n", join($/, keys %GetInfoType), $/
	if $opts{v} == 1;

$sql_h_path = "C:/Program Files/Microsoft SQL Server/80/Tools/DevTools/Include";
$sql_h_path = $opts{I} if defined($opts{I});

# 
# Build list of SQL_API constants from include files. 
#
foreach my $file (('sql.h', 'sqlext.h')) {
	$sql_h_file = "$sql_h_path/$file";
	open SQLH, "$sql_h_file" or die "could not open $sql_h_file\n";

	while( <SQLH> ) {
		next unless s/SQL_API_//;
		my ($define, $name, $value) = split;
		if( defined($api{$value}) ) {
			printf STDERR "overwriting %d (%s) with %s in $file\n", 
				$value, $api{$value}, $name, $file;
		}
		$api{$value} = $name;
	}
}

die unless %api;

# Capture array of supported/unsupported by SQL_API index value. 
my @supported = $dbh->func(0, "GetFunctions");

# Map names to supported/unsupported 
for ($api=1; $api < 100; $api++) {
	next if( !$supported[$api] && !$api{$api} ); 
	local $^W;
	$^W = 0;
	printf "%4d:\t%-30s\t%s supported\n", $api, $api{$api}, $supported[$api]? '   ':'not'
		if $opts{v} == 2;
	$functions{$api{$api}} = $supported[$api]? 'supported' : 'unsupported';
}

# Print names, sorted
foreach my $name (sort keys %functions) {
	printf "%-30s\t%s\n", $name, $functions{$name};
}

__END__

=head1 NAME

odbcfunc - List ODBC functions supported (and unsupported) by an ODBC driver. 

=head1 SYNOPSIS

	odbcfunc.pl -S DSN [-U username] [-P passwd] [-v verbosity]

=head1 DESCRIPTION

Prints results from the ODBC function SQLGetFunctions.  

=head1 OPTIONS

=over

=item -I include path for sql.h and sqlext.h 

=item -S DSN is a Data Source Name that uses the driver. 

=item -U is a username, if required. 

=item -P is a password, if required. 

=item -v may have a value of 1 or 2, which prints more details. 

=back
