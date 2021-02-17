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

require "svn/ra"

class SvnRaTest < Test::Unit::TestCase
  include SvnTestUtil

  def setup
    setup_basic
  end

  def teardown
    teardown_basic
  end

  def test_version
    assert_equal(Svn::Core.subr_version, Svn::Ra.version)
  end

  def test_uuid
    Svn::Ra::Session.open(@repos_uri) do |session|
      assert_equal(File.read(File.join(@repos_path, "db", "uuid")).strip,
                   session.uuid)
    end
  end

  def test_open_without_callback
    assert_nothing_raised do
      Svn::Ra::Session.open(@repos_uri).close
    end
  end

  def test_session
    log = "sample log"
    log2 = "sample log2"
    file = "sample.txt"
    src = "sample source"
    path = File.join(@wc_path, file)

    session = nil
    rev1 = nil
    path_props = nil

    make_context(log) do |ctx0|
      config = {}
      path_props = {"my-prop" => "value"}
      callbacks = Svn::Ra::Callbacks.new(ctx0.auth_baton)
      Svn::Ra::Session.open(@repos_uri, config,callbacks) do |session|

        assert_equal(youngest_rev, session.latest_revnum)
        assert_equal(@repos_uri, session.repos_root)

        File.open(path, "w") {|f| f.print(src)}
        ctx0.add(path)
        info = ctx0.ci(@wc_path)
        rev1 = info.revision

        assert_equal(info.revision, session.dated_revision(info.date))
        content, props = session.file(file, info.revision)
        assert_equal(src, content)
        assert_equal([
                       Svn::Core::PROP_ENTRY_COMMITTED_DATE,
                       Svn::Core::PROP_ENTRY_UUID,
                       Svn::Core::PROP_ENTRY_LAST_AUTHOR,
                       Svn::Core::PROP_ENTRY_COMMITTED_REV,
                     ].sort,
                     props.keys.sort)
        entries, props = session.dir("", info.revision)
        assert_equal([file], entries.keys)
        assert(entries[file].file?)
        assert_equal([
                       Svn::Core::PROP_ENTRY_COMMITTED_DATE,
                       Svn::Core::PROP_ENTRY_UUID,
                       Svn::Core::PROP_ENTRY_LAST_AUTHOR,
                       Svn::Core::PROP_ENTRY_COMMITTED_REV,
                     ].sort,
                     props.keys.sort)

        entries, props = session.dir("", info.revision, Svn::Core::DIRENT_KIND)
        assert_equal(Svn::Core::NODE_FILE, entries[file].kind)
        entries, props = session.dir("", info.revision, 0)
        assert_equal(Svn::Core::NODE_NONE, entries[file].kind)

        make_context(log2) do |ctx|
          File.open(path, "w") {|f| f.print(src * 2)}
          path_props.each do |key, value|
            ctx.prop_set(key, value, path)
          end
          info = ctx.ci(@wc_path)
          rev2 = info.revision

          logs = []
          receiver = Proc.new do |changed_paths, revision, author, date, message|
            logs << [revision, message]
          end
          session.log([file], rev1, rev2, rev2 - rev1 + 1, &receiver)
          assert_equal([
                         [rev1, log],
                         [rev2, log2],
                       ].sort_by {|rev, log| rev},
                       logs.sort_by {|rev, log| rev})

          assert_equal(Svn::Core::NODE_FILE, session.check_path(file))
          assert_equal(Svn::Core::NODE_FILE, session.stat(file).kind)

          assert_equal({
                         rev1 => "/#{file}",
                         rev2 => "/#{file}",
                       },
                       session.locations(file, [rev1, rev2]))

          infos = []
          session.file_revs(file, rev1, rev2) do |_path, rev, rev_props, prop_diffs|
            hashed_prop_diffs = {}
            prop_diffs.each do |prop|
              hashed_prop_diffs[prop.name] = prop.value
            end
            infos << [rev, _path, hashed_prop_diffs]
          end
          assert_equal([
                         [rev1, "/#{file}", {}],
                         [rev2, "/#{file}", path_props],
                       ],
                       infos)

          infos = []
          session.file_revs2(file, rev1, rev2) do |_path, rev, rev_props, prop_diffs|
            infos << [rev, _path, prop_diffs]
          end
          assert_equal([
                         [rev1, "/#{file}", {}],
                         [rev2, "/#{file}", path_props],
                       ],
                       infos)

          assert_equal({}, session.get_locks(""))
          locks = []
          session.lock({file => rev2}) do |_path, do_lock, lock, ra_err|
            locks << [_path, do_lock, lock, ra_err]
          end
          assert_equal([file],
                       locks.collect{|_path, *rest| _path}.sort)
          lock = locks.assoc(file)[2]
          assert_equal(["/#{file}"],
                       session.get_locks("").collect{|_path, *rest| _path})
          assert_equal(lock.token, session.get_lock(file).token)
          assert_equal([lock.token],
                       session.get_locks(file).values.collect{|l| l.token})
          session.unlock({file => lock.token})
          assert_equal({}, session.get_locks(file))
        end
      end
    end
  end

  def assert_property_access
    log = "sample log"
    file = "sample.txt"
    path = File.join(@wc_path, file)
    make_context(log) do |ctx|
      config = {}
      callbacks = Svn::Ra::Callbacks.new(ctx.auth_baton)
      Svn::Ra::Session.open(@repos_uri, config, callbacks) do |session|

        FileUtils.touch(path)
        ctx.add(path)
        info = ctx.commit(@wc_path)

        assert_equal(@author, yield(session, :get, Svn::Core::PROP_REVISION_AUTHOR))
        assert_equal(log, yield(session, :get, Svn::Core::PROP_REVISION_LOG))
        assert_equal([
                       Svn::Core::PROP_REVISION_AUTHOR,
                       Svn::Core::PROP_REVISION_DATE,
                       Svn::Core::PROP_REVISION_LOG,
                     ].sort,
                     yield(session, :list).keys.sort)
        yield(session, :set, Svn::Core::PROP_REVISION_LOG, nil)
        assert_nil(yield(session, :get ,Svn::Core::PROP_REVISION_LOG))
        assert_equal([
                       Svn::Core::PROP_REVISION_AUTHOR,
                       Svn::Core::PROP_REVISION_DATE,
                     ].sort,
                     yield(session, :list).keys.sort)
      end
    end
  end

  def test_prop
    assert_property_access do |session, action, *args|
      case action
      when :get
        key, = args
        session[key]
      when :set
        key, value = args
        session[key] = value
      when :list
        session.properties
      end
    end
  end

  def test_prop_with_no_ruby_way
    assert_property_access do |session, action, *args|
      case action
      when :get
        key, = args
        session.prop(key)
      when :set
        key, value = args
        session.set_prop(key, value)
      when :list
        session.proplist
      end
    end
  end

  def test_callback
    log = "sample log"
    file = "sample.txt"
    src1 = "sample source1"
    src2 = "sample source2"
    path = File.join(@wc_path, file)
    path_in_repos = "/#{file}"
    make_context(log) do |ctx|
      config = {}
      callbacks = Svn::Ra::Callbacks.new(ctx.auth_baton)
      Svn::Ra::Session.open(@repos_uri, config, callbacks) do |session|

        File.open(path, "w") {|f| f.print(src1)}
        ctx.add(path)
        rev1 = ctx.ci(@wc_path).revision

        File.open(path, "w") {|f| f.print(src2)}
        rev2 = ctx.ci(@wc_path).revision

        ctx.up(@wc_path)

        editor, editor_baton = session.commit_editor(log) {}
        reporter = session.update(rev2, "", editor, editor_baton)
        reporter.abort_report

        editor, editor_baton = session.commit_editor(log) {}
        reporter = session.update2(rev2, "", editor)
        reporter.abort_report
      end
    end
  end

  def test_diff
    log = "sample log"
    file = "sample.txt"
    src1 = "a\nb\nc\nd\ne\n"
    src2 = "a\nb\nC\nd\ne\n"
    path = File.join(@wc_path, file)
    path_in_repos = "/#{file}"
    make_context(log) do |ctx|
      config = {}
      callbacks = Svn::Ra::Callbacks.new(ctx.auth_baton)
      Svn::Ra::Session.open(@repos_uri, config, callbacks) do |session|

        File.open(path, "w") {|f| f.print(src1)}
        ctx.add(path)
        rev1 = ctx.ci(@wc_path).revision

        File.open(path, "w") {|f| f.print(src2)}
        rev2 = ctx.ci(@wc_path).revision

        ctx.up(@wc_path)

        editor = Svn::Delta::BaseEditor.new # dummy
        reporter = session.diff(rev2, "", @repos_uri, editor)
        reporter.set_path("", rev1, false, nil)
        reporter.finish_report
      end
    end
  end

  def test_commit_editor
    log = "sample log"
    dir_uri = "/test"
    config = {}
    make_context(log) do |ctx|
      callbacks = Svn::Ra::Callbacks.new(ctx.auth_baton)
      Svn::Ra::Session.open(@repos_uri, config, callbacks) do |session|
        actual_info = nil
        actual_date = nil

        expected_info = [1, @author]
        start_time = Time.now
        gc_disable do
          editor, baton = session.commit_editor(log) do |rev, date, author|
            actual_info = [rev, author]
            actual_date = date
          end
          editor.baton = baton

          root = editor.open_root(-1)
          editor.add_directory(dir_uri, root, nil, -1)
          gc_enable do
            GC.start
            editor.close_edit
          end
          assert_equal(expected_info, actual_info)
          assert_operator(start_time..(Time.now), :include?, actual_date)
        end
      end
    end
  end

  def test_commit_editor2
    log = "sample log"
    dir_uri = "/test"
    config = {}
    make_context(log) do |ctx|
      callbacks = Svn::Ra::Callbacks.new(ctx.auth_baton)
      Svn::Ra::Session.open(@repos_uri, config, callbacks) do |session|
        actual_info = nil
        actual_date = nil

        expected_info = [1, @author]
        start_time = Time.now
        gc_disable do
          editor = session.commit_editor2(log) do |info|
            actual_info = [info.revision, info.author]
            actual_date = info.date
          end

          root = editor.open_root(-1)
          editor.add_directory(dir_uri, root, nil, -1)
          gc_enable do
            GC.start
            editor.close_edit
          end
          assert_equal(expected_info, actual_info)
          assert_operator(start_time..(Time.now), :include?, actual_date)
        end
      end
    end
  end

  def test_reparent
    log = "sample log"
    dir = "dir"
    deep_dir = "deep"
    dir_path = File.join(@wc_path, dir)
    deep_dir_path = File.join(dir_path, deep_dir)
    config = {}
    make_context(log) do |ctx|

      ctx.mkdir(dir_path)
      ctx.ci(@wc_path)

      ctx.mkdir(deep_dir_path)
      ctx.ci(@wc_path)

      callbacks = Svn::Ra::Callbacks.new(ctx.auth_baton)
      Svn::Ra::Session.open(@repos_uri, config, callbacks) do |session|

        entries, props = session.dir(dir, nil)
        assert_equal([deep_dir], entries.keys)
        assert_raise(Svn::Error::FS_NOT_FOUND) do
          session.dir(deep_dir)
        end

        session.reparent("#{@repos_uri}/#{dir}")
        assert_raise(Svn::Error::FS_NOT_FOUND) do
          session.dir(dir)
        end
        entries, props = session.dir(deep_dir)
        assert_equal([], entries.keys)

        assert_raise(Svn::Error::RA_ILLEGAL_URL) do
          session.reparent("file:///tmp/xxx")
        end
      end
    end
  end

  def test_mergeinfo
    log = "sample log"
    file = "sample.txt"
    src = "sample\n"
    config = {}
    trunk = File.join(@wc_path, "trunk")
    branch = File.join(@wc_path, "branch")
    trunk_path = File.join(trunk, file)
    branch_path = File.join(branch, file)
    session_relative_trunk_path = "trunk"
    repository_branch_path = "/branch"

    make_context(log) do |ctx|
      ctx.mkdir(trunk, branch)
      File.open(trunk_path, "w") {}
      File.open(branch_path, "w") {}
      ctx.add(trunk_path)
      ctx.add(branch_path)
      rev1 = ctx.commit(@wc_path).revision

      File.open(branch_path, "w") {|f| f.print(src)}
      rev2 = ctx.commit(@wc_path).revision

      callbacks = Svn::Ra::Callbacks.new(ctx.auth_baton)
      Svn::Ra::Session.open(@repos_uri, config, callbacks) do |session|

        assert_nil(session.mergeinfo(session_relative_trunk_path))
        ctx.merge(branch, rev1, branch, rev2, trunk)
        assert_nil(session.mergeinfo(session_relative_trunk_path))

        rev3 = ctx.commit(@wc_path).revision
        mergeinfo = session.mergeinfo(session_relative_trunk_path)
        assert_equal([session_relative_trunk_path], mergeinfo.keys)
        trunk_mergeinfo = mergeinfo[session_relative_trunk_path]
        assert_equal([repository_branch_path], trunk_mergeinfo.keys)
        assert_equal([[1, 2, true]],
                     trunk_mergeinfo[repository_branch_path].collect {|range|
                        range.to_a})

        ctx.rm(branch_path)
        rev4 = ctx.commit(@wc_path).revision

        ctx.merge(branch, rev3, branch, rev4, trunk)
        assert(!File.exist?(trunk_path))
        rev5 = ctx.commit(@wc_path).revision

        mergeinfo = session.mergeinfo(session_relative_trunk_path, rev5)
        assert_equal([session_relative_trunk_path], mergeinfo.keys)
        trunk_mergeinfo = mergeinfo[session_relative_trunk_path]
        assert_equal([repository_branch_path], trunk_mergeinfo.keys)
        assert_equal([[1, 2, true], [3, 4, true]],
                     trunk_mergeinfo[repository_branch_path].collect {|range|
                        range.to_a})
      end
    end
  end
end
