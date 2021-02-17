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
import unittest

import svn.core, svn.client
import utils

class SubversionCoreTestCase(unittest.TestCase):
  """Test cases for the basic SWIG Subversion core"""

  def test_SubversionException(self):
    self.assertEqual(svn.core.SubversionException().args, ())
    self.assertEqual(svn.core.SubversionException('error message').args,
                     ('error message',))
    self.assertEqual(svn.core.SubversionException(None, 1).args, (None, 1))
    self.assertEqual(svn.core.SubversionException('error message', 1).args,
                     ('error message', 1))
    self.assertEqual(svn.core.SubversionException('error message', 1).apr_err,
                     1)
    self.assertEqual(svn.core.SubversionException('error message', 1).message,
                     'error message')

  def test_mime_type_is_binary(self):
    self.assertEqual(0, svn.core.svn_mime_type_is_binary("text/plain"))
    self.assertEqual(1, svn.core.svn_mime_type_is_binary("image/png"))

  def test_mime_type_validate(self):
    self.assertRaises(svn.core.SubversionException,
            svn.core.svn_mime_type_validate, "this\nis\ninvalid\n")
    svn.core.svn_mime_type_validate("unknown/but-valid; charset=utf8")

  def test_exception_interoperability(self):
    """Test if SubversionException is correctly converted into svn_error_t
    and vice versa."""
    t = utils.Temper()
    (_, _, repos_uri) = t.alloc_empty_repo(suffix='-core')
    rev = svn.core.svn_opt_revision_t()
    rev.kind = svn.core.svn_opt_revision_head
    ctx = svn.client.create_context()

    class Receiver:
      def __call__(self, path, info, pool):
        raise self.e

    rec = Receiver()
    args = (repos_uri, rev, rev, rec, svn.core.svn_depth_empty, None, ctx)

    try:
      # ordinary Python exceptions must be passed through
      rec.e = TypeError()
      self.assertRaises(TypeError, svn.client.info2, *args)

      # SubversionException will be translated into an svn_error_t, propagated
      # through the call chain and translated back to SubversionException.
      rec.e = svn.core.SubversionException("Bla bla bla.",
                                           svn.core.SVN_ERR_INCORRECT_PARAMS,
                                           file=__file__, line=866)
      rec.e.child = svn.core.SubversionException("Yada yada.",
                                             svn.core.SVN_ERR_INCOMPLETE_DATA)
      self.assertRaises(svn.core.SubversionException, svn.client.info2, *args)

      # It must remain unchanged through the process.
      try:
        svn.client.info2(*args)
      except svn.core.SubversionException, exc:
        # find the original exception
        while exc.file != rec.e.file: exc = exc.child

        self.assertEqual(exc.message, rec.e.message)
        self.assertEqual(exc.apr_err, rec.e.apr_err)
        self.assertEqual(exc.line, rec.e.line)
        self.assertEqual(exc.child.message, rec.e.child.message)
        self.assertEqual(exc.child.apr_err, rec.e.child.apr_err)
        self.assertEqual(exc.child.child, None)
        self.assertEqual(exc.child.file, None)
        self.assertEqual(exc.child.line, 0)

      # Incomplete SubversionExceptions must trigger Python exceptions, which
      # will be passed through.
      rec.e = svn.core.SubversionException("No fields except message.")
      # e.apr_err is None but should be an int
      self.assertRaises(TypeError, svn.client.info2, args)
    finally:
      # This would happen without the finally block as well, but we expliticly
      # order the operations so that the cleanup is not hindered by any open
      # handles.
      del ctx
      t.cleanup()

def suite():
    return unittest.defaultTestLoader.loadTestsFromTestCase(
      SubversionCoreTestCase)

if __name__ == '__main__':
    runner = unittest.TextTestRunner()
    runner.run(suite())
