#
# core.py: public Python interface for core components
#
# Subversion is a tool for revision control.
# See http://subversion.apache.org for more information.
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

from libsvn.core import *
import libsvn.core as _libsvncore
import atexit as _atexit
import sys

class SubversionException(Exception):

  # Python 2.6 deprecated BaseException.message, which we inadvertently use.
  # We override it here, so the users of this class are spared from
  # DeprecationWarnings.
  # Note that BaseException.message is not deprecated in Python 2.5, and
  # isn't present in all other versions.
  if sys.version_info[0:2] == (2, 6):
    message = None

  def __init__(self, message=None, apr_err=None, child=None,
               file=None, line=None):
    """Initialize a new Subversion exception object.

    Arguments:
    message     -- optional user-visible error message
    apr_err     -- optional integer error code (apr_status_t)
    child       -- optional SubversionException to wrap
    file        -- optional source file name where the error originated
    line        -- optional line number of the source file

    file and line are for C, not Python; they are redundant to the
    traceback information for exceptions raised in Python.
    """
    # Be compatible with Subversion <1.5 .args behavior:
    args = []
    if message is not None or apr_err is not None:
        args.append(message)
        if apr_err is not None:
            args.append(apr_err)
    Exception.__init__(self, *args)

    self.apr_err = apr_err
    self.message = message
    self.child = child
    self.file = file
    self.line = line

  def __str__(self):
    dump = '%d - %s' % (self.apr_err, self.message)
    if self.file != None:
      dump = dump + '\n at %s:%d' % (self.file, self.line)
    if self.child != None:
      dump = dump + '\n' + self.child.__str__()

    return dump

  @classmethod
  def _new_from_err_list(cls, errors):
    """Return new Subversion exception object from list of svn_error_t data.

    This alternative constructor is for turning a chain of svn_error_t
    objects in C into a chain of SubversionException objects in Python.
    errors is a list of (apr_err, message, file, line) tuples, in order
    from outer-most child to inner-most.

    Use svn_swig_py_svn_exception rather than calling this directly.

    Note: this modifies the errors list provided by the caller by
    reversing it.
    """
    child = None
    errors.reverse()
    for (apr_err, message, file, line) in errors:
      child = cls(message, apr_err, child, file, line)
    return child

def _cleanup_application_pool():
  """Cleanup the application pool before exiting"""
  if application_pool and application_pool.valid():
    application_pool.destroy()
_atexit.register(_cleanup_application_pool)

def _unprefix_names(symbol_dict, from_prefix, to_prefix = ''):
  for name, value in symbol_dict.items():
    if name.startswith(from_prefix):
      symbol_dict[to_prefix + name[len(from_prefix):]] = value


Pool = _libsvncore.svn_pool_create

# Setup consistent names for revnum constants
SVN_IGNORED_REVNUM = SWIG_SVN_IGNORED_REVNUM
SVN_INVALID_REVNUM = SWIG_SVN_INVALID_REVNUM

def svn_path_compare_paths(path1, path2):
  path1_len = len (path1);
  path2_len = len (path2);
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

def svn_mergeinfo_merge(mergeinfo, changes):
  return _libsvncore.svn_swig_mergeinfo_merge(mergeinfo, changes)

def svn_mergeinfo_sort(mergeinfo):
  return _libsvncore.svn_swig_mergeinfo_sort(mergeinfo)

def svn_rangelist_merge(rangelist, changes):
  return _libsvncore.svn_swig_rangelist_merge(rangelist, changes)

def svn_rangelist_reverse(rangelist):
  return _libsvncore.svn_swig_rangelist_reverse(rangelist)

class Stream:
  """A file-object-like wrapper for Subversion svn_stream_t objects."""
  def __init__(self, stream):
    self._stream = stream

  def read(self, amt=None):
    if amt is None:
      # read the rest of the stream
      chunks = [ ]
      while True:
        data = svn_stream_read(self._stream, SVN_STREAM_CHUNK_SIZE)
        if not data:
          break
        chunks.append(data)
      return ''.join(chunks)

    # read the amount specified
    return svn_stream_read(self._stream, int(amt))

  def write(self, buf):
    ### what to do with the amount written? (the result value)
    svn_stream_write(self._stream, buf)

def secs_from_timestr(svn_datetime, pool=None):
  """Convert a Subversion datetime string into seconds since the Epoch."""
  aprtime = svn_time_from_cstring(svn_datetime, pool)

  # ### convert to a time_t; this requires intimate knowledge of
  # ### the apr_time_t type
  # ### aprtime is microseconds; turn it into seconds
  return aprtime / 1000000


# ============================================================================
# Variations on this code are used in other places:
# - subversion/build/generator/gen_win.py
# - cvs2svn/cvs2svn

# Names that are not to be exported
import sys as _sys

if _sys.platform == "win32":
  import re as _re
  _escape_shell_arg_re = _re.compile(r'(\\+)(\"|$)')

  def escape_shell_arg(arg):
    # The (very strange) parsing rules used by the C runtime library are
    # described at:
    # http://msdn.microsoft.com/library/en-us/vclang/html/_pluslang_Parsing_C.2b2b_.Command.2d.Line_Arguments.asp

    # double up slashes, but only if they are followed by a quote character
    arg = _re.sub(_escape_shell_arg_re, r'\1\1\2', arg)

    # surround by quotes and escape quotes inside
    arg = '"' + arg.replace('"', '"^""') + '"'
    return arg


  def argv_to_command_string(argv):
    """Flatten a list of command line arguments into a command string.

    The resulting command string is expected to be passed to the system
    shell which os functions like popen() and system() invoke internally.
    """

    # According cmd's usage notes (cmd /?), it parses the command line by
    # "seeing if the first character is a quote character and if so, stripping
    # the leading character and removing the last quote character."
    # So to prevent the argument string from being changed we add an extra set
    # of quotes around it here.
    return '"' + " ".join(map(escape_shell_arg, argv)) + '"'

else:
  def escape_shell_arg(str):
    return "'" + str.replace("'", "'\\''") + "'"

  def argv_to_command_string(argv):
    """Flatten a list of command line arguments into a command string.

    The resulting command string is expected to be passed to the system
    shell which os functions like popen() and system() invoke internally.
    """

    return " ".join(map(escape_shell_arg, argv))
# ============================================================================
# Deprecated functions

def apr_initialize():
  """Deprecated. APR is now initialized automatically. This is
  a compatibility wrapper providing the interface of the
  Subversion 1.2.x and earlier bindings."""
  pass

def apr_terminate():
  """Deprecated. APR is now terminated automatically. This is
  a compatibility wrapper providing the interface of the
  Subversion 1.2.x and earlier bindings."""
  pass

def svn_pool_create(parent_pool=None):
  """Deprecated. Use Pool() instead. This is a compatibility
  wrapper providing the interface of the Subversion 1.2.x and
  earlier bindings."""
  return Pool(parent_pool)

def svn_pool_destroy(pool):
  """Deprecated. Pools are now destroyed automatically. If you
  want to manually destroy a pool, use Pool.destroy. This is
  a compatibility wrapper providing the interface of the
  Subversion 1.2.x and earlier bindings."""

  assert pool is not None

  # New in 1.3.x: All pools are automatically destroyed when Python shuts
  # down. For compatibility with 1.2.x, we won't report an error if your
  # app tries to destroy a pool during the shutdown process. Instead, we
  # check to make sure the application_pool is still around before calling
  # pool.destroy().
  if application_pool and application_pool.valid():
    pool.destroy()
apr_pool_destroy = svn_pool_destroy

def svn_pool_clear(pool):
  """Deprecated. Use Pool.clear instead. This is a compatibility
  wrapper providing the interface of the Subversion 1.2.x and
  earlier bindings."""

  assert pool is not None

  pool.clear()
apr_pool_clear = svn_pool_clear

def run_app(func, *args, **kw):
  '''Deprecated: Application-level pools are now created
  automatically. APR is also initialized and terminated
  automatically. This is a compatibility wrapper providing the
  interface of the Subversion 1.2.x and earlier bindings.

  Run a function as an "APR application".

  APR is initialized, and an application pool is created. Cleanup is
  performed as the function exits (normally or via an exception).
  '''
  return func(application_pool, *args, **kw)
