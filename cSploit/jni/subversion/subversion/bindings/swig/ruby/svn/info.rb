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
require "time"
require "digest/sha2"
require "tempfile"

require "nkf"
begin
  require "uconv"
rescue LoadError
  module Uconv #:nodoc:
    class Error < StandardError #:nodoc:
    end
    def self.u8toeuc(str)
      raise Error
    end
  end
end

require "svn/core"
require "svn/repos"
require "svn/fs"
require "svn/delta"

module Svn
  class Info

    attr_reader :author, :log, :date, :changed_dirs
    attr_reader :added_files, :deleted_files, :updated_files, :copied_files
    attr_reader :added_dirs, :deleted_dirs, :updated_dirs, :copied_dirs
    attr_reader :path, :revision, :diffs
    attr_reader :sha256, :entire_sha256

    def initialize(path, rev)
      setup(path, rev)
      get_info
      get_dirs_changed
      get_changed
      get_diff
      get_sha256
      teardown
    end

    def paths
      files + directories
    end

    def files
      @added_files + @deleted_files + @updated_files + @copied_files
    end

    def directories
      @added_dirs + @deleted_dirs + @updated_dirs + @copied_dirs
    end

    def sub_paths(prefix)
      prefixes = prefix.split(/\/+/)
      results = []
      paths.each do |path,|
        paths = path.split(/\/+/)
        if prefixes.size < paths.size and prefixes == paths[0, prefixes.size]
          results << paths[prefixes.size]
        end
      end
      results
    end

    private
    def setup(path, rev)
      @path = path
      @revision = Integer(rev)
      @prev_rev = @revision - 1
      @repos = Repos.open(@path)
      @fs = @repos.fs
      @root = @fs.root(@revision)
    end

    def teardown
      @repos.close
      @repos = @root = @fs = nil
    end

    def get_info
      @author = force_to_utf8(prop(Core::PROP_REVISION_AUTHOR))
      @date = prop(Core::PROP_REVISION_DATE)
      @log = force_to_utf8(prop(Core::PROP_REVISION_LOG))
    end

    def get_dirs_changed
      editor = traverse(Delta::ChangedDirsEditor)
      @changed_dirs = editor.changed_dirs
    end

    def get_changed
      editor = traverse(Delta::ChangedEditor, true)
      @copied_files = editor.copied_files
      @copied_dirs = editor.copied_dirs
      @added_files = editor.added_files
      @added_dirs = editor.added_dirs
      @deleted_files = editor.deleted_files
      @deleted_dirs = editor.deleted_dirs
      @updated_files = editor.updated_files
      @updated_dirs = editor.updated_dirs
    end

    def get_diff
      tree = @repos.delta_tree(@root, @prev_rev)
      @diffs = {}
      get_diff_recurse(tree, @fs.root(@prev_rev), "", "")
      @diffs.each do |key, values|
        values.each do |type, diff|
          diff.body = force_to_utf8(diff.body)
        end
      end
    end

    TYPE_TABLE = [
      [:copy?, :copied],
      [:add?, :added],
      [:delete?, :deleted],
      [:replace?, :modified],
    ]

    def get_type(node)
      TYPE_TABLE.each do |meth, type|
        return type if node.__send__(meth)
      end
      nil
    end

    def get_diff_recurse(node, base_root, path, base_path)
      if node.copy?
        base_path = node.copyfrom_path.sub(/\A\//, '')
        base_root = base_root.fs.root(node.copyfrom_rev)
      end

      if node.file?
        try_diff(node, base_root, path, base_path)
      end

      if node.prop_mod? and !node.delete?
        get_prop_diff(node, base_root, path, base_path)
      end

      node = node.child
      if node
        get_diff_recurse(node, base_root,
                         "#{path}/#{node.name}",
                         "#{base_path}/#{node.name}")
        while node.sibling
          node = node.sibling
          get_diff_recurse(node, base_root,
                           "#{path}/#{node.name}",
                           "#{base_path}/#{node.name}")
        end
      end
    end

    def get_prop_diff(node, base_root, path, base_path)
      local_props = @root.node_prop_list(path)
      if node.add?
        base_props = {}
      else
        base_props = base_root.node_prop_list(base_path)
      end
      prop_changes = Core::Property.diffs2(local_props, base_props)
      prop_changes.each do |name, value|
        entry = diff_entry(path, :property_changed)
        entry.body << "Name: #{force_to_utf8(name)}\n"
        orig_value = base_props[name]
        entry.body << "   - #{force_to_utf8(orig_value)}\n" if orig_value
        entry.body << "   + #{force_to_utf8(value)}\n" if value
      end
    end

    def try_diff(node, base_root, path, base_path)
      if node.replace? and node.text_mod?
        differ = Fs::FileDiff.new(base_root, base_path, @root, path)
        do_diff(node, base_root, path, base_path, differ)
      elsif node.add? and node.text_mod?
        differ = Fs::FileDiff.new(nil, base_path, @root, path)
        do_diff(node, base_root, path, base_path, differ)
      elsif node.delete?
        differ = Fs::FileDiff.new(base_root, base_path, nil, path)
        do_diff(node, base_root, path, base_path, differ)
      elsif node.copy?
        diff_entry(path, get_type(node))
      end
    end

    def do_diff(node, base_root, path, base_path, differ)
      entry = diff_entry(path, get_type(node))

      if differ.binary?
        entry.body = "(Binary files differ)\n"
      else
        base_rev = base_root.revision
        rev = @root.revision
        base_date = base_root.fs.prop(Core::PROP_REVISION_DATE, base_rev)
        date = @root.fs.prop(Core::PROP_REVISION_DATE, rev)
        base_date = format_date(base_date)
        date = format_date(date)
        stripped_base_path = base_path.sub(/\A\//, '')
        stripped_path = path.sub(/\A\//, '')
        base_label = "#{stripped_base_path}\t#{base_date} (rev #{base_rev})"
        label = "#{stripped_path}\t#{date} (rev #{rev})"
        entry.body = differ.unified(base_label, label, "UTF-8")
        parse_diff_unified(entry)
      end
    end

    def parse_diff_unified(entry)
      in_content = false
      entry.body.each do |line|
        case line
        when /^@@/
          in_content = true
        else
          if in_content
            case line
            when /^\+/
              entry.count_up_added_line!
            when /^-/
              entry.count_up_deleted_line!
            end
          end
        end
      end
    end

    def dump_contents(root, path)
      root.file_contents(path) do |f|
        if block_given?
          yield f
        else
          f.read
        end
      end
    end

    def diff_entry(target, type)
      target = target.sub(/\A\//, '')
      @diffs[target] ||= {}
      @diffs[target][type] ||= DiffEntry.new(type)
      @diffs[target][type]
    end

    def get_sha256
      sha = Digest::SHA256.new
      @sha256 = {}
      [
        @added_files,
#        @deleted_files,
        @updated_files,
      ].each do |files|
        files.each do |file|
          content = dump_contents(@root, file)
          sha << content
          @sha256[file] = {
            :file => file,
            :revision => @revision,
            :sha256 => Digest::SHA256.hexdigest(content),
          }
        end
      end
      @entire_sha256 = sha.hexdigest
    end

    def prop(name)
      @fs.prop(name, @revision)
    end

    def force_to_utf8(str)
      str = str.to_s
      begin
        # check UTF-8 or not
        Uconv.u8toeuc(str)
        str
      rescue Uconv::Error
        NKF.nkf("-w", str)
      end
    end

    def traverse(editor_class, pass_root=false)
      base_rev = @prev_rev
      base_root = @fs.root(base_rev)
      if pass_root
        editor = editor_class.new(@root, base_root)
      else
        editor = editor_class.new
      end
      base_root.dir_delta("", "", @root, "", editor)
      editor
    end

    def format_date(date)
      formatted = date.strftime("%Y-%m-%d %H:%M:%S")
      formatted << (" %+03d:00" % (date.hour - date.getutc.hour))
      formatted
    end

    class DiffEntry
      attr_reader :type, :added_line, :deleted_line
      attr_accessor :body

      def initialize(type)
        @type = type
        @added_line = 0
        @deleted_line = 0
        @body = ""
      end

      def count_up_added_line!
        @added_line += 1
      end

      def count_up_deleted_line!
        @deleted_line += 1
      end
    end
  end
end
