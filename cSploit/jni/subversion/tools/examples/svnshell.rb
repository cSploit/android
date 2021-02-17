#!/usr/bin/env ruby
#
# svnshell.rb : a Ruby-based shell interface for cruising 'round in
#               the filesystem.
#
# Usage: ruby svnshell.rb REPOS_PATH, where REPOS_PATH is a path to
# a repository on your local filesystem.
#
# NOTE: This program requires the Ruby readline extension.
# See http://wiki.rubyonrails.com/rails/show/ReadlineLibrary
# for details on how to install readline for Ruby.
#
######################################################################
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
######################################################################
#

require "readline"
require "shellwords"

require "svn/fs"
require "svn/core"
require "svn/repos"

# SvnShell: a Ruby-based shell interface for cruising 'round in
#           the filesystem.
class SvnShell

  # A list of potential commands. This list is populated by
  # the 'method_added' function (see below).
  WORDS = []

  # Check for methods that start with "do_"
  # and list them as potential commands
  class << self
    def method_added(name)
      if /^do_(.*)$/ =~ name.to_s
        WORDS << $1
      end
    end
  end

  # Constructor for SvnShell
  #
  # path: The path to a Subversion repository
  def initialize(path)
    @repos_path = path
    @path = "/"
    self.rev = youngest_rev
    @exited = false
  end

  # Run the shell
  def run

    # While the user hasn't typed 'exit' and there is still input to be read
    while !@exited and buf = Readline.readline(prompt, true)

      # Parse the command line into a single command and arguments
      cmd, *args = Shellwords.shellwords(buf)

      # Skip empty lines
      next if /\A\s*\z/ =~ cmd.to_s

      # Open a new connection to the repo
      @fs = Svn::Repos.open(@repos_path).fs
      setup_root

      # Execute the specified command
      dispatch(cmd, *args)

      # Find a path that exists in the current revision
      @path = find_available_path

      # Close the connection to the repo
      @root.close

    end
  end

  # Private functions
  private

  # Get the current prompt string
  def prompt

    # Gather data for the prompt string
    if rev_mode?
      mode = "rev"
      info = @rev
    else
      mode = "txn"
      info = @txn
    end

    # Return the prompt string
    "<#{mode}: #{info} #{@path}>$ "
  end

  # Dispatch a command to the appropriate do_* subroutine
  def dispatch(cmd, *args)

    # Dispatch cmd to the appropriate do_* function
    if respond_to?("do_#{cmd}", true)
      begin
        __send__("do_#{cmd}", *args)
      rescue ArgumentError
        # puts $!.message
        # puts $@
        puts("Invalid argument for #{cmd}: #{args.join(' ')}")
      end
    else
      puts("Unknown command: #{cmd}")
      puts("Try one of these commands: ", WORDS.sort.join(" "))
    end
  end

  # Output the contents of a file from the repository
  def do_cat(path)

    # Normalize the path to an absolute path
    normalized_path = normalize_path(path)

    # Check what type of node exists at the specified path
    case @root.check_path(normalized_path)
    when Svn::Core::NODE_NONE
      puts "Path '#{normalized_path}' does not exist."
    when Svn::Core::NODE_DIR
      puts "Path '#{normalized_path}' is not a file."
    else
      # Output the file to standard out
      @root.file_contents(normalized_path) do |stream|
        puts stream.read(@root.file_length(normalized_path))
      end
    end
  end

  # Set the current directory
  def do_cd(path="/")

    # Normalize the path to an absolute path
    normalized_path = normalize_path(path)

    # If it's a valid directory, then set the directory
    if @root.check_path(normalized_path) == Svn::Core::NODE_DIR
      @path = normalized_path
    else
      puts "Path '#{normalized_path}' is not a valid filesystem directory."
    end
  end

  # List the contents of the current directory or provided paths
  def do_ls(*paths)

    # Default to listing the contents of the current directory
    paths << @path if paths.empty?

    # Foreach path
    paths.each do |path|

      # Normalize the path to an absolute path
      normalized_path = normalize_path(path)

      # Is it a directory or file?
      case @root.check_path(normalized_path)
      when Svn::Core::NODE_DIR

        # Output the contents of the directory
        parent = normalized_path
        entries = @root.dir_entries(parent)

      when Svn::Core::NODE_FILE

        # Split the path into directory and filename components
        parts = path_to_parts(normalized_path)
        name = parts.pop
        parent = parts_to_path(parts)

        # Output the filename
        puts "#{parent}:#{name}"

        # Double check that the file exists
        # inside the parent directory
        parent_entries = @root.dir_entries(parent)
        if parent_entries[name].nil?
          # Hmm. We found the file, but it doesn't exist inside
          # the parent directory. That's a bit unusual.
          puts "No directory entry found for '#{normalized_path}'"
          next
        else
          # Save the path so it can be output in detail
          entries = {name => parent_entries[name]}
        end
      else
        # Path is not a directory or a file,
        # so it must not exist
        puts "Path '#{normalized_path}' not found."
        next
      end

      # Output a detailed listing of the files we found
      puts "   REV   AUTHOR  NODE-REV-ID     SIZE              DATE NAME"
      puts "-" * 76

      # For each entry we found...
      entries.keys.sort.each do |entry|

        # Calculate the full path to the directory entry
        fullpath = parent + '/' + entry
        if @root.dir?(fullpath)
          # If it's a directory, output an extra slash
          size = ''
          name = entry + '/'
        else
          # If it's a file, output the size of the file
          size = @root.file_length(fullpath).to_i.to_s
          name = entry
        end

        # Output the entry
        node_id = entries[entry].id.to_s
        created_rev = @root.node_created_rev(fullpath)
        author = @fs.prop(Svn::Core::PROP_REVISION_AUTHOR, created_rev).to_s
        date = @fs.prop(Svn::Core::PROP_REVISION_DATE, created_rev)
        args = [
          created_rev, author[0,8],
          node_id, size, date.strftime("%b %d %H:%M(%Z)"), name
        ]
        puts "%6s %8s <%10s> %8s %17s %s" % args

      end
    end
  end

  # List all currently open transactions available for browsing
  def do_lstxns

    # Get a sorted list of open transactions
    txns = @fs.transactions
    txns.sort
    counter = 0

    # Output the open transactions
    txns.each do |txn|
      counter = counter + 1
      puts "%8s  " % txn

      # Every six transactions, output an extra newline
      if counter == 6
        puts
        counter = 0
      end
    end
    puts
  end

  # Output the properties of a particular path
  def do_pcat(path=nil)

    # Default to the current directory
    catpath = path ? normalize_path(path) : @path

    # Make sure that the specified path exists
    if @root.check_path(catpath) == Svn::Core::NODE_NONE
      puts "Path '#{catpath}' does not exist."
      return
    end

    # Get the list of properties
    plist = @root.node_proplist(catpath)
    return if plist.nil?

    # Output each property
    plist.each do |key, value|
      puts "K #{key.size}"
      puts key
      puts "P #{value.size}"
      puts value
    end

    # That's all folks!
    puts 'PROPS-END'

  end

  # Set the current revision to view
  def do_setrev(rev)

    # Make sure the specified revision exists
    begin
      @fs.root(Integer(rev)).close
    rescue Svn::Error
      puts "Error setting the revision to '#{rev}': #{$!.message}"
      return
    end

    # Set the revision
    self.rev = Integer(rev)

  end

  # Open an existing transaction to view
  def do_settxn(name)

    # Make sure the specified transaction exists
    begin
      txn = @fs.open_txn(name)
      txn.root.close
    rescue Svn::Error
      puts "Error setting the transaction to '#{name}': #{$!.message}"
      return
    end

    # Set the transaction
    self.txn = name

  end

  # List the youngest revision available for browsing
  def do_youngest
    rev = @fs.youngest_rev
    puts rev
  end

  # Exit this program
  def do_exit
    @exited = true
  end

  # Find the youngest revision
  def youngest_rev
    Svn::Repos.open(@repos_path).fs.youngest_rev
  end

  # Set the current revision
  def rev=(new_value)
    @rev = new_value
    @txn = nil
    reset_root
  end

  # Set the current transaction
  def txn=(new_value)
    @txn = new_value
    reset_root
  end

  # Check whether we are in 'revision-mode'
  def rev_mode?
    @txn.nil?
  end

  # Close the current root and setup a new one
  def reset_root
    if @root
      @root.close
      setup_root
    end
  end

  # Setup a new root
  def setup_root
    if rev_mode?
      @root = @fs.root(@rev)
    else
      @root = @fs.open_txn(name).root
    end
  end

  # Convert a path into its component parts
  def path_to_parts(path)
    path.split(/\/+/)
  end

  # Join the component parts of a path into a string
  def parts_to_path(parts)
    normalized_parts = parts.reject{|part| part.empty?}
    "/#{normalized_parts.join('/')}"
  end

  # Convert a path to a normalized, absolute path
  def normalize_path(path)

    # Convert the path to an absolute path
    if path[0,1] != "/" and @path != "/"
      path = "#{@path}/#{path}"
    end

    # Split the path into its component parts
    parts = path_to_parts(path)

    # Build a list of the normalized parts of the path
    normalized_parts = []
    parts.each do |part|
      case part
      when "."
        # ignore
      when ".."
        normalized_parts.pop
      else
        normalized_parts << part
      end
    end

    # Join the normalized parts together into a string
    parts_to_path(normalized_parts)

  end

  # Find the parent directory of a specified path
  def parent_dir(path)
    normalize_path("#{path}/..")
  end

  # Try to land on the specified path as a directory.
  # If the specified path does not exist, look for
  # an ancestor path that does exist.
  def find_available_path(path=@path)
    if @root.check_path(path) == Svn::Core::NODE_DIR
      path
    else
      find_available_path(parent_dir(path))
    end
  end

end


# Autocomplete commands
Readline.completion_proc = Proc.new do |word|
  SvnShell::WORDS.grep(/^#{Regexp.quote(word)}/)
end

# Output usage information if necessary
if ARGV.size != 1
  puts "Usage: #{$0} REPOS_PATH"
  exit(1)
end

# Create a new SvnShell with the command-line arguments and run it
SvnShell.new(ARGV.shift).run
