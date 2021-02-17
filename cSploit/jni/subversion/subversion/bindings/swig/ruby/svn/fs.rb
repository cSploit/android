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
require "svn/delta"
require "svn/ext/fs"

module Svn
  module Fs
    Util.set_constants(Ext::Fs, self)
    Util.set_methods(Ext::Fs, self)

    @@fs_pool = Svn::Core::Pool.new
    initialize(@@fs_pool)

    class << self
      def modules
        print_modules("")
      end
    end

    @@alias_targets = %w(create delete open hotcopy recover)
    class << self
      @@alias_targets.each do |target|
        alias_method "_#{target}", target
      end
    end
    @@alias_targets.each do |target|
      alias_method "_#{target}", target
    end
    @@alias_targets = nil

    module_function
    def create(path, config={}, &block)
      _create(path, config, &block)
    end

    def delete(path)
      Fs.delete_fs(path)
    end

    def open(path, config={}, &block)
      _open(path, config, &block)
    end

    def hotcopy(src, dest, clean=false)
      _hotcopy(src, dest, clean)
    end

    def recover(path, &cancel_func)
      _recover(path, cancel_func)
    end

    FileSystem = SWIG::TYPE_p_svn_fs_t
    class FileSystem
      class << self
        # For backward compatibility
        def create(*args, &block)
          Fs.create(*args, &block)
        end

        def delete(*args, &block)
          Fs.delete(*args, &block)
        end

        def open(*args, &block)
          Fs.open(*args, &block)
        end
        alias new open

        def hotcopy(*args, &block)
          Fs.hotcopy(*args, &block)
        end

        def recover(*args, &block)
          Fs.recover(*args, &block)
        end
      end

      def set_warning_func(&func)
        Fs.set_warning_func_wrapper(self, func)
      end

      def path
        Fs.path(self)
      end

      def open_txn(name)
        Fs.open_txn(self, name)
      end

      def purge_txn(id)
        Fs.purge_txn(self, id)
      end

      def transaction(rev=nil, flags=0)
        txn = Fs.begin_txn2(self, rev || youngest_rev, flags)

        if block_given?
          yield(txn)
          txn.commit if transactions.include?(txn.name)
        else
          txn
        end
      end

      def youngest_revision
        Fs.youngest_rev(self)
      end
      alias_method :youngest_rev, :youngest_revision

      def deleted_revision(path, start_rev, end_rev)
        Repos.deleted_rev(self, path, start_rev, end_rev)
      end
      alias_method :deleted_rev, :deleted_revision

      def prop(name, rev=nil)
        value = Fs.revision_prop(self, rev || youngest_rev, name)
        if value and name == Svn::Core::PROP_REVISION_DATE
          Time.parse_svn_format(value)
        else
          value
        end
      end

      def set_prop(name, value, rev=nil)
        Fs.change_rev_prop(self, rev || youngest_rev, name, value)
      end

      def proplist(rev=nil)
        list = Fs.revision_proplist(self, rev || youngest_rev)
        date_str = list[Svn::Core::PROP_REVISION_DATE]
        if date_str
          list[Svn::Core::PROP_REVISION_DATE] = Time.parse_svn_format(date_str)
        end
        list
      end

      def transactions
        Fs.list_transactions(self)
      end

      def root(rev=nil)
        Fs.revision_root(self, rev || youngest_rev)
      end

      def access
        Fs.get_access(self)
      end

      def access=(new_access)
        Fs.set_access(self, new_access)
      end

      def deltify_revision(rev=nil)
        Fs.deltify_revision(self, rev || youngest_rev)
      end

      def uuid
        Fs.get_uuid(self)
      end

      def uuid=(new_uuid)
        Fs.set_uuid(self, new_uuid)
      end

      def lock(path, token=nil, comment=nil, dav_comment=true,
               expiration_date=nil, current_rev=nil, steal_lock=false)
        if expiration_date
          expiration_date = expiration_date.to_apr_time
        else
          expiration_date = 0
        end
        current_rev ||= youngest_rev
        Fs.lock(self, path, token, comment, dav_comment,
                expiration_date, current_rev, steal_lock)
      end

      def unlock(path, token, break_lock=false)
        Fs.unlock(self, path, token, break_lock)
      end

      def generate_lock_token
        Fs.generate_lock_token(self)
      end

      def get_lock(path)
        Fs.get_lock(self, path)
      end

      def get_locks(path)
        locks = []
        receiver = Proc.new do |lock|
          locks << lock
          yield(lock) if block_given?
        end
        Fs.get_locks(self, path, receiver)
        locks
      end

      def history(path, start_rev, end_rev,
                  cross_copies=true, authz_read_func=nil)
        hist = []
        history_func = Proc.new do |path, revision|
          yield(path, revision) if block_given?
          hist << [path, revision]
        end
        Repos.history2(self, path, history_func,
                       authz_read_func, start_rev, end_rev,
                       cross_copies)
        hist
      end

      def trace_node_locations(fs_path, location_revisions,
                               peg_rev=nil, &authz_read_func)
        peg_rev ||= youngest_rev
        Repos.trace_node_locations(self, fs_path, peg_rev,
                                   location_revisions,
                                   authz_read_func)
      end
    end

    Access = SWIG::TYPE_p_svn_fs_access_t
    class Access
      class << self
        def new(username)
          Fs.create_access(username)
        end
      end

      def username
        Fs.access_get_username(self)
      end

      def add_lock_token(token)
        Fs.access_add_lock_token(self, token)
      end
    end

    Transaction = SWIG::TYPE_p_svn_fs_txn_t
    class Transaction

      def name
        Fs.txn_name(self)
      end

      def prop(name)
        value = Fs.txn_prop(self, name)
        if name == Svn::Core::PROP_REVISION_DATE and value
          value = Time.parse_svn_format(value)
        end
        value
      end

      def set_prop(name, value, validate=true)
        if validate
          Repos.fs_change_txn_prop(self, name, value)
        else
          Fs.change_txn_prop(self, name, value)
        end
      end

      def base_revision
        Fs.txn_base_revision(self)
      end

      def root
        Fs.txn_root(self)
      end

      def proplist
        list = Fs.txn_proplist(self)
        date_str = list[Svn::Core::PROP_REVISION_DATE]
        if list[Svn::Core::PROP_REVISION_DATE]
          list[Svn::Core::PROP_REVISION_DATE] = Time.parse_svn_format(date_str)
        end
        list
      end

      def abort
        Fs.abort_txn(self)
      end

      def commit
        result = Fs.commit_txn(self)
        if result.is_a?(Array)
          result
        else
          [nil, result]
        end
      end
    end


    Root = SWIG::TYPE_p_svn_fs_root_t
    class Root
      attr_reader :editor

      def dir?(path)
        Fs.is_dir(self, path)
      end

      def file?(path)
        Fs.is_file(self, path)
      end

      def base_revision
        Fs.txn_root_base_revision(self)
      end

      def revision
        Fs.revision_root_revision(self)
      end

      def name
        Fs.txn_root_name(self)
      end

      def fs
        Fs.root_fs_wrapper(self)
      end

      def node_id(path)
        Fs.node_id(self, path)
      end

      def node_created_rev(path)
        Fs.node_created_rev(self, path)
      end

      def node_created_path(path)
        Fs.node_created_path(self, path)
      end

      def node_prop(path, key)
        Fs.node_prop(self, path, key)
      end

      def set_node_prop(path, key, value, validate=true)
        if validate
          Repos.fs_change_node_prop(self, path, key, value)
        else
          Fs.change_node_prop(self, path, key, value)
        end
      end

      def node_proplist(path)
        Fs.node_proplist(self, path)
      end
      alias node_prop_list node_proplist

      def check_path(path)
        Fs.check_path(self, path)
      end

      def file_length(path)
        Fs.file_length(self, path)
      end

      def file_md5_checksum(path)
        Fs.file_md5_checksum(self, path)
      end

      def file_contents(path)
        stream = Fs.file_contents(self, path)
        if block_given?
          begin
            yield stream
          ensure
            stream.close
          end
        else
          stream
        end
      end

      def close
        Fs.close_root(self)
      end

      def dir_entries(path)
        entries = {}
        Fs.dir_entries(self, path).each do |name, entry|
          entries[name] = entry
        end
        entries
      end

      def dir_delta(src_path, src_entry, tgt_root, tgt_path,
                    editor, text_deltas=false, recurse=true,
                    entry_props=false, ignore_ancestry=false,
                    &authz_read_func)
        Repos.dir_delta(self, src_path, src_entry, tgt_root,
                        tgt_path, editor, authz_read_func,
                        text_deltas, recurse, entry_props,
                        ignore_ancestry)
      end

      def replay(editor, base_dir=nil, low_water_mark=nil, send_deltas=false,
                 &callback)
        base_dir ||= ""
        low_water_mark ||= Core::INVALID_REVNUM
        Repos.replay2(self, base_dir, low_water_mark, send_deltas,
                      editor, callback)
      end

      def copied_from(path)
        Fs.copied_from(self, path)
      end

      def txn_root?
        Fs.is_txn_root(self)
      end

      def revision_root?
        Fs.is_revision_root(self)
      end

      def paths_changed
        Fs.paths_changed(self)
      end

      def node_history(path)
        Fs.node_history(self, path)
      end

      def props_changed?(path1, root2, path2)
        Fs.props_changed(self, path1, root2, path2)
      end

      def merge(target_path, source_root, source_path,
                ancestor_root, ancestor_path)
        Fs.merge(source_root, source_path, self, target_path,
                 ancestor_root, ancestor_path)
      end

      def make_dir(path)
        Fs.make_dir(self, path)
      end

      def delete(path)
        Fs._delete(self, path)
      end

      def copy(to_path, from_root, from_path)
        Fs.copy(from_root, from_path, self, to_path)
      end

      def revision_link(from_root, path)
        Fs.revision_link(from_root, self, path)
      end

      def make_file(path)
        Fs.make_file(self, path)
      end

      def apply_textdelta(path, base_checksum=nil, result_checksum=nil)
        handler, handler_baton = Fs.apply_textdelta(self, path,
                                                    base_checksum,
                                                    result_checksum)
        handler.baton = handler_baton
        handler
      end

      def apply_text(path, result_checksum=nil)
        Fs.apply_text(self, path, result_checksum)
      end

      def contents_changed?(path1, root2, path2)
        Fs.contents_changed(self, path1, root2, path2)
      end

      def file_delta_stream(arg1, arg2, target_path)
        if arg1.is_a?(self.class)
          source_root = arg1
          source_path = arg2
          target_root = self
        else
          source_root = self
          source_path = arg1
          target_root = arg2
        end
        Fs.get_file_delta_stream(source_root, source_path,
                                 target_root, target_path)
      end

      def stat(path)
        Repos.stat(self, path)
      end

      def committed_info(path)
        Repos.get_committed_info(self, path)
      end

      def closest_copy(path)
        Fs.closest_copy(self, path)
      end

      def mergeinfo(paths, inherit=nil, include_descendants=false)
        paths = [paths] unless paths.is_a?(Array)
        Fs.get_mergeinfo(self, paths, inherit, include_descendants)
      end

      def change_mergeinfo(path, info)
        Fs.change_mergeinfo(self, path, info)
      end
    end

    History = SWIG::TYPE_p_svn_fs_history_t

    class History
      def location
        Fs.history_location(self)
      end

      def prev(cross_copies=true)
        Fs.history_prev(self, cross_copies)
      end
    end


    DirectoryEntry = Dirent

    Id = SWIG::TYPE_p_svn_fs_id_t
    class Id
      def to_s
        unparse
      end

      def unparse
        Fs.unparse_id(self)
      end

      def compare(other)
        Fs.compare_ids(self, other)
      end

      def related?(other)
        Fs.check_related(self, other)
      end
    end

    class PathChange
      def modify?
        change_kind == Svn::Fs::PATH_CHANGE_MODIFY
      end

      def add?
        change_kind == Svn::Fs::PATH_CHANGE_ADD
      end

      def text_mod?
        text_mod
      end
    end

    class FileDiff

      def initialize(root1, path1, root2, path2)
        @tempfile1 = nil
        @tempfile2 = nil

        @binary = nil

        @root1 = root1
        @path1 = path1
        @root2 = root2
        @path2 = path2
      end

      def binary?
        if @binary.nil?
          @binary = _binary?(@root1, @path1)
          @binary ||= _binary?(@root2, @path2)
        end
        @binary
      end

      def files
        if @tempfile1
          [@tempfile1, @tempfile2]
        else
          @tempfile1 = Tempfile.new("svn_fs")
          @tempfile2 = Tempfile.new("svn_fs")

          begin
            dump_contents(@tempfile1, @root1, @path1)
            dump_contents(@tempfile2, @root2, @path2)
          ensure
            @tempfile1.close
            @tempfile2.close
          end

          [@tempfile1, @tempfile2]
        end
      end

      def diff
        files
        @diff ||= Core::Diff.file_diff(@tempfile1.path, @tempfile2.path)
      end

      def unified(label1, label2, header_encoding=nil)
        if diff and diff.diff?
          diff.unified(label1, label2, header_encoding)
        else
          ""
        end
      end

      private
      def dump_contents(open_tempfile, root, path)
        if root and path
          root.file_contents(path) do |stream|
            open_tempfile.print(stream.read)
          end
        end
      end

      def _binary?(root, path)
        if root and path
          prop = root.node_prop(path, Core::PROP_MIME_TYPE)
          prop and Core.binary_mime_type?(prop)
        end
      end
    end
  end
end
