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

require "test/unit/ui/testrunnermediator"

module Test
  module Unit
    module UI
      class TestRunnerMediator
        alias_method :original_run_suite, :run_suite
        def run_suite
          @notified_finished = false
          begin_time = Time.now
          original_run_suite
        rescue Interrupt
          unless @notified_finished
            end_time = Time.now
            elapsed_time = end_time - begin_time
            notify_listeners(FINISHED, elapsed_time)
          end
          raise
        end

        def notify_listeners(channel_name, *arguments)
          @notified_finished = true if channel_name == FINISHED
          super
        end
      end
    end
  end
end
