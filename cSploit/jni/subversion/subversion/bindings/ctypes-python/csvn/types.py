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
from csvn.ext.listmixin import ListMixin
from UserDict import DictMixin
from shutil import copyfileobj
from tempfile import TemporaryFile

# This class contains Pythonic wrappers for generic Subversion and
# APR datatypes (e.g. dates, streams, hashes, arrays, etc).
#
# These wrappers are used by higher-level APIs in csvn to wrap
# raw C datatypes in a way that is easy for Python developers
# to use.

class SvnDate(str):

    def as_apr_time_t(self):
        """Return this date to an apr_time_t object"""
        pool = Pool()
        when = apr_time_t()
        svn_time_from_cstring(byref(when), self, pool)
        return when

    def as_human_string(self):
        """Return this date to a human-readable date"""
        pool = Pool()
        return str(svn_time_to_human_cstring(self.as_apr_time_t(), pool))

class Hash(DictMixin):
    """A dictionary wrapper for apr_hash_t"""

    _keys = DictMixin.iterkeys

    def __init__(self, type, items={}, wrapper=None, dup=None):
        self.type = type
        self.pool = Pool()
        self.wrapper = wrapper
        self.dup = dup
        if dup:
            self.hash = apr_hash_make(self.pool)
            if items is None or isinstance(items, POINTER(apr_hash_t)):
                items = Hash(type, items)
            self.update(items)
        elif isinstance(items, POINTER(apr_hash_t)):
            self.hash = items
        elif items is None:
            self.hash = POINTER(apr_hash_t)()
        elif isinstance(items, Hash):
            self.hash = items.hash
        else:
            self.hash = apr_hash_make(self.pool)
            self.update(items)

    def __getitem__(self, key):
        value = apr_hash_get(self, cast(key, c_void_p), len(key))
        if not value:
            raise KeyError(key)
        value = cast(value, self.type)
        if self.wrapper:
            value = self.wrapper.from_param(value)
        return value

    def __setitem__(self, key, value):
        if self.wrapper:
            value = self.wrapper.to_param(value, self.pool)
        if self.dup:
            value = self.dup(value, self.pool)
        apr_hash_set(self, apr_pstrdup(self.pool, key), len(key), value)

    def __delitem__(self, key):
        apr_hash_set(self, key, len(key), NULL)

    def keys(self):
        return list(self._keys())

    def __iter__(self):
        for (key, _) in self.items():
            yield key

    def items(self):
        pool = Pool()
        hi = apr_hash_first(pool, self)
        while hi:
            key_vp = c_void_p()
            val_vp = c_void_p()
            apr_hash_this(hi, byref(key_vp), None, byref(val_vp))
            val = cast(val_vp, self.type)
            if self.wrapper:
                val = self.wrapper.from_param(val)
            yield (string_at(key_vp), val)
            hi = apr_hash_next(hi)

    def __len__(self):
        return int(apr_hash_count(self))

    def byref(self):
        return byref(self._as_parameter_)

    def pointer(self):
        return pointer(self._as_parameter_)

    _as_parameter_ = property(fget=lambda self: self.hash)


class Array(ListMixin):
    """An array wrapper for apr_array_header_t"""

    def __init__(self, type, items=None, size=0):
        self.type = type
        self.pool = Pool()
        if not items:
            self.header = apr_array_make(self.pool, size, sizeof(type))
        elif isinstance(items, POINTER(apr_array_header_t)):
            self.header = items
        elif isinstance(items, Array):
            self.header = apr_array_copy(self.pool, items)
        else:
            self.header = apr_array_make(self.pool, len(items),
                                         sizeof(type))
            self.extend(items)

    _as_parameter_ = property(fget=lambda self: self.header)
    elts = property(fget=lambda self: cast(self.header[0].elts.raw,
                                           POINTER(self.type)))

    def _get_element(self, i):
        return self.elts[i]

    def _set_element(self, i, value):
        self.elts[i] = value

    def __len__(self):
        return self.header[0].nelts

    def _resize_region(self, start, end, new_size):
        diff = start-end+new_size

        # Growing
        if diff > 0:
            l = len(self)

            # Make space for the new items
            for i in range(diff):
                apr_array_push(self)

            # Move the old items out of the way, if necessary
            if end < l:
                src_idx = max(end-diff,0)
                memmove(byref(self.elts + end),
                        byref(self.elts[src_idx]),
                        (l-src_idx)*self.header[0].elt_size)

        # Shrinking
        elif diff < 0:

            # Overwrite the deleted items with items we still need
            if end < len(self):
                memmove(byref(self.elts[end+diff]),
                        byref(self.elts[end]),
                        (len(self)-end)*self.header[0].elt_size)

            # Shrink the array
            for i in range(-diff):
                apr_array_pop(self)

try:
    # On Windows we need to do some magic to get the os-level file handle
    from msvcrt import get_osfhandle
except ImportError:
    get_osfhandle = lambda fileno: fileno

class APRFile(object):
    """Wrap a Python file-like object as an APR File"""

    def __init__(self, pyfile):
        self.pyfile = pyfile
        self.pool = Pool()
        self._as_parameter_ = POINTER(apr_file_t)()
        self.tempfile = None
        if hasattr(pyfile, "fileno"):
            # Looks like this is a real file. We can just write
            # directly to said file
            osfile = apr_os_file_t(get_osfhandle(pyfile.fileno()))
        else:
            # Looks like this is a StringIO buffer or a fake file.
            # Write to a temporary file and copy the output to the
            # buffer when we are closed or flushed
            self.tempfile = TemporaryFile()
            osfile = apr_os_file_t(get_osfhandle(self.tempfile.fileno()))
        apr_os_file_put(byref(self._as_parameter_), byref(osfile),
                        APR_CREATE | APR_WRITE | APR_BINARY, self.pool)

    def flush(self):
        """Flush output to the underlying Python object"""
        if self.tempfile:
            self.tempfile.seek(0)
            copyfileobj(self.tempfile, self.pyfile)
            self.tempfile.truncate(0)

    def close(self):
        """Close the APR file wrapper, leaving the underlying Python object
           untouched"""
        self.flush()
        if self.tempfile:
            self.tempfile.close()
            self.tempfile = None
        self.pool.destroy()
        self.pool = None

    def __del__(self):
        if self.pool:
            self.close()


class Stream(object):

    def __init__(self, buffer, disown=False):
        """Create a stream which wraps a Python file or file-like object"""

        self.pool = Pool()
        self.buffer = buffer
        self.stream = svn_stream_create(c_void_p(), self.pool)
        svn_stream_set_read(self.stream, svn_read_fn_t(self._read))
        svn_stream_set_write(self.stream, svn_write_fn_t(self._write))
        if not disown:
            svn_stream_set_close(self.stream, svn_close_fn_t(self._close))

    _as_parameter_ = property(fget=lambda self: self.stream)

    def _read(self, baton, buffer, l):
        s = self.buffer.read(l[0])
        memmove(buffer, string, len(s))
        l[0] = len(s)
        return SVN_NO_ERROR

    def _write(self, baton, data, l):
        s = string_at(data.raw, l[0])
        self.buffer.write(s)
        return SVN_NO_ERROR

    def _close(self, baton):
        self.buffer.close()
        return SVN_NO_ERROR

class SvnStringPtr(object):

    def to_param(obj, pool):
        return svn_string_ncreate(obj, len(obj), pool)
    to_param = staticmethod(to_param)

    def from_param(obj):

        assert isinstance(obj[0], svn_string_t)

        # Convert from a raw svn_string_t object. Pass in the length, so that
        # we handle binary property values with embedded NULLs correctly.
        return string_at(obj[0].data.raw, obj[0].len)
    from_param = staticmethod(from_param)

