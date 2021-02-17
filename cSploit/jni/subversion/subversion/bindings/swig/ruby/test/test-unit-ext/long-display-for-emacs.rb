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

require 'test/unit/failure'
require 'test/unit/error'

module Test
  module Unit
    BACKTRACE_INFO_RE = /.+:\d+:in `.+?'/
    class Failure
      alias_method :original_long_display, :long_display
      def long_display
        extract_backtraces_re =
          /^    \[(#{BACKTRACE_INFO_RE}(?:\n     #{BACKTRACE_INFO_RE})+)\]:$/
        original_long_display.gsub(extract_backtraces_re) do |backtraces|
          $1.gsub(/^     (#{BACKTRACE_INFO_RE})/, '\1') + ':'
        end
      end
    end

    class Error
      alias_method :original_long_display, :long_display
      def long_display
        original_long_display.gsub(/^    (#{BACKTRACE_INFO_RE})/, '\1')
      end
    end
  end
end
