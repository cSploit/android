#!./perl -w

use strict ;

use lib 't';
use BerkeleyDB; 
use util ;
use Test::More;

plan(skip_all => "Heap needs Berkeley DB 5.2.x or better\n" )
    if $BerkeleyDB::db_version < 5.2;

# TODO - fix this
plan(skip_all => "Heap suport not available\n" )
    if ! BerkeleyDB::has_heap() ;

plan tests => 68;

my $Dfile = "dbhash.tmp";
my $Dfile2 = "dbhash2.tmp";
my $Dfile3 = "dbhash3.tmp";
unlink $Dfile;

umask(0) ;


# Check for invalid parameters
{
    # Check for invalid parameters
    my $db ;
    eval ' $db = new BerkeleyDB::Heap  -Stupid => 3 ; ' ;
    ok $@ =~ /unknown key value\(s\) Stupid/  ;

    eval ' $db = new BerkeleyDB::Heap -Bad => 2, -Mode => 0345, -Stupid => 3; ' ;
    ok $@ =~ /unknown key value\(s\) (Bad,? |Stupid,? ){2}/  
        or print "# $@" ;

    eval ' $db = new BerkeleyDB::Heap -Env => 2 ' ;
    ok $@ =~ /^Env not of type BerkeleyDB::Env/ ;

    eval ' $db = new BerkeleyDB::Heap -Txn => "x" ' ;
    ok $@ =~ /^Txn not of type BerkeleyDB::Txn/ ;

    my $obj = bless [], "main" ;
    eval ' $db = new BerkeleyDB::Heap -Env => $obj ' ;
    ok $@ =~ /^Env not of type BerkeleyDB::Env/ ;
}

{
    # Tied Hash interface

    my $lex = new LexFile $Dfile ;
    my %hash ;
    eval " tie %hash, 'BerkeleyDB::Heap', -Filename => '$Dfile',
                                      -Flags    => DB_CREATE ; " ;
    ok $@ =~ /^Tied Hash interface not supported with BerkeleyDB::Heap/;

}
# Now check the interface to Heap

{
    my $lex = new LexFile $Dfile ;

    ok my $db = new BerkeleyDB::Heap -Filename => $Dfile, 
				    -Flags    => DB_CREATE 
        or diag "Cannot create Heap: [$!][$BerkeleyDB::Error]\n" ;

    # Add a k/v pair
    my $value ;
    my $status ;
    my $key1;
    my $key2;
    is $db->Env, undef;
    ok $db->db_put($key1, "some value", DB_APPEND) == 0  
        or diag "Cannot db_put: " . $db->status() . "[$!][$BerkeleyDB::Error]\n" ;
    ok $db->status() == 0 ;
    ok $db->db_get($key1, $value) == 0 
        or diag "Cannot db_get: [$!][$BerkeleyDB::Error]\n" ;
    ok $value eq "some value" ;
    ok $db->db_put($key2, "value", DB_APPEND) == 0  ;
    ok $db->db_get($key2, $value) == 0 
        or diag "Cannot db_get: [$!][$BerkeleyDB::Error]\n" ;
    ok $value eq "value" ;
    ok $db->db_del($key1) == 0 ;
    ok $db->db_get($key1, $value) == DB_NOTFOUND ;
    ok $db->status() == DB_NOTFOUND ;
    ok $db->status() =~ $DB_errors{'DB_NOTFOUND'} 
        or diag "Status is [" . $db->status() . "]";

    ok $db->db_sync() == 0 ;

    # Check NOOVERWRITE will make put fail when attempting to overwrite
    # an existing record.

    ok $db->db_put( $key2, 'x', DB_NOOVERWRITE) == DB_KEYEXIST ;
    ok $db->status() =~ $DB_errors{'DB_KEYEXIST'} ;
    ok $db->status() == DB_KEYEXIST ;


    # check that the value of the key  has not been changed by the
    # previous test
    ok $db->db_get($key2, $value) == 0 ;
    ok $value eq "value" ;

    # test DB_GET_BOTH
    my ($k, $v) = ($key2, "value") ;
    ok $db->db_get($k, $v, DB_GET_BOTH) == 0 ;

    ($k, $v) = ($key2, "fred") ;
    ok $db->db_get($k, $v, DB_GET_BOTH) == DB_NOTFOUND ;

    ($k, $v) = ("another", "value") ;
    ok $db->db_get($k, $v, DB_GET_BOTH) == DB_NOTFOUND ;


}


{
    # Check simple env works with a hash.
    my $lex = new LexFile $Dfile ;

    my $home = "./fred" ;
    ok my $lexD = new LexDir($home) ;

    ok my $env = new BerkeleyDB::Env -Flags => DB_CREATE|DB_INIT_MPOOL,
    					 @StdErrFile, -Home => $home ;
    ok my $db = new BerkeleyDB::Heap -Filename => $Dfile, 
				    -Env      => $env,
				    -Flags    => DB_CREATE ;

    isa_ok $db->Env, 'BerkeleyDB::Env';
    $db->Env->errPrefix("abc");
    is $db->Env->errPrefix("abc"), 'abc';
    # Add a k/v pair
    my $key ;
    my $value ;
    ok $db->db_put($key, "some value", DB_APPEND) == 0 ;
    ok $db->db_get($key, $value) == 0 ;
    ok $value eq "some value" ;
    undef $db ;
    undef $env ;
}

 
{
    # cursors

    my $lex = new LexFile $Dfile ;
    my %hash ;
    my ($k, $v) ;
    ok my $db = new BerkeleyDB::Heap -Filename => $Dfile, 
				     -Flags    => DB_CREATE ;
    #print "[$db] [$!] $BerkeleyDB::Error\n" ;				     

    # create some data
    my %data =  ();
    my %keys =  ();

    my $ret = 0 ;
    for my $v (qw(2 house sea)){
        my $key;
        $ret += $db->db_put($key, $v, DB_APPEND) ;
        $data{$key} = $v;
        $keys{$v} = $key;
    }

    ok $ret == 0 ;

    # create the cursor
    ok my $cursor = $db->db_cursor() ;

    $k = $v = "" ;
    my %copy = %data ;
    my $extras = 0 ;
    # sequence forwards
    while ($cursor->c_get($k, $v, DB_NEXT) == 0) {
        if ( $copy{$k} eq $v ) 
            { delete $copy{$k} }
	else
	    { ++ $extras }
    }
    ok $cursor->status() == DB_NOTFOUND ;
    ok $cursor->status() =~ $DB_errors{'DB_NOTFOUND'};
    ok keys %copy == 0 ;
    ok $extras == 0 ;

    # sequence backwards
    %copy = %data ;
    $extras = 0 ;
    my $status ;
    for ( $status = $cursor->c_get($k, $v, DB_LAST) ;
	  $status == 0 ;
    	  $status = $cursor->c_get($k, $v, DB_PREV)) {
        if ( $copy{$k} eq $v ) 
            { delete $copy{$k} }
	else
	    { ++ $extras }
    }
    ok $status == DB_NOTFOUND ;
    ok $status =~ $DB_errors{'DB_NOTFOUND'};
    ok $cursor->status() == $status ;
    ok $cursor->status() eq $status ;
    ok keys %copy == 0 ;
    ok $extras == 0, "extras == 0" ;

    ($k, $v) = ($keys{"house"}, "house") ;
    ok $cursor->c_get($k, $v, DB_GET_BOTH) == 0, "c_get BOTH" ;

    ($k, $v) = ($keys{"house"}, "door") ;
    ok $cursor->c_get($k, $v, DB_GET_BOTH) == DB_NOTFOUND, "DB_NOTFOUND" ;

    ($k, $v) = ("black", "house") ;
    ok $cursor->c_get($k, $v, DB_GET_BOTH) == DB_NOTFOUND, "DB_NOTFOUND" ;

}
 



{
    # in-memory file

    my $lex = new LexFile $Dfile ;
    my %hash ;
    my $fd ;
    my $value ;
    #ok my $db = tie %hash, 'BerkeleyDB::Heap' ;
    my $db = new BerkeleyDB::Heap 
				     -Flags    => DB_CREATE ;

    isa_ok $db, 'BerkeleyDB::Heap' ;
    my $key;
    ok $db->db_put($key, "some value", DB_APPEND) == 0  ;
    ok $db->db_get($key, $value) == 0 ;
    ok $value eq "some value", "some value" ;

}
 
if (0)
{
    # partial
    # check works via API

    my $lex = new LexFile $Dfile ;
    my $value ;
    ok my $db = new BerkeleyDB::Heap -Filename => $Dfile,
                                     -Flags    => DB_CREATE ;

    # create some data
    my $red;
    my $green;
    my $blue;
    my %data =  (
		"red"	=> "boat",
		"green"	=> "house",
		"blue"	=> "sea",
		) ;

    my %keys =  (
		"red" => \$red,
		"green" => \$green,
		"blue" => \$blue,
		) ;

    my $ret = 0 ;
    while (my ($k, $v) = each %data) {
        my $key;
        $ret += $db->db_put($key, $v, DB_APPEND) ;
        ${ $keys{$k} } = $key;
    }
    ok $ret == 0, "ret 0" ;


    # do a partial get
    my ($pon, $off, $len) = $db->partial_set(0,2) ;
    ok ! $pon && $off == 0 && $len == 0 ;
    ok $db->db_get($red, $value) == 0 && $value eq "bo" ;
    ok $db->db_get($green, $value) == 0 && $value eq "ho" ;
    ok $db->db_get($blue, $value) == 0 && $value eq "se" ;

    # do a partial get, off end of data
    ($pon, $off, $len) = $db->partial_set(3,2) ;
    ok $pon ;
    ok $off == 0 ;
    ok $len == 2, "len 2" ;
    ok $db->db_get($red, $value) == 0 && $value eq "t" ;
    ok $db->db_get($green, $value) == 0 && $value eq "se" ;
    ok $db->db_get($blue, $value) == 0 && $value eq "" ;

    # switch of partial mode
    ($pon, $off, $len) = $db->partial_clear() ;
    ok $pon ;
    ok $off == 3, "off 3" ;
    ok $len == 2 ;
    ok $db->db_get($red, $value) == 0 && $value eq "boat" ;
    ok $db->db_get($green, $value) == 0 && $value eq "house" ;
    ok $db->db_get($blue, $value) == 0 && $value eq "sea" ;

    # now partial put
    my $new;
    $db->partial_set(0,2) ;
    ok $db->db_put($red, "") == 0 ;
    ok $db->db_put($green, "AB") == 0 ;
    ok $db->db_put($blue, "XYZ") == 0 ;
    ok $db->db_put($new, "KLM", DB_APPEND) == 0 ;

    ($pon, $off, $len) = $db->partial_clear() ;
    ok $pon ;
    ok $off == 0 ;
    ok $len == 2, "len 2" ;
    ok $db->db_get($red, $value) == 0 && $value eq "at" ;
    ok $db->db_get($green, $value) == 0 && $value eq "ABuse" ;
    ok $db->db_get($blue, $value) == 0 && $value eq "XYZa" ;
    ok $db->db_get($new, $value) == 0 && $value eq "KLM" ;

    # now partial put
    ($pon, $off, $len) = $db->partial_set(3,2) ;
    ok ! $pon ;
    ok $off == 0 ;
    ok $len == 0 ;
    ok $db->db_put($red, "PPP") == 0 ;
    ok $db->db_put($green, "Q") == 0, "Q" ;
    ok $db->db_put($blue, "XYZ") == 0, "XYZ" ; # <<<<<<<<<<<<<<
    ok $db->db_put($new, "TU") == 0 ;

    $db->partial_clear() ;
    ok $db->db_get($red, $value) == 0 && $value eq "at\0PPP" ;
    ok $db->db_get($green, $value) == 0 && $value eq "ABuQ" ;
    ok $db->db_get($blue, $value) == 0 && $value eq "XYZXYZ" ;
    ok $db->db_get($new, $value) == 0 && $value eq "KLMTU", "KLMTU" ;
}


{
    # transaction

    my $lex = new LexFile $Dfile ;
    my %hash ;
    my $value ;

    my $home = "./fred" ;
    ok my $lexD = new LexDir($home) ;
    ok my $env = new BerkeleyDB::Env -Home => $home, @StdErrFile,
				     -Flags => DB_CREATE|DB_INIT_TXN|
					  	DB_INIT_MPOOL|DB_INIT_LOCK ;
    ok my $txn = $env->txn_begin() ;
    ok my $db1 = new BerkeleyDB::Heap -Filename => $Dfile,
                                      	       -Flags    =>  DB_CREATE ,
					       -Env 	 => $env,
					       -Txn	 => $txn ;

    ok ((my $Z = $txn->txn_commit()) == 0) ;
    ok $txn = $env->txn_begin() ;
    $db1->Txn($txn);
    
    # create some data
    my %data =  (
		"red"	=> "boat",
		"green"	=> "house",
		"blue"	=> "sea",
		) ;

    my $ret = 0 ;
    while (my ($k, $v) = each %data) {
        my $key;
        $ret += $db1->db_put($key, $v, DB_APPEND) ;
    }
    ok $ret == 0 ;

    # should be able to see all the records

    ok my $cursor = $db1->db_cursor() ;
    my ($k, $v) = ("", "") ;
    my $count = 0 ;
    # sequence forwards
    while ($cursor->c_get($k, $v, DB_NEXT) == 0) {
        ++ $count ;
    }
    ok $count == 3 ;
    undef $cursor ;

    # now abort the transaction
    #ok $txn->txn_abort() == 0 ;
    ok (($Z = $txn->txn_abort()) == 0) ;

    # there shouldn't be any records in the database
    $count = 0 ;
    # sequence forwards
    ok $cursor = $db1->db_cursor() ;
    while ($cursor->c_get($k, $v, DB_NEXT) == 0) {
        ++ $count ;
    }
    ok $count == 0 ;

    undef $txn ;
    undef $cursor ;
    undef $db1 ;
    undef $env ;
    untie %hash ;
}
exit;

{
    # DB_DUP

    my $lex = new LexFile $Dfile ;
    my %hash ;
    ok my $db = tie %hash, 'BerkeleyDB::Heap', -Filename => $Dfile,
				      -Property  => DB_DUP,
                                      -Flags    => DB_CREATE ;

    $hash{'Wall'} = 'Larry' ;
    $hash{'Wall'} = 'Stone' ;
    $hash{'Smith'} = 'John' ;
    $hash{'Wall'} = 'Brick' ;
    $hash{'Wall'} = 'Brick' ;
    $hash{'mouse'} = 'mickey' ;

    ok keys %hash == 6 ;

    # create a cursor
    ok my $cursor = $db->db_cursor() ;

    my $key = "Wall" ;
    my $value ;
    ok $cursor->c_get($key, $value, DB_SET) == 0 ;
    ok $key eq "Wall" && $value eq "Larry" ;
    ok $cursor->c_get($key, $value, DB_NEXT) == 0 ;
    ok $key eq "Wall" && $value eq "Stone" ;
    ok $cursor->c_get($key, $value, DB_NEXT) == 0 ;
    ok $key eq "Wall" && $value eq "Brick" ;
    ok $cursor->c_get($key, $value, DB_NEXT) == 0 ;
    ok $key eq "Wall" && $value eq "Brick" ;

    #my $ref = $db->db_stat() ; 
    #ok ($ref->{bt_flags} | DB_DUP) == DB_DUP ;
#print "bt_flags " . $ref->{bt_flags} . " DB_DUP " . DB_DUP ."\n";

    undef $db ;
    undef $cursor ;
    untie %hash ;

}

{
    # db_stat

    my $lex = new LexFile $Dfile ;
    my $recs = ($BerkeleyDB::db_version >= 3.1 ? "bt_ndata" : "bt_nrecs") ;
    my %hash ;
    my ($k, $v) ;
    ok my $db = new BerkeleyDB::Heap -Filename => $Dfile, 
				     -Flags    => DB_CREATE,
				 	-Minkey	=>3 ,
					-Pagesize	=> 2 **12 
					;

    my $ref = $db->db_stat() ; 
    ok $ref->{$recs} == 0;
    ok $ref->{'bt_minkey'} == 3;
    ok $ref->{'bt_pagesize'} == 2 ** 12;

    # create some data
    my %data =  (
		"red"	=> 2,
		"green"	=> "house",
		"blue"	=> "sea",
		) ;

    my $ret = 0 ;
    while (($k, $v) = each %data) {
        $ret += $db->db_put($k, $v) ;
    }
    ok $ret == 0 ;

    $ref = $db->db_stat() ; 
    ok $ref->{$recs} == 3;
}

{
   # sub-class test

   package Another ;

   use strict ;

   open(FILE, ">SubDB.pm") or die "Cannot open SubDB.pm: $!\n" ;
   print FILE <<'EOM' ;

   package SubDB ;

   use strict ;
   use vars qw( @ISA @EXPORT) ;

   require Exporter ;
   use BerkeleyDB;
   @ISA=qw(BerkeleyDB BerkeleyDB::Heap );
   @EXPORT = @BerkeleyDB::EXPORT ;

   sub db_put { 
	my $self = shift ;
        my $key = shift ;
        my $value = shift ;
        $self->SUPER::db_put($key, $value * 3) ;
   }

   sub db_get { 
	my $self = shift ;
        $self->SUPER::db_get($_[0], $_[1]) ;
	$_[1] -= 2 ;
   }

   sub A_new_method
   {
	my $self = shift ;
        my $key = shift ;
        my $value = $self->FETCH($key) ;
	return "[[$value]]" ;
   }

   1 ;
EOM

    close FILE ;

    use Test::More;
    BEGIN { push @INC, '.'; }    
    eval 'use SubDB ; ';
    ok $@ eq "" ;
    my %h ;
    my $X ;
    eval '
	$X = tie(%h, "SubDB", -Filename => "dbbtree.tmp", 
			-Flags => DB_CREATE,
			-Mode => 0640 );
	' ;

    ok $@ eq "" && $X ;

    my $ret = eval '$h{"fred"} = 3 ; return $h{"fred"} ' ;
    ok $@ eq "" ;
    ok $ret == 7 ;

    my $value = 0;
    $ret = eval '$X->db_put("joe", 4) ; $X->db_get("joe", $value) ; return $value' ;
    ok $@ eq "" ;
    ok $ret == 10 ;

    $ret = eval ' DB_NEXT eq main::DB_NEXT ' ;
    ok $@ eq ""  ;
    ok $ret == 1 ;

    $ret = eval '$X->A_new_method("joe") ' ;
    ok $@ eq "" ;
    ok $ret eq "[[10]]" ;

    undef $X;
    untie %h;
    unlink "SubDB.pm", "dbbtree.tmp" ;

}


