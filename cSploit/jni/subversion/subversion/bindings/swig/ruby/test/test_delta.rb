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

require "util"
require "stringio"
require 'md5'
require 'tempfile'

require "svn/info"

class SvnDeltaTest < Test::Unit::TestCase
  include SvnTestUtil

  def setup
    setup_basic
  end

  def teardown
    teardown_basic
  end

  def test_version
    assert_equal(Svn::Core.subr_version, Svn::Delta.version)
  end

  def test_txdelta_window
    s = ("a\nb\nc\nd\ne" + "\n" * 100) * 1000
    t = ("a\nb\nX\nd\ne" + "\n" * 100) * 1000
    source = StringIO.new(s)
    target = StringIO.new(t)
    stream = Svn::Delta::TextDeltaStream.new(source, target)
    assert_nil(stream.md5_digest)
    _wrap_assertion do
      stream.each do |window|
        window.ops.each do |op|
          op_size = op.offset + op.length
          case op.action_code
          when Svn::Delta::TXDELTA_SOURCE
            assert_operator(op_size, :<=, window.sview_len)
          when Svn::Delta::TXDELTA_NEW
            assert_operator(op_size, :<=, window.new_data.length)
          when Svn::Delta::TXDELTA_TARGET
            assert_operator(op_size, :<=, window.tview_len)
          else
            flunk
          end
        end
      end
    end
    assert_equal(MD5.new(t).hexdigest, stream.md5_digest)
  end

  def test_txdelta_window_compose
    s = ("a\nb\nc\nd\ne" + "\n" * 100) * 1000
    t = ("a\nb\nX\nd\ne" + "\n" * 100) * 1000
    source = StringIO.new(s)
    target = StringIO.new(t)
    stream = Svn::Delta::TextDeltaStream.new(source, target)
    composed_window = nil
    stream.each do |window|
      if composed_window.nil?
        composed_window = window
      else
        composed_window = composed_window.compose(window)
      end
    end

    _wrap_assertion do
      composed_window.ops.each do |op|
        op_size = op.offset + op.length
        case op.action_code
        when Svn::Delta::TXDELTA_SOURCE
          assert_operator(op_size, :<=, composed_window.sview_len)
        when Svn::Delta::TXDELTA_NEW
          assert_operator(op_size, :<=, composed_window.new_data.length)
        when Svn::Delta::TXDELTA_TARGET
          assert_operator(op_size, :<=, composed_window.tview_len)
        else
          flunk
        end
      end
    end
  end

  def test_txdelta_apply_instructions
    s = ("a\nb\nc\nd\ne" + "\n" * 100) * 1000
    t = ("a\nb\nX\nd\ne" + "\n" * 100) * 1000
    source = StringIO.new(s)
    target = StringIO.new(t)
    stream = Svn::Delta::TextDeltaStream.new(source, target)

    result = ""
    offset = 0
    stream.each do |window|
      result << window.apply_instructions(s[offset, window.sview_len])
      offset += window.sview_len
    end
    assert_equal(t, result)
  end

  def test_push_target
    source = StringIO.new("abcde")
    target_content = "ZZZ" * 100
    data = ""
    finished = false
    handler = Proc.new do |window|
      if window
        data << window.new_data
      else
        finished = true
      end
    end
    target = Svn::Delta::TextDeltaStream.push_target(source, &handler)
    target.write(target_content)
    assert(!finished)
    target.close
    assert(finished)
    assert_equal(target_content, data)
  end

  def test_apply
    source_text = "abcde"
    target_text = "abXde"
    source = StringIO.new(source_text)
    target = StringIO.new(target_text)
    stream = Svn::Delta::TextDeltaStream.new(source, target)

    apply_source = StringIO.new(source_text)
    apply_result = StringIO.new("")

    handler, digest = Svn::Delta.apply(apply_source, apply_result)
    assert_nil(digest)
    handler.send(stream)
    apply_result.rewind
    assert_equal(target_text, apply_result.read)

    handler, digest = Svn::Delta.apply(apply_source, apply_result)
    handler.send(target_text)
    apply_result.rewind
    assert_equal(target_text * 2, apply_result.read)

    handler, digest = Svn::Delta.apply(apply_source, apply_result)
    handler.send(StringIO.new(target_text))
    apply_result.rewind
    assert_equal(target_text * 3, apply_result.read)
  end

  def test_svndiff
    source_text = "abcde"
    target_text = "abXde"
    source = StringIO.new(source_text)
    target = StringIO.new(target_text)
    stream = Svn::Delta::TextDeltaStream.new(source, target)

    output = StringIO.new("")
    handler = Svn::Delta.svndiff_handler(output)

    Svn::Delta.send(target_text, handler)
    output.rewind
    result = output.read
    assert_match(/\ASVN.*#{target_text}\z/, result)

    # skip svndiff window
    input = StringIO.new(result[4..-1])
    window = Svn::Delta.read_svndiff_window(input, 0)
    assert_equal(target_text, window.new_data)

    finished = false
    data = ""
    stream = Svn::Delta.parse_svndiff do |window|
      if window
        data << window.new_data
      else
        finished = true
      end
    end
    stream.write(result)
    stream.close
    assert(finished)
    assert_equal(target_text, data)
  end

  def test_path_driver
    editor = Svn::Delta::BaseEditor.new
    sorted_paths = []
    callback = Proc.new do |parent_baton, path|
      sorted_paths << path
    end

    targets = [
      "/",
      "/file1",
      "/dir1",
      "/dir2/file2",
      "/dir2/dir3/file3",
      "/dir2/dir3/file4"
    ]
    10.times do
      x = rand(targets.size)
      y = rand(targets.size)
      targets[x], targets[y] = targets[y], targets[x]
    end
    Svn::Delta.path_driver(editor, 0, targets, &callback)
    assert_equal(targets.sort, sorted_paths)
  end

  def test_changed
    dir = "changed_dir"
    dir1 = "changed_dir1"
    dir2 = "changed_dir2"
    dir_path = File.join(@wc_path, dir)
    dir1_path = File.join(@wc_path, dir1)
    dir2_path = File.join(@wc_path, dir2)
    dir_svn_path = dir
    dir1_svn_path = dir1
    dir2_svn_path = dir2

    file1 = "changed1.txt"
    file2 = "changed2.txt"
    file3 = "changed3.txt"
    file4 = "changed4.txt"
    file5 = "changed5.txt"
    file1_path = File.join(@wc_path, file1)
    file2_path = File.join(dir_path, file2)
    file3_path = File.join(@wc_path, file3)
    file4_path = File.join(dir_path, file4)
    file5_path = File.join(@wc_path, file5)
    file1_svn_path = file1
    file2_svn_path = [dir_svn_path, file2].join("/")
    file3_svn_path = file3
    file4_svn_path = [dir_svn_path, file4].join("/")
    file5_svn_path = file5

    first_rev = nil

    log = "added 3 dirs\nanded 5 files"
    make_context(log) do |ctx|

      ctx.mkdir([dir_path, dir1_path, dir2_path])

      FileUtils.touch(file1_path)
      FileUtils.touch(file2_path)
      FileUtils.touch(file3_path)
      FileUtils.touch(file4_path)
      FileUtils.touch(file5_path)
      ctx.add(file1_path)
      ctx.add(file2_path)
      ctx.add(file3_path)
      ctx.add(file4_path)
      ctx.add(file5_path)

      commit_info = ctx.commit(@wc_path)
      first_rev = commit_info.revision

      editor = traverse(Svn::Delta::ChangedEditor, commit_info.revision, true)
      assert_equal([
                     file1_svn_path, file2_svn_path,
                     file3_svn_path, file4_svn_path,
                     file5_svn_path,
                   ].sort,
                   editor.added_files)
      assert_equal([], editor.updated_files)
      assert_equal([], editor.deleted_files)
      assert_equal([].sort, editor.updated_dirs)
      assert_equal([].sort, editor.deleted_dirs)
      assert_equal([
                     "#{dir_svn_path}/",
                     "#{dir1_svn_path}/",
                     "#{dir2_svn_path}/"
                   ].sort,
                   editor.added_dirs)
    end

    log = "deleted 2 dirs\nchanged 3 files\ndeleted 2 files\nadded 3 files"
    make_context(log) do |ctx|

      dir3 = "changed_dir3"
      dir4 = "changed_dir4"
      dir3_path = File.join(dir_path, dir3)
      dir4_path = File.join(@wc_path, dir4)
      dir3_svn_path = [dir_svn_path, dir3].join("/")
      dir4_svn_path = dir4

      file6 = "changed6.txt"
      file7 = "changed7.txt"
      file8 = "changed8.txt"
      file9 = "changed9.txt"
      file10 = "changed10.txt"
      file6_path = File.join(dir_path, file6)
      file7_path = File.join(@wc_path, file7)
      file8_path = File.join(dir_path, file8)
      file9_path = File.join(dir_path, file9)
      file10_path = File.join(dir_path, file10)
      file6_svn_path = [dir_svn_path, file6].join("/")
      file7_svn_path = file7
      file8_svn_path = [dir_svn_path, file8].join("/")
      file9_svn_path = [dir_svn_path, file9].join("/")
      file10_svn_path = [dir_svn_path, file10].join("/")

      File.open(file1_path, "w") {|f| f.puts "changed"}
      File.open(file2_path, "w") {|f| f.puts "changed"}
      File.open(file3_path, "w") {|f| f.puts "changed"}
      ctx.rm_f([file4_path, file5_path])
      FileUtils.touch(file6_path)
      FileUtils.touch(file7_path)
      FileUtils.touch(file8_path)
      ctx.add(file6_path)
      ctx.add(file7_path)
      ctx.add(file8_path)
      ctx.cp(file1_path, file9_path)
      ctx.cp(file2_path, file10_path)
      ctx.mv(dir2_path, dir3_path)
      ctx.cp(dir1_path, dir4_path)
      ctx.rm(dir1_path)

      commit_info = ctx.commit(@wc_path)
      second_rev = commit_info.revision

      editor = traverse(Svn::Delta::ChangedEditor, commit_info.revision, true)
      assert_equal([file1_svn_path, file2_svn_path, file3_svn_path].sort,
                   editor.updated_files)
      assert_equal([file4_svn_path, file5_svn_path].sort,
                   editor.deleted_files)
      assert_equal([file6_svn_path, file7_svn_path, file8_svn_path].sort,
                   editor.added_files)
      assert_equal([].sort, editor.updated_dirs)
      assert_equal([
                     [file9_svn_path, file1_svn_path, first_rev],
                     [file10_svn_path, file2_svn_path, first_rev],
                   ].sort_by{|x| x[0]},
                   editor.copied_files)
      assert_equal([
                     ["#{dir3_svn_path}/", "#{dir2_svn_path}/", first_rev],
                     ["#{dir4_svn_path}/", "#{dir1_svn_path}/", first_rev],
                   ].sort_by{|x| x[0]},
                   editor.copied_dirs)
      assert_equal(["#{dir1_svn_path}/", "#{dir2_svn_path}/"].sort,
                   editor.deleted_dirs)
      assert_equal([].sort, editor.added_dirs)
    end
  end

  def test_change_prop
    prop_name = "prop"
    prop_value = "value"

    dir = "dir"
    dir_path = File.join(@wc_path, dir)
    dir_svn_path = dir

    file1 = "file1.txt"
    file2 = "file2.txt"
    file1_path = File.join(@wc_path, file1)
    file2_path = File.join(dir_path, file2)

    log = "added 1 dirs\nanded 2 files"
    make_context(log) do |ctx|

      ctx.mkdir([dir_path])

      file1_svn_path = file1
      file2_svn_path = [dir_svn_path, file2].join("/")
      FileUtils.touch(file1_path)
      FileUtils.touch(file2_path)
      ctx.add(file1_path)
      ctx.add(file2_path)

      ctx.propset(prop_name, prop_value, dir_path)

      commit_info = ctx.commit(@wc_path)

      editor = traverse(Svn::Delta::ChangedDirsEditor, commit_info.revision)
      assert_equal(["", dir_svn_path].collect{|path| "#{path}/"}.sort,
                   editor.changed_dirs)
    end

    log = "prop changed"
    make_context(log) do |ctx|

      ctx.propdel(prop_name, dir_path)

      commit_info = ctx.commit(@wc_path)

      editor = traverse(Svn::Delta::ChangedDirsEditor, commit_info.revision)
      assert_equal([dir_svn_path].collect{|path| "#{path}/"}.sort,
                   editor.changed_dirs)


      ctx.propset(prop_name, prop_value, file1_path)

      commit_info = ctx.commit(@wc_path)

      editor = traverse(Svn::Delta::ChangedDirsEditor, commit_info.revision)
      assert_equal([""].collect{|path| "#{path}/"}.sort,
                   editor.changed_dirs)


      ctx.propdel(prop_name, file1_path)
      ctx.propset(prop_name, prop_value, file2_path)

      commit_info = ctx.commit(@wc_path)

      editor = traverse(Svn::Delta::ChangedDirsEditor, commit_info.revision)
      assert_equal(["", dir_svn_path].collect{|path| "#{path}/"}.sort,
                   editor.changed_dirs)
    end
  end

  def test_deep_copy
    dir1 = "dir1"
    dir2 = "dir2"
    dir1_path = File.join(@wc_path, dir1)
    dir2_path = File.join(dir1_path, dir2)
    dir1_svn_path = dir1
    dir2_svn_path = [dir1, dir2].join("/")

    first_rev = nil

    log = "added 2 dirs\nanded 3 files"
    make_context(log) do |ctx|

      ctx.mkdir([dir1_path, dir2_path])

      file1 = "file1.txt"
      file2 = "file2.txt"
      file3 = "file3.txt"
      file1_path = File.join(@wc_path, file1)
      file2_path = File.join(dir1_path, file2)
      file3_path = File.join(dir2_path, file3)
      file1_svn_path = file1
      file2_svn_path = [dir1_svn_path, file2].join("/")
      file3_svn_path = [dir2_svn_path, file3].join("/")
      FileUtils.touch(file1_path)
      FileUtils.touch(file2_path)
      FileUtils.touch(file3_path)
      ctx.add(file1_path)
      ctx.add(file2_path)
      ctx.add(file3_path)

      commit_info = ctx.commit(@wc_path)
      first_rev = commit_info.revision

      editor = traverse(Svn::Delta::ChangedEditor, commit_info.revision, true)
      assert_equal([
                     file1_svn_path, file2_svn_path,
                     file3_svn_path,
                   ].sort,
                   editor.added_files)
      assert_equal([].sort, editor.updated_files)
      assert_equal([].sort, editor.deleted_files)
      assert_equal([].sort, editor.updated_dirs)
      assert_equal([].sort, editor.deleted_dirs)
      assert_equal([
                     "#{dir1_svn_path}/",
                     "#{dir2_svn_path}/",
                   ].sort,
                   editor.added_dirs)
    end

    log = "copied top dir"
    make_context(log) do |ctx|

      dir3 = "dir3"
      dir3_path = File.join(@wc_path, dir3)
      dir3_svn_path = dir3

      ctx.cp(dir1_path, dir3_path)

      commit_info = ctx.commit(@wc_path)
      second_rev = commit_info.revision

      editor = traverse(Svn::Delta::ChangedEditor, commit_info.revision, true)
      assert_equal([].sort, editor.updated_files)
      assert_equal([].sort, editor.deleted_files)
      assert_equal([].sort, editor.added_files)
      assert_equal([].sort, editor.updated_dirs)
      assert_equal([].sort, editor.copied_files)
      assert_equal([
                     ["#{dir3_svn_path}/", "#{dir1_svn_path}/", first_rev]
                   ].sort_by{|x| x[0]},
                   editor.copied_dirs)
      assert_equal([].sort, editor.deleted_dirs)
      assert_equal([].sort, editor.added_dirs)
    end
  end

  private
  def traverse(editor_class, rev, pass_root=false)
    root = @fs.root
    base_rev = rev - 1
    base_root = @fs.root(base_rev)
    if pass_root
      editor = editor_class.new(root, base_root)
    else
      editor = editor_class.new
    end
    base_root.dir_delta("", "", root, "", editor)
    editor
  end
end
