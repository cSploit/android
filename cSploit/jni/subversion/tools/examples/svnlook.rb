#!/usr/bin/env ruby
#
# svnlook.rb : a Ruby-based replacement for svnlook
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

require "svn/core"
require "svn/fs"
require "svn/delta"
require "svn/repos"

# Chomp off trailing slashes
def basename(path)
  path.chomp("/")
end

# SvnLook: a Ruby-based replacement for svnlook
class SvnLook

  # Initialize the SvnLook application
  def initialize(path, rev, txn)
    # Open a repository
    @fs = Svn::Repos.open(basename(path)).fs

    # If a transaction was specified, open it
    if txn
      @txn = @fs.open_txn(txn)
    else
      # Use the latest revision from the repo,
      # if they haven't specified a revision
      @txn = nil
      rev ||= @fs.youngest_rev
    end

    @rev = rev
  end

  # Dispatch all commands to appropriate subroutines
  def run(cmd, *args)
    dispatch(cmd, *args)
  end

  private

  # Dispatch all commands to appropriate subroutines
  def dispatch(cmd, *args)
    if respond_to?("cmd_#{cmd}", true)
      begin
        __send__("cmd_#{cmd}", *args)
      rescue ArgumentError
        puts $!.message
        puts $@
        puts("invalid argument for #{cmd}: #{args.join(' ')}")
      end
    else
      puts("unknown command: #{cmd}")
    end
  end

  # Default command: Run the 'info' and 'tree' commands
  def cmd_default
    cmd_info
    cmd_tree
  end

  # Print the 'author' of the specified revision or transaction
  def cmd_author
    puts(property(Svn::Core::PROP_REVISION_AUTHOR) || "")
  end

  # Not implemented yet
  def cmd_cat
  end

  # Find out what has changed in the specified revision or transaction
  def cmd_changed
    print_tree(ChangedEditor, nil, true)
  end

  # Output the date that the current revision was committed.
  def cmd_date
    if @txn
      # It's not committed yet, so output nothing
      puts
    else
      # Get the time the revision was committed
      date = property(Svn::Core::PROP_REVISION_DATE)

      if date
        # Print out the date in a nice format
        puts date.strftime('%Y-%m-%d %H:%M(%Z)')
      else
        # The specified revision doesn't have an associated date.
        # Output just a blank line.
        puts
      end
    end
  end

  # Output what changed in the specified revision / transaction
  def cmd_diff
    print_tree(DiffEditor, nil, true)
  end

  # Output what directories changed in the specified revision / transaction
  def cmd_dirs_changed
    print_tree(DirsChangedEditor)
  end

  # Output the tree, with node ids
  def cmd_ids
    print_tree(Editor, 0, true)
  end

  # Output the author, date, and the log associated with the specified
  # revision / transaction
  def cmd_info
    cmd_author
    cmd_date
    cmd_log(true)
  end

  # Output the log message associated with the specified revision / transaction
  def cmd_log(print_size=false)
    log = property(Svn::Core::PROP_REVISION_LOG) || ''
    puts log.length if print_size
    puts log
  end

  # Output the tree associated with the provided tree
  def cmd_tree
    print_tree(Editor, 0)
  end

  # Output the repository's UUID.
  def cmd_uuid
    puts @fs.uuid
  end

  # Output the repository's youngest revision.
  def cmd_youngest
    puts @fs.youngest_rev
  end

  # Return a property of the specified revision or transaction.
  # Name: the ID of the property you want to retrieve.
  #       E.g. Svn::Core::PROP_REVISION_LOG
  def property(name)
    if @txn
      @txn.prop(name)
    else
      @fs.prop(name, @rev)
    end
  end

  # Print a tree of differences between two revisions
  def print_tree(editor_class, base_rev=nil, pass_root=false)
    if base_rev.nil?
      if @txn
        # Output changes since the base revision of the transaction
        base_rev = @txn.base_revision
      else
        # Output changes since the previous revision
        base_rev = @rev - 1
      end
    end

    # Get the root of the specified transaction or revision
    if @txn
      root = @txn.root
    else
      root = @fs.root(@rev)
    end

    # Get the root of the base revision
    base_root = @fs.root(base_rev)

    # Does the provided editor need to know
    # the revision and base revision we're working with?
    if pass_root
      # Create a new editor with the provided root and base_root
      editor = editor_class.new(root, base_root)
    else
      # Create a new editor with nil root and base_roots
      editor = editor_class.new
    end

    # Do a directory delta between the two roots with
    # the specified editor
    base_root.dir_delta('', '', root, '', editor)
  end

  # Output the current tree for a specified revision
  class Editor < Svn::Delta::BaseEditor

    # Initialize the Editor object
    def initialize(root=nil, base_root=nil)
      @root = root
      # base_root ignored

      @indent = ""
    end

    # Recurse through the root (and increase the indent level)
    def open_root(base_revision)
      puts "/#{id('/')}"
      @indent << ' '
    end

    # If a directory is added, output this and increase
    # the indent level
    def add_directory(path, *args)
      puts "#{@indent}#{basename(path)}/#{id(path)}"
      @indent << ' '
    end

    alias open_directory add_directory

    # If a directory is closed, reduce the ident level
    def close_directory(baton)
      @indent.chop!
    end

    # If a file is added, output that it has been changed
    def add_file(path, *args)
      puts "#{@indent}#{basename(path)}#{id(path)}"
    end

    alias open_file add_file

    # Private methods
    private

    # Get the node id of a particular path
    def id(path)
      if @root
        fs_id = @root.node_id(path)
        " <#{fs_id.unparse}>"
      else
        ""
      end
    end
  end


  # Output directories that have been changed.
  # In this class, methods such as open_root and add_file
  # are inherited from Svn::Delta::ChangedDirsEditor.
  class DirsChangedEditor < Svn::Delta::ChangedDirsEditor

    # Private functions
    private

    # Print out the name of a directory if it has been changed.
    # But only do so once.
    # This behaves in a way like a callback function does.
    def dir_changed(baton)
      if baton[0]
        # The directory hasn't been printed yet,
        # so print it out.
        puts baton[1] + '/'

        # Make sure we don't print this directory out twice
        baton[0] = nil
      end
    end
  end

  # Output files that have been changed between two roots
  class ChangedEditor < Svn::Delta::BaseEditor

    # Constructor
    def initialize(root, base_root)
      @root = root
      @base_root = base_root
    end

    # Look at the root node
    def open_root(base_revision)
      # Nothing has been printed out yet, so return 'true'.
      [true, '']
    end

    # Output deleted files
    def delete_entry(path, revision, parent_baton)
      # Output deleted paths with a D in front of them
      print "D   #{path}"

      # If we're deleting a directory,
      # indicate this with a trailing slash
      if @base_root.dir?('/' + path)
        puts "/"
      else
        puts
      end
    end

    # Output that a directory has been added
    def add_directory(path, parent_baton,
                      copyfrom_path, copyfrom_revision)
      # Output 'A' to indicate that the directory was added.
      # Also put a trailing slash since it's a directory.
      puts "A   #{path}/"

      # The directory has been printed -- don't print it again.
      [false, path]
    end

    # Recurse inside directories
    def open_directory(path, parent_baton, base_revision)
      # Nothing has been printed out yet, so return true.
      [true, path]
    end

    def change_dir_prop(dir_baton, name, value)
      # Has the directory been printed yet?
      if dir_baton[0]
        # Print the directory
        puts "_U  #{dir_baton[1]}/"

        # Don't let this directory get printed again.
        dir_baton[0] = false
      end
    end

    def add_file(path, parent_baton,
                 copyfrom_path, copyfrom_revision)
      # Output that a directory has been added
      puts "A   #{path}"

      # We've already printed out this entry, so return '_'
      # to prevent it from being printed again
      ['_', ' ', nil]
    end


    def open_file(path, parent_baton, base_revision)
      # Changes have been made -- return '_' to indicate as such
      ['_', ' ', path]
    end

    def apply_textdelta(file_baton, base_checksum)
      # The file has been changed -- we'll print that out later.
      file_baton[0] = 'U'
      nil
    end

    def change_file_prop(file_baton, name, value)
      # The file has been changed -- we'll print that out later.
      file_baton[1] = 'U'
    end

    def close_file(file_baton, text_checksum)
      text_mod, prop_mod, path = file_baton
      # Test the path. It will be nil if we added this file.
      if path
        status = text_mod + prop_mod
        # Was there some kind of change?
        if status != '_ '
          puts "#{status}  #{path}"
        end
      end
    end
  end

  # Output diffs of files that have been changed
  class DiffEditor < Svn::Delta::BaseEditor

    # Constructor
    def initialize(root, base_root)
      @root = root
      @base_root = base_root
    end

    # Handle deleted files and directories
    def delete_entry(path, revision, parent_baton)
      # Print out diffs of deleted files, but not
      # deleted directories
      unless @base_root.dir?('/' + path)
        do_diff(path, nil)
      end
    end

    # Handle added files
    def add_file(path, parent_baton,
                 copyfrom_path, copyfrom_revision)
      # If a file has been added, print out the diff.
      do_diff(nil, path)

      ['_', ' ', nil]
    end

    # Handle files
    def open_file(path, parent_baton, base_revision)
      ['_', ' ', path]
    end

    # If a file is changed, print out the diff
    def apply_textdelta(file_baton, base_checksum)
      if file_baton[2].nil?
        nil
      else
        do_diff(file_baton[2], file_baton[2])
      end
    end

    private

    # Print out a diff between two paths
    def do_diff(base_path, path)
      if base_path.nil?
        # If there's no base path, then the file
        # must have been added
        puts("Added: #{path}")
        name = path
      elsif path.nil?
        # If there's no new path, then the file
        # must have been deleted
        puts("Removed: #{base_path}")
        name = base_path
      else
        # Otherwise, the file must have been modified
        puts "Modified: #{path}"
        name = path
      end

      # Set up labels for the two files
      base_label = "#{name} (original)"
      label = "#{name} (new)"

      # Output a unified diff between the two files
      puts "=" * 78
      differ = Svn::Fs::FileDiff.new(@base_root, base_path, @root, path)
      puts differ.unified(base_label, label)
      puts
    end
  end
end

# Output usage message and exit
def usage
  messages = [
    "usage: #{$0} REPOS_PATH rev REV [COMMAND] - inspect revision REV",
    "       #{$0} REPOS_PATH txn TXN [COMMAND] - inspect transaction TXN",
    "       #{$0} REPOS_PATH [COMMAND] - inspect the youngest revision",
    "",
    "REV is a revision number > 0.",
    "TXN is a transaction name.",
    "",
    "If no command is given, the default output (which is the same as",
    "running the subcommands `info' then `tree') will be printed.",
    "",
    "COMMAND can be one of: ",
    "",
    "   author:        print author.",
    "   changed:       print full change summary: all dirs & files changed.",
    "   date:          print the timestamp (revisions only).",
    "   diff:          print GNU-style diffs of changed files and props.",
    "   dirs-changed:  print changed directories.",
    "   ids:           print the tree, with nodes ids.",
    "   info:          print the author, data, log_size, and log message.",
    "   log:           print log message.",
    "   tree:          print the tree.",
    "   uuid:          print the repository's UUID (REV and TXN ignored).",
    "   youngest:      print the youngest revision number (REV and TXN ignored).",
  ]
  puts(messages.join("\n"))
  exit(1)
end

# Output usage if necessary
if ARGV.empty?
  usage
end

# Process arguments
path = ARGV.shift
cmd = ARGV.shift
rev = nil
txn = nil

case cmd
when "rev"
  rev = Integer(ARGV.shift)
  cmd = ARGV.shift
when "txn"
  txn = ARGV.shift
  cmd = ARGV.shift
end

# If no command is specified, use the default
cmd ||= "default"

# Replace dashes in the command with underscores
cmd = cmd.gsub(/-/, '_')

# Start SvnLook with the specified command
SvnLook.new(path, rev, txn).run(cmd)
