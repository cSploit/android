#
#  testcase.py:  Control of test case execution.
#
#  Subversion is a tool for revision control.
#  See http://subversion.tigris.org for more information.
#
# ====================================================================
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

import os, types, sys

import svntest

# if somebody does a "from testcase import *", they only get these names
__all__ = ['_XFail', '_Wimp', '_Skip', '_SkipUnless']

RESULT_OK = 'ok'
RESULT_FAIL = 'fail'
RESULT_SKIP = 'skip'


class TextColors:
  '''Some ANSI terminal constants for output color'''
  ENDC = '\033[0;m'
  FAILURE = '\033[1;31m'
  SUCCESS = '\033[1;32m'

  @classmethod
  def disable(cls):
    cls.ENDC = ''
    cls.FAILURE = ''
    cls.SUCCESS = ''

  @classmethod
  def success(cls, str):
    return lambda: cls.SUCCESS + str + cls.ENDC

  @classmethod
  def failure(cls, str):
    return lambda: cls.FAILURE + str + cls.ENDC


if not sys.stdout.isatty() or sys.platform == 'win32':
  TextColors.disable()


class TestCase:
  """A thing that can be tested.  This is an abstract class with
  several methods that need to be overridden."""

  _result_map = {
    RESULT_OK:   (0, TextColors.success('PASS: '), True),
    RESULT_FAIL: (1, TextColors.failure('FAIL: '), False),
    RESULT_SKIP: (2, TextColors.success('SKIP: '), True),
    }

  def __init__(self, delegate=None, cond_func=lambda: True, doc=None, wip=None,
               issues=None):
    """Create a test case instance based on DELEGATE.

    COND_FUNC is a callable that is evaluated at test run time and should
    return a boolean value that determines how a pass or failure is
    interpreted: see the specialized kinds of test case such as XFail and
    Skip for details.  The evaluation of COND_FUNC is deferred so that it
    can base its decision on useful bits of information that are not
    available at __init__ time (like the fact that we're running over a
    particular RA layer).

    DOC is ...

    WIP is a string describing the reason for the work-in-progress
    """
    assert hasattr(cond_func, '__call__')

    self._delegate = delegate
    self._cond_func = cond_func
    self.description = doc or delegate.description
    self.inprogress = wip
    self.issues = issues

  def get_function_name(self):
    """Return the name of the python function implementing the test."""
    return self._delegate.get_function_name()

  def get_sandbox_name(self):
    """Return the name that should be used for the sandbox.

    If a sandbox should not be constructed, this method returns None.
    """
    return self._delegate.get_sandbox_name()

  def set_issues(self, issues):
    """Set the issues associated with this test."""
    self.issues = issues

  def run(self, sandbox):
    """Run the test within the given sandbox."""
    return self._delegate.run(sandbox)

  def list_mode(self):
    return ''

  def results(self, result):
    # if our condition applied, then use our result map. otherwise, delegate.
    if self._cond_func():
      val = list(self._result_map[result])
      val[1] = val[1]()
      return val
    return self._delegate.results(result)


class FunctionTestCase(TestCase):
  """A TestCase based on a naked Python function object.

  FUNC should be a function that returns None on success and throws an
  svntest.Failure exception on failure.  It should have a brief
  docstring describing what it does (and fulfilling certain conditions).
  FUNC must take one argument, an Sandbox instance.  (The sandbox name
  is derived from the file name in which FUNC was defined)
  """

  def __init__(self, func, issues=None):
    # it better be a function that accepts an sbox parameter and has a
    # docstring on it.
    assert isinstance(func, types.FunctionType)

    name = func.func_name

    assert func.func_code.co_argcount == 1, \
        '%s must take an sbox argument' % name

    doc = func.__doc__.strip()
    assert doc, '%s must have a docstring' % name

    # enforce stylistic guidelines for the function docstrings:
    # - no longer than 50 characters
    # - should not end in a period
    # - should not be capitalized
    assert len(doc) <= 50, \
        "%s's docstring must be 50 characters or less" % name
    assert doc[-1] != '.', \
        "%s's docstring should not end in a period" % name
    assert doc[0].lower() == doc[0], \
        "%s's docstring should not be capitalized" % name

    TestCase.__init__(self, doc=doc, issues=issues)
    self.func = func

  def get_function_name(self):
    return self.func.func_name

  def get_sandbox_name(self):
    """Base the sandbox's name on the name of the file in which the
    function was defined."""

    filename = self.func.func_code.co_filename
    return os.path.splitext(os.path.basename(filename))[0]

  def run(self, sandbox):
    return self.func(sandbox)


class _XFail(TestCase):
  """A test that is expected to fail, if its condition is true."""

  _result_map = {
    RESULT_OK:   (1, TextColors.failure('XPASS:'), False),
    RESULT_FAIL: (0, TextColors.success('XFAIL:'), True),
    RESULT_SKIP: (2, TextColors.success('SKIP: '), True),
    }

  def __init__(self, test_case, cond_func=lambda: True, wip=None,
               issues=None):
    """Create an XFail instance based on TEST_CASE.  COND_FUNC is a
    callable that is evaluated at test run time and should return a
    boolean value.  If COND_FUNC returns true, then TEST_CASE is
    expected to fail (and a pass is considered an error); otherwise,
    TEST_CASE is run normally.  The evaluation of COND_FUNC is
    deferred so that it can base its decision on useful bits of
    information that are not available at __init__ time (like the fact
    that we're running over a particular RA layer).

    WIP is ...

    ISSUES is an issue number (or a list of issue numbers) tracking this."""

    TestCase.__init__(self, create_test_case(test_case), cond_func, wip=wip,
                      issues=issues)

  def list_mode(self):
    # basically, the only possible delegate is a Skip test. favor that mode.
    return self._delegate.list_mode() or 'XFAIL'


class _Wimp(_XFail):
  """Like XFail, but indicates a work-in-progress: an unexpected pass
  is not considered a test failure."""

  _result_map = {
    RESULT_OK:   (0, TextColors.success('XPASS:'), True),
    RESULT_FAIL: (0, TextColors.success('XFAIL:'), True),
    RESULT_SKIP: (2, TextColors.success('SKIP: '), True),
    }

  def __init__(self, wip, test_case, cond_func=lambda: True):
    _XFail.__init__(self, test_case, cond_func, wip)


class _Skip(TestCase):
  """A test that will be skipped if its conditional is true."""

  def __init__(self, test_case, cond_func=lambda: True, issues=None):
    """Create a Skip instance based on TEST_CASE.  COND_FUNC is a
    callable that is evaluated at test run time and should return a
    boolean value.  If COND_FUNC returns true, then TEST_CASE is
    skipped; otherwise, TEST_CASE is run normally.
    The evaluation of COND_FUNC is deferred so that it can base its
    decision on useful bits of information that are not available at
    __init__ time (like the fact that we're running over a
    particular RA layer)."""

    TestCase.__init__(self, create_test_case(test_case), cond_func,
                      issues=issues)

  def list_mode(self):
    if self._cond_func():
      return 'SKIP'
    return self._delegate.list_mode()

  def get_sandbox_name(self):
    if self._cond_func():
      return None
    return self._delegate.get_sandbox_name()

  def run(self, sandbox):
    if self._cond_func():
      raise svntest.Skip
    return self._delegate.run(sandbox)


class _SkipUnless(_Skip):
  """A test that will be skipped if its conditional is false."""

  def __init__(self, test_case, cond_func):
    _Skip.__init__(self, test_case, lambda c=cond_func: not c())


def create_test_case(func, issues=None):
  if isinstance(func, TestCase):
    return func
  else:
    return FunctionTestCase(func, issues=issues)


# Various decorators to make declaring tests as such simpler
def XFail_deco(cond_func = lambda: True):
  def _second(func):
    if isinstance(func, TestCase):
      return _XFail(func, cond_func, issues=func.issues)
    else:
      return _XFail(func, cond_func)

  return _second


def Wimp_deco(wip, cond_func = lambda: True):
  def _second(func):
    if isinstance(func, TestCase):
      return _Wimp(wip, func, cond_func, issues=func.issues)
    else:
      return _Wimp(wip, func, cond_func)

  return _second


def Skip_deco(cond_func = lambda: True):
  def _second(func):
    if isinstance(func, TestCase):
      return _Skip(func, cond_func, issues=func.issues)
    else:
      return _Skip(func, cond_func)

  return _second


def SkipUnless_deco(cond_func):
  def _second(func):
    if isinstance(func, TestCase):
      return _Skip(func, lambda c=cond_func: not c(), issues=func.issues)
    else:
      return _Skip(func, lambda c=cond_func: not c())

  return _second


def Issues_deco(*issues):
  def _second(func):
    if isinstance(func, TestCase):
      # if the wrapped thing is already a test case, just set the issues
      func.set_issues(issues)
      return func

    else:
      # we need to wrap the function
      return create_test_case(func, issues=issues)

  return _second

# Create a singular alias, for linguistic correctness
Issue_deco = Issues_deco
