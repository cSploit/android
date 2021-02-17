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

# A script that allows some simple testing of Subversion, in
# particular concurrent read, write and read-write access by the 'svn'
# client. It can also create working copy trees containing a large
# number of files and directories. All repository access is via the
# 'svnadmin' and 'svn' commands.
#
# This script constructs a repository, and populates it with
# files. Then it loops making changes to a subset of the files and
# committing the tree. Thus when two, or more, instances are run in
# parallel there is concurrent read and write access. Sometimes a
# commit will fail due to a commit conflict. This is expected, and is
# automatically resolved by updating the working copy.
#
# Each file starts off containing:
#    A0
#    0
#    A1
#    1
#    A2
#    .
#    .
#    A9
#    9
#
# The script runs with an ID in the range 0-9, and when it modifies a
# file it modifes the line that starts with its ID. Thus scripts with
# different IDs will make changes that can be merged automatically.
#
# The main loop is then:
#
#   step 1: modify a random selection of files
#
#   step 2: optional sleep or wait for RETURN keypress
#
#   step 3: update the working copy automatically merging out-of-date files
#
#   step 4: try to commit, if not successful go to step 3 otherwise go to step 1
#
# To allow break-out of potentially infinite loops, the script will
# terminate if it detects the presence of a "stop file", the path to
# which is specified with the -S option (default ./stop). This allows
# the script to be stopped without any danger of interrupting an 'svn'
# command, which experiment shows may require Berkeley db_recover to
# be used on the repository.
#
#  Running the Script
#  ==================
#
# Use three xterms all with shells on the same directory.  In the
# first xterm run (note, this will remove anything called repostress
# in the current directory)
#
#         % stress.pl -c -s1
#
# When the message "Committed revision 1." scrolls pass use the second
# xterm to run
#
#         % stress.pl -s1
#
# Both xterms will modify, update and commit separate working copies to
# the same repository.
#
# Use the third xterm to touch a file 'stop' to cause the scripts to
# exit cleanly, i.e. without interrupting an svn command.
#
# To run a third, fourth, etc. instance of the script use -i
#
#         % stress.pl -s1 -i2
#         % stress.pl -s1 -i3
#
# Running several instances at once will cause a *lot* of disk
# activity. I have run ten instances simultaneously on a Linux tmpfs
# (RAM based) filesystem -- watching ten xterms scroll irregularly
# can be quite hypnotic!

use strict;
use IPC::Open3;
use Getopt::Std;
use File::Find;
use File::Path;
use File::Spec::Functions;
use Cwd;

# The name of this script, for error messages.
my $stress = 'stress.pl';

# When testing BDB 4.4 and later with DB_RECOVER enabled, the criteria
# for a failed update and commit are a bit looser than otherwise.
my $dbrecover = undef;

# Repository check/create
sub init_repo
  {
    my ( $repo, $create, $no_sync, $fsfs ) = @_;
    if ( $create )
      {
        rmtree([$repo]) if -e $repo;
        my $svnadmin_cmd = "svnadmin create $repo";
        $svnadmin_cmd .= " --fs-type bdb" if not $fsfs;
        $svnadmin_cmd .= " --bdb-txn-nosync" if $no_sync;
        system( $svnadmin_cmd) and die "$stress: $svnadmin_cmd: failed: $?\n";
        open ( CONF, ">>$repo/conf/svnserve.conf")
          or die "$stress: open svnserve.conf: $!\n";
        print CONF "[general]\nanon-access = write\n";
        close CONF or die "$stress: close svnserve.conf: $!\n";
      }
    $repo = getcwd . "/$repo" if not file_name_is_absolute $repo;
    $dbrecover = 1 if -e "$repo/db/__db.register";
    print "$stress: BDB automatic database recovery enabled\n" if $dbrecover;
    return $repo;
  }

# Check-out a working copy
sub check_out
  {
    my ( $url, $options ) = @_;
    my $wc_dir = "wcstress.$$";
    mkdir "$wc_dir", 0755 or die "$stress: mkdir wcstress.$$: $!\n";
    my $svn_cmd = "svn co $url $wc_dir $options";
    system( $svn_cmd ) and die "$stress: $svn_cmd: failed: $?\n";
    return $wc_dir;
  }

# Print status and update. The update is to do any required merges.
sub status_update
  {
    my ( $options, $wc_dir, $wait_for_key, $disable_status,
         $resolve_conflicts ) = @_;
    my $svn_cmd = "svn st -u $options $wc_dir";
    if ( not $disable_status ) {
      print "Status:\n";
      system( $svn_cmd ) and die "$stress: $svn_cmd: failed: $?\n";
    }
    print "Press return to update/commit\n" if $wait_for_key;
    read STDIN, $wait_for_key, 1 if $wait_for_key;
    print "Updating:\n";
    $svn_cmd = "svn up --non-interactive $options $wc_dir";

    # Check for conflicts during the update.  If any exist, we resolve them.
    my $pid = open3(\*UPDATE_WRITE, \*UPDATE_READ, \*UPDATE_ERR_READ,
                    $svn_cmd);
    my @conflicts = ();
    while ( <UPDATE_READ> )
      {
        print;
        s/\r*$//;               # [Windows compat] Remove trailing \r's
        if ( /^C  (.*)$/ )
          {
            push(@conflicts, ($1))
          }
      }

    # Print any errors.
    my $acceptable_error = 0;
    while ( <UPDATE_ERR_READ> )
      {
        print;
        if ($dbrecover)
          {
            s/\r*$//;          # [Windows compat] Remove trailing \r's
            $acceptable_error = 1 if ( /^svn:[ ]
                                       (
                                        bdb:[ ]PANIC
                                        |
                                        DB_RUNRECOVERY
                                       )
                                       /x );
          }
      }

    # Close up the streams.
    close UPDATE_ERR_READ or die "$stress: close UPDATE_ERR_READ: $!\n";
    close UPDATE_WRITE or die "$stress: close UPDATE_WRITE: $!\n";
    close UPDATE_READ or die "$stress: close UPDATE_READ: $!\n";

    # Get commit subprocess exit status
    die "$stress: waitpid: $!\n" if $pid != waitpid $pid, 0;
    die "$stress: unexpected update fail: exit status: $?\n"
      unless $? == 0 or ( $? == 256 and $acceptable_error );

    if ($resolve_conflicts)
      {
        foreach my $conflict (@conflicts)
          {
            $svn_cmd = "svn resolved $conflict";
            system( $svn_cmd ) and die "$stress: $svn_cmd: failed: $?\n";
          }
      }
  }

# Print status, update and commit. The update is to do any required
# merges.  Returns 0 if the commit succeeds and 1 if it fails due to a
# conflict.
sub status_update_commit
  {
    my ( $options, $wc_dir, $wait_for_key, $disable_status,
         $resolve_conflicts ) = @_;
    status_update $options, $wc_dir, $wait_for_key, $disable_status, \
                  $resolve_conflicts;
    print "Committing:\n";
    # Use current time as log message
    my $now_time = localtime;
    # [Windows compat] Must use double quotes for the log message.
    my $svn_cmd = "svn ci $options $wc_dir -m \"$now_time\"";

    # Need to handle the commit carefully. It could fail for all sorts
    # of reasons, but errors that indicate a conflict are "acceptable"
    # while other errors are not.  Thus there is a need to check the
    # return value and parse the error text.
    my $pid = open3(\*COMMIT_WRITE, \*COMMIT_READ, \*COMMIT_ERR_READ,
                    $svn_cmd);
    print while ( <COMMIT_READ> );

    # Look for acceptable errors, ones we expect to occur due to conflicts
    my $acceptable_error = 0;
    while ( <COMMIT_ERR_READ> )
      {
        print;
        s/\r*$//;               # [Windows compat] Remove trailing \r's
        $acceptable_error = 1 if ( /^svn:[ ]
                                   (
                                    .*out[ ]of[ ]date
                                    |
                                    Conflict[ ]at
                                    |
                                    Baseline[ ]incorrect
                                    |
                                   )
                                   /ix )
            or ( $dbrecover and  ( /^svn:[ ]
                                   (
                                    bdb:[ ]PANIC
                                    |
                                    DB_RUNRECOVERY
                                   )
                                   /x ));


      }
    close COMMIT_ERR_READ or die "$stress: close COMMIT_ERR_READ: $!\n";
    close COMMIT_WRITE or die "$stress: close COMMIT_WRITE: $!\n";
    close COMMIT_READ or die "$stress: close COMMIT_READ: $!\n";

    # Get commit subprocess exit status
    die "$stress: waitpid: $!\n" if $pid != waitpid $pid, 0;
    die "$stress: unexpected commit fail: exit status: $?\n"
      if ( $? != 0 and $? != 256 ) or ( $? == 256 and $acceptable_error != 1 );

    return $? == 256 ? 1 : 0;
  }

# Get a list of all versioned files in the working copy
{
  my @get_list_of_files_helper_array;
  sub GetListOfFilesHelper
    {
      $File::Find::prune = 1 if $File::Find::name =~ m[/.svn];
      return if $File::Find::prune or -d;
      push @get_list_of_files_helper_array, $File::Find::name;
    }
  sub GetListOfFiles
    {
      my ( $wc_dir ) = @_;
      @get_list_of_files_helper_array = ();
      find( \&GetListOfFilesHelper, $wc_dir);
      return @get_list_of_files_helper_array;
    }
}

# Populate a working copy
sub populate
  {
    my ( $dir, $dir_width, $file_width, $depth, $pad, $props ) = @_;
    return if not $depth--;

    for my $nfile ( 1..$file_width )
      {
        my $filename = "$dir/foo$nfile";
        open( FOO, ">$filename" ) or die "$stress: open $filename: $!\n";

        for my $line ( 0..9 )
          {
            print FOO "A$line\n$line\n"
                or die "$stress: write to $filename: $!\n";
            map { print FOO $_ x 255, "\n"; } ("a", "b", "c", "d")
              foreach (1..$pad);
          }
        print FOO "\$HeadURL: \$\n"
            or die "$stress: write to $filename: $!\n" if $props;
        close FOO or die "$stress: close $filename: $!\n";

        my $svn_cmd = "svn add $filename";
        system( $svn_cmd ) and die "$stress: $svn_cmd: failed: $?\n";

        if ( $props )
          {
            $svn_cmd = "svn propset svn:eol-style native $filename";
            system( $svn_cmd ) and die "$stress: $svn_cmd: failed: $?\n";

            $svn_cmd = "svn propset svn:keywords HeadURL $filename";
            system( $svn_cmd ) and die "$stress: $svn_cmd: failed: $?\n";
          }
      }

    if ( $depth )
      {
        for my $ndir ( 1..$dir_width )
          {
            my $dirname = "$dir/bar$ndir";
            my $svn_cmd = "svn mkdir $dirname";
            system( $svn_cmd ) and die "$stress: $svn_cmd: failed: $?\n";

            populate( "$dirname", $dir_width, $file_width, $depth, $pad,
                      $props );
          }
      }
  }

# Modify a versioned file in the working copy
sub ModFile
  {
    my ( $filename, $mod_number, $id ) = @_;

    # Read file into memory replacing the line that starts with our ID
    open( FOO, "<$filename" ) or die "$stress: open $filename: $!\n";
    my @lines = map { s[(^$id.*)][$1,$mod_number]; $_ } <FOO>;
    close FOO or die "$stress: close $filename: $!\n";

    # Write the memory back to the file
    open( FOO, ">$filename" ) or die "$stress: open $filename: $!\n";
    print FOO or die "$stress: print $filename: $!\n" foreach @lines;
    close FOO or die "$stress: close $filename: $!\n";
  }

sub ParseCommandLine
  {
    my %cmd_opts;
    my $usage = "
usage: stress.pl [-cdfhprW] [-i num] [-n num] [-s secs] [-x num] [-o options]
                 [-D num] [-F num] [-N num] [-P num] [-R path] [-S path]
                 [-U url]

where
  -c cause repository creation
  -d don't make the status calls
  -f use --fs-type fsfs during repository creation
  -h show this help information (other options will be ignored)
  -i the ID (valid IDs are 0 to 9, default is 0 if -c given, 1 otherwise)
  -n the number of sets of changes to commit
  -p add svn:eol-style and svn:keywords properties to the files
  -r perform update-time conflict resolution
  -s the sleep delay (-1 wait for key, 0 none)
  -x the number of files to modify in each commit
  -o options to pass for subversion client
  -D the number of sub-directories per directory in the tree
  -F the number of files per directory in the tree
  -N the depth of the tree
  -P the number of 10K blocks with which to pad the file
  -R the path to the repository
  -S the path to the file whose presence stops this script
  -U the URL to the repository (file:///<-R path> by default)
  -W use --bdb-txn-nosync during repository creation
";

    # defaults
    $cmd_opts{'D'} = 2;            # number of subdirs per dir
    $cmd_opts{'F'} = 2;            # number of files per dir
    $cmd_opts{'N'} = 2;            # depth
    $cmd_opts{'P'} = 0;            # padding blocks
    $cmd_opts{'R'} = "repostress"; # repository name
    $cmd_opts{'S'} = "stop";       # path of file to stop the script
    $cmd_opts{'U'} = "none";       # URL
    $cmd_opts{'W'} = 0;            # create with --bdb-txn-nosync
    $cmd_opts{'c'} = 0;            # create repository
    $cmd_opts{'d'} = 0;            # disable status
    $cmd_opts{'f'} = 0;            # create with --fs-type fsfs
    $cmd_opts{'h'} = 0;            # help
    $cmd_opts{'i'} = 0;            # ID
    $cmd_opts{'n'} = 200;          # sets of changes
    $cmd_opts{'p'} = 0;            # add file properties
    $cmd_opts{'r'} = 0;            # conflict resolution
    $cmd_opts{'s'} = -1;           # sleep interval
    $cmd_opts{'x'} = 4;            # files to modify
    $cmd_opts{'o'} = "";           # no options passed

    getopts( 'cdfhi:n:prs:x:o:D:F:N:P:R:S:U:W', \%cmd_opts ) or die $usage;

    # print help info (and exit nicely) if requested
    if ( $cmd_opts{'h'} )
      {
        print( $usage );
        exit 0;
      }

    # default ID if not set
    $cmd_opts{'i'} = 1 - $cmd_opts{'c'} if not $cmd_opts{'i'};
    die $usage if $cmd_opts{'i'} !~ /^[0-9]$/;

    return %cmd_opts;
  }

############################################################################
# Main

# Why the fixed seed?  I use this script for more than stress testing,
# I also use it to create test repositories.  When creating a test
# repository, while I don't care exactly which files get modified, I
# find it useful for the repositories to be reproducible, i.e. to have
# the same files modified each time.  When using this script for
# stress testing one could remove this fixed seed and Perl will
# automatically use a pseudo-random seed.  However it doesn't much
# matter, the stress testing really depends on the real-time timing
# differences between mutiple instances of the script, rather than the
# randomness of the chosen files.
srand 123456789;

my %cmd_opts = ParseCommandLine();

my $repo = init_repo( $cmd_opts{'R'}, $cmd_opts{'c'}, $cmd_opts{'W'},
                      $cmd_opts{'f'} );

# [Windows compat]
# Replace backslashes in the path, and tweak the number of slashes
# in the scheme separator to make the URL always correct.
my $urlsep = ($repo =~ m/^\// ? '//' : '///');
$repo =~ s/\\/\//g;

# Make URL from path if URL not explicitly specified
$cmd_opts{'U'} = "file:$urlsep$repo" if $cmd_opts{'U'} eq "none";

my $wc_dir = check_out $cmd_opts{'U'}, $cmd_opts{'o'};

if ( $cmd_opts{'c'} )
  {
    my $svn_cmd = "svn mkdir $wc_dir/trunk";
    system( $svn_cmd ) and die "$stress: $svn_cmd: failed: $?\n";
    populate( "$wc_dir/trunk", $cmd_opts{'D'}, $cmd_opts{'F'}, $cmd_opts{'N'},
              $cmd_opts{'P'}, $cmd_opts{'p'} );
    status_update_commit $cmd_opts{'o'}, $wc_dir, 0, 1
        and die "$stress: populate checkin failed\n";
  }

my @wc_files = GetListOfFiles $wc_dir;
die "$stress: not enough files in repository\n"
    if $#wc_files + 1 < $cmd_opts{'x'};

my $wait_for_key = $cmd_opts{'s'} < 0;

my $stop_file = $cmd_opts{'S'};

for my $mod_number ( 1..$cmd_opts{'n'} )
  {
    my @chosen;
    for ( 1..$cmd_opts{'x'} )
      {
        # Extract random file from list and modify it
        my $mod_file = splice @wc_files, int rand $#wc_files, 1;
        ModFile $mod_file, $mod_number, $cmd_opts{'i'};
        push @chosen, $mod_file;
      }
    # Reinstate list of files, the order doesn't matter
    push @wc_files, @chosen;

    if ( $cmd_opts{'x'} > 0 ) {
      # Loop committing until successful or the stop file is created
      1 while not -e $stop_file
        and status_update_commit $cmd_opts{'o'}, $wc_dir, $wait_for_key, \
                                 $cmd_opts{'d'}, $cmd_opts{'r'};
    } else {
      status_update $cmd_opts{'o'}, $wc_dir, $wait_for_key, $cmd_opts{'d'}, \
                    $cmd_opts{'r'};
    }

    # Break out of loop, or sleep, if required
    print( "stop file '$stop_file' detected\n" ), last if -e $stop_file;
    sleep $cmd_opts{'s'} if $cmd_opts{'s'} > 0;
  }

