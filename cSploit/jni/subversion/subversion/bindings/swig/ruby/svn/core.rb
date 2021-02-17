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

require "English"
require "time"
require "stringio"
require "tempfile"
require "svn/util"
require "svn/error"
require "svn/ext/core"

class Time
  MILLION = 1_000_000 #:nodoc:

  class << self
    def from_apr_time(apr_time)
      return apr_time if apr_time.is_a?(Time)
      sec, usec = apr_time.divmod(MILLION)
      Time.at(sec, usec)
    end

    def from_svn_format(str)
      return nil if str.nil?
      return str if str.is_a?(Time)
      from_apr_time(Svn::Core.time_from_cstring(str))
    end

    def parse_svn_format(str)
      return str if str.is_a?(Time)
      matched, result = Svn::Core.parse_date(str, Time.now.to_apr_time)
      if matched
        from_apr_time(result)
      else
        nil
      end
    end
  end

  def to_apr_time
    to_i * MILLION + usec
  end

  def to_svn_format
    Svn::Core.time_to_cstring(self.to_apr_time)
  end

  def to_svn_human_format
    Svn::Core.time_to_human_cstring(self.to_apr_time)
  end
end

module Svn
  module Core
    Util.set_constants(Ext::Core, self)
    Util.set_methods(Ext::Core, self)

    nls_init
    Util.reset_message_directory

    # for backward compatibility
    SWIG_INVALID_REVNUM = INVALID_REVNUM
    SWIG_IGNORED_REVNUM = IGNORED_REVNUM

    class << self
      alias binary_mime_type? mime_type_is_binary
      alias prop_diffs2 prop_diffs

      def prop_diffs(target_props, source_props)
        Property.prop_diffs(target_props, source_props)
      end
    end


    DEFAULT_CHARSET = default_charset
    LOCALE_CHARSET = locale_charset

    AuthCredSSLClientCert = AuthCredSslClientCert
    AuthCredSSLClientCertPw = AuthCredSslClientCertPw
    AuthCredSSLServerTrust = AuthCredSslServerTrust

    dirent_all = 0
    constants.each do |name|
      dirent_all |= const_get(name) if /^DIRENT_/ =~ name
    end
    DIRENT_ALL = dirent_all

    Pool = Svn::Ext::Core::Apr_pool_wrapper_t

    class Pool
      RECOMMENDED_MAX_FREE_SIZE = ALLOCATOR_RECOMMENDED_MAX_FREE
      MAX_FREE_UNLIMITED = ALLOCATOR_MAX_FREE_UNLIMITED

      class << self
        def number_of_pools
          ObjectSpace.each_object(Pool) {}
        end
      end

      alias _initialize initialize
      private :_initialize
      def initialize(parent=nil)
        _initialize(parent)
        @parent = parent
      end

      def destroy
        @parent = nil
        _destroy
      end
      private :_destroy
    end

    class Stream
      if Core.const_defined?(:STREAM_CHUNK_SIZE)
        CHUNK_SIZE = Core::STREAM_CHUNK_SIZE
      else
        CHUNK_SIZE = 8192
      end

      def write(data)
        Core.stream_write(self, data)
      end

      def read(len=nil)
        if len.nil?
          read_all
        else
          buf = ""
          while len > CHUNK_SIZE
            buf << _read(CHUNK_SIZE)
            len -= CHUNK_SIZE
          end
          buf << _read(len)
          buf
        end
      end

      def close
        Core.stream_close(self)
      end

      def copy(other, &cancel_proc)
        Core.stream_copy2(self, other, cancel_proc)
      end

      private
      def _read(size)
        Core.stream_read(self, size)
      end

      def read_all
        buf = ""
        while chunk = _read(CHUNK_SIZE)
          buf << chunk
        end
        buf
      end
    end


    class AuthBaton
      attr_reader :providers, :parameters

      alias _initialize initialize
      private :_initialize
      def initialize(providers=[], parameters={})
        _initialize(providers)
        @providers = providers
        self.parameters = parameters
      end

      def [](name)
        Core.auth_get_parameter(self, name)
      end

      def []=(name, value)
        Core.auth_set_parameter(self, name, value)
        @parameters[name] = value
      end

      def parameters=(params)
        @parameters = {}
        params.each do |key, value|
          self[key] = value
        end
      end
    end

    module Authenticatable
      attr_accessor :auth_baton

      def add_simple_provider
        add_provider(Core.auth_get_simple_provider)
      end

      if Util.windows?
        if Core.respond_to?(:auth_get_windows_simple_provider)
          def add_windows_simple_provider
            add_provider(Core.auth_get_windows_simple_provider)
          end
        elsif Core.respond_to?(:auth_get_platform_specific_provider)
          def add_windows_simple_provider
            add_provider(Core.auth_get_platform_specific_provider("windows","simple"))
          end
        end
      end

      if Core.respond_to?(:auth_get_keychain_simple_provider)
        def add_keychain_simple_provider
          add_provider(Core.auth_get_keychain_simple_provider)
        end
      end

      def add_username_provider
        add_provider(Core.auth_get_username_provider)
      end

      def add_ssl_client_cert_file_provider
        add_provider(Core.auth_get_ssl_client_cert_file_provider)
      end

      def add_ssl_client_cert_pw_file_provider
        add_provider(Core.auth_get_ssl_client_cert_pw_file_provider)
      end

      def add_ssl_server_trust_file_provider
        add_provider(Core.auth_get_ssl_server_trust_file_provider)
      end

      if Core.respond_to?(:auth_get_windows_ssl_server_trust_provider)
        def add_windows_ssl_server_trust_provider
          add_provider(Core.auth_get_windows_ssl_server_trust_provider)
        end
      end

      def add_simple_prompt_provider(retry_limit, prompt=Proc.new)
        args = [retry_limit]
        klass = AuthCredSimple
        add_prompt_provider("simple", args, prompt, klass)
      end

      def add_username_prompt_provider(retry_limit, prompt=Proc.new)
        args = [retry_limit]
        klass = AuthCredUsername
        add_prompt_provider("username", args, prompt, klass)
      end

      def add_ssl_server_trust_prompt_provider(prompt=Proc.new)
        args = []
        klass = AuthCredSSLServerTrust
        add_prompt_provider("ssl_server_trust", args, prompt, klass)
      end

      def add_ssl_client_cert_prompt_provider(retry_limit, prompt=Proc.new)
        args = [retry_limit]
        klass = AuthCredSSLClientCert
        add_prompt_provider("ssl_client_cert", args, prompt, klass)
      end

      def add_ssl_client_cert_pw_prompt_provider(retry_limit, prompt=Proc.new)
        args = [retry_limit]
        klass = AuthCredSSLClientCertPw
        add_prompt_provider("ssl_client_cert_pw", args, prompt, klass)
      end

      def add_platform_specific_client_providers(config=nil)
        add_providers(Core.auth_get_platform_specific_client_providers(config))
      end

      private
      def add_prompt_provider(name, args, prompt, credential_class)
        real_prompt = Proc.new do |*prompt_args|
          credential = credential_class.new
          prompt.call(credential, *prompt_args)
          credential
        end
        method_name = "swig_rb_auth_get_#{name}_prompt_provider"
        baton, provider = Core.send(method_name, real_prompt, *args)
        provider.instance_variable_set("@baton", baton)
        provider.instance_variable_set("@prompt", real_prompt)
        add_provider(provider)
      end

      def add_provider(provider)
        add_providers([provider])
      end

      def add_providers(new_providers)
        if auth_baton
          providers = auth_baton.providers
          parameters = auth_baton.parameters
        else
          providers = []
          parameters = {}
        end
        self.auth_baton = AuthBaton.new(providers + new_providers, parameters)
      end
    end

    class AuthProviderObject
      class << self
        undef new
      end
    end


    Diff = SWIG::TYPE_p_svn_diff_t
    class Diff
      attr_accessor :original, :modified, :latest, :ancestor

      class << self
        def version
          Core.diff_version
        end

        def file_diff(original, modified, options=nil)
          options ||= Core::DiffFileOptions.new
          diff = Core.diff_file_diff_2(original, modified, options)
          if diff
            diff.original = original
            diff.modified = modified
          end
          diff
        end

        def file_diff3(original, modified, latest, options=nil)
          options ||= Core::DiffFileOptions.new
          diff = Core.diff_file_diff3_2(original, modified, latest, options)
          if diff
            diff.original = original
            diff.modified = modified
            diff.latest = latest
          end
          diff
        end

        def file_diff4(original, modified, latest, ancestor, options=nil)
          options ||= Core::DiffFileOptions.new
          args = [original, modified, latest, ancestor, options]
          diff = Core.diff_file_diff4_2(*args)
          if diff
            diff.original = original
            diff.modified = modified
            diff.latest = latest
            diff.ancestor = ancestor
          end
          diff
        end
      end

      def unified(orig_label, mod_label, header_encoding=nil)
        header_encoding ||= Svn::Core.locale_charset
        output = StringIO.new
        args = [
          output, self, @original, @modified,
          orig_label, mod_label, header_encoding
        ]
        Core.diff_file_output_unified2(*args)
        output.rewind
        output.read
      end

      def merge(conflict_original=nil, conflict_modified=nil,
                conflict_latest=nil, conflict_separator=nil,
                display_original_in_conflict=true,
                display_resolved_conflicts=true)
        header_encoding ||= Svn::Core.locale_charset
        output = StringIO.new
        args = [
          output, self, @original, @modified, @latest,
          conflict_original, conflict_modified,
          conflict_latest, conflict_separator,
          display_original_in_conflict,
          display_resolved_conflicts,
        ]
        Core.diff_file_output_merge(*args)
        output.rewind
        output.read
      end

      def conflict?
        Core.diff_contains_conflicts(self)
      end

      def diff?
        Core.diff_contains_diffs(self)
      end
    end

    class DiffFileOptions
      class << self
        def parse(*args)
          options = new
          options.parse(*args)
          options
        end
      end

      def parse(*args)
        args = args.first if args.size == 1 and args.first.is_a?(Array)
        Svn::Core.diff_file_options_parse(self, args)
      end
    end

    class Version

      alias _initialize initialize
      def initialize(major=nil, minor=nil, patch=nil, tag=nil)
        _initialize
        self.major = major if major
        self.minor = minor if minor
        self.patch = patch if patch
        self.tag = tag || ""
      end

      def ==(other)
        valid? and other.valid? and Core.ver_equal(self, other)
      end

      def compatible?(other)
        valid? and other.valid? and Core.ver_compatible(self, other)
      end

      def valid?
        (major and minor and patch and tag) ? true : false
      end

      alias _tag= tag=
      def tag=(value)
        @tag = value
        self._tag = value
      end

      def to_a
        [major, minor, patch, tag]
      end

      def to_s
        "#{major}.#{minor}.#{patch}#{tag}"
      end
    end

    # Following methods are also available:
    #
    # [created_rev]
    #   Returns a revision at which the instance was last modified.
    # [have_props?]
    #   Returns +true+ if the instance has properties.
    # [last_author]
    #   Returns an author who last modified the instance.
    # [size]
    #   Returns a size of the instance.
    class Dirent
      alias have_props? has_props

      # Returns +true+ when the instance is none.
      def none?
        kind == NODE_NONE
      end

      # Returns +true+ when the instance is a directory.
      def directory?
        kind == NODE_DIR
      end

      # Returns +true+ when the instance is a file.
      def file?
        kind == NODE_FILE
      end

      # Returns +true+ when the instance is an unknown node.
      def unknown?
        kind == NODE_UNKNOWN
      end

      # Returns a Time when the instance was last changed.
      #
      # Svn::Core::Dirent#time is replaced by this method, _deprecated_,
      # and provided for backward compatibility with the 1.3 API.
      def time2
        __time = time
        __time && Time.from_apr_time(__time)
      end
    end

    Config = SWIG::TYPE_p_svn_config_t

    class Config
      include Enumerable

      class << self
        def get(path=nil)
          Core.config_get_config(path)
        end
        alias config get

        def read(file, must_exist=true)
          Core.config_read(file, must_exist)
        end

        def ensure(dir)
          Core.config_ensure(dir)
        end

        def read_auth_data(cred_kind, realm_string, config_dir=nil)
          Core.config_read_auth_data(cred_kind, realm_string, config_dir)
        end

        def write_auth_data(hash, cred_kind, realm_string, config_dir=nil)
          Core.config_write_auth_data(hash, cred_kind,
                                      realm_string, config_dir)
        end
      end

      def merge(file, must_exist=true)
        Core.config_merge(self, file, must_exist)
      end

      def get(section, option, default=nil)
        Core.config_get(self, section, option, default)
      end

      def get_bool(section, option, default)
        Core.config_get_bool(self, section, option, default)
      end

      def set(section, option, value)
        Core.config_set(self, section, option, value)
      end
      alias_method :[]=, :set

      def set_bool(section, option, value)
        Core.config_set_bool(self, section, option, value)
      end

      def each
        each_section do |section|
          each_option(section) do |name, value|
            yield(section, name, value)
            true
          end
          true
        end
      end

      def each_option(section)
        receiver = Proc.new do |name, value|
          yield(name, value)
        end
        Core.config_enumerate2(self, section, receiver)
      end

      def each_section
        receiver = Proc.new do |name|
          yield(name)
        end
        Core.config_enumerate_sections2(self, receiver)
      end

      def find_group(key, section)
        Core.config_find_group(self, key, section)
      end

      def get_server_setting(group, name, default=nil)
        Core.config_get_server_setting(self, group, name, default)
      end

      def get_server_setting_int(group, name, default)
        Core.config_get_server_setting_int(self, group, name, default)
      end

      alias_method :_to_s, :to_s
      def to_s
        result = ""
        each_section do |section|
          result << "[#{section}]\n"
          each_option(section) do |name, value|
            result << "#{name} = #{value}\n"
          end
          result << "\n"
        end
        result
      end

      def inspect
        "#{_to_s}#{to_hash.inspect}"
      end

      def to_hash
        sections = {}
        each do |section, name, value|
          sections[section] ||= {}
          sections[section][name] = value
        end
        sections
      end

      def ==(other)
        other.is_a?(self.class) and to_hash == other.to_hash
      end
    end

    module Property
      module_function
      def kind(name)
        kind, len = Core.property_kind(name)
        [kind, name[0...len]]
      end

      def svn_prop?(name)
        Core.prop_is_svn_prop(name)
      end

      def needs_translation?(name)
        Core.prop_needs_translation(name)
      end

      def categorize(props)
        categorize2(props).collect do |categorized_props|
          Util.hash_to_prop_array(categorized_props)
        end
      end
      alias_method :categorize_props, :categorize
      module_function :categorize_props

      def categorize2(props)
        Core.categorize_props(props)
      end

      def diffs(target_props, source_props)
        Util.hash_to_prop_array(diffs2(target_props, source_props))
      end
      alias_method :prop_diffs, :diffs
      module_function :prop_diffs

      def diffs2(target_props, source_props)
        Core.prop_diffs2(target_props, source_props)
      end

      def has_svn_prop?(props)
        Core.prop_has_svn_prop(props)
      end
      alias_method :have_svn_prop?, :has_svn_prop?
      module_function :have_svn_prop?

      def valid_name?(name)
        Core.prop_name_is_valid(name)
      end
    end

    module Depth
      module_function
      def from_string(str)
        return nil if str.nil?
        Core.depth_from_word(str)
      end

      def to_string(depth)
        Core.depth_to_word(depth)
      end

      def infinity_or_empty_from_recurse(depth_or_recurse)
        case depth_or_recurse
          when true  then DEPTH_INFINITY
          when false then DEPTH_EMPTY
          else depth_or_recurse
        end
      end

      def infinity_or_immediates_from_recurse(depth_or_recurse)
        case depth_or_recurse
          when true  then DEPTH_INFINITY
          when false then DEPTH_IMMEDIATES
          else depth_or_recurse
        end
      end
    end

    module MimeType
      module_function
      def parse(source)
        file = Tempfile.new("svn-ruby-mime-type")
        file.print(source)
        file.close
        Core.io_parse_mimetypes_file(file.path)
      end

      def parse_file(path)
        Core.io_parse_mimetypes_file(path)
      end

      def detect(path, type_map={})
        Core.io_detect_mimetype2(path, type_map)
      end
    end

    class CommitInfo
      alias _date date
      def date
        Time.from_svn_format(_date)
      end
    end

    # Following methods are also available:
    #
    # [action]
    #   Returns an action taken to the path at the revision.
    # [copyfrom_path]
    #   If the path was added at the revision by the copy action from
    #   another path at another revision, returns an original path.
    #   Otherwise, returns +nil+.
    # [copyfrom_rev]
    #   If the path was added at the revision by the copy action from
    #   another path at another revision, returns an original revision.
    #   Otherwise, returns <tt>-1</tt>.
    class LogChangedPath
      # Returns +true+ when the path is added by the copy action.
      def copied?
        Util.copy?(copyfrom_path, copyfrom_rev)
      end
    end

    # For backward compatibility
    class Prop
      attr_accessor :name, :value
      def initialize(name, value)
        @name = name
        @value = value
      end

      def ==(other)
        other.is_a?(self.class) and
          [@name, @value] == [other.name, other.value]
      end
    end

    class MergeRange
      def to_a
        [self.start, self.end, self.inheritable]
      end

      def inspect
        super.gsub(/>$/, ":#{to_a.inspect}>")
      end

      def ==(other)
        to_a == other.to_a
      end
    end

    class MergeInfo < Hash
      class << self
        def parse(input)
          new(Core.mergeinfo_parse(input))
        end
      end

      def initialize(info)
        super()
        info.each do |path, ranges|
          self[path] = RangeList.new(*ranges)
        end
      end

      def diff(to, consider_inheritance=false)
        Core.mergeinfo_diff(self, to, consider_inheritance).collect do |result|
          self.class.new(result)
        end
      end

      def merge(changes)
        self.class.new(Core.swig_mergeinfo_merge(self, changes))
      end

      def remove(eraser)
        self.class.new(Core.mergeinfo_remove(eraser, self))
      end

      def sort
        self.class.new(Core.swig_mergeinfo_sort(self))
      end

      def to_s
        Core.mergeinfo_to_string(self)
      end
    end

    class RangeList < Array
      def initialize(*ranges)
        super()
        ranges.each do |range|
          self << Svn::Core::MergeRange.new(*range.to_a)
        end
      end

      def diff(to, consider_inheritance=false)
        result = Core.rangelist_diff(self, to, consider_inheritance)
        deleted = result.pop
        added = result
        [added, deleted].collect do |result|
          self.class.new(*result)
        end
      end

      def merge(changes)
        self.class.new(*Core.swig_rangelist_merge(self, changes))
      end

      def remove(eraser, consider_inheritance=false)
        self.class.new(*Core.rangelist_remove(eraser, self,
                                              consider_inheritance))
      end

      def intersect(other, consider_inheritance=false)
        self.class.new(*Core.rangelist_intersect(self, other,
                                                 consider_inheritance))
      end

      def reverse
        self.class.new(*Core.swig_rangelist_reverse(self))
      end

      def to_s
        Core.rangelist_to_string(self)
      end
    end

    class LogEntry
      alias_method(:revision_properties, :revprops)
      alias_method(:has_children?, :has_children)
      undef_method(:has_children)
    end
  end
end
