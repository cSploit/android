#!/usr/bin/env python
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

import errno, os, re, sys, tempfile

from distutils import log
from distutils.cmd import Command
from distutils.command.build import build as _build
from distutils.command.clean import clean as _clean
from distutils.core import setup
from distutils.dir_util import remove_tree
from distutils.errors import DistutilsExecError
from distutils.errors import DistutilsOptionError

from glob import glob

class clean(_clean):
  """Special distutils command for cleaning the Subversion ctypes bindings."""
  description = "clean the Subversion ctypes Python bindings"

  def initialize_options (self):
    _clean.initialize_options(self)

  # initialize_options()

  def finalize_options (self):
    _clean.finalize_options(self)

  # finalize_options()

  def run(self):
    functions_py = os.path.join(os.path.dirname(__file__), "csvn", "core",
                                "functions.py")
    functions_pyc = os.path.join(os.path.dirname(__file__), "csvn", "core",
                                 "functions.pyc")
    svn_all_py = os.path.join(os.path.dirname(__file__), "svn_all.py")
    svn_all2_py = os.path.join(os.path.dirname(__file__), "svn_all2.py")

    for f in (functions_py, functions_pyc, svn_all_py, svn_all2_py):
      if os.path.exists(f):
        log.info("removing '%s'", os.path.normpath(f))

        if not self.dry_run:
          os.remove(f)
      else:
        log.warn("'%s' does not exist -- can't clean it", os.path.normpath(f))

    # Run standard clean command
    _clean.run(self)

  # run()

# class clean

class build(_build):
  """Special distutils command for building the Subversion ctypes bindings."""

  description = "build the Subversion ctypes Python bindings"

  _build.user_options.append(("apr=", None, "full path to where apr is "
                              "installed or the full path to the apr-1-config or "
                              "apr-config file"))
  _build.user_options.append(("apr-util=", None, "full path to where apr-util "
                              "is installed or the full path to the apu-1-config or "
                              "apu-config file"))
  _build.user_options.append(("subversion=", None, "full path to where "
                              "Subversion is installed"))
  _build.user_options.append(("svn-headers=", None, "Full path to the "
                              "Subversion header files, if they are not in a "
                              "standard location"))
  _build.user_options.append(("ctypesgen=", None, "full path to where ctypesgen "
                              "is installed, to the ctypesgen source tree or "
                              "the full path to ctypesgen.py"))
  _build.user_options.append(("cppflags=", None, "extra flags to pass to the c "
                              "preprocessor"))
  _build.user_options.append(("ldflags=", None, "extra flags to pass to the "
                              "ctypesgen linker"))
  _build.user_options.append(("lib-dirs=", None, "colon-delimited list of paths "
                              "to append to the search path"))
  _build.user_options.append(("save-preprocessed-headers=", None, "full path to "
                              "where to save the preprocessed headers"))

  def initialize_options (self):
    _build.initialize_options(self)
    self.apr = None
    self.apr_util = None
    self.ctypesgen = None
    self.subversion = None
    self.svn_headers = None
    self.cppflags = ""
    self.ldflags = ""
    self.lib_dirs = None
    self.save_preprocessed_headers = None

  # initialize_options()

  def finalize_options (self):
    _build.finalize_options(self)

    # Distutils doesn't appear to like when you have --dry-run after the build
    # build command so fail out if this is the case.
    if self.dry_run != self.distribution.dry_run:
      raise DistutilsOptionError("The --dry-run flag must be specified " \
                                 "before the 'build' command")

  # finalize_options()

  ##############################################################################
  # Get build configuration
  ##############################################################################
  def get_build_config (self):
    flags = []
    ldflags = []
    library_path = []
    ferr = None
    apr_include_dir = None

    fout = self.run_cmd("%s --includes --cppflags" % self.apr_config)
    if fout:
      flags = fout.split()
      apr_prefix = self.run_cmd("%s --prefix" % self.apr_config)
      apr_prefix = apr_prefix.strip()
      apr_include_dir = self.run_cmd("%s --includedir" % self.apr_config).strip()
      apr_version = self.run_cmd("%s --version" % self.apr_config).strip()
      cpp  = self.run_cmd("%s --cpp" % self.apr_config).strip()
      fout = self.run_cmd("%s --ldflags --link-ld" % self.apr_config)
      if fout:
        ldflags = fout.split()
      else:
        log.error(ferr)
        raise DistutilsExecError("Problem running '%s'.  Check the output " \
                                 "for details" % self.apr_config)

      fout = self.run_cmd("%s --includes" % self.apu_config)
      if fout:
        flags += fout.split()
        fout = self.run_cmd("%s --ldflags --link-ld" % self.apu_config)
        if fout:
          ldflags += fout.split()
        else:
          log.error(ferr)
          raise DistutilsExecError("Problem running '%s'.  Check the output " \
                                   "for details" % self.apr_config)

        subversion_prefixes = [
          self.subversion,
          "/usr/local",
          "/usr"
          ]

        if self.subversion != "/usr":
          ldflags.append("-L%s/lib" % self.subversion)
        if self.svn_include_dir[-18:] == "subversion/include":
          flags.append("-Isubversion/include")
        else:
          flags.append("-I%s" % self.svn_include_dir)

        # List the libraries in the order they should be loaded
        libraries = [
          "svn_subr-1",
          "svn_diff-1",
          "svn_delta-1",
          "svn_fs-1",
          "svn_repos-1",
          "svn_wc-1",
          "svn_ra-1",
          "svn_client-1",
          ]

        for lib in libraries:
          ldflags.append("-l%s" % lib)

        if apr_prefix != '/usr':
          library_path.append("%s/lib" % apr_prefix)
          if self.subversion != '/usr' and self.subversion != apr_prefix:
            library_path.append("%s/lib" % self.subversion)

        return (apr_prefix, apr_include_dir, cpp + " " + self.cppflags,
                " ".join(ldflags) + " " + self.ldflags, " ".join(flags),
                ":".join(library_path))

  # get_build_config()

  ##############################################################################
  # Build csvn/core/functions.py
  ##############################################################################
  def build_functions_py(self):
    (apr_prefix, apr_include_dir, cpp, ldflags, flags,
     library_path) = self.get_build_config()
    cwd = os.getcwd()
    if self.svn_include_dir[-18:] == "subversion/include":
      includes = ('subversion/include/svn_*.h '
                  '%s/ap[ru]_*.h' % apr_include_dir)
      cmd = ["%s %s --cpp '%s %s' %s "
             "%s -o subversion/bindings/ctypes-python/svn_all.py "
             "--no-macro-warnings --strip-build-path=%s" % (sys.executable,
                                                            self.ctypesgen_py, cpp,
                                                            flags, ldflags,
                                                            includes, self.svn_include_dir[:-19])]
      os.chdir(self.svn_include_dir[:-19])
    else:
      includes = ('%s/svn_*.h '
                  '%s/ap[ru]_*.h' % (self.svn_include_dir, apr_include_dir))
      cmd = ["%s %s --cpp '%s %s' %s "
             "%s -o svn_all.py --no-macro-warnings" % (sys.executable,
                                                       self.ctypesgen_py, cpp,
                                                       flags, ldflags,
                                                       includes)]
    if self.lib_dirs:
      cmd.extend('-R ' + x for x in self.lib_dirs.split(":"))
    cmd = ' '.join(cmd)

    if self.save_preprocessed_headers:
      cmd += " --save-preprocessed-headers=%s" % \
          os.path.abspath(self.save_preprocessed_headers)

    if self.verbose or self.dry_run:
      status = self.execute(os.system, (cmd,), cmd)
    else:
      f = os.popen(cmd, 'r')
      f.read() # Required to avoid the 'Broken pipe' error.
      status = f.close() # None is returned for the usual 0 return code

    os.chdir(cwd)

    if os.name == "posix" and status and status != 0:
      if os.WIFEXITED(status):
        status = os.WEXITSTATUS(status)
        if status != 0:
          sys.exit(status)
        elif os.WIFSIGNALED(status):
          log.error("ctypesgen.py killed with signal %d" % os.WTERMSIG(status))
          sys.exit(2)
        elif os.WIFSTOPPED(status):
          log.error("ctypesgen.py stopped with signal %d" % os.WSTOPSIG(status))
          sys.exit(2)
        else:
          log.error("ctypesgen.py exited with invalid status %d", status)
          sys.exit(2)

    if not self.dry_run:
      r = re.compile(r"(\s+\w+)\.restype = POINTER\(svn_error_t\)")
      out = open("svn_all2.py", "w")
      for line in open("svn_all.py"):
        line = r.sub("\\1.restype = POINTER(svn_error_t)\n\\1.errcheck = _svn_errcheck", line)

        if not line.startswith("FILE ="):
          out.write(line)
      out.close()

    cmd = "cat csvn/core/functions.py.in svn_all2.py > csvn/core/functions.py"
    self.execute(os.system, (cmd,), cmd)

    log.info("Generated csvn/core/functions.py successfully")

  # build_functions_py()

  def run_cmd(self, cmd):
    return os.popen(cmd).read()

  # run_cmd()

  def validate_options(self):
    # Validate apr
    if not self.apr:
      self.apr = find_in_path('apr-1-config')

      if not self.apr:
        self.apr = find_in_path('apr-config')

      if self.apr:
        log.info("Found %s" % self.apr)
      else:
        raise DistutilsOptionError("Could not find apr-1-config or " \
                                   "apr-config.  Please rerun with the " \
                                   "--apr option.")

    if os.path.exists(self.apr):
      if os.path.isdir(self.apr):
        if os.path.exists(os.path.join(self.apr, "bin", "apr-1-config")):
          self.apr_config = os.path.join(self.apr, "bin", "apr-1-config")
        elif os.path.exists(os.path.join(self.apr, "bin", "apr-config")):
          self.apr_config = os.path.join(self.apr, "bin", "apr-config")
        else:
          self.apr_config = None
      elif os.path.basename(self.apr) in ("apr-1-config", "apr-config"):
        self.apr_config = self.apr
      else:
        self.apr_config = None
    else:
      self.apr_config = None

    if not self.apr_config:
      raise DistutilsOptionError("The --apr option is not valid.  It must " \
                                 "point to a valid apr installation or " \
                                 "to either the apr-1-config file or the " \
                                 "apr-config file")

    # Validate apr-util
    if not self.apr_util:
      self.apr_util = find_in_path('apu-1-config')

      if not self.apr_util:
        self.apr_util = find_in_path('apu-config')

      if self.apr_util:
        log.info("Found %s" % self.apr_util)
      else:
        raise DistutilsOptionError("Could not find apu-1-config or " \
                                   "apu-config.  Please rerun with the " \
                                   "--apr-util option.")

    if os.path.exists(self.apr_util):
      if os.path.isdir(self.apr_util):
        if os.path.exists(os.path.join(self.apr_util, "bin", "apu-1-config")):
          self.apu_config = os.path.join(self.apr_util, "bin", "apu-1-config")
        elif os.path.exists(os.path.join(self.apr_util, "bin", "apu-config")):
          self.apu_config = os.path.join(self.apr_util, "bin", "apu-config")
        else:
          self.apu_config = None
      elif os.path.basename(self.apr_util) in ("apu-1-config", "apu-config"):
        self.apu_config = self.apr_util
      else:
        self.apu_config = None
    else:
      self.apu_config = None

    if not self.apu_config:
      raise DistutilsOptionError("The --apr-util option is not valid.  It " \
                                 "must point to a valid apr-util " \
                                 "installation or to either the apu-1-config " \
                                 "file or the apu-config file")

    # Validate subversion
    if not self.subversion:
      self.subversion = find_in_path('svn')

      if self.subversion:
        log.info("Found %s" % self.subversion)
        # Get the installation root instead of path to 'svn'
        self.subversion = os.path.normpath(os.path.join(self.subversion, "..",
                                                        ".."))
      else:
        raise DistutilsOptionError("Could not find Subversion.  Please rerun " \
                                   "with the --subversion option.")

    # Validate svn-headers, if present
    if self.svn_headers:
      if os.path.isdir(self.svn_headers):
        if os.path.exists(os.path.join(self.svn_headers, "svn_client.h")):
          self.svn_include_dir = self.svn_headers
        elif os.path.exists(os.path.join(self.svn_headers, "subversion-1",
                                         "svn_client.h")):
          self.svn_include_dir = os.path.join(self.svn_headers, "subversion-1")
        else:
          self.svn_include_dir = None
      else:
        self.svn_include_dir = None
    elif os.path.exists(os.path.join(self.subversion, "include",
                                     "subversion-1")):
      self.svn_include_dir = "%s/include/subversion-1" % self.subversion
    else:
      self.svn_include_dir = None

    if not self.svn_include_dir:
      msg = ""

      if self.svn_headers:
        msg = "The --svn-headers options is not valid.  It must point to " \
              "either a Subversion include directory or the Subversion " \
              "include/subversion-1 directory."
      else:
        msg = "The --subversion option is not valid. " \
              "Could not locate %s/include/" \
              "subversion-1/svn_client.h" % self.subversion

      raise DistutilsOptionError(msg)

    # Validate ctypesgen
    if not self.ctypesgen:
      self.ctypesgen = find_in_path('ctypesgen.py')

      if self.ctypesgen:
        log.info("Found %s" % self.ctypesgen)
      else:
        raise DistutilsOptionError("Could not find ctypesgen.  Please rerun " \
                                   "with the --ctypesgen option.")

    if os.path.exists(self.ctypesgen):
      if os.path.isdir(self.ctypesgen):
        if os.path.exists(os.path.join(self.ctypesgen, "ctypesgen.py")):
          self.ctypesgen_py = os.path.join(self.ctypesgen, "ctypesgen.py")
        elif os.path.exists(os.path.join(self.ctypesgen, "bin",
                                         "ctypesgen.py")):
          self.ctypesgen_py = os.path.join(self.ctypesgen, "bin",
                                           "ctypesgen.py")
        else:
          self.ctypesgen_py = None
      elif os.path.basename(self.ctypesgen) == "ctypesgen.py":
          self.ctypesgen_py = self.ctypesgen
      else:
        self.ctypesgen_py = None
    else:
      self.ctypesgen_py = None

    if not self.ctypesgen_py:
      raise DistutilsOptionError("The --ctypesgen option is not valid.  It " \
                                 "must point to a valid ctypesgen " \
                                 "installation, a ctypesgen source tree or " \
                                 "to the ctypesgen.py script")

  # validate_functions()

  def run (self):
    # We only want to build if functions.py is not present.
    if not os.path.exists(os.path.join(os.path.dirname(__file__),
                                       "csvn", "core",
                                       "functions.py")):
      if 'build' not in self.distribution.commands:
        raise DistutilsOptionError("You must run 'build' explicitly before " \
                                   "you can proceed")

      # Validate the command line options
      self.validate_options()

      # Generate functions.py
      self.build_functions_py()
    else:
      log.info("csvn/core/functions.py was not regenerated (output up-to-date)")

    # Run the standard build command.
    _build.run(self)

  # run()

# class build

def find_in_path(file):
  paths = []
  if os.environ.has_key('PATH'):
    paths = os.environ['PATH'].split(os.pathsep)

  for path in paths:
    file_path = os.path.join(path, file)

    if os.path.exists(file_path):
      # Return the path to the first existing file found in PATH
      return os.path.abspath(file_path)

  return None
# find_in_path()

setup(cmdclass={'build': build, 'clean': clean},
      name='svn-ctypes-python-bindings',
      version='0.1',
      description='Python bindings for the Subversion version control system.',
      author='The Subversion Team',
      author_email='dev@subversion.apache.org',
      url='http://subversion.apache.org',
      packages=['csvn', 'csvn.core', 'csvn.ext'],
      license='Apache License, Version 2.0',
     )

# TODO: We need to create our own bdist_rpm implementation so that we can pass
#       our required arguments to the build command being called by bdist_rpm.
