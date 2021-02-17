#!./perl -w

use strict ;

use lib 't';
use BerkeleyDB; 
use util ;
use Test::More;

plan(skip_all => "this needs Berkeley DB 6.x.x or better\n" )
    if $BerkeleyDB::db_version < 6;

plan tests => 84;

my $Dfile = "dbhash.tmp";
my $Dfile2 = "dbhash2.tmp";
my $Dfile3 = "dbhash3.tmp";
unlink $Dfile;

umask(0) ;

sub isBlob
{
    my $cursor = shift ;
    my $key = shift;

    my $v = '';
    $cursor->partial_set(0,0) ;
    $cursor->c_get($key, $v, DB_SET) ;
    $cursor->partial_clear() ;
    return defined $cursor->db_stream(DB_STREAM_WRITE);
}

for my $TYPE ( qw(BerkeleyDB::Hash BerkeleyDB::Btree ))
{
    #diag "Test $TYPE";
    my $lex = new LexFile $Dfile ;
    my $home = "./fred" ;
    my $lexd = new LexDir $home ;
    my $threshold = 1234 ;

    ok my $env = new BerkeleyDB::Env 
                        Flags => DB_CREATE|DB_INIT_MPOOL,
                        #@StdErrFile, 
                        BlobDir => $home,
                        Home => $home ;

    ok my $db = new $TYPE Filename => $Dfile, 
				    Env      => $env,
                    BlobThreshold => $threshold,
				    Flags    => DB_CREATE ;

    isa_ok $db, $TYPE ;

    ok $env->get_blob_threshold(my $t1) == 0 ;
    is $t1, 0," env threshold is 0" ;

    ok $env->get_blob_dir(my $dir1) == 0 ;
    is $dir1, $home," env threshold is 0" ;

    ok $db->get_blob_threshold(my $t2) == 0 ;
    is $t2, $threshold," db threshold is $threshold" ;

    ok $db->get_blob_dir(my $dir2) == 0 ;
    is $dir2, $home, " env threshold is 0" ;

    my $smallData = "a234";
    my $bigData = "x" x ($threshold+1) ;
    ok $db->db_put("1", $bigData) == 0  ;
    ok $db->db_put("2", $smallData) == 0  ;

    my $v2 ;
    ok $db->db_get("1", $v2) == 0 ;
    is $v2, $bigData;

    my $v1 ;
    ok $db->db_get("2", $v1) == 0 ;
    is $v1, $smallData;

    ok my $cursor = $db->db_cursor() ;

    ok isBlob($cursor, "1");
    ok !isBlob($cursor, "2");

    my $k = "1";
    my $v = '';
    $cursor->partial_set(0,0) ;
    ok $cursor->c_get($k, $v, DB_SET) == 0, "set cursor"
        or diag "Status is [" . $cursor->status() . "]";
    $cursor->partial_clear() ;
    is $k, "1";
    ok my $dbstream = $cursor->db_stream(DB_STREAM_WRITE)
        or diag "Status is [" . $cursor->status() . "]";
    isa_ok $dbstream, 'BerkeleyDB::DbStream';
    ok $dbstream->size(my $s) == 0 , "size";
    is $s, length $bigData, "length ok";
    my $new ;
    ok $dbstream->read($new, 0, length $bigData) == 0 , "read"
        or diag "Status is [" . $cursor->status() . "]";
    is $new, $bigData;
    my $newData = "hello world" ;
    ok $dbstream->write($newData) == 0 , "write";

    substr($bigData, 0, length($newData)) = $newData;
    
    my $new1;
    ok $dbstream->read($new, 0, 5) == 0 , "read";
    is $new, "hello";

    ok $dbstream->close() == 0 , "close";

    $k = "1";
    my $stream = $cursor->c_get_db_stream($k, DB_SET, DB_STREAM_WRITE) ;
    isa_ok $stream, 'BerkeleyDB::DbStream';
    is $k, "1";
    ok $stream->size($s) == 0 , "size";
    is $s, length $bigData, "length ok";
    $new = 'abc';
    ok $stream->read($new, 0, 5) == 0 , "read";
    is $new, "hello";
    ok $stream->close() == 0 , "close";


    ok my $cursor1 = $db->db_cursor() ;
    my $d1 ;
    my $d2 ;
    while (1)
    {
        my $k = '';
        my $v = '';
        $cursor->partial_set(0,0) ;
        my $status = $cursor1->c_get($k, $v, DB_NEXT) ;
        $cursor->partial_clear();

        last if $status != 0 ;

        my $stream = $cursor1->db_stream(DB_STREAM_WRITE);

        if (defined $stream)
        {
            $stream->size(my $s) ;
            my $d = '';
            my $delta = 1024;
            my $off = 0;
            while ($s)
            {
                $delta = $s if $s - $delta < 0 ;

                $stream->read($d, $off, $delta);
                $off += $delta ;
                $s -= $delta ;
                $d1 .= $d ;
            }

        }
        else
        {
            $cursor1->c_get($k, $d2, DB_CURRENT) ;
        }
    }

    is $d1, $bigData;
    is $d2, $smallData;

}

