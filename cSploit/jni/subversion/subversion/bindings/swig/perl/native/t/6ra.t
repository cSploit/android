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

use Test::More tests => 40;
use File::Temp qw(tempdir);
use File::Path qw(rmtree);
use strict;

use SVN::Core;
use SVN::Repos;
use SVN::Ra;
use SVN::Fs;
use SVN::Delta;

my $BINARY_DATA = "foo\0\n\t\x1F\x7F\x80\xA0\x{FF}bar";

my $repospath = tempdir('svn-perl-test-XXXXXX', TMPDIR => 1, CLEANUP => 1);

my $repos;
ok($repos = SVN::Repos::create("$repospath", undef, undef, undef, undef),
   "create repository at $repospath");

# r1: create a dir, a file, and add some properties
my $fs = $repos->fs;
my $txn = $fs->begin_txn($fs->youngest_rev);
$txn->root->make_dir('trunk');
$txn->root->change_node_prop('trunk', 'dir-prop', 'frob');
$txn->root->make_file('trunk/filea');
$txn->root->change_node_prop('trunk/filea', 'test-prop', 'foo');
$txn->root->change_node_prop('trunk/filea', 'binary-prop', $BINARY_DATA);
$txn->root->make_file('trunk/fileb');
$txn->commit;

# r2: remove a property
$txn = $fs->begin_txn($fs->youngest_rev);
$txn->root->change_node_prop('trunk/filea', 'test-prop', undef);
$txn->commit;

my $uri = $repospath;
$uri =~ s{^|\\}{/}g if $^O eq 'MSWin32';
$uri = "file://$uri";

{
    my $ra = SVN::Ra->new($uri);
    isa_ok($ra, 'SVN::Ra', 'create with only one argument');
}
my $ra = SVN::Ra->new(url => $uri);
isa_ok($ra, 'SVN::Ra', 'create with hash param');

is($ra->get_uuid, $fs->get_uuid, 'get_uuid');
is($ra->get_latest_revnum, 2, 'get_latest_revnum');
is($ra->get_repos_root, $uri, 'get_repos_root');

# get_dir
{
    my ($dirents, $revnum, $props) = $ra->get_dir('trunk',
                                                  $SVN::Core::INVALID_REVNUM);
    isa_ok($dirents, 'HASH', 'get_dir: dirents');
    is(scalar(keys %$dirents), 2, 'get_dir: num dirents');
    isa_ok($dirents->{$_}, '_p_svn_dirent_t', "get_dir: dirent $_")
        for qw( filea fileb );
    is($revnum, $ra->get_latest_revnum, 'get_dir: revnum');
    isa_ok($props, 'HASH', 'get_dir: props');
    is($props->{'dir-prop'}, 'frob', 'get_dir: property dir-prop');
}

# get_file
{
    my ($revnum, $props) = $ra->get_file('trunk/filea',
                                         $SVN::Core::INVALID_REVNUM, undef);
    is($revnum, $ra->get_latest_revnum, 'get_file: revnum');
    isa_ok($props, 'HASH', 'get_file: props');
    ok(!exists $props->{'test-prop'}, 'get_file: property test-prop deleted');
    is($props->{'binary-prop'}, $BINARY_DATA, 'get_file: property binary-prop');
}

# Revision properties
isa_ok($ra->rev_proplist(1), 'HASH', 'rev_proplist: object');
is($ra->rev_prop(1, 'nonexistent'), undef, 'rev_prop: nonexistent');
like($ra->rev_prop(1, 'svn:date'), qr/^\d+-\d+-\d+T\d+:\d+:\d+\.\d+Z$/,
     'rev_prop: svn:date');

# To create a revision property is a bit more difficult, because we have
# to set up a 'pre-revprop-change' hook script.  These tests are skipped
# on systems on which I don't know how to do that.
SKIP: {
    skip "don't know how to create 'pre-revprop-change' hook script on $^O", 3
        if $^O eq 'MSWin32';

    my $script_filename = "$repospath/hooks/pre-revprop-change";
    open my $script, '>', $script_filename
        or die "error creating hook script '$script_filename': $!";
    print $script "#!/bin/sh\nexit 0\n";
    close $script;
    chmod 0755, $script_filename
        or die "error making hook script '$script_filename' executable: $!";

    $ra->change_rev_prop(1, 'test-prop', 'foo');
    is($ra->rev_prop(1, 'test-prop'), 'foo', 'change_rev_prop');

    $ra->change_rev_prop(1, 'test-prop', undef);
    is($ra->rev_prop(1, 'test-prop'), undef, 'change_rev_prop: deleted');

    $ra->change_rev_prop(1, 'binary-prop', $BINARY_DATA);
    is($ra->rev_prop(1, 'binary-prop'), $BINARY_DATA,
       'change_rev_prop with binary data');
}

# Information about nodes in the filesystem.
is($ra->check_path('trunk', 1), $SVN::Node::dir, 'check_path');
{
    my $dirent = $ra->stat('trunk', 1);
    isa_ok($dirent, '_p_svn_dirent_t', 'stat dir: dirent object');
    is($dirent->kind, $SVN::Node::dir, 'stat dir: kind');
    is($dirent->size, 0, 'stat dir: size');
    is($dirent->created_rev, 1, 'stat dir: created_rev');
    ok($dirent->has_props, 'stat dir: has_props');

    $dirent = $ra->stat('trunk/fileb', 1);
    is($dirent->kind, $SVN::Node::file, 'stat file: kind');
    ok(!$dirent->has_props, 'stat file: has_props');
}

# do_update
my $ed = MockEditor->new;
my $reporter = $ra->do_update(2, '', 1, $ed);
isa_ok($reporter, 'SVN::Ra::Reporter');
$reporter->set_path('', 0, 1, undef);
$reporter->finish_report;

is($ed->{_base_revnum}, 0, 'do_update: base_revision');
is($ed->{_target_revnum}, 2, 'do_update: target_revnum');
is($ed->{trunk}{props}{'dir-prop'}, 'frob', 'do_update: dir-prop');
ok(!exists $ed->{'trunk/filea'}{props}{'test-prop'},
   'do_update: deleted property');
is($ed->{'trunk/filea'}{props}{'binary-prop'}, $BINARY_DATA,
   'do_update: binary-prop');

# replay
$ed = MockEditor->new;
$ra->replay(1, 0, 1, $ed);
is($ed->{trunk}{type}, 'dir', "replay: got trunk");
is($ed->{trunk}{props}{'dir-prop'}, 'frob', 'replay: dir-prop');
is($ed->{'trunk/filea'}{props}{'binary-prop'}, $BINARY_DATA,
   'replay: binary-prop');

END {
diag "cleanup";
rmtree($repospath);
}


package MockEditor;

sub new { bless {}, shift }

sub set_target_revision {
    my ($self, $revnum) = @_;
    $self->{_target_revnum} = $revnum;
}

sub delete_entry {
    my ($self, $path) = @_;
    die "delete_entry called";
}

sub add_directory {
    my ($self, $path, $baton) = @_;
    return $self->{$path} = { type => 'dir' };
}

sub open_root {
    my ($self, $base_revision, $dir_pool) = @_;
    $self->{_base_revnum} = $base_revision;
    return $self->{_root} = { type => 'root' };
}

sub open_directory {
    my ($self, $path) = @_;
    die "open_directory on file" unless $self->{$path}{type} eq 'dir';
    return $self->{$path};
}

sub open_file {
    my ($self, $path) = @_;
    die "open_file on directory" unless $self->{$path}{type} eq 'file';
    return $self->{$path};
}

sub change_dir_prop {
    my ($self, $baton, $name, $value) = @_;
    $baton->{props}{$name} = $value;
}

sub change_file_prop {
    my ($self, $baton, $name, $value) = @_;
    $baton->{props}{$name} = $value;
}

sub absent_directory {
    my ($self, $path) = @_;
    die "absent_directory called";
}

sub absent_file {
    my ($self, $path) = @_;
    die "absent_file called";
}

sub close_directory {
    my ($self, $baton) = @_;
}

sub close_file {
    my ($self, $baton) = @_;
}

sub add_file {
    my ($self, $path, $baton) = @_;
    return $self->{$path} = { type => 'file' };
}

sub apply_textdelta {
    my ($self, $baton, $base_checksum, $pool) = @_;
    open my $out_fh, '>', \$baton->{data}
        or die "error opening in-memory file to store Subversion update: $!";
    open my $in_fh, '<', \''
        or die "error opening in-memory file for delta source: $!";
    return [ SVN::TxDelta::apply($in_fh, $out_fh, undef, "$baton", $pool) ];
}

sub close_edit {
    my ($self, $pool) = @_;
}

sub abort_edit {
    my ($self, $pool) = @_;
    die "abort_edit called";
}
