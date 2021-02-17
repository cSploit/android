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

# experimental

require "optparse"
require "ostruct"
require "time"
require "net/smtp"
require "socket"

require "svn/info"

class OptionParser
  class CannotCoexistOption < ParseError
    const_set(:Reason, 'cannot coexist option'.freeze)
  end
end

module Svn
  class CommitMailer
    KILO_SIZE = 1000
    DEFAULT_MAX_SIZE = "100M"

    class << self
      def run(argv=nil)
        argv ||= ARGV
        repository_path, revision, to, options = parse(argv)
        to = [to, *options.to].compact
        mailer = CommitMailer.new(repository_path, revision, to)
        apply_options(mailer, options)
        mailer.run
      end

      def parse(argv)
        options = make_options

        parser = make_parser(options)
        argv = argv.dup
        parser.parse!(argv)
        repository_path, revision, to, *rest = argv

        [repository_path, revision, to, options]
      end

      def format_size(size)
        return "no limit" if size.nil?
        return "#{size}B" if size < KILO_SIZE
        size /= KILO_SIZE.to_f
        return "#{size}KB" if size < KILO_SIZE
        size /= KILO_SIZE.to_f
        return "#{size}MB" if size < KILO_SIZE
        size /= KILO_SIZE.to_f
        "#{size}GB"
      end

      private
      def apply_options(mailer, options)
        mailer.from = options.from
        mailer.from_domain = options.from_domain
        mailer.add_diff = options.add_diff
        mailer.max_size = options.max_size
        mailer.repository_uri = options.repository_uri
        mailer.rss_path = options.rss_path
        mailer.rss_uri = options.rss_uri
        mailer.rss_title = options.rss_title
        mailer.rss_description = options.rss_description
        mailer.multi_project = options.multi_project
        mailer.show_path = options.show_path
        mailer.trunk_path = options.trunk_path
        mailer.branches_path = options.branches_path
        mailer.tags_path = options.tags_path
        mailer.name = options.name
        mailer.use_utf7 = options.use_utf7
        mailer.server = options.server
        mailer.port = options.port
      end

      def parse_size(size)
        case size
        when /\A(.+?)GB?\z/i
          Float($1) * KILO_SIZE ** 3
        when /\A(.+?)MB?\z/i
          Float($1) * KILO_SIZE ** 2
        when /\A(.+?)KB?\z/i
          Float($1) * KILO_SIZE
        when /\A(.+?)B?\z/i
          Float($1)
        else
          raise ArgumentError, "invalid size: #{size.inspect}"
        end
      end

      def make_options
        options = OpenStruct.new
        options.to = []
        options.error_to = []
        options.from = nil
        options.from_domain = nil
        options.add_diff = true
        options.max_size = parse_size(DEFAULT_MAX_SIZE)
        options.repository_uri = nil
        options.rss_path = nil
        options.rss_uri = nil
        options.rss_title = nil
        options.rss_description = nil
        options.multi_project = false
        options.show_path = false
        options.trunk_path = "trunk"
        options.branches_path = "branches"
        options.tags_path = "tags"
        options.name = nil
        options.use_utf7 = false
        options.server = "localhost"
        options.port = Net::SMTP.default_port
        options
      end

      def make_parser(options)
        OptionParser.new do |opts|
          opts.banner += " REPOSITORY_PATH REVISION TO"

          add_email_options(opts, options)
          add_input_options(opts, options)
          add_rss_options(opts, options)
          add_other_options(opts, options)

          opts.on_tail("--help", "Show this message") do
            puts opts
            exit!
          end
        end
      end

      def add_email_options(opts, options)
        opts.separator ""
        opts.separator "E-mail related options:"

        opts.on("-sSERVER", "--server=SERVER",
                "Use SERVER as SMTP server (#{options.server})") do |server|
          options.server = server
        end

        opts.on("-pPORT", "--port=PORT", Integer,
                "Use PORT as SMTP port (#{options.port})") do |port|
          options.port = port
        end

        opts.on("-tTO", "--to=TO", "Add TO to To: address") do |to|
          options.to << to unless to.nil?
        end

        opts.on("-eTO", "--error-to=TO",
                "Add TO to To: address when an error occurs") do |to|
          options.error_to << to unless to.nil?
        end

        opts.on("-fFROM", "--from=FROM", "Use FROM as from address") do |from|
          if options.from_domain
            raise OptionParser::CannotCoexistOption,
                    "cannot coexist with --from-domain"
          end
          options.from = from
        end

        opts.on("--from-domain=DOMAIN",
                "Use author@DOMAIN as from address") do |domain|
          if options.from
            raise OptionParser::CannotCoexistOption,
                    "cannot coexist with --from"
          end
          options.from_domain = domain
        end
      end

      def add_input_options(opts, options)
        opts.separator ""
        opts.separator "Output related options:"

        opts.on("--[no-]multi-project",
                "Treat as multi-project hosting repository") do |bool|
          options.multi_project = bool
        end

        opts.on("--name=NAME", "Use NAME as repository name") do |name|
          options.name = name
        end

        opts.on("--[no-]show-path",
                "Show commit target path") do |bool|
          options.show_path = bool
        end

        opts.on("--trunk-path=PATH",
                "Treat PATH as trunk path (#{options.trunk_path})") do |path|
          options.trunk_path = path
        end

        opts.on("--branches-path=PATH",
                "Treat PATH as branches path",
                "(#{options.branches_path})") do |path|
          options.branches_path = path
        end

        opts.on("--tags-path=PATH",
                "Treat PATH as tags path (#{options.tags_path})") do |path|
          options.tags_path = path
        end

        opts.on("-rURI", "--repository-uri=URI",
                "Use URI as URI of repository") do |uri|
          options.repository_uri = uri
        end

        opts.on("-n", "--no-diff", "Don't add diffs") do |diff|
          options.add_diff = false
        end

        opts.on("--max-size=SIZE",
                "Limit mail body size to SIZE",
                "G/GB/M/MB/K/KB/B units are available",
                "(#{format_size(options.max_size)})") do |max_size|
          begin
            options.max_size = parse_size(max_size)
          rescue ArgumentError
            raise OptionParser::InvalidArgument, max_size
          end
        end

        opts.on("--no-limit-size",
                "Don't limit mail body size",
                "(#{options.max_size.nil?})") do |not_limit_size|
          options.max_size = nil
        end

        opts.on("--[no-]utf7",
                "Use UTF-7 encoding for mail body instead",
                "of UTF-8 (#{options.use_utf7})") do |use_utf7|
          options.use_utf7 = use_utf7
        end
      end

      def add_rss_options(opts, options)
        opts.separator ""
        opts.separator "RSS related options:"

        opts.on("--rss-path=PATH", "Use PATH as output RSS path") do |path|
          options.rss_path = path
        end

        opts.on("--rss-uri=URI", "Use URI as output RSS URI") do |uri|
          options.rss_uri = uri
        end

        opts.on("--rss-title=TITLE", "Use TITLE as output RSS title") do |title|
          options.rss_title = title
        end

        opts.on("--rss-description=DESCRIPTION",
                "Use DESCRIPTION as output RSS description") do |description|
          options.rss_description = description
        end
      end

      def add_other_options(opts, options)
        opts.separator ""
        opts.separator "Other options:"

        return
        opts.on("-IPATH", "--include=PATH", "Add PATH to load path") do |path|
          $LOAD_PATH.unshift(path)
        end
      end
    end

    attr_reader :to
    attr_writer :from, :add_diff, :multi_project, :show_path, :use_utf7
    attr_accessor :from_domain, :max_size, :repository_uri
    attr_accessor :rss_path, :rss_uri, :rss_title, :rss_description
    attr_accessor :trunk_path, :branches_path
    attr_accessor :tags_path, :name, :server, :port

    def initialize(repository_path, revision, to)
      @info = Svn::Info.new(repository_path, revision)
      @to = to
    end

    def from
      @from || "#{@info.author}@#{@from_domain}".sub(/@\z/, '')
    end

    def run
      send_mail(make_mail)
      output_rss
    end

    def use_utf7?
      @use_utf7
    end

    def add_diff?
      @add_diff
    end

    def multi_project?
      @multi_project
    end

    def show_path?
      @show_path
    end

    private
    def extract_email_address(address)
      if /<(.+?)>/ =~ address
        $1
      else
        address
      end
    end

    def send_mail(mail)
      _from = extract_email_address(from)
      to = @to.collect {|address| extract_email_address(address)}
      Net::SMTP.start(@server || "localhost", @port) do |smtp|
        smtp.open_message_stream(_from, to) do |f|
          f.print(mail)
        end
      end
    end

    def output_rss
      return unless rss_output_available?
      prev_rss = nil
      begin
        if File.exist?(@rss_path)
          File.open(@rss_path) do |f|
            prev_rss = RSS::Parser.parse(f)
          end
        end
      rescue RSS::Error
      end

      rss = make_rss(prev_rss).to_s
      File.open(@rss_path, "w") do |f|
        f.print(rss)
      end
    end

    def rss_output_available?
      if @repository_uri and @rss_path and @rss_uri
        begin
          require 'rss'
          true
        rescue LoadError
          false
        end
      else
        false
      end
    end

    def make_mail
      utf8_body = make_body
      utf7_body = nil
      utf7_body = utf8_to_utf7(utf8_body) if use_utf7?
      if utf7_body
        body = utf7_body
        encoding = "utf-7"
        bit = "7bit"
      else
        body = utf8_body
        encoding = "utf-8"
        bit = "8bit"
      end

      unless @max_size.nil?
        body = truncate_body(body, !utf7_body.nil?)
      end

      make_header(encoding, bit) + "\n" + body
    end

    def make_body
      body = ""
      body << "#{@info.author}\t#{format_time(@info.date)}\n"
      body << "\n"
      body << "  New Revision: #{@info.revision}\n"
      body << "\n"
      body << "  Log:\n"
      stripped_log = @info.log.rstrip
      stripped_log << "\n" unless stripped_log.empty?
      stripped_log.each_line do |line|
        body << "    #{line}"
      end
      body << "\n"
      body << added_dirs
      body << added_files
      body << copied_dirs
      body << copied_files
      body << deleted_dirs
      body << deleted_files
      body << modified_dirs
      body << modified_files
      body << "\n"
      body << change_info
      body
    end

    def format_time(time)
      time.strftime('%Y-%m-%d %X %z (%a, %d %b %Y)')
    end

    def changed_items(title, type, items)
      rv = ""
      unless items.empty?
        rv << "  #{title} #{type}:\n"
        if block_given?
          yield(rv, items)
        else
          rv << items.collect {|item| "    #{item}\n"}.join('')
        end
      end
      rv
    end

    def changed_files(title, files, &block)
      changed_items(title, "files", files, &block)
    end

    def added_files
      changed_files("Added", @info.added_files)
    end

    def deleted_files
      changed_files("Removed", @info.deleted_files)
    end

    def modified_files
      changed_files("Modified", @info.updated_files)
    end

    def copied_files
      changed_files("Copied", @info.copied_files) do |rv, files|
        rv << files.collect do |file, from_file, from_rev|
          <<-INFO
    #{file}
      (from rev #{from_rev}, #{from_file})
INFO
        end.join("")
      end
    end

    def changed_dirs(title, files, &block)
      changed_items(title, "directories", files, &block)
    end

    def added_dirs
      changed_dirs("Added", @info.added_dirs)
    end

    def deleted_dirs
      changed_dirs("Removed", @info.deleted_dirs)
    end

    def modified_dirs
      changed_dirs("Modified", @info.updated_dirs)
    end

    def copied_dirs
      changed_dirs("Copied", @info.copied_dirs) do |rv, dirs|
        rv << dirs.collect do |dir, from_dir, from_rev|
          "    #{dir} (from rev #{from_rev}, #{from_dir})\n"
        end.join("")
      end
    end


    CHANGED_TYPE = {
      :added => "Added",
      :modified => "Modified",
      :deleted => "Deleted",
      :copied => "Copied",
      :property_changed => "Property changed",
    }

    CHANGED_MARK = Hash.new("=")
    CHANGED_MARK[:property_changed] = "_"

    def change_info
      result = changed_dirs_info
      result = "\n#{result}" unless result.empty?
      result << "\n"
      diff_info.each do |key, infos|
        infos.each do |desc, link|
          result << "#{desc}\n"
        end
      end
      result
    end

    def changed_dirs_info
      rev = @info.revision
      (@info.added_dirs.collect do |dir|
         "  Added: #{dir}\n"
       end + @info.copied_dirs.collect do |dir, from_dir, from_rev|
         <<-INFO
  Copied: #{dir}
    (from rev #{from_rev}, #{from_dir})
INFO
       end + @info.deleted_dirs.collect do |dir|
     <<-INFO
  Deleted: #{dir}
    % svn ls #{[@repository_uri, dir].compact.join("/")}@#{rev - 1}
INFO
       end + @info.updated_dirs.collect do |dir|
         "  Modified: #{dir}\n"
       end).join("\n")
    end

    def diff_info
      @info.diffs.collect do |key, values|
        [
         key,
         values.collect do |type, value|
           args = []
           rev = @info.revision
           case type
           when :added
             command = "cat"
           when :modified, :property_changed
             command = "diff"
             args.concat(["-r", "#{@info.revision - 1}:#{@info.revision}"])
           when :deleted
             command = "cat"
             rev -= 1
           when :copied
             command = "cat"
           else
             raise "unknown diff type: #{value.type}"
           end

           command += " #{args.join(' ')}" unless args.empty?

           link = [@repository_uri, key].compact.join("/")

           line_info = "+#{value.added_line} -#{value.deleted_line}"
           desc = <<-HEADER
  #{CHANGED_TYPE[value.type]}: #{key} (#{line_info})
#{CHANGED_MARK[value.type] * 67}
HEADER

           if add_diff?
             desc << value.body
           else
             desc << <<-CONTENT
    % svn #{command} #{link}@#{rev}
CONTENT
           end

           [desc, link]
         end
        ]
      end
    end

    def make_header(body_encoding, body_encoding_bit)
      headers = []
      headers << x_author
      headers << x_revision
      headers << x_repository
      headers << x_id
      headers << "MIME-Version: 1.0"
      headers << "Content-Type: text/plain; charset=#{body_encoding}"
      headers << "Content-Transfer-Encoding: #{body_encoding_bit}"
      headers << "From: #{from}"
      headers << "To: #{to.join(', ')}"
      headers << "Subject: #{make_subject}"
      headers << "Date: #{Time.now.rfc2822}"
      headers.find_all do |header|
        /\A\s*\z/ !~ header
      end.join("\n")
    end

    def detect_project
      return nil unless multi_project?
      project = nil
      @info.paths.each do |path, from_path,|
        [path, from_path].compact.each do |target_path|
          first_component = target_path.split("/", 2)[0]
          project ||= first_component
          return nil if project != first_component
        end
      end
      project
    end

    def affected_paths(project)
      paths = []
      [nil, :branches_path, :tags_path].each do |target|
        prefix = [project]
        prefix << send(target) if target
        prefix = prefix.compact.join("/")
        sub_paths = @info.sub_paths(prefix)
        if target.nil?
          sub_paths = sub_paths.find_all do |sub_path|
            sub_path == trunk_path
          end
        end
        paths.concat(sub_paths)
      end
      paths.uniq
    end

    def make_subject
      subject = ""
      project = detect_project
      subject << "#{@name} " if @name
      revision_info = "r#{@info.revision}"
      if show_path?
        _affected_paths = affected_paths(project)
        unless _affected_paths.empty?
          revision_info = "(#{_affected_paths.join(',')}) #{revision_info}"
        end
      end
      if project
        subject << "[#{project} #{revision_info}] "
      else
        subject << "#{revision_info}: "
      end
      subject << @info.log.lstrip.to_a.first.to_s.chomp
      NKF.nkf("-WM", subject)
    end

    def x_author
      "X-SVN-Author: #{@info.author}"
    end

    def x_revision
      "X-SVN-Revision: #{@info.revision}"
    end

    def x_repository
      if @repository_uri
        repository = "#{@repository_uri}/"
      else
        repository = "XXX"
      end
      "X-SVN-Repository: #{repository}"
    end

    def x_id
      "X-SVN-Commit-Id: #{@info.entire_sha256}"
    end

    def utf8_to_utf7(utf8)
      require 'iconv'
      Iconv.conv("UTF-7", "UTF-8", utf8)
    rescue InvalidEncoding
      begin
        Iconv.conv("UTF7", "UTF8", utf8)
      rescue Exception
        nil
      end
    rescue Exception
      nil
    end

    def truncate_body(body, use_utf7)
      return body if body.size < @max_size

      truncated_body = body[0, @max_size]
      formatted_size = self.class.format_size(@max_size)
      truncated_message = "... truncated to #{formatted_size}\n"
      truncated_message = utf8_to_utf7(truncated_message) if use_utf7
      truncated_message_size = truncated_message.size

      lf_index = truncated_body.rindex(/(?:\r|\r\n|\n)/)
      while lf_index
        if lf_index + truncated_message_size < @max_size
          truncated_body[lf_index, @max_size] = "\n#{truncated_message}"
          break
        else
          lf_index = truncated_body.rindex(/(?:\r|\r\n|\n)/, lf_index - 1)
        end
      end

      truncated_body
    end

    def make_rss(base_rss)
      RSS::Maker.make("1.0") do |maker|
        maker.encoding = "UTF-8"

        maker.channel.about = @rss_uri
        maker.channel.title = rss_title
        maker.channel.link = @repository_uri
        maker.channel.description = rss_description
        maker.channel.dc_date = @info.date

        if base_rss
          base_rss.items.each do |item|
            item.setup_maker(maker)
          end
        end

        diff_info.each do |name, infos|
          infos.each do |desc, link|
            item = maker.items.new_item
            item.title = name
            item.description = @info.log
            item.content_encoded = "<pre>#{RSS::Utils.html_escape(desc)}</pre>"
            item.link = link
            item.dc_date = @info.date
            item.dc_creator = @info.author
          end
        end

        maker.items.do_sort = true
        maker.items.max_size = 15
      end
    end

    def rss_title
      @rss_title || @name || @repository_uri
    end

    def rss_description
      @rss_description || "Repository of #{@name || @repository_uri}"
    end
  end
end
