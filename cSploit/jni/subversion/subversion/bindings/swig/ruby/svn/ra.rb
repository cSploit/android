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

require "English"
require "tempfile"
require "svn/error"
require "svn/util"
require "svn/core"
require "svn/repos"
require "svn/ext/ra"

module Svn
  module Ra
    Util.set_constants(Ext::Ra, self)
    Util.set_methods(Ext::Ra, self)

    @@ra_pool = Svn::Core::Pool.new
    Ra.initialize(@@ra_pool)

    class << self
      def modules
        print_modules("")
      end
    end

    Session = SWIG::TYPE_p_svn_ra_session_t

    class Session
      class << self
        def open(url, config=nil, callbacks=nil)
          pool = Core::Pool.new
          session = Ra.open2(url, callbacks, config || Svn::Core::Config.get, pool)
          session.instance_variable_set(:@pool, pool)
          return session unless block_given?
          begin
            yield session
          ensure
            session.close
          end
        end
      end

      def close
        @pool.destroy
      end

      def latest_revnum
        Ra.get_latest_revnum(self)
      end
      alias latest_revision latest_revnum

      def dated_revision(time)
        Ra.get_dated_revision(self, time.to_apr_time)
      end

      def set_prop(name, value, rev=nil)
        Ra.change_rev_prop(self, rev || latest_revnum, name, value)
      end

      def []=(name, *args)
        value = args.pop
        set_prop(name, value, *args)
        value
      end

      def proplist(rev=nil)
        Ra.rev_proplist(self, rev || latest_revnum)
      end
      alias properties proplist

      def prop(name, rev=nil)
        Ra.rev_prop(self, rev || latest_revnum, name)
      end
      alias [] prop

      def commit_editor(log_msg, lock_tokens={}, keep_lock=false)
        callback = Proc.new do |new_revision, date, author|
          yield(new_revision, date, author)
        end
        editor, editor_baton = Ra.get_commit_editor(self, log_msg, callback,
                                                    lock_tokens, keep_lock)
        editor.baton = editor_baton
        [editor, editor_baton]
      end

      def commit_editor2(log_msg_or_rev_props, lock_tokens={},
                         keep_lock=false, &callback)
        if log_msg_or_rev_props.is_a?(Hash)
          rev_props = log_msg_or_rev_props
        else
          rev_props = {Svn::Core::PROP_REVISION_LOG => log_msg_or_rev_props}
        end
        editor, editor_baton = Ra.get_commit_editor3(self, rev_props, callback,
                                                     lock_tokens, keep_lock)
        editor.baton = editor_baton
        editor
      end

      def file(path, rev=nil)
        output = StringIO.new
        rev ||= latest_revnum
        fetched_rev, props = Ra.get_file(self, path, rev, output)
        output.rewind
        props_filter(props)
        [output.read, props]
      end

      def dir(path, rev=nil, fields=nil)
        rev ||= latest_revnum
        fields ||= Core::DIRENT_ALL
        entries, fetched_rev, props = Ra.get_dir2(self, path, rev, fields)
        props_filter(props)
        [entries, props]
      end

      def update(revision_to_update_to, update_target,
                 editor, editor_baton, depth=nil, &block)
        editor.baton = editor_baton
        update2(revision_to_update_to, update_target,
                editor, depth, &block)
      end

      def update2(revision_to_update_to, update_target, editor, depth=nil,
                  send_copyfrom_args=nil)
        reporter, reporter_baton = Ra.do_update2(self, revision_to_update_to,
                                                 update_target, depth,
                                                 send_copyfrom_args, editor)
        reporter.baton = reporter_baton
        if block_given?
          yield(reporter)
          reporter.finish_report
          nil
        else
          reporter
        end
      end

      def switch(revision_to_switch_to, switch_target, switch_url,
                 editor, editor_baton, depth=nil, &block)
        editor.baton = editor_baton
        switch2(revision_to_switch_to, switch_target, switch_url,
                editor, depth, &block)
      end

      def switch2(revision_to_switch_to, switch_target, switch_url,
                  editor, depth=nil)
        reporter, reporter_baton = Ra.do_switch2(self, revision_to_switch_to,
                                                 switch_target, depth,
                                                 switch_url, editor)
        reporter.baton = reporter_baton
        if block_given?
          yield(reporter)
          reporter.finish_report
          nil
        else
          reporter
        end
      end

      def status(revision, status_target, editor, editor_baton,
                 recurse=true, &block)
        editor.baton = editor_baton
        status2(revision, status_target, editor, recurse, &block)
      end

      def status2(revision, status_target, editor, recurse=true)
        reporter, reporter_baton = Ra.do_status2(self, status_target,
                                                 revision, recurse, editor)
        reporter.baton = reporter_baton
        if block_given?
          yield(reporter)
          reporter.finish_report
          nil
        else
          reporter
        end
      end

      def diff(rev, target, versus_url, editor,
               depth=nil, ignore_ancestry=true, text_deltas=true)
        args = [self, rev, target, depth, ignore_ancestry,
                text_deltas, versus_url, editor]
        reporter, baton = Ra.do_diff3(*args)
        reporter.baton = baton
        reporter
      end

      def log(paths, start_rev, end_rev, limit,
              discover_changed_paths=true,
              strict_node_history=false)
        paths = [paths] unless paths.is_a?(Array)
        receiver = Proc.new do |changed_paths, revision, author, date, message|
          yield(changed_paths, revision, author, date, message)
        end
        Ra.get_log(self, paths, start_rev, end_rev, limit,
                   discover_changed_paths, strict_node_history,
                   receiver)
      end

      def check_path(path, rev=nil)
        Ra.check_path(self, path, rev || latest_revnum)
      end

      def stat(path, rev=nil)
        Ra.stat(self, path, rev || latest_revnum)
      end

      def uuid
        Ra.get_uuid(self)
      end

      def repos_root
        Ra.get_repos_root(self)
      end

      def locations(path, location_revisions, peg_revision=nil)
        peg_revision ||= latest_revnum
        Ra.get_locations(self, path, peg_revision, location_revisions)
      end

      def file_revs(path, start_rev, end_rev=nil)
        args = [path, start_rev, end_rev]
        if block_given?
          revs = file_revs2(*args) do |path, rev, rev_props, prop_diffs|
            yield(path, rev, rev_props, Util.hash_to_prop_array(prop_diffs))
          end
        else
          revs = file_revs2(*args)
        end
        revs.collect do |path, rev, rev_props, prop_diffs|
          [path, rev, rev_props, Util.hash_to_prop_array(prop_diffs)]
        end
      end

      def file_revs2(path, start_rev, end_rev=nil)
        end_rev ||= latest_revnum
        revs = []
        handler = Proc.new do |path, rev, rev_props, prop_diffs|
          revs << [path, rev, rev_props, prop_diffs]
          yield(path, rev, rev_props, prop_diffs) if block_given?
        end
        Ra.get_file_revs(self, path, start_rev, end_rev, handler)
        revs
      end

      def lock(path_revs, comment=nil, steal_lock=false)
        lock_func = Proc.new do |path, do_lock, lock, ra_err|
          yield(path, do_lock, lock, ra_err)
        end
        Ra.lock(self, path_revs, comment, steal_lock, lock_func)
      end

      def unlock(path_tokens, break_lock=false, &lock_func)
        Ra.unlock(self, path_tokens, break_lock, lock_func)
      end

      def get_lock(path)
        Ra.get_lock(self, path)
      end

      def get_locks(path)
        Ra.get_locks(self, path)
      end

      def replay(rev, low_water_mark, editor, send_deltas=true)
        Ra.replay(self, rev, low_water_mark, send_deltas, editor)
      end

      def reparent(url)
        Ra.reparent(self, url)
      end

      def mergeinfo(paths, revision=nil, inherit=nil, include_descendants=false)
        paths = [paths] unless paths.is_a?(Array)
        revision ||= Svn::Core::INVALID_REVNUM
        info = Ra.get_mergeinfo(self, paths, revision, inherit,
                                include_descendants)
        unless info.nil?
          info.each_key do |key|
            info[key] = Core::MergeInfo.new(info[key])
          end
        end
        info
      end

      private
      def props_filter(props)
        date_str = props[Svn::Core::PROP_ENTRY_COMMITTED_DATE]
        if date_str
          date = Time.parse_svn_format(date_str)
          props[Svn::Core::PROP_ENTRY_COMMITTED_DATE] = date
        end
        props
      end
    end

    class Reporter3
      attr_accessor :baton
      def set_path(path, revision, depth=nil, start_empty=true,
                   lock_token=nil)
        Ra.reporter3_invoke_set_path(self, @baton, path, revision,
                                     depth, start_empty, lock_token)
      end

      def delete_path(path)
        Ra.reporter3_invoke_delete_path(self, @baton, path)
      end

      def link_path(path, url, revision, depth=nil,
                    start_empty=true, lock_token=nil)
        Ra.reporter3_invoke_link_path(self, @baton, path, url,
                                      revision, depth, start_empty, lock_token)
      end

      def finish_report
        Ra.reporter3_invoke_finish_report(self, @baton)
      end

      def abort_report
        Ra.reporter3_invoke_abort_report(self, @baton)
      end
    end

    remove_const(:Callbacks)
    class Callbacks
      include Core::Authenticatable

      def initialize(auth_baton=nil)
        self.auth_baton = auth_baton || Core::AuthBaton.new
      end

      def open_tmp_file
        tmp = Tempfile.new("Svn::Ra")
        path = tmp.path
        tmp.close(true)
        path
      end

      def get_wc_prop(relpath, name)
        nil
      end

      def set_wc_prop(path, name, value)
      end

      def push_wc_prop(path, name, value)
      end

      def invalidate_wc_props(path, name)
      end

      def progress_func(progress, total)
      end
    end
  end
end
