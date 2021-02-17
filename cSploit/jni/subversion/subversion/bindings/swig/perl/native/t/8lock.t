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

use Test::More tests => 8;
use strict;
no warnings 'once'; # shut up about variables that are only used once.

require SVN::Core;
require SVN::Repos;
require SVN::Fs;
use File::Path qw(rmtree);
use File::Temp qw(tempdir);
use POSIX qw(locale_h);

setlocale(LC_ALL, "C");

my $repospath = tempdir('svn-perl-test-XXXXXX', TMPDIR => 1, CLEANUP => 1);

my $repos;

ok($repos = SVN::Repos::create("$repospath", undef, undef, undef, undef),
   "create repository at $repospath");

my $fs = $repos->fs;

my $acc = SVN::Fs::create_access('foo');
is($acc->get_username, 'foo');
$fs->set_access($acc);

my $txn = $fs->begin_txn($fs->youngest_rev);
$txn->root->make_file('testfile');
{
my $stream = $txn->root->apply_text('testfile', undef);
print $stream 'orz';
}
$txn->commit;

my $token = "opaquelocktoken:notauuid-$$";

$fs->lock('/testfile', $token, 'we hate software', 0, 0, $fs->youngest_rev, 0);

ok(my $lock = $fs->get_lock('/testfile'));
is($lock->token, $token);
is($lock->owner, 'foo');

$acc = SVN::Fs::create_access('fnord');
is($acc->get_username, 'fnord');
$fs->set_access($acc);

eval {
$fs->lock('/testfile', $token, 'we hate software', 0, 0, $fs->youngest_rev, 0);
};

like($@, qr/already locked/);

eval {
$fs->unlock('/testfile', 'software', 0)
};
like($@, qr/no such lock/);

$fs->unlock('/testfile', 'software', 1);
