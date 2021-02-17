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


import setup_path
import unittest
from csvn.core import *
import csvn.types as _types

class TypesTestCase(unittest.TestCase):

    def test_hash(self):
        self.pydict = {"bruce":"batman", "clark":"superman",
            "vic":"the question"}
        self.svnhash = _types.Hash(c_char_p, self.pydict)
        self.assertEqual(self.svnhash["clark"].value,
            self.pydict["clark"])
        self.assertEqual(self.svnhash["vic"].value,
            self.pydict["vic"])
        self.assertNotEqual(self.svnhash["clark"].value,
            self.pydict["bruce"])

    def test_array(self):
        self.pyarray = ["vini", "vidi", "vici"]
        self.svnarray = _types.Array(c_char_p, self.pyarray)
        self.assertEqual(self.svnarray[0], "vini")
        self.assertEqual(self.svnarray[2], "vici")
        self.assertEqual(self.svnarray[1], "vidi")

def suite():
    return unittest.makeSuite(TypesTestCase, 'test')

if __name__ == '__main__':
    runner = unittest.TextTestRunner()
    runner.run(suite())
