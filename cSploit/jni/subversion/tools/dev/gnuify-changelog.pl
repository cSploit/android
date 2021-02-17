#!/usr/bin/perl -w
# ====================================================================
#    Licensed to the Apache Software Foundation (ASF) under one
#    or more contributor license agreements.  See the NOTICE file
#    distributed with this work for additional information
#    regarding copyright ownership.  The ASF licenses this file
#    to you under the Apache License, Version 2.0 (the
#    "License"); you may not use this file except in compliance
#    with the License.  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing,
#    software distributed under the License is distributed on an
#    "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
#    KIND, either express or implied.  See the License for the
#    specific language governing permissions and limitations
#    under the License.
# ====================================================================
# a script to munge the output of 'svn log' into something approaching the
# style of a GNU ChangeLog.
#
# to use this, just fill in the 'hackers' hash with the usernames and
# name/emails of the people who work on your project, go to the top level
# of your working copy, and run:
#
# $ svn log | /path/to/gnuify-changelog.pl > ChangeLog

require 5.0;
use strict;

my %hackers = (
    "svn"          => 'Collab.net Subversion Team',
    "jimb"         => 'Jim Blandy  <jimb@redhat.com>',
    "sussman"      => 'Ben Collins-Sussman  <sussman@collab.net>',
    "kfogel"       => 'Karl Fogel  <kfogel@collab.net>',
    "gstein"       => 'Greg Stein  <gstein@lyra.org>',
    "brane"        => 'Branko Cibej  <brane@xbc.nu>',
    "joe"          => 'Joe Orton  <joe@light.plus.com>',
    "ghudson"      => 'Greg Hudson  <ghudson@mit.edu>',
    "lefty"        => 'Lee P. W. Burgess  <lefty@red-bean.com>',
    "fitz"         => 'Brian Fitzpatrick  <fitz@red-bean.com>',
    "mab"          => 'Matthew Braithwaite  <matt@braithwaite.net>',
    "daniel"       => 'Daniel Stenberg  <daniel@haxx.se>',
    "mmurphy"      => 'Mark Murphy  <mmurphy@collab.net>',
    "cmpilato"     => 'C. Michael Pilato  <cmpilato@collab.net>',
    "kevin"        => 'Kevin Pilch-Bisson  <kevin@pilch-bisson.net>',
    "philip"       => 'Philip Martin  <philip@codematters.co.uk>',
    "jerenkrantz"  => 'Justin Erenkrantz  <jerenkrantz@apache.org>',
    "rooneg"       => 'Garrett Rooney  <rooneg@electricjellyfish.net>',
    "bcollins"     => 'Ben Collins  <bcollins@debian.org>',
    "blair"        => 'Blair Zajac  <blair@orcaware.com>',
    "striker"      => 'Sander Striker  <striker@apache.org>',
    "XelaRellum"   => 'Alexander Mueller  <alex@littleblue.de>',
    "yoshiki"      => 'Yoshiki Hayashi  <yoshiki@xemacs.org>',
    "david"        => 'David Summers  <david@summersoft.fay.ar.us>',
    "rassilon"     => 'Bill Tutt  <rassilon@lyra.org>',
    "kbohling"     => 'Kirby C. Bohling  <kbohling@birddog.com>',
    "breser"       => 'Ben Reser  <ben@reser.org>',
    "bliss"        => 'Tobias Ringstrom  <tobias@ringstrom.mine.nu>',
    "dionisos"     => 'Erik Huelsmann  <e.huelsmann@gmx.net>',
    "josander"     => 'Jostein Andersen  <jostein@josander.net>',
    "julianfoad"   => 'Julian Foad  <julianfoad@btopenworld.com>',
    "clkao"        => 'Chia-Liang Kao  <clkao@clkao.org>',
    "xsteve"       => 'Stefan Reichör  <reichoer@web.de>',
    "mbk"          => 'Mark Benedetto King  <mbk@lowlatency.com>',
    "patrick"      => 'Patrick Mayweg  <mayweg@qint.de>',
    "jrepenning"   => 'Jack Repenning  <jrepenning@collab.net>',
    "epg"          => 'Eric Gillespie  <epg@pretzelnet.org>',
    "dwhedon"      => 'David Kimdon  <David_Kimdon@alumni.hmc.edu>',
    "djh"          => 'D.J. Heap  <dj@shadyvale.net>',
    "mprice"       => 'Michael Price  <mprice@atl.lmco.com>',
    "jszakmeister" => 'John Szakmeister  <john@szakmeister.net>',
    "bdenny"       => 'Brian Denny  <brian@briandenny.net>',
    "rey4"         => 'Russell Yanofsky  <rey4@columbia.edu>',
    "maxb"         => 'Max Bowsher  <maxb@ukf.net>',
    "dlr"          => 'Daniel Rall  <dlr@finemaltcoding.com>',
    "jaa"          => 'Jani Averbach  <jaa@iki.fi>',
    "pll"          => 'Paul Lussier  <p.lussier@comcast.net>',
    "shlomif"      => 'Shlomi Fish  <shlomif@vipe.technion.ac.il>',
    "jpieper"      => 'Josh Pieper  <jpieper@andrew.cmu.edu>',
    "dimentiy"     => 'Dmitriy O. Popkov  <dimentiy@dimentiy.info>',
    "kellin"       => 'Shamim Islam  <files@poetryunlimited.com>',
    "sergeyli"     => 'Sergey A. Lipnevich  <sergey@optimaltec.com>',
    "kraai"        => 'Matt Kraai  <kraai@alumni.cmu.edu>',
    "ballbach"     => 'Michael Ballbach  <ballbach@rten.net>',
    "kon"          => 'Kalle Olavi Niemitalo  <kon@iki.fi>',
    "knacke"       => 'Kai Nacke  <kai.nacke@redstar.de>',
    "gthompson"    => 'Glenn A. Thompson  <gthompson@cdr.net>',
    "jespersm"     => 'Jesper Steen Møller  <jesper@selskabet.org>',
    "naked"        => 'Nuutti Kotivuori  <naked@iki.fi>',
    "niemeyer"     => 'Gustavo Niemeyer  <niemeyer@conectiva.com>',
    "trow"         => 'Jon Trowbridge  <trow@ximian.com>',
    "mmacek"       => 'Marko Macek  <Marko.Macek@gmx.net>',
    "zbrown"       => 'Zack Brown  <zbrown@tumblerings.org>',
    "morten"       => 'Morten Ludvigsen  <morten@2ps.dk>',
    "fmatias"      => 'Féliciano Matias  <feliciano.matias@free.fr>',
    "nsd"          => 'Nick Duffek  <nick@duffek.com>',
);

my $parse_next_line = 0;
my $last_line_empty = 0;
my $last_rev = "";

while (my $entry = <>) {

  # Axe windows style line endings, since we should try to be consistent, and
  # the repos has both styles in its log entries
  $entry =~ s/\r\n$/\n/;

  # Remove trailing whitespace
  $entry =~ s/\s+$/\n/;

  my $this_line_empty = $entry eq "\n";

  # Avoid duplicate empty lines
  next if $this_line_empty and $last_line_empty;

  # Don't fail on valid dash-only lines
  if ($entry =~ /^-+$/ and length($entry) >= 72) {

    # We're at the start of a log entry, so we need to parse the next line
    $parse_next_line = 1;

    # Check to see if the final line of the commit message was blank,
    # if not insert one
    print "\n" if $last_rev ne "" and !$last_line_empty;

  } elsif ($parse_next_line) {

    # Transform from svn style to GNU style
    $parse_next_line = 0;

    my @parts = split (/ /, $entry);
    $last_rev  = $parts[0];
    my $hacker = $parts[2];
    my $tstamp = $parts[4];

    # Use alias if we can't resolve to name, email
    $hacker = $hackers{$hacker} if defined $hackers{$hacker};

    printf "%s  %s\n", $tstamp, $hacker;

  } elsif ($this_line_empty) {

    print "\n";

  } else {

    print "\t$entry";

  }

  $last_line_empty = $this_line_empty;
}

# As a HERE doc so it also sets the final changelog's coding
print <<LOCAL;
;; Local Variables:
;; coding: utf-8
;; End:
LOCAL

1;
