#!/usr/local/bin/perl
# $Id: odbctest.pl,v 1.2 2005-04-04 22:05:33 jklowden Exp $

use DBI;

my ($dsn, $user, $pass, $sql) = @ARGV;

$dsn  = "dbi:ODBC:JDBC" unless $dsn;
$user = 'guest'  unless $user;
$pass = 'sybase' unless $pass;

$sql ="select \@\@servername" unless $sql;

my $dbh = DBI->connect($dsn, $user, $pass, {PrintError => 1});

die "Unable for connect to server $DBI::errstr"
    unless $dbh;

my $rc;

my $sth = $dbh->prepare($sql);
if($sth->execute) {
    while(@dat = $sth->fetchrow) {
		print "@dat\n";
    }
}
