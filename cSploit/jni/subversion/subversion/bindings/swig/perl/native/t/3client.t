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

use Test::More tests => 119;
use strict;

# shut up about variables that are only used once.
# these come from constants and variables used
# by the bindings but not elsewhere in perl space.
no warnings 'once';

use_ok('SVN::Core');
use_ok('SVN::Repos');
use_ok('SVN::Client');
use_ok('SVN::Wc'); # needed for status
use File::Spec::Functions;
use File::Temp qw(tempdir);
use File::Path qw(rmtree);

# do not use cleanup because it will fail, some files we
# will not have write perms to.
my $testpath = tempdir('svn-perl-test-XXXXXX', TMPDIR => 1, CLEANUP => 0);

my $repospath = catdir($testpath,'repo');
my $reposurl = 'file://' . (substr($repospath,0,1) ne '/' ? '/' : '')
               . $repospath;
my $wcpath = catdir($testpath,'wc');
my $importpath = catdir($testpath,'import');

# track current rev ourselves to test against
my $current_rev = 0;

# We want to trap errors ourself
$SVN::Error::handler = undef;

# Get username we are running as
my $username = getpwuid($>);

# This is ugly to create the test repo with SVN::Repos, but
# it seems to be the most reliable way.
ok(SVN::Repos::create("$repospath", undef, undef, undef, undef),
   "create repository at $repospath");

my ($ctx) = SVN::Client->new;
isa_ok($ctx,'SVN::Client','Client Object');

my $uuid_from_url = $ctx->uuid_from_url($reposurl);
ok($uuid_from_url,'Valid return from uuid_from_url method form');

# test non method invocation passing a SVN::Client
ok(SVN::Client::uuid_from_url($reposurl,$ctx),
   'Valid return from uuid_from_url function form with SVN::Client object');

# test non method invocation passing a _p_svn_client_ctx_t
ok(SVN::Client::uuid_from_url($reposurl,$ctx->{'ctx'}),
   'Valid return from uuid_from_url function form with _p_svn_client_ctx object');


my ($ci_dir1) = $ctx->mkdir(["$reposurl/dir1"]);
isa_ok($ci_dir1,'_p_svn_client_commit_info_t');
$current_rev++;
is($ci_dir1->revision,$current_rev,"commit info revision equals $current_rev");



my ($rpgval,$rpgrev) = $ctx->revprop_get('svn:author',$reposurl,$current_rev);
is($rpgval,$username,'svn:author set to expected username from revprop_get');
is($rpgrev,$current_rev,'Returned revnum of current rev from revprop_get');

SKIP: {
    skip 'Difficult to test on Win32', 3 if $^O eq 'MSWin32';

    ok(rename("$repospath/hooks/pre-revprop-change.tmpl",
              "$repospath/hooks/pre-revprop-change"),
       'Rename pre-revprop-change hook');
    ok(chmod(0700,"$repospath/hooks/pre-revprop-change"),
       'Change permissions on pre-revprop-change hook');

    my ($rps_rev) = $ctx->revprop_set('svn:log','mkdir dir1',
                                      $reposurl, $current_rev, 0);
    is($rps_rev,$current_rev,
       'Returned revnum of current rev from revprop_set');

}

my ($rph, $rplrev) = $ctx->revprop_list($reposurl,$current_rev);
isa_ok($rph,'HASH','Returned hash reference form revprop_list');
is($rplrev,$current_rev,'Returned current rev from revprop_list');
is($rph->{'svn:author'},$username,
   'svn:author is expected user from revprop_list');
if ($^O eq 'MSWin32') {
    # we skip the log change test on win32 so we have to test
    # for a different var here
    is($rph->{'svn:log'},'Make dir1',
       'svn:log is expected value from revprop_list');
} else {
    is($rph->{'svn:log'},'mkdir dir1',
       'svn:log is expected value from revprop_list');
}
ok($rph->{'svn:date'},'svn:date is set from revprop_list');

is($ctx->checkout($reposurl,$wcpath,'HEAD',1),$current_rev,
   'Returned current rev from checkout');

is(SVN::Client::url_from_path($wcpath),$reposurl,
   "Returned $reposurl from url_from_path");

ok(open(NEW, ">$wcpath/dir1/new"),'Open new file for writing');
ok(print(NEW 'addtest'), 'Print to new file');
ok(close(NEW),'Close new file');

# no return means success
is($ctx->add("$wcpath/dir1/new",0),undef,
   'Returned undef from add schedule operation');

# test the log_msg callback
$ctx->log_msg(
    sub
    {
        my ($log_msg,$tmp_file,$commit_items,$pool) = @_;
        isa_ok($log_msg,'SCALAR','log_msg param to callback is a SCALAR');
        isa_ok($tmp_file,'SCALAR','tmp_file param to callback is a SCALAR');
        isa_ok($commit_items,'ARRAY',
               'commit_items param to callback is a SCALAR');
        isa_ok($pool,'_p_apr_pool_t',
               'pool param to callback is a _p_apr_pool_t');
        my $commit_item = shift @$commit_items;
        isa_ok($commit_item,'_p_svn_client_commit_item3_t',
               'commit_item element is a _p_svn_client_commit_item3_t');
        is($commit_item->path(),"$wcpath/dir1/new",
           "commit_item has proper path for committed file");
        is($commit_item->kind(),$SVN::Node::file,
           "kind() shows the node as a file");
        is($commit_item->url(),"$reposurl/dir1/new",
           'URL matches our repos url');
        # revision is INVALID because the commit has not happened yet
        # and this is not a copy
        is($commit_item->revision(),$SVN::Core::INVALID_REVNUM,
           'Revision is INVALID since commit has not happened yet');
        is($commit_item->copyfrom_url(),undef,
           'copyfrom_url is undef since file is not a copy');
        is($commit_item->state_flags(),$SVN::Client::COMMIT_ITEM_ADD |
                                       $SVN::Client::COMMIT_ITEM_TEXT_MODS,
           'state_flags are ADD and TEXT_MODS');
        my $prop_changes = $commit_item->incoming_prop_changes();
        isa_ok($prop_changes, 'ARRAY',
               'incoming_prop_changes returns an ARRAY');
        is(scalar(@$prop_changes), 0,
           'No elements in the incoming_prop_changes array because ' .
           ' we did not make any');
        $prop_changes = $commit_item->outgoing_prop_changes();
        is($prop_changes, undef,
           'No outgoing_prop_changes array because we did not create one');
        $$log_msg = 'Add new';
        return 0;
    } );


my ($ci_commit1) = $ctx->commit($wcpath,0);
isa_ok($ci_commit1,'_p_svn_client_commit_info_t',
       'Commit returns a _p_svn_client_commit_info');
$current_rev++;
is($ci_commit1->revision,$current_rev,
   "commit info revision equals $current_rev");

# get rid of log_msg callback
is($ctx->log_msg(undef),undef,
   'Clearing the log_msg callback works');

# test info() on WC
is($ctx->info("$wcpath/dir1/new", undef, 'WORKING',
              sub
              {
                 my($infopath,$svn_info_t,$pool) = @_;
                 is($infopath,"new",'path passed to receiver is same as WC');
                 isa_ok($svn_info_t,'_p_svn_info_t');
                 isa_ok($pool,'_p_apr_pool_t',
                        'pool param is _p_apr_pool_t');
              }, 0),
   undef,
   'info should return undef');

my $svn_error = $ctx->info("$wcpath/dir1/newxyz", undef, 'WORKING', sub {}, 0);
isa_ok($svn_error, '_p_svn_error_t',
       'info should return _p_svn_error_t for a nonexistent file');
$svn_error->clear(); #don't leak this

# test getting the log
is($ctx->log("$reposurl/dir1/new",$current_rev,$current_rev,1,0,
             sub
             {
                 my ($changed_paths,$revision,
                     $author,$date,$message,$pool) = @_;
                 isa_ok($changed_paths,'HASH',
                        'changed_paths param is a HASH');
                 isa_ok($changed_paths->{'/dir1/new'},
                        '_p_svn_log_changed_path_t',
                        'Hash value is a _p_svn_log_changed_path_t');
                 is($changed_paths->{'/dir1/new'}->action(),'A',
                    'action returns A for add');
                 is($changed_paths->{'/dir1/new'}->copyfrom_path(),undef,
                    'copyfrom_path returns undef as it is not a copy');
                 is($changed_paths->{'/dir1/new'}->copyfrom_rev(),
                    $SVN::Core::INVALID_REVNUM,
                    'copyfrom_rev is set to INVALID as it is not a copy');
                 is($revision,$current_rev,
                    'revision param matches current rev');
                 is($author,$username,
                    'author param matches expected username');
                 ok($date,'date param is defined');
                 is($message,'Add new',
                    'message param is the expected value');
                 isa_ok($pool,'_p_apr_pool_t',
                        'pool param is _p_apr_pool_t');
             }),
   undef,
   'log returns undef');

is($ctx->update($wcpath,'HEAD',1),$current_rev,
   'Return from update is the current rev');

# no return so we should get undef as the result
# we will get a _p_svn_error_t if there is an error.
is($ctx->propset('perl-test','test-val',"$wcpath/dir1",0),undef,
   'propset on a working copy path returns undef');

my ($ph) = $ctx->propget('perl-test',"$wcpath/dir1",undef,0);
isa_ok($ph,'HASH','propget returns a hash');
is($ph->{"$wcpath/dir1"},'test-val','perl-test property has the correct value');

# No revnum for the working copy so we should get INVALID_REVNUM
is($ctx->status($wcpath, undef, sub {
                                      my ($path,$wc_status) = @_;
                                      is($path,"$wcpath/dir1",
                                         'path param to status callback is' .
                                         'the correct path.');
                                      isa_ok($wc_status,'_p_svn_wc_status_t',
                                             'wc_stats param is a' .
                                             ' _p_svn_wc_status_t');
                                      is($wc_status->prop_status(),
                                         $SVN::Wc::status_modified,
                                         'prop_status is status_modified');
                                      # TODO test the rest of the members
                                    },
                1, 0, 0, 0),
   $SVN::Core::INVALID_REVNUM,
   'status returns INVALID_REVNUM when run against a working copy');

my ($ci_commit2) = $ctx->commit($wcpath,0);
isa_ok($ci_commit2,'_p_svn_client_commit_info_t',
       'commit returns a _p_svn_client_commit_info_t');
$current_rev++;
is($ci_commit2->revision(),$current_rev,
   "commit info revision equals $current_rev");

my $dir1_rev = $current_rev;


my($pl) = $ctx->proplist($reposurl,$current_rev,1);
isa_ok($pl,'ARRAY','proplist returns an ARRAY');
isa_ok($pl->[0], '_p_svn_client_proplist_item_t',
       'array element is a _p_svn_client_proplist_item_t');
is($pl->[0]->node_name(),"$reposurl/dir1",
   'node_name is the expected value');
my $plh = $pl->[0]->prop_hash();
isa_ok($plh,'HASH',
       'prop_hash returns a HASH');
is_deeply($plh, {'perl-test' => 'test-val'}, 'test prop list prop_hash values');

# add a dir to test update
my ($ci_dir2) = $ctx->mkdir(["$reposurl/dir2"]);
isa_ok($ci_dir2,'_p_svn_client_commit_info_t',
       'mkdir returns a _p_svn_client_commit_info_t');
$current_rev++;
is($ci_dir2->revision(),$current_rev,
   "commit info revision equals $current_rev");

# Use explicit revnum to test that instead of just HEAD.
is($ctx->update($wcpath,$current_rev,$current_rev),$current_rev,
   'update returns current rev');

# commit action against a repo returns undef
is($ctx->delete(["$wcpath/dir2"],0),undef,
   'delete returns undef');

# no return means success
is($ctx->revert($wcpath,1),undef,
   'revert returns undef');

my ($ci_copy) = $ctx->copy("$reposurl/dir1",2,"$reposurl/dir3");
isa_ok($ci_copy,'_p_svn_client_commit_info_t',
       'copy returns a _p_svn_client_commitn_info_t when run against repo');
$current_rev++;
is($ci_copy->revision,$current_rev,
   "commit info revision equals $current_rev");

ok(mkdir($importpath),'Make import path dir');
ok(open(FOO, ">$importpath/foo"),'Open file for writing in import path dir');
ok(print(FOO 'foobar'),'Print to the file in import path dir');
ok(close(FOO),'Close file in import path dir');

my ($ci_import) = $ctx->import($importpath,$reposurl,0);
isa_ok($ci_import,'_p_svn_client_commit_info_t',
       'Import returns _p_svn_client_commint_info_t');
$current_rev++;
is($ci_import->revision,$current_rev,
   "commit info revision equals $current_rev");

is($ctx->blame("$reposurl/foo",'HEAD','HEAD', sub {
                                              my ($line_no,$rev,$author,
                                                  $date, $line,$pool) = @_;
                                              is($line_no,0,
                                                 'line_no param is zero');
                                              is($rev,$current_rev,
                                                 'rev param is current rev');
                                              is($author,$username,
                                                 'author param is expected' .
                                                 'value');
                                              ok($date,'date is defined');
                                              is($line,'foobar',
                                                 'line is expected value');
                                              isa_ok($pool,'_p_apr_pool_t',
                                                     'pool param is ' .
                                                     '_p_apr_pool_t');
                                            }),
   undef,
   'blame returns undef');

ok(open(CAT, "+>$testpath/cattest"),'open file for cat output');
is($ctx->cat(\*CAT, "$reposurl/foo", 'HEAD'),undef,
   'cat returns undef');
ok(seek(CAT,0,0),
   'seek the beginning of the cat file');
is(readline(*CAT),'foobar',
   'read the first line of the cat file');
ok(close(CAT),'close cat file');

# the string around the $current_rev exists to expose a past
# bug.  In the past we did not accept values that simply
# had not been converted to a number yet.
my ($dirents) = $ctx->ls($reposurl,"$current_rev", 1);
isa_ok($dirents, 'HASH','ls returns a HASH');
isa_ok($dirents->{'dir1'},'_p_svn_dirent_t',
       'hash value is a _p_svn_dirent_t');
is($dirents->{'dir1'}->kind(),$SVN::Core::node_dir,
   'kind() returns a dir node');
is($dirents->{'dir1'}->size(),0,
   'size() returns 0 for a directory');
is($dirents->{'dir1'}->has_props(),1,
   'has_props() returns true');
is($dirents->{'dir1'}->created_rev(),$dir1_rev,
   'created_rev() returns expected rev');
ok($dirents->{'dir1'}->time(),
   'time is defined');
#diag(scalar(localtime($dirents->{'dir1'}->time() / 1000000)));
is($dirents->{'dir1'}->last_author(),$username,
   'last_auth() returns expected username');

# test removing a property
is($ctx->propset('perl-test', undef, "$wcpath/dir1", 0),undef,
   'propset returns undef');

my ($ph2) = $ctx->propget('perl-test', "$wcpath/dir1", 'WORKING', 0);
isa_ok($ph2,'HASH','propget returns HASH');
is(scalar(keys %$ph2),0,
   'No properties after deleting a property');

SKIP: {
    # This is ugly.  It is included here as an aide to understand how
    # to test this and because it makes my life easier as I only have
    # one command to run to test it.  If you want to use this you need
    # to change the usernames, passwords, and paths to the client cert.
    # It assumes that there is a repo running on localhost port 443 at
    # via SSL.  The repo cert should trip a client trust issue.  The
    # client cert should be encrypted and require a pass to use it.
    # Finally uncomment the skip line below.

    # Before shipping make sure the following line is uncommented.
    skip 'Impossible to test without external effort to setup https', 7;

    sub simple_prompt {
        my $cred = shift;
        my $realm = shift;
        my $username_passed = shift;
        my $may_save = shift;
        my $pool = shift;

        ok(1,'simple_prompt called');
        $cred->username('breser');
        $cred->password('foo');
    }

    sub ssl_server_trust_prompt {
        my $cred = shift;
        my $realm = shift;
        my $failures = shift;
        my $cert_info = shift;
        my $may_save = shift;
        my $pool = shift;

        ok(1,'ssl_server_trust_prompt called');
        $cred->may_save(0);
        $cred->accepted_failures($failures);
    }

    sub ssl_client_cert_prompt {
        my $cred = shift;
        my $realm = shift;
        my $may_save = shift;
        my $pool = shift;

        ok(1,'ssl_client_cert_prompt called');
        $cred->cert_file('/home/breser/client-pass.p12');
    }

    sub ssl_client_cert_pw_prompt {
        my $cred = shift;
        my $may_save = shift;
        my $pool = shift;

        ok(1,'ssl_client_cert_pw_prompt called');
        $cred->password('test');
    }

    my $oldauthbaton = $ctx->auth();

    isa_ok($ctx->auth(SVN::Client::get_simple_prompt_provider(
                                sub { simple_prompt(@_,'x') },2),
               SVN::Client::get_ssl_server_trust_prompt_provider(
                                \&ssl_server_trust_prompt),
               SVN::Client::get_ssl_client_cert_prompt_provider(
                                \&ssl_client_cert_prompt,2),
               SVN::Client::get_ssl_client_cert_pw_prompt_provider(
                                \&ssl_client_cert_pw_prompt,2)
              ),'_p_svn_auth_baton_t',
              'auth() accessor returns _p_svn_auth_baton');

    # if this doesn't work we will get an svn_error_t so by
    # getting a hash we know it worked.
    my ($dirents) = $ctx->ls('https://localhost/svn/test','HEAD',1);
    isa_ok($dirents,'HASH','ls returns a HASH');

    # return the auth baton to its original setting
    isa_ok($ctx->auth($oldauthbaton),'_p_svn_auth_baton_t',
           'Successfully set auth_baton back to old value');
}

# Keep track of the ok-ness ourselves, since we need to know the exact
# number of tests at the start of this file. The 'subtest' feature of
# Test::More would be perfect for this, but it's only available in very
# recent perl versions, it seems.
my $ok = 1;
# Get a list of platform specific providers, using the default
# configuration and pool.
my @providers = @{SVN::Core::auth_get_platform_specific_client_providers(undef, undef)};
foreach my $p (@providers) {
    $ok &= defined($p) && $p->isa('_p_svn_auth_provider_object_t');
}
ok($ok, 'svn_auth_get_platform_specific_client_providers returns _p_svn_auth_provider_object_t\'s');

END {
diag('cleanup');
rmtree($testpath);
}
