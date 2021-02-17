#!/usr/bin/env ruby
#
# info.rb : output some info about a subversion url
#
# Example based on a blogpost by Mark Deepwell
# http://www.markdeepwell.com/2010/06/ruby-subversion-bindings/
#
######################################################################
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
######################################################################
#

require "svn/core"
require "svn/client"
require "svn/wc"
require "svn/repos"

# Prompt function mimicking svn's own prompt
simple_prompt = Proc.new do
  |result, realm, username, default, may_save, pool|

  puts "Authentication realm: #{realm}"
  if username != nil
    result.username = username
  else
    print "Username: "
    result.username = STDIN.gets.strip
  end
  print "Password for '#{result.username}': "
  result.password = STDIN.gets.strip
end


if ARGV.length != 1
  puts "Usage: info.rb URL[@REV]"
else
  ctx = Svn::Client::Context.new()
  ctx.add_platform_specific_client_providers
  ctx.add_simple_provider
  ctx.add_simple_prompt_provider(2, simple_prompt)
  ctx.add_username_provider
  ctx.add_ssl_server_trust_file_provider
  ctx.add_ssl_client_cert_file_provider
  ctx.add_ssl_client_cert_pw_file_provider

  repos_uri, revision = ARGV[0].split("@", 2)
  if revision
    revision = Integer(revision)
  end

  begin
    ctx.info(repos_uri, revision) do |path, info|
      puts("Url: #{info.url}")
      puts("Last changed rev: #{info.last_changed_rev}")
      puts("Last changed author: #{info.last_changed_author}")
      puts("Last changed date: #{info.last_changed_date}")
      puts("Kind: #{info.kind}")
    end
  rescue Svn::Error => e
    # catch a generic svn error
    raise "Failed to retrieve SVN info at revision " + revision.to_s
  end
end
