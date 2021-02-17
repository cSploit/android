#!/usr/local/bin/perl
# $Id: odbc_rpc.pl,v 1.10 2009-01-16 20:27:57 jklowden Exp $
#
# Contributed by James K. Lowden and is hereby placed in 
# the public domain.  No rights reserved.  
#
# This program demonstrates calling the ODBC "dynamic" functions, using
# placeholders and prepared statements.  
#
# By default, arguments are bound to type SQL_VARCHAR.  If the stored procedure 
# uses other types, they may be specified in the form :TYPE:data, where TYPE is one
# of the DBI symbolic constants.  If your data happen to begin with a colon, 
# prefix the string with ':SQL_VARCHAR:'.  
#
# Example: a datetime parameter:  ':SQL_DATETIME:2005-04-01 16:46:00' 
# (Unfortunately, it appears DBD::ODBC has difficulty with SQL_DATETIME:
# http://www.opensubscriber.com/message/perl-win32-users@listserv.ActiveState.com/108164.html
#	
#
# To find the symbolic constants for DBI, perldoc DBI recommends:
#  	use DBI;         
#  	foreach (@{ $DBI::EXPORT_TAGS{sql_types} }) {
#             printf "%s=%d\n", $_, &{"DBI::$_"};
#  	}
# 

use DBI qw(:sql_types);
use Getopt::Std;
use File::Basename;

# Stolen shamelessly from DBI: testerrhandler.pl

sub err_handler {
   my ($state, $msg) = @_;
   # Strip out all of the driver ID stuff
   $msg =~ s/^(\[[\w\s]*\])+//;
   print "===> state: $state msg: $msg\n";
   return 0;
}

sub setup_error_handler($)
{ my ($dbh) = @_;

	$dbh->{odbc_err_handler} = \&err_handler;
	#dbh->{odbc_async_exec} = 1;
	print "odbc_async_exec is: $dbh->{odbc_async_exec}\n";
}

$program = basename($0);

Getopt::Std::getopts('U:P:D:d:hlv', \%opts);

if( $opts{l} ) {
	foreach (@{ $DBI::EXPORT_TAGS{sql_types} }) {
	      printf "%s=%d\n", $_, &{"DBI::$_"};
	}
	exit 0;
}

open VERBOSE, $opts{v}? '>&STDERR' : '>/dev/null' or die;

my ($dsn, $user, $pass, $database);
$dsn  = $opts{D}? $opts{D} : "dbi:ODBC:JDBC";
$user = $opts{U}? $opts{U} : 'guest';
$pass = $opts{P}? $opts{P} : 'sybase';

die qq(Syntax: \t$program [-D dsn] [-U username] [-P password] procedure [arg1[, argn]]\n) 
	if( $opts{h} || 0 == @ARGV );

# Connect
my $dbh = DBI->connect($dsn, $user, $pass, {RaiseError => 1, PrintError => 1, AutoCommit => 1})
	or die "Unable for connect to $dsn $DBI::errstr";

setup_error_handler($dbh);
$dbh->{odbc_putdata_start} = 2 ** 31;

# Construct an odbc placeholder list like (?, ?, ?)
# for any arguments after $ARGV[0]. 
my $placeholders = '';
if( @ARGV > 1 ) {
	my @placeholders = ('?') x (@ARGV - 1); 
	$placeholders = '(' . join( q(, ),  @placeholders ) . ')';
	printf VERBOSE qq(%d arguments found for procedure "$ARGV[0]"\n), scalar(@placeholders);
}

# Ready the odbc call
my $sql = "{? = call $ARGV[0] $placeholders}";
my $sth = $dbh->prepare( $sql );

# Bind the return code as "inout".
my $rc;
print VERBOSE qq(Binding parameter #1, the return code\n);
$sth->bind_param_inout(1, \$rc, SQL_INTEGER);

# Bind the input parameters (we don't do outputs in this example).
# Placeholder numbers are 1-based; the first "parameter" 
# is the return code, $rc, above.
for( my $i=1; $i < @ARGV; $i++ ) {
    my $type = SQL_VARCHAR;
    my $typename = 'SQL_VARCHAR';
    my $data = $ARGV[$i];
    if( $data =~ /^:([[:upper:]_]+):(.+)/ ) { # parse out the datatype, if any
    	$ARGV[$i] = $2;
    	if( $1 eq 'FILE' ) {
		$typename = 'SQL_LONGVARCHAR';
		open INPUT, $2 or die qq(could not open "$2"\n);
		my @data = <INPUT> or die qq(error reading "$2"\n);
		$data = join '', @data;
		close INPUT;
	} else {
		$typename = $1;
		$data = $2;
	}
        $type = eval($typename);
	warn qq("$typename" will probably cause a "can't rebind parameter" message\n) 
		if $type == SQL_DATETIME;
    }
    printf VERBOSE qq(Binding parameter #%d (%s): "$data"\n), ($i+1), $typename;
    $sth->bind_param( 1 + $i, $data, $type ) or die $sth->errstr;
}

print STDOUT qq(\nExecuting: "$sth->{Statement}" with parameters '), 
	     join(q(', '), @ARGV[1..$#ARGV]), qq('\n);

# Execute the SQL and print the (possibly several) results
$rc = $sth->execute;
print STDOUT qq(execute returned: '$rc'\n) if $rc;

$i = 1;
while ( $sth->{Active} ) { 
	printf "Result #%d:\n", $i++;
        my @names = @{$sth->{NAME}};	# print column names for each result set
	print '[', join("], [", @{$sth->{NAME}}), "]\n" if @names;
	while(@dat = $sth->fetchrow_array) {
		print q('), join(q(', '), @dat), qq('\n);
	}
}

$dbh->disconnect();

exit 0;


__END__

=head1 NAME

odbc_rpc - execute a stored procedure via DBD::ODBC

=head1 SYNOPSIS

odbc_rpc [-D dsn] [-U username] [-P password] I<procedure> [arg1[, argn]]
odbc_rpc -l 

=head1 DESCRIPTION

Execute I<procedure> with zero or more arguments.  Arguments are assumed to be bound to 
SQL_VARCHAR.  To use other bindings -- which might or might not work -- prefix the value with 
:SQL_type: where type is a valid binding type.  For a list of recognized types, use "odbc_rpc -l". 

To populate your parameter from a file instead of inline, use the form :FILE:filename.  odbc_rpc will
open I<filename> and pass its contents as data.  


=head1 EXAMPLE

  odbc_rpc -D dbi:ODBC:mpquant -d testdb -U $U -P $P \
  	   xmltest :FILE:samples/data.xml

=head1 BUGS

There is no way in DBD::ODBC to pass "long data".  The contents of the file must fit in memory. 


=head1 Author

Contributed to the FreeTDS project by James K. Lowden.  

=cut
