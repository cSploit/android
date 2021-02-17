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

require "svn/core"
require "svn/wc"
require "svn/repos"
require "svn/ra"

class SvnWcTest < Test::Unit::TestCase
  include SvnTestUtil

  def setup
    setup_basic
  end

  def teardown
    teardown_basic
  end

  def test_version
    assert_equal(Svn::Core.subr_version, Svn::Wc.version)
  end

  def test_status
    file1 = "a"
    file1_path = File.join(@wc_path, file1)

    Svn::Wc::AdmAccess.open(nil, @wc_path, false, 0) do |adm|
      status = adm.status(file1_path)
      assert_equal(Svn::Wc::STATUS_NONE, status.text_status)
      assert_nil(status.entry)
    end

    non_exist_child_path = File.join(@wc_path, "NOT-EXIST")
    assert_nothing_raised do
      Svn::Wc::AdmAccess.probe_open(nil, non_exist_child_path, false, 0){}
    end

    FileUtils.touch(file1_path)
    Svn::Wc::AdmAccess.open(nil, @wc_path, false, 0) do |adm|
      status = adm.status(file1_path)
      assert_equal(Svn::Wc::STATUS_UNVERSIONED, status.text_status)
      assert_nil(status.entry)
    end

    log = "sample log"
    make_context(log) do |ctx|
      ctx.add(file1_path)
      Svn::Wc::AdmAccess.open(nil, @wc_path, false, 0) do |adm|
        status = adm.status(file1_path)
        assert_equal(Svn::Wc::STATUS_ADDED, status.text_status)
      end

      commit_info = ctx.commit(@wc_path)

      Svn::Wc::AdmAccess.open(nil, @wc_path, false, 0) do |adm|
        status = adm.status(file1_path)
        assert_equal(Svn::Wc::STATUS_NORMAL, status.text_status)
        assert_equal(commit_info.revision, status.entry.revision)
      end
    end
  end

  def test_wc
    assert_not_equal(0, Svn::Wc.check_wc(@wc_path))
    assert(Svn::Wc.normal_prop?("name"))
    assert(Svn::Wc.wc_prop?("#{Svn::Core::PROP_WC_PREFIX}name"))
    assert(Svn::Wc.entry_prop?("#{Svn::Core::PROP_ENTRY_PREFIX}name"))
  end

  def test_adm_access
    log = "sample log"
    source = "sample source"
    dir = "dir"
    file = "file"
    prop_name = "name"
    prop_value = "value"
    dir_path = File.join(@wc_path, dir)
    path = File.join(dir_path, file)
    make_context(log) do |ctx|

      ctx.mkdir(dir_path)
      File.open(path, "w") {|f| f.print(source)}
      ctx.add(path)
      rev = ctx.ci(@wc_path).revision

      result = Svn::Wc::AdmAccess.open_anchor(path, true, 5)
      anchor_access, target_access, target = result

      assert_equal(file, target)
      assert_equal(dir_path, anchor_access.path)
      assert_equal(dir_path, target_access.path)

      assert(anchor_access.locked?)
      assert(target_access.locked?)

      assert(!target_access.has_binary_prop?(path))
      assert(!target_access.text_modified?(path))
      assert(!target_access.props_modified?(path))

      File.open(path, "w") {|f| f.print(source * 2)}
      target_access.set_prop(prop_name, prop_value, path)
      assert_equal(prop_value, target_access.prop(prop_name, path))
      assert_equal({prop_name => prop_value},
                   target_access.prop_list(path))
      assert(target_access.text_modified?(path))
      assert(target_access.props_modified?(path))

      target_access.set_prop("name", nil, path)
      assert(!target_access.props_modified?(path))

      target_access.revert(path)
      assert(!target_access.text_modified?(path))
      assert(!target_access.props_modified?(path))

      anchor_access.close
      target_access.close

      access = Svn::Wc::AdmAccess.probe_open(nil, path, false, 5)
      assert(!Svn::Wc.default_ignores({}).empty?)
      assert_equal(Svn::Wc.default_ignores({}), access.ignores({}))
      assert(access.wc_root?(@wc_path))
      access.close

      access = Svn::Wc::AdmAccess.probe_open(nil, @wc_path, false, 5)
      assert_equal(@wc_path, access.path)
      assert_equal(dir_path, access.retrieve(dir_path).path)
      assert_equal(dir_path, access.probe_retrieve(dir_path).path)
      assert_equal(dir_path, access.probe_try(dir_path, false, 5).path)
      access.close

      Svn::Wc::AdmAccess.probe_open(nil, @wc_path, false, 5) do |access|
        assert(!access.locked?)
      end

      Svn::Wc::AdmAccess.probe_open(nil, @wc_path, true, 5) do |access|
        assert(access.locked?)
      end
    end
  end

  def test_traversal_info
    info = Svn::Wc::TraversalInfo.new
    assert_equal([{}, {}], info.edited_externals)
  end

  def assert_externals_description
    dir = "dir"
    url = "http://svn.example.com/trunk"
    description = "#{dir} #{url}"
    items = yield([@wc_path, description])
    assert_equal([[dir, url]],
                 items.collect {|item| [item.target_dir, item.url]})
    assert_kind_of(Svn::Wc::ExternalItem2, items.first)
  end

  def test_externals_description
    assert_externals_description do |args|
      Svn::Wc::ExternalsDescription.parse(*args)
    end
  end

  def test_externals_description_for_backward_compatibility
    assert_externals_description do |args|
      Svn::Wc.parse_externals_description(*args)
    end
  end

  def test_notify
    path = @wc_path
    action = Svn::Wc::NOTIFY_SKIP
    notify = Svn::Wc::Notify.new(path, action)
    assert_equal(path, notify.path)
    assert_equal(action, notify.action)

    notify2 = notify.dup
    assert_equal(path, notify2.path)
    assert_equal(action, notify2.action)
  end

  def test_entry
    log = "sample log"
    source1 = "source"
    source2 = "SOURCE"
    source3 = "SHOYU"
    file = "file"
    path = File.join(@wc_path, file)
    make_context(log) do |ctx|

      File.open(path, "w") {|f| f.print(source1)}
      ctx.add(path)
      rev1 = ctx.ci(@wc_path).revision

      Svn::Wc::AdmAccess.open(nil, @wc_path, false, 5) do |access|
        show = true
        entry = Svn::Wc::Entry.new(path, access, show)
        assert_equal(file, entry.name)
        assert_equal(file, entry.dup.name)

        entries = access.read_entries
        assert_equal(["", file].sort, entries.keys.sort)
        entry = entries[file]
        assert_equal(file, entry.name)

        assert(!entry.conflicted?(@wc_path))
      end

      File.open(path, "w") {|f| f.print(source2)}
      rev2 = ctx.ci(@wc_path).revision

      ctx.up(@wc_path, rev1)
      File.open(path, "w") {|f| f.print(source3)}
      ctx.up(@wc_path, rev2)

      Svn::Wc::AdmAccess.open(nil, @wc_path, true, 5) do |access|
        entry = access.read_entries[file]
        assert(entry.conflicted?(@wc_path))
        assert(entry.text_conflicted?(@wc_path))
        assert(!entry.prop_conflicted?(@wc_path))

        access.resolved_conflict(path)
        assert(!entry.conflicted?(@wc_path))

        access.process_committed(@wc_path, rev2 + 1)
      end
    end
  end

  def test_committed_queue
    log = "sample log"
    source1 = "source"
    source2 = "SOURCE"
    file1 = "file1"
    file2 = "file2"
    path1 = File.join(@wc_path, file1)
    path2 = File.join(@wc_path, file2)
    make_context(log) do |ctx|

      File.open(path1, "w") {|f| f.print(source1)}
      File.open(path2, "w") {|f| f.print(source2)}
      ctx.add(path1)
      ctx.add(path2)
      rev1 = ctx.ci(@wc_path).revision
      next_rev = rev1 + 1

      Svn::Wc::AdmAccess.open(nil, @wc_path, true, 5) do |access|
        queue = Svn::Wc::CommittedQueue.new
        queue.push(access, path1, true, {"my-prop" => "value"})
        queue.process(access, next_rev)
      end

      Svn::Wc::AdmAccess.open(nil, @wc_path, true, 5) do |access|
        queue = Svn::Wc::CommittedQueue.new
        queue.push(access, path1, true, [Svn::Core::Prop.new("my-prop", "value")])
        queue.process(access, next_rev)
      end
    end
  end

  def test_ancestry
    file1 = "file1"
    file2 = "file2"
    path1 = File.join(@wc_path, file1)
    path2 = File.join(@wc_path, file2)
    log = "sample log"
    make_context(log) do |ctx|

      FileUtils.touch(path1)
      ctx.add(path1)
      rev1 = ctx.ci(@wc_path).revision

      ctx.cp(path1, path2)
      rev2 = ctx.ci(@wc_path).revision

      Svn::Wc::AdmAccess.open(nil, @wc_path, false, 5) do |access|
        assert_equal(["#{@repos_uri}/#{file2}", rev2],
                     access.ancestry(path2))
        result = []
        callbacks = Proc.new do |path, entry|
          result << [path, entry]
        end
        def callbacks.found_entry(path, entry)
          call(path, entry)
        end
        access.walk_entries(@wc_path, callbacks)
        assert_equal([@wc_path, path1, path2].sort,
                     result.collect{|path, entry| path}.sort)
        entry = result.assoc(path2)[1]
        assert_equal(file2, entry.name)

        callbacks = Proc.new do |path, entry|
          raise Svn::Error::Cancelled
        end
        def callbacks.found_entry(path, entry)
          call(path, entry)
        end
        assert_raises(Svn::Error::Cancelled) do
          access.walk_entries(@wc_path, callbacks)
        end

        def callbacks.ignored_errors=(value)
          @ignored_errors = value
        end
        def callbacks.handle_error(path, err)
          @ignored_errors << [path, err] if err
        end
        ignored_errors = []
        callbacks.ignored_errors = ignored_errors
        access.walk_entries(@wc_path, callbacks)
        assert_equal([
                      [@wc_path, Svn::Error::Cancelled],
                      [path1, Svn::Error::Cancelled],
                      [path2, Svn::Error::Cancelled],
                     ],
                     ignored_errors.collect {|path, err| [path, err.class]})
      end
    end
  end

  def test_adm_ensure
    adm_dir = Dir.glob(File.join(@wc_path, "{.,_}svn")).first
    assert(File.exists?(adm_dir))
    FileUtils.rm_rf(adm_dir)
    assert(!File.exists?(adm_dir))
    Svn::Wc.ensure_adm(@wc_path, @fs.uuid, @repos_uri, @repos_uri, 0)
    assert(File.exists?(adm_dir))
  end

  def test_merge
    left = <<-EOL
a
b
c
d
e
EOL
    right = <<-EOR
A
b
c
d
E
EOR
    merge_target = <<-EOM
a
b
C
d
e
EOM
    expect = <<-EOE
A
b
C
d
E
EOE

    left_label = "left"
    right_label = "right"
    merge_target_label = "merge_target"

    left_file = "left"
    right_file = "right"
    merge_target_file = "merge_target"
    left_path = File.join(@wc_path, left_file)
    right_path = File.join(@wc_path, right_file)
    merge_target_path = File.join(@wc_path, merge_target_file)

    log = "sample log"
    make_context(log) do |ctx|

      File.open(left_path, "w") {|f| f.print(left)}
      File.open(right_path, "w") {|f| f.print(right)}
      File.open(merge_target_path, "w") {|f| f.print(merge_target)}
      ctx.add(left_path)
      ctx.add(right_path)
      ctx.add(merge_target_path)

      ctx.ci(@wc_path)

      Svn::Wc::AdmAccess.open(nil, @wc_path, true, 5) do |access|
        assert_equal(Svn::Wc::MERGE_MERGED,
                     access.merge(left_path, right_path, merge_target_path,
                                  left_label, right_label, merge_target_label))
        assert_equal(expect, File.read(merge_target_path))
      end
    end
  end

  def test_status
    source = "source"
    file1 = "file1"
    file2 = "file2"
    file3 = "file3"
    file4 = "file4"
    file5 = "file5"
    path1 = File.join(@wc_path, file1)
    path2 = File.join(@wc_path, file2)
    path3 = File.join(@wc_path, file3)
    path4 = File.join(@wc_path, file4)
    path5 = File.join(@wc_path, file5)
    log = "sample log"
    make_context(log) do |ctx|

      File.open(path1, "w") {|f| f.print(source)}
      ctx.add(path1)
      rev1 = ctx.ci(@wc_path).revision

      ctx.cp(path1, path2)
      rev2 = ctx.ci(@wc_path).revision

      FileUtils.rm(path1)

      Svn::Wc::AdmAccess.open(nil, @wc_path, false, 5) do |access|
        status = access.status(path1)
        assert_equal(Svn::Wc::STATUS_MISSING, status.text_status)
        assert_equal(Svn::Wc::STATUS_MISSING, status.dup.text_status)
      end

      ctx.revert(path1)

      Svn::Wc::AdmAccess.open(nil, @wc_path, true, 5) do |access|
        access.copy(path1, file3)
        assert(File.exist?(path3))
        access.delete(path1)
        assert(!File.exist?(path1))
        FileUtils.touch(path4)
        access.add(path4)
        assert(File.exist?(path4))
        access.add_repos_file2(path5, path2, {})
        assert_equal(source, File.open(path5) {|f| f.read})

        status = access.status(path2)
        assert_equal(Svn::Wc::STATUS_MISSING, status.text_status)
        access.remove_from_revision_control(file2)
        status = access.status(path2)
        assert_equal(Svn::Wc::STATUS_NONE, status.text_status)
      end
    end
  end

  def test_delete
    source = "source"
    file = "file"
    path = File.join(@wc_path, file)
    log = "sample log"
    make_context(log) do |ctx|

      File.open(path, "w") {|f| f.print(source)}
      ctx.add(path)
      ctx.ci(@wc_path).revision

      assert(File.exists?(path))
      Svn::Wc::AdmAccess.open(nil, @wc_path, true, 5) do |access|
        access.delete(path)
      end
      assert(!File.exists?(path))

      ctx.revert(path)

      assert(File.exists?(path))
      Svn::Wc::AdmAccess.open(nil, @wc_path, true, 5) do |access|
        access.delete(path, nil, nil, true)
      end
      assert(File.exists?(path))
    end
  end

  def test_locked
    assert(!Svn::Wc.locked?(@wc_path))
    Svn::Wc::AdmAccess.open(nil, @wc_path, true, -1) do |access|
      assert(Svn::Wc.locked?(@wc_path))
    end
    assert(!Svn::Wc.locked?(@wc_path))
    Svn::Wc::AdmAccess.open(nil, @wc_path, false, -1) do |access|
      assert(!Svn::Wc.locked?(@wc_path))
    end
    assert(!Svn::Wc.locked?(@wc_path))
  end

  def assert_translated_eol(method_name)
    src_file = "src"
    crlf_file = "crlf"
    cr_file = "cr"
    lf_file = "lf"
    src_path = File.join(@wc_path, src_file)
    crlf_path = File.join(@wc_path, crlf_file)
    cr_path = File.join(@wc_path, cr_file)
    lf_path = File.join(@wc_path, lf_file)

    source = "a\n"
    crlf_source = source.gsub(/\n/, "\r\n")
    cr_source = source.gsub(/\n/, "\r")
    lf_source = source.gsub(/\n/, "\n")

    File.open(crlf_path, "w") {}
    File.open(cr_path, "w") {}
    File.open(lf_path, "w") {}

    log = "log"
    make_context(log) do |ctx|
      ctx.add(crlf_path)
      ctx.add(cr_path)
      ctx.add(lf_path)
      ctx.prop_set("svn:eol-style", "CRLF", crlf_path)
      ctx.prop_set("svn:eol-style", "CR", cr_path)
      ctx.prop_set("svn:eol-style", "LF", lf_path)
      ctx.ci(crlf_path)
      ctx.ci(cr_path)
      ctx.ci(lf_path)

      Svn::Wc::AdmAccess.open(nil, @wc_path, true, 5) do |access|
        _wrap_assertion do
          File.open(src_path, "wb") {|f| f.print(source)}
          args = [method_name, src_path, crlf_path, Svn::Wc::TRANSLATE_FROM_NF]
          result = yield(access.send(*args), source)
          result ||= File.open(src_path, "rb") {|f| f.read}
          assert_equal(crlf_source, result)

          File.open(src_path, "wb") {|f| f.print(source)}
          args = [method_name, src_path, cr_path, Svn::Wc::TRANSLATE_FROM_NF]
          result = yield(access.send(*args), source)
          result ||= File.open(src_path, "rb") {|f| f.read}
          assert_equal(cr_source, result)

          File.open(src_path, "wb") {|f| f.print(source)}
          args = [method_name, src_path, lf_path, Svn::Wc::TRANSLATE_FROM_NF]
          result = yield(access.send(*args), source)
          result ||= File.open(src_path, "rb") {|f| f.read}
          assert_equal(lf_source, result)
        end
      end
    end
  end

  def test_translated_file_eol
    assert_translated_eol(:translated_file) do |name, source|
      File.open(name, "rb") {|f| f.read}
    end
  end

  def test_translated_file2_eol
    assert_translated_eol(:translated_file2) do |file, source|
      result = file.read
      file.close
      result
    end
  end

  def test_translated_stream_eol
    assert_translated_eol(:translated_stream) do |stream, source|
      if source
        stream.write(source)
        stream.close
        nil
      else
        stream.read
      end
    end
  end

  def assert_translated_keyword(method_name)
    src_file = "$Revision$"
    revision_file = "revision_file"
    src_path = File.join(@wc_path, src_file)
    revision_path = File.join(@wc_path, revision_file)

    source = "$Revision$\n"
    revision_source = source.gsub(/\$Revision\$/, "$Revision: 1 $")

    File.open(revision_path, "w") {}

    log = "log"
    make_context(log) do |ctx|
      ctx.add(revision_path)
      ctx.prop_set("svn:keywords", "Revision", revision_path)
      ctx.ci(revision_path)

      Svn::Wc::AdmAccess.open(nil, @wc_path, true, 5) do |access|
        File.open(src_path, "wb") {|f| f.print(source)}
        args = [method_name, src_path, revision_path, Svn::Wc::TRANSLATE_FROM_NF]
        result = yield(access.send(*args), source)
        result ||= File.open(src_path, "rb") {|f| f.read}
        assert_equal(revision_source, result)

        File.open(src_path, "wb") {|f| f.print(source)}
        args = [method_name, src_path, revision_path, Svn::Wc::TRANSLATE_TO_NF]
        assert_equal(source, yield(access.send(*args)))
      end
    end
  end

  def test_translated_file_keyword
    assert_translated_keyword(:translated_file) do |name, source|
      File.open(name, "rb") {|f| f.read}
    end
  end

  def test_translated_file2_keyword
    assert_translated_keyword(:translated_file2) do |file, source|
      file.read
    end
  end

  def test_translated_stream_keyword
    assert_translated_keyword(:translated_stream) do |stream, source|
      if source
        stream.write(source)
        stream.close
        nil
      else
        result = stream.read
        stream.close
        result
      end
    end
  end

  def test_revision_status
    log = "log"
    file = "file"
    path = File.join(@wc_path, file)

    File.open(path, "w") {|f| f.print("a")}
    make_context(log) do |ctx|
      ctx.add(path)
      ctx.ci(path)

      File.open(path, "w") {|f| f.print("b")}
      ctx.ci(path)

      File.open(path, "w") {|f| f.print("c")}
      rev = ctx.ci(path).revision

      status = Svn::Wc::RevisionStatus.new(path, nil, true)
      assert_equal(rev, status.min_rev)
      assert_equal(rev, status.max_rev)
    end
  end

  def test_external_item_new
    assert_raises(NoMethodError) do
      Svn::Wc::ExternalItem.new
    end

    item = Svn::Wc::ExternalItem2.new
    assert_kind_of(Svn::Wc::ExternalItem2, item)
  end

  def test_external_item_dup
    item = Svn::Wc::ExternalItem2.new
    assert_kind_of(Svn::Wc::ExternalItem2, item)

    item.target_dir = "xxx"
    duped_item = item.dup
    assert_equal(item.target_dir, duped_item.target_dir)
  end

  def test_committed_queue_new
    queue = Svn::Wc::CommittedQueue.new
    assert_kind_of(Svn::Wc::CommittedQueue, queue)
  end

  def assert_diff_callbacks(method_name)
    log = "sample log"
    file1 = "hello.txt"
    file2 = "hello2.txt"
    src = "Hello"
    dir = "dir"
    dir_path = File.join(@wc_path, dir)
    path1 = File.join(dir_path, file1)
    path2 = File.join(dir_path, file2)
    prop_name = "my-prop"
    prop_value = "value"

    make_context(log) do |ctx|
      config = {}
      callbacks = Svn::Ra::Callbacks.new(ctx.auth_baton)
      Svn::Ra::Session.open(@repos_uri, config, callbacks) do |session|

        FileUtils.mkdir(dir_path)
        File.open(path1, "w") {|f| f.print(src)}
        ctx.add(dir_path)
        rev1 = ctx.commit(@wc_path).revision

        File.open(path1, "w") {|f| f.print(src * 2)}
        File.open(path2, "w") {|f| f.print(src)}
        ctx.add(path2)
        ctx.prop_set(prop_name, prop_value, path1)

        Svn::Wc::AdmAccess.open(nil, @wc_path, true, 5) do |adm|
          callbacks = Object.new
          def callbacks.result
            @result
          end
          def callbacks.result=(result)
            @result = result
          end
          def callbacks.file_added(access, path, tmpfile1, tmpfile2, rev1,
                                   rev2, mimetype1, mimetype2,
                                   prop_changes, original_props)
            @result << [:file_added, path, prop_changes]
            [Svn::Wc::NOTIFY_STATE_UNCHANGED, Svn::Wc::NOTIFY_STATE_UNCHANGED]
          end
          def callbacks.file_changed(access, path, tmpfile1, tmpfile2, rev1,
                                     rev2, mimetype1, mimetype2,
                                     prop_changes, original_props)
            @result << [:file_changed, path, prop_changes]
            [Svn::Wc::NOTIFY_STATE_UNCHANGED, Svn::Wc::NOTIFY_STATE_UNCHANGED]
          end
          def callbacks.dir_props_changed(access, path, prop_changes, original_props)
            @result << [:dir_props_changed, path, prop_changes]
            Svn::Wc::NOTIFY_STATE_UNCHANGED
          end
          callbacks.result = []
          editor = adm.__send__(method_name, dir, callbacks)
          reporter = session.diff(rev1, "", @repos_uri, editor)
          adm.crawl_revisions(dir_path, reporter)

          property_info = {
            :dir_changed_prop_names => [
                                        "svn:entry:committed-date",
                                        "svn:entry:uuid",
                                        "svn:entry:last-author",
                                        "svn:entry:committed-rev"
                                       ],
            :file_changed_prop_name => prop_name,
            :file_changed_prop_value => prop_value,
          }
          sorted_result = callbacks.result.sort_by {|r| r.first.to_s}
          expected_props, actual_result = yield(property_info, sorted_result)
          dir_changed_props, file_changed_props, empty_changed_props = expected_props
          assert_equal([
                        [:dir_props_changed, @wc_path, dir_changed_props],
                        [:file_added, path2, empty_changed_props],
                        [:file_changed, path1, file_changed_props],
                       ],
                       sorted_result)
        end
      end
    end
  end

  def test_diff_callbacks_for_backward_compatibility
    assert_diff_callbacks(:diff_editor) do |property_info, result|
      dir_changed_prop_names = property_info[:dir_changed_prop_names]
      dir_changed_props = dir_changed_prop_names.sort.collect do |name|
        Svn::Core::Prop.new(name, nil)
      end
      prop_name = property_info[:file_changed_prop_name]
      prop_value = property_info[:file_changed_prop_value]
      file_changed_props = [Svn::Core::Prop.new(prop_name, prop_value)]
      empty_changed_props = []

      sorted_result = result.dup
      dir_prop_changed = sorted_result.assoc(:dir_props_changed)
      dir_prop_changed[2] = dir_prop_changed[2].sort_by {|prop| prop.name}

      [[dir_changed_props, file_changed_props, empty_changed_props],
       sorted_result]
    end
  end

  def test_diff_callbacks
    assert_diff_callbacks(:diff_editor2) do |property_info, result|
      dir_changed_props = {}
      property_info[:dir_changed_prop_names].each do |name|
        dir_changed_props[name] = nil
      end
      prop_name = property_info[:file_changed_prop_name]
      prop_value = property_info[:file_changed_prop_value]
      file_changed_props = {prop_name => prop_value}
      empty_changed_props = {}
      [[dir_changed_props, file_changed_props, empty_changed_props],
       result]
    end
  end

  def test_update_editor
    log = "sample log"
    file1 = "hello.txt"
    file2 = "hello2.txt"
    src = "Hello"
    dir = "dir"
    dir_path = File.join(@wc_path, dir)
    path1 = File.join(dir_path, file1)
    path2 = File.join(dir_path, file2)

    make_context(log) do |ctx|
      config = {}
      callbacks = Svn::Ra::Callbacks.new(ctx.auth_baton)
      Svn::Ra::Session.open(@repos_uri, config, callbacks) do |session|

        FileUtils.mkdir(dir_path)
        File.open(path1, "w") {|f| f.print(src)}
        ctx.add(dir_path)
        rev1 = ctx.commit(@wc_path).revision

        File.open(path1, "w") {|f| f.print(src * 2)}
        File.open(path2, "w") {|f| f.print(src)}
        ctx.add(path2)
        rev2 = ctx.commit(@wc_path).revision

        assert(File.exists?(path2))
        assert_equal(0, ctx.up(@wc_path, 0))
        assert(!File.exists?(path2))
        Svn::Wc::AdmAccess.open(nil, @wc_path) do |access|
          editor = access.update_editor('', 0)
          assert_equal(0, editor.target_revision)

          reporter = session.update2(rev2, "", editor)
          access.crawl_revisions(@wc_path, reporter)
          assert_equal(rev2, editor.target_revision)
        end
      end
    end
  end

  def test_update_editor_options
    log = "sample log"
    file1 = "hello.txt"
    file2 = "hello2.txt"
    src = "Hello"
    dir = "dir"
    dir_path = File.join(@wc_path, dir)
    path1 = File.join(dir_path, file1)
    path2 = File.join(dir_path, file2)

    make_context(log) do |ctx|
      config = {}
      callbacks = Svn::Ra::Callbacks.new(ctx.auth_baton)
      Svn::Ra::Session.open(@repos_uri) do |session|

        FileUtils.mkdir(dir_path)
        File.open(path1, "w") {|f| f.print(src)}
        ctx.add(dir_path)
        rev1 = ctx.commit(@wc_path).revision

        File.open(path1, "w") {|f| f.print(src * 2)}
        File.open(path2, "w") {|f| f.print(src)}
        ctx.add(path2)
        rev2 = ctx.commit(@wc_path).revision

        assert(File.exists?(path2))
        assert_equal(0, ctx.up(@wc_path, 0))
        assert(!File.exists?(path2))
        notification_count = 0
        Svn::Wc::AdmAccess.open(nil, @wc_path) do |access|
          notify_func = Proc.new {|n| notification_count += 1}
          assert_raises(ArgumentError) do
            access.update_editor2(:target_revision => 0,
                                  :target => '',
                                  :notify_fun => notify_func)
          end
          editor = access.update_editor('', 0, true, nil, false, nil,
                                        notify_func)
          assert_equal(0, editor.target_revision)

          reporter = session.update2(rev2, "", editor)
          access.crawl_revisions(@wc_path, reporter)
          assert_equal(rev2, editor.target_revision)
        end
        assert_equal(4, notification_count, "wrong number of notifications")
      end
    end
  end

  def test_update_editor2_conflict_func
    log = "sample log"
    source1 = "source"
    source2 = "SOURCE"
    source3 = "SHOYU"
    file = "file"
    path = File.join(@wc_path, file)
    make_context(log) do |ctx|

      File.open(path, "w") {|f| f.print(source1)}
      ctx.add(path)
      rev1 = ctx.ci(@wc_path).revision

      File.open(path, "w") {|f| f.print(source2)}
      rev2 = ctx.ci(@wc_path).revision

      ctx.up(@wc_path, rev1)
      File.open(path, "w") {|f| f.print(source3)}

      Svn::Ra::Session.open(@repos_uri) do |session|

        conflicted_paths = {}

        Svn::Wc::AdmAccess.open(nil, @wc_path) do |access|
          editor = access.update_editor2(
              :target => '',
              :conflict_func => lambda{|n|
                conflicted_paths[n.path]=true
                Svn::Wc::CONFLICT_CHOOSE_MERGED
              },
              :target_revision => 0
          )

          assert_equal(0, editor.target_revision)

          reporter = session.update2(rev2, "", editor)
          access.crawl_revisions(@wc_path, reporter)
          assert_equal(rev2, editor.target_revision)
        end

        assert_equal([File.expand_path(path)], conflicted_paths.keys);
      end
    end
  end

  def test_switch_editor
    log = "sample log"
    file1 = "hello.txt"
    file2 = "hello2.txt"
    src = "Hello"
    dir1 = "dir1"
    dir2 = "dir2"
    dir1_path = File.join(@wc_path, dir1)
    dir2_path = File.join(@wc_path, dir2)
    dir1_uri = "#{@repos_uri}/#{dir1}"
    dir2_uri = "#{@repos_uri}/#{dir2}"
    path1 = File.join(dir1_path, file1)
    path2 = File.join(dir2_path, file2)

    make_context(log) do |ctx|
      config = {}
      callbacks = Svn::Ra::Callbacks.new(ctx.auth_baton)
      Svn::Ra::Session.open(@repos_uri, config, callbacks) do |session|

        FileUtils.mkdir(dir1_path)
        File.open(path1, "w") {|f| f.print(src)}
        ctx.add(dir1_path)
        rev1 = ctx.commit(@wc_path).revision

        FileUtils.mkdir(dir2_path)
        File.open(path2, "w") {|f| f.print(src)}
        ctx.add(dir2_path)
        rev2 = ctx.commit(@wc_path).revision

        assert(File.exists?(path1))
        assert_equal(rev2, ctx.switch(@wc_path, dir2_uri))
        assert(File.exists?(File.join(@wc_path, file2)))
        Svn::Wc::AdmAccess.open_anchor(@wc_path) do |access, dir_access, target|
          editor = dir_access.switch_editor('', dir1_uri, rev2)
          assert_equal(rev2, editor.target_revision)

          reporter = session.switch2(rev1, dir1, dir1_uri, editor)
          dir_access.crawl_revisions(@wc_path, reporter)
          assert_equal(rev1, editor.target_revision)
        end
      end
    end
  end

  def test_relocate
    log = "sample log"
    file1 = "hello.txt"
    file2 = "hello2.txt"
    src = "Hello"
    dir1 = "dir1"
    dir2 = "dir2"
    dir1_path = File.join(@wc_path, dir1)
    dir2_path = File.join(@wc_path, dir2)
    dir1_uri = "#{@repos_uri}/#{dir1}"
    dir2_uri = "#{@repos_uri}/#{dir2}"
    path1 = File.join(dir1_path, file1)
    path2 = File.join(dir2_path, file2)

    make_context(log) do |ctx|
      config = {}
      callbacks = Svn::Ra::Callbacks.new(ctx.auth_baton)
      Svn::Ra::Session.open(@repos_uri, config, callbacks) do |session|

        FileUtils.mkdir(dir1_path)
        File.open(path1, "w") {|f| f.print(src)}
        ctx.add(dir1_path)
        rev1 = ctx.commit(@wc_path).revision

        FileUtils.mkdir(dir2_path)
        File.open(path2, "w") {|f| f.print(src)}
        ctx.add(dir2_path)
        rev2 = ctx.commit(@wc_path).revision

        Svn::Wc::AdmAccess.probe_open(nil, @wc_path) do |access|
          assert(@repos_uri, access.entry(@wc_path).url)
          values = []
          access.relocate(@wc_path, @repos_uri, dir2_uri) do |uuid, url, root_url|
            values << [uuid, url, root_url]
          end
          assert(!values.empty?)
          assert(dir2_uri, access.entry(@wc_path).url)
        end
      end
    end
  end

  def test_ignore?
    assert(!Svn::Wc.ignore?("xxx.c", []))
    assert(Svn::Wc.ignore?("xxx.c", ["*.c"]))
    assert(!Svn::Wc.ignore?("xxx.c", ["XXX.c"]))
    assert(Svn::Wc.ignore?("xxx.c", ["xxx.c"]))
    assert(!Svn::Wc.ignore?("xxx.c", ["*.H", "*.C"]))
    assert(Svn::Wc.ignore?("xxx.C", ["*.H", "*.C"]))
  end

  def test_changelist
    log = "sample log"
    file = "hello.txt"
    src = "Hello"
    path = File.join(@wc_path, file)

    make_context(log) do |ctx|
      File.open(path, "w") {|f| f.print(src)}
      ctx.add(path)
      rev1 = ctx.commit(@wc_path).revision
    end

    Svn::Wc::AdmAccess.open(nil, @wc_path) do |access|
      assert_nil(access.entry(@wc_path).changelist)
    end

    notifies = []
    notify_collector = Proc.new {|notify| notifies << notify }
    Svn::Wc::AdmAccess.open(nil, @wc_path) do |access|
      access.set_changelist(path, "123", nil, notify_collector)
    end
    assert_equal([[File.expand_path(path), nil]],
                 notifies.collect {|notify| [notify.path, notify.err]})

    notifies = []
    Svn::Wc::AdmAccess.open(nil, @wc_path) do |access|
      access.set_changelist(path, "456", nil, notify_collector)
    end
    assert_equal([[File.expand_path(path), Svn::Wc::NOTIFY_CHANGELIST_CLEAR],
                  [File.expand_path(path), Svn::Wc::NOTIFY_CHANGELIST_SET]],
                 notifies.collect {|notify| [notify.path, notify.action]})

    notifies = []

    Svn::Wc::AdmAccess.open(nil, @wc_path) do |access|
      access.set_changelist(@wc_path, "789", nil, notify_collector)
    end
  end


  def test_context_new_default_config
    assert_not_nil context = Svn::Wc::Context.new
  ensure
    context.destroy
  end

  def test_context_new_specified_config
    config_file = File.join(@config_path, Svn::Core::CONFIG_CATEGORY_CONFIG)
    config = Svn::Core::Config.read(config_file)
    assert_not_nil context = Svn::Wc::Context.new(:config=>config)
  ensure
    context.destroy
  end

  def test_context_create
    assert_nothing_raised do
      result = Svn::Wc::Context.create do |context|
        assert_not_nil context
        assert_kind_of Svn::Wc::Context, context
      end
      assert_nil result;
    end
  end

end
