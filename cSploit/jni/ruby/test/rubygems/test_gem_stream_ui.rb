require_relative 'gemutilities'
require 'rubygems/user_interaction'

class TestGemStreamUI < RubyGemTestCase

  module IsTty
    attr_accessor :tty

    def tty?
      @tty = true unless defined? @tty
      return @tty
    end

    alias_method :isatty, :tty?

    def noecho
      yield self
    end
  end

  def setup
    super

    @cfg = Gem.configuration

    @in = StringIO.new
    @out = StringIO.new
    @err = StringIO.new

    @in.extend IsTty

    @sui = Gem::StreamUI.new @in, @out, @err
  end

  def test_ask
    timeout(1) do
      expected_answer = "Arthur, King of the Britons"
      @in.string = "#{expected_answer}\n"
      actual_answer = @sui.ask("What is your name?")
      assert_equal expected_answer, actual_answer
    end
  end

  def test_ask_no_tty
    @in.tty = false

    timeout(0.1) do
      answer = @sui.ask("what is your favorite color?")
      assert_equal nil, answer
    end
  end

  def test_ask_for_password
    skip 'Always uses $stdin on windows' if Gem.win_platform?

    timeout(1) do
      expected_answer = "Arthur, King of the Britons"
      @in.string = "#{expected_answer}\n"
      actual_answer = @sui.ask_for_password("What is your name?")
      assert_equal expected_answer, actual_answer
    end
  end

  def test_ask_for_password_no_tty
    @in.tty = false

    timeout(0.1) do
      answer = @sui.ask_for_password("what is the airspeed velocity of an unladen swallow?")
      assert_equal nil, answer
    end
  end

  def test_ask_yes_no_no_tty_with_default
    @in.tty = false

    timeout(0.1) do
      answer = @sui.ask_yes_no("do coconuts migrate?", false)
      assert_equal false, answer

      answer = @sui.ask_yes_no("do coconuts migrate?", true)
      assert_equal true, answer
    end
  end

  def test_ask_yes_no_no_tty_without_default
    @in.tty = false

    timeout(0.1) do
      assert_raises(Gem::OperationNotSupportedError) do
        @sui.ask_yes_no("do coconuts migrate?")
      end
    end
  end

  def test_choose_from_list
    @in.puts "1"
    @in.rewind

    result = @sui.choose_from_list 'which one?', %w[foo bar]

    assert_equal ['foo', 0], result
    assert_equal "which one?\n 1. foo\n 2. bar\n> ", @out.string
  end

  def test_choose_from_list_EOF
    result = @sui.choose_from_list 'which one?', %w[foo bar]

    assert_equal [nil, nil], result
    assert_equal "which one?\n 1. foo\n 2. bar\n> ", @out.string
  end

  def test_proress_reporter_silent_nil
    @cfg.verbose = nil
    reporter = @sui.progress_reporter 10, 'hi'
    assert_kind_of Gem::StreamUI::SilentProgressReporter, reporter
  end

  def test_proress_reporter_silent_false
    @cfg.verbose = false
    reporter = @sui.progress_reporter 10, 'hi'
    assert_kind_of Gem::StreamUI::SilentProgressReporter, reporter
    assert_equal "", @out.string
  end

  def test_proress_reporter_simple
    @cfg.verbose = true
    reporter = @sui.progress_reporter 10, 'hi'
    assert_kind_of Gem::StreamUI::SimpleProgressReporter, reporter
    assert_equal "hi\n", @out.string
  end

  def test_proress_reporter_verbose
    @cfg.verbose = 0
    reporter = @sui.progress_reporter 10, 'hi'
    assert_kind_of Gem::StreamUI::VerboseProgressReporter, reporter
    assert_equal "hi\n", @out.string
  end

end

