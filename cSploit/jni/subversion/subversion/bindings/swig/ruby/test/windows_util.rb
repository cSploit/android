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

require 'etc'
require 'fileutils'

module SvnTestUtil
  module Windows
    module Svnserve
      def service_name
        "test-svn-server--port-#{@svnserve_port}"
      end

      class << self
        def escape_value(value)
          escaped_value = value.gsub(/"/, '\\"') # "
          "\"#{escaped_value}\""
        end
      end

      def service_control(command, args={})
        args = args.collect do |key, value|
          "#{key}= #{Svnserve.escape_value(value)}"
        end.join(" ")
        result = `sc #{command} #{service_name} #{args}`
        if result.match(/FAILED/)
          raise "Failed to #{command} #{service_name}: #{args}"
        end
        /^\s*STATE\s*:\s\d+\s*(.*?)\s*$/ =~ result
        $1
      end

      def grant_everyone_full_access(dir)
        dir = dir.tr(File::SEPARATOR, File::ALT_SEPARATOR)
        `cacls #{Svnserve.escape_value(dir)} /T /E /P Everyone:F`
      end

      def service_exists?
        begin
          service_control("query")
          true
        rescue
          false
        end
      end

      def service_stopped?
        "STOPPED" == service_control("query") rescue true
      end

      def setup_svnserve
        @svnserve_port = @svnserve_ports.last
        @repos_svnserve_uri = "svn://#{@svnserve_host}:#{@svnserve_port}"
        grant_everyone_full_access(@full_repos_path)

        @@service_created ||= begin
          @@service_created = true
          service_control('stop') unless service_stopped?
          service_control('delete') if service_exists?

          svnserve_dir = File.expand_path("svnserve")
          FileUtils.mkdir_p(svnserve_dir)
          at_exit do
            service_control('stop') unless service_stopped?
            service_control('delete') if service_exists?
            FileUtils.rm_rf(svnserve_dir)
          end
          trap("INT") do
            service_control('stop') unless service_stopped?
            service_control('delete') if service_exists?
            FileUtils.rm_rf(svnserve_dir)
          end

          config = SetupEnvironment.gen_make_opts
          apr_version_include = Pathname.new(config["--with-apr"])  +
              'include' + 'apr_version.h'
          %r'^\s*#define\s+APR_MAJOR_VERSION\s+(\d+)' =~ apr_version_include.read
          apr_major_version = $1 == '0' ? '' : "-#{$1}"

          targets = %W(svnserve.exe libsvn_subr-1.dll libsvn_repos-1.dll
                       libsvn_fs-1.dll libsvn_delta-1.dll
                       libaprutil#{apr_major_version}.dll
                       libapr#{apr_major_version}.dll
                       libapriconv#{apr_major_version}.dll
                       libdb44.dll libdb44d.dll)
          ENV["PATH"].split(";").each do |path|
            found_targets = []
            targets.each do |target|
              target_path = "#{path}\\#{target}"
              if File.exists?(target_path)
                found_targets << target
                FileUtils.cp(target_path, svnserve_dir)
              end
            end
            targets -= found_targets
            break if targets.empty?
          end
          # Remove optional targets instead of raising below.  If they are really
          # needed, svnserve won't start anyway.
          targets -= %W[libapriconv#{apr_major_version}.dll]
          unless targets.empty?
            raise "can't find libraries to work svnserve: #{targets.join(' ')}"
          end

          grant_everyone_full_access(svnserve_dir)

          svnserve_path = File.join(svnserve_dir, "svnserve.exe")
          svnserve_path = svnserve_path.tr(File::SEPARATOR,
                                           File::ALT_SEPARATOR)
          svnserve_path = Svnserve.escape_value(svnserve_path)

          root = @full_repos_path.tr(File::SEPARATOR, File::ALT_SEPARATOR)

          args = ["--service", "--root", Svnserve.escape_value(root),
                  "--listen-host", @svnserve_host,
                  "--listen-port", @svnserve_port]
          user = ENV["USERNAME"] || Etc.getlogin
          service_control('create',
                          [["binPath", "#{svnserve_path} #{args.join(' ')}"],
                           ["DisplayName", service_name],
                           ["type", "own"]])
        end
        service_control('start')
        true
      end

      def teardown_svnserve
        service_control('stop') unless service_stopped?
      end

      def add_pre_revprop_change_hook
        File.open("#{@repos.pre_revprop_change_hook}.cmd", "w") do |hook|
          hook.print <<-HOOK
set REPOS=%1
set REV=%2
set USER=%3
set PROPNAME=%4
if "%PROPNAME%" == "#{Svn::Core::PROP_REVISION_LOG}" if "%USER%" == "#{@author}" exit 0
exit 1
          HOOK
        end
      end
    end

    module SetupEnvironment
      def setup_test_environment(top_dir, base_dir, ext_dir)
        @@top_dir = top_dir

        build_type = ENV["BUILD_TYPE"] || "Release"

        FileUtils.mkdir_p(ext_dir)

        relative_base_dir =
          base_dir.sub(/^#{Regexp.escape(top_dir + File::SEPARATOR)}/, '')
        build_base_dir = File.join(top_dir, build_type, relative_base_dir)

        dll_dir = File.expand_path(build_base_dir)
        subversion_dir = File.join(build_base_dir, "..", "..", "..")
        subversion_dir = File.expand_path(subversion_dir)

        util_name = "util"
        build_conf = File.join(top_dir, "build.conf")
        File.open(File.join(ext_dir, "#{util_name}.rb" ), 'w') do |util|
          setup_dll_wrapper_util(dll_dir, util)
          add_depended_dll_path_to_dll_wrapper_util(top_dir, build_type, util)
          add_svn_dll_path_to_dll_wrapper_util(build_conf, subversion_dir, util)
          setup_dll_wrappers(build_conf, ext_dir, dll_dir, util_name) do |lib|
            svn_lib_dir = File.join(subversion_dir, "libsvn_#{lib}")
            util.puts("add_path.call(#{svn_lib_dir.dump})")
          end

          svnserve_dir = File.join(subversion_dir, "svnserve")
          util.puts("add_path.call(#{svnserve_dir.dump})")
        end
      end

      def gen_make_opts
        @gen_make_opts ||= begin
          lines = []
          gen_make_opts = File.join(@@top_dir, "gen-make.opts")
          lines = File.read(gen_make_opts).to_a if File.exists?(gen_make_opts)
          config = Hash.new do |hash, key|
            if /^--with-(.*)$/ =~ key
              hash[key] = File.join(@@top_dir, $1)
            end
          end

          lines.each do |line|
            name, value = line.chomp.split(/\s*=\s*/, 2)
            if value
              config[name] = Pathname.new(value).absolute? ?
                value :
                File.join(@@top_dir, value)
            end
          end
          config
        end
      end
      module_function :gen_make_opts

      private
      def setup_dll_wrapper_util(dll_dir, util)
        libsvn_swig_ruby_dll_dir = File.join(dll_dir, "libsvn_swig_ruby")

        util.puts(<<-EOC)
paths = ENV["PATH"].split(';')
add_path = Proc.new do |path|
  win_path = path.tr(File::SEPARATOR, File::ALT_SEPARATOR)
  unless paths.include?(win_path)
    ENV["PATH"] = "\#{win_path};\#{ENV['PATH']}"
  end
end

add_path.call(#{dll_dir.dump})
add_path.call(#{libsvn_swig_ruby_dll_dir.dump})
EOC
      end

      def add_depended_dll_path_to_dll_wrapper_util(top_dir, build_type, util)
        [
         ["apr", build_type],
         ["apr-util", build_type],
         ["apr-iconv", build_type],
         ["berkeley-db", "bin"],
         ["libintl", "bin"],
         ["sasl", "lib"],
        ].each do |lib, sub_dir|
          lib_dir = Pathname.new(gen_make_opts["--with-#{lib}"])
          dll_dir = lib_dir + sub_dir
          dll_dir = dll_dir.expand_path
          util.puts("add_path.call(#{dll_dir.to_s.dump})")
        end
      end

      def add_svn_dll_path_to_dll_wrapper_util(build_conf, subversion_dir, util)
        File.open(build_conf) do |f|
          f.each do |line|
            if /^\[(libsvn_.+)\]\s*$/ =~ line
              lib_name = $1
              lib_dir = File.join(subversion_dir, lib_name)
              util.puts("add_path.call(#{lib_dir.dump})")
            end
          end
        end
      end

      def setup_dll_wrappers(build_conf, ext_dir, dll_dir, util_name)
        File.open(build_conf) do |f|
          f.each do |line|
            if /^\[swig_(.+)\]\s*$/ =~ line
              lib_name = $1
              File.open(File.join(ext_dir, "#{lib_name}.rb" ), 'w') do |rb|
                rb.puts(<<-EOC)
require File.join(File.dirname(__FILE__), #{util_name.dump})
require File.join(#{dll_dir.dump}, File.basename(__FILE__, '.rb')) + '.so'
EOC
              end

              yield(lib_name)
            end
          end
        end
      end
    end
  end
end
