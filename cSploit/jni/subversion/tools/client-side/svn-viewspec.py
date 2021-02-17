#!/usr/bin/env python
#
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

"""\
Usage: 1. __SCRIPTNAME__ checkout VIEWSPEC-FILE TARGET-DIR
       2. __SCRIPTNAME__ examine VIEWSPEC-FILE
       3. __SCRIPTNAME__ help
       4. __SCRIPTNAME__ help-format

VIEWSPEC-FILE is the path of a file whose contents describe a
Subversion sparse checkouts layout, or '-' if that description should
be read from stdin.  TARGET-DIR is the working copy directory created
by this script as it checks out the specified layout.

1. Parse VIEWSPEC-FILE and execute the necessary 'svn' command-line
   operations to build out a working copy tree at TARGET-DIR.

2. Parse VIEWSPEC-FILE and dump out a human-readable representation of
   the tree described in the specification.

3. Show this usage message.

4. Show information about the file format this program expects.

"""

FORMAT_HELP = """\
Viewspec File Format
====================

The viewspec file format used by this tool is a collection of headers
(using the typical one-per-line name:value syntax), followed by an
empty line, followed by a set of one-per-line rules.

The headers must contain at least the following:

    Format   - version of the viewspec format used throughout the file
    Url      - base URL applied to all rules; tree checkout location

The following headers are optional:

    Revision - version of the tree items to checkout

Following the headers and blank line separator are the path rules.
The rules are list of URLs -- relative to the base URL stated in the
headers -- with optional annotations to specify the desired working
copy depth of each item:

    PATH/**  - checkout PATH and all its children to infinite depth
    PATH/*   - checkout PATH and its immediate children
    PATH/~   - checkout PATH and its file children
    PATH     - checkout PATH non-recursively

By default, the top-level directory (associated with the base URL) is
checked out with empty depth.  You can override this using the special
rules '**', '*', and '~' as appropriate.

It is not necessary to explicitly list the parent directories of each
path associated with a rule.  If the parent directory of a given path
is not "covered" by a previous rule, it will be checked out with empty
depth.

Examples
========

Here's a sample viewspec file:

    Format: 1
    Url: http://svn.apache.org/repos/asf/subversion
    Revision: 36366

    trunk/**
    branches/1.5.x/**
    branches/1.6.x/**
    README
    branches/1.4.x/STATUS
    branches/1.4.x/subversion/tests/cmdline/~

You may wish to version your viewspec files.  If so, you can use this
script in conjunction with 'svn cat' to fetch, parse, and act on a
versioned viewspec file:

    $ svn cat http://svn.example.com/specs/dev-spec.txt |
         __SCRIPTNAME__ checkout - /path/to/target/directory

"""

#########################################################################
###  Possible future improvements that could be made:
###
###    - support for excluded paths (PATH!)
###    - support for static revisions of individual paths (PATH@REV/**)
###

import sys
import os
import urllib

DEPTH_EMPTY      = 'empty'
DEPTH_FILES      = 'files'
DEPTH_IMMEDIATES = 'immediates'
DEPTH_INFINITY   = 'infinity'


class TreeNode:
    """A representation of a single node in a Subversion sparse
    checkout tree."""

    def __init__(self, name, depth):
        self.name = name    # the basename of this tree item
        self.depth = depth  # its depth (one of the DEPTH_* values)
        self.children = {}  # its children (basename -> TreeNode)

    def add_child(self, child_node):
        child_name = child_node.name
        assert not self.children.has_key(child_node)
        self.children[child_name] = child_node

    def dump(self, recurse=False, indent=0):
        sys.stderr.write(" " * indent)
        sys.stderr.write("Path: %s (depth=%s)\n" % (self.name, self.depth))
        if recurse:
            child_names = self.children.keys()
            child_names.sort(svn_path_compare_paths)
            for child_name in child_names:
                self.children[child_name].dump(recurse, indent + 2)

class SubversionViewspec:
    """A representation of a Subversion sparse checkout specification."""

    def __init__(self, base_url, revision, tree):
        self.base_url = base_url  # base URL of the checkout
        self.revision = revision  # revision of the checkout (-1 == HEAD)
        self.tree = tree          # the top-most TreeNode item

def svn_path_compare_paths(path1, path2):
    """Compare PATH1 and PATH2 as paths, sorting depth-first-ily.

    NOTE: Stolen unapologetically from Subversion's Python bindings
    module svn.core."""

    path1_len = len(path1);
    path2_len = len(path2);
    min_len = min(path1_len, path2_len)
    i = 0

    # Are the paths exactly the same?
    if path1 == path2:
        return 0

    # Skip past common prefix
    while (i < min_len) and (path1[i] == path2[i]):
        i = i + 1

    # Children of paths are greater than their parents, but less than
    # greater siblings of their parents
    char1 = '\0'
    char2 = '\0'
    if (i < path1_len):
        char1 = path1[i]
    if (i < path2_len):
        char2 = path2[i]

    if (char1 == '/') and (i == path2_len):
        return 1
    if (char2 == '/') and (i == path1_len):
        return -1
    if (i < path1_len) and (char1 == '/'):
        return -1
    if (i < path2_len) and (char2 == '/'):
        return 1

    # Common prefix was skipped above, next character is compared to
    # determine order
    return cmp(char1, char2)

def parse_viewspec_headers(viewspec_fp):
    """Parse the headers from the viewspec file, return them as a
    dictionary mapping header names to values."""

    headers = {}
    while 1:
        line = viewspec_fp.readline().strip()
        if not line:
            break
        name, value = [x.strip() for x in line.split(':', 1)]
        headers[name] = value
    return headers

def parse_viewspec(viewspec_fp):
    """Parse the viewspec file, returning a SubversionViewspec object
    that represents the specification."""

    headers = parse_viewspec_headers(viewspec_fp)
    format = headers['Format']
    assert format == '1'
    base_url = headers['Url']
    revision = int(headers.get('Revision', -1))
    root_depth = DEPTH_EMPTY
    rules = {}
    while 1:
        line = viewspec_fp.readline()
        if not line:
            break
        line = line.rstrip()

        # These are special rules for the top-most dir; don't fall thru.
        if line == '**':
            root_depth = DEPTH_INFINITY
            continue
        elif line == '*':
            root_depth = DEPTH_IMMEDIATES
            continue
        elif line == '~':
            root_depth = DEPTH_FILES
            continue

        # These are the regular per-path rules.
        elif line[-3:] == '/**':
            depth = DEPTH_INFINITY
            path = line[:-3]
        elif line[-2:] == '/*':
            depth = DEPTH_IMMEDIATES
            path = line[:-2]
        elif line[-2:] == '/~':
            depth = DEPTH_FILES
            path = line[:-2]
        else:
            depth = DEPTH_EMPTY
            path = line

        # Add our rule to the set thereof.
        assert not rules.has_key(path)
        rules[path] = depth

    tree = TreeNode('', root_depth)
    paths = rules.keys()
    paths.sort(svn_path_compare_paths)
    for path in paths:
        depth = rules[path]
        path_parts = filter(None, path.split('/'))
        tree_ptr = tree
        for part in path_parts[:-1]:
            child_node = tree_ptr.children.get(part, None)
            if not child_node:
                child_node = TreeNode(part, DEPTH_EMPTY)
                tree_ptr.add_child(child_node)
            tree_ptr = child_node
        tree_ptr.add_child(TreeNode(path_parts[-1], depth))
    return SubversionViewspec(base_url, revision, tree)

def checkout_tree(base_url, revision, tree_node, target_dir, is_top=True):
    """Checkout from BASE_URL, and into TARGET_DIR, the TREE_NODE
    sparse checkout item.  IS_TOP is set iff this node represents the
    root of the checkout tree.  REVISION is the revision to checkout,
    or -1 if checking out HEAD."""

    depth = tree_node.depth
    revision_str = ''
    if revision != -1:
        revision_str = "--revision=%d " % (revision)
    if is_top:
        os.system('svn checkout "%s" "%s" --depth=%s %s'
                  % (base_url, target_dir, depth, revision_str))
    else:
        os.system('svn update "%s" --set-depth=%s %s'
                  % (target_dir, depth, revision_str))
    child_names = tree_node.children.keys()
    child_names.sort(svn_path_compare_paths)
    for child_name in child_names:
        checkout_tree(base_url + '/' + child_name,
                      revision,
                      tree_node.children[child_name],
                      os.path.join(target_dir, urllib.unquote(child_name)),
                      False)

def checkout_spec(viewspec, target_dir):
    """Checkout the view specification VIEWSPEC into TARGET_DIR."""

    checkout_tree(viewspec.base_url,
                  viewspec.revision,
                  viewspec.tree,
                  target_dir)

def usage_and_exit(errmsg=None):
    stream = errmsg and sys.stderr or sys.stdout
    msg = __doc__.replace("__SCRIPTNAME__", os.path.basename(sys.argv[0]))
    stream.write(msg)
    if errmsg:
        stream.write("ERROR: %s\n" % (errmsg))
    sys.exit(errmsg and 1 or 0)

def main():
    argc = len(sys.argv)
    if argc < 2:
        usage_and_exit('Not enough arguments.')
    subcommand = sys.argv[1]
    if subcommand == 'help':
        usage_and_exit()
    elif subcommand == 'help-format':
        msg = FORMAT_HELP.replace("__SCRIPTNAME__",
                                  os.path.basename(sys.argv[0]))
        sys.stdout.write(msg)
    elif subcommand == 'examine':
        if argc < 3:
            usage_and_exit('No viewspec file specified.')
        fp = (sys.argv[2] == '-') and sys.stdin or open(sys.argv[2], 'r')
        viewspec = parse_viewspec(fp)
        sys.stdout.write("Url: %s\n" % (viewspec.base_url))
        revision = viewspec.revision
        if revision != -1:
            sys.stdout.write("Revision: %s\n" % (revision))
        else:
            sys.stdout.write("Revision: HEAD\n")
        sys.stdout.write("\n")
        viewspec.tree.dump(True)
    elif subcommand == 'checkout':
        if argc < 3:
            usage_and_exit('No viewspec file specified.')
        if argc < 4:
            usage_and_exit('No target directory specified.')
        fp = (sys.argv[2] == '-') and sys.stdin or open(sys.argv[2], 'r')
        checkout_spec(parse_viewspec(fp), sys.argv[3])
    else:
        usage_and_exit('Unknown subcommand "%s".' % (subcommand))

if __name__ == "__main__":
    main()
