#!/usr/bin/perl
## This file is in the public domain.
use File::Basename;

$basename = basename($0);
$srcdir = "$ARGV[0]/";

# get list of character sets we know about
$filename = "${srcdir}alternative_character_sets.h";
open ALT, $filename or die qq($basename: could not open "$filename"\n);
while(<ALT>){
	next unless /^\t[, ] {\s+"(.+?)", "(.+?)"/;
	$alternates{$2} = $1;
}
close ALT;

$alternates{'UTF8'}	= 'UTF-8';
$alternates{'ISO_1'}    = 'ISO-8859-1';
$alternates{'ASCII_8'}  = 'ISO-8859-1';
$alternates{'ISO_1'}    = 'ISO-8859-1';
$alternates{'ISO646'}   = 'ANSI_X3.4-1968';

$alternates{'MAC_CYR'}  = 'MACCYRILLIC';
#alternates{'MAC_EE'}   = '';
$alternates{'MACTURK'}  = 'MACTURKISH';

$alternates{'KOI8'}  = 'KOI8-R';

$filename = "${srcdir}sybase_character_sets.h";
open(OUT, ">$filename") or die qq($basename: could not open "$filename"\n);
print OUT "/*\n";
print OUT " * This file produced from $0\n";
print OUT ' * $Id: encodings.pl,v 1.12 2010-11-26 19:46:55 freddy77 Exp $', "\n";
print OUT " */\n";

# look up the canonical name
$comma = ' ';
while(<DATA>){
	next if /^#/;
	next if /^\s*$/;
	($name) = split;
	$Name = uc $name;
	$iconv_name = $alternates{$Name};
	
	if( !$iconv_name ) { # try predictable transformations
		$Name =~ s/ISO8859(\d{1,2})$/ISO-8859-$1/;
		$Name =~ s/ISO(\d{1,2})$/ISO-8859-$1/;
		
		$iconv_name = $alternates{$Name};
	}
	
	if( !$iconv_name ) { # try crude transformation
		$Name =~ s/[\-\_]+//g;
		$iconv_name = $alternates{$Name};
	}
	
	$name = qq/"$name"/;
	if( $iconv_name ) { 	# found, print element
		$iconv_name = qq/"$iconv_name",/;
		printf OUT "\t$comma { %20s %-15s }\n", $iconv_name , $name;
	} else {		# not found, print comment
		$iconv_name = qq/"",/;
		printf OUT "\t /* %20s %-15s */\n", $iconv_name , $name;

		# grep for similar names, as an aid to the to programmer.  
		$name =~ s/[\-\_]+//g;
		print STDERR $Name.":  $name alternative_character_sets.h\n";
	}
	$comma = ',';
}
print  OUT "\t/* stopper row */\n";
printf OUT "\t$comma { %20s %-15s }\n", 'NULL,' , 'NULL';
close(OUT);



print "/*\n";
$date = localtime;
print " * This file produced from $0 on $date\n";
print ' * $Id: encodings.pl,v 1.12 2010-11-26 19:46:55 freddy77 Exp $', "\n";
print " */\n";

%charsets = ();
$filename = "${srcdir}character_sets.h";
open(IN, "<$filename") or die qq($basename: could not open "$filename"\n);
while(<IN>)
{
	if (/{.*"(.*)".*,\s*([0-9]+)\s*,\s*([0-9]+)\s*}/)
	{
		next if !$1;
		$charsets{$1} = [$2,$3];
	}
}
close(IN);

# from all iconv to canonic
%alternates = ();
$filename = "${srcdir}alternative_character_sets.h";
open(IN, "<$filename") or die qq($basename: could not open "$filename"\n);
while(<IN>)
{
	if (/{\s*"(.+)"\s*,\s*"(.+)"\s*}/)
	{
		$alternates{$2} = $1;
	}
}
close(IN);

# from sybase to canonic
%sybase = ();
$filename = "${srcdir}sybase_character_sets.h";
open(IN, "<$filename") or die qq($basename: could not open "$filename"\n);
while(<IN>)
{
	if (/{\s*"(.+)"\s*,\s*"(.+)"\s*}/)
	{
		$sybase{$2} = $alternates{$1};
	}
}
close(IN);

# give an index to all canonic
%index = ();
$i = 0;
$index{"ISO-8859-1"} = $i++;
$index{"UTF-8"} = $i++;
$index{"UCS-2LE"} = $i++;
$index{"UCS-2BE"} = $i++;
foreach $n (sort grep(!/^(ISO-8859-1|UTF-8|UCS-2LE|UCS-2BE)$/,keys %charsets))
{
	next if exists($index{$n}) || !$n;
	$index{$n} = $i++;
}

print "#ifdef TDS_ICONV_ENCODING_TABLES\n\n";

# output all charset
print "static const TDS_ENCODING canonic_charsets[] = {\n";
$i=0;
foreach $n (sort { $index{$a} <=> $index{$b} } keys %charsets)
{
	my ($a, $b) = @{$charsets{$n}};
	next if !$n;
	printf "\t{%20s,\t$a, $b, %3d},\t/* %3d */\n", qq/"$n"/, $i, $i;
	++$i;
}
die('too much encodings') if $i >= 256;
print "\t{\"\",\t0, 0, 0}\n";
print "};\n\n";

# output all alias
print "static const CHARACTER_SET_ALIAS iconv_aliases[] = {\n";
foreach $n (sort keys %alternates)
{
	$a = $index{$alternates{$n}};
	next if ("$a" eq "");
	printf "\t{%25s,  %3d },\n", qq/"$n"/, $a;
}
print "\t{NULL,\t0}\n";
print "};\n\n";

# output all sybase
print "static const CHARACTER_SET_ALIAS sybase_aliases[] = {\n";
foreach $n (sort keys %sybase)
{
	$a = $index{$sybase{$n}};
	next if ("$a" eq "");
	printf "\t{%20s, %3d },\n", qq/"$n"/, $a;
}
print "\t{NULL,\t0}\n";
print "};\n";

print "#endif\n\n";

# output enumerated charset indexes
print "enum {\n";
$i=0;
foreach $n (sort { $index{$a} <=> $index{$b} } keys %charsets)
{
	$n =~ tr/-a-z/_A-Z/;
	printf "\t%30s =%4d,\n", "TDS_CHARSET_$n", $i++;
}
printf "\t%30s =%4d\n};\n\n", "TDS_NUM_CHARSETS", $i++;

exit 0;
__DATA__
#http://www.sybase.com/detail/1,6904,1016214,00.html
						
ascii_8
big5
big5hk
cp1026
cp1047
cp1140
cp1141
cp1142
cp1143
cp1144
cp1145
cp1146
cp1147
cp1148
cp1149
cp1250
cp1251
cp1252
cp1253
cp1254
cp1255
cp1256
cp1257
cp1258
cp273
cp277
cp278
cp280
cp284
cp285
cp297
cp420
cp424
cp437
cp500
cp5026
cp5026yen
cp5035
cp5035yen
cp737
cp775
cp850
cp852
cp855
cp857
cp858
cp860
cp861
cp862
cp863
cp864
cp865
cp866
cp869
cp870
cp871
cp874
cp874ibm
cp875
cp921
cp923
cp930
cp930yen
cp932
cp932ms
cp933
cp935
cp936
cp937
cp939
cp939yen
cp949
cp950
cp954
deckanji
euccns
eucgb
eucjis
eucksc
greek8
iso10
iso13
iso14
iso15
iso646
iso88592
iso88595
iso88596
iso88597
iso88598
iso88599
iso_1
koi8
mac
mac_cyr
mac_ee
macgrk2
macgreek
macthai
macturk
roman8
roman9
rcsu
sjis
tis620
turkish8
utf8
