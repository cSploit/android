#!/usr/bin/perl -w
#
#
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.
#
#

use Test::More tests => 6;
use File::Temp qw(tempdir);
use File::Path qw(rmtree);
use strict;

require SVN::Core;
require SVN::Repos;
require SVN::Fs;
require SVN::Delta;
use File::Path;

my $repospath = tempdir('svn-perl-test-XXXXXX', TMPDIR => 1, CLEANUP => 1);

my $repos;

ok($repos = SVN::Repos::create("$repospath", undef, undef, undef, undef),
   "create repository at $repospath");

my $fs = $repos->fs;

sub committed {
    diag "committed ".join(',',@_);
}

my $editor = SVN::Delta::Editor->
    new(SVN::Repos::get_commit_editor($repos, "file://$repospath",
                                      '/', 'root', 'FOO', \&committed));

my $rootbaton = $editor->open_root(0);

my $dirbaton = $editor->add_directory('trunk', $rootbaton, undef, 0);

my $fbaton = $editor->add_file('trunk/filea', $dirbaton, undef, -1);

my $ret = $editor->apply_textdelta($fbaton, undef);

SVN::TxDelta::send_string("FILEA CONTENT", @$ret);

$editor->close_edit();

cmp_ok($fs->youngest_rev, '==', 1);
{
$editor = SVN::Delta::Editor->
    new(SVN::Repos::get_commit_editor($repos, "file://$repospath",
                                      '/', 'root', 'FOO', \&committed));
my $rootbaton = $editor->open_root(1);

my $dirbaton = $editor->add_directory('tags', $rootbaton, undef, 1);
my $subdirbaton = $editor->add_directory('tags/foo', $dirbaton,
                                         "file://$repospath/trunk", 1);

$editor->close_edit();
}
cmp_ok($fs->youngest_rev, '==', 2);

my @history;

SVN::Repos::history($fs, 'tags/foo/filea',
                    sub {push @history, [@_[0,1]]}, 0, 2, 1);

is_deeply(\@history, [['/tags/foo/filea',2],['/trunk/filea',1]],
          'repos_history');

{
my $pool = SVN::Pool->new_default;
my $something = bless {}, 'something';
$editor = SVN::Delta::Editor->
    new(SVN::Repos::get_commit_editor($repos, "file://$repospath",
                                      '/', 'root', 'FOO', sub {committed(@_);
                                                               $something;
                                                          }));

my $rootbaton = $editor->open_root(2);
$editor->delete_entry('tags', 2, $rootbaton);

$editor->close_edit();
}
ok($main::something_destroyed, 'callback properly destroyed');

cmp_ok($fs->youngest_rev, '==', 3);

END {
diag "cleanup";
rmtree($repospath);
}

package something;

sub DESTROY {
    $main::something_destroyed++;
}

1;
