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


from csvn.core import *

class User(object):

    def __init__(self, username=None, password=None):
        """Create a user object which represents a user
           with the specified username and password."""

        self._username = username
        self._password = password
        self.pool = Pool()

    def username(self):
        """Return the current username.

           By default, this function just returns the username
           which was supplied in the constructor, but subclasses
           may behave differently."""
        return self._username

    def password(self):
        """Return the current password.

           By default, this function just returns the password
           which was supplied in the constructor, but subclasses
           may behave differently."""
        return self._password

    def allow_access(self, requested_access, path):
        """Check whether the current user has the REQUESTED_ACCESS
           to PATH.

           If PATH is None, this function should check if the
           REQUESTED_ACCESS is granted for at least one path
           in the repository.

           REQUESTED_ACCESS is an integer which may consist of
           any combination of the following fields:
              svn_authz_read:      The path can be read
              svn_authz_write:     The path can be altered
              svn_authz_recursive: The other access credentials
                                   are recursive.

           By default, this function always returns True, but
           subclasses may behave differently.

           This function is used by the "Repository" class to check
           permissions (see repos.py).

           FIXME: This function isn't currently used, because we
           haven't implemented higher-level authz yet.
        """

        return True

    def setup_auth_baton(self, auth_baton):

        # Setup the auth baton using the default options from the
        # command-line client
        svn_cmdline_setup_auth_baton(auth_baton, TRUE,
            self._username, self._password, NULL, TRUE, POINTER(svn_config_t)(),
            svn_cancel_func_t(), NULL, self.pool)

