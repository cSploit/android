#
# wc.py: public Python interface for wc components
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

from libsvn.wc import *
from svn.core import _unprefix_names
_unprefix_names(locals(), 'svn_wc_')
_unprefix_names(locals(), 'SVN_WC_')
del _unprefix_names


class DiffCallbacks2:
    def file_changed(self, adm_access, path,
                     tmpfile1, tmpfile2, rev1, rev2,
                     mimetype1, mimetype2,
                     propchanges, originalprops):
        return (notify_state_unknown, notify_state_unknown)

    def file_added(self, adm_access, path,
                   tmpfile1, tmpfile2, rev1, rev2,
                   mimetype1, mimetype2,
                   propchanges, originalprops):
        return (notify_state_unknown, notify_state_unknown)

    def file_deleted(self, adm_access, path, tmpfile1, tmpfile2,
                     mimetype1, mimetype2, originalprops):
        return notify_state_unknown

    def dir_added(self, adm_access, path, rev):
        return notify_state_unknown

    def dir_deleted(self, adm_access, path):
        return notify_state_unknown

    def dir_props_changed(self, adm_access, path,
                          propchanges, original_props):
        return notify_state_unknown
