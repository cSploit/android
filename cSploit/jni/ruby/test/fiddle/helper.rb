require 'minitest/autorun'
require 'dl'
require 'fiddle'

# FIXME: this is stolen from DL and needs to be refactored.
require_relative '../ruby/envutil'

libc_so = libm_so = nil

case RUBY_PLATFORM
when /cygwin/
  libc_so = "cygwin1.dll"
  libm_so = "cygwin1.dll"
when /x86_64-linux/
  libc_so = "/lib64/libc.so.6"
  libm_so = "/lib64/libm.so.6"
when /linux/
  libdir = '/lib'
  case [0].pack('L!').size
  when 4
    # 32-bit ruby
    libdir = '/lib32' if File.directory? '/lib32'
  when 8
    # 64-bit ruby
    libdir = '/lib64' if File.directory? '/lib64'
  end
  libc_so = File.join(libdir, "libc.so.6")
  libm_so = File.join(libdir, "libm.so.6")
when /mingw/, /mswin/
  require "rbconfig"
  libc_so = libm_so = RbConfig::CONFIG["RUBY_SO_NAME"].split(/-/, 2)[0] + ".dll"
when /darwin/
  libc_so = "/usr/lib/libc.dylib"
  libm_so = "/usr/lib/libm.dylib"
when /kfreebsd/
  libc_so = "/lib/libc.so.0.1"
  libm_so = "/lib/libm.so.1"
when /bsd|dragonfly/
  libc_so = "/usr/lib/libc.so"
  libm_so = "/usr/lib/libm.so"
else
  libc_so = ARGV[0] if ARGV[0] && ARGV[0][0] == ?/
  libm_so = ARGV[1] if ARGV[1] && ARGV[1][0] == ?/
  if( !(libc_so && libm_so) )
    $stderr.puts("libc and libm not found: #{$0} <libc> <libm>")
  end
end

libc_so = nil if !libc_so || (libc_so[0] == ?/ && !File.file?(libc_so))
libm_so = nil if !libm_so || (libm_so[0] == ?/ && !File.file?(libm_so))

if !libc_so || !libm_so
  ruby = EnvUtil.rubybin
  ldd = `ldd #{ruby}`
  #puts ldd
  libc_so = $& if !libc_so && %r{/\S*/libc\.so\S*} =~ ldd
  libm_so = $& if !libm_so && %r{/\S*/libm\.so\S*} =~ ldd
  #p [libc_so, libm_so]
end

Fiddle::LIBC_SO = libc_so
Fiddle::LIBM_SO = libm_so

module Fiddle
  class TestCase < MiniTest::Unit::TestCase
    def setup
      @libc = DL.dlopen(LIBC_SO)
      @libm = DL.dlopen(LIBM_SO)
    end
  end
end
