#!./perl -w

use strict ;

use lib 't';
use BerkeleyDB; 
use util ;
use Test::More;

plan tests => 250;

my $Dfile = "dbhash.tmp";
my $Dfile2 = "dbhash2.tmp";
my $Dfile3 = "dbhash3.tmp";
unlink $Dfile;

umask(0) ;


# Check for invalid parameters
{
    # Check for invalid parameters
    my $db ;
    eval ' $db = new BerkeleyDB::Btree  -Stupid => 3 ; ' ;
    ok $@ =~ /unknown key value\(s\) Stupid/  ;

    eval ' $db = new BerkeleyDB::Btree -Bad => 2, -Mode => 0345, -Stupid => 3; ' ;
    ok $@ =~ /unknown key value\(s\) (Bad,? |Stupid,? ){2}/  
        or print "# $@" ;

    eval ' $db = new BerkeleyDB::Btree -Env => 2 ' ;
    ok $@ =~ /^Env not of type BerkeleyDB::Env/ ;

    eval ' $db = new BerkeleyDB::Btree -Txn => "x" ' ;
    ok $@ =~ /^Txn not of type BerkeleyDB::Txn/ ;

    my $obj = bless [], "main" ;
    eval ' $db = new BerkeleyDB::Btree -Env => $obj ' ;
    ok $@ =~ /^Env not of type BerkeleyDB::Env/ ;
}

# Now check the interface to Btree

{
    my $lex = new LexFile $Dfile ;

    ok my $db = new BerkeleyDB::Btree -Filename => $Dfile, 
				    -Flags    => DB_CREATE ;

    # Add a k/v pair
    my $value ;
    my $status ;
    is $db->Env, undef;
    ok $db->db_put("some key", "some value") == 0  ;
    ok $db->status() == 0 ;
    ok $db->db_get("some key", $value) == 0 ;
    ok $value eq "some value" ;
    ok $db->db_put("key", "value") == 0  ;
    ok $db->db_get("key", $value) == 0 ;
    ok $value eq "value" ;
    ok $db->db_del("some key") == 0 ;
    ok $db->db_get("some key", $value) == DB_NOTFOUND ;
    ok $db->status() == DB_NOTFOUND ;
    ok $db->status() =~ $DB_errors{'DB_NOTFOUND'} 
        or diag "Status is [" . $db->status() . "]";

    ok $db->db_sync() == 0 ;

    # Check NOOVERWRITE will make put fail when attempting to overwrite
    # an existing record.

    ok $db->db_put( 'key', 'x', DB_NOOVERWRITE) == DB_KEYEXIST ;
    ok $db->status() =~ $DB_errors{'DB_KEYEXIST'} ;
    ok $db->status() == DB_KEYEXIST ;


    # check that the value of the key  has not been changed by the
    # previous test
    ok $db->db_get("key", $value) == 0 ;
    ok $value eq "value" ;

    # test DB_GET_BOTH
    my ($k, $v) = ("key", "value") ;
    ok $db->db_get($k, $v, DB_GET_BOTH) == 0 ;

    ($k, $v) = ("key", "fred") ;
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
    ok my $db = new BerkeleyDB::Btree -Filename => $Dfile, 
				    -Env      => $env,
				    -Flags    => DB_CREATE ;

    isa_ok $db->Env, 'BerkeleyDB::Env';
    $db->Env->errPrefix("abc");
    is $db->Env->errPrefix("abc"), 'abc';
    # Add a k/v pair
    my $value ;
    ok $db->db_put("some key", "some value") == 0 ;
    ok $db->db_get("some key", $value) == 0 ;
    ok $value eq "some value" ;
    undef $db ;
    undef $env ;
}

 
{
    # cursors

    my $lex = new LexFile $Dfile ;
    my %hash ;
    my ($k, $v) ;
    ok my $db = new BerkeleyDB::Btree -Filename => $Dfile, 
				     -Flags    => DB_CREATE ;
    #print "[$db] [$!] $BerkeleyDB::Error\n" ;				     

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
    ok $extras == 0 ;

    ($k, $v) = ("green", "house") ;
    ok $cursor->c_get($k, $v, DB_GET_BOTH) == 0 ;

    ($k, $v) = ("green", "door") ;
    ok $cursor->c_get($k, $v, DB_GET_BOTH) == DB_NOTFOUND ;

    ($k, $v) = ("black", "house") ;
    ok $cursor->c_get($k, $v, DB_GET_BOTH) == DB_NOTFOUND ;

}
 
{
    # Tied Hash interface

    my $lex = new LexFile $Dfile ;
    my %hash ;
    ok tie %hash, 'BerkeleyDB::Btree', -Filename => $Dfile,
                                      -Flags    => DB_CREATE ;

    is((tied %hash)->Env, undef);
    # check "each" with an empty database
    my $count = 0 ;
    while (my ($k, $v) = each %hash) {
	++ $count ;
    }
    ok ((tied %hash)->status() == DB_NOTFOUND) ;
    ok $count == 0 ;

    # Add a k/v pair
    my $value ;
    $hash{"some key"} = "some value";
    ok ((tied %hash)->status() == 0) ;
    ok $hash{"some key"} eq "some value";
    ok defined $hash{"some key"} ;
    ok ((tied %hash)->status() == 0) ;
    ok exists $hash{"some key"} ;
    ok !defined $hash{"jimmy"} ;
    ok ((tied %hash)->status() == DB_NOTFOUND) ;
    ok !exists $hash{"jimmy"} ;
    ok ((tied %hash)->status() == DB_NOTFOUND) ;

    delete $hash{"some key"} ;
    ok ((tied %hash)->status() == 0) ;
    ok ! defined $hash{"some key"} ;
    ok ((tied %hash)->status() == DB_NOTFOUND) ;
    ok ! exists $hash{"some key"} ;
    ok ((tied %hash)->status() == DB_NOTFOUND) ;

    $hash{1} = 2 ;
    $hash{10} = 20 ;
    $hash{1000} = 2000 ;

    my ($keys, $values) = (0,0);
    $count = 0 ;
    while (my ($k, $v) = each %hash) {
        $keys += $k ;
	$values += $v ;
	++ $count ;
    }
    ok $count == 3 ;
    ok $keys == 1011 ;
    ok $values == 2022 ;

    # now clear the hash
    %hash = () ;
    ok keys %hash == 0 ;

    untie %hash ;
}

{
    # override default compare
    my $lex = new LexFile $Dfile, $Dfile2, $Dfile3 ;
    my $value ;
    my (%h, %g, %k) ;
    my @Keys = qw( 0123 12 -1234 9 987654321 def  ) ; 
    ok tie %h, "BerkeleyDB::Btree", -Filename => $Dfile, 
				     -Compare   => sub { $_[0] <=> $_[1] },
				     -Flags    => DB_CREATE ;

    ok tie %g, 'BerkeleyDB::Btree', -Filename => $Dfile2, 
				     -Compare   => sub { $_[0] cmp $_[1] },
				     -Flags    => DB_CREATE ;

    ok tie %k, 'BerkeleyDB::Btree', -Filename => $Dfile3, 
				   -Compare   => sub { length $_[0] <=> length $_[1] },
				   -Flags    => DB_CREATE ;

    my @srt_1 ;
    { local $^W = 0 ;
      @srt_1 = sort { $a <=> $b } @Keys ; 
    }
    my @srt_2 = sort { $a cmp $b } @Keys ;
    my @srt_3 = sort { length $a <=> length $b } @Keys ;

    foreach (@Keys) {
        local $^W = 0 ;
        $h{$_} = 1 ; 
        $g{$_} = 1 ;
        $k{$_} = 1 ;
    }

    is_deeply [keys %h], \@srt_1 ;
    is_deeply [keys %g], \@srt_2 ;
    is_deeply [keys %k], \@srt_3 ;
}

{
    # override default compare, with duplicates, don't sort values
    my $lex = new LexFile $Dfile, $Dfile2, $Dfile3 ;
    my $value ;
    my (%h, %g, %k) ;
    my @Keys   = qw( 0123 9 12 -1234 9 987654321 def  ) ; 
    my @Values = qw( 1    0 3   dd   x abc       0    ) ; 
    ok tie %h, "BerkeleyDB::Btree", -Filename => $Dfile, 
				     -Compare   => sub { $_[0] <=> $_[1] },
				     -Property  => DB_DUP,
				     -Flags    => DB_CREATE ;

    ok tie %g, 'BerkeleyDB::Btree', -Filename => $Dfile2, 
				     -Compare   => sub { $_[0] cmp $_[1] },
				     -Property  => DB_DUP,
				     -Flags    => DB_CREATE ;

    ok tie %k, 'BerkeleyDB::Btree', -Filename => $Dfile3, 
				   -Compare   => sub { length $_[0] <=> length $_[1] },
				   -Property  => DB_DUP,
				   -Flags    => DB_CREATE ;

    my @srt_1 ;
    { local $^W = 0 ;
      @srt_1 = sort { $a <=> $b } @Keys ; 
    }
    my @srt_2 = sort { $a cmp $b } @Keys ;
    my @srt_3 = sort { length $a <=> length $b } @Keys ;

    foreach (@Keys) {
        local $^W = 0 ;
        my $value = shift @Values ;
        $h{$_} = $value ; 
        $g{$_} = $value ;
        $k{$_} = $value ;
    }

    sub getValues
    {
        my $hash = shift ;
        my $db = tied %$hash ;
        my $cursor = $db->db_cursor() ;
        my @values = () ;
        my ($k, $v) = (0,0) ;
        while ($cursor->c_get($k, $v, DB_NEXT) == 0) {
            push @values, $v ;
        }
        return @values ;
    }

    is_deeply [keys %h], \@srt_1 ;
    is_deeply [keys %g], \@srt_2 ;
    is_deeply [keys %k], \@srt_3 ;

    is_deeply [getValues \%h], [qw(dd 0 0 x 3 1 abc)];
    is_deeply [getValues \%g], [qw(dd 1 3 0 x abc 0)]
        or diag "Expected [dd 1 0 3 x abc 0] got [@{ [getValues(\%g)] }]\n";
    is_deeply [getValues \%k], [qw(0 x 3 0 1 dd abc)];

    # test DB_DUP_NEXT
    ok my $cur = (tied %g)->db_cursor() ;
    my ($k, $v) = (9, "") ;
    ok $cur->c_get($k, $v, DB_SET) == 0 ;
    ok $k == 9 && $v == 0 ;
    ok $cur->c_get($k, $v, DB_NEXT_DUP) == 0 ;
    ok $k == 9 && $v eq "x" ;
    ok $cur->c_get($k, $v, DB_NEXT_DUP) == DB_NOTFOUND ;
}

{
    # override default compare, with duplicates, sort values
    my $lex = new LexFile $Dfile, $Dfile2;
    my $value ;
    my (%h, %g) ;
    my @Keys   = qw( 0123 9 12 -1234 9 987654321 9 def  ) ; 
    my @Values = qw( 1    11 3   dd   x abc      2 0    ) ; 
    ok tie %h, "BerkeleyDB::Btree", -Filename => $Dfile, 
				     -Compare   => sub { $_[0] <=> $_[1] },
				     -DupCompare   => sub { $_[0] cmp $_[1] },
				     -Property  => DB_DUP,
				     -Flags    => DB_CREATE ;

    ok tie %g, 'BerkeleyDB::Btree', -Filename => $Dfile2, 
				     -Compare   => sub { $_[0] cmp $_[1] },
				     -DupCompare   => sub { $_[0] <=> $_[1] },
				     -Property  => DB_DUP,
				     
				     
				     
				     -Flags    => DB_CREATE ;

    my @srt_1 ;
    { local $^W = 0 ;
      @srt_1 = sort { $a <=> $b } @Keys ; 
    }
    my @srt_2 = sort { $a cmp $b } @Keys ;

    foreach (@Keys) {
        local $^W = 0 ;
        my $value = shift @Values ;
        $h{$_} = $value ; 
        $g{$_} = $value ;
    }

    is_deeply [keys %h], \@srt_1 ;
    is_deeply [keys %g], \@srt_2 ;
    is_deeply [getValues \%h], [qw(dd 0 11 2 x 3 1 abc)];
    is_deeply [getValues \%h], [qw(dd 0 11 2 x 3 1 abc)];

}

{
    # get_dup etc
    my $lex = new LexFile $Dfile;
    my %hh ;

    ok my $YY = tie %hh, "BerkeleyDB::Btree", -Filename => $Dfile, 
				     -DupCompare   => sub { $_[0] cmp $_[1] },
				     -Property  => DB_DUP,
				     -Flags    => DB_CREATE ;

    $hh{'Wall'} = 'Larry' ;
    $hh{'Wall'} = 'Stone' ; # Note the duplicate key
    $hh{'Wall'} = 'Brick' ; # Note the duplicate key
    $hh{'Smith'} = 'John' ;
    $hh{'mouse'} = 'mickey' ;
    
    # first work in scalar context
    ok scalar $YY->get_dup('Unknown') == 0 ;
    ok scalar $YY->get_dup('Smith') == 1 ;
    ok scalar $YY->get_dup('Wall') == 3 ;
    
    # now in list context
    my @unknown = $YY->get_dup('Unknown') ;
    ok "@unknown" eq "" ;
    
    my @smith = $YY->get_dup('Smith') ;
    ok "@smith" eq "John" ;
    
    {
    my @wall = $YY->get_dup('Wall') ;
    my %wall ;
    @wall{@wall} = @wall ;
    ok (@wall == 3 && $wall{'Larry'} && $wall{'Stone'} && $wall{'Brick'});
    }
    
    # hash
    my %unknown = $YY->get_dup('Unknown', 1) ;
    ok keys %unknown == 0 ;
    
    my %smith = $YY->get_dup('Smith', 1) ;
    ok keys %smith == 1 && $smith{'John'} ;
    
    my %wall = $YY->get_dup('Wall', 1) ;
    ok keys %wall == 3 && $wall{'Larry'} == 1 && $wall{'Stone'} == 1 
    		&& $wall{'Brick'} == 1 ;
    
    undef $YY ;
    untie %hh ;

}

{
    # in-memory file

    my $lex = new LexFile $Dfile ;
    my %hash ;
    my $fd ;
    my $value ;
    ok my $db = tie %hash, 'BerkeleyDB::Btree' ;

    ok $db->db_put("some key", "some value") == 0  ;
    ok $db->db_get("some key", $value) == 0 ;
    ok $value eq "some value" ;

}
 
{
    # partial
    # check works via API

    my $lex = new LexFile $Dfile ;
    my $value ;
    ok my $db = new BerkeleyDB::Btree -Filename => $Dfile,
                                       -Flags    => DB_CREATE ;

    # create some data
    my %data =  (
		"red"	=> "boat",
		"green"	=> "house",
		"blue"	=> "sea",
		) ;

    my $ret = 0 ;
    while (my ($k, $v) = each %data) {
        $ret += $db->db_put($k, $v) ;
    }
    ok $ret == 0 ;


    # do a partial get
    my ($pon, $off, $len) = $db->partial_set(0,2) ;
    ok ! $pon && $off == 0 && $len == 0 ;
    ok $db->db_get("red", $value) == 0 && $value eq "bo" ;
    ok $db->db_get("green", $value) == 0 && $value eq "ho" ;
    ok $db->db_get("blue", $value) == 0 && $value eq "se" ;

    # do a partial get, off end of data
    ($pon, $off, $len) = $db->partial_set(3,2) ;
    ok $pon ;
    ok $off == 0 ;
    ok $len == 2 ;
    ok $db->db_get("red", $value) == 0 && $value eq "t" ;
    ok $db->db_get("green", $value) == 0 && $value eq "se" ;
    ok $db->db_get("blue", $value) == 0 && $value eq "" ;

    # switch of partial mode
    ($pon, $off, $len) = $db->partial_clear() ;
    ok $pon ;
    ok $off == 3 ;
    ok $len == 2 ;
    ok $db->db_get("red", $value) == 0 && $value eq "boat" ;
    ok $db->db_get("green", $value) == 0 && $value eq "house" ;
    ok $db->db_get("blue", $value) == 0 && $value eq "sea" ;

    # now partial put
    $db->partial_set(0,2) ;
    ok $db->db_put("red", "") == 0 ;
    ok $db->db_put("green", "AB") == 0 ;
    ok $db->db_put("blue", "XYZ") == 0 ;
    ok $db->db_put("new", "KLM") == 0 ;

    ($pon, $off, $len) = $db->partial_clear() ;
    ok $pon ;
    ok $off == 0 ;
    ok $len == 2 ;
    ok $db->db_get("red", $value) == 0 && $value eq "at" ;
    ok $db->db_get("green", $value) == 0 && $value eq "ABuse" ;
    ok $db->db_get("blue", $value) == 0 && $value eq "XYZa" ;
    ok $db->db_get("new", $value) == 0 && $value eq "KLM" ;

    # now partial put
    ($pon, $off, $len) = $db->partial_set(3,2) ;
    ok ! $pon ;
    ok $off == 0 ;
    ok $len == 0 ;
    ok $db->db_put("red", "PPP") == 0 ;
    ok $db->db_put("green", "Q") == 0 ;
    ok $db->db_put("blue", "XYZ") == 0 ;
    ok $db->db_put("new", "TU") == 0 ;

    $db->partial_clear() ;
    ok $db->db_get("red", $value) == 0 && $value eq "at\0PPP" ;
    ok $db->db_get("green", $value) == 0 && $value eq "ABuQ" ;
    ok $db->db_get("blue", $value) == 0 && $value eq "XYZXYZ" ;
    ok $db->db_get("new", $value) == 0 && $value eq "KLMTU" ;
}

{
    # partial
    # check works via tied hash 

    my $lex = new LexFile $Dfile ;
    my %hash ;
    my $value ;
    ok my $db = tie %hash, 'BerkeleyDB::Btree', -Filename => $Dfile,
                                      	       -Flags    => DB_CREATE ;

    # create some data
    my %data =  (
		"red"	=> "boat",
		"green"	=> "house",
		"blue"	=> "sea",
		) ;

    while (my ($k, $v) = each %data) {
	$hash{$k} = $v ;
    }


    # do a partial get
    $db->partial_set(0,2) ;
    ok $hash{"red"} eq "bo" ;
    ok $hash{"green"} eq "ho" ;
    ok $hash{"blue"}  eq "se" ;

    # do a partial get, off end of data
    $db->partial_set(3,2) ;
    ok $hash{"red"} eq "t" ;
    ok $hash{"green"} eq "se" ;
    ok $hash{"blue"} eq "" ;

    # switch of partial mode
    $db->partial_clear() ;
    ok $hash{"red"} eq "boat" ;
    ok $hash{"green"} eq "house" ;
    ok $hash{"blue"} eq "sea" ;

    # now partial put
    $db->partial_set(0,2) ;
    ok $hash{"red"} = "" ;
    ok $hash{"green"} = "AB" ;
    ok $hash{"blue"} = "XYZ" ;
    ok $hash{"new"} = "KLM" ;

    $db->partial_clear() ;
    ok $hash{"red"} eq "at" ;
    ok $hash{"green"} eq "ABuse" ;
    ok $hash{"blue"} eq "XYZa" ;
    ok $hash{"new"} eq "KLM" ;

    # now partial put
    $db->partial_set(3,2) ;
    ok $hash{"red"} = "PPP" ;
    ok $hash{"green"} = "Q" ;
    ok $hash{"blue"} = "XYZ" ;
    ok $hash{"new"} = "TU" ;

    $db->partial_clear() ;
    ok $hash{"red"} eq "at\0PPP" ;
    ok $hash{"green"} eq "ABuQ" ;
    ok $hash{"blue"} eq "XYZXYZ" ;
    ok $hash{"new"} eq "KLMTU" ;
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
    ok my $db1 = tie %hash, 'BerkeleyDB::Btree', -Filename => $Dfile,
                                      	       -Flags    =>  DB_CREATE ,
					       -Env 	 => $env,
					       -Txn	 => $txn ;

    isa_ok((tied %hash)->Env, 'BerkeleyDB::Env');
    (tied %hash)->Env->errPrefix("abc");
    is((tied %hash)->Env->errPrefix("abc"), 'abc');
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
        $ret += $db1->db_put($k, $v) ;
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

{
    # DB_DUP

    my $lex = new LexFile $Dfile ;
    my %hash ;
    ok my $db = tie %hash, 'BerkeleyDB::Btree', -Filename => $Dfile,
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
    ok my $db = new BerkeleyDB::Btree -Filename => $Dfile, 
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
   @ISA=qw(BerkeleyDB BerkeleyDB::Btree );
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

{
    # DB_RECNUM, DB_SET_RECNO & DB_GET_RECNO

    my $lex = new LexFile $Dfile ;
    my %hash ;
    my ($k, $v) = ("", "");
    ok my $db = new BerkeleyDB::Btree 
				-Filename  => $Dfile, 
			     	-Flags     => DB_CREATE,
			     	-Property  => DB_RECNUM ;


    # create some data
    my @data =  (
		"A zero",
		"B one",
		"C two",
		"D three",
		"E four"
		) ;

    my $ix = 0 ;
    my $ret = 0 ;
    foreach (@data) {
        $ret += $db->db_put($_, $ix) ;
	++ $ix ;
    }
    ok $ret == 0 ;

    # db_get & DB_SET_RECNO
    $k = 1 ;
    ok $db->db_get($k, $v, DB_SET_RECNO) == 0;
    ok $k eq "B one" && $v == 1 ;

    $k = 3 ;
    ok $db->db_get($k, $v, DB_SET_RECNO) == 0;
    ok $k eq "D three" && $v == 3 ;

    $k = 4 ;
    ok $db->db_get($k, $v, DB_SET_RECNO) == 0;
    ok $k eq "E four" && $v == 4 ;

    $k = 0 ;
    ok $db->db_get($k, $v, DB_SET_RECNO) == 0;
    ok $k eq "A zero" && $v == 0 ;

    # cursor & DB_SET_RECNO

    # create the cursor
    ok my $cursor = $db->db_cursor() ;

    $k = 2 ;
    ok $db->db_get($k, $v, DB_SET_RECNO) == 0;
    ok $k eq "C two" && $v == 2 ;

    $k = 0 ;
    ok $cursor->c_get($k, $v, DB_SET_RECNO) == 0;
    ok $k eq "A zero" && $v == 0 ;

    $k = 3 ;
    ok $db->db_get($k, $v, DB_SET_RECNO) == 0;
    ok $k eq "D three" && $v == 3 ;

    # cursor & DB_GET_RECNO
    ok $cursor->c_get($k, $v, DB_FIRST) == 0 ;
    ok $k eq "A zero" && $v == 0 ;
    ok $cursor->c_get($k, $v, DB_GET_RECNO) == 0;
    ok $v == 0 ;

    ok $cursor->c_get($k, $v, DB_NEXT) == 0 ;
    ok $k eq "B one" && $v == 1 ;
    ok $cursor->c_get($k, $v, DB_GET_RECNO) == 0;
    ok $v == 1 ;

    ok $cursor->c_get($k, $v, DB_LAST) == 0 ;
    ok $k eq "E four" && $v == 4 ;
    ok $cursor->c_get($k, $v, DB_GET_RECNO) == 0;
    ok $v == 4 ;

}

