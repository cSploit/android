#!/usr/local/bin/perl

my $ignore_re = '^(' . join("|", 
	qw(
		_
		[a-z]
		DBM
		DBC
		DB_AM_
		DB_BT_
		DB_RE_
		DB_HS_
		DB_FUNC_
		DB_DBT_
		DB_DBM
		DB_TSL
		MP
		TXN
        DB_TXN_GETPGNOS
        DB_TXN_BACKWARD_ALLOC
        DB_ALIGN8
	)) . ')' ;

my %ignore_def = map {$_, 1} qw() ;

%ignore_enums = map {$_, 1} qw( ACTION db_status_t db_notices db_lockmode_t ) ;

my %ignore_exact_enum = map { $_ => 1}
	qw(
                DB_TXN_GETPGNOS
                DB_TXN_BACKWARD_ALLOC
                );

my $filler = ' ' x 26 ;

chdir "libraries" || die "Cannot chdir into './libraries': $!\n";

foreach my $name (sort tuple glob "[2-9]*")
{
    next if $name =~ /(NOHEAP|NC|private)$/;

    my $inc = "$name/include/db.h" ;
    next unless -f $inc ;

    my $file = readFile($inc) ;
    StripCommentsAndStrings($file) ;
    my $result = scan($name, $file) ;
    print "\n\t#########\n\t# $name\n\t#########\n\n$result" 
        if $result;
}
exit ;


sub scan
{
    my $version = shift ;
    my $file = shift ;

    my %seen_define = () ;
    my $result = "" ;

    if (1) {
        # Preprocess all tri-graphs 
        # including things stuck in quoted string constants.
        $file =~ s/\?\?=/#/g;                         # | ??=|  #|
        $file =~ s/\?\?\!/|/g;                        # | ??!|  ||
        $file =~ s/\?\?'/^/g;                         # | ??'|  ^|
        $file =~ s/\?\?\(/[/g;                        # | ??(|  [|
        $file =~ s/\?\?\)/]/g;                        # | ??)|  ]|
        $file =~ s/\?\?\-/~/g;                        # | ??-|  ~|
        $file =~ s/\?\?\//\\/g;                       # | ??/|  \|
        $file =~ s/\?\?</{/g;                         # | ??<|  {|
        $file =~ s/\?\?>/}/g;                         # | ??>|  }|
    }
    
    while ( $file =~ /^\s*#\s*define\s+([\$\w]+)\b(?!\()\s*(.*)/gm ) 
    {
        my $def = $1;
        my $rest = $2;
        my $ignore = 0 ;
    
        $ignore = 1 if $ignore_def{$def} || $def =~ /$ignore_re/o ;
    
        # Cannot do: (-1) and ((LHANDLE)3) are OK:
        #print("Skip non-wordy $def => $rest\n"),
    
        $rest =~ s/\s*$//;
        #next if $rest =~ /[^\w\$]/;
    
        #print "Matched $_ ($def)\n" ;

	next if $before{$def} ++ ;
    
        if ($ignore)
          { $seen_define{$def} = 'IGNORE' }
        elsif ($rest =~ /"/) 
          { $seen_define{$def} = 'STRING' }
        else
          { $seen_define{$def} = 'DEFINE' }
    }
    
    foreach $define (sort keys %seen_define)
    { 
        my $out = $filler ;
        substr($out,0, length $define) = $define;
        $result .= "\t$out => $seen_define{$define},\n" ;
    }
    
    while ($file =~ /\btypedef\s+enum\s*{(.*?)}\s*(\w+)/gs )
    {
        my $enum = $1 ;
        my $name = $2 ;
        my $ignore = 0 ;
    
        $ignore = 1 if $ignore_enums{$name} ;
    
        #$enum =~ s/\s*=\s*\S+\s*(,?)\s*\n/$1/g;
        $enum =~ s/^\s*//;
        $enum =~ s/\s*$//;
    
        my @tokens = map { s/\s*=.*// ; $_} split /\s*,\s*/, $enum ;
        my @new =  grep { ! $Enums{$_}++ } @tokens ;

        if (@new)
        {
            my $value ;
            if ($ignore)
              { $value = "IGNORE, # $version" }
            else
              { $value = "'$version'," }

            $result .= "\n\t# enum $name\n";
            my $out = $filler ;
            foreach $name (@new)
            {
                next if $ignore_exact_enum{$name} ;
                $out = $filler ;
                substr($out,0, length $name) = $name;
                $result .= "\t$out => $value\n" ;
            }
        }
    }

    return $result ;
}


sub StripCommentsAndStrings
{

  # Strip C & C++ coments
  # From the perlfaq
  $_[0] =~

    s{
       /\*         ##  Start of /* ... */ comment
       [^*]*\*+    ##  Non-* followed by 1-or-more *'s
       (
         [^/*][^*]*\*+
       )*          ##  0-or-more things which don't start with /
                   ##    but do end with '*'
       /           ##  End of /* ... */ comment
 
     |         ##     OR  C++ Comment
       //          ## Start of C++ comment // 
       [^\n]*      ## followed by 0-or-more non end of line characters

     |         ##     OR  various things which aren't comments:
 
       (
         "           ##  Start of " ... " string
         (
           \\.           ##  Escaped char
         |               ##    OR
           [^"\\]        ##  Non "\
         )*
         "           ##  End of " ... " string
 
       |         ##     OR
 
         '           ##  Start of ' ... ' string
         (
           \\.           ##  Escaped char
         |               ##    OR
           [^'\\]        ##  Non '\
         )*
         '           ##  End of ' ... ' string
 
       |         ##     OR
 
         .           ##  Anything other char
         [^/"'\\]*   ##  Chars which doesn't start a comment, string or escape
       )
     }{$2}gxs;



  # Remove double-quoted strings.
  #$_[0] =~ s#"(\\.|[^"\\])*"##g;

  # Remove single-quoted strings.
  #$_[0] =~ s#'(\\.|[^'\\])*'##g;

  # Remove leading whitespace.
  $_[0] =~ s/\A\s+//m ;

  # Remove trailing whitespace.
  $_[0] =~ s/\s+\Z//m ;

  # Replace all multiple whitespace by a single space.
  #$_[0] =~ s/\s+/ /g ;
}


sub readFile
{
   my $filename = shift ;
   open F, "<$filename" || die "Cannot open $filename: $!\n" ;
   local $/ ;
   my $x = <F> ;
   close F ;
   return $x ;
}

sub tuple
{
    my (@a) = split(/\./, $a) ;
    my (@b) = split(/\./, $b) ;
    if (@a != @b) {
        my $diff = @a - @b ;
        push @b, (0 x $diff) if $diff > 0 ;
        push @a, (0 x -$diff) if $diff < 0 ;
    }
    foreach $A (@a) {
        $B = shift @b ;
        $A == $B or return $A <=> $B ;
    }
    return 0;
}          

__END__

