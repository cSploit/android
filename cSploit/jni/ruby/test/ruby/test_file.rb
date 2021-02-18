require 'test/unit'
require 'tempfile'
require_relative 'ut_eof'

class TestFile < Test::Unit::TestCase

  # I don't know Ruby's spec about "unlink-before-close" exactly.
  # This test asserts current behaviour.
  def test_unlink_before_close
    Dir.mktmpdir('rubytest-file') {|tmpdir|
      filename = tmpdir + '/' + File.basename(__FILE__) + ".#{$$}"
      w = File.open(filename, "w")
      w << "foo"
      w.close
      r = File.open(filename, "r")
      begin
        if /(mswin|bccwin|mingw|emx)/ =~ RUBY_PLATFORM
          assert_raise(Errno::EACCES) {File.unlink(filename)}
        else
          assert_nothing_raised {File.unlink(filename)}
        end
      ensure
        r.close
        File.unlink(filename) if File.exist?(filename)
      end
    }
  end

  include TestEOF
  def open_file(content)
    f = Tempfile.new("test-eof")
    f << content
    f.rewind
    yield f
  end
  alias open_file_rw open_file

  include TestEOF::Seek

  def test_empty_file_bom
    bug6487 = '[ruby-core:45203]'
    f = Tempfile.new(__method__.to_s)
    f.close
    assert File.exist? f.path
    assert_nothing_raised(bug6487) {File.read(f.path, mode: 'r:utf-8')}
    assert_nothing_raised(bug6487) {File.read(f.path, mode: 'r:bom|utf-8')}
    f.close(true)
  end

  def assert_bom(bytes, name)
    bug6487 = '[ruby-core:45203]'

    f = Tempfile.new(name.to_s)
    f.sync = true
    expected = ""
    result = nil
    bytes[0...-1].each do |x|
      f.write x
      f.write ' '
      f.pos -= 1
      expected << x
      assert_nothing_raised(bug6487) {result = File.read(f.path, mode: 'rb:bom|utf-8')}
      assert_equal("#{expected} ".force_encoding("utf-8"), result)
    end
    f.write bytes[-1]
    assert_nothing_raised(bug6487) {result = File.read(f.path, mode: 'rb:bom|utf-8')}
    assert_equal '', result, "valid bom"
    f.close(true)
  end

  def test_bom_8
    assert_bom(["\xEF", "\xBB", "\xBF"], __method__)
  end

  def test_bom_16be
    assert_bom(["\xFE", "\xFF"], __method__)
  end

  def test_bom_16le
    assert_bom(["\xFF", "\xFE"], __method__)
  end

  def test_bom_32be
    assert_bom(["\0", "\0", "\xFE", "\xFF"], __method__)
  end

  def test_bom_32le
    assert_bom(["\xFF\xFE\0", "\0"], __method__)
  end

  def test_truncate_wbuf
    f = Tempfile.new("test-truncate")
    f.print "abc"
    f.truncate(0)
    f.print "def"
    f.flush
    assert_equal("\0\0\0def", File.read(f.path), "[ruby-dev:24191]")
    f.close
  end

  def test_truncate_rbuf
    f = Tempfile.new("test-truncate")
    f.puts "abc"
    f.puts "def"
    f.close
    f.open
    assert_equal("abc\n", f.gets)
    f.truncate(3)
    assert_equal(nil, f.gets, "[ruby-dev:24197]")
  end

  def test_truncate_beyond_eof
    f = Tempfile.new("test-truncate")
    f.print "abc"
    f.truncate 10
    assert_equal("\0" * 7, f.read(100), "[ruby-dev:24532]")
  end

  def test_read_all_extended_file
    [nil, {:textmode=>true}, {:binmode=>true}].each do |mode|
      f = Tempfile.new("test-extended-file", mode)
      assert_nil(f.getc)
      f.print "a"
      f.rewind
      assert_equal("a", f.read, "mode = <#{mode}>")
    end
  end

  def test_gets_extended_file
    [nil, {:textmode=>true}, {:binmode=>true}].each do |mode|
      f = Tempfile.new("test-extended-file", mode)
      assert_nil(f.getc)
      f.print "a"
      f.rewind
      assert_equal("a", f.gets("a"), "mode = <#{mode}>")
    end
  end

  def test_gets_para_extended_file
    [nil, {:textmode=>true}, {:binmode=>true}].each do |mode|
      f = Tempfile.new("test-extended-file", mode)
      assert_nil(f.getc)
      f.print "\na"
      f.rewind
      assert_equal("a", f.gets(""), "mode = <#{mode}>")
    end
  end

  def test_each_char_extended_file
    [nil, {:textmode=>true}, {:binmode=>true}].each do |mode|
      f = Tempfile.new("test-extended-file", mode)
      assert_nil(f.getc)
      f.print "a"
      f.rewind
      result = []
      f.each_char {|b| result << b }
      assert_equal([?a], result, "mode = <#{mode}>")
    end
  end

  def test_each_byte_extended_file
    [nil, {:textmode=>true}, {:binmode=>true}].each do |mode|
      f = Tempfile.new("test-extended-file", mode)
      assert_nil(f.getc)
      f.print "a"
      f.rewind
      result = []
      f.each_byte {|b| result << b.chr }
      assert_equal([?a], result, "mode = <#{mode}>")
    end
  end

  def test_getc_extended_file
    [nil, {:textmode=>true}, {:binmode=>true}].each do |mode|
      f = Tempfile.new("test-extended-file", mode)
      assert_nil(f.getc)
      f.print "a"
      f.rewind
      assert_equal(?a, f.getc, "mode = <#{mode}>")
    end
  end

  def test_getbyte_extended_file
    [nil, {:textmode=>true}, {:binmode=>true}].each do |mode|
      f = Tempfile.new("test-extended-file", mode)
      assert_nil(f.getc)
      f.print "a"
      f.rewind
      assert_equal(?a, f.getbyte.chr, "mode = <#{mode}>")
    end
  end

  def test_s_chown
    assert_nothing_raised { File.chown(-1, -1) }
    assert_nothing_raised { File.chown nil, nil }
  end

  def test_chown
    assert_nothing_raised {
      File.open(__FILE__) {|f| f.chown(-1, -1) }
    }
    assert_nothing_raised("[ruby-dev:27140]") {
      File.open(__FILE__) {|f| f.chown nil, nil }
    }
  end

  def test_uninitialized
    assert_raise(TypeError) { File::Stat.allocate.readable? }
    assert_nothing_raised { File::Stat.allocate.inspect }
  end

  def test_realpath
    Dir.mktmpdir('rubytest-realpath') {|tmpdir|
      realdir = File.realpath(tmpdir)
      tst = realdir.sub(/#{Regexp.escape(File::SEPARATOR)}/, '\0\0\0')
      assert_equal(realdir, File.realpath(tst))
      assert_equal(realdir, File.realpath(".", tst))
      if File::ALT_SEPARATOR
        bug2961 = '[ruby-core:28653]'
        assert_equal(realdir, File.realpath(realdir.tr(File::SEPARATOR, File::ALT_SEPARATOR)), bug2961)
      end
    }
  end

  def test_realdirpath
    Dir.mktmpdir('rubytest-realdirpath') {|tmpdir|
      realdir = File.realpath(tmpdir)
      tst = realdir.sub(/#{Regexp.escape(File::SEPARATOR)}/, '\0\0\0')
      assert_equal(realdir, File.realdirpath(tst))
      assert_equal(realdir, File.realdirpath(".", tst))
      assert_equal(File.join(realdir, "foo"), File.realdirpath("foo", tst))
    }
  end

  def test_utime
    bug6385 = '[ruby-core:44776]'

    mod_time_contents = Time.at 1306527039

    file = Tempfile.new("utime")
    file.close
    path = file.path

    File.utime(File.atime(path), mod_time_contents, path)
    stats = File.stat(path)

    file.open
    file_mtime = file.mtime
    file.close(true)

    assert_equal(mod_time_contents, file_mtime, bug6385)
    assert_equal(mod_time_contents, stats.mtime, bug6385)
  end

  def test_chmod_m17n
    bug5671 = '[ruby-dev:44898]'
    Dir.mktmpdir('test-file-chmod-m17n-') do |tmpdir|
      file = File.join(tmpdir, "\u3042")
      File.open(file, 'w'){}
      assert_equal(File.chmod(0666, file), 1, bug5671)
    end
  end

  def test_open_nul
    Dir.mktmpdir(__method__.to_s) do |tmpdir|
      path = File.join(tmpdir, "foo")
      assert_raise(ArgumentError) do
        open(path + "\0bar", "w") {}
      end
      refute File.exist?(path)
    end
  end
end
