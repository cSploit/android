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
use Test::More tests => 3;
require SVN::Core;
require SVN::Delta;

my ($srctext, $tgttext, $result) = ('abcd===eflfjgjkx', 'abcd=--ef==lfjffgjx', '');

open my $source, '<', \$srctext;
open my $target, '<', \$tgttext;
open my $aresult, '>', \$result;

my $txstream = SVN::TxDelta::new($source, $target);

isa_ok($txstream, '_p_svn_txdelta_stream_t');
open my $asource, '<', \$srctext;
my ($md5, @handle) = SVN::TxDelta::apply($asource, $aresult, undef);

SVN::TxDelta::send_txstream($txstream, @handle);

is($result, $tgttext, 'delta self test');

is("$md5", 'a22b3dadcbddac48d2f1eae3ec5fb86a', 'md5 matched');
