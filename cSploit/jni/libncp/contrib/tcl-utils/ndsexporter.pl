#!/usr/bin/perl

#    ndsexporter - print a list of NDS properties for a full NDS context
#    Copyright (C) 2002 by Patrick Pollet

#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.

#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.

#    You should have received a copy of the GNU General Public License
#    along with this program; if not, write to the Free Software
#    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


#    Revision history:

#        1.00 2002   June 06             Patrick Pollet <patrick.pollet@insa-lyon.fr>
#                the program is an Perl script wrapper around ncplist to get a list of objects of a NDS class
#		and ncpreadprops to retrieve a liste of NDS properties for one user
#
# exemples of uses
#./ndsexporter.pl  -C PC -c "group"		get the list of NDS groups in context PC
# ./ndsexporter.pl  -C PC -c "volume" -A "cn,Host Server,Host Resource Name"	get the selected properties for volumes in context PC
#./ndsexporter.pl  -C PC -c "organizational unit"  -A "ou"  get the list of NDS OU in context PC
#./ndsexporter.pl  -C PC -c "group" -A "cn,member"  -m "/" |grep 2a   get all members of group 2a.PC separated with a :



use Getopt::Std;

getopts("T:C:A:c:m:v:f:n:h",\%options);

if ($options{'h'}) {
	usage();
	exit (0);
}

$DEFTREE="INSA_ROOT";
$DEFCONTEXT="[root]";
$DEFSEARCH="cn";
$DEFCLASS="user";

$DEFCTN=99999999999;

$tree    = $options {'T'} ? $options{'T'}:$DEFTREE;
$ctx     = $options {'C'} ? $options{'C'}:$DEFCONTEXT;
$search  = $options {'A'} ? $options{'A'}:$DEFSEARCH;
$class    =$options {'c'} ? $options {'c'}:$DEFCLASS;
$sepv     =$options {'m'} ? $options {'m'}:" ";
$sepa     =$options {'f'} ? $options {'f'}:":";
$flags    =$options {'v'} ? $options {'v'}:"4";
$cnt      =$options {'n'} ? $options {'n'}:$DEFCNT;
$TMPFILE1="/tmp/nw2ldap.tmp1";


system("ncplist -v 4 -o \"$ctx\"  -T \"$tree\" -Q  -l \"$class\"  |sort >$TMPFILE1");

open (F1,$TMPFILE1) || die ("$TMPFILE1 not found");
while (<F1>) {
  chomp;
  system("ncpreadprops -T \"$tree\" -o  \"$_\" -f \"$sepa\" -m \"$sepv\" -A \"$search\" -v $flags ");
  #print;
  $cnt--;
  last if (! $cnt);

}
close(F1);
unlink($TMPFILE1);
exit (0);

sub usage () {
  print <<EOP
usage: ndsexporter.pl [options]
-h  Print this help text
-c  class of objects to list (default "user")
-A  attribute_list  One NDS attribute or a quoted & comma separated list (default "cn")
-T  treename
-C  context        Default= root
-f  character      separator between attributes (default= :)
-m  character      separator between values of multi-valued attributes (default = space)
-v  number         Context DCK Flags (default 4)
-n  number         Limit ouput to n lines (testing)
EOP
}
