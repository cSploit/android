#!/usr/bin/python2.4
# Copyright (c) 2009 International Business Machines
# Corporation and others. All Rights Reserved.
#
#   file name:  ucdcopy.py
#   encoding:   US-ASCII
#   tab size:   8 (not used)
#   indentation:4
#
#   created on: 2009aug04
#   created by: Markus W. Scherer
#
# Copy Unicode Character Database (ucd) files from a tree
# of files downloaded from ftp://www.unicode.org/Public/5.2.0/
# to a folder like ICU's source/data/unidata/
# and modify some of the files to make them more compact.
#
# Invoke with two command-line parameters, for the source
# and destination folders.

import os
import os.path
import re
import shutil
import sys

_strip_re = re.compile("^([0-9a-fA-F]+.+?) *#.*")
_code_point_re = re.compile("\s*([0-9a-fA-F]+)\s*;")

def CopyAndStripWithOptionalMerge(s, t, do_merge):
  in_file = open(s, "r")
  out_file = open(t, "w")
  first = -1  # First code point with first_data.
  last = -1  # Last code point with first_data.
  first_data = ""  # Common data for code points [first..last].
  for line in in_file:
    match = _strip_re.match(line)
    if match:
      line = match.group(1)
    else:
      line = line.rstrip()
    if do_merge:
      match = _code_point_re.match(line)
      if match:
        c = int(match.group(1), 16)
        data = line[match.end() - 1:]
      else:
        c = -1
        data = ""
      if last >= 0 and (c != (last + 1) or data != first_data):
        # output the current range
        if first == last:
          out_file.write("%04X%s\n" % (first, first_data))
        else:
          out_file.write("%04X..%04X%s\n" % (first, last, first_data))
        first = -1
        last = -1
        first_data = ""
      if c < 0:
        # no data on this line, output as is
        out_file.write(line)
        out_file.write("\n")
      else:
        # data on this line, store for possible range compaction
        if last < 0:
          # set as the first line in a possible range
          first = c
          last = c
          first_data = data
        else:
          # must be c == (last + 1) and data == first_data
          # because of previous conditions
          # continue with the current range
          last = c
    else:
      # Only strip, don't merge: just output the stripped line.
      out_file.write(line)
      out_file.write("\n")
  if do_merge and last >= 0:
    # output the last range in the file
    if first == last:
      out_file.write("%04X%s\n" % (first, first_data))
    else:
      out_file.write("%04X..%04X%s\n" % (first, last, first_data))
    first = -1
    last = -1
    first_data = ""
  in_file.close()
  out_file.flush()
  out_file.close()


def CopyAndStrip(s, t):
  """Copies a file and removes comments behind data lines but not in others."""
  CopyAndStripWithOptionalMerge(s, t, False)


def CopyAndStripAndMerge(s, t):
  """Copies and strips a file and merges lines.

  Copies a file, removes comments, and
  merges lines with adjacent code point ranges and identical per-code point
  data lines into one line with range syntax.
  """
  CopyAndStripWithOptionalMerge(s, t, True)


_unidata_files = {
  # Simply copy these files.
  "BidiMirroring.txt": shutil.copy,
  "BidiTest.txt": shutil.copy,
  "Blocks.txt": shutil.copy,
  "CaseFolding.txt": shutil.copy,
  "DerivedAge.txt": shutil.copy,
  "DerivedBidiClass.txt": shutil.copy,
  "DerivedJoiningGroup.txt": shutil.copy,
  "DerivedJoiningType.txt": shutil.copy,
  "DerivedNumericValues.txt": shutil.copy,
  "NameAliases.txt": shutil.copy,
  "NormalizationCorrections.txt": shutil.copy,
  "PropertyAliases.txt": shutil.copy,
  "PropertyValueAliases.txt": shutil.copy,
  "SpecialCasing.txt": shutil.copy,
  "UnicodeData.txt": shutil.copy,

  # Copy these files and remove comments behind data lines but not in others.
  "DerivedCoreProperties.txt": CopyAndStrip,
  "DerivedNormalizationProps.txt": CopyAndStrip,
  "GraphemeBreakProperty.txt": CopyAndStrip,
  "NormalizationTest.txt": CopyAndStrip,
  "PropList.txt": CopyAndStrip,
  "Scripts.txt": CopyAndStrip,
  "SentenceBreakProperty.txt": CopyAndStrip,
  "WordBreakProperty.txt": CopyAndStrip,

  # Also merge lines with adjacent code point ranges.
  "EastAsianWidth.txt": CopyAndStripAndMerge,
  "LineBreak.txt": CopyAndStripAndMerge
}

_file_version_re = re.compile("^([a-zA-Z0-9]+)" +
                              "-[0-9](?:\\.[0-9])*(?:d[0-9]+)?" +
                              "(\\.[a-z]+)$")

def main():
  source_root = sys.argv[1]
  dest_root = sys.argv[2]
  source_files = []
  for root, dirs, files in os.walk(source_root):
    for file in files:
      source_files.append(os.path.join(root, file))
  files_processed = set()
  for source_file in source_files:
    basename = os.path.basename(source_file)
    match = _file_version_re.match(basename)
    if match:
      basename = match.group(1) + match.group(2)
      print basename
    if basename in _unidata_files:
      if basename in files_processed:
        print "duplicate file basename %s!" % basename
        sys.exit(1)
      files_processed.add(basename)
      dest_file = os.path.join(dest_root, basename)
      _unidata_files[basename](source_file, dest_file)


if __name__ == "__main__":
  main()
