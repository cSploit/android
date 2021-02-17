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

use Test::More tests => 22;
use strict;
no warnings 'once'; # shut up about variables that are only used once.

require SVN::Core;
require SVN::Repos;
require SVN::Fs;
use File::Path qw(rmtree);
use File::Temp qw(tempdir);

my $BINARY_DATA = "foo\0\n\t\x1F\x7F\x80\xA0\x{FF}bar";

my $repospath = tempdir('svn-perl-test-XXXXXX', TMPDIR => 1, CLEANUP => 1);

my $repos;

ok($repos = SVN::Repos::create("$repospath", undef, undef, undef, undef),
   "create repository at $repospath");

my $fs = $repos->fs;

cmp_ok($fs->youngest_rev, '==', 0,
       "new repository start with rev 0");

is($fs->path, "$repospath/db", '$fs->path()');
is(SVN::Fs::type($fs->path), 'fsfs', 'SVN::Fs::type()');

my $txn = $fs->begin_txn($fs->youngest_rev);

my $txns = $fs->list_transactions;
ok(eq_array($fs->list_transactions, [$txn->name]), 'list transaction');

isa_ok($txn->root, '_p_svn_fs_root_t', '$txn->root()');
is($txn->root->txn_name, $txn->name, '$txn->root->txn_name()');
is($fs->revision_root($fs->youngest_rev)->txn_name, undef);

$txn->root->make_dir('trunk');

my $path = 'trunk/filea';
my $text = "this is just a test\n";
$txn->root->make_file($path);
{
    my $stream = $txn->root->apply_text($path, undef);
    isa_ok($stream, 'SVN::Stream', '$txn->root->apply_text');
    print $stream $text;
    close $stream;
}
$txn->commit;

cmp_ok($fs->youngest_rev, '==', 1, 'revision increased');

my $root = $fs->revision_root($fs->youngest_rev);

cmp_ok($root->check_path($path), '==', $SVN::Node::file, 'check_path');
ok(!$root->is_dir($path), 'is_dir');
ok($root->is_file($path), 'is_file');
{
    my $stream = $root->file_contents($path);
    local $/;
    is(<$stream>, $text, 'content verified');
    is($root->file_md5_checksum($path), 'dd2314129f81675e95b940ff94ddc935',
       'md5 verified');
}

cmp_ok($root->file_length($path), '==', length($text), 'file_length');

# Revision properties
isa_ok($fs->revision_proplist(1), 'HASH', 'revision_proplist: object');
is($fs->revision_prop(1, 'not:exists'), undef, 'revision_prop: nonexistent');
like($fs->revision_prop(1, 'svn:date'), qr/^\d+-\d+-\d+T\d+:\d+:\d+\.\d+Z$/,
     'revision_prop: svn:date');

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

    $fs->change_rev_prop(1, 'test-prop', 'foo');
    is($fs->revision_prop(1, 'test-prop'), 'foo', 'change_rev_prop');

    $fs->change_rev_prop(1, 'test-prop', undef);
    is($fs->revision_prop(1, 'test-prop'), undef, 'change_rev_prop: deleted');

    $fs->change_rev_prop(1, 'binary-prop', $BINARY_DATA);
    is($fs->revision_prop(1, 'binary-prop'), $BINARY_DATA,
       'change_rev_prop with binary data');
}

END {
diag "cleanup";
rmtree($repospath);
}
