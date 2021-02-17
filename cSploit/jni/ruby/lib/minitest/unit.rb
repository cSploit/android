############################################################
# This file is imported from a different project.
# DO NOT make modifications in this repo.
# File a patch instead and assign it to Ryan Davis
############################################################

require 'optparse'

##
# Minimal (mostly drop-in) replacement for test-unit.

module MiniTest

  ##
  # Assertion base class

  class Assertion < Exception; end

  ##
  # Assertion raised when skipping a test

  class Skip < Assertion; end

  file = if RUBY_VERSION =~ /^1\.9/ then  # bt's expanded, but __FILE__ isn't :(
           File.expand_path __FILE__
         elsif  __FILE__ =~ /^[^\.]/ then # assume both relative
           require 'pathname'
           pwd = Pathname.new Dir.pwd
           pn = Pathname.new File.expand_path(__FILE__)
           pn = File.join(".", pn.relative_path_from(pwd)) unless pn.relative?
           pn.to_s
         else                             # assume both are expanded
           __FILE__
         end

  # './lib' in project dir, or '/usr/local/blahblah' if installed
  MINI_DIR = File.dirname(File.dirname(file)) # :nodoc:

  def self.filter_backtrace bt # :nodoc:
    return ["No backtrace"] unless bt

    new_bt = []
    bt.each do |line|
      break if line.rindex(MINI_DIR, 0)
      new_bt << line
    end

    new_bt = bt.reject { |line| line.rindex(MINI_DIR, 0) } if new_bt.empty?
    new_bt = bt.dup if new_bt.empty?
    new_bt
  end

  ##
  # MiniTest Assertions.  All assertion methods accept a +msg+ which is
  # printed if the assertion fails.

  module Assertions

    ##
    # mu_pp gives a human-readable version of +obj+.  By default #inspect is
    # called.  You can override this to use #pretty_print if you want.

    def mu_pp obj
      s = obj.inspect
      s = s.force_encoding(Encoding.default_external) if defined? Encoding
      s
    end

    def _assertions= n # :nodoc:
      @_assertions = n
    end

    def _assertions # :nodoc:
      @_assertions ||= 0
    end

    ##
    # Fails unless +test+ is a true value.

    def assert test, msg = nil
      msg ||= "Failed assertion, no message given."
      self._assertions += 1
      unless test then
        msg = msg.call if Proc === msg
        raise MiniTest::Assertion, msg
      end
      true
    end

    ##
    # Fails unless the block returns a true value.

    def assert_block msg = nil
      msg = message(msg) { "Expected block to return true value" }
      assert yield, msg
    end

    ##
    # Fails unless +obj+ is empty.

    def assert_empty obj, msg = nil
      msg = message(msg) { "Expected #{obj.inspect} to be empty" }
      assert_respond_to obj, :empty?
      assert obj.empty?, msg
    end

    ##
    # Fails unless <tt>exp == act</tt>.
    #
    # For floats use assert_in_delta

    def assert_equal exp, act, msg = nil
      msg = message(msg) { "Expected #{mu_pp(exp)}, not #{mu_pp(act)}" }
      assert(exp == act, msg)
    end

    ##
    # For comparing Floats.  Fails unless +exp+ and +act+ are within +delta+
    # of each other.
    #
    #   assert_in_delta Math::PI, (22.0 / 7.0), 0.01

    def assert_in_delta exp, act, delta = 0.001, msg = nil
      n = (exp - act).abs
      msg = message(msg) { "Expected #{exp} - #{act} (#{n}) to be < #{delta}" }
      assert delta >= n, msg
    end

    ##
    # For comparing Floats.  Fails unless +exp+ and +act+ have a relative
    # error less than +epsilon+.

    def assert_in_epsilon a, b, epsilon = 0.001, msg = nil
      assert_in_delta a, b, [a, b].min * epsilon, msg
    end

    ##
    # Fails unless +collection+ includes +obj+.

    def assert_includes collection, obj, msg = nil
      msg = message(msg) {
        "Expected #{mu_pp(collection)} to include #{mu_pp(obj)}"
      }
      assert_respond_to collection, :include?
      assert collection.include?(obj), msg
    end

    ##
    # Fails unless +obj+ is an instace of +cls+.

    def assert_instance_of cls, obj, msg = nil
      msg = message(msg) {
        "Expected #{mu_pp(obj)} to be an instance of #{cls}, not #{obj.class}"
      }

      assert obj.instance_of?(cls), msg
    end

    ##
    # Fails unless +obj+ is a kind of +cls+.

    def assert_kind_of cls, obj, msg = nil # TODO: merge with instance_of
      msg = message(msg) {
        "Expected #{mu_pp(obj)} to be a kind of #{cls}, not #{obj.class}" }

      assert obj.kind_of?(cls), msg
    end

    ##
    # Fails unless +exp+ is <tt>=~</tt> +act+.

    def assert_match exp, act, msg = nil
      msg = message(msg) { "Expected #{mu_pp(exp)} to match #{mu_pp(act)}" }
      assert_respond_to act, :"=~"
      exp = /#{Regexp.escape exp}/ if String === exp && String === act
      assert exp =~ act, msg
    end

    ##
    # Fails unless +obj+ is nil

    def assert_nil obj, msg = nil
      msg = message(msg) { "Expected #{mu_pp(obj)} to be nil" }
      assert obj.nil?, msg
    end

    ##
    # For testing equality operators and so-forth.
    #
    #   assert_operator 5, :<=, 4

    def assert_operator o1, op, o2, msg = nil
      msg = message(msg) { "Expected #{mu_pp(o1)} to be #{op} #{mu_pp(o2)}" }
      assert o1.__send__(op, o2), msg
    end

    ##
    # Fails unless the block raises one of +exp+

    def assert_raises *exp
      msg = String === exp.last ? exp.pop : nil
      msg = msg.to_s + "\n" if msg
      should_raise = false
      begin
        yield
        should_raise = true
      rescue Exception => e
        details = "#{msg}#{mu_pp(exp)} exception expected, not"
        assert(exp.any? { |ex|
                 ex.instance_of?(Module) ? e.kind_of?(ex) : ex == e.class
               }, exception_details(e, details))

        return e
      end

      exp = exp.first if exp.size == 1
      flunk "#{msg}#{mu_pp(exp)} expected but nothing was raised." if
        should_raise
    end

    ##
    # Fails unless +obj+ responds to +meth+.

    def assert_respond_to obj, meth, msg = nil
      msg = message(msg) {
          "Expected #{mu_pp(obj)} (#{obj.class}) to respond to ##{meth}"
        }
      assert obj.respond_to?(meth), msg
    end

    ##
    # Fails unless +exp+ and +act+ are #equal?

    def assert_same exp, act, msg = nil
      msg = message(msg) {
        data = [mu_pp(act), act.object_id, mu_pp(exp), exp.object_id]
        "Expected %s (oid=%d) to be the same as %s (oid=%d)" % data
      }
      assert exp.equal?(act), msg
    end

    ##
    # +send_ary+ is a receiver, message and arguments.
    #
    # Fails unless the call returns a true value

    def assert_send send_ary, m = nil
      recv, msg, *args = send_ary
      m = message(m) {
        "Expected #{mu_pp(recv)}.#{msg}(*#{mu_pp(args)}) to return true" }
      assert recv.__send__(msg, *args), m
    end

    ##
    # Fails unless the block throws +sym+

    def assert_throws sym, msg = nil
      default = "Expected #{mu_pp(sym)} to have been thrown"
      caught = true
      catch(sym) do
        begin
          yield
        rescue ArgumentError => e     # 1.9 exception
          default += ", not #{e.message.split(/ /).last}"
        rescue NameError => e         # 1.8 exception
          default += ", not #{e.name.inspect}"
        end
        caught = false
      end

      assert caught, message(msg) { default }
    end

    ##
    # Captures $stdout and $stderr into strings:
    #
    #   out, err = capture_io do
    #     warn "You did a bad thing"
    #   end
    #
    #   assert_match %r%bad%, err

    def capture_io
      require 'stringio'

      orig_stdout, orig_stderr         = $stdout, $stderr
      captured_stdout, captured_stderr = StringIO.new, StringIO.new
      $stdout, $stderr                 = captured_stdout, captured_stderr

      yield

      return captured_stdout.string, captured_stderr.string
    ensure
      $stdout = orig_stdout
      $stderr = orig_stderr
    end

    ##
    # Returns details for exception +e+

    def exception_details e, msg
      "#{msg}\nClass: <#{e.class}>\nMessage: <#{e.message.inspect}>\n---Backtrace---\n#{MiniTest::filter_backtrace(e.backtrace).join("\n")}\n---------------"
    end

    ##
    # Fails with +msg+

    def flunk msg = nil
      msg ||= "Epic Fail!"
      assert false, msg
    end

    ##
    # Returns a proc that will output +msg+ along with the default message.

    def message msg = nil, &default
      proc {
        if msg then
          msg = msg.to_s unless String === msg
          msg += '.' unless msg.empty?
          msg += "\n#{default.call}."
          msg.strip
        else
          "#{default.call}."
        end
      }
    end

    ##
    # used for counting assertions

    def pass msg = nil
      assert true
    end

    ##
    # Fails if +test+ is a true value

    def refute test, msg = nil
      msg ||= "Failed refutation, no message given"
      not assert(! test, msg)
    end

    ##
    # Fails if +obj+ is empty.

    def refute_empty obj, msg = nil
      msg = message(msg) { "Expected #{obj.inspect} to not be empty" }
      assert_respond_to obj, :empty?
      refute obj.empty?, msg
    end

    ##
    # Fails if <tt>exp == act</tt>.
    #
    # For floats use refute_in_delta.

    def refute_equal exp, act, msg = nil
      msg = message(msg) {
        "Expected #{mu_pp(act)} to not be equal to #{mu_pp(exp)}"
      }
      refute exp == act, msg
    end

    ##
    # For comparing Floats.  Fails if +exp+ is within +delta+ of +act+
    #
    #   refute_in_delta Math::PI, (22.0 / 7.0)

    def refute_in_delta exp, act, delta = 0.001, msg = nil
      n = (exp - act).abs
      msg = message(msg) {
        "Expected #{exp} - #{act} (#{n}) to not be < #{delta}"
      }
      refute delta > n, msg
    end

    ##
    # For comparing Floats.  Fails if +exp+ and +act+ have a relative error
    # less than +epsilon+.

    def refute_in_epsilon a, b, epsilon = 0.001, msg = nil
      refute_in_delta a, b, a * epsilon, msg
    end

    ##
    # Fails if +collection+ includes +obj+

    def refute_includes collection, obj, msg = nil
      msg = message(msg) {
        "Expected #{mu_pp(collection)} to not include #{mu_pp(obj)}"
      }
      assert_respond_to collection, :include?
      refute collection.include?(obj), msg
    end

    ##
    # Fails if +obj+ is an instance of +cls+

    def refute_instance_of cls, obj, msg = nil
      msg = message(msg) {
        "Expected #{mu_pp(obj)} to not be an instance of #{cls}"
      }
      refute obj.instance_of?(cls), msg
    end

    ##
    # Fails if +obj+ is a kind of +cls+

    def refute_kind_of cls, obj, msg = nil # TODO: merge with instance_of
      msg = message(msg) { "Expected #{mu_pp(obj)} to not be a kind of #{cls}" }
      refute obj.kind_of?(cls), msg
    end

    ##
    # Fails if +exp+ <tt>=~</tt> +act+

    def refute_match exp, act, msg = nil
      msg = message(msg) { "Expected #{mu_pp(exp)} to not match #{mu_pp(act)}" }
      assert_respond_to act, :"=~"
      exp = (/#{Regexp.escape exp}/) if String === exp and String === act
      refute exp =~ act, msg
    end

    ##
    # Fails if +obj+ is nil.

    def refute_nil obj, msg = nil
      msg = message(msg) { "Expected #{mu_pp(obj)} to not be nil" }
      refute obj.nil?, msg
    end

    ##
    # Fails if +o1+ is not +op+ +o2+ nil. eg:
    #
    #   refute_operator 1, :>, 2 #=> pass
    #   refute_operator 1, :<, 2 #=> fail

    def refute_operator o1, op, o2, msg = nil
      msg = message(msg) {
        "Expected #{mu_pp(o1)} to not be #{op} #{mu_pp(o2)}"
      }
      refute o1.__send__(op, o2), msg
    end

    ##
    # Fails if +obj+ responds to the message +meth+.

    def refute_respond_to obj, meth, msg = nil
      msg = message(msg) { "Expected #{mu_pp(obj)} to not respond to #{meth}" }

      refute obj.respond_to?(meth), msg
    end

    ##
    # Fails if +exp+ is the same (by object identity) as +act+.

    def refute_same exp, act, msg = nil
      msg = message(msg) {
        data = [mu_pp(act), act.object_id, mu_pp(exp), exp.object_id]
        "Expected %s (oid=%d) to not be the same as %s (oid=%d)" % data
      }
      refute exp.equal?(act), msg
    end

    ##
    # Skips the current test. Gets listed at the end of the run but
    # doesn't cause a failure exit code.

    def skip msg = nil, bt = caller
      msg ||= "Skipped, no message given"
      raise MiniTest::Skip, msg, bt
    end
  end

  class Unit
    VERSION = "1.6.0" # :nodoc:

    attr_accessor :report, :failures, :errors, :skips # :nodoc:
    attr_accessor :test_count, :assertion_count       # :nodoc:
    attr_accessor :start_time                         # :nodoc:

    @@installed_at_exit ||= false
    @@out = $stdout

    ##
    # Registers MiniTest::Unit to run tests at process exit

    def self.autorun
      at_exit {
        next if $! # don't run if there was an exception
        exit_code = MiniTest::Unit.new.run(ARGV)
        exit false if exit_code && exit_code != 0
      } unless @@installed_at_exit
      @@installed_at_exit = true
    end

    ##
    # Sets MiniTest::Unit to write output to +stream+.  $stdout is the default
    # output

    def self.output= stream
      @@out = stream
    end

    def location e # :nodoc:
      last_before_assertion = ""
      e.backtrace.reverse_each do |s|
        break if s =~ /in .(assert|refute|flunk|pass|fail|raise|must|wont)/
        last_before_assertion = s
      end
      last_before_assertion.sub(/:in .*$/, '')
    end

    ##
    # Writes status for failed test +meth+ in +klass+ which finished with
    # exception +e+

    def puke klass, meth, e
      e = case e
          when MiniTest::Skip then
            @skips += 1
            "Skipped:\n#{meth}(#{klass}) [#{location e}]:\n#{e.message}\n"
          when MiniTest::Assertion then
            @failures += 1
            "Failure:\n#{meth}(#{klass}) [#{location e}]:\n#{e.message}\n"
          else
            @errors += 1
            bt = MiniTest::filter_backtrace(e.backtrace).join("\n    ")
            "Error:\n#{meth}(#{klass}):\n#{e.class}: #{e.message}\n    #{bt}\n"
          end
      @report << e
      e[0, 1]
    end

    def initialize # :nodoc:
      @report = []
      @errors = @failures = @skips = 0
      @verbose = false
    end

    def process_args args = []
      options = {}

      OptionParser.new do |opts|
        opts.banner  = 'minitest options:'
        opts.version = MiniTest::Unit::VERSION

        opts.on '-h', '--help', 'Display this help.' do
          puts opts
          exit
        end

        opts.on '-s', '--seed SEED', Integer, "Sets random seed" do |m|
          options[:seed] = m.to_i
        end

        opts.on '-v', '--verbose', "Verbose. Show progress processing files." do
          options[:verbose] = true
        end

        opts.on '-n', '--name PATTERN', "Filter test names on pattern." do |a|
          options[:filter] = a
        end

        opts.parse args
      end

      options
    end

    ##
    # Top level driver, controls all output and filtering.

    def run args = []
      options = process_args args

      @verbose = options[:verbose]

      filter = options[:filter] || '/./'
      filter = Regexp.new $1 if filter and filter =~ /\/(.*)\//

      seed = options[:seed]
      unless seed then
        srand
        seed = srand % 0xFFFF
      end

      srand seed

      @@out.puts "Loaded suite #{$0.sub(/\.rb$/, '')}\nStarted"

      start = Time.now
      run_test_suites filter

      @@out.puts
      @@out.puts "Finished in #{'%.6f' % (Time.now - start)} seconds."

      @report.each_with_index do |msg, i|
        @@out.puts "\n%3d) %s" % [i + 1, msg]
      end

      @@out.puts

      status

      @@out.puts

      help = ["--seed", seed]
      help.push "--verbose" if @verbose
      help.push("--name", options[:filter].inspect) if options[:filter]

      @@out.puts "Test run options: #{help.join(" ")}"

      return failures + errors if @test_count > 0 # or return nil...
    rescue Interrupt
      abort 'Interrupted'
    end

    ##
    # Writes status to +io+

    def status io = @@out
      format = "%d tests, %d assertions, %d failures, %d errors, %d skips"
      io.puts format % [test_count, assertion_count, failures, errors, skips]
    end

    ##
    # Runs test suites matching +filter+

    def run_test_suites filter = /./
      @test_count, @assertion_count = 0, 0
      old_sync, @@out.sync = @@out.sync, true if @@out.respond_to? :sync=
      TestCase.test_suites.each do |suite|
        suite.test_methods.grep(filter).each do |test|
          inst = suite.new test
          inst._assertions = 0
          @@out.print "#{suite}##{test}: " if @verbose

          @start_time = Time.now
          result = inst.run(self)

          @@out.print "%.2f s: " % (Time.now - @start_time) if @verbose
          @@out.print result
          @@out.puts if @verbose
          @test_count += 1
          @assertion_count += inst._assertions
        end
      end
      @@out.sync = old_sync if @@out.respond_to? :sync=
      [@test_count, @assertion_count]
    end

    ##
    # Subclass TestCase to create your own tests.  Typically you'll want a
    # TestCase subclass per implementation class.

    class TestCase
      attr_reader :__name__ # :nodoc:

      PASSTHROUGH_EXCEPTIONS = [NoMemoryError, SignalException,
                                Interrupt, SystemExit] # :nodoc:

      SUPPORTS_INFO_SIGNAL = Signal.list['INFO'] # :nodoc:

      ##
      # Runs the tests reporting the status to +runner+

      def run runner
        trap 'INFO' do
          warn '%s#%s %.2fs' % [self.class, self.__name__,
            (Time.now - runner.start_time)]
          runner.status $stderr
        end if SUPPORTS_INFO_SIGNAL

        result = '.'
        begin
          @passed = nil
          self.setup
          self.__send__ self.__name__
          @passed = true
        rescue *PASSTHROUGH_EXCEPTIONS
          raise
        rescue Exception => e
          @passed = false
          result = runner.puke(self.class, self.__name__, e)
        ensure
          begin
            self.teardown
          rescue *PASSTHROUGH_EXCEPTIONS
            raise
          rescue Exception => e
            result = runner.puke(self.class, self.__name__, e)
          end
          trap 'INFO', 'DEFAULT' if SUPPORTS_INFO_SIGNAL
        end
        result
      end

      def initialize name # :nodoc:
        @__name__ = name
        @passed = nil
      end

      def self.reset # :nodoc:
        @@test_suites = {}
      end

      reset

      def self.inherited klass # :nodoc:
        @@test_suites[klass] = true
      end

      ##
      # Defines test order and is subclassable. Defaults to :random
      # but can be overridden to return :alpha if your tests are order
      # dependent (read: weak).

      def self.test_order
        :random
      end

      def self.test_suites # :nodoc:
        @@test_suites.keys.sort_by { |ts| ts.name }
      end

      def self.test_methods # :nodoc:
        methods = public_instance_methods(true).grep(/^test/).map { |m| m.to_s }

        case self.test_order
        when :random then
          max = methods.size
          methods.sort.sort_by { rand(max) }
        when :alpha, :sorted then
          methods.sort
        else
          raise "Unknown test_order: #{self.test_order.inspect}"
        end
      end

      ##
      # Returns true if the test passed.

      def passed?
        @passed
      end

      ##
      # Runs before every test. Use this to refactor test initialization.

      def setup; end

      ##
      # Runs after every test. Use this to refactor test cleanup.

      def teardown; end

      include MiniTest::Assertions
    end # class TestCase
  end # class Unit
end # module MiniTest

if $DEBUG then
  module Test                # :nodoc:
    module Unit              # :nodoc:
      class TestCase         # :nodoc:
        def self.inherited x # :nodoc:
          # this helps me ferret out porting issues
          raise "Using minitest and test/unit in the same process: #{x}"
        end
      end
    end
  end
end

