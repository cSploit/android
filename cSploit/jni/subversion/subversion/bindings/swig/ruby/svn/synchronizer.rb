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

# Experimental

require "cgi"
require "svn/ra"

module Svn
  class Synchronizer
    class Error < StandardError
    end

    class NotInitialized < Error
      attr_reader :destination_url
      def initialize(destination_url)
        @destination_url = destination_url
        super("#{@destination_url} is not initialized yet.")
      end
    end

    class ConsistencyError < Error
      attr_reader :destination_url, :destination_latest_revision, :next_revision
      def initialize(destination_url, destination_latest_revision, next_revision)
        @destination_url = destination_url
        @destination_latest_revision = destination_latest_revision
        @next_revision = next_revision
        super("the latest revision (#{@destination_latest_revision}) of " +
              "the destination repository (#{@destination_url}) " +
              "should be #{@next_revision - 1}.")
      end
    end

    module Property
      Svn::Ext::Core.constants.each do |name|
        case name.to_s
        when /\ASVNSYNC_PROP_/
          const_set($POSTMATCH, Svn::Ext::Core.const_get(name))
        end
      end
    end

    def initialize(destination_url, callbacks=nil)
      @destination = Ra::Session.open(destination_url, nil, callbacks)
      if @destination.repos_root != destination_url
        raise ArgumentError,
              "URL '#{destination_url}' is not repository root " +
              "(#{@destination.repos_root})."
      end
    end

    def source=(source)
      @destination[Property::FROM_URL, 0] = source.repos_root
      @destination[Property::FROM_UUID, 0] = source.uuid
      @destination[Property::LAST_MERGED_REV, 0] = "0"

      sync_property_re = /\A#{Property::PREFIX}/
      source.properties(0).each do |key, value|
        next if sync_property_re =~ key
        @destination[key, 0] = value
      end
    end

    def sync(callbacks=nil)
      last_merged_rev = @destination[Property::LAST_MERGED_REV, 0]
      raise NotInitialized.new(@destination.repos_root) if last_merged_rev.nil?

      last_merged_rev = Integer(last_merged_rev)
      source = Ra::Session.open(@destination[Property::FROM_URL, 0], nil,
                                callbacks)
      (last_merged_rev + 1).upto(source.latest_revision) do |revision|
        unless @destination.latest_revision + 1 == revision
          raise ConsistencyError.new(@destination.repos_root,
                                     @destination.latest_revision,
                                     revision)
        end
        editor = sync_editor(source, revision)
        source.replay(revision, 0, editor)
        editor.close_edit
        sync_svn_revision_properties(source, revision)
        @destination[Property::LAST_MERGED_REV, 0] = revision.to_s
      end
    end

    private
    def sync_editor(source, base_revision)
      properties = source.properties(base_revision)
      commit_editor = @destination.commit_editor2(properties)
      Editor.new(commit_editor, @destination.repos_root, base_revision)
    end

    def sync_svn_revision_properties(source, revision)
      [Core::PROP_REVISION_AUTHOR, Core::PROP_REVISION_DATE].each do |key|
        @destination[key, revision] = source[key, revision]
      end
    end

    class Editor < Delta::WrapEditor
      def initialize(wrapped_editor, dest_url, base_revision)
        super(wrapped_editor)
        @dest_url = dest_url
        @base_revision = base_revision
        @opened_root = false
      end

      def open_root(base_revision)
        @opened_root = true
        super
      end

      def add_directory(path, parent_baton, copy_from_path, copy_from_rev)
        if copy_from_path
          copy_from_path = "#{@dest_url}#{CGI.escape(copy_from_path)}"
        end
        super(path, parent_baton, copy_from_path, copy_from_rev)
      end

      def add_file(path, parent_baton, copy_from_path, copy_from_rev)
        if copy_from_path
          copy_from_path = "#{@dest_url}#{CGI.escape(copy_from_path)}"
        end
        super(path, parent_baton, copy_from_path, copy_from_rev)
      end

      def close_edit
        unless @opened_root
          baton = @wrapped_editor.open_root(@base_revision)
          @wrapped_editor.close_directory(baton)
        end
        super
      end
    end
  end
end
