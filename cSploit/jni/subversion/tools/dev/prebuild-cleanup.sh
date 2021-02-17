#!/bin/sh
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

### Purify a system, to simulate building Subversion on a "clean" box.
###
### You'll probably need to run this as `root', and may need to change
### some paths for your system.

# Clean out old apr, apr-util config scripts.
rm /usr/local/bin/apr-config
rm /usr/local/bin/apu-config

# Clean out libs.
rm -f /usr/local/lib/APRVARS
rm -f /usr/local/lib/libapr*
rm -f /usr/local/lib/libexpat*
rm -f /usr/local/lib/libneon*
rm -f /usr/local/lib/libsvn*

# Clean out headers.
rm -f /usr/local/include/apr*
rm -f /usr/local/include/svn*
rm -f /usr/local/include/neon/*

### Not sure this would be useful:
# rm -f /usr/local/apache2/lib/*
