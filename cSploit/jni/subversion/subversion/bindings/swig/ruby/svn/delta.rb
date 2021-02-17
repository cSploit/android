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
require "forwardable"

require "svn/error"
require "svn/util"
require "svn/core"
require "svn/ext/delta"

module Svn
  module Delta
    Util.set_constants(Ext::Delta, self)
    Util.set_methods(Ext::Delta, self)

    class << self
      alias _path_driver path_driver
    end
    alias _path_driver path_driver

    module_function
    def svndiff_handler(output, version=nil)
      args = [output, version || 0]
      handler, handler_baton = Delta.txdelta_to_svndiff2(*args)
      handler.baton = handler_baton
      handler
    end

    def read_svndiff_window(stream, version)
      Delta.txdelta_read_svndiff_window(stream, version)
    end

    def skip_svndiff_window(file, version)
      Delta.txdelta_skip_svndiff_window(file, version)
    end

    def path_driver(editor, revision, paths, &callback_func)
      Delta._path_driver(editor, revision, paths, callback_func)
    end

    def send(string_or_stream, handler=nil)
      if handler.nil? and block_given?
        handler = Proc.new {|window| yield(window)}
        Delta.setup_handler_wrapper(handler)
      end
      if string_or_stream.is_a?(TextDeltaStream)
        string_or_stream.send(handler)
      elsif string_or_stream.is_a?(String)
        Delta.txdelta_send_string(string_or_stream, handler)
      else
        Delta.txdelta_send_stream(string_or_stream, handler)
      end
    end

    def apply(source, target, error_info=nil)
      result = Delta.txdelta_apply_wrapper(source, target, error_info)
      handler, handler_baton = result
      handler.baton = handler_baton
      [handler,nil]
    end

    def parse_svndiff(error_on_early_close=true, &handler)
      Delta.txdelta_parse_svndiff(handler, error_on_early_close)
    end

    def setup_handler_wrapper(wrapper)
      Proc.new do |window|
        Delta.txdelta_invoke_window_handler_wrapper(wrapper, window)
      end
    end

    TextDeltaStream = SWIG::TYPE_p_svn_txdelta_stream_t

    class TextDeltaStream
      class << self
        def new(source, target)
          Delta.txdelta(source, target)
        end

        def push_target(source, &handler)
          Delta.txdelta_target_push(handler, source)
        end
      end

      def md5_digest
        Delta.txdelta_md5_digest_as_cstring(self)
      end

      def next_window
        Delta.txdelta_next_window(self, Svn::Core::Pool.new)
      end

      def each
        while window = next_window
          yield(window)
        end
      end

      def send(handler=nil)
        if handler.nil? and block_given?
          handler = Proc.new {|window| yield(window)}
          Delta.setup_handler_wrapper(handler)
        end
        Delta.txdelta_send_txstream(self, handler)
      end
    end

    TextDeltaWindow = TxdeltaWindow

    class TextDeltaWindow
      def compose(other_window)
        Delta.txdelta_compose_windows(other_window, self)
      end

      def ops
        Delta.txdelta_window_t_ops_get(self)
      end

      def apply_instructions(source_buffer)
        Delta.swig_rb_txdelta_apply_instructions(self, source_buffer)
      end
    end

    TextDeltaWindowHandler =
      SWIG::TYPE_p_f_p_svn_txdelta_window_t_p_void__p_svn_error_t

    class TextDeltaWindowHandler
      attr_accessor :baton

      def call(window)
        Delta.txdelta_invoke_window_handler(self, window, @baton)
      end

      def send(string_or_stream)
        if string_or_stream.is_a?(TextDeltaStream)
          Delta.txdelta_send_txstream(string_or_stream, self, @baton)
        elsif string_or_stream.is_a?(String)
          Delta.txdelta_send_string(string_or_stream, self, @baton)
        else
          Delta.txdelta_send_stream(string_or_stream, self, @baton)
        end
      end
    end

    class Editor

      %w(set_target_revision open_root delete_entry
         add_directory open_directory change_dir_prop
         close_directory absent_directory add_file open_file
         apply_textdelta change_file_prop close_file
         absent_file close_edit abort_edit).each do |name|
        alias_method("_#{name}", name)
        undef_method("#{name}=")
      end

      attr_accessor :baton
      attr_writer :target_revision_address
      private :target_revision_address=

      def target_revision
        Svn::Delta.swig_rb_delta_editor_get_target_revision(self)
      end

      def set_target_revision(target_revision)
        args = [self, @baton, target_revision]
        Svn::Delta.editor_invoke_set_target_revision(*args)
      end

      def open_root(base_revision)
        args = [self, @baton, base_revision]
        Svn::Delta.editor_invoke_open_root_wrapper(*args)
      end

      def delete_entry(path, revision, parent_baton)
        args = [self, path, revision, parent_baton]
        Svn::Delta.editor_invoke_delete_entry(*args)
      end

      def add_directory(path, parent_baton,
                        copyfrom_path, copyfrom_revision)
        args = [self, path, parent_baton, copyfrom_path, copyfrom_revision]
        Svn::Delta.editor_invoke_add_directory_wrapper(*args)
      end

      def open_directory(path, parent_baton, base_revision)
        args = [self, path, parent_baton, base_revision]
        Svn::Delta.editor_invoke_open_directory_wrapper(*args)
      end

      def change_dir_prop(dir_baton, name, value)
        args = [self, dir_baton, name, value]
        Svn::Delta.editor_invoke_change_dir_prop(*args)
      end

      def close_directory(dir_baton)
        args = [self, dir_baton]
        Svn::Delta.editor_invoke_close_directory(*args)
      end

      def absent_directory(path, parent_baton)
        args = [self, path, parent_baton]
        Svn::Delta.editor_invoke_absent_directory(*args)
      end

      def add_file(path, parent_baton,
                   copyfrom_path, copyfrom_revision)
        args = [self, path, parent_baton, copyfrom_path, copyfrom_revision]
        Svn::Delta.editor_invoke_add_file_wrapper(*args)
      end

      def open_file(path, parent_baton, base_revision)
        args = [self, path, parent_baton, base_revision]
        Svn::Delta.editor_invoke_open_file_wrapper(*args)
      end

      def apply_textdelta(file_baton, base_checksum)
        args = [self, file_baton, base_checksum]
        handler, handler_baton =
          Svn::Delta.editor_invoke_apply_textdelta_wrapper(*args)
        handler.baton = handler_baton
        handler
      end

      def change_file_prop(file_baton, name, value)
        args = [self, file_baton, name, value]
        Svn::Delta.editor_invoke_change_file_prop(*args)
      end

      def close_file(file_baton, text_checksum)
        args = [self, file_baton, text_checksum]
        Svn::Delta.editor_invoke_close_file(*args)
      end

      def absent_file(path, parent_baton)
        args = [self, path, parent_baton]
        Svn::Delta.editor_invoke_absent_file(*args)
      end

      def close_edit
        args = [self, @baton]
        Svn::Delta.editor_invoke_close_edit(*args)
      ensure
        Core::Pool.destroy(self)
      end

      def abort_edit
        args = [self, @baton]
        Svn::Delta.editor_invoke_abort_edit(*args)
      ensure
        Core::Pool.destroy(self)
      end
    end

    class BaseEditor
      # open_root -> add_directory -> open_directory -> add_file -> open_file
      def set_target_revision(target_revision)
      end

      def open_root(base_revision)
      end

      def delete_entry(path, revision, parent_baton)
      end

      def add_directory(path, parent_baton,
                        copyfrom_path, copyfrom_revision)
      end

      def open_directory(path, parent_baton, base_revision)
      end

      def change_dir_prop(dir_baton, name, value)
      end

      def close_directory(dir_baton)
      end

      def absent_directory(path, parent_baton)
      end

      def add_file(path, parent_baton,
                   copyfrom_path, copyfrom_revision)
      end

      def open_file(path, parent_baton, base_revision)
      end

      # return nil or object which has `call' method.
      def apply_textdelta(file_baton, base_checksum)
      end

      def change_file_prop(file_baton, name, value)
      end

      def close_file(file_baton, text_checksum)
      end

      def absent_file(path, parent_baton)
      end

      def close_edit(baton)
      end

      def abort_edit(baton)
      end
    end

    class CopyDetectableEditor < BaseEditor
      def add_directory(path, parent_baton,
                        copyfrom_path, copyfrom_revision)
      end

      def add_file(path, parent_baton,
                   copyfrom_path, copyfrom_revision)
      end
    end

    class ChangedDirsEditor < BaseEditor
      attr_reader :changed_dirs

      def initialize
        @changed_dirs = []
      end

      def open_root(base_revision)
        [true, '']
      end

      def delete_entry(path, revision, parent_baton)
        dir_changed(parent_baton)
      end

      def add_directory(path, parent_baton,
                        copyfrom_path, copyfrom_revision)
        dir_changed(parent_baton)
        [true, path]
      end

      def open_directory(path, parent_baton, base_revision)
        [true, path]
      end

      def change_dir_prop(dir_baton, name, value)
        dir_changed(dir_baton)
      end

      def add_file(path, parent_baton,
                   copyfrom_path, copyfrom_revision)
        dir_changed(parent_baton)
      end

      def open_file(path, parent_baton, base_revision)
        dir_changed(parent_baton)
      end

      def close_edit(baton)
        @changed_dirs.sort!
      end

      private
      def dir_changed(baton)
        if baton[0]
          # the directory hasn't been printed yet. do it.
          @changed_dirs << "#{baton[1]}/"
          baton[0] = nil
        end
      end
    end

    class ChangedEditor < BaseEditor

      attr_reader :copied_files, :copied_dirs
      attr_reader :added_files, :added_dirs
      attr_reader :deleted_files, :deleted_dirs
      attr_reader :updated_files, :updated_dirs

      def initialize(root, base_root)
        @root = root
        @base_root = base_root
        @in_copied_dir = []
        @copied_files = []
        @copied_dirs = []
        @added_files = []
        @added_dirs = []
        @deleted_files = []
        @deleted_dirs = []
        @updated_files = []
        @updated_dirs = []
      end

      def open_root(base_revision)
        [true, '']
      end

      def delete_entry(path, revision, parent_baton)
        if @base_root.dir?("/#{path}")
          @deleted_dirs << "#{path}/"
        else
          @deleted_files << path
        end
      end

      def add_directory(path, parent_baton,
                        copyfrom_path, copyfrom_revision)
        copyfrom_rev, copyfrom_path = @root.copied_from(path)
        if in_copied_dir?
          @in_copied_dir.push(true)
        elsif Util.copy?(copyfrom_path, copyfrom_rev)
          @copied_dirs << ["#{path}/", "#{copyfrom_path.sub(/^\//, '')}/", copyfrom_rev]
          @in_copied_dir.push(true)
        else
          @added_dirs << "#{path}/"
          @in_copied_dir.push(false)
        end
        [false, path]
      end

      def open_directory(path, parent_baton, base_revision)
        [true, path]
      end

      def change_dir_prop(dir_baton, name, value)
        if dir_baton[0]
          @updated_dirs << "#{dir_baton[1]}/"
          dir_baton[0] = false
        end
        dir_baton
      end

      def close_directory(dir_baton)
        unless dir_baton[0]
          @in_copied_dir.pop
        end
      end

      def add_file(path, parent_baton,
                   copyfrom_path, copyfrom_revision)
        copyfrom_rev, copyfrom_path = @root.copied_from(path)
        if in_copied_dir?
          # do nothing
        elsif Util.copy?(copyfrom_path, copyfrom_rev)
          @copied_files << [path, copyfrom_path.sub(/^\//, ''), copyfrom_rev]
        else
          @added_files << path
        end
        [nil, nil, nil]
      end

      def open_file(path, parent_baton, base_revision)
        [nil, nil, path]
      end

      def apply_textdelta(file_baton, base_checksum)
        file_baton[0] = :update
        nil
      end

      def change_file_prop(file_baton, name, value)
        file_baton[1] = :update
      end

      def close_file(file_baton, text_checksum)
        text_mod, prop_mod, path = file_baton
        # test the path. it will be nil if we added this file.
        if path
          if [text_mod, prop_mod] != [nil, nil]
            @updated_files << path
          end
        end
      end

      def close_edit(baton)
        @copied_files.sort! {|a, b| a[0] <=> b[0]}
        @copied_dirs.sort! {|a, b| a[0] <=> b[0]}
        @added_files.sort!
        @added_dirs.sort!
        @deleted_files.sort!
        @deleted_dirs.sort!
        @updated_files.sort!
        @updated_dirs.sort!
      end

      private
      def in_copied_dir?
        @in_copied_dir.last
      end
    end

    class WrapEditor
      extend Forwardable

      def_delegators :@wrapped_editor, :set_target_revision, :open_root
      def_delegators :@wrapped_editor, :delete_entry, :add_directory
      def_delegators :@wrapped_editor, :open_directory, :change_dir_prop
      def_delegators :@wrapped_editor, :close_directory, :absent_directory
      def_delegators :@wrapped_editor, :add_file, :open_file, :apply_textdelta
      def_delegators :@wrapped_editor, :change_file_prop, :close_file
      def_delegators :@wrapped_editor, :absent_file, :close_edit, :abort_edit

      def initialize(wrapped_editor)
        @wrapped_editor = wrapped_editor
      end
    end
  end
end
