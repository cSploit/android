#
# delta.py: public Python interface for delta components
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

from libsvn.delta import *
from svn.core import _unprefix_names
_unprefix_names(locals(), 'svn_delta_')
_unprefix_names(locals(), 'svn_txdelta_', 'tx_')
del _unprefix_names

# Force our accessor since it appears that there isn't a more civilized way
# to make SWIG use it.
svn_txdelta_window_t.ops = property(svn_txdelta_window_t_ops_get)

class Editor:

  def set_target_revision(self, target_revision, pool=None):
    pass

  def open_root(self, base_revision, dir_pool=None):
    return None

  def delete_entry(self, path, revision, parent_baton, pool=None):
    pass

  def add_directory(self, path, parent_baton,
                    copyfrom_path, copyfrom_revision, dir_pool=None):
    return None

  def open_directory(self, path, parent_baton, base_revision, dir_pool=None):
    return None

  def change_dir_prop(self, dir_baton, name, value, pool=None):
    pass

  def close_directory(self, dir_baton, pool=None):
    pass

  def add_file(self, path, parent_baton,
               copyfrom_path, copyfrom_revision, file_pool=None):
    return None

  def open_file(self, path, parent_baton, base_revision, file_pool=None):
    return None

  def apply_textdelta(self, file_baton, base_checksum, pool=None):
    return None

  def change_file_prop(self, file_baton, name, value, pool=None):
    pass

  def close_file(self, file_baton, text_checksum, pool=None):
    pass

  def close_edit(self, pool=None):
    pass

  def abort_edit(self, pool=None):
    pass


def make_editor(editor, pool=None):
  return svn_swig_py_make_editor(editor, pool)
