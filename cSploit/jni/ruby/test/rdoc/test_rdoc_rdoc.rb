require 'tempfile'
require 'tmpdir'
require 'rubygems'
require 'minitest/autorun'
require 'rdoc/rdoc'

class TestRDocRDoc < MiniTest::Unit::TestCase

  def setup
    @rdoc = RDoc::RDoc.new
    @tempfile = Tempfile.new 'test_rdoc_rdoc'
    @tempfile.binmode
  end

  def teardown
    @tempfile.close rescue nil # HACK for 1.8.6
  end

  def test_gather_files
    file = File.expand_path __FILE__
    assert_equal [file], @rdoc.gather_files([file, file])
  end

  def test_normalized_file_list
    files = @rdoc.normalized_file_list [__FILE__]

    files = files.map { |file| File.expand_path file }

    assert_equal [File.expand_path(__FILE__)], files
  end

  def test_normalized_file_list_not_modified
    files = [__FILE__]

    @rdoc.last_modified[__FILE__] = File.stat(__FILE__).mtime

    files = @rdoc.normalized_file_list [__FILE__]

    assert_empty files
  end

  def test_read_file_contents
    @tempfile.write "hi everybody"
    @tempfile.flush

    assert_equal "hi everybody", @rdoc.read_file_contents(@tempfile.path)
  end

  def test_read_file_contents_encoding
    skip "Encoding not implemented" unless defined? ::Encoding

    @tempfile.write "# coding: utf-8\nhi everybody"
    @tempfile.flush

    contents = @rdoc.read_file_contents @tempfile.path
    assert_equal "# coding: utf-8\nhi everybody", contents
    assert_equal Encoding::UTF_8, contents.encoding
  end

  def test_read_file_contents_encoding_fancy
    skip "Encoding not implemented" unless defined? ::Encoding

    @tempfile.write "# -*- coding: utf-8; fill-column: 74 -*-\nhi everybody"
    @tempfile.flush

    contents = @rdoc.read_file_contents @tempfile.path
    assert_equal("# -*- coding: utf-8; fill-column: 74 -*-\nhi everybody",
                 contents)
    assert_equal Encoding::UTF_8, contents.encoding
  end

  def test_read_file_contents_encoding_with_signature
    skip "Encoding not implemented" unless defined? ::Encoding

    @tempfile.write "\xEF\xBB\xBF""hi everybody"
    @tempfile.flush

    bug3360 = '[ruby-dev:41452]'
    contents = @rdoc.read_file_contents @tempfile.path
    assert_equal "hi everybody", contents, bug3360
    assert_equal Encoding::UTF_8, contents.encoding, bug3360
  end

  def test_remove_unparsable
    file_list = %w[
      blah.class
      blah.eps
      blah.erb
      blah.scpt.txt
      blah.ttf
      blah.yml
    ]

    assert_empty @rdoc.remove_unparseable file_list
  end

  def test_setup_output_dir
    skip "No Dir::mktmpdir, upgrade your ruby" unless Dir.respond_to? :mktmpdir

    Dir.mktmpdir {|d|
      path = File.join(d, 'testdir')

      last = @rdoc.setup_output_dir path, false

      assert_empty last

      assert File.directory? path
    }
  end

  def test_setup_output_dir_exists
    skip "No Dir::mktmpdir, upgrade your ruby" unless Dir.respond_to? :mktmpdir

    Dir.mktmpdir {|path|
      open @rdoc.output_flag_file(path), 'w' do |io|
        io.puts Time.at 0
        io.puts "./lib/rdoc.rb\t#{Time.at 86400}"
      end

      last = @rdoc.setup_output_dir path, false

      assert_equal 1, last.size
      assert_equal Time.at(86400), last['./lib/rdoc.rb']
    }
  end

  def test_setup_output_dir_exists_empty_created_rid
    skip "No Dir::mktmpdir, upgrade your ruby" unless Dir.respond_to? :mktmpdir

    Dir.mktmpdir {|path|
      open @rdoc.output_flag_file(path), 'w' do end

      e = assert_raises RDoc::Error do
        @rdoc.setup_output_dir path, false
      end

      assert_match %r%Directory #{Regexp.escape path} already exists%, e.message
    }
  end

  def test_setup_output_dir_exists_file
    path = @tempfile.path

    e = assert_raises RDoc::Error do
      @rdoc.setup_output_dir path, false
    end

    assert_match(%r%#{Regexp.escape path} exists and is not a directory%,
                 e.message)
  end

  def test_setup_output_dir_exists_not_rdoc
    skip "No Dir::mktmpdir, upgrade your ruby" unless Dir.respond_to? :mktmpdir

    Dir.mktmpdir do |dir|
      e = assert_raises RDoc::Error do
        @rdoc.setup_output_dir dir, false
      end

      assert_match %r%Directory #{Regexp.escape dir} already exists%, e.message
    end
  end

end

