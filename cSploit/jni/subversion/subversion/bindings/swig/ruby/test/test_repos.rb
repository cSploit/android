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

require "tempfile"

require "my-assertions"
require "util"

require "svn/core"
require "svn/fs"
require "svn/repos"
require "svn/client"

class SvnReposTest < Test::Unit::TestCase
  include SvnTestUtil

  def setup
    setup_basic
  end

  def teardown
    teardown_basic
  end

  def test_version
    assert_equal(Svn::Core.subr_version, Svn::Repos.version)
  end

  def test_path
    assert_equal(@repos_path, @repos.path)

    assert_equal(File.join(@repos_path, "db"), @repos.db_env)

    assert_equal(File.join(@repos_path, "conf"), @repos.conf_dir)
    assert_equal(File.join(@repos_path, "conf", "svnserve.conf"),
                 @repos.svnserve_conf)

    locks_dir = File.join(@repos_path, "locks")
    assert_equal(locks_dir, @repos.lock_dir)
    assert_equal(File.join(locks_dir, "db.lock"),
                 @repos.db_lockfile)
    assert_equal(File.join(locks_dir, "db-logs.lock"),
                 @repos.db_logs_lockfile)

    hooks_dir = File.join(@repos_path, "hooks")
    assert_equal(hooks_dir, @repos.hook_dir)

    assert_equal(File.join(hooks_dir, "start-commit"),
                 @repos.start_commit_hook)
    assert_equal(File.join(hooks_dir, "pre-commit"),
                 @repos.pre_commit_hook)
    assert_equal(File.join(hooks_dir, "post-commit"),
                 @repos.post_commit_hook)

    assert_equal(File.join(hooks_dir, "pre-revprop-change"),
                 @repos.pre_revprop_change_hook)
    assert_equal(File.join(hooks_dir, "post-revprop-change"),
                 @repos.post_revprop_change_hook)

    assert_equal(File.join(hooks_dir, "pre-lock"),
                 @repos.pre_lock_hook)
    assert_equal(File.join(hooks_dir, "post-lock"),
                 @repos.post_lock_hook)

    assert_equal(File.join(hooks_dir, "pre-unlock"),
                 @repos.pre_unlock_hook)
    assert_equal(File.join(hooks_dir, "post-unlock"),
                 @repos.post_unlock_hook)


    search_path = @repos_path
    assert_equal(@repos_path, Svn::Repos.find_root_path(search_path))
    search_path = "#{@repos_path}/XXX"
    assert_equal(@repos_path, Svn::Repos.find_root_path(search_path))

    search_path = "not-found"
    assert_equal(nil, Svn::Repos.find_root_path(search_path))
  end

  def test_create
    tmp_repos_path = File.join(@tmp_path, "repos")
    fs_type = Svn::Fs::TYPE_FSFS
    fs_config = {Svn::Fs::CONFIG_FS_TYPE => fs_type}
    repos = nil
    Svn::Repos.create(tmp_repos_path, {}, fs_config) do |repos|
      assert(File.exist?(tmp_repos_path))
      fs_type_path = File.join(repos.fs.path, Svn::Fs::CONFIG_FS_TYPE)
      assert_equal(fs_type, File.open(fs_type_path) {|f| f.read.chop})
      repos.fs.set_warning_func(&warning_func)
    end

    assert(repos.closed?)
    assert_raises(Svn::Error::ReposAlreadyClose) do
      repos.fs
    end

    Svn::Repos.delete(tmp_repos_path)
    assert(!File.exist?(tmp_repos_path))
  end

  def test_logs
    log1 = "sample log1"
    log2 = "sample log2"
    log3 = "sample log3"
    file = "file"
    src = "source"
    props = {"myprop" => "value"}
    path = File.join(@wc_path, file)

    ctx1 = make_context(log1)
    File.open(path, "w") {|f| f.print(src)}
    ctx1.add(path)
    info1 = ctx1.ci(@wc_path)
    start_rev = info1.revision

    ctx2 = make_context(log2)
    File.open(path, "a") {|f| f.print(src)}
    info2 = ctx2.ci(@wc_path)

    ctx3 = make_context(log3)
    File.open(path, "a") {|f| f.print(src)}
    props.each do |key, value|
      ctx3.prop_set(key, value, path)
    end
    info3 = ctx3.ci(@wc_path)
    end_rev = info3.revision

    logs = @repos.logs(file, start_rev, end_rev, end_rev - start_rev + 1)
    logs = logs.collect do |changed_paths, revision, author, date, message|
      paths = {}
      changed_paths.each do |key, changed_path|
        paths[key] = changed_path.action
      end
      [paths, revision, author, date, message]
    end
    assert_equal([
                   [
                     {"/#{file}" => "A"},
                     info1.revision,
                     @author,
                     info1.date,
                     log1,
                   ],
                   [
                     {"/#{file}" => "M"},
                     info2.revision,
                     @author,
                     info2.date,
                     log2,
                   ],
                   [
                     {"/#{file}" => "M"},
                     info3.revision,
                     @author,
                     info3.date,
                     log3,
                   ],
                 ],
                 logs)
    revs = []
    args = [file, start_rev, end_rev]
    @repos.file_revs(*args) do |path, rev, rev_props, prop_diffs|
      hashed_prop_diffs = {}
      prop_diffs.each do |prop|
        hashed_prop_diffs[prop.name] = prop.value
      end
      revs << [path, rev, hashed_prop_diffs]
    end
    assert_equal([
                   ["/#{file}", info1.revision, {}],
                   ["/#{file}", info2.revision, {}],
                   ["/#{file}", info3.revision, props],
                 ],
                 revs)

    revs = []
    @repos.file_revs2(*args) do |path, rev, rev_props, prop_diffs|
      revs << [path, rev, prop_diffs]
    end
    assert_equal([
                   ["/#{file}", info1.revision, {}],
                   ["/#{file}", info2.revision, {}],
                   ["/#{file}", info3.revision, props],
                 ],
                 revs)


    rev, date, author = @repos.fs.root.committed_info("/")
    assert_equal(info3.revision, rev)
    assert_equal(info3.date, date)
    assert_equal(info3.author, author)
  ensure
    ctx3.destroy unless ctx3.nil?
    ctx2.destroy unless ctx2.nil?
    ctx1.destroy unless ctx1.nil?
  end

  def test_hotcopy
    log = "sample log"
    file = "hello.txt"
    path = File.join(@wc_path, file)
    FileUtils.touch(path)

    # So we can later rename files when running the tests on
    # Windows, close access to the repos created by the test setup.
    test_repos_path = @repos.path
    @repos.close
    @repos = nil
    @fs.close
    @fs = nil

    rev = make_context(log) do |ctx|
      ctx.add(path)
      commit_info = ctx.commit(@wc_path)
      rev = commit_info.revision

      assert_equal(log, ctx.log_message(path, rev))
      rev
    end

    dest_path = File.join(@tmp_path, "dest")
    backup_path = File.join(@tmp_path, "back")
    config = {}
    fs_config = {}

    dest_repos = Svn::Repos.create(dest_path, config, fs_config)
    dest_repos.fs.set_warning_func(&warning_func)
    dest_repos_path = dest_repos.path
    dest_repos.close

    FileUtils.mv(test_repos_path, backup_path)
    FileUtils.mv(dest_repos_path, test_repos_path)

    make_context(log) do |ctx|
      assert_raises(Svn::Error::FsNoSuchRevision) do
        assert_equal(log, ctx.log_message(path, rev))
      end

      FileUtils.rm_r(test_repos_path)
      Svn::Repos.hotcopy(backup_path, test_repos_path)
      assert_equal(log, ctx.log_message(path, rev))
    end
  end

  def assert_transaction
    log = "sample log"
    make_context(log) do |ctx|
      ctx.checkout(@repos_uri, @wc_path)
      ctx.mkdir(["#{@wc_path}/new_dir"])

      prev_rev = @repos.youngest_rev
      past_date = Time.now
      args = {
        :author => @author,
        :log => log,
        :revision => prev_rev,
      }
      callback = Proc.new do |txn|
        txn.abort
      end
      yield(:commit, @repos, args, callback)
      assert_equal(prev_rev, @repos.youngest_rev)
      assert_equal(prev_rev, @repos.dated_revision(past_date))

      prev_rev = @repos.youngest_rev
      @repos.transaction_for_commit(@author, log) do |txn|
      end
      assert_equal(prev_rev + 1, @repos.youngest_rev)
      assert_equal(prev_rev, @repos.dated_revision(past_date))
      assert_equal(prev_rev + 1, @repos.dated_revision(Time.now))

      prev_rev = @repos.youngest_rev
      args = {
        :author => @author,
        :revision => prev_rev,
      }
      callback = Proc.new do |txn|
      end
      yield(:update, @repos, args, callback)
      assert_equal(prev_rev, @repos.youngest_rev)
    end
  end

  def test_transaction
    assert_transaction do |type, repos, args, callback|
      case type
      when :commit
        repos.transaction_for_commit(args[:author], args[:log], &callback)
      when :update
        repos.transaction_for_update(args[:author], &callback)
      end
    end
  end

  def test_transaction_with_revision
    assert_transaction do |type, repos, args, callback|
      case type
      when :commit
        repos.transaction_for_commit(args[:author], args[:log],
                                     args[:revision], &callback)
      when :update
        repos.transaction_for_update(args[:author], args[:revision], &callback)
      end
    end
  end

  def test_transaction2
    assert_transaction do |type, repos, args, callback|
      case type
      when :commit
        props = {
          Svn::Core::PROP_REVISION_AUTHOR => args[:author],
          Svn::Core::PROP_REVISION_LOG => args[:log],
        }
        repos.transaction_for_commit(props, &callback)
      when :update
        repos.transaction_for_update(args[:author], &callback)
      end
    end
  end

  def test_transaction2_with_revision
    assert_transaction do |type, repos, args, callback|
      case type
      when :commit
        props = {
          Svn::Core::PROP_REVISION_AUTHOR => args[:author],
          Svn::Core::PROP_REVISION_LOG => args[:log],
        }
        repos.transaction_for_commit(props,
                                     args[:revision],
                                     &callback)
      when :update
        repos.transaction_for_update(args[:author],
                                     args[:revision],
                                     &callback)
      end
    end
  end

  def test_trace_node_locations
    file1 = "file1"
    file2 = "file2"
    file3 = "file3"
    path1 = File.join(@wc_path, file1)
    path2 = File.join(@wc_path, file2)
    path3 = File.join(@wc_path, file3)
    log = "sample log"
    make_context(log) do |ctx|

      FileUtils.touch(path1)
      ctx.add(path1)
      rev1 = ctx.ci(@wc_path).revision

      ctx.mv(path1, path2)
      rev2 = ctx.ci(@wc_path).revision

      ctx.cp(path2, path3)
      rev3 = ctx.ci(@wc_path).revision

      assert_equal({
                     rev1 => "/#{file1}",
                     rev2 => "/#{file2}",
                     rev3 => "/#{file2}",
                   },
                   @repos.fs.trace_node_locations("/#{file2}",
                                                  [rev1, rev2, rev3]))
    end
  end

  def assert_report
    file = "file"
    fs_base = "/"
    path = File.join(@wc_path, file)
    source = "sample source"
    log = "sample log"
    make_context(log) do |ctx|

      File.open(path, "w") {|f| f.print(source)}
      ctx.add(path)
      rev = ctx.ci(@wc_path).revision

      assert_equal(Svn::Core::NODE_FILE, @repos.fs.root.stat(file).kind)

      editor = TestEditor.new
      args = {
        :revision => rev,
        :user_name => @author,
        :fs_base => fs_base,
        :target => "",
        :target_path => nil,
        :editor => editor,
        :text_deltas => true,
        :recurse => true,
        :ignore_ancestry => false,
      }
      callback = Proc.new do |baton|
      end
      yield(@repos, args, callback)
      editor_seq = editor.sequence.collect{|meth, *args| meth}

      # Delete change_file_prop() and change_dir_prop() calls from the
      # actual editor sequence, as they can vary in ways not easily
      # determined by a black-box test author.
      editor_seq.delete(:change_file_prop)
      editor_seq.delete(:change_dir_prop)

      assert_equal([
                     :set_target_revision,
                     :open_root,
                     :close_directory,
                     :close_edit,
                   ],
                   editor_seq)
    end
  end

  def test_report
    assert_report do |repos, args, callback|
      @repos.report(args[:revision], args[:user_name], args[:fs_base],
                    args[:target], args[:target_path], args[:editor],
                    args[:text_deltas], args[:recurse], args[:ignore_ancestry],
                    &callback)
    end
  end

  def test_report2
    assert_report do |repos, args, callback|
      if args[:recurse]
        depth = Svn::Core::DEPTH_INFINITY
      else
        depth = Svn::Core::DEPTH_FILES
      end
      @repos.report2(args[:revision], args[:fs_base], args[:target],
                     args[:target_path], args[:editor], args[:text_deltas],
                     args[:ignore_ancestry], depth, &callback)
    end
  end

  def assert_commit_editor
    trunk = "trunk"
    tags = "tags"
    tags_sub = "sub"
    file = "file"
    source = "sample source"
    user = "user"
    log_message = "log"
    trunk_dir_path = File.join(@wc_path, trunk)
    tags_dir_path = File.join(@wc_path, tags)
    tags_sub_dir_path = File.join(tags_dir_path, tags_sub)
    trunk_path = File.join(trunk_dir_path, file)
    tags_path = File.join(tags_dir_path, file)
    tags_sub_path = File.join(tags_sub_dir_path, file)
    trunk_repos_uri = "#{@repos_uri}/#{trunk}"
    rev1 = @repos.youngest_rev

    commit_callback_result = {}
    args = {
      :repos_url => @repos_uri,
      :base_path => "/",
      :user => user,
      :log_message => log_message,
     }

    editor = yield(@repos, commit_callback_result, args)
    root_baton = editor.open_root(rev1)
    dir_baton = editor.add_directory(trunk, root_baton, nil, rev1)
    file_baton = editor.add_file("#{trunk}/#{file}", dir_baton, nil, -1)
    ret = editor.apply_textdelta(file_baton, nil)
    ret.send(source)
    editor.close_edit

    assert_equal(rev1 + 1, @repos.youngest_rev)
    assert_equal({
                   :revision => @repos.youngest_rev,
                   :date => @repos.prop(Svn::Core::PROP_REVISION_DATE),
                   :author => user,
                 },
                 commit_callback_result)
    rev2 = @repos.youngest_rev

    make_context("") do |ctx|
      ctx.up(@wc_path)
      assert_equal(source, File.open(trunk_path) {|f| f.read})

      commit_callback_result = {}
      editor = yield(@repos, commit_callback_result, args)
      root_baton = editor.open_root(rev2)
      dir_baton = editor.add_directory(tags, root_baton, nil, rev2)
      subdir_baton = editor.add_directory("#{tags}/#{tags_sub}",
                                          dir_baton,
                                          trunk_repos_uri,
                                          rev2)
      editor.close_edit

      assert_equal(rev2 + 1, @repos.youngest_rev)
      assert_equal({
                     :revision => @repos.youngest_rev,
                     :date => @repos.prop(Svn::Core::PROP_REVISION_DATE),
                     :author => user,
                   },
                   commit_callback_result)
      rev3 = @repos.youngest_rev

      ctx.up(@wc_path)
      assert_equal([
                     ["/#{tags}/#{tags_sub}/#{file}", rev3],
                     ["/#{trunk}/#{file}", rev2],
                   ],
                   @repos.fs.history("#{tags}/#{tags_sub}/#{file}",
                                     rev1, rev3, rev2))

      commit_callback_result = {}
      editor = yield(@repos, commit_callback_result, args)
      root_baton = editor.open_root(rev3)
      dir_baton = editor.delete_entry(tags, rev3, root_baton)
      editor.close_edit

      assert_equal({
                     :revision => @repos.youngest_rev,
                     :date => @repos.prop(Svn::Core::PROP_REVISION_DATE),
                     :author => user,
                   },
                   commit_callback_result)

      ctx.up(@wc_path)
      assert(!File.exist?(tags_path))
    end
  end

  def test_commit_editor
    assert_commit_editor do |receiver, commit_callback_result, args|
      commit_callback = Proc.new do |revision, date, author|
        commit_callback_result[:revision] = revision
        commit_callback_result[:date] = date
        commit_callback_result[:author] = author
      end
      receiver.commit_editor(args[:repos_url], args[:base_path], args[:txn],
                             args[:user], args[:log_message], commit_callback)
    end
  end

  def test_commit_editor2
    assert_commit_editor do |receiver, commit_callback_result, args|
      commit_callback = Proc.new do |info|
        commit_callback_result[:revision] = info.revision
        commit_callback_result[:date] = info.date
        commit_callback_result[:author] = info.author
      end
      receiver.commit_editor2(args[:repos_url], args[:base_path], args[:txn],
                              args[:user], args[:log_message], commit_callback)
    end
  end

  def test_commit_editor3
    assert_commit_editor do |receiver, commit_callback_result, args|
      props = {
        Svn::Core::PROP_REVISION_AUTHOR => args[:user],
        Svn::Core::PROP_REVISION_LOG => args[:log_message],
      }
      commit_callback = Proc.new do |info|
        commit_callback_result[:revision] = info.revision
        commit_callback_result[:date] = info.date
        commit_callback_result[:author] = info.author
      end
      receiver.commit_editor3(args[:repos_url], args[:base_path], args[:txn],
                              props, commit_callback)
    end
  end

  def test_prop
    file = "file"
    path = File.join(@wc_path, file)
    source = "sample source"
    log = "sample log"
    make_context(log) do |ctx|

      File.open(path, "w") {|f| f.print(source)}
      ctx.add(path)
      ctx.ci(@wc_path)

      assert_equal([
                     Svn::Core::PROP_REVISION_AUTHOR,
                     Svn::Core::PROP_REVISION_LOG,
                     Svn::Core::PROP_REVISION_DATE,
                   ].sort,
                   @repos.proplist.keys.sort)
      assert_equal(log, @repos.prop(Svn::Core::PROP_REVISION_LOG))
      @repos.set_prop(@author, Svn::Core::PROP_REVISION_LOG, nil)
      assert_nil(@repos.prop(Svn::Core::PROP_REVISION_LOG))
      assert_equal([
                     Svn::Core::PROP_REVISION_AUTHOR,
                     Svn::Core::PROP_REVISION_DATE,
                   ].sort,
                   @repos.proplist.keys.sort)

      assert_raises(Svn::Error::ReposHookFailure) do
        @repos.set_prop(@author, Svn::Core::PROP_REVISION_DATE, nil)
      end
      assert_not_nil(@repos.prop(Svn::Core::PROP_REVISION_DATE))

      assert_nothing_raised do
        @repos.set_prop(@author, Svn::Core::PROP_REVISION_DATE, nil, nil, nil,
                        false)
      end
      assert_nil(@repos.prop(Svn::Core::PROP_REVISION_DATE))
      assert_equal([
                     Svn::Core::PROP_REVISION_AUTHOR,
                   ].sort,
                   @repos.proplist.keys.sort)
    end
  end

  def test_dump
    file = "file"
    path = File.join(@wc_path, file)
    source = "sample source"
    log = "sample log"
    make_context(log) do |ctx|

      File.open(path, "w") {|f| f.print(source)}
      ctx.add(path)
      rev1 = ctx.ci(@wc_path).revision

      File.open(path, "a") {|f| f.print(source)}
      rev2 = ctx.ci(@wc_path).revision

      assert_nothing_raised do
        @repos.dump_fs(nil, nil, rev1, rev2)
      end

      dump = StringIO.new("")
      feedback = StringIO.new("")
      @repos.dump_fs(dump, feedback, rev1, rev2)

      dump_unless_feedback = StringIO.new("")
      @repos.dump_fs(dump_unless_feedback, nil, rev1, rev2)

      dump.rewind
      dump_unless_feedback.rewind
      assert_equal(dump.read, dump_unless_feedback.read)
    end
  end

  def test_load
    file = "file"
    path = File.join(@wc_path, file)
    source = "sample source"
    log = "sample log"
    make_context(log) do |ctx|

      File.open(path, "w") {|f| f.print(source)}
      ctx.add(path)
      rev1 = ctx.ci(@wc_path).revision

      File.open(path, "a") {|f| f.print(source)}
      rev2 = ctx.ci(@wc_path).revision

      dump = StringIO.new("")
      @repos.dump_fs(dump, nil, rev1, rev2)

      dest_path = File.join(@tmp_path, "dest")
      Svn::Repos.create(dest_path) do |repos|
        assert_raises(NoMethodError) do
          repos.load_fs(nil)
        end
      end

      [
       [StringIO.new(""), Svn::Repos::LOAD_UUID_DEFAULT, "/"],
       [StringIO.new("")],
       [],
      ].each_with_index do |args, i|
        dest_path = File.join(@tmp_path, "dest#{i}")
        Svn::Repos.create(dest_path) do |repos|
          assert_not_equal(@repos.fs.root.committed_info("/"),
                           repos.fs.root.committed_info("/"))
          dump.rewind
          repos.load_fs(dump, *args)
          assert_equal(@repos.fs.root.committed_info("/"),
                       repos.fs.root.committed_info("/"))
        end
      end
    end
  end

  def test_node_editor
    file = "file"
    dir1 = "dir1"
    dir2 = "dir2"
    dir3 = "dir3"
    dir1_path = File.join(@wc_path, dir1)
    dir2_path = File.join(dir1_path, dir2)
    dir3_path = File.join(dir2_path, dir3)
    path = File.join(dir3_path, file)
    source = "sample source"
    log = "sample log"

    make_context(log) do |ctx|
      FileUtils.mkdir_p(dir3_path)
      FileUtils.touch(path)
      ctx.add(dir1_path)
      rev1 = ctx.ci(@wc_path).revision

      ctx.rm(dir3_path)
      rev2 = ctx.ci(@wc_path).revision

      rev1_root = @repos.fs.root(rev1)
      rev2_root = @repos.fs.root(rev2)
      editor = @repos.node_editor(rev1_root, rev2_root)
      rev2_root.replay(editor)

      tree = editor.baton.node

      assert_equal("", tree.name)
      assert_equal(dir1, tree.child.name)
      assert_equal(dir2, tree.child.child.name)
    end
  end

  def test_lock
    file = "file"
    log = "sample log"
    path = File.join(@wc_path, file)
    path_in_repos = "/#{file}"
    make_context(log) do |ctx|

      FileUtils.touch(path)
      ctx.add(path)
      rev = ctx.ci(@wc_path).revision

      access = Svn::Fs::Access.new(@author)
      @repos.fs.access = access
      lock = @repos.lock(file)
      locks = @repos.get_locks(file)
      assert_equal([path_in_repos], locks.keys)
      assert_equal(lock.token, locks[path_in_repos].token)
      @repos.unlock(file, lock.token)
      assert_equal({}, @repos.get_locks(file))
    end
  end

  def test_authz
    name = "REPOS"
    conf_path = File.join(@tmp_path, "authz_file")
    File.open(conf_path, "w") do |f|
      f.print(<<-EOF)
[/]
#{@author} = r
EOF
    end

    authz = Svn::Repos::Authz.read(conf_path)
    assert(authz.can_access?(name, "/", @author, Svn::Repos::AUTHZ_READ))
    assert(!authz.can_access?(name, "/", @author, Svn::Repos::AUTHZ_WRITE))
    assert(!authz.can_access?(name, "/", "FOO", Svn::Repos::AUTHZ_READ))
  end

  def test_recover
    started = false
    Svn::Repos.recover(@repos_path, false) do
      started = true
    end
    assert(started)
  end

  def test_mergeinfo
    log = "sample log"
    file = "sample.txt"
    src = "sample\n"
    trunk = File.join(@wc_path, "trunk")
    branch = File.join(@wc_path, "branch")
    trunk_path = File.join(trunk, file)
    branch_path = File.join(branch, file)
    trunk_path_in_repos = "/trunk/#{file}"
    branch_path_in_repos = "/branch/#{file}"

    make_context(log) do |ctx|
      ctx.mkdir(trunk, branch)
      File.open(trunk_path, "w") {}
      File.open(branch_path, "w") {}
      ctx.add(trunk_path)
      ctx.add(branch_path)
      original_rev = ctx.commit(@wc_path).revision

      File.open(branch_path, "w") {|f| f.print(src)}
      merged_rev = ctx.commit(@wc_path).revision

      ctx.merge(branch, original_rev, branch, merged_rev, trunk)
      ctx.commit(@wc_path)

      mergeinfo = Svn::Core::MergeInfo.parse("#{branch_path_in_repos}:#{merged_rev}")
      assert_equal({trunk_path_in_repos => mergeinfo},
                   @repos.mergeinfo([trunk_path_in_repos]))
      assert_equal(mergeinfo, @repos.mergeinfo(trunk_path_in_repos))
    end
  end

  private
  def warning_func
    Proc.new do |err|
      STDERR.puts err if $DEBUG
    end
  end

  class TestEditor < Svn::Delta::BaseEditor
    attr_reader :sequence
    def initialize
      @sequence = []
    end

    def set_target_revision(target_revision)
      @sequence << [:set_target_revision, target_revision]
    end

    def open_root(base_revision)
      @sequence << [:open_root, base_revision]
    end

    def delete_entry(path, revision, parent_baton)
      @sequence << [:delete_entry, path, revision, parent_baton]
    end

    def add_directory(path, parent_baton,
                      copyfrom_path, copyfrom_revision)
      @sequence << [:add_directory, path, parent_baton,
        copyfrom_path, copyfrom_revision]
    end

    def open_directory(path, parent_baton, base_revision)
      @sequence << [:open_directory, path, parent_baton, base_revision]
    end

    def change_dir_prop(dir_baton, name, value)
      @sequence << [:change_dir_prop, dir_baton, name, value]
    end

    def close_directory(dir_baton)
      @sequence << [:close_directory, dir_baton]
    end

    def absent_directory(path, parent_baton)
      @sequence << [:absent_directory, path, parent_baton]
    end

    def add_file(path, parent_baton,
                 copyfrom_path, copyfrom_revision)
      @sequence << [:add_file, path, parent_baton,
        copyfrom_path, copyfrom_revision]
    end

    def open_file(path, parent_baton, base_revision)
      @sequence << [:open_file, path, parent_baton, base_revision]
    end

    # return nil or object which has `call' method.
    def apply_textdelta(file_baton, base_checksum)
      @sequence << [:apply_textdelta, file_baton, base_checksum]
      nil
    end

    def change_file_prop(file_baton, name, value)
      @sequence << [:change_file_prop, file_baton, name, value]
    end

    def close_file(file_baton, text_checksum)
      @sequence << [:close_file, file_baton, text_checksum]
    end

    def absent_file(path, parent_baton)
      @sequence << [:absent_file, path, parent_baton]
    end

    def close_edit(baton)
      @sequence << [:close_edit, baton]
    end

    def abort_edit(baton)
      @sequence << [:abort_edit, baton]
    end
  end
end
