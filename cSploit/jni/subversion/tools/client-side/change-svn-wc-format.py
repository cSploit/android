#!/usr/bin/env python
#
# change-svn-wc-format.py: Change the format of a Subversion working copy.
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

import sys
import os
import getopt
try:
  my_getopt = getopt.gnu_getopt
except AttributeError:
  my_getopt = getopt.getopt

### The entries file parser in subversion/tests/cmdline/svntest/entry.py
### handles the XML-based WC entries file format used by Subversion
### 1.3 and lower.  It could be rolled into this script.

LATEST_FORMATS = { "1.4" : 8,
                   "1.5" : 9,
                   "1.6" : 10,
                   # Do NOT add format 11 here.  See comment in must_retain_fields
                   # for why.
                 }

def usage_and_exit(error_msg=None):
  """Write usage information and exit.  If ERROR_MSG is provide, that
  error message is printed first (to stderr), the usage info goes to
  stderr, and the script exits with a non-zero status.  Otherwise,
  usage info goes to stdout and the script exits with a zero status."""
  progname = os.path.basename(sys.argv[0])

  stream = error_msg and sys.stderr or sys.stdout
  if error_msg:
    stream.write("ERROR: %s\n\n" % error_msg)
  stream.write("""\
usage: %s WC_PATH SVN_VERSION [--verbose] [--force] [--skip-unknown-format]
       %s --help

Change the format of a Subversion working copy to that of SVN_VERSION.

  --skip-unknown-format    : skip directories with unknown working copy
                             format and continue the update

""" % (progname, progname))
  stream.flush()
  sys.exit(error_msg and 1 or 0)

def get_adm_dir():
  """Return the name of Subversion's administrative directory,
  adjusted for the SVN_ASP_DOT_NET_HACK environment variable.  See
  <http://svn.apache.org/repos/asf/subversion/trunk/notes/asp-dot-net-hack.txt>
  for details."""
  return "SVN_ASP_DOT_NET_HACK" in os.environ and "_svn" or ".svn"

class WCFormatConverter:
  "Performs WC format conversions."
  root_path = None
  error_on_unrecognized = True
  force = False
  verbosity = 0

  def write_dir_format(self, format_nbr, dirname, paths):
    """Attempt to write the WC format FORMAT_NBR to the entries file
    for DIRNAME.  Throws LossyConversionException when not in --force
    mode, and unconvertable WC data is encountered."""

    # Avoid iterating in unversioned directories.
    if not (get_adm_dir() in paths):
      del paths[:]
      return

    # Process the entries file for this versioned directory.
    if self.verbosity:
      print("Processing directory '%s'" % dirname)
    entries = Entries(os.path.join(dirname, get_adm_dir(), "entries"))
    entries_parsed = True
    if self.verbosity:
      print("Parsing file '%s'" % entries.path)
    try:
      entries.parse(self.verbosity)
    except UnrecognizedWCFormatException, e:
      if self.error_on_unrecognized:
        raise
      sys.stderr.write("%s, skipping\n" % e)
      sys.stderr.flush()
      entries_parsed = False

    if entries_parsed:
      format = Format(os.path.join(dirname, get_adm_dir(), "format"))
      if self.verbosity:
        print("Updating file '%s'" % format.path)
      format.write_format(format_nbr, self.verbosity)
    else:
      if self.verbosity:
        print("Skipping file '%s'" % format.path)

    if self.verbosity:
      print("Checking whether WC format can be converted")
    try:
      entries.assert_valid_format(format_nbr, self.verbosity)
    except LossyConversionException, e:
      # In --force mode, ignore complaints about lossy conversion.
      if self.force:
        print("WARNING: WC format conversion will be lossy. Dropping "\
              "field(s) %s " % ", ".join(e.lossy_fields))
      else:
        raise

    if self.verbosity:
      print("Writing WC format")
    entries.write_format(format_nbr)

  def change_wc_format(self, format_nbr):
    """Walk all paths in a WC tree, and change their format to
    FORMAT_NBR.  Throw LossyConversionException or NotImplementedError
    if the WC format should not be converted, or is unrecognized."""
    for dirpath, dirs, files in os.walk(self.root_path):
      self.write_dir_format(format_nbr, dirpath, dirs + files)

class Entries:
  """Represents a .svn/entries file.

  'The entries file' section in subversion/libsvn_wc/README is a
  useful reference."""

  # The name and index of each field composing an entry's record.
  entry_fields = (
    "name",
    "kind",
    "revision",
    "url",
    "repos",
    "schedule",
    "text-time",
    "checksum",
    "committed-date",
    "committed-rev",
    "last-author",
    "has-props",
    "has-prop-mods",
    "cachable-props",
    "present-props",
    "conflict-old",
    "conflict-new",
    "conflict-wrk",
    "prop-reject-file",
    "copied",
    "copyfrom-url",
    "copyfrom-rev",
    "deleted",
    "absent",
    "incomplete",
    "uuid",
    "lock-token",
    "lock-owner",
    "lock-comment",
    "lock-creation-date",
    "changelist",
    "keep-local",
    "working-size",
    "depth",
    "tree-conflicts",
    "file-external",
  )

  # The format number.
  format_nbr = -1

  # How many bytes the format number takes in the file.  (The format number
  # may have leading zeroes after using this script to convert format 10 to
  # format 9 -- which would write the format number as '09'.)
  format_nbr_bytes = -1

  def __init__(self, path):
    self.path = path
    self.entries = []

  def parse(self, verbosity=0):
    """Parse the entries file.  Throw NotImplementedError if the WC
    format is unrecognized."""

    input = open(self.path, "r")

    # Read WC format number from INPUT.  Validate that it
    # is a supported format for conversion.
    format_line = input.readline()
    try:
      self.format_nbr = int(format_line)
      self.format_nbr_bytes = len(format_line.rstrip()) # remove '\n'
    except ValueError:
      self.format_nbr = -1
      self.format_nbr_bytes = -1
    if not self.format_nbr in LATEST_FORMATS.values():
      raise UnrecognizedWCFormatException(self.format_nbr, self.path)

    # Parse file into individual entries, to later inspect for
    # non-convertable data.
    entry = None
    while True:
      entry = self.parse_entry(input, verbosity)
      if entry is None:
        break
      self.entries.append(entry)

    input.close()

  def assert_valid_format(self, format_nbr, verbosity=0):
    if verbosity >= 2:
      print("Validating format for entries file '%s'" % self.path)
    for entry in self.entries:
      if verbosity >= 3:
        print("Validating format for entry '%s'" % entry.get_name())
      try:
        entry.assert_valid_format(format_nbr)
      except LossyConversionException:
        if verbosity >= 3:
          sys.stderr.write("Offending entry:\n%s\n" % entry)
          sys.stderr.flush()
        raise

  def parse_entry(self, input, verbosity=0):
    "Read an individual entry from INPUT stream."
    entry = None

    while True:
      line = input.readline()
      if line in ("", "\x0c\n"):
        # EOF or end of entry terminator encountered.
        break

      if entry is None:
        entry = Entry()

      # Retain the field value, ditching its field terminator ("\x0a").
      entry.fields.append(line[:-1])

    if entry is not None and verbosity >= 3:
      sys.stdout.write(str(entry))
      print("-" * 76)
    return entry

  def write_format(self, format_nbr):
    # Overwrite all bytes of the format number (which are the first bytes in
    # the file).  Overwrite format '10' by format '09', which will be converted
    # to '9' by Subversion when it rewrites the file.  (Subversion 1.4 and later
    # ignore leading zeroes in the format number.)
    assert len(str(format_nbr)) <= self.format_nbr_bytes
    format_string = '%0' + str(self.format_nbr_bytes) + 'd'

    os.chmod(self.path, 0600)
    output = open(self.path, "r+", 0)
    output.write(format_string % format_nbr)
    output.close()
    os.chmod(self.path, 0400)

class Entry:
  "Describes an entry in a WC."

  # Maps format numbers to indices of fields within an entry's record that must
  # be retained when downgrading to that format.
  must_retain_fields = {
      # Not in 1.4: changelist, keep-local, depth, tree-conflicts, file-externals
      8  : (30, 31, 33, 34, 35),
      # Not in 1.5: tree-conflicts, file-externals
      9  : (34, 35),
      10 : (),
      # Downgrading from format 11 (1.7-dev) to format 10 is not possible,
      # because 11 does not use has-props and cachable-props (but 10 does).
      # Naively downgrading in that situation causes properties to disappear
      # from the wc.
      #
      # Downgrading from the 1.7 SQLite-based format to format 10 is not
      # implemented.
      }

  def __init__(self):
    self.fields = []

  def assert_valid_format(self, format_nbr):
    "Assure that conversion will be non-lossy by examining fields."

    # Check whether lossy conversion is being attempted.
    lossy_fields = []
    for field_index in self.must_retain_fields[format_nbr]:
      if len(self.fields) - 1 >= field_index and self.fields[field_index]:
        lossy_fields.append(Entries.entry_fields[field_index])
    if lossy_fields:
      raise LossyConversionException(lossy_fields,
        "Lossy WC format conversion requested for entry '%s'\n"
        "Data for the following field(s) is unsupported by older versions "
        "of\nSubversion, and is likely to be subsequently discarded, and/or "
        "have\nunexpected side-effects: %s\n\n"
        "WC format conversion was cancelled, use the --force option to "
        "override\nthe default behavior."
        % (self.get_name(), ", ".join(lossy_fields)))

  def get_name(self):
    "Return the name of this entry."
    return len(self.fields) > 0 and self.fields[0] or ""

  def __str__(self):
    "Return all fields from this entry as a multi-line string."
    rep = ""
    for i in range(0, len(self.fields)):
      rep += "[%s] %s\n" % (Entries.entry_fields[i], self.fields[i])
    return rep

class Format:
  """Represents a .svn/format file."""

  def __init__(self, path):
    self.path = path

  def write_format(self, format_nbr, verbosity=0):
    format_string = '%d\n'
    if os.path.exists(self.path):
      if verbosity >= 1:
        print("%s will be updated." % self.path)
      os.chmod(self.path,0600)
    else:
      if verbosity >= 1:
        print("%s does not exist, creating it." % self.path)
    format = open(self.path, "w")
    format.write(format_string % format_nbr)
    format.close()
    os.chmod(self.path, 0400)

class LocalException(Exception):
  """Root of local exception class hierarchy."""
  pass

class LossyConversionException(LocalException):
  "Exception thrown when a lossy WC format conversion is requested."
  def __init__(self, lossy_fields, str):
    self.lossy_fields = lossy_fields
    self.str = str
  def __str__(self):
    return self.str

class UnrecognizedWCFormatException(LocalException):
  def __init__(self, format, path):
    self.format = format
    self.path = path
  def __str__(self):
    return ("Unrecognized WC format %d in '%s'; "
            "only formats 8, 9, and 10 can be supported") % (self.format, self.path)


def main():
  try:
    opts, args = my_getopt(sys.argv[1:], "vh?",
                           ["debug", "force", "skip-unknown-format",
                            "verbose", "help"])
  except:
    usage_and_exit("Unable to process arguments/options")

  converter = WCFormatConverter()

  # Process arguments.
  if len(args) == 2:
    converter.root_path = args[0]
    svn_version = args[1]
  else:
    usage_and_exit()

  # Process options.
  debug = False
  for opt, value in opts:
    if opt in ("--help", "-h", "-?"):
      usage_and_exit()
    elif opt == "--force":
      converter.force = True
    elif opt == "--skip-unknown-format":
      converter.error_on_unrecognized = False
    elif opt in ("--verbose", "-v"):
      converter.verbosity += 1
    elif opt == "--debug":
      debug = True
    else:
      usage_and_exit("Unknown option '%s'" % opt)

  try:
    new_format_nbr = LATEST_FORMATS[svn_version]
  except KeyError:
    usage_and_exit("Unsupported version number '%s'; "
                   "only 1.4, 1.5, and 1.6 can be supported" % svn_version)

  try:
    converter.change_wc_format(new_format_nbr)
  except LocalException, e:
    if debug:
      raise
    sys.stderr.write("%s\n" % e)
    sys.stderr.flush()
    sys.exit(1)

  print("Converted WC at '%s' into format %d for Subversion %s" % \
        (converter.root_path, new_format_nbr, svn_version))

if __name__ == "__main__":
  main()
