#!/usr/bin/env ruby
# $RoughId: test.rb,v 1.9 2002/02/25 08:20:14 knu Exp $
# $Id$

# Please only run this test on machines reasonable for testing.
# If in doubt, ask your admin.

require 'test/unit'
require 'syslog'

class TestSyslog < Test::Unit::TestCase
  def test_new
    assert_raises(NoMethodError) {
      Syslog.new
    }
  end

  def test_instance
    sl1 = Syslog.instance
    sl2 = Syslog.open
    sl3 = Syslog.instance

    assert_equal(Syslog, sl1)
    assert_equal(Syslog, sl2)
    assert_equal(Syslog, sl3)
  ensure
    Syslog.close if Syslog.opened?
  end

  def test_open
    # default parameters
    Syslog.open

    assert_equal($0, Syslog.ident)
    assert_equal(Syslog::LOG_PID | Syslog::LOG_CONS, Syslog.options)
    assert_equal(Syslog::LOG_USER, Syslog.facility)

    # open without close
    assert_raises(RuntimeError) {
      Syslog.open
    }

    Syslog.close

    # given parameters
    Syslog.open("foo", Syslog::LOG_NDELAY | Syslog::LOG_PERROR, Syslog::LOG_DAEMON)

    assert_equal('foo', Syslog.ident)
    assert_equal(Syslog::LOG_NDELAY | Syslog::LOG_PERROR, Syslog.options)
    assert_equal(Syslog::LOG_DAEMON, Syslog.facility)

    Syslog.close

    # default parameters again (after close)
    Syslog.open
    Syslog.close

    assert_equal(nil, Syslog.ident)
    assert_equal(nil, Syslog.options)
    assert_equal(nil, Syslog.facility)

    # block
    param = nil
    Syslog.open { |syslog|
      param = syslog
    }
    assert_equal(Syslog, param)
  ensure
    Syslog.close if Syslog.opened?
  end

  def test_opened?
    assert_equal(false, Syslog.opened?)

    Syslog.open
    assert_equal(true, Syslog.opened?)

    Syslog.close
    assert_equal(false, Syslog.opened?)

    Syslog.open {
      assert_equal(true, Syslog.opened?)
    }

    assert_equal(false, Syslog.opened?)
  end

  def test_close
    assert_raises(RuntimeError) {
      Syslog.close
    }
  end

  def test_mask
    assert_equal(nil, Syslog.mask)

    Syslog.open

    orig = Syslog.mask

    Syslog.mask = Syslog.LOG_UPTO(Syslog::LOG_ERR)
    assert_equal(Syslog.LOG_UPTO(Syslog::LOG_ERR), Syslog.mask)

    Syslog.mask = Syslog.LOG_MASK(Syslog::LOG_CRIT)
    assert_equal(Syslog.LOG_MASK(Syslog::LOG_CRIT), Syslog.mask)

    Syslog.mask = orig
  ensure
    Syslog.close if Syslog.opened?
  end

  def test_log
    stderr = IO::pipe

    pid = fork {
      stderr[0].close
      STDERR.reopen(stderr[1])
      stderr[1].close

      options = Syslog::LOG_PERROR | Syslog::LOG_NDELAY

      Syslog.open("syslog_test", options) { |sl|
	sl.log(Syslog::LOG_NOTICE, "test1 - hello, %s!", "world")
	sl.notice("test1 - hello, %s!", "world")
      }

      Syslog.open("syslog_test", options | Syslog::LOG_PID) { |sl|
	sl.log(Syslog::LOG_CRIT, "test2 - pid")
	sl.crit("test2 - pid")
      }
      exit!
    }

    stderr[1].close
    Process.waitpid(pid)

    # LOG_PERROR is not yet implemented on Cygwin.
    return if RUBY_PLATFORM =~ /cygwin/

    2.times {
      assert_equal("syslog_test: test1 - hello, world!\n", stderr[0].gets)
    }

    2.times {
      assert_equal(format("syslog_test[%d]: test2 - pid\n", pid), stderr[0].gets)
    }
  end

  def test_inspect
    Syslog.open { |sl|
      assert_equal(format('<#%s: opened=true, ident="%s", options=%d, facility=%d, mask=%d>',
			  Syslog,
			  sl.ident,
			  sl.options,
			  sl.facility,
			  sl.mask),
		   sl.inspect)
    }

    assert_equal(format('<#%s: opened=false>', Syslog), Syslog.inspect)
  end
end
