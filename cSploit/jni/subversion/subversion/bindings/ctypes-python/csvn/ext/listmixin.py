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


"""
Mixin class for easy creation of user lists.

Connelly Barnes 2005.  Public domain.

Use ListMixin to make classes which implement the Python list
interface.  Note that in many cases it is easier to subclass the
Python builtin list class.  If one subclasses list, then the data
is stored in-memory with the Python standard storage scheme.  The
ListMixin class is useful for different storage schemes, more
complicated data structures, or other nefarious hackery.

Features:
 - Compatible with Python 2.4, and Psyco.

Example:

 >>> import listmixin
 >>> class TestList(listmixin.ListMixin):
 ...   def __init__(self, L=[]):
 ...     self.L = list(L)
 ...   def _constructor(self, iterable):
 ...     return TestList(iterable)
 ...   def __len__(self):
 ...     return len(self.L)
 ...   def _get_element(self, i):
 ...     assert 0 <= i < len(self)
 ...     return self.L[i]
 ...   def _set_element(self, i, x):
 ...     assert 0 <= i < len(self)
 ...     self.L[i] = x
 ...   def _resize_region(self, start, end, new_size):
 ...     assert 0 <= start <= len(self)
 ...     assert 0 <= end   <= len(self)
 ...     self.L[start:end] = [None] * new_size
 ...
 >>> # Now TestList() has behavior identical to that of list().
 >>> x = TestList([1, 2, 3]) + TestList([4, 5, 6])
 >>> x.extend([9, 8])
 >>> x.append(5)
 >>> x[5:8] = [7, 8]
 >>> x[1:9:3] = [1, 2, 3]
 >>> list(x)
 [1, 1, 3, 4, 2, 7, 8, 3]

"""

__version__ = '1.0.3'

import copy
import sys

class ListMixin(object):
  """
  Defines all list operations from a small subset of methods.

  Subclasses should define _get_element(i), _set_element(i, value),
  __len__(), _resize_region(start, end, new_size) and
  _constructor(iterable).  Define __iter__() for extra speed.

  The _get_element() and _set_element() methods are given indices with
  0 <= i < len(self).

  The _resize_region() method should resize the slice self[start:end]
  so that it has size new_size.  It is given indices such that
  start <= end, 0 <= start <= len(self) and 0 <= end <= len(self).
  The resulting elements in self[start:start+new_size] can be set to
  None or arbitrary Python values.

  The _constructor() method accepts an iterable and should return a
  new instance of the same class as self, populated with the elements
  of the given iterable.
  """
  def __cmp__(self, other):
    return cmp(list(self), list(other))

  def __hash__(self):
    raise TypeError('list objects are unhashable')

  def __iter__(self):
    for i in range(len(self)):
      yield self._get_element(i)

  def _tuple_from_slice(self, i):
    """
    Get (start, end, step) tuple from slice object.
    """
    (start, end, step) = i.indices(len(self))
    # Replace (0, -1, 1) with (0, 0, 1) (misfeature in .indices()).
    if step == 1:
      if end < start:
        end = start
      step = None
    if i.step == None:
      step = None
    return (start, end, step)

  def _fix_index(self, i):
    if i < 0:
      i += len(self)
    if i < 0 or i >= len(self):
      raise IndexError('list index out of range')
    return i

  def __getitem__(self, i):
    if isinstance(i, slice):
      (start, end, step) = self._tuple_from_slice(i)
      if step == None:
        indices = range(start, end)
      else:
        indices = range(start, end, step)
      return self._constructor([self._get_element(i) for i in indices])
    else:
      return self._get_element(self._fix_index(i))

  def __setitem__(self, i, value):
    if isinstance(i, slice):
      (start, end, step) = self._tuple_from_slice(i)
      if step != None:
        # Extended slice
        indices = list(range(start, end, step))
        if len(value) != len(indices):
          raise ValueError(('attempt to assign sequence of size %d' +
                            ' to extended slice of size %d') %
                           (len(value), len(indices)))
        for (j, assign_val) in enumerate(value):
          self._set_element(indices[j], assign_val)
      else:
        # Normal slice
        if len(value) != (end - start):
          self._resize_region(start, end, len(value))
        for (j, assign_val) in enumerate(value):
          self._set_element(start + j, assign_val)
    else:
      # Single element
      self._set_element(self._fix_index(i), value)

  def __delitem__(self, i):
    if isinstance(i, slice):
      (start, end, step) = self._tuple_from_slice(i)
      if step != None:
        # Extended slice
        indices = list(range(start, end, step))
        # Sort indices descending
        if len(indices) > 0 and indices[0] < indices[-1]:
          indices.reverse()
        for j in indices:
          del self[j]
      else:
        # Normal slice
        self._resize_region(start, end, 0)
    else:
      # Single element
      i = self._fix_index(i)
      self._resize_region(i, i + 1, 0)

  def __add__(self, other):
    if isinstance(other, self.__class__):
      ans = self._constructor(self)
      ans += other
      return ans
    return list(self) + other

  def __mul__(self, other):
    ans = self._constructor(self)
    ans *= other
    return ans

  def __radd__(self, other):
    if isinstance(other, self.__class__):
      ans = other._constructor(self)
      ans += self
      return ans
    return other + list(self)

  def __rmul__(self, other):
    return self * other

  def __iadd__(self, other):
    self[len(self):len(self)] = other
    return self

  def __imul__(self, other):
    if other <= 0:
      self[:] = []
    elif other > 1:
      aux = list(self)
      for i in range(other-1):
        self.extend(aux)
    return self

  def append(self, other):
    self[len(self):len(self)] = [other]

  def extend(self, other):
    self[len(self):len(self)] = other

  def count(self, other):
    ans = 0
    for item in self:
      if item == other:
        ans += 1
    return ans

  def reverse(self):
    for i in range(len(self)//2):
      j = len(self) - 1 - i
      (self[i], self[j]) = (self[j], self[i])

  def index(self, x, i=0, j=None):
    if i != 0 or j is not None:
      (i, j, ignore) = self._tuple_from_slice(slice(i, j))
    if j is None:
      j = len(self)
    for k in range(i, j):
      if self._get_element(k) == x:
        return k
    raise ValueError('index(x): x not in list')

  def insert(self, i, x):
    self[i:i] = [x]

  def pop(self, i=None):
    if i == None:
      i = len(self)-1
    ans = self[i]
    del self[i]
    return ans

  def remove(self, x):
    for i in range(len(self)):
      if self._get_element(i) == x:
        del self[i]
        return
    raise ValueError('remove(x): x not in list')

  # Define sort() as appropriate for the Python version.
  if sys.version_info[:3] < (2, 4, 0):
    def sort(self, cmpfunc=None):
      ans = list(self)
      ans.sort(cmpfunc)
      self[:] = ans
  else:
    def sort(self, cmpfunc=None, key=None, reverse=False):
      ans = list(self)
      if reverse == True:
        ans.sort(cmpfunc, key, reverse)
      elif key != None:
        ans.sort(cmpfunc, key)
      else:
        ans.sort(cmpfunc)
      self[:] = ans

  def __copy__(self):
    return self._constructor(self)

  def __deepcopy__(self, memo={}):
    ans = self._constructor([])
    memo[id(self)] = ans
    ans[:] = copy.deepcopy(tuple(self), memo)
    return ans

  # Tracking idea from R. Hettinger's deque class.  It's not
  # multithread safe, but does work with the builtin Python classes.
  def __str__(self, track=[]):
    if id(self) in track:
      return '...'
    track.append(id(self))
    ans = '%r' % (list(self),)
    track.remove(id(self))
    return ans

  def __repr__(self):
    return self.__class__.__name__ + '(' + str(self) + ')'


class TestList(ListMixin):
  """
  Simple list class built using ListMixin.
  """
  def __init__(self, L=[]):
    self.L = list(L)

  def _constructor(self, iterable):
    return TestList(iterable)

  def __len__(self):
    return len(self.L)

  def _get_element(self, i):
    assert 0 <= i < len(self)
    return self.L[i]

  def _set_element(self, i, x):
    assert 0 <= i < len(self)
    self.L[i] = x

  def _resize_region(self, start, end, new_size):
    assert 0 <= start <= len(self)
    assert 0 <= end   <= len(self)
    assert start <= end
    self.L[start:end] = [None] * new_size

  def __iter__(self):
    return iter(self.L)


def test_list_mixin(list_class=TestList, rand_elem=None):
  """
  Unit test for ListMixin.
  """
  if list_class is TestList:
    L1 = TestList()
    L2 = TestList()
    L3 = TestList()
    L1.extend([L1, L2, L3])
    L2.append(L3)
    L3.append(L1)
    assert (repr(L1) == 'TestList([TestList(...), TestList([TestList'+
                   '([TestList(...)])]), TestList([TestList(...)])])')
    L1 = TestList()
    L1.append(L1)
    L2 = copy.deepcopy(L1)
    assert id(L1) == id(L1[0])
    assert id(L2) == id(L2[0])
    L3 = copy.copy(L2)
    assert id(L3) != id(L3[0])
    assert id(L3[0]) == id(L2[0]) == id(L2)

  if rand_elem is None:
    def rand_elem():
      return random.randrange(50)

  def get_or_none(obj, getindex):
    try:
      return obj[getindex]
    except IndexError:
      return None

  def set_or_none(obj, setindex, setvalue):
    try:
      obj[setindex] = setvalue
      return setvalue
    except IndexError:
      return None

  def extended_set_or_none(obj, setindex, setvalue):
    try:
      obj[setindex] = setvalue
      return setvalue
    except ValueError:
      return None

  def insert_or_none(obj, insertindex, insertvalue):
    try:
      obj.insert(insertindex, insertvalue)
    except IndexError:
      return None

  def pop_or_none(obj, *popargs):
    try:
      obj.pop(*popargs)
    except IndexError:
      return None

  def remove_or_none(obj, removevalue):
    try:
      obj.remove(removevalue)
    except IndexError:
      return None

  import random
  import sys
  for i in [1, 3, 8, 16, 18, 29, 59, 111, 213, 501, 1013,
            2021, 3122, 4039, 5054]:
    x = [rand_elem() for j in range(i)]
    y = list_class(x)

    assert isinstance(y[:], list_class)
    assert isinstance(y[:5], list_class)
    assert isinstance(y[:5:2], list_class)

    for j in range(100):
      r = random.randrange(13)
      if r == 0:
        # Set element at a random index
        if len(x) != 0:
          k = random.randrange(-2*len(x),2*len(x))
          v = rand_elem()
          assert set_or_none(x, k, v) == set_or_none(y, k, v)
      elif r == 1:
        # Delete element at random index
        if len(x) != 0:
          k = random.randrange(len(x))
          del x[k]
          del y[k]
      elif r == 2:
        # In place add some elements
        addelems = [rand_elem() for inx in range(random.randrange(4))]
        x += addelems
        y += addelems
      elif r == 3:
        # In place add an element
        addelem = rand_elem()
        x.append(addelem)
        y.append(addelem)
      elif r == 4:
        # In place add some elements
        addelems = [rand_elem() for inx in range(random.randrange(4))]
        x += addelems
        y += addelems
      elif r == 5:
        # Insert an element
        addelem = rand_elem()
        insertidx = random.randrange(-2*len(x), 2*len(x)+1)
        assert insert_or_none(x, insertidx, addelem) == \
               insert_or_none(y, insertidx, addelem)
      elif r == 6:
        # Pop an element
        popidx = random.randrange(-2*len(x), 2*len(x)+1)
        assert pop_or_none(x, popidx) == pop_or_none(y, popidx)
      elif r == 7:
        # Pop last element
        assert pop_or_none(x) == pop_or_none(y)
      elif r == 8:
        # Remove an element
        if len(x) != 0:
          remvalue = random.choice(x)
          assert remove_or_none(x, remvalue) == remove_or_none(y, remvalue)
      elif r == 9:
        if random.randrange(5) == 0:
          # Sort
          if sys.version_info[:3] >= (2, 4, 0):
            r2 = random.randrange(6)
          else:
            r2 = random.randrange(2)

          def keyfunc(keyitem):
            return (keyitem - 5) ** 2
          def cmpfunc(a, b):
            return cmp((a+9)**2, (b-5)**3)

          if r2 == 0:
            x.sort()
            y.sort()
          elif r2 == 1:
            x.sort(cmpfunc)
            y.sort(cmpfunc)
          elif r2 == 2:
            x.sort(cmpfunc, keyfunc)
            y.sort(cmpfunc, keyfunc)
          elif r2 == 3:
            x.sort(cmpfunc, keyfunc, True)
            y.sort(cmpfunc, keyfunc, True)
          elif r2 == 4:
            x.sort(cmpfunc, reverse=True)
            y.sort(cmpfunc, reverse=True)
          elif r2 == 5:
            x.sort(None, keyfunc, True)
            y.sort(None, keyfunc, True)
      elif r == 10:
        # Remove a slice
        start = random.randrange(-2*len(x), 2*len(x)+1)
        end   = random.randrange(-2*len(x), 2*len(x)+1)
        step  = random.randrange(-2*len(x), 2*len(x)+1)
        r2 = random.randrange(3)
        if r2 == 0:
          step = random.randrange(-5, 5)
        elif r2 == 1:
          step = 1
        if step == 0:
          step = 1
        del x[start:end:step]
        del y[start:end:step]
      elif r == 11:
        # Assign to a regular slice
        start = random.randrange(-2*len(x), 2*len(x)+1)
        end   = random.randrange(-2*len(x), 2*len(x)+1)
        assignval = [rand_elem() for assignidx in
                     range(random.randrange(10))]
        x[start:end] = assignval
        y[start:end] = assignval
      elif r == 12:
        # Assign to an extended slice
        start = random.randrange(-2*len(x), 2*len(x)+1)
        end   = random.randrange(-2*len(x), 2*len(x)+1)
        step  = random.randrange(-2*len(x), 2*len(x)+1)
        r2 = random.randrange(3)
        if r2 == 0:
          step = random.randrange(-5, 5)
        elif r2 == 1:
          step = 1
        if step == 0:
          step = 1
        if step == 1:
          step = 2
        indices = list(range(*slice(start, end, step).indices(len(x))))
        assignval = [rand_elem() for assignidx in
                     indices]
        if random.randrange(2) == 0:
          # Make right hand value have incorrect length
          if random.randrange(2) == 0 and len(assignval) > 0:
            if random.randrange(2) == 0:
              assignval.pop()
            else:
              assignval = []
          else:
            assignval.append(1)
        assert (extended_set_or_none(x, slice(start, end, step), assignval) ==
                extended_set_or_none(y, slice(start, end, step), assignval))

      # Check that x == y in a variety of ways.

      if len(x) != 0:
        for i4 in range(20):
          i3 = random.randrange(-2*len(x), 2*len(x))
          assert get_or_none(x, i3) == get_or_none(y, i3)

      assert list(iter(x)) == list(iter(y))
      assert list(iter(iter(x))) == list(iter(iter(y)))

      assert str(x) == str(y)

      assert x + [1,2] == y + [1,2]
      assert x * 0 == y * 0
      assert x * -1 == y * -1
      assert x * -5000 == y * -5000
      assert x * 1 == y * 1
      assert x * 2 == y * 2
      assert isinstance(x + y, list)
      assert x + y == x + list(y)
      elems = set(x)
      elems2 = set(y)
      assert elems == elems2
      def index_or_none(obj, search, *args):
        try:
          return obj.index(search, *args)
        except ValueError:
          return None
      for key in elems:
        assert x.count(key) == y.count(key)
        assert index_or_none(x, key) == index_or_none(y, key)
        i1 = random.randrange(-len(x)-2,len(x)+2)
        i2 = random.randrange(-len(x)-2,len(x)+2)
        assert index_or_none(x, key, i1, i2) == \
               index_or_none(y, key, i1, i2)
      assert x[:] == y[:]
      # Get slices
      for sliceidx in range(10):
        if len(x) != 0:
          start = random.randrange(-2*len(x), 2*len(x)+1)
          end   = random.randrange(-2*len(x), 2*len(x)+1)
          step  = random.randrange(-2*len(x), 2*len(x))
          r2 = random.randrange(3)
          if r2 == 0:
            step = random.randrange(-5, 5)
          elif r2 == 1:
            step = 1
          if step == 0:
            step = 1
          assert x[start:end:step] == y[start:end:step]
      assert cmp(x, y) == 0
      assert len(x) == len(y)
      assert x == y
      x.reverse()
      y.reverse()
      assert x == y
      x.reverse()
      y.reverse()
      for k in range(len(x)):
        assert x[k] == y[k]


def test(list_class=TestList):
  """
  Unit test main routine.
  """
  print('Testing:')
  test_list_mixin(list_class)
  print('  ListMixin:     OK')


if __name__ == '__main__':
  test()
