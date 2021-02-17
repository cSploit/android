#!/usr/bin/env python
#
# Copyright (C) 2010 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# Generate ICU dat files for locale relevant resources.
#
# Usage:
#    icu_dat_generator.py  icu-version [-v] [-h]
#
# Sample usage:
#   $ANDROID_BUILD_TOP/external/icu4c/stubdata$ ./icu_dat_generator.py  4.4 --verbose
#
#  Add new dat file:
#    1. Add icudtxxl-<datname>.txt to $ANDROID_BUILD_TOP/external/icu4c/stubdata.
#       Check the exemplar file under
#       $ANDROID_BUILD_TOP/external/icu4c/stubdata/icudt42l-us.dat.
#    2. Add an entry to main() --> datlist[]
#    3. Run this script to generate dat files.
#
#  For ICU upgrade
#    We cannot get CLDR version from dat file unless calling ICU function.
#    If there is a CLDR version change, please modify "global CLDR_VERSION".

import getopt
import os.path
import shutil
import subprocess
import sys

# Return 0 if the version_string contains non-digit characters.
def GetIcuVersion(version_string):
  list = version_string.split(".")
  version = ""
  for number in list:
    if (number.isdigit()):
      version += number
    else:
      return -1
  return version


def PrintHelp():
  print "Usage:"
  print "icu_dat_generator.py  icu-version [-v|--verbose] [-h|--help]"
  print "Example:"
  print "$ANDROID_BUILD_TOP/external/icu4c/stubdata$ ./icu_dat_generator.py 4.4"


def InvokeIcuTool(tool, working_dir, args):
  command_list = [os.path.join(GetIcuPrebuiltDir(), tool)]
  command_list.extend(args)

  if VERBOSE:
    command = "[%s] %s" % (working_dir, " ".join(command_list))
    print command
  
  ret = subprocess.call(command_list, cwd = working_dir)
  if ret != 0:
    sys.exit(command_list[0:])


def GetIcuPrebuiltDir():
  return os.path.join(os.environ.get("ANDROID_EABI_TOOLCHAIN"),  "..", "..",
                      "..", "icu-" + ICU_VERSION)


def ExtractAllResourceToTempDir():
  # copy icudtxxl-all.dat to icudtxxl.dat
  source_dat = os.path.join(ANDROID_ROOT, "external", "icu4c", "stubdata",
                            ICUDATA + "-all.dat")
  dest_dat = os.path.join(ANDROID_ROOT, "external", "icu4c", "stubdata",
                          ICUDATA_DAT)
  shutil.copyfile(source_dat, dest_dat)
  InvokeIcuTool("icupkg", None, [dest_dat, "-x", "*", "-d", TMP_DAT_PATH])


def MakeDat(icu_dat_path, dat_name):
  # Get the resource list. e.g. icudt42l-us.txt, icudt42l-default.txt.
  dat_list_file_path = os.path.join(icu_dat_path, ICUDATA + "-" + dat_name +
                                    ".txt")
  if not os.path.isfile(dat_list_file_path):
    print "%s not present for resource list." % dat_list_file_path
    return
  GenResIndex(dat_list_file_path)
  CopyAndroidCnvFiles(icu_dat_path)
  os.chdir(TMP_DAT_PATH)
  # Run command such as "icupkg -tl -s icudt42l -a icudt42l-us.txt
  # new icudt42l.dat"
  InvokeIcuTool("icupkg", None, ["-tl", "-s", TMP_DAT_PATH, "-a", dat_list_file_path, "new",
                ICUDATA_DAT])


def WriteIndex(path, list, cldr_version = None):
  res_index = "res_index.txt"
  empty_value = " {\"\"}\n"  # key-value pair for all locale entries

  f = open(path, "w")
  f.write("res_index:table(nofallback) {\n")
  if cldr_version:
    f.write("  CLDRVersion { %s }\n" % cldr_version)
  f.write("  InstalledLocales {\n")
  for item in list:
    f.write(item + empty_value)

  f.write("  }\n")
  f.write("}\n")
  f.close()


def ShowMissing(whats, locales, data_dir_name, dat_file):
  if not len(whats):
    return
  dat_file = os.path.basename(dat_file)
  for missing in locales.difference(whats):
    p = os.path.join(ANDROID_ROOT, "external", "icu4c", "data", data_dir_name,
                     missing + ".txt")
    if os.path.exists(p):
      print "warning: %s exists but isn't included in %s" % (p, dat_file)


# Open dat file such as icudt42l-us.txt.
# Go through the list and generate res_index.txt for locales, brkitr,
# coll and rbnf.
def GenResIndex(dat_list_file_path):
  res_index = "res_index.txt"

  locales = set()
  brkitrs = set()
  colls = set()
  currs = set()
  langs = set()
  regions = set()
  zones = set()
  rbnfs = set()

  for line in open(dat_list_file_path, "r"):
    if line.find("root.") >= 0:
      continue
    if line.find("res_index") >= 0:
      continue
    if line.find("_.res") >= 0:
      continue;
    if line.find("brkitr/") >= 0:
      end = line.find(".res")
      if end > 0:
        brkitrs.add(line[line.find("/")+1:end])
    elif line.find("coll/") >= 0:
      end = line.find(".res")
      if end > 0:
        colls.add(line[line.find("/")+1:end])
    elif line.find("curr/") >= 0:
      end = line.find(".res")
      if end > 0:
        currs.add(line[line.find("/")+1:end])
    elif line.find("lang/") >= 0:
      end = line.find(".res")
      if end > 0:
        langs.add(line[line.find("/")+1:end])
    elif line.find("region/") >= 0:
      end = line.find(".res")
      if end > 0:
        regions.add(line[line.find("/")+1:end])
    elif line.find("zone/") >= 0:
      end = line.find(".res")
      if end > 0:
        zones.add(line[line.find("/")+1:end])
    elif line.find("rbnf/") >= 0:
      end = line.find(".res")
      if end > 0:
        rbnfs.add(line[line.find("/")+1:end])
    elif line.find(".res") >= 0:
      # We need to determine the resource is locale resource or misc resource.
      # To determine the locale resource, we assume max script length is 3.
      end = line.find(".res")
      if end <= 3 or (line.find("_") <= 3 and line.find("_") > 0):
        locales.add(line[:end])

  ShowMissing(brkitrs, locales, "brkitr", dat_list_file_path)
  ShowMissing(colls, locales, "coll", dat_list_file_path)
  ShowMissing(currs, locales, "curr", dat_list_file_path)
  ShowMissing(langs, locales, "lang", dat_list_file_path)
  ShowMissing(regions, locales, "region", dat_list_file_path)
  ShowMissing(zones, locales, "zone", dat_list_file_path)
  ShowMissing(rbnfs, locales, "rbnf", dat_list_file_path)

  WriteIndex(os.path.join(TMP_DAT_PATH, res_index), locales, CLDR_VERSION)
  WriteIndex(os.path.join(TMP_DAT_PATH, "brkitr", res_index), brkitrs)
  WriteIndex(os.path.join(TMP_DAT_PATH, "coll", res_index), colls)
  WriteIndex(os.path.join(TMP_DAT_PATH, "curr", res_index), currs)
  WriteIndex(os.path.join(TMP_DAT_PATH, "lang", res_index), langs)
  WriteIndex(os.path.join(TMP_DAT_PATH, "region", res_index), regions)
  WriteIndex(os.path.join(TMP_DAT_PATH, "zone", res_index), zones)
  WriteIndex(os.path.join(TMP_DAT_PATH, "rbnf", res_index), rbnfs)

  # Call genrb to generate new res_index.res.
  InvokeIcuTool("genrb", TMP_DAT_PATH, [res_index])
  InvokeIcuTool("genrb", os.path.join(TMP_DAT_PATH, "brkitr"), [res_index])
  InvokeIcuTool("genrb", os.path.join(TMP_DAT_PATH, "coll"), [res_index])
  InvokeIcuTool("genrb", os.path.join(TMP_DAT_PATH, "curr"), [res_index])
  InvokeIcuTool("genrb", os.path.join(TMP_DAT_PATH, "lang"), [res_index])
  InvokeIcuTool("genrb", os.path.join(TMP_DAT_PATH, "region"), [res_index])
  InvokeIcuTool("genrb", os.path.join(TMP_DAT_PATH, "zone"), [res_index])
  if len(rbnfs):
    InvokeIcuTool("genrb", os.path.join(TMP_DAT_PATH, "rbnf"), [res_index])


def CopyAndroidCnvFiles(icu_dat_path):
  android_specific_cnv = ["gsm-03.38-2000.cnv",
                          "iso-8859_16-2001.cnv",
                          "docomo-shift_jis-2007.cnv",
                          "kddi-jisx-208-2007.cnv",
                          "kddi-shift_jis-2007.cnv",
                          "softbank-jisx-208-2007.cnv",
                          "softbank-shift_jis-2007.cnv"]
  for cnv_file in android_specific_cnv:
    source_path = os.path.join(icu_dat_path, "cnv", cnv_file)
    dest_path = os.path.join(TMP_DAT_PATH, cnv_file)
    shutil.copyfile(source_path, dest_path)
    if VERBOSE:
      print "copy " + source_path + " " + dest_path


def main():
  global ANDROID_ROOT  # Android project home directory
  global ICU_VERSION   # ICU version number
  global ICUDATA       # e.g. "icudt42l"
  global ICUDATA_DAT   # e.g. "icudt42l.dat"
  global CLDR_VERSION  # CLDR version. The value can be vary upon ICU release.
  global TMP_DAT_PATH  # temp directory to store all resource files and
                       # intermediate dat files.
  global HELP
  global VERBOSE

  argc = len(sys.argv)
  if argc < 2:
    print "You must provide icu version number."
    print "Example: ./icu_dat_generator.py 4.4"
    sys.exit(1)
  ICU_VERSION = sys.argv[1]
  version = GetIcuVersion(ICU_VERSION)
  if (version ==  -1):
    print sys.argv[1] + " is not a valid icu version number!"
    sys.exit(1)
  ICUDATA = "icudt" + version + "l"
  CLDR_VERSION = "1.8"
  ANDROID_ROOT = os.environ.get("ANDROID_BUILD_TOP")
  if not ANDROID_ROOT:
    print "$ANDROID_BUILD_TOP not set! Run 'env_setup.sh'."
    sys.exit(1)
  ICUDATA_DAT = ICUDATA + ".dat"
  HELP = False
  VERBOSE = False

  try:
    opts, args = getopt.getopt(sys.argv[2:], 'hv', ['help', 'verbose'])
  except getopt.error:
    print "Invalid option"
    PrintHelp()
    sys.exit(1)
  for opt, arg in opts:
    if opt in ('-h', '--help'):
      PrintHelp()
      sys.exit(1)
    elif opt in ('-v', '--verbose'):
      VERBOSE = True


  # Check for requiered source files.
  icu_dat_path = os.path.join(ANDROID_ROOT, "external", "icu4c", "stubdata")
  full_data_filename = os.path.join(icu_dat_path, ICUDATA + "-all.dat")
  if not os.path.isfile(full_data_filename):
    print "%s not present." % full_data_filename
    sys.exit(1)

  # Create a temporary working directory.
  TMP_DAT_PATH = os.path.join(ANDROID_ROOT, "external", "icu4c", "tmp")
  if os.path.exists(TMP_DAT_PATH):
    shutil.rmtree(TMP_DAT_PATH)
  os.mkdir(TMP_DAT_PATH)

  # Extract resource files from icudtxxl-all.dat to TMP_DAT_PATH.
  ExtractAllResourceToTempDir()

  datlist = ["us", "us-euro", "default", "us-japan", "zh", "medium", "large"]
  for dat_subtag in datlist:
    MakeDat(icu_dat_path, dat_subtag)
    # Copy icudtxxl.dat to stubdata directory with corresponding subtag.
    shutil.copyfile(os.path.join(TMP_DAT_PATH, ICUDATA_DAT),
                    os.path.join(icu_dat_path, ICUDATA + "-" + dat_subtag + ".dat"))
    print "Generate ICU data:" + os.path.join(icu_dat_path, ICUDATA + "-" + dat_subtag + ".dat")

  # Cleanup temporary working directory and icudtxxl.dat
  shutil.rmtree(TMP_DAT_PATH)
  os.remove(os.path.join(icu_dat_path, ICUDATA_DAT))

if __name__ == "__main__":
  main()
