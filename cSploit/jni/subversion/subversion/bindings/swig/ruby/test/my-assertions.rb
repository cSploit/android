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
require "test/unit/assertions"

module Test
  module Unit
    module Assertions

      def assert_true(boolean, message=nil)
        _wrap_assertion do
          assert_equal(true, boolean, message)
        end
      end

      def assert_false(boolean, message=nil)
        _wrap_assertion do
          assert_equal(false, boolean, message)
        end
      end

      def assert_nested_sorted_array(expected, actual, message=nil)
        _wrap_assertion do
          assert_equal(expected.collect {|elem| elem.sort},
                       actual.collect {|elem| elem.sort},
                       message)
        end
      end

      def assert_equal_log_entries(expected, actual, message=nil)
        _wrap_assertion do
          actual = actual.collect do |entry|
            changed_paths = entry.changed_paths
            changed_paths.each_key do |path|
              changed_path = changed_paths[path]
              changed_paths[path] = [changed_path.action,
                                     changed_path.copyfrom_path,
                                     changed_path.copyfrom_rev]
            end
            [changed_paths,
             entry.revision,
             entry.revision_properties.reject {|key, value| key == "svn:date"},
             entry.has_children?]
          end
          assert_equal(expected, actual, message)
        end
      end
    end
  end
end
