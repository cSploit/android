#!/usr/bin/perl -w
# vim:ts=2:sw=2:expandtab
#
# svn-graph.pl - produce a GraphViz .dot graph for the branch history
#                of a node
#
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
#
# View graphs using a command like:
#
#   svn-graph.pl file:///tmp/repos | dotty -
#
# TODO:
#  - Calculate the repository root at runtime so the user can pass
#    the node of interest as a single URL.
#  - (Also?) produce the graphical output ourselves (SVG?) instead
#    of writing a GraphViz ".dot" data file.  This can be done with
#    GraphViz using 'dot'.
#  - Display svnmerge.py/Subversion merge history.
#

use strict;
use Getopt::Std;

# Turn off output buffering
$|=1;

require SVN::Core;
require SVN::Ra;
require SVN::Client;

# The URL of the Subversion repository we wish to graph
# (e.g. "http://svn.apache.org/repos/asf/subversion").
my $repos_url;

# The revision range we operate on, from $startrev -> $youngest.
my $youngest;
my $startrev;

# This is the node we're interested in
my $startpath;

# Set the variables declared above.
parse_commandline();

# Point at the root of a repository so we get can look at
# every revision.
my $auth = (new SVN::Client())->auth;
my $ra = SVN::Ra->new(url => $repos_url, auth => $auth);

# Handle identifier for the aboslutely youngest revision.
if ($youngest eq 'HEAD')
{
  $youngest = $ra->get_latest_revnum();
}

# The "interesting" nodes are potential sources for copies.  This list
#   grows as we move through time.
# The "tracking" nodes are the most recent revisions of paths we're
#   following as we move through time.  If we hit a delete of a path
#   we remove it from the tracking array (i.e. we're no longer interested
#   in it).
my %interesting = ("$startpath:$startrev", 1);
my %tracking = ("$startpath", $startrev);

my %codeline_changes_forward = ();
my %codeline_changes_back = ();
my %copysource = ();
my %copydest = ();

write_graph_descriptor();
#print STDERR "\n";



# Validate the command-line arguments, and set the global variables
# $repos_url, $youngest, $startrev, and $startpath.
sub parse_commandline
{
  my %cmd_opts;
  my $usage = "
usage: svn-graph.pl [-r START_REV:END_REV] [-p PATH] REPOS_URL

  -r the revision range (defaults to 0 through HEAD)
  -p the repository-relative path (defaults to /trunk)
  -h show this help information (other options will be ignored)
";

  # Defaults.
  $cmd_opts{'r'} = '1:HEAD';
  $cmd_opts{'p'} = '/trunk';

  getopts('r:p:h', \%cmd_opts) or die $usage;

  die $usage if scalar(@ARGV) < 1;
  $repos_url = $ARGV[0];

  $cmd_opts{'r'} =~ m/(\d+)(:(.+))?/;
  if ($3)
  {
    $youngest = ($3 eq 'HEAD' ? $3 : int($3));
    $startrev = int($1);
  }
  else
  {
    $youngest = ($3 eq 'HEAD' ? $1 : int($1));
    $startrev = 1;
  }

  $startpath = $cmd_opts{'p'};

  # Print help info (and exit nicely) if requested.
  if ($cmd_opts{'h'})
  {
    print($usage);
    exit 0;
  }
}

# This function is a callback which is invoked for every revision as
# we traverse change log messages.
sub process_revision
{
  my $changed_paths = shift;
  my $revision = shift || '';
  my $author = shift || '';
  my $date = shift || '';
  my $message = shift || '';
  my $pool = shift;

  #print STDERR "$revision\r";

  foreach my $path (keys %$changed_paths)
  {
    my $copyfrom_path = $$changed_paths{$path}->copyfrom_path;
    my $copyfrom_rev = undef;
    my $action = $$changed_paths{$path}->action;

    # See if we're deleting one of our tracking nodes
    if ($action eq 'D' and exists($tracking{$path}))
    {
      print "\t\"$path:$tracking{$path}\" ";
      print "[label=\"$path:$tracking{$path}\\nDeleted in r$revision\",color=red];\n";
      delete($tracking{$path});
      next;
    }

    ### TODO: Display a commit which was the result of a merge
    ### operation with [sytle=dashed,color=blue]

    # If this is a copy, work out if it was from somewhere interesting
    if (defined($copyfrom_path))
    {
      $copyfrom_rev = $tracking{$copyfrom_path};
    }
    if (defined($copyfrom_rev) &&
        exists($interesting{$copyfrom_path . ':' . $copyfrom_rev}))
    {
      $interesting{$path . ':' . $revision} = 1;
      $tracking{$path} = $revision;
      print "\t\"$copyfrom_path:$copyfrom_rev\" -> ";
      print " \"$path:$revision\"";
      print " [label=\"copy at r$revision\",color=green];\n";

      $copysource{"$copyfrom_path:$copyfrom_rev"} = 1;
      $copydest{"$path:$revision"} = 1;
    }

    # For each change, we'll walk up the path one component at a time,
    # updating any parents that we're tracking (i.e. a change to
    # /trunk/asdf/foo updates /trunk).  We mark that parent as
    # interesting (a potential source for copies), draw a link, and
    # update its tracking revision.
    do
    {
      if (exists($tracking{$path}) && $tracking{$path} != $revision)
      {
        $codeline_changes_forward{"$path:$tracking{$path}"} =
          "$path:$revision";
        $codeline_changes_back{"$path:$revision"} =
          "$path:$tracking{$path}";
        $interesting{$path . ':' . $revision} = 1;
        $tracking{$path} = $revision;
      }
      $path =~ s:/[^/]*$::;
    } until ($path eq '');
  }
}

# Write a descriptor for the graph in GraphViz .dot format to stdout.
sub write_graph_descriptor
{
  # Begin writing the graph descriptor.
  print "digraph tree {\n";
  print "\tgraph [bgcolor=white];\n";
  print "\tnode [color=lightblue2, style=filled];\n";
  print "\tedge [color=black, labeljust=r];\n";
  print "\n";

  # Retrieve the requested history.
  $ra->get_log(['/'], $startrev, $youngest, 0, 1, 0, \&process_revision);

  # Now ensure that everything is linked.
  foreach my $codeline_change (keys %codeline_changes_forward)
  {
    # If this node is not the first in its codeline chain, and it isn't
    # the source of a copy, it won't be the source of an edge
    if (exists($codeline_changes_back{$codeline_change}) &&
        !exists($copysource{$codeline_change}))
    {
      next;
    }

    # If this node is the first in its chain, or the source of
    # a copy, then we'll print it, and then find the next in
    # the chain that needs to be printed too
    if (!exists($codeline_changes_back{$codeline_change}) or
         exists($copysource{$codeline_change}) )
    {
      print "\t\"$codeline_change\" -> ";
      my $nextchange = $codeline_changes_forward{$codeline_change};
      my $changecount = 1;
      while (defined($nextchange))
      {
        if (exists($copysource{$nextchange}) or
            !exists($codeline_changes_forward{$nextchange}) )
        {
          print "\"$nextchange\" [label=\"$changecount change";
          if ($changecount > 1)
          {
            print 's';
          }
          print '"];';
          last;
        }
        $changecount++;
        $nextchange = $codeline_changes_forward{$nextchange};
      }
      print "\n";
    }
  }

  # Complete the descriptor (delaying write of font size to avoid
  # inheritance by any subgraphs).
  #my $title = "Family Tree\n$startpath, $startrev through $youngest";
  #print "\tgraph [label=\"$title\", fontsize=18];\n";
  print "}\n";
}
