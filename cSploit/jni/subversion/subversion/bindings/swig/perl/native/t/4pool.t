#!/usr/bin/perl
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

use strict;
use Test::More tests => 6;
use File::Path q(rmtree);
use File::Temp qw(tempdir);

# shut up about variables that are only used once.
# these come from constants and variables used
# by the bindings but not elsewhere in perl space.
no warnings 'once';

require SVN::Core;
require SVN::Repos;
require SVN::Fs;
require SVN::Delta;

package TestEditor;
our @ISA = qw(SVN::Delta::Editor);

sub add_directory {
    my ($self, $path, undef, undef, undef, $pool) = @_;
    $pool->default;
    main::is_pool_default($pool, 'default pool from c calls');
}

package main;
sub is_pool_default {
    my ($pool, $text) = @_;
    is(ref($pool) eq 'SVN::Pool' ? $$$pool : $$pool,
       $$SVN::_Core::current_pool, $text);
}

my $repospath = tempdir('svn-perl-test-XXXXXX', TMPDIR => 1, CLEANUP => 1);

my $repos;

ok($repos = SVN::Repos::create("$repospath", undef, undef, undef, undef),
   "create repository at $repospath");

my $fs = $repos->fs;

my $pool = SVN::Pool->new_default;

is_pool_default($pool, 'default pool');

{
    my $spool = SVN::Pool->new_default_sub;
    is_pool_default($spool, 'lexical default pool default');
}

is_pool_default($pool, 'lexical default pool destroyed');

my $root = $fs->revision_root(0);

my $txn = $fs->begin_txn(0);

$txn->root->make_dir('trunk');

$txn->commit;


SVN::Repos::dir_delta($root, '', '',
                      $fs->revision_root(1), '',
                      TestEditor->new(),
                      undef, 1, 1, 0, 1);


is_pool_default($pool, 'default pool from c calls destroyed');

END {
diag('cleanup');
rmtree($repospath);
}
