require 'fileutils'
require 'rubygems/command'
require 'rubygems/remote_fetcher'
require 'rubygems/source_info_cache'
require 'rubygems/spec_fetcher'
require 'rubygems/local_remote_options'

class Gem::Commands::SourcesCommand < Gem::Command

  include Gem::LocalRemoteOptions

  def initialize
    super 'sources',
          'Manage the sources and cache file RubyGems uses to search for gems'

    add_option '-a', '--add SOURCE_URI', 'Add source' do |value, options|
      options[:add] = value
    end

    add_option '-l', '--list', 'List sources' do |value, options|
      options[:list] = value
    end

    add_option '-r', '--remove SOURCE_URI', 'Remove source' do |value, options|
      options[:remove] = value
    end

    add_option '-c', '--clear-all',
               'Remove all sources (clear the cache)' do |value, options|
      options[:clear_all] = value
    end

    add_option '-u', '--update', 'Update source cache' do |value, options|
      options[:update] = value
    end

    add_proxy_option
  end

  def defaults_str
    '--list'
  end

  def execute
    options[:list] = !(options[:add] ||
                       options[:clear_all] ||
                       options[:remove] ||
                       options[:update])

    if options[:clear_all] then
      path = Gem::SpecFetcher.fetcher.dir
      FileUtils.rm_rf path

      if not File.exist?(path) then
        say "*** Removed specs cache ***"
      elsif not File.writable?(path) then
        say "*** Unable to remove source cache (write protected) ***"
      else
        say "*** Unable to remove source cache ***"
      end

      sic = Gem::SourceInfoCache
      remove_cache_file 'user',          sic.user_cache_file
      remove_cache_file 'latest user',   sic.latest_user_cache_file
      remove_cache_file 'system',        sic.system_cache_file
      remove_cache_file 'latest system', sic.latest_system_cache_file
    end

    if options[:add] then
      source_uri = options[:add]
      uri = URI.parse source_uri

      begin
        Gem::SpecFetcher.fetcher.load_specs uri, 'specs'
        Gem.sources << source_uri
        Gem.configuration.write

        say "#{source_uri} added to sources"
      rescue URI::Error, ArgumentError
        say "#{source_uri} is not a URI"
      rescue Gem::RemoteFetcher::FetchError => e
        yaml_uri = uri + 'yaml'
        gem_repo = Gem::RemoteFetcher.fetcher.fetch_size yaml_uri rescue false

        if e.uri =~ /specs\.#{Regexp.escape Gem.marshal_version}\.gz$/ and
           gem_repo then

          alert_warning <<-EOF
RubyGems 1.2+ index not found for:
\t#{source_uri}

Will cause RubyGems to revert to legacy indexes, degrading performance.
          EOF

          say "#{source_uri} added to sources"
        else
          say "Error fetching #{source_uri}:\n\t#{e.message}"
        end
      end
    end

    if options[:remove] then
      source_uri = options[:remove]

      unless Gem.sources.include? source_uri then
        say "source #{source_uri} not present in cache"
      else
        Gem.sources.delete source_uri
        Gem.configuration.write

        say "#{source_uri} removed from sources"
      end
    end

    if options[:update] then
      fetcher = Gem::SpecFetcher.fetcher

      if fetcher.legacy_repos.empty? then
        Gem.sources.each do |update_uri|
          update_uri = URI.parse update_uri
          fetcher.load_specs update_uri, 'specs'
          fetcher.load_specs update_uri, 'latest_specs'
        end
      else
        Gem::SourceInfoCache.cache true
        Gem::SourceInfoCache.cache.flush
      end

      say "source cache successfully updated"
    end

    if options[:list] then
      say "*** CURRENT SOURCES ***"
      say

      Gem.sources.each do |source|
        say source
      end
    end
  end

  private

  def remove_cache_file(desc, path)
    FileUtils.rm_rf path

    if not File.exist?(path) then
      say "*** Removed #{desc} source cache ***"
    elsif not File.writable?(path) then
      say "*** Unable to remove #{desc} source cache (write protected) ***"
    else
      say "*** Unable to remove #{desc} source cache ***"
    end
  end

end

