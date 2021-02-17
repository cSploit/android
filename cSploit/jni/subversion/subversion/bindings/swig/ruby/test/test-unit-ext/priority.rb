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

module Test
  module Unit
    class TestCase
      class << self
        def inherited(sub)
          super
          sub.instance_variable_set("@priority_initialized", true)
          sub.instance_variable_set("@priority_table", {})
          sub.priority :normal
        end

        def include(*args)
          args.reverse_each do |mod|
            super(mod)
            next unless defined?(@priority_initialized)
            mod.instance_methods(false).each do |name|
              set_priority(name)
            end
          end
        end

        def method_added(name)
          set_priority(name) if defined?(@priority_initialized)
        end

        def priority(name, *tests)
          singleton_class = (class << self; self; end)
          priority_check_method = priority_check_method_name(name)
          unless singleton_class.private_method_defined?(priority_check_method)
            raise ArgumentError, "unknown priority: #{name}"
          end
          if tests.empty?
            @current_priority = name
          else
            tests.each do |test|
              set_priority(test, name)
            end
          end
        end

        def need_to_run?(test_name)
          normalized_test_name = normalize_test_name(test_name)
          priority = @priority_table[normalized_test_name]
          return true unless priority
          __send__(priority_check_method_name(priority), test_name)
        end

        private
        def priority_check_method_name(priority_name)
          "run_priority_#{priority_name}?"
        end

        def normalize_test_name(test_name)
          "test_#{test_name.to_s.sub(/^test_/, '')}"
        end

        def set_priority(name, priority=@current_priority)
          @priority_table[normalize_test_name(name)] = priority
        end

        def run_priority_must?(test_name)
          true
        end

        def run_priority_important?(test_name)
          rand > 0.1
        end

        def run_priority_high?(test_name)
          rand > 0.3
        end

        def run_priority_normal?(test_name)
          rand > 0.5
        end

        def run_priority_low?(test_name)
          rand > 0.75
        end

        def run_priority_never?(test_name)
          false
        end
      end

      def need_to_run?
        !previous_test_success? or self.class.need_to_run?(@method_name)
      end

      alias_method :original_run, :run
      def run(result, &block)
        original_run(result, &block)
      ensure
        if passed?
          FileUtils.touch(passed_file)
        else
          FileUtils.rm_f(passed_file)
        end
      end

      private
      def previous_test_success?
        File.exist?(passed_file)
      end

      def result_dir
        dir = File.join(File.dirname($0), ".test-result",
                        self.class.name, escaped_method_name)
        dir = File.expand_path(dir)
        FileUtils.mkdir_p(dir)
        dir
      end

      def passed_file
        File.join(result_dir, "passed")
      end

      def escaped_method_name
        @method_name.to_s.gsub(/[!?]$/) do |matched|
          case matched
          when "!"
            ".destructive"
          when "?"
            ".predicate"
          end
        end
      end
    end

    class TestSuite
      @@priority_mode = false

      class << self
        def priority_mode=(bool)
          @@priority_mode = bool
        end
      end

      alias_method :original_run, :run
      def run(*args, &block)
        priority_mode = @@priority_mode
        if priority_mode
          @original_tests = @tests
          apply_priority
        end
        original_run(*args, &block)
      ensure
        @tests = @original_tests if priority_mode
      end

      def apply_priority
        @tests = @tests.reject {|test| !test.need_to_run?}
      end

      def need_to_run?
        apply_priority
        !@tests.empty?
      end
    end

    class AutoRunner
      alias_method :original_options, :options
      def options
        opts = original_options
        opts.on("--[no-]priority", "use priority mode") do |bool|
          TestSuite.priority_mode = bool
        end
        opts
      end
    end
  end
end
