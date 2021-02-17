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

require "svn/info"

class SvnInfoTest < Test::Unit::TestCase
  include SvnTestUtil

  def setup
    setup_basic
  end

  def teardown
    teardown_basic
  end

  def test_info
    file = "hello.txt"
    path = File.join(@wc_path, file)
    log = "test commit\nnew line"
    FileUtils.touch(path)

    date, revision = make_context(log) do |ctx|
      ctx.add(path)
      commit_info = ctx.commit([@wc_path])
      [commit_info.date, commit_info.revision]
    end

    info = make_info(revision)
    assert_equal(@author, info.author)
    assert_equal(date, info.date)
    assert_equal(revision, info.revision)
    assert_equal(log, info.log)
  end

  def test_dirs_changed
    dir = "new_dir"
    file = "new.txt"
    dir_path = File.join(@wc_path, dir)
    file_path = File.join(dir_path, file)
    log = "added dir"

    revision = make_context(log) do |ctx|
      ctx.mkdir(dir_path)
      FileUtils.touch(file_path)
      ctx.add(file_path)
      ctx.commit(@wc_path).revision
    end

    info = make_info(revision)
    assert_equal(["/", "#{dir}/"], info.changed_dirs)
    assert_equal(revision, info.revision)
    assert_equal(log, info.log)
  end

  def test_changed
    dir = "changed_dir"
    tmp_dir = "changed_tmp_dir"
    dir_path = File.join(@wc_path, dir)
    tmp_dir_path = File.join(@wc_path, tmp_dir)
    dir_svn_path = dir
    tmp_dir_svn_path = tmp_dir

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

    file6 = "changed6.txt"
    file7 = "changed7.txt"
    file8 = "changed8.txt"
    file9 = "changed9.txt"
    file6_path = File.join(dir_path, file6)
    file7_path = File.join(@wc_path, file7)
    file8_path = File.join(dir_path, file8)
    file9_path = File.join(dir_path, file9)
    file6_svn_path = [dir_svn_path, file6].join("/")
    file7_svn_path = file7
    file8_svn_path = [dir_svn_path, file8].join("/")
    file9_svn_path = [dir_svn_path, file9].join("/")

    first_rev = nil

    log = "added 2 dirs\nanded 5 files"
    make_context(log) do |ctx|

      ctx.mkdir([dir_path, tmp_dir_path])
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

      info = make_info(commit_info.revision)
      assert_equal([].sort, info.updated_dirs)
      assert_equal([].sort, info.deleted_dirs)
      assert_equal(["#{dir_svn_path}/", "#{tmp_dir_svn_path}/"].sort,
                   info.added_dirs)
    end

    log = "changed 3 files\ndeleted 2 files\nadded 3 files"
    make_context(log) do |ctx|
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
      ctx.rm(tmp_dir_path)

      commit_info = ctx.commit(@wc_path)
      second_rev = commit_info.revision

      info = make_info(commit_info.revision)
      assert_equal([file1_svn_path, file2_svn_path, file3_svn_path].sort,
                   info.updated_files)
      assert_equal([file4_svn_path, file5_svn_path].sort,
                   info.deleted_files)
      assert_equal([file6_svn_path, file7_svn_path, file8_svn_path].sort,
                   info.added_files)
      assert_equal([].sort, info.updated_dirs)
      assert_equal([
                     [file9_svn_path, file1_svn_path, first_rev]
                   ].sort_by{|x| x[0]},
                   info.copied_files)
      assert_equal([], info.copied_dirs)
      assert_equal(["#{tmp_dir_svn_path}/"].sort, info.deleted_dirs)
      assert_equal([].sort, info.added_dirs)
    end
  end

  def test_diff
    log = "diff"
    make_context(log) do |ctx|

      file1 = "diff1.txt"
      file2 = "diff2.txt"
      file3 = "diff3.txt"
      file1_path = File.join(@wc_path, "diff1.txt")
      file2_path = File.join(@wc_path, "diff2.txt")
      file3_path = File.join(@wc_path, "diff3.txt")
      file1_prop_key = "AAA"
      file1_prop_value = "BBB"
      FileUtils.touch(file1_path)
      File.open(file2_path, "w") {|f| f.puts "changed"}
      FileUtils.touch(file3_path)

      ctx.add(file1_path)
      ctx.add(file2_path)
      ctx.add(file3_path)
      ctx.propset(file1_prop_key, file1_prop_value, file1_path)

      ctx.commit(@wc_path)

      file4 = "diff4.txt"
      file5 = "diff5.txt"
      file4_path = File.join(@wc_path, file4)
      file5_path = File.join(@wc_path, file5)
      file4_prop_key = "XXX"
      file4_prop_value = "YYY"
      File.open(file1_path, "w") {|f| f.puts "changed"}
      File.open(file2_path, "wb") {|f| f.puts "removed\nadded"}
      FileUtils.touch(file4_path)
      ctx.add(file4_path)
      ctx.propdel(file1_prop_key, file1_path)
      ctx.propset(file4_prop_key, file4_prop_value, file4_path)
      ctx.cp(file3_path, file5_path)

      commit_info = ctx.commit(@wc_path)

      info = make_info(commit_info.revision)
      keys = info.diffs.keys.sort
      file5_key = keys.last
      assert_equal(4, info.diffs.size)
      assert_equal([file1, file2, file4].sort, keys[0..-2])
      assert_match(/\A#{file5}/, file5_key)
      assert(info.diffs[file1].has_key?(:modified))
      assert(info.diffs[file1].has_key?(:property_changed))
      assert(info.diffs[file2].has_key?(:modified))
      assert(info.diffs[file4].has_key?(:added))
      assert(info.diffs[file4].has_key?(:property_changed))
      assert(info.diffs[file5_key].has_key?(:copied))
      assert_equal(1, info.diffs[file1][:modified].added_line)
      assert_equal(0, info.diffs[file1][:modified].deleted_line)
      assert_equal(2, info.diffs[file2][:modified].added_line)
      assert_equal(1, info.diffs[file2][:modified].deleted_line)
      assert_equal(0, info.diffs[file4][:added].added_line)
      assert_equal(0, info.diffs[file4][:added].deleted_line)
      assert_equal(0, info.diffs[file5_key][:copied].added_line)
      assert_equal(0, info.diffs[file5_key][:copied].deleted_line)
      assert_equal("Name: #{file1_prop_key}\n   - #{file1_prop_value}\n",
                   info.diffs[file1][:property_changed].body)
      assert_equal("Name: #{file4_prop_key}\n   + #{file4_prop_value}\n",
                   info.diffs[file4][:property_changed].body)
      assert_equal(commit_info.revision, info.revision)
      assert_equal(log, info.log)
    end
  end

  def test_diff_path
    log = "diff path"
    make_context(log) do |ctx|

      parent_dir = "parent_dir"
      child_dir = "child_dir"
      parent_dir_path = File.join(@wc_path, parent_dir)
      child_dir_path = File.join(parent_dir_path, child_dir)
      parent_dir_svn_path = parent_dir
      child_dir_svn_path = [parent_dir, child_dir].join("/")

      ctx.mkdir([parent_dir_path, child_dir_path])

      file1 = "diff1.txt"
      file2 = "diff2.txt"
      file1_path = File.join(@wc_path, file1)
      file2_path = File.join(child_dir_path, file2)
      file1_svn_path = file1
      file2_svn_path = [child_dir_svn_path, file2].join("/")
      File.open(file1_path, "w") {|f| f.puts "new"}
      File.open(file2_path, "w") {|f| f.puts "deep"}
      ctx.add(file1_path)
      ctx.add(file2_path)

      commit_info = ctx.commit(@wc_path)

      info = make_info(commit_info.revision)
      assert_equal(2, info.diffs.size)
      assert(info.diffs.has_key?(file1_svn_path))
      assert(info.diffs.has_key?(file2_svn_path))
      assert(info.diffs[file1_svn_path].has_key?(:added))
      assert(info.diffs[file2_svn_path].has_key?(:added))
      assert_equal(1, info.diffs[file1_svn_path][:added].added_line)
      assert_equal(0, info.diffs[file1_svn_path][:added].deleted_line)
      assert_equal(1, info.diffs[file2_svn_path][:added].added_line)
      assert_equal(0, info.diffs[file2_svn_path][:added].deleted_line)


      file3 = "diff3.txt"
      file4 = "diff4.txt"
      file3_path = File.join(parent_dir_path, file3)
      file4_path = File.join(child_dir_path, file4)
      file3_svn_path = [parent_dir_svn_path, file3].join("/")
      file4_svn_path = [child_dir_svn_path, file4].join("/")

      ctx.mv(file2_path, file3_path)
      ctx.mv(file1_path, file4_path)

      commit_info = ctx.commit(@wc_path)

      info = make_info(commit_info.revision)
      assert_equal([
                     file1_svn_path,
                     file2_svn_path,
                     file3_svn_path,
                     file4_svn_path,
                   ].sort,
                   info.diffs.keys.sort)
      assert(info.diffs[file1_svn_path].has_key?(:deleted))
      assert(info.diffs[file2_svn_path].has_key?(:deleted))
      assert(info.diffs[file3_svn_path].has_key?(:copied))
      assert(info.diffs[file4_svn_path].has_key?(:copied))
    end
  end

  def test_sha256
    log = "sha256"
    make_context(log) do |ctx|

      file1 = "diff1.txt"
      file2 = "diff2.txt"
      file3 = "diff3.txt"
      file1_path = File.join(@wc_path, "diff1.txt")
      file2_path = File.join(@wc_path, "diff2.txt")
      file3_path = File.join(@wc_path, "diff3.txt")
      file1_content = file1
      file2_content = file2
      file3_content = file3
      all_content = [file1, file2, file3].sort.join("")
      File.open(file1_path, "w") {|f| f.print file1_content}
      File.open(file2_path, "w") {|f| f.print file2_content}
      File.open(file3_path, "w") {|f| f.print file3_content}

      ctx.add(file1_path)
      ctx.add(file2_path)
      ctx.add(file3_path)

      commit_info = ctx.commit(@wc_path)

      info = make_info(commit_info.revision)
      assert_equal(3, info.sha256.size)
      assert_equal(Digest::SHA256.hexdigest(file1_content),
                   info.sha256[file1][:sha256])
      assert_equal(Digest::SHA256.hexdigest(file2_content),
                   info.sha256[file2][:sha256])
      assert_equal(Digest::SHA256.hexdigest(file3_content),
                   info.sha256[file3][:sha256])
      assert_equal(Digest::SHA256.hexdigest(all_content),
                   info.entire_sha256)
      assert_equal(commit_info.revision, info.revision)
      assert_equal(log, info.log)
    end
  end

  def make_info(rev=nil)
    Svn::Info.new(@repos_path, rev || @fs.youngest_rev)
  end

end
