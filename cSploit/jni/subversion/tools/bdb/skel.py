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
# Python parser for Subversion skels

import string, re

def parse(s):
  if s[0] != '(' and s[-1] != ')':
    raise ValueError("Improperly bounded skel: '%s'" % s)
  wholeskel = s
  s = s[1:-1].lstrip()
  prev_accums = []
  accum = []
  while True:
    if len(s) == 0:
      return accum
    if s[0] in string.digits:
      split_tuple = s.split(' ',1)
      count = int(split_tuple[0])
      if len(split_tuple) > 1:
        s = split_tuple[1]
      else:
        s = ""
      accum.append(s[:count])
      s = s[count:].lstrip()
      continue
    if s[0] in string.ascii_letters:
      i = 0
      while (s[i] not in ' ()'):
        i += 1
        if i == len(s):
          break
      accum.append(s[:i])
      s = s[i:].lstrip()
      continue
    if s[0] == '(':
      new_accum = []
      accum.append(new_accum)
      prev_accums.append(accum)
      accum = new_accum
      s = s[1:].lstrip()
      continue
    if s[0] == ')':
      accum = prev_accums.pop()
      s = s[1:].lstrip()
      continue
    if s[0] == ' ':
      s = str.lstrip(' ')
      continue
    raise ValueError("Unexpected contents in skel: '%s'\n'%s'" % (s, wholeskel))


_ok_implicit = re.compile(r'^[A-Za-z]([^\(\) \r\n\t\f]*)$')
def unparse(struc):
  accum = []
  for ent in struc:
    if isinstance(ent, str):
      if len(ent) > 0 and _ok_implicit.match(ent[0]):
        accum.append(ent)
      else:
        accum.append(str(len(ent)))
        accum.append(ent)
    else:
      accum.append(unparse(ent))
  return "("+" ".join(accum)+")"


class Rev:
  def __init__(self, skelstring="(revision null)"):
    sk = parse(skelstring)
    if len(sk) == 2 and sk[0] == "revision" and isinstance(sk[1], str):
      self.txn = sk[1]
    else:
      raise ValueError("Invalid revision skel: %s" % skelstring)

  def unparse(self):
    return unparse( ("revision", self.txn) )


class Change:
  def __init__(self, skelstring="(change null null null 0  0 )"):
    sk = parse(skelstring)
    if len(sk) == 6 and sk[0] == "change" and type(sk[1]) == type(sk[2]) \
        == type(sk[3]) == type(sk[4]) == type(sk[5]) == str:
          self.path = sk[1]
          self.node = sk[2]
          self.kind = sk[3]
          self.textmod = sk[4]
          self.propmod = sk[5]
    else:
      raise ValueError("Invalid change skel: %s" % skelstring)

  def unparse(self):
    return unparse( ("change", self.path, self.node, self.kind,
                     self.textmod and "1" or "", self.propmod and "1" or "") )


class Copy:
  def __init__(self, skelstring="(copy null null null)"):
    sk = parse(skelstring)
    if len(sk) == 4 and sk[0] in ("copy", "soft-copy") and type(sk[1]) \
        == type(sk[2]) == type(sk[3]) == str:
          self.kind = sk[0]
          self.srcpath = sk[1]
          self.srctxn = sk[2]
          self.destnode = sk[3]
    else:
      raise ValueError("Invalid copy skel: %s" % skelstring)

  def unparse(self):
    return unparse( (self.kind, self.srcpath, self.srctxn, self.destnode) )


class Node:
  def __init__(self,skelstring="((file null null 1 0) null null)"):
    sk = parse(skelstring)
    if (len(sk) == 3 or (len(sk) == 4 and isinstance(sk[3], str))) \
        and isinstance(sk[0], list) and isinstance(sk[1], str) \
        and isinstance(sk[2], str) and sk[0][0] in ("file", "dir") \
        and type(sk[0][1]) == type(sk[0][2]) == type(sk[0][3]) == str:
          self.kind = sk[0][0]
          self.createpath = sk[0][1]
          self.prednode = sk[0][2]
          self.predcount = int(sk[0][3])
          self.proprep = sk[1]
          self.datarep = sk[2]
          if len(sk) > 3:
            self.editrep = sk[3]
          else:
            self.editrep = None
    else:
      raise ValueError("Invalid node skel: %s" % skelstring)

  def unparse(self):
    structure = [ (self.kind, self.createpath, self.prednode,
        str(self.predcount)), self.proprep, self.datarep ]
    if self.editrep:
      structure.append(self.editrep)
    return unparse( structure )


class Txn:
  def __init__(self,skelstring="(transaction null null () ())"):
    sk = parse(skelstring)
    if len(sk) == 5 and sk[0] in ("transaction", "committed", "dead") \
        and type(sk[1]) == type(sk[2]) == str \
        and type(sk[3]) == type(sk[4]) == list and len(sk[3]) % 2 == 0:
          self.kind = sk[0]
          self.rootnode = sk[1]
          if self.kind == "committed":
            self.rev = sk[2]
          else:
            self.basenode = sk[2]
          self.proplist = sk[3]
          self.copies = sk[4]
    else:
      raise ValueError("Invalid transaction skel: %s" % skelstring)

  def unparse(self):
    if self.kind == "committed":
      base_item = self.rev
    else:
      base_item = self.basenode
    return unparse( (self.kind, self.rootnode, base_item, self.proplist,
      self.copies) )


class SvnDiffWindow:
  def __init__(self, skelstructure):
    self.offset = skelstructure[0]
    self.svndiffver = skelstructure[1][0][1]
    self.str = skelstructure[1][0][2]
    self.size = skelstructure[1][1]
    self.vs_rep = skelstructure[1][2]

  def _unparse_structure(self):
    return ([ self.offset, [ [ 'svndiff', self.svndiffver, self.str ],
        self.size, self.vs_rep ] ])


class Rep:
  def __init__(self, skelstring="((fulltext 0  (md5 16 \0\0\0\0\0\0\0\0" \
          "\0\0\0\0\0\0\0\0)) null)"):
    sk = parse(skelstring)
    if isinstance(sk[0], list) and len(sk[0]) == 3 \
        and isinstance(sk[0][1], str) \
        and isinstance(sk[0][2], list) and len(sk[0][2]) == 2 \
        and type(sk[0][2][0]) == type(sk[0][2][1]) == str:
          self.kind = sk[0][0]
          self.txn = sk[0][1]
          self.cksumtype = sk[0][2][0]
          self.cksum = sk[0][2][1]
          if len(sk) == 2 and sk[0][0] == "fulltext":
            self.str = sk[1]
          elif len(sk) >= 2 and sk[0][0] == "delta":
            self.windows = list(map(SvnDiffWindow, sk[1:]))
    else:
      raise ValueError("Invalid representation skel: %s" % repr(skelstring))

  def unparse(self):
    structure = [ [self.kind, self.txn, [self.cksumtype, self.cksum] ] ]
    if self.kind == "fulltext":
      structure.append(self.str)
    elif self.kind == "delta":
      for w in self.windows:
        structure.append(w._unparse_structure())
    return unparse( structure )

