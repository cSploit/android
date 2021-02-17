#
# test Win32OLE avoids cfp consistency error when the exception raised
# in WIN32OLE_EVENT handler block. [ruby-dev:35450]
#

begin
  require 'win32ole'
rescue LoadError
end
require 'rbconfig'
if defined?(WIN32OLE)
  require 'mkmf'
  require 'test/unit'
  class TestErrInCallBack < Test::Unit::TestCase
    def setup
      @ruby = nil
      if File.exist?("./" + CONFIG["RUBY_INSTALL_NAME"] + CONFIG["EXEEXT"])
        sep = File::ALT_SEPARATOR || "/"
        @ruby = "." + sep + CONFIG["RUBY_INSTALL_NAME"]
        @iopt = $:.map {|e|
          " -I " + e
        }.join("")
        @script = File.dirname(__FILE__) + "/err_in_callback.rb"
      end
    end

    def test_err_in_callback
      if @ruby
        cmd = "#{@ruby} -v #{@iopt} #{@script} > test_err_in_callback.log 2>&1"
        system(cmd)
        str = ""
        open("test_err_in_callback.log") {|ifs|
          str = ifs.read
        }
        assert_match(/NameError/, str)
      end
    end

    def teardown
      File.unlink("test_err_in_callback.log")
    end

  end
end
