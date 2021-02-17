require_relative 'gem_installer_test_case'
require 'rubygems/uninstaller'

class TestGemUninstaller < GemInstallerTestCase

  def setup
    super

    ui = MockGemUi.new
    util_setup_gem ui

    @user_spec.executables = ["my_exec"]

    # HACK util_make_exec
    user_bin_dir = File.join Gem.user_dir, 'gems', @user_spec.full_name, 'bin'
    FileUtils.mkdir_p user_bin_dir
    exec_path = File.join user_bin_dir, "my_exec"
    File.open exec_path, 'w' do |f|
      f.puts "#!/usr/bin/ruby"
    end

    user_bin_dir = File.join Gem.user_dir, 'bin'
    FileUtils.mkdir_p user_bin_dir
    exec_path = File.join user_bin_dir, "my_exec"
    File.open exec_path, 'w' do |f|
      f.puts "#!/usr/bin/ruby"
    end

    build_rake_in do
      use_ui ui do
        @installer.install
        @user_installer.install
        Gem::Uninstaller.new(@user_spec.name, :executables => false).uninstall
      end
    end
  end

  def test_initialize_expand_path
    uninstaller = Gem::Uninstaller.new nil, :install_dir => '/foo//bar'

    assert_match %r|/foo/bar$|, uninstaller.instance_variable_get(:@gem_home)
  end

  def test_remove_executables_force_keep
    uninstaller = Gem::Uninstaller.new nil, :executables => false

    use_ui @ui do
      uninstaller.remove_executables @spec
    end

    assert_equal true, File.exist?(File.join(@gemhome, 'bin', 'executable'))

    assert_equal "Executables and scripts will remain installed.\n", @ui.output
  end

  def test_remove_executables_force_remove
    uninstaller = Gem::Uninstaller.new nil, :executables => true

    use_ui @ui do
      uninstaller.remove_executables @spec
    end

    assert_equal "Removing executable\n", @ui.output

    assert_equal false, File.exist?(File.join(@gemhome, 'bin', 'executable'))
  end

  def test_remove_executables_user
    uninstaller = Gem::Uninstaller.new nil, :executables => true

    use_ui @ui do
      uninstaller.remove_executables @user_spec
    end

    exec_path = File.join Gem.user_dir, 'bin', 'my_exec'
    assert_equal false, File.exist?(exec_path), 'removed exec from bin dir'

    assert_equal "Removing my_exec\n", @ui.output
  end

  def test_path_ok_eh
    uninstaller = Gem::Uninstaller.new nil

    assert_equal true, uninstaller.path_ok?(@gemhome, @spec)
  end

  def test_path_ok_eh_legacy
    uninstaller = Gem::Uninstaller.new nil

    @spec.loaded_from.gsub! @spec.full_name, '\&-legacy'
    @spec.platform = 'legacy'

    assert_equal true, uninstaller.path_ok?(@gemhome, @spec)
  end

  def test_path_ok_eh_user
    uninstaller = Gem::Uninstaller.new nil

    assert_equal true, uninstaller.path_ok?(Gem.user_dir, @user_spec)
  end

  def test_uninstall
    uninstaller = Gem::Uninstaller.new @spec.name, :executables => true

    gem_dir = File.join @gemhome, 'gems', @spec.full_name

    Gem.pre_uninstall do
      assert File.exist?(gem_dir), 'gem_dir should exist'
    end

    Gem.post_uninstall do
      refute File.exist?(gem_dir), 'gem_dir should not exist'
    end

    uninstaller.uninstall

    refute File.exist?(gem_dir)

    assert_same uninstaller, @pre_uninstall_hook_arg
    assert_same uninstaller, @post_uninstall_hook_arg
  end

  def test_uninstall_user
    uninstaller = Gem::Uninstaller.new @user_spec.name, :executables => true,
                  :user_install => true

    gem_dir = File.join Gem.user_dir, 'gems', @user_spec.full_name

    Gem.pre_uninstall do
      assert File.exist?(gem_dir), 'gem_dir should exist'
    end

    Gem.post_uninstall do
      refute File.exist?(gem_dir), 'gem_dir should not exist'
    end

    uninstaller.uninstall

    refute File.exist?(gem_dir)

    assert_same uninstaller, @pre_uninstall_hook_arg
    assert_same uninstaller, @post_uninstall_hook_arg
  end

end

