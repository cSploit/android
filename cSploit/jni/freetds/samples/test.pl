#!/usr/bin/perl
#
use DBI;

my ($servername, $username, $password) = @ARGV;

$servername = 'JDBC' unless $servername;
$username = 'guest' unless $username;
$password = 'sybase' unless $password;

my $dbh = DBI->connect("dbi:Sybase:server=$servername", $username, $password, {PrintError => 0});

die "Unable for connect to server $DBI::errstr"
    unless $dbh;

my $rc;
my $sth;

if (@ARGV > 3) {
	$query = $ARGV[3];
} else {
	$query = "select \@\@version";
}

$sth = $dbh->prepare($query);
print $sth->{"ChopBlanks"} = 0;

if($sth->execute) {
    while(@dat = $sth->fetchrow) {
		print "@dat\n";
    }
}
