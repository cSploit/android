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

require "my-assertions"
require "util"
require "time"
require "md5"

require "svn/core"
require "svn/fs"
require "svn/repos"
require "svn/client"

class SvnFsTest < Test::Unit::TestCase
  include SvnTestUtil

  def setup
    setup_basic
  end

  def teardown
    teardown_basic
  end

  def test_version
    assert_equal(Svn::Core.subr_version, Svn::Fs.version)
  end

  def assert_create
    path = File.join(@tmp_path, "fs")
    fs_type = Svn::Fs::TYPE_FSFS
    config = {Svn::Fs::CONFIG_FS_TYPE => fs_type}

    assert(!File.exist?(path))
    fs = nil
    callback = Proc.new do |fs|
      assert(File.exist?(path))
      assert_equal(fs_type, Svn::Fs.type(path))
      fs.set_warning_func do |err|
        p err
        abort
      end
      assert_equal(path, fs.path)
    end
    yield(:create, [path, config], callback)

    assert(fs.closed?)
    assert_raises(Svn::Error::FsAlreadyClose) do
      fs.path
    end

    yield(:delete, [path])
    assert(!File.exist?(path))
  end

  def test_create
    assert_create do |method, args, callback|
      Svn::Fs.__send__(method, *args, &callback)
    end
  end

  def test_create_for_backward_compatibility
    assert_create do |method, args, callback|
      Svn::Fs::FileSystem.__send__(method, *args, &callback)
    end
  end

  def assert_hotcopy
    log = "sample log"
    file = "hello.txt"
    path = File.join(@wc_path, file)
    FileUtils.touch(path)

    rev = nil

    backup_path = make_context(log) do |ctx|
      ctx.add(path)
      commit_info = ctx.commit(@wc_path)
      rev = commit_info.revision

      assert_equal(log, ctx.log_message(path, rev))

      backup_path = File.join(@tmp_path, "back")
    end

    fs_path = @fs.path
    @fs.close
    @fs = nil
    @repos.close
    @repos = nil

    FileUtils.mv(fs_path, backup_path)
    FileUtils.mkdir_p(fs_path)

    make_context(log) do |ctx|
      assert_raises(Svn::Error::RaLocalReposOpenFailed) do
        ctx.log_message(path, rev)
      end

      yield(backup_path, fs_path)
      assert_equal(log, ctx.log_message(path, rev))
    end
  end

  def test_hotcopy
    assert_hotcopy do |src, dest|
      Svn::Fs.hotcopy(src, dest)
    end
  end

  def test_hotcopy_for_backward_compatibility
    assert_hotcopy do |src, dest|
      Svn::Fs::FileSystem.hotcopy(src, dest)
    end
  end

  def test_root
    log = "sample log"
    file = "sample.txt"
    src = "sample source"
    path_in_repos = "/#{file}"
    path = File.join(@wc_path, file)

    assert_nil(@fs.root.name)
    assert_equal(Svn::Core::INVALID_REVNUM, @fs.root.base_revision)

    make_context(log) do |ctx|
      FileUtils.touch(path)
      ctx.add(path)
      rev1 = ctx.commit(@wc_path).revision
      file_id1 = @fs.root.node_id(path_in_repos)

      assert_equal(rev1, @fs.root.revision)
      assert_equal(Svn::Core::NODE_FILE, @fs.root.check_path(path_in_repos))
      assert(@fs.root.file?(path_in_repos))
      assert(!@fs.root.dir?(path_in_repos))

      assert_equal([path_in_repos], @fs.root.paths_changed.keys)
      info = @fs.root.paths_changed[path_in_repos]
      assert(info.text_mod?)
      assert(info.add?)

      File.open(path, "w") {|f| f.print(src)}
      rev2 = ctx.commit(@wc_path).revision
      file_id2 = @fs.root.node_id(path_in_repos)

      assert_equal(src, @fs.root.file_contents(path_in_repos){|f| f.read})
      assert_equal(src.length, @fs.root.file_length(path_in_repos))
      assert_equal(MD5.new(src).hexdigest,
                   @fs.root.file_md5_checksum(path_in_repos))

      assert_equal([path_in_repos], @fs.root.paths_changed.keys)
      info = @fs.root.paths_changed[path_in_repos]
      assert(info.text_mod?)
      assert(info.modify?)

      assert_equal([path_in_repos, rev2],
                   @fs.root.node_history(file).location)
      assert_equal([path_in_repos, rev2],
                   @fs.root.node_history(file).prev.location)
      assert_equal([path_in_repos, rev1],
                   @fs.root.node_history(file).prev.prev.location)

      assert(!@fs.root.dir?(path_in_repos))
      assert(@fs.root.file?(path_in_repos))

      assert(file_id1.related?(file_id2))
      assert_equal(1, file_id1.compare(file_id2))
      assert_equal(1, file_id2.compare(file_id1))

      assert_equal(rev2, @fs.root.node_created_rev(path_in_repos))
      assert_equal(path_in_repos, @fs.root.node_created_path(path_in_repos))

      assert_raises(Svn::Error::FsNotTxnRoot) do
        @fs.root.set_node_prop(path_in_repos, "name", "value")
      end
    end
  end

  def test_transaction
    log = "sample log"
    file = "sample.txt"
    src = "sample source"
    path_in_repos = "/#{file}"
    path = File.join(@wc_path, file)
    prop_name = "prop"
    prop_value = "value"

    make_context(log) do |ctx|
      File.open(path, "w") {|f| f.print(src)}
      ctx.add(path)
      ctx.commit(@wc_path)
    end

    assert_raises(Svn::Error::FsNoSuchTransaction) do
      @fs.open_txn("NOT-EXIST")
    end

    start_time = Time.now
    txn1 = @fs.transaction
    assert_equal([Svn::Core::PROP_REVISION_DATE], txn1.proplist.keys)
    assert_instance_of(Time, txn1.proplist[Svn::Core::PROP_REVISION_DATE])
    date = txn1.prop(Svn::Core::PROP_REVISION_DATE)

    # Subversion's clock is more precise than Ruby's on
    # Windows.  So this test can fail intermittently because
    # the begin and end of the range are the same (to 3
    # decimal places), but the time from Subversion has 6
    # decimal places so it looks like it's not in the range.
    # So we just add a smidgen to the end of the Range.
    assert_operator(start_time..(Time.now + 0.001), :include?, date)
    txn1.set_prop(Svn::Core::PROP_REVISION_DATE, nil)
    assert_equal([], txn1.proplist.keys)
    assert_equal(youngest_rev, txn1.base_revision)
    assert(txn1.root.txn_root?)
    assert(!txn1.root.revision_root?)
    assert_equal(txn1.name, txn1.root.name)
    assert_equal(txn1.base_revision, txn1.root.base_revision)

    @fs.transaction do |txn|
      assert_nothing_raised do
        @fs.open_txn(txn.name)
      end
      txn2 = txn
    end

    txn3 = @fs.transaction

    assert_equal([txn1.name, txn3.name].sort, @fs.transactions.sort)
    @fs.purge_txn(txn3.name)
    assert_equal([txn1.name].sort, @fs.transactions.sort)

    @fs.transaction do |txn|
      assert(@fs.transactions.include?(txn.name))
      txn.abort
      assert(!@fs.transactions.include?(txn.name))
    end

    txn4 = @fs.transaction
    assert_equal({}, txn1.root.node_proplist(path_in_repos))
    assert_nil(txn1.root.node_prop(path_in_repos, prop_name))
    txn1.root.set_node_prop(path_in_repos, prop_name, prop_value)
    assert_equal(prop_value, txn1.root.node_prop(path_in_repos, prop_name))
    assert_equal({prop_name => prop_value},
                 txn1.root.node_proplist(path_in_repos))
    assert(txn1.root.props_changed?(path_in_repos, txn4.root, path_in_repos))
    assert(!txn1.root.props_changed?(path_in_repos, txn1.root, path_in_repos))
    txn1.root.set_node_prop(path_in_repos, prop_name, nil)
    assert_nil(txn1.root.node_prop(path_in_repos, prop_name))
    assert_equal({}, txn1.root.node_proplist(path_in_repos))
  end

  def test_operation
    log = "sample log"
    file = "sample.txt"
    file2 = "sample2.txt"
    file3 = "sample3.txt"
    dir = "sample"
    src = "sample source"
    path_in_repos = "/#{file}"
    path2_in_repos = "/#{file2}"
    path3_in_repos = "/#{file3}"
    dir_path_in_repos = "/#{dir}"
    path = File.join(@wc_path, file)
    path2 = File.join(@wc_path, file2)
    path3 = File.join(@wc_path, file3)
    dir_path = File.join(@wc_path, dir)
    token = @fs.generate_lock_token
    make_context(log) do |ctx|

      @fs.transaction do |txn|
        txn.root.make_file(file)
        txn.root.make_dir(dir)
      end
      ctx.up(@wc_path)
      assert(File.exist?(path))
      assert(File.directory?(dir_path))

      @fs.transaction do |txn|
        txn.root.copy(file2, @fs.root, file)
        txn.root.delete(file)
        txn.abort
      end
      ctx.up(@wc_path)
      assert(File.exist?(path))
      assert(!File.exist?(path2))

      @fs.transaction do |txn|
        txn.root.copy(file2, @fs.root, file)
        txn.root.delete(file)
      end
      ctx.up(@wc_path)
      assert(!File.exist?(path))
      assert(File.exist?(path2))

      prev_root = @fs.root(youngest_rev - 1)
      assert(!prev_root.contents_changed?(file, @fs.root, file2))
      File.open(path2, "w") {|f| f.print(src)}
      ctx.ci(@wc_path)
      assert(prev_root.contents_changed?(file, @fs.root, file2))

      txn1 = @fs.transaction
      access = Svn::Fs::Access.new(@author)
      @fs.access = access
      @fs.access.add_lock_token(token)
      assert_equal([], @fs.get_locks(file2))
      lock = @fs.lock(file2)
      assert_equal(lock.token, @fs.get_lock(file2).token)
      assert_equal([lock.token],
                   @fs.get_locks(file2).collect{|l| l.token})
      @fs.unlock(file2, lock.token)
      assert_equal([], @fs.get_locks(file2))

      entries = @fs.root.dir_entries("/")
      assert_equal([file2, dir].sort, entries.keys.sort)
      assert_equal(@fs.root.node_id(path2_in_repos).to_s,
                   entries[file2].id.to_s)
      assert_equal(@fs.root.node_id(dir_path_in_repos).to_s,
                   entries[dir].id.to_s)

      @fs.transaction do |txn|
        prev_root = @fs.root(youngest_rev - 2)
        txn.root.revision_link(prev_root, file)
      end
      ctx.up(@wc_path)
      assert(File.exist?(path))

      closest_root, closet_path = @fs.root.closest_copy(file2)
      assert_equal(path2_in_repos, closet_path)
    end
  end

  def test_delta(use_deprecated_api=false)
    log = "sample log"
    file = "source.txt"
    src = "a\nb\nc\nd\ne\n"
    modified = "A\nb\nc\nd\nE\n"
    result = "a\n\n\n\ne\n"
    expected = "A\n\n\n\nE\n"
    path_in_repos = "/#{file}"
    path = File.join(@wc_path, file)

    make_context(log) do |ctx|

      File.open(path, "w") {|f| f.print(src)}
      ctx.add(path)
      rev1 = ctx.ci(@wc_path).revision

      File.open(path, "w") {|f| f.print(modified)}
      @fs.transaction do |txn|
        checksum = MD5.new(normalize_line_break(result)).hexdigest
        stream = txn.root.apply_text(path_in_repos, checksum)
        stream.write(normalize_line_break(result))
        stream.close
      end
      ctx.up(@wc_path)
      assert_equal(expected, File.open(path){|f| f.read})

      rev2 = ctx.ci(@wc_path).revision
      if use_deprecated_api
        stream = @fs.root(rev2).file_delta_stream(@fs.root(rev1),
                                                  path_in_repos,
                                                  path_in_repos)
      else
        stream = @fs.root(rev1).file_delta_stream(path_in_repos,
                                                  @fs.root(rev2),
                                                  path_in_repos)
      end

      data = ''
      stream.each{|w| data << w.new_data}
      assert_equal(normalize_line_break(expected), data)

      File.open(path, "w") {|f| f.print(src)}
      rev3 = ctx.ci(@wc_path).revision

      File.open(path, "w") {|f| f.print(modified)}
      @fs.transaction do |txn|
        base_checksum = MD5.new(normalize_line_break(src)).hexdigest
        checksum = MD5.new(normalize_line_break(result)).hexdigest
        handler = txn.root.apply_textdelta(path_in_repos,
                                           base_checksum, checksum)
        assert_raises(Svn::Error::ChecksumMismatch) do
          handler.call(nil)
        end
      end
    end
  end

  def test_delta_with_deprecated_api
    test_delta(true)
  end

  def test_prop
    log = "sample log"
    make_context(log) do |ctx|
      ctx.checkout(@repos_uri, @wc_path)
      ctx.mkdir(["#{@wc_path}/new_dir"])

      start_time = Time.now
      info = ctx.commit([@wc_path])

      assert_equal(@author, info.author)
      assert_equal(@fs.youngest_rev, info.revision)
      assert_operator(start_time..(Time.now), :include?, info.date)

      assert_equal(@author, @fs.prop(Svn::Core::PROP_REVISION_AUTHOR))
      assert_equal(log, @fs.prop(Svn::Core::PROP_REVISION_LOG))
      assert_equal([
                     Svn::Core::PROP_REVISION_AUTHOR,
                     Svn::Core::PROP_REVISION_DATE,
                     Svn::Core::PROP_REVISION_LOG,
                   ].sort,
                   @fs.proplist.keys.sort)
      @fs.set_prop(Svn::Core::PROP_REVISION_LOG, nil)
      assert_nil(@fs.prop(Svn::Core::PROP_REVISION_LOG))
      assert_equal([
                     Svn::Core::PROP_REVISION_AUTHOR,
                     Svn::Core::PROP_REVISION_DATE,
                   ].sort,
                   @fs.proplist.keys.sort)
    end
  end

  def assert_recover
    path = File.join(@tmp_path, "fs")
    fs_type = Svn::Fs::TYPE_FSFS
    config = {Svn::Fs::CONFIG_FS_TYPE => fs_type}

    yield(:create, [path, config], Proc.new{})

    assert_nothing_raised do
      yield(:recover, [path], Proc.new{})
    end
  end

  def test_recover_for_backward_compatibility
    assert_recover do |method, args, block|
      Svn::Fs::FileSystem.__send__(method, *args, &block)
    end
  end

  def test_recover
    assert_recover do |method, args, block|
      Svn::Fs.__send__(method, *args, &block)
    end
  end

  def test_deleted_revision
    file = "file"
    log = "sample log"
    path = File.join(@wc_path, file)
    path_in_repos = "/#{file}"
    make_context(log) do |ctx|

      FileUtils.touch(path)
      ctx.add(path)
      rev1 = ctx.ci(@wc_path).revision

      ctx.rm_f(path)
      rev2 = ctx.ci(@wc_path).revision

      FileUtils.touch(path)
      ctx.add(path)
      rev3 = ctx.ci(@wc_path).revision

      ctx.rm_f(path)
      rev4 = ctx.ci(@wc_path).revision

      assert_equal(Svn::Core::INVALID_REVNUM,
                   @fs.deleted_revision(path_in_repos, 0, rev4))
      assert_equal(rev2, @fs.deleted_revision(path_in_repos, rev1, rev4))
      assert_equal(Svn::Core::INVALID_REVNUM,
                   @fs.deleted_revision(path_in_repos, rev2, rev4))
      assert_equal(rev4, @fs.deleted_revision(path_in_repos, rev3, rev4))
      assert_equal(Svn::Core::INVALID_REVNUM,
                   @fs.deleted_revision(path_in_repos, rev4, rev4))
    end
  end

  def test_mergeinfo
    log = "sample log"
    file = "sample.txt"
    src = "sample\n"
    trunk = File.join(@wc_path, "trunk")
    branch = File.join(@wc_path, "branch")
    trunk_path = File.join(trunk, file)
    branch_path = File.join(branch, file)
    trunk_in_repos = "/trunk"
    branch_in_repos = "/branch"

    make_context(log) do |ctx|
      ctx.mkdir(trunk, branch)
      File.open(trunk_path, "w") {}
      File.open(branch_path, "w") {}
      ctx.add(trunk_path)
      ctx.add(branch_path)
      rev1 = ctx.commit(@wc_path).revision

      File.open(branch_path, "w") {|f| f.print(src)}
      rev2 = ctx.commit(@wc_path).revision

      assert_equal({}, @fs.root.mergeinfo(trunk_in_repos))
      ctx.merge(branch, rev1, branch, rev2, trunk)
      assert_equal({}, @fs.root.mergeinfo(trunk_in_repos))

      rev3 = ctx.commit(@wc_path).revision
      mergeinfo = Svn::Core::MergeInfo.parse("#{branch_in_repos}:2")
      assert_equal({trunk_in_repos => mergeinfo},
                   @fs.root.mergeinfo(trunk_in_repos))

      ctx.rm(branch_path)
      rev4 = ctx.commit(@wc_path).revision

      ctx.merge(branch, rev3, branch, rev4, trunk)
      assert(!File.exist?(trunk_path))
      rev5 = ctx.commit(@wc_path).revision
      assert_equal({trunk_in_repos => Svn::Core::MergeInfo.parse("#{branch_in_repos}:2,4")},
                   @fs.root.mergeinfo(trunk_in_repos))
    end
  end
end
