#
#  tree.py: tools for comparing directory trees
#
#  Subversion is a tool for revision control.
#  See http://subversion.tigris.org for more information.
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
######################################################################

import re
import os
import sys
if sys.version_info[0] >= 3:
  # Python >=3.0
  from io import StringIO
else:
  # Python <3.0
  from StringIO import StringIO
from xml.dom.minidom import parseString
import base64

import svntest

# Tree Exceptions.

# All tree exceptions should inherit from SVNTreeError
class SVNTreeError(svntest.Failure):
  "Exception raised if you screw up in the tree module."
  pass

class SVNTreeUnequal(SVNTreeError):
  "Exception raised if two trees are unequal."
  pass

class SVNTypeMismatch(SVNTreeError):
  "Exception raised if one node is file and other is dir"
  pass

#========================================================================

# ===>  Overview of our Datastructures  <===

# The general idea here is that many, many things can be represented by
# a tree structure:

#   - a working copy's structure and contents
#   - the output of 'svn status'
#   - the output of 'svn checkout/update'
#   - the output of 'svn commit'

# The idea is that a test function creates a "expected" tree of some
# kind, and is then able to compare it to an "actual" tree that comes
# from running the Subversion client.  This is what makes a test
# automated; if an actual and expected tree match exactly, then the test
# has passed.  (See compare_trees() below.)

# The SVNTreeNode class is the fundamental data type used to build tree
# structures.  The class contains a method for "dropping" a new node
# into an ever-growing tree structure. (See also create_from_path()).

# We have four parsers in this file for the four use cases listed above:
# each parser examines some kind of input and returns a tree of
# SVNTreeNode objects.  (See build_tree_from_checkout(),
# build_tree_from_commit(), build_tree_from_status(), and
# build_tree_from_wc()).  These trees are the "actual" trees that result
# from running the Subversion client.

# Also necessary, of course, is a convenient way for a test to create an
# "expected" tree.  The test *could* manually construct and link a bunch
# of SVNTreeNodes, certainly.  But instead, all the tests are using the
# build_generic_tree() routine instead.

# build_generic_tree() takes a specially-formatted list of lists as
# input, and returns a tree of SVNTreeNodes.  The list of lists has this
# structure:

#   [ ['/full/path/to/item', 'text contents', {prop-hash}, {att-hash}],
#     [...],
#     [...],
#     ...   ]

# You can see that each item in the list essentially defines an
# SVNTreeNode.  build_generic_tree() instantiates a SVNTreeNode for each
# item, and then drops it into a tree by parsing each item's full path.

# So a typical test routine spends most of its time preparing lists of
# this format and sending them to build_generic_tree(), rather than
# building the "expected" trees directly.

#   ### Note: in the future, we'd like to remove this extra layer of
#   ### abstraction.  We'd like the SVNTreeNode class to be more
#   ### directly programmer-friendly, providing a number of accessor
#   ### routines, so that tests can construct trees directly.

# The first three fields of each list-item are self-explanatory.  It's
# the fourth field, the "attribute" hash, that needs some explanation.
# The att-hash is used to place extra information about the node itself,
# depending on the parsing context:

#   - in the 'svn co/up' use-case, each line of output starts with two
#     characters from the set of (A, D, G, U, C, _) or 'Restored'.  The
#     status code is stored in a attribute named 'status'.  In the case
#     of a restored file, the word 'Restored' is stored in an attribute
#     named 'verb'.

#   - in the 'svn ci/im' use-case, each line of output starts with one
#      of the words (Adding, Deleting, Sending).  This verb is stored in
#      an attribute named 'verb'.

#   - in the 'svn status' use-case (which is always run with the -v
#     (--verbose) flag), each line of output contains a working revision
#     number and a two-letter status code similar to the 'svn co/up'
#     case.  This information is stored in attributes named 'wc_rev'
#     and 'status'.  The repository revision is also printed, but it
#     is ignored.

#   - in the working-copy use-case, the att-hash is ignored.


# Finally, one last explanation: the file 'actions.py' contain a number
# of helper routines named 'run_and_verify_FOO'.  These routines take
# one or more "expected" trees as input, then run some svn subcommand,
# then push the output through an appropriate parser to derive an
# "actual" tree.  Then it runs compare_trees() and raises an exception
# on failure.  This is why most tests typically end with a call to
# run_and_verify_FOO().



#========================================================================

# A node in a tree.
#
# If CHILDREN is None, then the node is a file.  Otherwise, CHILDREN
# is a list of the nodes making up that directory's children.
#
# NAME is simply the name of the file or directory.  CONTENTS is a
# string that contains the file's contents (if a file), PROPS are
# properties attached to files or dirs, and ATTS is a dictionary of
# other metadata attached to the node.

class SVNTreeNode:

  def __init__(self, name, children=None, contents=None, props={}, atts={}):
    self.name = name
    self.children = children
    self.contents = contents
    self.props = props
    self.atts = atts
    self.path = name

# TODO: Check to make sure contents and children are mutually exclusive

  def add_child(self, newchild):
    child_already_exists = 0
    if self.children is None:  # if you're a file,
      self.children = []     # become an empty dir.
    else:
      for a in self.children:
        if a.name == newchild.name:
          child_already_exists = 1
          break

    if child_already_exists:
      if newchild.children is None:
        # this is the 'end' of the chain, so copy any content here.
        a.contents = newchild.contents
        a.props = newchild.props
        a.atts = newchild.atts
        a.path = os.path.join(self.path, newchild.name)
      else:
        # try to add dangling children to your matching node
        for i in newchild.children:
          a.add_child(i)
    else:
      self.children.append(newchild)
      newchild.path = os.path.join(self.path, newchild.name)


  def pprint(self, stream = sys.stdout):
    "Pretty-print the meta data for this node to STREAM."
    stream.write(" * Node name:   %s\n" % self.name)
    stream.write("    Path:       %s\n" % self.path)
    mime_type = self.props.get("svn:mime-type")
    if not mime_type or mime_type.startswith("text/"):
      if self.children is not None:
        stream.write("    Contents:   N/A (node is a directory)\n")
      else:
        stream.write("    Contents:   %s\n" % self.contents)
    else:
      stream.write("    Contents:   %d bytes (binary)\n" % len(self.contents))
    stream.write("    Properties: %s\n" % self.props)
    stream.write("    Attributes: %s\n" % self.atts)
    ### FIXME: I'd like to be able to tell the difference between
    ### self.children is None (file) and self.children == [] (empty
    ### directory), but it seems that most places that construct
    ### SVNTreeNode objects don't even try to do that.  --xbc
    ###
    ### See issue #1611 about this problem.  -kfogel
    if self.children is not None:
      stream.write("    Children:   %s\n" % len(self.children))
    else:
      stream.write("    Children:  None (node is probably a file)\n")
    stream.flush()

  def get_printable_path(self):
    """Remove some occurrences of root_node_name = "__SVN_ROOT_NODE",
    it is in the way when matching for a subtree, and looks bad."""
    path = self.path
    if path.startswith(root_node_name + os.sep):
      path = path[len(root_node_name + os.sep):]
    return path

  def print_script(self, stream = sys.stdout, subtree = "", prepend="\n  ",
                   drop_empties = True):
    """Python-script-print the meta data for this node to STREAM.
    Print only those nodes whose path string starts with the string SUBTREE,
    and print only the part of the path string that remains after SUBTREE.
    PREPEND is a string prepended to each node printout (does the line
    feed if desired, don't include a comma in PREPEND).
    If DROP_EMPTIES is true, all dir nodes that have no data set in them
    (no props, no atts) and that have children (so they are included
    implicitly anyway) are not printed.
    Return 1 if this node was printed, 0 otherwise (added up by
    dump_tree_script())"""

    # figure out if this node would be obsolete to print.
    if drop_empties and len(self.props) < 1 and len(self.atts) < 1 and \
       self.contents is None and self.children is not None:
      return 0

    path = self.get_printable_path()

    # remove the subtree path, skip this node if necessary.
    if path.startswith(subtree):
      path = path[len(subtree):]
    else:
      return 0

    if path.startswith(os.sep):
      path = path[1:]

    line = prepend
    line += "%-20s: Item(" % ("'%s'" % path.replace(os.sep, '/'))
    comma = False

    mime_type = self.props.get("svn:mime-type")
    if not mime_type or mime_type.startswith("text/"):
      if self.contents is not None:
        # Escape some characters for nicer script and readability.
        # (This is error output. I guess speed is no consideration here.)
        line += "contents=\"%s\"" % (self.contents
                                     .replace('\n','\\n')
                                     .replace('"','\\"')
                                     .replace('\r','\\r')
                                     .replace('\t','\\t'))
        comma = True
    else:
      line += 'content is binary data'
      comma = True

    if self.props:
      if comma:
        line += ", "
      line += "props={"
      comma = False

      for name in self.props:
        if comma:
          line += ", "
        line += "'%s':'%s'" % (name, self.props[name])
        comma = True

      line += "}"
      comma = True

    for name in self.atts:
      if comma:
        line += ", "
      line += "%s='%s'" % (name, self.atts[name])
      comma = True

    line += "),"
    stream.write("%s" % line)
    stream.flush()
    return 1


  def __str__(self):
    s = StringIO()
    self.pprint(s)
    return s.getvalue()


  def __cmp__(self, other):
    """Define a simple ordering of two nodes without regard to their full
    path (i.e. position in the tree). This can be used for sorting the
    children within a directory."""
    return cmp(self.name, other.name)

  def as_state(self, prefix=None):
    """Return an svntest.wc.State instance that is equivalent to this tree."""
    root = self
    if self.path == root_node_name:
      assert prefix is None
      wc_dir = ''
      while True:
        if root is not self:  # don't prepend ROOT_NODE_NAME
          wc_dir = os.path.join(wc_dir, root.name)
        if root.contents or root.props or root.atts:
          break
        if not root.children or len(root.children) != 1:
          break
        root = root.children[0]
      state = svntest.wc.State(wc_dir, { })
      if root.contents or root.props or root.atts:
        state.add({'': root.as_item()})
      prefix = wc_dir
    else:
      assert prefix is not None

      path = self.path
      if path.startswith(root_node_name):
        path = path[len(root_node_name)+1:]
      # prefix should only be set on a recursion, which means a child,
      # which means this path better not be the same as the prefix.
      assert path != prefix, 'not processing a child of the root'
      l = len(prefix)
      if l > 0:
        assert path[:l] == prefix, \
            '"%s" is not a prefix of "%s"' % (prefix, path)
        # return the portion after the separator
        path = path[l+1:].replace(os.sep, '/')

      state = svntest.wc.State('', {
          path: self.as_item()
          })

    if root.children:
      for child in root.children:
        state.add_state('', child.as_state(prefix))

    return state

  def as_item(self):
    return svntest.wc.StateItem(self.contents,
                                self.props,
                                self.atts.get('status'),
                                self.atts.get('verb'),
                                self.atts.get('wc_rev'),
                                self.atts.get('locked'),
                                self.atts.get('copied'),
                                self.atts.get('switched'),
                                self.atts.get('writelocked'),
                                self.atts.get('treeconflict'))

  def recurse(self, function):
    results = []
    results += [ function(self) ]
    if self.children:
      for child in self.children:
        results += child.recurse(function)
    return results

  def find_node(self, path):
    if self.get_printable_path() == path:
      return self
    if self.children:
      for child in self.children:
        result = child.find_node(path)
        if result:
          return result
    return None

# reserved name of the root of the tree
root_node_name = "__SVN_ROOT_NODE"


# helper func
def add_elements_as_path(top_node, element_list):
  """Add the elements in ELEMENT_LIST as if they were a single path
  below TOP_NODE."""

  # The idea of this function is to take a list like so:
  # ['A', 'B', 'C'] and a top node, say 'Z', and generate a tree
  # like this:
  #
  #             Z -> A -> B -> C
  #
  # where 1 -> 2 means 2 is a child of 1.
  #

  prev_node = top_node
  for i in element_list:
    new_node = SVNTreeNode(i, None)
    prev_node.add_child(new_node)
    prev_node = new_node


# Helper for compare_trees
def compare_file_nodes(a, b):
  """Compare two nodes, A (actual) and B (expected). Compare their names,
  contents, properties and attributes, ignoring children.  Return 0 if the
  same, 1 otherwise."""
  if a.name != b.name:
    return 1
  if a.contents != b.contents:
    return 1
  if a.props != b.props:
    return 1
  if a.atts == b.atts:
    # No fixes necessary
    return 0

  # Fix a pre-WC-NG assumptions in our testsuite
  if (b.atts == {'status': 'A ', 'wc_rev': '0'})     \
     and (a.atts == {'status': 'A ', 'wc_rev': '-'}):
    return 0
  return 1

# Helper for compare_trees
def compare_dir_nodes(a, b):
  """Compare two nodes, A (actual) and B (expected). Compare their names,
  properties and attributes, ignoring children.  Return 0 if the
  same, 1 otherwise."""
  if a.name != b.name:
    return 1
  if (a.props != b.props):
    return 1
  if (a.atts == b.atts):
    # No fixes necessary
    return 0

  # Fix a pre-WC-NG assumptions in our testsuite
  if (b.atts == {'status': 'A ', 'wc_rev': '0'})     \
     and (a.atts == {'status': 'A ', 'wc_rev': '-'}):
    return 0
  return 1


# Internal utility used by most build_tree_from_foo() routines.
#
# (Take the output and .add_child() it to a root node.)

def create_from_path(path, contents=None, props={}, atts={}):
  """Create and return a linked list of treenodes, given a PATH
  representing a single entry into that tree.  CONTENTS and PROPS are
  optional arguments that will be deposited in the tail node."""

  # get a list of all the names in the path
  # each of these will be a child of the former
  if os.sep != "/":
    path = path.replace(os.sep, "/")
  elements = path.split("/")
  if len(elements) == 0:
    ### we should raise a less generic error here. which?
    raise SVNTreeError

  root_node = None

  # if this is Windows: if the path contains a drive name (X:), make it
  # the root node.
  if os.name == 'nt':
    m = re.match("([a-zA-Z]:)(.+)", elements[0])
    if m:
      root_node = SVNTreeNode(m.group(1), None)
      elements[0] = m.group(2)
      add_elements_as_path(root_node, elements[0:])

  if not root_node:
    root_node = SVNTreeNode(elements[0], None)
    add_elements_as_path(root_node, elements[1:])

  # deposit contents in the very last node.
  node = root_node
  while True:
    if node.children is None:
      node.contents = contents
      node.props = props
      node.atts = atts
      break
    node = node.children[0]

  return root_node


eol_re = re.compile(r'(\r\n|\r)')

# helper for build_tree_from_wc()
def get_props(paths):
  """Return a hash of hashes of props for PATHS, using the svn client. Convert
     each embedded end-of-line to a single LF character."""

  # It's not kosher to look inside .svn/ and try to read the internal
  # property storage format.  Instead, we use 'svn proplist'.  After
  # all, this is the only way the user can retrieve them, so we're
  # respecting the black-box paradigm.

  files = {}
  exit_code, output, errput = svntest.main.run_svn(1,
                                                   "proplist",
                                                   "--verbose",
                                                   "--xml",
                                                   *paths)

  output = (line for line in output if not line.startswith('DBG:'))
  dom = parseString(''.join(output))
  target_nodes = dom.getElementsByTagName('target')
  for target_node in target_nodes:
    filename = target_node.attributes['path'].nodeValue
    file_props = {}
    for property_node in target_node.getElementsByTagName('property'):
      name = property_node.attributes['name'].nodeValue
      if property_node.hasChildNodes():
        text_node = property_node.firstChild
        value = text_node.nodeValue
      else:
        value = ''
      try:
        encoding = property_node.attributes['encoding'].nodeValue
        if encoding == 'base64':
          value = base64.b64decode(value)
        else:
          raise Exception("Unknown encoding '%s' for file '%s' property '%s'"
                          % (encoding, filename, name,))
      except KeyError:
        pass
      # If the property value contained a CR, or if under Windows an
      # "svn:*" property contains a newline, then the XML output
      # contains a CR character XML-encoded as '&#13;'.  The XML
      # parser converts it back into a CR character.  So again convert
      # all end-of-line variants into a single LF:
      value = eol_re.sub('\n', value)
      file_props[name] = value
    files[filename] = file_props

  dom.unlink()
  return files


### ridiculous function. callers should do this one line themselves.
def get_text(path):
  "Return a string with the textual contents of a file at PATH."

  # sanity check
  if not os.path.isfile(path):
    return None

  return open(path, 'r').read()


def get_child(node, name):
  """If SVNTreeNode NODE contains a child named NAME, return child;
  else, return None. If SVNTreeNode is not a directory, exit completely."""
  if node.children == None:
    print("Error: Foolish call to get_child.")
    sys.exit(1)
  for n in node.children:
    if name == n.name:
      return n
  return None


# Helper for compare_trees
def default_singleton_handler(node, description):
  """Print SVNTreeNode NODE's name, describing it with the string
  DESCRIPTION, then raise SVNTreeUnequal."""
  print("Couldn't find node '%s' in %s tree" % (node.name, description))
  node.pprint()
  raise SVNTreeUnequal

# A test helper function implementing the singleton_handler_a API.
def detect_conflict_files(node, extra_files):
  """NODE has been discovered, an extra file on disk.  Verify that it
  matches one of the regular expressions in the EXTRA_FILES list.  If
  it matches, remove the match from the list.  If it doesn't match,
  raise an exception."""

  for pattern in extra_files:
    mo = re.match(pattern, node.name)
    if mo:
      extra_files.pop(extra_files.index(pattern)) # delete pattern from list
      break
  else:
    msg = "Encountered unexpected disk path '" + node.name + "'"
    print(msg)
    node.pprint()
    raise SVNTreeUnequal(msg)

###########################################################################
###########################################################################
# EXPORTED ROUTINES ARE BELOW


# Main tree comparison routine!

def compare_trees(label,
                  a, b,
                  singleton_handler_a = None,
                  a_baton = None,
                  singleton_handler_b = None,
                  b_baton = None):
  """Compare SVNTreeNodes A (actual) and B (expected), expressing
  differences using FUNC_A and FUNC_B.  FUNC_A and FUNC_B are
  functions of two arguments (a SVNTreeNode and a context baton), and
  may raise exception SVNTreeUnequal, in which case they use the
  string LABEL to describe the error (their return value is ignored).
  LABEL is typically "output", "disk", "status", or some other word
  that labels the trees being compared.

  If A and B are both files, then return if their contents,
  properties, and names are all the same; else raise a SVNTreeUnequal.
  If A is a file and B is a directory, raise a SVNTreeUnequal; same
  vice-versa.  If both are directories, then for each entry that
  exists in both, call compare_trees on the two entries; otherwise, if
  the entry exists only in A, invoke FUNC_A on it, and likewise for
  B with FUNC_B."""

  def display_nodes(a, b):
    'Display two nodes, expected and actual.'
    print("=============================================================")
    print("Expected '%s' and actual '%s' in %s tree are different!"
          % (b.name, a.name, label))
    print("=============================================================")
    print("EXPECTED NODE TO BE:")
    print("=============================================================")
    b.pprint()
    print("=============================================================")
    print("ACTUAL NODE FOUND:")
    print("=============================================================")
    a.pprint()

  # Setup singleton handlers
  if singleton_handler_a is None:
    singleton_handler_a = default_singleton_handler
    a_baton = "expected " + label
  if singleton_handler_b is None:
    singleton_handler_b = default_singleton_handler
    b_baton = "actual " + label

  try:
    # A and B are both files.
    if (a.children is None) and (b.children is None):
      if compare_file_nodes(a, b):
        display_nodes(a, b)
        raise SVNTreeUnequal
    # One is a file, one is a directory.
    elif (((a.children is None) and (b.children is not None))
          or ((a.children is not None) and (b.children is None))):
      display_nodes(a, b)
      raise SVNTypeMismatch
    # They're both directories.
    else:
      if compare_dir_nodes(a, b):
        display_nodes(a, b)
        raise SVNTreeUnequal

      accounted_for = []
      # For each child of A, check and see if it's in B.  If so, run
      # compare_trees on the two children and add b's child to
      # accounted_for.  If not, run FUNC_A on the child.  Next, for each
      # child of B, check and see if it's in accounted_for.  If it is,
      # do nothing. If not, run FUNC_B on it.
      for a_child in a.children:
        b_child = get_child(b, a_child.name)
        if b_child:
          accounted_for.append(b_child)
          compare_trees(label, a_child, b_child,
                        singleton_handler_a, a_baton,
                        singleton_handler_b, b_baton)
        else:
          singleton_handler_a(a_child, a_baton)
      for b_child in b.children:
        if b_child not in accounted_for:
          singleton_handler_b(b_child, b_baton)
  except SVNTypeMismatch:
    print('Unequal Types: one Node is a file, the other is a directory')
    raise SVNTreeUnequal
  except IndexError:
    print("Error: unequal number of children")
    raise SVNTreeUnequal
  except SVNTreeUnequal:
    if a.name != root_node_name:
      print("Unequal at node %s" % a.name)
    raise



# Visually show a tree's structure

def dump_tree(n,indent=""):
  """Print out a nice representation of the structure of the tree in
  the SVNTreeNode N. Prefix each line with the string INDENT."""

  # Code partially stolen from Dave Beazley
  tmp_children = sorted(n.children or [])

  if n.name == root_node_name:
    print("%s%s" % (indent, "ROOT"))
  else:
    print("%s%s" % (indent, n.name))

  indent = indent.replace("-", " ")
  indent = indent.replace("+", " ")
  for i in range(len(tmp_children)):
    c = tmp_children[i]
    if i == len(tmp_children)-1:
      dump_tree(c,indent + "  +-- ")
    else:
      dump_tree(c,indent + "  |-- ")


def dump_tree_script__crawler(n, subtree="", stream=sys.stdout):
  "Helper for dump_tree_script. See that comment."
  count = 0

  # skip printing the root node.
  if n.name != root_node_name:
    count += n.print_script(stream, subtree)

  for child in n.children or []:
    count += dump_tree_script__crawler(child, subtree, stream)

  return count


def dump_tree_script(n, subtree="", stream=sys.stdout, wc_varname='wc_dir'):
  """Print out a python script representation of the structure of the tree
  in the SVNTreeNode N. Print only those nodes whose path string starts
  with the string SUBTREE, and print only the part of the path string
  that remains after SUBTREE.
  The result is printed to STREAM.
  The WC_VARNAME is inserted in the svntest.wc.State(wc_dir,{}) call
  that is printed out (this is used by factory.py)."""

  stream.write("svntest.wc.State(" + wc_varname + ", {")
  count = dump_tree_script__crawler(n, subtree, stream)
  if count > 0:
    stream.write('\n')
  stream.write("})")


###################################################################
###################################################################
# PARSERS that return trees made of SVNTreeNodes....


###################################################################
# Build an "expected" static tree from a list of lists


# Create a list of lists, of the form:
#
#  [ [path, contents, props, atts], ... ]
#
#  and run it through this parser.  PATH is a string, a path to the
#  object.  CONTENTS is either a string or None, and PROPS and ATTS are
#  populated dictionaries or {}.  Each CONTENTS/PROPS/ATTS will be
#  attached to the basename-node of the associated PATH.

def build_generic_tree(nodelist):
  "Given a list of lists of a specific format, return a tree."

  root = SVNTreeNode(root_node_name)

  for list in nodelist:
    new_branch = create_from_path(list[0], list[1], list[2], list[3])
    root.add_child(new_branch)

  return root


####################################################################
# Build trees from different kinds of subcommand output.


# Parse co/up output into a tree.
#
#   Tree nodes will contain no contents, a 'status' att, and a
#   'treeconflict' att.

def build_tree_from_checkout(lines, include_skipped=True):
  "Return a tree derived by parsing the output LINES from 'co' or 'up'."

  return svntest.wc.State.from_checkout(lines, include_skipped).old_tree()


# Parse ci/im output into a tree.
#
#   Tree nodes will contain no contents, and only one 'verb' att.

def build_tree_from_commit(lines):
  "Return a tree derived by parsing the output LINES from 'ci' or 'im'."

  return svntest.wc.State.from_commit(lines).old_tree()


# Parse status output into a tree.
#
#   Tree nodes will contain no contents, and these atts:
#
#          'status', 'wc_rev',
#             ... and possibly 'locked', 'copied', 'switched',
#             'writelocked' and 'treeconflict',
#             IFF columns non-empty.
#

def build_tree_from_status(lines):
  "Return a tree derived by parsing the output LINES from 'st -vuq'."

  return svntest.wc.State.from_status(lines).old_tree()


# Parse merge "skipped" output

def build_tree_from_skipped(lines):

  return svntest.wc.State.from_skipped(lines).old_tree()


def build_tree_from_diff_summarize(lines):
  "Build a tree from output of diff --summarize"

  return svntest.wc.State.from_summarize(lines).old_tree()


####################################################################
# Build trees by looking at the working copy


#   The reason the 'load_props' flag is off by default is because it
#   creates a drastic slowdown -- we spawn a new 'svn proplist'
#   process for every file and dir in the working copy!


def build_tree_from_wc(wc_path, load_props=0, ignore_svn=1):
    """Takes WC_PATH as the path to a working copy.  Walks the tree below
    that path, and creates the tree based on the actual found
    files.  If IGNORE_SVN is true, then exclude SVN admin dirs from the tree.
    If LOAD_PROPS is true, the props will be added to the tree."""

    return svntest.wc.State.from_wc(wc_path, load_props, ignore_svn).old_tree()
