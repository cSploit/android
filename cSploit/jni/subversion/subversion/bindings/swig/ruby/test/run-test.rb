#!/usr/bin/env ruby
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

require "test/unit"
require "fileutils"

test_dir = File.expand_path(File.join(File.dirname(__FILE__)))
base_src_dir = File.expand_path(File.join(File.dirname(__FILE__), ".."))
base_dir = Dir.pwd
top_dir = File.expand_path(File.join(base_dir, "..", "..", "..", ".."))

ext_dir = File.join(base_dir, ".ext")
ext_svn_dir = File.join(ext_dir, "svn")
ext_svn_ext_dir = File.join(ext_svn_dir, "ext")
FileUtils.mkdir_p(ext_svn_dir)
at_exit {FileUtils.rm_rf(ext_dir)}

$LOAD_PATH.unshift(test_dir)
require 'util'
require 'test-unit-ext'

SvnTestUtil.setup_test_environment(top_dir, base_dir, ext_svn_ext_dir)

$LOAD_PATH.unshift(ext_dir)
$LOAD_PATH.unshift(base_src_dir)
$LOAD_PATH.unshift(base_dir)
$LOAD_PATH.unshift(test_dir)

require 'svn/core'
Svn::Locale.set

if Test::Unit::AutoRunner.respond_to?(:standalone?)
  exit Test::Unit::AutoRunner.run($0, File.dirname($0))
else
  exit Test::Unit::AutoRunner.run(false, File.dirname($0))
end
