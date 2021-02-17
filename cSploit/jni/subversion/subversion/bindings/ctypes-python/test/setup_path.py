#!/usr/bin/python -i
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


# Set things up to use the bindings in the build directory.
# Used by the testsuite, but you may also run:
#   $ python -i tests/setup_path.py
# to start an interactive interpreter.

import sys
import os

src_swig_python_tests_dir = os.path.dirname(os.path.dirname(__file__))
sys.path[0:0] = [ src_swig_python_tests_dir ]

import csvn.core
csvn.core.svn_cmdline_init("", csvn.core.stderr)
