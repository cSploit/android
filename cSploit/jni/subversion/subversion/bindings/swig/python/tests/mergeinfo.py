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
import unittest, os, sys, gc
from svn import core, repos, fs
import utils

class RevRange:
  """ Proxy object for a revision range, used for comparison. """

  def __init__(self, start, end):
    self.start = start
    self.end = end

def get_svn_merge_range_t_objects():
  """Returns a list 'svn_merge_range_t' objects being tracked by the
     garbage collector, used for detecting memory leaks."""
  return [
    o for o in gc.get_objects()
      if hasattr(o, '__class__') and
        o.__class__.__name__ == 'svn_merge_range_t'
  ]

class SubversionMergeinfoTestCase(unittest.TestCase):
  """Test cases for mergeinfo"""

  # Some textual mergeinfo.
  TEXT_MERGEINFO1 = "/trunk:3-9,27,42*"
  TEXT_MERGEINFO2 = "/trunk:27-29,41-43*"

  # Meta data used in conjunction with this mergeinfo.
  MERGEINFO_SRC = "/trunk"
  MERGEINFO_NBR_REV_RANGES = 3

  def setUp(self):
    """Load the mergeinfo-full Subversion repository.  This dumpfile is
       created by dumping the repository generated for command line log
       tests 16.  If it needs to be updated (mergeinfo format changes, for
       example), we can go there to get a new version."""
    self.temper = utils.Temper()
    (self.repos, _, _) = self.temper.alloc_known_repo('data/mergeinfo.dump',
                                                      suffix='-mergeinfo')
    self.fs = repos.fs(self.repos)
    self.rev = fs.youngest_rev(self.fs)

  def tearDown(self):
    del self.fs
    del self.repos
    self.temper.cleanup()

  def test_mergeinfo_parse(self):
    """Test svn_mergeinfo_parse()"""
    mergeinfo = core.svn_mergeinfo_parse(self.TEXT_MERGEINFO1)
    self.inspect_mergeinfo_dict(mergeinfo, self.MERGEINFO_SRC,
                                self.MERGEINFO_NBR_REV_RANGES)

  def test_rangelist_merge(self):
    mergeinfo1 = core.svn_mergeinfo_parse(self.TEXT_MERGEINFO1)
    mergeinfo2 = core.svn_mergeinfo_parse(self.TEXT_MERGEINFO2)
    rangelist1 = mergeinfo1.get(self.MERGEINFO_SRC)
    rangelist2 = mergeinfo2.get(self.MERGEINFO_SRC)
    rangelist3 = core.svn_rangelist_merge(rangelist1, rangelist2)
    self.inspect_rangelist_tuple(rangelist3, 3)

  def test_mergeinfo_merge(self):
    """Test svn_mergeinfo_merge()"""
    mergeinfo1 = core.svn_mergeinfo_parse(self.TEXT_MERGEINFO1)
    mergeinfo2 = core.svn_mergeinfo_parse(self.TEXT_MERGEINFO2)
    mergeinfo3 = core.svn_mergeinfo_merge(mergeinfo1, mergeinfo2)
    self.inspect_mergeinfo_dict(mergeinfo3, self.MERGEINFO_SRC,
                                self.MERGEINFO_NBR_REV_RANGES)

  def test_rangelist_reverse(self):
    mergeinfo = core.svn_mergeinfo_parse(self.TEXT_MERGEINFO1)
    rangelist = mergeinfo.get(self.MERGEINFO_SRC)
    reversed_rl = core.svn_rangelist_reverse(rangelist)
    expected_ranges = ((42, 41), (27, 26), (9, 2))
    for i in range(0, len(reversed_rl)):
      self.assertEquals(reversed_rl[i].start, expected_ranges[i][0],
                        "Unexpected range start: %d" % reversed_rl[i].start)
      self.assertEquals(reversed_rl[i].end, expected_ranges[i][1],
                        "Unexpected range end: %d" % reversed_rl[i].end)

  def test_mergeinfo_sort(self):
    mergeinfo = core.svn_mergeinfo_parse(self.TEXT_MERGEINFO1)

    # Swap the order of two revision ranges to misorder the contents
    # of a rangelist.
    rangelist = mergeinfo.get(self.MERGEINFO_SRC)
    rev_range = rangelist[0]
    rangelist[0] = rangelist[1]
    rangelist[1] = rev_range

    mergeinfo = core.svn_mergeinfo_sort(mergeinfo)
    self.inspect_mergeinfo_dict(mergeinfo, self.MERGEINFO_SRC,
                                self.MERGEINFO_NBR_REV_RANGES)

  def test_mergeinfo_get(self):
    mergeinfo = repos.fs_get_mergeinfo(self.repos, ['/trunk'], self.rev,
                                       core.svn_mergeinfo_inherited,
                                       False, None, None)
    expected_mergeinfo = \
      { '/trunk' :
          { '/branches/a' : [RevRange(2, 11)],
            '/branches/b' : [RevRange(9, 13)],
            '/branches/c' : [RevRange(2, 16)],
            '/trunk'      : [RevRange(1, 9)],  },
      }
    self.compare_mergeinfo_catalogs(mergeinfo, expected_mergeinfo)

  def test_mergeinfo_leakage__incorrect_range_t_refcounts(self):
    """Ensure that the ref counts on svn_merge_range_t objects returned by
       svn_mergeinfo_parse() are correct."""
    # When reference counting is working properly, each svn_merge_range_t in
    # the returned mergeinfo will have a ref count of 1...
    mergeinfo = core.svn_mergeinfo_parse(self.TEXT_MERGEINFO1)
    for (path, rangelist) in mergeinfo.items():
      # ....and now 2 (incref during iteration of rangelist)

      for (i, r) in enumerate(rangelist):
        # ....and now 3 (incref during iteration of each range object)

        refcount = sys.getrefcount(r)
        # ....and finally, 4 (getrefcount() also increfs)
        expected = 4

        # Note: if path and index are not '/trunk' and 0 respectively, then
        # only some of the range objects are leaking, which is, as far as
        # leaks go, even more impressive.
        self.assertEquals(refcount, expected, (
          "Memory leak!  Expected a ref count of %d for svn_merge_range_t "
          "object, but got %d instead (path: %s, index: %d).  Probable "
          "cause: incorrect Py_INCREF/Py_DECREF usage in libsvn_swig_py/"
          "swigutil_py.c." % (expected, refcount, path, i)))

    del mergeinfo
    gc.collect()

  def test_mergeinfo_leakage__lingering_range_t_objects_after_del(self):
    """Ensure that there are no svn_merge_range_t objects being tracked by
       the garbage collector after we explicitly `del` the results returned
       by svn_mergeinfo_parse().  We call gc.collect() to force an explicit
       garbage collection cycle after the `del`;
       if our reference counts are correct, the allocated svn_merge_range_t
       objects will be garbage collected and thus, not appear in the list of
       objects returned by gc.get_objects()."""
    mergeinfo = core.svn_mergeinfo_parse(self.TEXT_MERGEINFO1)
    del mergeinfo
    gc.collect()
    lingering = get_svn_merge_range_t_objects()
    self.assertEquals(lingering, list(), (
      "Memory leak!  Found lingering svn_merge_range_t objects left over from "
      "our call to svn_mergeinfo_parse(), even though we explicitly deleted "
      "the returned mergeinfo object.  Probable cause: incorrect Py_INCREF/"
      "Py_DECREF usage in libsvn_swig_py/swigutil_py.c.  Lingering objects:\n"
      "%s" % lingering))

  def inspect_mergeinfo_dict(self, mergeinfo, merge_source, nbr_rev_ranges):
    rangelist = mergeinfo.get(merge_source)
    self.inspect_rangelist_tuple(rangelist, nbr_rev_ranges)

  def inspect_rangelist_tuple(self, rangelist, nbr_rev_ranges):
    self.assert_(rangelist is not None,
                 "Rangelist for '%s' not parsed" % self.MERGEINFO_SRC)
    self.assertEquals(len(rangelist), nbr_rev_ranges,
                      "Wrong number of revision ranges parsed")
    self.assertEquals(rangelist[0].inheritable, True,
                      "Unexpected revision range 'non-inheritable' flag: %s" %
                      rangelist[0].inheritable)
    self.assertEquals(rangelist[1].start, 26,
                      "Unexpected revision range end: %d" % rangelist[1].start)
    self.assertEquals(rangelist[2].inheritable, False,
                      "Missing revision range 'non-inheritable' flag")

  def compare_mergeinfo_catalogs(self, catalog1, catalog2):
    keys1 = sorted(catalog1.keys())
    keys2 = sorted(catalog2.keys())
    self.assertEqual(keys1, keys2)

    for k in catalog1.keys():
        self.compare_mergeinfos(catalog1[k], catalog2[k])

  def compare_mergeinfos(self, mergeinfo1, mergeinfo2):
    keys1 = sorted(mergeinfo1.keys())
    keys2 = sorted(mergeinfo2.keys())
    self.assertEqual(keys1, keys2)

    for k in mergeinfo1.keys():
        self.compare_rangelists(mergeinfo1[k], mergeinfo2[k])

  def compare_rangelists(self, rangelist1, rangelist2):
    self.assertEqual(len(rangelist1), len(rangelist2))
    for r1, r2 in zip(rangelist1, rangelist2):
        self.assertEqual(r1.start, r2.start)
        self.assertEqual(r1.end, r2.end)


def suite():
    return unittest.defaultTestLoader.loadTestsFromTestCase(
      SubversionMergeinfoTestCase)

if __name__ == '__main__':
    runner = unittest.TextTestRunner()
    runner.run(suite())
