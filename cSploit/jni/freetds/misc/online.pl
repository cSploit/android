#!/usr/bin/perl

$magic = '@!@!@!@!@!@!@!@!@!@!@!@!@!@!@!@';
$result = -1;
$num = '000';

sub data($) {
	my $data = shift;
	if ($started) {
		print OUT "$data\n";
	}
}

sub out_header($) {
	$_ = shift;
        s,  +,</th><th>,g;
	s,^,<table border="1"><tr><th>,;
	s,$,</th></tr>,;
	print OUT "$_\n";
}

sub out_footer() {
	print OUT "</table><br />";
}

sub out_row($) {
	$_ = shift;
	s,  +,</td><td>,g;
	s,^,<tr><td>,;
	s,$,</td></tr>,;
	s,<td>([^<]*):-\)</td>,<td><font color="green">\1</font></td>,g;
	s,<td>([^<]*):-\(</td>,<td bgcolor="red">\1</td>,g;
	s,<td>([^<]*):\(</td>,<td bgcolor="yellow">\1</td>,g;
	print OUT "$_\n";
}


$cur = {};
while (<>) {
	s/\r?\n$//;
	if (/(.*)$magic- ([^ ]+) (.+) -$magic(.*)/) {
		$last = $4;
		data($1) if ($1 ne '');
#		print "$2 - $3\n";
		if ($2 eq 'START') {
			++$num;
			$cur = {
				num => $num,
				name => $3,
				result => -1
			};
			$started = 1;
			open(OUT, ">test$num.txt") or die qq(could not open "test$num.txt" ($!));
		} elsif ($2 eq 'END') {
			close(OUT);
			push @files, $cur;
			# reset state
			$started = 0;
			$cur = {};
		} elsif ($2 eq 'FILE') {
			$cur->{fn} = $3;
		} elsif ($2 eq 'TEST') {
			$cur->{test} = 0 + $3;
		} elsif ($2 eq 'VALGRIND') {
			$cur->{valgrind} = 0 + $3;
		} elsif ($2 eq 'RESULT') {
			$cur->{result} = $3;
		} elsif ($2 eq 'INFO') {
			$title = $msg = '';
			if ($3 =~ /^HOSTNAME (.*)/) {
				$title = 'Hostname';
				$msg = $1;
			} elsif ($3 =~ /^GCC (.*)/) {
				$title = 'gcc version';
				$msg = $1;
			} elsif ($3 =~ /^UNAME (.*)/) {
				$title = 'uname -a';
				$msg = $1;
			} elsif ($3 =~ /^DATE (.*)/) {
				$title = 'date';
				$msg = $1;
			}
			$msg =~ s/^\s*(.*?)\s*$/\1/;
			if ($title && $msg) {
				$msg =~ s,&,&amp;,g;
				$msg =~ s,<,&lt;,g;
				$msg =~ s,>,&gt;,g;
				$info .= "<tr><th>$title</th><td>$msg</td></tr>\n";
			}
		} else {
			if ($3 !~ /compile/ && ($2 ne 'start' || $2 ne 'end')) {
				die("invalid command $2");
			}
		}
		data($last) if ($last ne '');
	} else {
		data($_);
	}
}

$Header = qq|<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd">
<html>
<head>
<title>%1\$s</title>
<style type="text/css">
.error { background-color: red; color: white }
.info  { color: blue }
</style>
</head>
<body>
<h1>%1\$s</h1>
<p><a href="index.html">Main</a></p>
<table border="1">
$info
</table>
<br />
|;

$Footer = q|<p><a href="index.html">Main</a></p>
</body>
</html>
|;

%names = ();
open(OUT, ">index.html") or die qq(could not open "index.html" ($!));
printf OUT $Header, 'Test output';
$vg = 0;
foreach $i (@files) {
	$result = $i->{result};
	$name = $i->{name};
	$fn = $i->{fn};
	$num = $i->{num};

	$title = $name;
	$title = 'make tests' if ($name eq 'maketest');
	if ($name =~ /^\.\// && $i->{test} && $fn =~ m!/ftds_comp.*/src/!) {
		$title = $fn;
		$title =~ s!.*/ftds_comp.*/src/!src/!;
	}
	$i->{title} = $title;
	
	$html = $title;
	$html =~ s/\s+$//s;
	$html =~ s/.exe$//i;
	$html =~ s/.php$/_php/i;
	$html = 'test' if (!$html);
	$html =~ s!/unittests/!_!;
	($html) = $html =~ m/([a-z0-9_]+)$/is;
	$i->{html} = $html.'.'.++$names{$html}.".html";
#	print "name $i->{name}\n\tfn $i->{fn}\n\ttitle $i->{title}\n\thtml $i->{html}\n";

	# make html from txt
	open(HTML, ">$i->{html}") or die qq(could not open "$i->{html}" ($!));
	printf HTML $Header, $title;

	open(IN, "<test$num.txt") or die qq(could not open "test$num.txt" ($!));
	@a = <IN>;
	close(IN);

	$_ = join("", @a);
	@a = ();
	$i->{warning} = 'no :-)';
	$i->{warning} = 'yes :-(' if (/^\+?2:/m);
	if ($i->{valgrind}) {
		$vg = 1;
		$i->{leak} = "no :-)";
		if (!/:==.*no leaks are possible/) {
			$i->{leak} = "yes :-(" if (!/:==.*definitely lost: 0 bytes in 0 blocks/);
			$i->{leak} = "yes :-(" if (!/:==.*possibly lost: 0 bytes in 0 blocks/);
			$i->{leak} = "yes :-(" if (!/:==.*still reachable: 0 bytes in 0 blocks/);
		}
		$i->{vgerr} = "no :-)";
		$i->{vgerr} = "yes :-(" if (!/ERROR SUMMARY: 0 errors from 0 contexts/);
	}
	s,&,&amp;,g; s,<,&lt;,g; s,>,&gt;,g;
	s,\n\+2:([^\n]*),<span class="error">\1</span>,sg;
	s,\n\+3:([^\n]*),<span class="info">\1</span>,sg;
	s,^2:(.*)$,<span class="error">\1</span>,mg;
	s,^3:(.*)$,<span class="info">\1</span>,mg;
	s,^1:,,mg;
	print HTML "<pre>$_</pre>";
	$_ = '';

	printf HTML $Footer;
	close(HTML);

	$i->{success} = $result == 0 ? 'yes :-)' : 'no :-(';
	$i->{warning} = 'ignored' if ($result != 0);
}

out_header('Operation  Success  Warnings  Log');
foreach $i (grep { !$_->{test} } @files) {
	out_row("$i->{title}  $i->{success}  $i->{warning}  <a href=\"$i->{html}\">log</a>");
}
out_footer();
@files = grep { $_->{test} } @files;

if (!$vg) {
	out_header('Test  Success  Warnings  Log');
	foreach $i (@files) {
		$i->{warning} =~ s/:-\(/:\(/;
		out_row("$i->{title}  $i->{success}  $i->{warning}  <a href=\"$i->{html}\">log</a>");
	}
	out_footer();
} else {
	out_header('Test  Success  Warnings  Log  VG Success  VG warnings  VG errors  VG leaks  VG log');
	for ($n=0; $n <= $#files; ++$n) {
		$norm = "unknown  unknown  not present";
		$vg = "unknown  unknown  unknown  unknown  not present";
		$i = $files[$n];
		$i->{warning} =~ s/:-\(/:\(/;
		if ($i->{valgrind}) {
			$vg = "$i->{success}  $i->{warning}  $i->{vgerr}  $i->{leak}  <a href=\"$i->{html}\">log</a>";
			if ($n < $#files && $files[$n+1]->{title} eq $i->{title}) {
				$i = $files[++$n];
			}
		}
		if (!$i->{valgrind}) {
			$norm = "$i->{success}  $i->{warning}  <a href=\"$i->{html}\">log</a>";
		}
		out_row("$i->{title}  $norm  $vg");
	}
	out_footer();
}

printf OUT $Footer;
close(OUT);
