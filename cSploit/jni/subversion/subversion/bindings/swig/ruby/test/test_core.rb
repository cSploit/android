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

require "stringio"

require "svn/core"
require "svn/repos"

class SvnCoreTest < Test::Unit::TestCase
  include SvnTestUtil

  def setup
    setup_default_variables
    @config_file = File.join(@config_path, Svn::Core::CONFIG_CATEGORY_CONFIG)
    @servers_file = File.join(@config_path, Svn::Core::CONFIG_CATEGORY_SERVERS)
    setup_repository(@repos_path)
    setup_config
    setup_tmp
  end

  def teardown
    teardown_repository(@repos_path)
    teardown_config
    teardown_tmp
  end

  def test_binary_mime_type?
    assert(Svn::Core.binary_mime_type?("image/png"))
    assert(!Svn::Core.binary_mime_type?("text/plain"))
  end

  def test_time
    now = Time.now.gmtime
    str = now.strftime("%Y-%m-%dT%H:%M:%S.") + "#{now.usec}Z"

    assert_equal(now, Time.from_svn_format(str))

    apr_time = now.to_i * 1000000 + now.usec
    assert_equal(apr_time, now.to_apr_time)
  end

  def test_not_new_auth_provider_object
    assert_raise(NoMethodError) do
      Svn::Core::AuthProviderObject.new
    end
  end

  def test_version_to_x
    major = 1
    minor = 2
    patch = 3
    tag = "-dev"
    ver = Svn::Core::Version.new(major, minor, patch, tag)

    assert_equal("#{major}.#{minor}.#{patch}#{tag}", ver.to_s)
    assert_equal([major, minor, patch, tag], ver.to_a)
  end

  def test_version_valid?
    assert_true(Svn::Core::Version.new(1, 2, 3, "-devel").valid?)
    assert_true(Svn::Core::Version.new(nil, nil, nil, "").valid?)
    assert_true(Svn::Core::Version.new.valid?)
  end

  def test_version_equal
    major = 1
    minor = 2
    patch = 3
    tag = ""
    ver1 = Svn::Core::Version.new(major, minor, patch, tag)
    ver2 = Svn::Core::Version.new(major, minor, patch, tag)
    ver3 = Svn::Core::Version.new
    assert_equal(ver1, ver2)
    assert_not_equal(ver1, ver3)
  end

  def test_version_compatible?
    major = 1
    minor = 2
    patch = 3

    my_tag = "-devel"
    lib_tag = "-devel"
    ver1 = Svn::Core::Version.new(major, minor, patch, my_tag)
    ver2 = Svn::Core::Version.new(major, minor, patch, lib_tag)
    ver3 = Svn::Core::Version.new(major, minor, patch, lib_tag + "x")
    assert_true(ver1.compatible?(ver2))
    assert_false(ver1.compatible?(ver3))

    my_tag = "-devel"
    lib_tag = ""
    ver1 = Svn::Core::Version.new(major, minor, patch, my_tag)
    ver2 = Svn::Core::Version.new(major, minor, patch, lib_tag)
    ver3 = Svn::Core::Version.new(major, minor, patch - 1, lib_tag)
    assert_false(ver1.compatible?(ver2))
    assert_true(ver1.compatible?(ver3))

    tag = ""
    ver1 = Svn::Core::Version.new(major, minor, patch, tag)
    ver2 = Svn::Core::Version.new(major, minor, patch, tag)
    ver3 = Svn::Core::Version.new(major, minor, patch - 1, tag)
    ver4 = Svn::Core::Version.new(major, minor + 1, patch, tag)
    ver5 = Svn::Core::Version.new(major, minor - 1, patch, tag)
    assert_true(ver1.compatible?(ver2))
    assert_true(ver1.compatible?(ver3))
    assert_true(ver1.compatible?(ver4))
    assert_false(ver1.compatible?(ver5))
  end

  def test_version
    vers = [Svn::Core::VER_MAJOR, Svn::Core::VER_MINOR, Svn::Core::VER_PATCH]
    ver_num = vers.collect {|ver| ver.to_s}.join(".")
    assert_equal(ver_num, Svn::Core::VER_NUM)
    assert_equal("#{ver_num}#{Svn::Core::VER_NUMTAG}", Svn::Core::VER_NUMBER)
    assert_equal("#{Svn::Core::VER_NUMBER}#{Svn::Core::VER_TAG}", Svn::Core::VERSION)
  end

  def test_auth_parameter
    key = "key"
    value = "value"
    auth = Svn::Core::AuthBaton.new
    assert_nil(auth[key])
    auth[key] = value
    assert_equal(value, auth[key])

    assert_raise(TypeError) do
      auth[key] = 1
    end
  end

  def test_pool_GC
    gc_disable do
      made_number_of_pool = 100
      pools = []

      gc
      before_number_of_pools = Svn::Core::Pool.number_of_pools
      made_number_of_pool.times do
        pools << used_pool
      end
      gc
      current_number_of_pools = Svn::Core::Pool.number_of_pools
      created_number_of_pools = current_number_of_pools - before_number_of_pools
      assert_operator(made_number_of_pool, :<=, current_number_of_pools)

      gc
      pools.clear
      before_number_of_pools = Svn::Core::Pool.number_of_pools
      gc
      current_number_of_pools = Svn::Core::Pool.number_of_pools
      recycled_number_of_pools =
        before_number_of_pools - current_number_of_pools
      assert_operator(made_number_of_pool * 0.8, :<=, recycled_number_of_pools)
    end
  end

  def test_config
    assert_equal([
                   Svn::Core::CONFIG_CATEGORY_CONFIG,
                   Svn::Core::CONFIG_CATEGORY_SERVERS,
                 ].sort,
                 Svn::Core::Config.config(@config_path).keys.sort)

    config = Svn::Core::Config.read(@config_file)
    section = Svn::Core::CONFIG_SECTION_HELPERS
    option = Svn::Core::CONFIG_OPTION_DIFF_CMD
    value = "diff"

    assert_nil(config.get(section, option))
    config.set(section, option, value)
    assert_equal(value, config.get(section, option))
  end

  def test_config_bool
    config = Svn::Core::Config.read(@config_file)
    section = Svn::Core::CONFIG_SECTION_MISCELLANY
    option = Svn::Core::CONFIG_OPTION_ENABLE_AUTO_PROPS

    assert(config.get_bool(section, option, true))
    config.set_bool(section, option, false)
    assert(!config.get_bool(section, option, true))
  end

  def test_config_each
    config = Svn::Core::Config.read(@config_file)
    section = Svn::Core::CONFIG_SECTION_HELPERS
    options = {
      Svn::Core::CONFIG_OPTION_DIFF_CMD => "diff",
      Svn::Core::CONFIG_OPTION_DIFF3_CMD => "diff3",
    }

    infos = {}
    config.each_option(section) do |name, value|
      infos[name] = value
      true
    end
    assert_equal({}, infos)

    section_names = []
    config.each_section do |name|
      section_names << name
      true
    end
    assert_equal([], section_names)

    options.each do |option, value|
      config.set(section, option, value)
    end

    config.each_option(section) do |name, value|
      infos[name] = value
      true
    end
    assert_equal(options, infos)

    config.each_section do |name|
      section_names << name
      true
    end
    assert_equal([section], section_names)

    infos = options.collect {|key, value| [section, key, value]}
    config_infos = []
    config.each do |section, name, value|
      config_infos << [section, name, value]
    end
    assert_equal(infos.sort, config_infos.sort)
    assert_equal(infos.sort, config.collect {|args| args}.sort)
  end

  def test_config_find_group
    config = Svn::Core::Config.read(@config_file)
    section = Svn::Core::CONFIG_SECTION_HELPERS
    option = Svn::Core::CONFIG_OPTION_DIFF_CMD
    value = "diff"

    assert_nil(config.find_group(value, section))
    config.set(section, option, value)
    assert_equal(option, config.find_group(value, section))
  end

  def test_config_get_server_setting
    group = "group1"
    host_prop_name = "http-proxy-host"
    host_prop_value = "*.example.com"
    default_host_value = "example.net"
    port_prop_name = "http-proxy-port"
    port_prop_value = 8080
    default_port_value = 1818

    File.open(@servers_file, "w") do |f|
      f.puts("[#{group}]")
    end

    config = Svn::Core::Config.read(@servers_file)
    assert_equal(default_host_value,
                 config.get_server_setting(group,
                                           host_prop_name,
                                           default_host_value))
    assert_equal(default_port_value,
                 config.get_server_setting_int(group,
                                               port_prop_name,
                                               default_port_value))

    File.open(@servers_file, "w") do |f|
      f.puts("[#{group}]")
      f.puts("#{host_prop_name} = #{host_prop_value}")
      f.puts("#{port_prop_name} = #{port_prop_value}")
    end

    config = Svn::Core::Config.read(@servers_file)
    assert_equal(host_prop_value,
                 config.get_server_setting(group,
                                           host_prop_name,
                                           default_host_value))
    assert_equal(port_prop_value,
                 config.get_server_setting_int(group,
                                               port_prop_name,
                                               default_port_value))
  end

  def test_config_auth_data
    cred_kind = Svn::Core::AUTH_CRED_SIMPLE
    realm_string = "sample"
    assert_nil(Svn::Core::Config.read_auth_data(cred_kind,
                                                realm_string,
                                                @config_path))
    Svn::Core::Config.write_auth_data({},
                                      cred_kind,
                                      realm_string,
                                      @config_path)
    assert_equal({Svn::Core::CONFIG_REALMSTRING_KEY => realm_string},
                 Svn::Core::Config.read_auth_data(cred_kind,
                                                  realm_string,
                                                  @config_path))
  end

  def test_config_to_s
    config = Svn::Core::Config.read(@config_file)
    section = Svn::Core::CONFIG_SECTION_HELPERS
    options = {
      Svn::Core::CONFIG_OPTION_DIFF_CMD => "diff",
      Svn::Core::CONFIG_OPTION_DIFF3_CMD => "diff3",
    }

    options.each do |option, value|
      config[section, option] = value
    end

    temp_config = Tempfile.new("svn-test-config")
    temp_config.open
    temp_config.print(config.to_s)
    temp_config.close

    parsed_config = Svn::Core::Config.read(temp_config.path)
    assert_equal({section => options}, parsed_config.to_hash)
  end

  def test_config_to_hash
    config = Svn::Core::Config.read(@config_file)
    section = Svn::Core::CONFIG_SECTION_HELPERS
    options = {
      Svn::Core::CONFIG_OPTION_DIFF_CMD => "diff",
      Svn::Core::CONFIG_OPTION_DIFF3_CMD => "diff3",
    }

    assert_equal({}, config.to_hash)

    options.each do |option, value|
      config[section, option] = value
    end

    assert_equal({section => options}, config.to_hash)
  end

  def test_diff_version
    assert_equal(Svn::Core.subr_version, Svn::Core::Diff.version)
  end

  def test_diff_unified
    original = Tempfile.new("original")
    modified = Tempfile.new("modified")
    original_src = <<-EOS
  a
b
  c
EOS
    modified_src = <<-EOS
a

 c
EOS
    original_header = "(orig)"
    modified_header = "(mod)"

    original.open
    original.print(original_src)
    original.close
    modified.open
    modified.print(modified_src)
    modified.close

    expected = <<-EOD
--- #{original_header}
+++ #{modified_header}
@@ -1,3 +1,3 @@
-  a
-b
-  c
+a
+
+ c
EOD
    diff = Svn::Core::Diff.file_diff(original.path, modified.path)
    assert_equal(normalize_line_break(expected),
                 diff.unified(original_header, modified_header))

    options = Svn::Core::DiffFileOptions.parse("--ignore-space-change")
    expected = <<-EOD
--- #{original_header}
+++ #{modified_header}
@@ -1,3 +1,3 @@
-  a
-b
+a
+
   c
EOD
    diff = Svn::Core::Diff.file_diff(original.path, modified.path, options)
    assert_equal(normalize_line_break(expected),
                 diff.unified(original_header, modified_header))

    options = Svn::Core::DiffFileOptions.parse("--ignore-all-space")
    expected = <<-EOD
--- #{original_header}
+++ #{modified_header}
@@ -1,3 +1,3 @@
   a
-b
+
   c
EOD
    diff = Svn::Core::Diff.file_diff(original.path, modified.path, options)
    assert_equal(normalize_line_break(expected),
                 diff.unified(original_header, modified_header))
  end

  def test_diff_merge
    original = Tempfile.new("original")
    modified = Tempfile.new("modified")
    latest = Tempfile.new("latest")
    original_src = <<-EOS
a
 b
c
d
e
EOS
    modified_src = <<-EOS
a
 b

d
e
EOS
    latest_src = <<-EOS

  b
c
d
 e
EOS

    original.open
    original.print(original_src)
    original.close
    modified.open
    modified.print(modified_src)
    modified.close
    latest.open
    latest.print(latest_src)
    latest.close

    expected = <<-EOD

  b

d
 e
EOD
    diff = Svn::Core::Diff.file_diff3(original.path,
                                      modified.path,
                                      latest.path)
    assert_equal(normalize_line_break(expected), diff.merge)

    options = Svn::Core::DiffFileOptions.parse("--ignore-space-change")
    expected = <<-EOD

 b

d
 e
EOD
    diff = Svn::Core::Diff.file_diff3(original.path,
                                      modified.path,
                                      latest.path,
                                      options)
    assert_equal(normalize_line_break(expected), diff.merge)

    options = Svn::Core::DiffFileOptions.parse("--ignore-all-space")
    expected = <<-EOD

 b

d
e
EOD
    diff = Svn::Core::Diff.file_diff3(original.path,
                                      modified.path,
                                      latest.path,
                                      options)
    assert_equal(normalize_line_break(expected), diff.merge)
  end

  def test_diff_file_options
    args = ["--ignore-all-space"]
    options = Svn::Core::DiffFileOptions.parse(*args)
    assert_equal(Svn::Core::DIFF_FILE_IGNORE_SPACE_ALL,
                 options.ignore_space)
    assert_false(options.ignore_eol_style)

    args = ["--ignore-space-change"]
    options = Svn::Core::DiffFileOptions.parse(*args)
    assert_equal(Svn::Core::DIFF_FILE_IGNORE_SPACE_CHANGE,
                 options.ignore_space)
    assert_false(options.ignore_eol_style)

    args = ["--ignore-space-change", "--ignore-eol-style"]
    options = Svn::Core::DiffFileOptions.parse(*args)
    assert_equal(Svn::Core::DIFF_FILE_IGNORE_SPACE_CHANGE,
                 options.ignore_space)
    assert_true(options.ignore_eol_style)

    options = Svn::Core::DiffFileOptions.parse(args)
    assert_equal(Svn::Core::DIFF_FILE_IGNORE_SPACE_CHANGE,
                 options.ignore_space)
    assert_true(options.ignore_eol_style)
  end

  def test_create_commit_info
    info = Svn::Core::CommitInfo.new
    now = Time.now.gmtime
    date_str = now.strftime("%Y-%m-%dT%H:%M:%S")
    date_str << ".#{now.usec}Z"
    info.date = date_str
    assert_equal(now, info.date)
  end

  def test_svn_prop
    assert(Svn::Core::Property.svn_prop?("svn:mime-type"))
    assert(!Svn::Core::Property.svn_prop?("my-mime-type"))

    assert(Svn::Core::Property.has_svn_prop?({"svn:mime-type" => "text/plain"}))
    assert(!Svn::Core::Property.has_svn_prop?({"my-mime-type" => "text/plain"}))

    assert(Svn::Core::Property.have_svn_prop?({"svn:mime-type" => "text/plain"}))
    assert(!Svn::Core::Property.have_svn_prop?({"my-mime-type" => "text/plain"}))
  end

  def test_valid_prop_name
    assert(Svn::Core::Property.valid_name?("svn:mime-type"))
    assert(Svn::Core::Property.valid_name?("my-mime-type"))
    assert(!Svn::Core::Property.valid_name?("プロパティ"))
  end

  def test_depth_conversion
    %w(unknown empty files immediates infinity).each do |depth|
      depth_value = Svn::Core.const_get("DEPTH_#{depth.upcase}")
      assert_equal(depth_value, Svn::Core::Depth.from_string(depth))
      assert_equal(depth, Svn::Core::Depth.to_string(depth_value))
    end
  end

  def test_depth_input
    depth_infinity = Svn::Core::DEPTH_INFINITY
    assert_equal("infinity", Svn::Core::Depth.to_string(depth_infinity))
    assert_equal("infinity", Svn::Core::Depth.to_string("infinity"))
    assert_equal("infinity", Svn::Core::Depth.to_string(:infinity))
    assert_equal("unknown", Svn::Core::Depth.to_string("XXX"))
    assert_raises(ArgumentError) do
      Svn::Core::Depth.to_string([])
    end
  end

  def test_stream_copy
    source = "content"
    original = StringIO.new(source)
    copied = StringIO.new("")
    original_stream = Svn::Core::Stream.new(original)
    copied_stream = Svn::Core::Stream.new(copied)


    original_stream.copy(copied_stream)

    copied.rewind
    assert_equal("", original.read)
    assert_equal(source, copied.read)


    original.rewind
    copied.string = ""
    assert_raises(Svn::Error::Cancelled) do
      original_stream.copy(copied_stream) do
        raise Svn::Error::Cancelled
      end
    end

    copied.rewind
    assert_equal(source, original.read)
    assert_equal("", copied.read)
  end

  def test_mime_type_parse
    type_map = {
      "html" => "text/html",
      "htm" => "text/html",
      "png" => "image/png",
    }
    mime_types_source = <<-EOM
text/html html htm
application/octet-stream

image/png png
EOM

    mime_types = Tempfile.new("svn-ruby-mime-type")
    mime_types.puts(mime_types_source)
    mime_types.close

    assert_equal(type_map, Svn::Core::MimeType.parse_file(mime_types.path))
    assert_equal(type_map, Svn::Core::MimeType.parse(mime_types_source))
  end

  def test_mime_type_detect
    empty_file = Tempfile.new("svn-ruby-mime-type")
    assert_equal(nil, Svn::Core::MimeType.detect(empty_file.path))

    binary_file = Tempfile.new("svn-ruby-mime-type")
    binary_file.print("\0\1\2")
    binary_file.close
    assert_equal("application/octet-stream",
                 Svn::Core::MimeType.detect(binary_file.path))

    text_file = Tempfile.new("svn-ruby-mime-type")
    text_file.print("abcde")
    text_file.close
    assert_equal(nil, Svn::Core::MimeType.detect(text_file.path))
  end

  def test_mime_type_detect_with_type_map
    type_map = {
      "html" => "text/html",
      "htm" => "text/html",
      "png" => "image/png",
    }

    nonexistent_html_file = File.join(@tmp_path, "nonexistent.html")
    assert_raises(Svn::Error::BadFilename) do
      Svn::Core::MimeType.detect(nonexistent_html_file)
    end
    assert_equal("text/html",
                 Svn::Core::MimeType.detect(nonexistent_html_file, type_map))

    empty_html_file = File.join(@tmp_path, "empty.html")
    FileUtils.touch(empty_html_file)
    assert_equal(nil, Svn::Core::MimeType.detect(empty_html_file))
    assert_equal("text/html",
                 Svn::Core::MimeType.detect(empty_html_file, type_map))

    empty_htm_file = File.join(@tmp_path, "empty.htm")
    FileUtils.touch(empty_htm_file)
    assert_equal(nil, Svn::Core::MimeType.detect(empty_htm_file))
    assert_equal("text/html",
                 Svn::Core::MimeType.detect(empty_htm_file, type_map))


    dummy_png_file = File.join(@tmp_path, "dummy.png")
    File.open(dummy_png_file, "wb") do |png|
      png.print("\211PNG\r\n\032\n")
    end
    assert_equal(nil, Svn::Core::MimeType.detect(dummy_png_file))
    assert_equal("image/png",
                 Svn::Core::MimeType.detect(dummy_png_file, type_map))

    empty_png_file = File.join(@tmp_path, "empty.png")
    FileUtils.touch(empty_png_file)
    assert_equal(nil, Svn::Core::MimeType.detect(empty_png_file))
    assert_equal("image/png",
                 Svn::Core::MimeType.detect(empty_png_file, type_map))

    invalid_png_file = File.join(@tmp_path, "invalid.png")
    File.open(invalid_png_file, "w") do |png|
      png.puts("text")
    end
    assert_equal(nil, Svn::Core::MimeType.detect(invalid_png_file))
    assert_equal("image/png",
                 Svn::Core::MimeType.detect(invalid_png_file, type_map))
  end

  def test_prop_categorize
    name = "svn:mime-type"
    value = "text/plain"
    entry_name = "svn:entry:XXX"
    entry_value = "XXX"

    props = [Svn::Core::Prop.new(name, value),
             Svn::Core::Prop.new(entry_name, entry_value)]
    assert_equal([
                  [Svn::Core::Prop.new(entry_name, entry_value)],
                  [],
                  [Svn::Core::Prop.new(name, value)],
                 ],
                 Svn::Core::Property.categorize(props))

    props = {name => value, entry_name => entry_value}
    assert_equal([
                  {entry_name => entry_value },
                  {},
                  {name => value},
                 ],
                 Svn::Core::Property.categorize2(props))
  end

  def test_mergeinfo_parse
    assert_equal({}, Svn::Core::MergeInfo.parse(""))

    input = "/trunk: 5,7-9,10,11,13,14"
    result = Svn::Core::MergeInfo.parse(input)
    assert_equal(["/trunk"], result.keys)
    assert_equal([[4, 5, true], [6, 11, true], [12, 14, true]],
                 result["/trunk"].collect {|range| range.to_a})

    input = "/trunk: 5*,7-9,10,11,13,14"
    result = Svn::Core::MergeInfo.parse(input)
    assert_equal(["/trunk"], result.keys)
    assert_equal([[4, 5, false], [6, 11, true], [12, 14, true]],
                 result["/trunk"].collect {|range| range.to_a})
  end

  def test_mergeinfo_diff
    input1 = "/trunk: 5,7-9,10,11,13,14"
    input2 = "/trunk: 5,6,7-9,10,11"

    info1 = Svn::Core::MergeInfo.parse(input1)
    info2 = Svn::Core::MergeInfo.parse(input2)
    result = info1.diff(info2)
    deleted, added = result
    assert_equal(["/trunk"], deleted.keys)
    assert_equal(["/trunk"], added.keys)
    assert_equal([[12, 14, true]],
                 deleted["/trunk"].collect {|range| range.to_a})
    assert_equal([[5, 6, true]],
                 added["/trunk"].collect {|range| range.to_a})
  end

  def test_mergeinfo_merge
    info = Svn::Core::MergeInfo.parse("/trunk: 5,7-9")
    assert_equal(["/trunk"], info.keys)
    assert_equal([[4, 5, true], [6, 9, true]],
                 info["/trunk"].collect {|range| range.to_a})

    changes = Svn::Core::MergeInfo.parse("/trunk: 6-13")
    merged = info.merge(changes)
    assert_equal(["/trunk"], merged.keys)
    assert_equal([[4, 13, true]],
                 merged["/trunk"].collect {|range| range.to_a})
  end

  def test_mergeinfo_remove
    info = Svn::Core::MergeInfo.parse("/trunk: 5-13")
    assert_equal(["/trunk"], info.keys)
    assert_equal([[4, 13, true]],
                 info["/trunk"].collect {|range| range.to_a})

    eraser = Svn::Core::MergeInfo.parse("/trunk: 7,9-11")
    removed = info.remove(eraser)
    assert_equal(["/trunk"], removed.keys)
    assert_equal([[4, 6, true], [7, 8, true], [11, 13, true]],
                 removed["/trunk"].collect {|range| range.to_a})
  end

  def test_mergeinfo_to_s
    info = Svn::Core::MergeInfo.parse("/trunk: 5,7,9-13")
    assert_equal("/trunk:5,7,9-13", info.to_s)
    assert_not_equal(info.to_s, info.inspect)

    info = Svn::Core::MergeInfo.parse("/trunk: 5*,7,9-13")
    assert_equal("/trunk:5*,7,9-13", info.to_s)
    assert_not_equal(info.to_s, info.inspect)
  end

  def test_mergeinfo_sort
    info = Svn::Core::MergeInfo.parse("/trunk: 5,7,9-13")

    info["/trunk"] = info["/trunk"].reverse
    assert_equal(["/trunk"], info.keys)
    assert_equal([[13, 8, true], [7, 6, true], [5, 4, true]],
                 info["/trunk"].collect {|range| range.to_a})

    sorted_info = info.sort
    assert_equal(["/trunk"], sorted_info.keys)
    assert_equal([[5, 4, true], [7, 6, true], [13, 8, true]],
                 sorted_info["/trunk"].collect {|range| range.to_a})
  end

  def test_range_list_diff
    range_list1 = Svn::Core::RangeList.new([4, 5, true], [9, 13, true])
    range_list2 = Svn::Core::RangeList.new([7, 11, true])

    deleted, added = range_list1.diff(range_list2)
    assert_equal([[7, 9, true]], added.collect {|range| range.to_a})
    assert_equal([[4, 5, true], [11, 13, true]],
                 deleted.collect {|range| range.to_a})
  end

  def test_range_list_merge
    range_list1 = Svn::Core::RangeList.new([4, 5, true],
                                           [7, 8, true], [9, 13, true])
    range_list2 = Svn::Core::RangeList.new([5, 9, true])

    merged = range_list1.merge(range_list2)
    assert_equal([[4, 13, true]], merged.collect {|range| range.to_a})
  end

  def test_range_list_remove
    range_list1 = Svn::Core::RangeList.new([4, 5, true],
                                           [7, 8, true], [10, 13, true])
    range_list2 = Svn::Core::RangeList.new([4, 9, true])

    removed = range_list1.remove(range_list2)
    assert_equal([[10, 13, true]], removed.collect {|range| range.to_a})
  end

  def test_range_list_intersect
    range_list1 = Svn::Core::RangeList.new([4, 9, true])
    range_list2 = Svn::Core::RangeList.new([4, 5, true],
                                           [7, 8, true], [9, 13, true])

    intersected = range_list1.intersect(range_list2)
    assert_equal([[4, 5, true], [7, 8, true]],
                 intersected.collect {|range| range.to_a})
  end

  def test_range_list_reverse
    range_list = Svn::Core::RangeList.new([4, 5, true],
                                          [7, 8, true], [10, 13, true])

    reversed = range_list.reverse
    assert_equal([[13, 10, true], [8, 7, true], [5, 4, true]],
                 reversed.collect {|range| range.to_a})
  end

  def test_range_list_to_s
    range_list = Svn::Core::RangeList.new([4, 6, true],
                                          [6, 8, true], [9, 13, true])
    expectation = "5-6,7-8,10-13"
    assert_equal(expectation, range_list.to_s)
    assert_not_equal(expectation, range_list.inspect)
  end

  def test_mergerange_equality
    mergerange1 = Svn::Core::MergeRange.new(1,2,true)
    mergerange2 = Svn::Core::MergeRange.new(1,2,true)
    mergerange3 = Svn::Core::MergeRange.new(1,2,false)
    mergerange4 = Svn::Core::MergeRange.new(1,4,true)

    assert_equal(mergerange1, mergerange2)
    assert_not_equal(mergerange1, mergerange3)
    assert_not_equal(mergerange1, mergerange4)
  end

  private
  def used_pool
    pool = Svn::Core::Pool.new
    now = Time.now.gmtime
    Svn::Core.time_to_human_cstring(now.to_apr_time, pool)
    pool
  end
end
