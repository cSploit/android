# ====================================================================
#    Licensed to the Apache Software Foundation (ASF) under one
#    or more contributor license agreements.  See the NOTICE file
#    distributed with this work for additional information
#    regarding copyright ownership.  The ASF licenses this file
#    to you under the Apache License, Version 2.0 (the
#    "License"); you may not use this file except in compliance
#    with the License.  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing,
#    software distributed under the License is distributed on an
#    "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
#    KIND, either express or implied.  See the License for the
#    specific language governing permissions and limitations
#    under the License.
# ====================================================================

require "fileutils"
require "pathname"
require "svn/util"
require "tmpdir"

require "my-assertions"

class Time
  unless instance_methods.include?("to_int")
    alias to_int to_i
  end
end

require 'greek_tree'

module SvnTestUtil
  def setup_default_variables
    @author = ENV["USER"] || "sample-user"
    @password = "sample-password"
    @realm = "sample realm"
    @repos_path = "repos"
    @full_repos_path = File.expand_path(@repos_path)
    @repos_uri = "file://#{@full_repos_path.sub(/^\/?/, '/')}"
    @svnserve_host = "127.0.0.1"
    @svnserve_ports = (64152..64282).collect{|x| x.to_s}
    @wc_base_dir = File.join(Dir.tmpdir, "wc-tmp")
    @wc_path = File.join(@wc_base_dir, "wc")
    @full_wc_path = File.expand_path(@wc_path)
    @tmp_path = "tmp"
    @config_path = "config"
    @greek = Greek.new(@tmp_path, @wc_path, @repos_uri)
  end

  def setup_basic(need_svnserve=false)
    @need_svnserve = need_svnserve
    setup_default_variables
    setup_tmp
    setup_repository
    add_hooks
    setup_svnserve if @need_svnserve
    setup_config
    setup_wc
    add_authentication
    GC.stress = true if GC.respond_to?(:stress=) and $DEBUG
  end

  def teardown_basic
    GC.stress = false if GC.respond_to?(:stress=)
    teardown_svnserve if @need_svnserve
    teardown_repository
    teardown_wc
    teardown_config
    teardown_tmp
    gc
  end

  def gc
    if $DEBUG
      before_pools = Svn::Core::Pool.number_of_pools
      puts
      puts "before pools: #{before_pools}"
    end
    gc_enable do
      GC.start
    end
    if $DEBUG
      after_pools = Svn::Core::Pool.number_of_pools
      puts "after pools: #{after_pools}"
      STDOUT.flush
    end
  end

  def change_gc_status(prev_disabled)
    begin
      yield
    ensure
      if prev_disabled
        GC.disable
      else
        GC.enable
      end
    end
  end

  def gc_disable(&block)
    change_gc_status(GC.disable, &block)
  end

  def gc_enable(&block)
    change_gc_status(GC.enable, &block)
  end

  def setup_tmp(path=@tmp_path)
    remove_recursively_with_retry(path)
    FileUtils.mkdir_p(path)
  end

  def teardown_tmp(path=@tmp_path)
    remove_recursively_with_retry(path)
  end

  def setup_repository(path=@repos_path, config={}, fs_config={})
    require "svn/repos"
    remove_recursively_with_retry(path)
    FileUtils.mkdir_p(File.dirname(path))
    @repos = Svn::Repos.create(path, config, fs_config)
    @fs = @repos.fs
  end

  def teardown_repository(path=@repos_path)
    @fs.close unless @fs.nil?
    @repos.close unless @repos.nil?
    remove_recursively_with_retry(path)
    @repos = nil
    @fs = nil
  end

  def setup_wc
    teardown_wc
    make_context("") { |ctx| ctx.checkout(@repos_uri, @wc_path) }
  end

  def teardown_wc
    remove_recursively_with_retry(@wc_base_dir)
  end

  def setup_config
    teardown_config
    Svn::Core::Config.ensure(@config_path)
  end

  def teardown_config
    remove_recursively_with_retry(@config_path)
  end

  def add_authentication
    passwd_file = "passwd"
    File.open(@repos.svnserve_conf, "w") do |conf|
      conf.print <<-CONF
[general]
anon-access = none
auth-access = write
password-db = #{passwd_file}
realm = #{@realm}
      CONF
    end
    File.open(File.join(@repos.conf_dir, passwd_file), "w") do |f|
      f.print <<-PASSWD
[users]
#{@author} = #{@password}
      PASSWD
    end
  end

  def add_hooks
    add_pre_revprop_change_hook
  end

  def youngest_rev
    @fs.youngest_rev
  end

  def root(rev=nil)
    @fs.root(rev)
  end

  def prop(name, rev=nil)
    @fs.prop(name, rev)
  end

  def make_context(log)
    ctx = Svn::Client::Context.new
    ctx.set_log_msg_func do |items|
      [true, log]
    end
    ctx.add_username_prompt_provider(0) do |cred, realm, username, may_save|
      cred.username = @author
      cred.may_save = false
    end
    setup_auth_baton(ctx.auth_baton)
    return ctx unless block_given?
    begin
      yield ctx
    ensure
      ctx.destroy
    end
  end

  def setup_auth_baton(auth_baton)
    auth_baton[Svn::Core::AUTH_PARAM_CONFIG_DIR] = @config_path
    auth_baton[Svn::Core::AUTH_PARAM_DEFAULT_USERNAME] = @author
  end

  def normalize_line_break(str)
    if Svn::Util.windows?
      str.gsub(/\n/, "\r\n")
    else
      str
    end
  end

  def setup_greek_tree
    make_context("setup greek tree") { |ctx| @greek.setup(ctx) }
  end

  def remove_recursively_with_retry(path)
    retries = 0
    while (retries+=1) < 100 && File.exist?(path)
      begin
        FileUtils.rm_r(path, :secure=>true)
      rescue
        sleep 0.1
      end
    end
    assert(!File.exist?(path), "#{Dir.glob(path+'/**/*').join("\n")} should not exist after #{retries} attempts to delete")
  end

  module Svnserve
    def setup_svnserve
      @svnserve_port = nil
      @repos_svnserve_uri = nil

      # Look through the list of potential ports until we're able to
      # successfully start svnserve on a free one.
      @svnserve_ports.each do |port|
        @svnserve_pid = fork {
          STDERR.close
          exec("svnserve",
               "--listen-host", @svnserve_host,
               "--listen-port", port,
               "-d", "--foreground")
        }
        pid, status = Process.waitpid2(@svnserve_pid, Process::WNOHANG)
        if status and status.exited?
          if $DEBUG
            STDERR.puts "port #{port} couldn't be used for svnserve"
          end
        else
          # svnserve started successfully.  Note port number and cease
          # startup attempts.
          @svnserve_port = port
          @repos_svnserve_uri =
            "svn://#{@svnserve_host}:#{@svnserve_port}#{@full_repos_path}"
          # Avoid a race by waiting a short time for svnserve to start up.
          # Without this, tests can fail with "Connection refused" errors.
          sleep 1
          break
        end
      end
      if @svnserve_port.nil?
        msg = "Can't run svnserve because available port "
        msg << "isn't exist in [#{@svnserve_ports.join(', ')}]"
        raise msg
      end
    end

    def teardown_svnserve
      if @svnserve_pid
        Process.kill(:TERM, @svnserve_pid)
        begin
          Process.waitpid(@svnserve_pid)
        rescue Errno::ECHILD
        end
      end
    end

    def add_pre_revprop_change_hook
      File.open(@repos.pre_revprop_change_hook, "w") do |hook|
        hook.print <<-HOOK
#!/bin/sh
REPOS="$1"
REV="$2"
USER="$3"
PROPNAME="$4"

if [ "$PROPNAME" = "#{Svn::Core::PROP_REVISION_LOG}" -a \
     "$USER" = "#{@author}" ]; then
  exit 0
fi

exit 1
        HOOK
      end
      FileUtils.chmod(0755, @repos.pre_revprop_change_hook)
    end
  end

  module SetupEnvironment
    def setup_test_environment(top_dir, base_dir, ext_dir)
      svnserve_dir = File.join(top_dir, 'subversion', 'svnserve')
      ENV["PATH"] = "#{svnserve_dir}:#{ENV['PATH']}"
      FileUtils.ln_sf(File.join(base_dir, ".libs"), ext_dir)
    end
  end

  if Svn::Util.windows?
    require 'windows_util'
    include Windows::Svnserve
    extend Windows::SetupEnvironment
  else
    include Svnserve
    extend SetupEnvironment
  end
end
