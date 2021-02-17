require 'test/unit'
require 'stringio'
require 'tempfile'

begin
  require 'zlib'
rescue LoadError
end

if defined? Zlib
  class TestZlibDeflate < Test::Unit::TestCase
    def test_initialize
      z = Zlib::Deflate.new
      s = z.deflate("foo", Zlib::FINISH)
      assert_equal("foo", Zlib::Inflate.inflate(s))

      z = Zlib::Deflate.new
      s = z.deflate("foo")
      s << z.deflate(nil, Zlib::FINISH)
      assert_equal("foo", Zlib::Inflate.inflate(s))

      assert_raise(Zlib::StreamError) { Zlib::Deflate.new(10000) }
    end

    def test_dup
      z1 = Zlib::Deflate.new
      s = z1.deflate("foo")
      z2 = z1.dup
      s1 = s + z1.deflate("bar", Zlib::FINISH)
      s2 = s + z2.deflate("baz", Zlib::FINISH)
      assert_equal("foobar", Zlib::Inflate.inflate(s1))
      assert_equal("foobaz", Zlib::Inflate.inflate(s2))
    end

    def test_deflate
      s = Zlib::Deflate.deflate("foo")
      assert_equal("foo", Zlib::Inflate.inflate(s))

      assert_raise(Zlib::StreamError) { Zlib::Deflate.deflate("foo", 10000) }
    end

    def test_addstr
      z = Zlib::Deflate.new
      z << "foo"
      s = z.deflate(nil, Zlib::FINISH)
      assert_equal("foo", Zlib::Inflate.inflate(s))
    end

    def test_flush
      z = Zlib::Deflate.new
      z << "foo"
      s = z.flush
      z << "bar"
      s << z.flush_next_in
      z << "baz"
      s << z.flush_next_out
      s << z.deflate("qux", Zlib::FINISH)
      assert_equal("foobarbazqux", Zlib::Inflate.inflate(s))
    end

    def test_avail
      z = Zlib::Deflate.new
      assert_equal(0, z.avail_in)
      assert_equal(0, z.avail_out)
      z << "foo"
      z.avail_out += 100
      z << "bar"
      s = z.finish
      assert_equal("foobar", Zlib::Inflate.inflate(s))
    end

    def test_total
      z = Zlib::Deflate.new
      1000.times { z << "foo" }
      s = z.finish
      assert_equal(3000, z.total_in)
      assert_operator(3000, :>, z.total_out)
      assert_equal("foo" * 1000, Zlib::Inflate.inflate(s))
    end

    def test_data_type
      z = Zlib::Deflate.new
      assert([Zlib::ASCII, Zlib::BINARY, Zlib::UNKNOWN].include?(z.data_type))
    end

    def test_adler
      z = Zlib::Deflate.new
      z << "foo"
      s = z.finish
      assert_equal(0x02820145, z.adler)
    end

    def test_finished_p
      z = Zlib::Deflate.new
      assert_equal(false, z.finished?)
      z << "foo"
      assert_equal(false, z.finished?)
      s = z.finish
      assert_equal(true, z.finished?)
      z.close
      assert_raise(Zlib::Error) { z.finished? }
    end

    def test_closed_p
      z = Zlib::Deflate.new
      assert_equal(false, z.closed?)
      z << "foo"
      assert_equal(false, z.closed?)
      s = z.finish
      assert_equal(false, z.closed?)
      z.close
      assert_equal(true, z.closed?)
    end

    def test_params
      z = Zlib::Deflate.new
      z << "foo"
      z.params(Zlib::DEFAULT_COMPRESSION, Zlib::DEFAULT_STRATEGY)
      z << "bar"
      s = z.finish
      assert_equal("foobar", Zlib::Inflate.inflate(s))

      data = ('a'..'z').to_a.join
      z = Zlib::Deflate.new(Zlib::NO_COMPRESSION, Zlib::MAX_WBITS,
                            Zlib::DEF_MEM_LEVEL, Zlib::DEFAULT_STRATEGY)
      z << data[0, 10]
      z.params(Zlib::BEST_COMPRESSION, Zlib::DEFAULT_STRATEGY)
      z << data[10 .. -1]
      assert_equal(data, Zlib::Inflate.inflate(z.finish))

      z = Zlib::Deflate.new
      s = z.deflate("foo", Zlib::FULL_FLUSH)
      z.avail_out = 0
      z.params(Zlib::NO_COMPRESSION, Zlib::FILTERED)
      s << z.deflate("bar", Zlib::FULL_FLUSH)
      z.avail_out = 0
      z.params(Zlib::BEST_COMPRESSION, Zlib::HUFFMAN_ONLY)
      s << z.deflate("baz", Zlib::FINISH)
      assert_equal("foobarbaz", Zlib::Inflate.inflate(s))

      z = Zlib::Deflate.new
      assert_raise(Zlib::StreamError) { z.params(10000, 10000) }
      z.close # without this, outputs `zlib(finalizer): the stream was freed prematurely.'
    end

    def test_set_dictionary
      z = Zlib::Deflate.new
      z.set_dictionary("foo")
      s = z.deflate("foo" * 100, Zlib::FINISH)
      z = Zlib::Inflate.new
      assert_raise(Zlib::NeedDict) { z.inflate(s) }
      z.set_dictionary("foo")
      assert_equal("foo" * 100, z.inflate(s)) # ???

      z = Zlib::Deflate.new
      z << "foo"
      assert_raise(Zlib::StreamError) { z.set_dictionary("foo") }
      z.close # without this, outputs `zlib(finalizer): the stream was freed prematurely.'
    end

    def test_reset
      z = Zlib::Deflate.new
      z << "foo"
      z.reset
      z << "bar"
      s = z.finish
      assert_equal("bar", Zlib::Inflate.inflate(s))
    end

    def test_close
      z = Zlib::Deflate.new
      z.close
      assert_raise(Zlib::Error) { z << "foo" }
      assert_raise(Zlib::Error) { z.reset }
    end
  end

  class TestZlibInflate < Test::Unit::TestCase
    def test_initialize
      assert_raise(Zlib::StreamError) { Zlib::Inflate.new(-1) }

      s = Zlib::Deflate.deflate("foo")
      z = Zlib::Inflate.new
      z << s << nil
      assert_equal("foo", z.finish)
    end

    def test_inflate
      s = Zlib::Deflate.deflate("foo")
      z = Zlib::Inflate.new
      s = z.inflate(s)
      s << z.inflate(nil)
      assert_equal("foo", s)
      z.inflate("foo") # ???
      z << "foo" # ???
    end

    def test_sync
      z = Zlib::Deflate.new
      s = z.deflate("foo" * 1000, Zlib::FULL_FLUSH)
      z.avail_out = 0
      z.params(Zlib::NO_COMPRESSION, Zlib::FILTERED)
      s << z.deflate("bar" * 1000, Zlib::FULL_FLUSH)
      z.avail_out = 0
      z.params(Zlib::BEST_COMPRESSION, Zlib::HUFFMAN_ONLY)
      s << z.deflate("baz" * 1000, Zlib::FINISH)

      z = Zlib::Inflate.new
      assert_raise(Zlib::DataError) { z << "\0" * 100 }
      assert_equal(false, z.sync(""))
      assert_equal(false, z.sync_point?)

      z = Zlib::Inflate.new
      assert_raise(Zlib::DataError) { z << "\0" * 100 + s }
      assert_equal(true, z.sync(""))
      #assert_equal(true, z.sync_point?)

      z = Zlib::Inflate.new
      assert_equal(false, z.sync("\0" * 100))
      assert_equal(false, z.sync_point?)

      z = Zlib::Inflate.new
      assert_equal(true, z.sync("\0" * 100 + s))
      #assert_equal(true, z.sync_point?)
    end

    def test_set_dictionary
      z = Zlib::Inflate.new
      assert_raise(Zlib::StreamError) { z.set_dictionary("foo") }
      z.close
    end
  end

  class TestZlibGzipFile < Test::Unit::TestCase
    def test_to_io
      t = Tempfile.new("test_zlib_gzip_file")
      t.close
      Zlib::GzipWriter.open(t.path) {|gz| gz.print("foo") }

      f = Zlib::GzipReader.open(t.path)
      assert_kind_of(IO, f.to_io)
    end

    def test_crc
      t = Tempfile.new("test_zlib_gzip_file")
      t.close
      Zlib::GzipWriter.open(t.path) {|gz| gz.print("foo") }

      f = Zlib::GzipReader.open(t.path)
      f.read
      assert_equal(0x8c736521, f.crc)
    end

    def test_mtime
      tim = Time.now

      t = Tempfile.new("test_zlib_gzip_file")
      t.close
      Zlib::GzipWriter.open(t.path) do |gz|
        gz.mtime = -1
        gz.mtime = tim
        gz.print("foo")
        gz.flush
        assert_raise(Zlib::GzipFile::Error) { gz.mtime = Time.now }
      end

      f = Zlib::GzipReader.open(t.path)
      assert_equal(tim.to_i, f.mtime.to_i)
    end

    def test_level
      t = Tempfile.new("test_zlib_gzip_file")
      t.close
      Zlib::GzipWriter.open(t.path) {|gz| gz.print("foo") }

      f = Zlib::GzipReader.open(t.path)
      assert_equal(Zlib::DEFAULT_COMPRESSION, f.level)
    end

    def test_os_code
      t = Tempfile.new("test_zlib_gzip_file")
      t.close
      Zlib::GzipWriter.open(t.path) {|gz| gz.print("foo") }

      f = Zlib::GzipReader.open(t.path)
      assert_equal(Zlib::OS_CODE, f.os_code)
    end

    def test_orig_name
      t = Tempfile.new("test_zlib_gzip_file")
      t.close
      Zlib::GzipWriter.open(t.path) do |gz|
        gz.orig_name = "foobarbazqux\0quux"
        gz.print("foo")
        gz.flush
        assert_raise(Zlib::GzipFile::Error) { gz.orig_name = "quux" }
      end

      f = Zlib::GzipReader.open(t.path)
      assert_equal("foobarbazqux", f.orig_name)
    end

    def test_comment
      t = Tempfile.new("test_zlib_gzip_file")
      t.close
      Zlib::GzipWriter.open(t.path) do |gz|
        gz.comment = "foobarbazqux\0quux"
        gz.print("foo")
        gz.flush
        assert_raise(Zlib::GzipFile::Error) { gz.comment = "quux" }
      end

      f = Zlib::GzipReader.open(t.path)
      assert_equal("foobarbazqux", f.comment)
    end

    def test_lineno
      t = Tempfile.new("test_zlib_gzip_file")
      t.close
      Zlib::GzipWriter.open(t.path) {|gz| gz.print("foo\nbar\nbaz\nqux\n") }

      f = Zlib::GzipReader.open(t.path)
      assert_equal([0, "foo\n"], [f.lineno, f.gets])
      assert_equal([1, "bar\n"], [f.lineno, f.gets])
      f.lineno = 1000
      assert_equal([1000, "baz\n"], [f.lineno, f.gets])
      assert_equal([1001, "qux\n"], [f.lineno, f.gets])
    end

    def test_closed_p
      t = Tempfile.new("test_zlib_gzip_file")
      t.close
      Zlib::GzipWriter.open(t.path) {|gz| gz.print("foo") }

      f = Zlib::GzipReader.open(t.path)
      assert_equal(false, f.closed?)
      f.read
      assert_equal(false, f.closed?)
      f.close
      assert_equal(true, f.closed?)
    end

    def test_sync
      t = Tempfile.new("test_zlib_gzip_file")
      t.close
      Zlib::GzipWriter.open(t.path) {|gz| gz.print("foo") }

      f = Zlib::GzipReader.open(t.path)
      f.sync = true
      assert_equal(true, f.sync)
      f.read
      f.sync = false
      assert_equal(false, f.sync)
      f.close
    end

    def test_pos
      t = Tempfile.new("test_zlib_gzip_file")
      t.close
      Zlib::GzipWriter.open(t.path) do |gz|
        gz.print("foo")
        gz.flush
        assert_equal(3, gz.tell)
      end
    end

    def test_path
      t = Tempfile.new("test_zlib_gzip_file")
      t.close

      gz = Zlib::GzipWriter.open(t.path)
      gz.print("foo")
      assert_equal(t.path, gz.path)
      gz.close
      assert_equal(t.path, gz.path)

      f = Zlib::GzipReader.open(t.path)
      assert_equal(t.path, f.path)
      f.close
      assert_equal(t.path, f.path)

      s = ""
      sio = StringIO.new(s)
      gz = Zlib::GzipWriter.new(sio)
      gz.print("foo")
      assert_raise(NoMethodError) { gz.path }
      gz.close

      sio = StringIO.new(s)
      f = Zlib::GzipReader.new(sio)
      assert_raise(NoMethodError) { f.path }
      f.close
    end
  end

  class TestZlibGzipReader < Test::Unit::TestCase
    D0 = "\037\213\010\000S`\017A\000\003\003\000\000\000\000\000\000\000\000\000"
    def test_read0
      assert_equal("", Zlib::GzipReader.new(StringIO.new(D0)).read(0))
    end

    def test_ungetc
      s = ""
      w = Zlib::GzipWriter.new(StringIO.new(s))
      w << (1...1000).to_a.inspect
      w.close
      r = Zlib::GzipReader.new(StringIO.new(s))
      r.read(100)
      r.ungetc ?a
      assert_nothing_raised("[ruby-dev:24060]") {
        r.read(100)
        r.read
        r.close
      }
    end

    def test_ungetc_paragraph
      s = ""
      w = Zlib::GzipWriter.new(StringIO.new(s))
      w << "abc"
      w.close
      r = Zlib::GzipReader.new(StringIO.new(s))
      r.ungetc ?\n
      assert_equal("abc", r.gets(""))
      assert_nothing_raised("[ruby-dev:24065]") {
        r.read
        r.close
      }
    end

    def test_open
      t = Tempfile.new("test_zlib_gzip_reader")
      t.close
      e = assert_raise(Zlib::GzipFile::Error) {
        Zlib::GzipReader.open(t.path)
      }
      assert_equal("not in gzip format", e.message)
      assert_nil(e.input)
      open(t.path, "wb") {|f| f.write("foo")}
      e = assert_raise(Zlib::GzipFile::Error) {
        Zlib::GzipReader.open(t.path)
      }
      assert_equal("not in gzip format", e.message)
      assert_equal("foo", e.input)
      open(t.path, "wb") {|f| f.write("foobarzothoge")}
      e = assert_raise(Zlib::GzipFile::Error) {
        Zlib::GzipReader.open(t.path)
      }
      assert_equal("not in gzip format", e.message)
      assert_equal("foobarzothoge", e.input)

      Zlib::GzipWriter.open(t.path) {|gz| gz.print("foo") }

      assert_raise(ArgumentError) { Zlib::GzipReader.open }

      assert_equal("foo", Zlib::GzipReader.open(t.path) {|gz| gz.read })

      f = Zlib::GzipReader.open(t.path)
      assert_equal("foo", f.read)
      f.close
    end

    def test_rewind
      t = Tempfile.new("test_zlib_gzip_reader")
      t.close
      Zlib::GzipWriter.open(t.path) {|gz| gz.print("foo") }

      f = Zlib::GzipReader.open(t.path)
      assert_equal("foo", f.read)
      f.rewind
      assert_equal("foo", f.read)
      f.close
    end

    def test_unused
      t = Tempfile.new("test_zlib_gzip_reader")
      t.close
      Zlib::GzipWriter.open(t.path) {|gz| gz.print("foobar") }

      f = Zlib::GzipReader.open(t.path)
      assert_equal("foo", f.read(3))
      f.unused
      assert_equal("bar", f.read)
      f.unused
      f.close
    end

    def test_read
      t = Tempfile.new("test_zlib_gzip_reader")
      t.close
      str = "\u3042\u3044\u3046"
      Zlib::GzipWriter.open(t.path) {|gz| gz.print(str) }

      f = Zlib::GzipReader.open(t.path, encoding: "UTF-8")
      assert_raise(ArgumentError) { f.read(-1) }
      assert_equal(str, f.read)
    end

    def test_readpartial
      t = Tempfile.new("test_zlib_gzip_reader")
      t.close
      Zlib::GzipWriter.open(t.path) {|gz| gz.print("foobar") }

      f = Zlib::GzipReader.open(t.path)
      assert("foo".start_with?(f.readpartial(3)))

      f = Zlib::GzipReader.open(t.path)
      s = ""
      f.readpartial(3, s)
      assert("foo".start_with?(s))

      assert_raise(ArgumentError) { f.readpartial(-1) }
    end

    def test_getc
      t = Tempfile.new("test_zlib_gzip_reader")
      t.close
      Zlib::GzipWriter.open(t.path) {|gz| gz.print("foobar") }

      f = Zlib::GzipReader.open(t.path)
      "foobar".each_char {|c| assert_equal(c, f.getc) }
      assert_nil(f.getc)
    end

    def test_getbyte
      t = Tempfile.new("test_zlib_gzip_reader")
      t.close
      Zlib::GzipWriter.open(t.path) {|gz| gz.print("foobar") }

      f = Zlib::GzipReader.open(t.path)
      "foobar".each_byte {|c| assert_equal(c, f.getbyte) }
      assert_nil(f.getbyte)
    end

    def test_readchar
      t = Tempfile.new("test_zlib_gzip_reader")
      t.close
      Zlib::GzipWriter.open(t.path) {|gz| gz.print("foobar") }

      f = Zlib::GzipReader.open(t.path)
      "foobar".each_byte {|c| assert_equal(c, f.readchar.ord) }
      assert_raise(EOFError) { f.readchar }
    end

    def test_each_byte
      t = Tempfile.new("test_zlib_gzip_reader")
      t.close
      Zlib::GzipWriter.open(t.path) {|gz| gz.print("foobar") }

      f = Zlib::GzipReader.open(t.path)
      a = []
      f.each_byte {|c| a << c }
      assert_equal("foobar".each_byte.to_a, a)
    end

    def test_gets2
      t = Tempfile.new("test_zlib_gzip_reader")
      t.close
      Zlib::GzipWriter.open(t.path) {|gz| gz.print("foo\nbar\nbaz\n") }

      f = Zlib::GzipReader.open(t.path)
      assert_equal("foo\n", f.gets)
      assert_equal("bar\n", f.gets)
      assert_equal("baz\n", f.gets)
      assert_nil(f.gets)
      f.close

      f = Zlib::GzipReader.open(t.path)
      assert_equal("foo\nbar\nbaz\n", f.gets(nil))
      f.close
    end

    def test_gets
      t = Tempfile.new("test_zlib_gzip_reader")
      t.close
      Zlib::GzipWriter.open(t.path) {|gz| gz.print("foo\nbar\nbaz\n") }

      f = Zlib::GzipReader.open(t.path)
      assert_equal("foo\n", f.readline)
      assert_equal("bar\n", f.readline)
      assert_equal("baz\n", f.readline)
      assert_raise(EOFError) { f.readline }
      f.close
    end

    def test_each
      t = Tempfile.new("test_zlib_gzip_reader")
      t.close
      Zlib::GzipWriter.open(t.path) {|gz| gz.print("foo\nbar\nbaz\n") }

      f = Zlib::GzipReader.open(t.path)
      a = ["foo\n", "bar\n", "baz\n"]
      f.each {|l| assert_equal(a.shift, l) }
      f.close
    end

    def test_readlines
      t = Tempfile.new("test_zlib_gzip_reader")
      t.close
      Zlib::GzipWriter.open(t.path) {|gz| gz.print("foo\nbar\nbaz\n") }

      f = Zlib::GzipReader.open(t.path)
      assert_equal(["foo\n", "bar\n", "baz\n"], f.readlines)
      f.close
    end

    def test_reader_wrap
      t = Tempfile.new("test_zlib_gzip_reader")
      t.close
      Zlib::GzipWriter.open(t.path) {|gz| gz.print("foo") }
      f = open(t.path)
      assert_equal("foo", Zlib::GzipReader.wrap(f) {|gz| gz.read })
      assert_raise(IOError) { f.close }
    end
  end

  class TestZlibGzipWriter < Test::Unit::TestCase
    def test_invalid_new
      assert_raise(NoMethodError, "[ruby-dev:23228]") { Zlib::GzipWriter.new(nil).close }
      assert_raise(NoMethodError, "[ruby-dev:23344]") { Zlib::GzipWriter.new(true).close }
      assert_raise(NoMethodError, "[ruby-dev:23344]") { Zlib::GzipWriter.new(0).close }
      assert_raise(NoMethodError, "[ruby-dev:23344]") { Zlib::GzipWriter.new(:hoge).close }
    end

    def test_open
      assert_raise(ArgumentError) { Zlib::GzipWriter.open }

      t = Tempfile.new("test_zlib_gzip_writer")
      t.close
      Zlib::GzipWriter.open(t.path) {|gz| gz.print("foo") }
      assert_equal("foo", Zlib::GzipReader.open(t.path) {|gz| gz.read })

      f = Zlib::GzipWriter.open(t.path)
      f.print("bar")
      f.close
      assert_equal("bar", Zlib::GzipReader.open(t.path) {|gz| gz.read })

      assert_raise(Zlib::StreamError) { Zlib::GzipWriter.open(t.path, 10000) }
    end

    def test_write
      t = Tempfile.new("test_zlib_gzip_writer")
      t.close
      Zlib::GzipWriter.open(t.path) {|gz| gz.print("foo") }
      assert_equal("foo", Zlib::GzipReader.open(t.path) {|gz| gz.read })

      o = Object.new
      def o.to_s; "bar"; end
      Zlib::GzipWriter.open(t.path) {|gz| gz.print(o) }
      assert_equal("bar", Zlib::GzipReader.open(t.path) {|gz| gz.read })
    end

    def test_putc
      t = Tempfile.new("test_zlib_gzip_writer")
      t.close
      Zlib::GzipWriter.open(t.path) {|gz| gz.putc(?x) }
      assert_equal("x", Zlib::GzipReader.open(t.path) {|gz| gz.read })

      # todo: multibyte char
    end

    def test_writer_wrap
      t = Tempfile.new("test_zlib_gzip_writer")
      Zlib::GzipWriter.wrap(t) {|gz| gz.print("foo") }
      t.close
      assert_equal("foo", Zlib::GzipReader.open(t.path) {|gz| gz.read })
    end
  end

  class TestZlib < Test::Unit::TestCase
    def test_version
      assert_instance_of(String, Zlib.zlib_version)
      assert(Zlib.zlib_version.tainted?)
    end

    def test_adler32
      assert_equal(0x00000001, Zlib.adler32)
      assert_equal(0x02820145, Zlib.adler32("foo"))
      assert_equal(0x02820145, Zlib.adler32("o", Zlib.adler32("fo")))
      assert_equal(0x8a62c964, Zlib.adler32("abc\x01\x02\x03" * 10000))
    end

    def test_adler32_combine
      one = Zlib.adler32("fo")
      two = Zlib.adler32("o")
      begin
        assert_equal(0x02820145, Zlib.adler32_combine(one, two, 1))
      rescue NotImplementedError
        skip "adler32_combine is not implemented"
      end
    end

    def test_crc32
      assert_equal(0x00000000, Zlib.crc32)
      assert_equal(0x8c736521, Zlib.crc32("foo"))
      assert_equal(0x8c736521, Zlib.crc32("o", Zlib.crc32("fo")))
      assert_equal(0x07f0d68f, Zlib.crc32("abc\x01\x02\x03" * 10000))
    end

    def test_crc32_combine
      one = Zlib.crc32("fo")
      two = Zlib.crc32("o")
      begin
        assert_equal(0x8c736521, Zlib.crc32_combine(one, two, 1))
      rescue NotImplementedError
        skip "crc32_combine is not implemented"
      end
    end

    def test_crc_table
      t = Zlib.crc_table
      assert_instance_of(Array, t)
      t.each {|x| assert_kind_of(Integer, x) }
    end
  end
end
