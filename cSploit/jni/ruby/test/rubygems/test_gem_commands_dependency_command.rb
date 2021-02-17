require_relative 'gemutilities'
require 'rubygems/commands/dependency_command'

class TestGemCommandsDependencyCommand < RubyGemTestCase

  def setup
    super

    @cmd = Gem::Commands::DependencyCommand.new
    @cmd.options[:domain] = :local

    util_setup_fake_fetcher true
  end

  def test_execute
    quick_gem 'foo' do |gem|
      gem.add_dependency 'bar', '> 1'
      gem.add_dependency 'baz', '> 1'
    end

    Gem.source_index = nil

    @cmd.options[:args] = %w[foo]

    use_ui @ui do
      @cmd.execute
    end

    assert_equal "Gem foo-2\n  bar (> 1, runtime)\n  baz (> 1, runtime)\n\n",
                 @ui.output
    assert_equal '', @ui.error
  end

  def test_execute_no_args
    Gem.source_index = nil

    @cmd.options[:args] = []

    use_ui @ui do
      @cmd.execute
    end

    expected = <<-EOF
Gem a-1

Gem a-2.a

Gem a-2

Gem a-3.a

Gem a_evil-9

Gem b-2

Gem c-1.2

Gem pl-1-x86-linux

    EOF

    assert_equal expected, @ui.output
    assert_equal '', @ui.error
  end

  def test_execute_no_match
    @cmd.options[:args] = %w[foo]

    assert_raises MockGemUi::TermError do
      use_ui @ui do
        @cmd.execute
      end
    end

    assert_equal "No gems found matching foo (>= 0)\n", @ui.output
    assert_equal '', @ui.error
  end

  def test_execute_pipe_format
    quick_gem 'foo' do |gem|
      gem.add_dependency 'bar', '> 1'
    end

    @cmd.options[:args] = %w[foo]
    @cmd.options[:pipe_format] = true

    use_ui @ui do
      @cmd.execute
    end

    assert_equal "bar --version '> 1'\n", @ui.output
    assert_equal '', @ui.error
  end

  def test_execute_regexp
    Gem.source_index = nil

    @cmd.options[:args] = %w[/[ab]/]

    use_ui @ui do
      @cmd.execute
    end

    expected = <<-EOF
Gem a-1

Gem a-2.a

Gem a-2

Gem a-3.a

Gem a_evil-9

Gem b-2

    EOF

    assert_equal expected, @ui.output
    assert_equal '', @ui.error
  end

  def test_execute_reverse
    quick_gem 'foo' do |gem|
      gem.add_dependency 'bar', '> 1'
    end

    quick_gem 'baz' do |gem|
      gem.add_dependency 'foo'
    end

    Gem.source_index = nil

    @cmd.options[:args] = %w[foo]
    @cmd.options[:reverse_dependencies] = true

    use_ui @ui do
      @cmd.execute
    end

    expected = <<-EOF
Gem foo-2
  bar (> 1, runtime)
  Used by
    baz-2 (foo (>= 0, runtime))

    EOF

    assert_equal expected, @ui.output
    assert_equal '', @ui.error
  end

  def test_execute_reverse_remote
    @cmd.options[:args] = %w[foo]
    @cmd.options[:reverse_dependencies] = true
    @cmd.options[:domain] = :remote

    assert_raises MockGemUi::TermError do
      use_ui @ui do
        @cmd.execute
      end
    end

    expected = <<-EOF
ERROR:  Only reverse dependencies for local gems are supported.
    EOF

    assert_equal '', @ui.output
    assert_equal expected, @ui.error
  end

  def test_execute_remote
    foo = quick_gem 'foo' do |gem|
      gem.add_dependency 'bar', '> 1'
    end

    @fetcher = Gem::FakeFetcher.new
    Gem::RemoteFetcher.fetcher = @fetcher

    util_setup_spec_fetcher foo

    FileUtils.rm File.join(@gemhome, 'specifications', foo.spec_name)

    @cmd.options[:args] = %w[foo]
    @cmd.options[:domain] = :remote

    use_ui @ui do
      @cmd.execute
    end

    assert_equal "Gem foo-2\n  bar (> 1, runtime)\n\n", @ui.output
    assert_equal '', @ui.error
  end

  def test_execute_prerelease
    @fetcher = Gem::FakeFetcher.new
    Gem::RemoteFetcher.fetcher = @fetcher

    util_setup_spec_fetcher @a2_pre

    FileUtils.rm File.join(@gemhome, 'specifications', @a2_pre.spec_name)

    @cmd.options[:args] = %w[a]
    @cmd.options[:domain] = :remote
    @cmd.options[:prerelease] = true

    use_ui @ui do
      @cmd.execute
    end

    assert_equal "Gem a-2.a\n\n", @ui.output
    assert_equal '', @ui.error
  end

end

