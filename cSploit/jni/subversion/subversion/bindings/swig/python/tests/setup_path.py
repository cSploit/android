#
#
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.
#
#
# Set things up to use the bindings in the build directory.
# Used by the testsuite, but you may also run:
#   $ python -i tests/setup_path.py
# to start an interactive interpreter.

import sys
import os

# os.getcwd() is used here to attempt to support VPATH builds -
# it is expected that the current directory be what 'make check-swig-py'
# arranges for it to be: i.e. BUILDDIR/subversion/bindings/swig/python .
src_swig_python_tests_dir = os.path.dirname(__file__)
bld_swig_python_dir = os.getcwd()
sys.path[0:0] = [ src_swig_python_tests_dir,
                  bld_swig_python_dir,
                  "%s/.libs" % bld_swig_python_dir,
                  "%s/.." % src_swig_python_tests_dir,
                  "%s/../.libs" % src_swig_python_tests_dir ]

# OSes without RPATH support are going to have to do things here to make
# the correct shared libraries be found.
if sys.platform == 'cygwin':
  import glob
  svndir = os.path.normpath("../../../%s" % bld_swig_python_dir)
  libpath = os.getenv("PATH").split(":")
  libpath.insert(0, "%s/libsvn_swig_py/.libs" % bld_swig_python_dir)
  for libdir in glob.glob("%s/libsvn_*" % svndir):
    libpath.insert(0, "%s/.libs" % (libdir))
  os.putenv("PATH", ":".join(libpath))

