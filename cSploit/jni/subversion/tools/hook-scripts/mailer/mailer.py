#!/usr/bin/env python
# -*- coding: utf-8 -*-
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
# mailer.py: send email describing a commit
#
# $HeadURL: http://svn.apache.org/repos/asf/subversion/branches/1.7.x/tools/hook-scripts/mailer/mailer.py $
# $LastChangedDate: 2010-12-30 20:46:50 +0000 (Thu, 30 Dec 2010) $
# $LastChangedBy: hwright $
# $LastChangedRevision: 1053998 $
#
# USAGE: mailer.py commit      REPOS REVISION [CONFIG-FILE]
#        mailer.py propchange  REPOS REVISION AUTHOR REVPROPNAME [CONFIG-FILE]
#        mailer.py propchange2 REPOS REVISION AUTHOR REVPROPNAME ACTION \
#                              [CONFIG-FILE]
#        mailer.py lock        REPOS AUTHOR [CONFIG-FILE]
#        mailer.py unlock      REPOS AUTHOR [CONFIG-FILE]
#
#   Using CONFIG-FILE, deliver an email describing the changes between
#   REV and REV-1 for the repository REPOS.
#
#   ACTION was added as a fifth argument to the post-revprop-change hook
#   in Subversion 1.2.0.  Its value is one of 'A', 'M' or 'D' to indicate
#   if the property was added, modified or deleted, respectively.
#
#   See _MIN_SVN_VERSION below for which version of Subversion's Python
#   bindings are required by this version of mailer.py.

import os
import sys
try:
  # Python >=3.0
  import configparser
  from urllib.parse import quote as urllib_parse_quote
except ImportError:
  # Python <3.0
  import ConfigParser as configparser
  from urllib import quote as urllib_parse_quote
import time
import subprocess
if sys.version_info[0] >= 3:
  # Python >=3.0
  from io import StringIO
else:
  # Python <3.0
  from cStringIO import StringIO
import smtplib
import re
import tempfile

# Minimal version of Subversion's bindings required
_MIN_SVN_VERSION = [1, 5, 0]

# Import the Subversion Python bindings, making sure they meet our
# minimum version requirements.
try:
  import svn.fs
  import svn.delta
  import svn.repos
  import svn.core
except ImportError:
  sys.stderr.write(
    "You need version %s or better of the Subversion Python bindings.\n" \
    % ".".join([str(x) for x in _MIN_SVN_VERSION]))
  sys.exit(1)
if _MIN_SVN_VERSION > [svn.core.SVN_VER_MAJOR,
                       svn.core.SVN_VER_MINOR,
                       svn.core.SVN_VER_PATCH]:
  sys.stderr.write(
    "You need version %s or better of the Subversion Python bindings.\n" \
    % ".".join([str(x) for x in _MIN_SVN_VERSION]))
  sys.exit(1)


SEPARATOR = '=' * 78

def main(pool, cmd, config_fname, repos_dir, cmd_args):
  ### TODO:  Sanity check the incoming args

  if cmd == 'commit':
    revision = int(cmd_args[0])
    repos = Repository(repos_dir, revision, pool)
    cfg = Config(config_fname, repos, { 'author' : repos.author })
    messenger = Commit(pool, cfg, repos)
  elif cmd == 'propchange' or cmd == 'propchange2':
    revision = int(cmd_args[0])
    author = cmd_args[1]
    propname = cmd_args[2]
    action = (cmd == 'propchange2' and cmd_args[3] or 'A')
    repos = Repository(repos_dir, revision, pool)
    # Override the repos revision author with the author of the propchange
    repos.author = author
    cfg = Config(config_fname, repos, { 'author' : author })
    messenger = PropChange(pool, cfg, repos, author, propname, action)
  elif cmd == 'lock' or cmd == 'unlock':
    author = cmd_args[0]
    repos = Repository(repos_dir, 0, pool) ### any old revision will do
    # Override the repos revision author with the author of the lock/unlock
    repos.author = author
    cfg = Config(config_fname, repos, { 'author' : author })
    messenger = Lock(pool, cfg, repos, author, cmd == 'lock')
  else:
    raise UnknownSubcommand(cmd)

  messenger.generate()


def remove_leading_slashes(path):
  while path and path[0] == '/':
    path = path[1:]
  return path


class OutputBase:
  "Abstract base class to formalize the interface of output methods"

  def __init__(self, cfg, repos, prefix_param):
    self.cfg = cfg
    self.repos = repos
    self.prefix_param = prefix_param
    self._CHUNKSIZE = 128 * 1024

    # This is a public member variable. This must be assigned a suitable
    # piece of descriptive text before make_subject() is called.
    self.subject = ""

  def make_subject(self, group, params):
    prefix = self.cfg.get(self.prefix_param, group, params)
    if prefix:
      subject = prefix + ' ' + self.subject
    else:
      subject = self.subject

    try:
      truncate_subject = int(
          self.cfg.get('truncate_subject', group, params))
    except ValueError:
      truncate_subject = 0

    if truncate_subject and len(subject) > truncate_subject:
      subject = subject[:(truncate_subject - 3)] + "..."
    return subject

  def start(self, group, params):
    """Override this method.
    Begin writing an output representation. GROUP is the name of the
    configuration file group which is causing this output to be produced.
    PARAMS is a dictionary of any named subexpressions of regular expressions
    defined in the configuration file, plus the key 'author' contains the
    author of the action being reported."""
    raise NotImplementedError

  def finish(self):
    """Override this method.
    Flush any cached information and finish writing the output
    representation."""
    raise NotImplementedError

  def write(self, output):
    """Override this method.
    Append the literal text string OUTPUT to the output representation."""
    raise NotImplementedError

  def run(self, cmd):
    """Override this method, if the default implementation is not sufficient.
    Execute CMD, writing the stdout produced to the output representation."""
    # By default we choose to incorporate child stderr into the output
    pipe_ob = subprocess.Popen(cmd, stdout=subprocess.PIPE,
                               stderr=subprocess.STDOUT,
                               close_fds=sys.platform != "win32")

    buf = pipe_ob.stdout.read(self._CHUNKSIZE)
    while buf:
      self.write(buf)
      buf = pipe_ob.stdout.read(self._CHUNKSIZE)

    # wait on the child so we don't end up with a billion zombies
    pipe_ob.wait()


class MailedOutput(OutputBase):
  def __init__(self, cfg, repos, prefix_param):
    OutputBase.__init__(self, cfg, repos, prefix_param)

  def start(self, group, params):
    # whitespace (or another character) separated list of addresses
    # which must be split into a clean list
    to_addr_in = self.cfg.get('to_addr', group, params)
    # if list of addresses starts with '[.]'
    # use the character between the square brackets as split char
    # else use whitespaces
    if len(to_addr_in) >= 3 and to_addr_in[0] == '[' \
                            and to_addr_in[2] == ']':
      self.to_addrs = \
        [_f for _f in to_addr_in[3:].split(to_addr_in[1]) if _f]
    else:
      self.to_addrs = [_f for _f in to_addr_in.split() if _f]
    self.from_addr = self.cfg.get('from_addr', group, params) \
                     or self.repos.author or 'no_author'
    # if the from_addr (also) starts with '[.]' (may happen if one
    # map is used for both to_addr and from_addr) remove '[.]'
    if len(self.from_addr) >= 3 and self.from_addr[0] == '[' \
                                and self.from_addr[2] == ']':
      self.from_addr = self.from_addr[3:]
    self.reply_to = self.cfg.get('reply_to', group, params)
    # if the reply_to (also) starts with '[.]' (may happen if one
    # map is used for both to_addr and reply_to) remove '[.]'
    if len(self.reply_to) >= 3 and self.reply_to[0] == '[' \
                               and self.reply_to[2] == ']':
      self.reply_to = self.reply_to[3:]

  def mail_headers(self, group, params):
    subject = self.make_subject(group, params)
    try:
      subject.encode('ascii')
    except UnicodeError:
      from email.Header import Header
      subject = Header(subject, 'utf-8').encode()
    hdrs = 'From: %s\n'    \
           'To: %s\n'      \
           'Subject: %s\n' \
           'MIME-Version: 1.0\n' \
           'Content-Type: text/plain; charset=UTF-8\n' \
           'Content-Transfer-Encoding: 8bit\n' \
           'X-Svn-Commit-Project: %s\n' \
           'X-Svn-Commit-Author: %s\n' \
           'X-Svn-Commit-Revision: %d\n' \
           'X-Svn-Commit-Repository: %s\n' \
           % (self.from_addr, ', '.join(self.to_addrs), subject,
              group, self.repos.author or 'no_author', self.repos.rev,
              os.path.basename(self.repos.repos_dir))
    if self.reply_to:
      hdrs = '%sReply-To: %s\n' % (hdrs, self.reply_to)
    return hdrs + '\n'


class SMTPOutput(MailedOutput):
  "Deliver a mail message to an MTA using SMTP."

  def start(self, group, params):
    MailedOutput.start(self, group, params)

    self.buffer = StringIO()
    self.write = self.buffer.write

    self.write(self.mail_headers(group, params))

  def finish(self):
    server = smtplib.SMTP(self.cfg.general.smtp_hostname)
    if self.cfg.is_set('general.smtp_username'):
      server.login(self.cfg.general.smtp_username,
                   self.cfg.general.smtp_password)
    server.sendmail(self.from_addr, self.to_addrs, self.buffer.getvalue())
    server.quit()


class StandardOutput(OutputBase):
  "Print the commit message to stdout."

  def __init__(self, cfg, repos, prefix_param):
    OutputBase.__init__(self, cfg, repos, prefix_param)
    self.write = sys.stdout.write

  def start(self, group, params):
    self.write("Group: " + (group or "defaults") + "\n")
    self.write("Subject: " + self.make_subject(group, params) + "\n\n")

  def finish(self):
    pass


class PipeOutput(MailedOutput):
  "Deliver a mail message to an MTA via a pipe."

  def __init__(self, cfg, repos, prefix_param):
    MailedOutput.__init__(self, cfg, repos, prefix_param)

    # figure out the command for delivery
    self.cmd = cfg.general.mail_command.split()

  def start(self, group, params):
    MailedOutput.start(self, group, params)

    ### gotta fix this. this is pretty specific to sendmail and qmail's
    ### mailwrapper program. should be able to use option param substitution
    cmd = self.cmd + [ '-f', self.from_addr ] + self.to_addrs

    # construct the pipe for talking to the mailer
    self.pipe = subprocess.Popen(cmd, stdin=subprocess.PIPE,
                                 close_fds=sys.platform != "win32")
    self.write = self.pipe.stdin.write

    # start writing out the mail message
    self.write(self.mail_headers(group, params))

  def finish(self):
    # signal that we're done sending content
    self.pipe.stdin.close()

    # wait to avoid zombies
    self.pipe.wait()


class Messenger:
  def __init__(self, pool, cfg, repos, prefix_param):
    self.pool = pool
    self.cfg = cfg
    self.repos = repos

    if cfg.is_set('general.mail_command'):
      cls = PipeOutput
    elif cfg.is_set('general.smtp_hostname'):
      cls = SMTPOutput
    else:
      cls = StandardOutput

    self.output = cls(cfg, repos, prefix_param)


class Commit(Messenger):
  def __init__(self, pool, cfg, repos):
    Messenger.__init__(self, pool, cfg, repos, 'commit_subject_prefix')

    # get all the changes and sort by path
    editor = svn.repos.ChangeCollector(repos.fs_ptr, repos.root_this, \
                                       self.pool)
    e_ptr, e_baton = svn.delta.make_editor(editor, self.pool)
    svn.repos.replay(repos.root_this, e_ptr, e_baton, self.pool)

    self.changelist = sorted(editor.get_changes().items())

    log = repos.get_rev_prop(svn.core.SVN_PROP_REVISION_LOG) or ''

    # collect the set of groups and the unique sets of params for the options
    self.groups = { }
    for path, change in self.changelist:
      for (group, params) in self.cfg.which_groups(path, log):
        # turn the params into a hashable object and stash it away
        param_list = sorted(params.items())
        # collect the set of paths belonging to this group
        if (group, tuple(param_list)) in self.groups:
          old_param, paths = self.groups[group, tuple(param_list)]
        else:
          paths = { }
        paths[path] = None
        self.groups[group, tuple(param_list)] = (params, paths)

    # figure out the changed directories
    dirs = { }
    for path, change in self.changelist:
      if change.item_kind == svn.core.svn_node_dir:
        dirs[path] = None
      else:
        idx = path.rfind('/')
        if idx == -1:
          dirs[''] = None
        else:
          dirs[path[:idx]] = None

    dirlist = list(dirs.keys())

    commondir, dirlist = get_commondir(dirlist)

    # compose the basic subject line. later, we can prefix it.
    dirlist.sort()
    dirlist = ' '.join(dirlist)
    if commondir:
      self.output.subject = 'r%d - in %s: %s' % (repos.rev, commondir, dirlist)
    else:
      self.output.subject = 'r%d - %s' % (repos.rev, dirlist)

  def generate(self):
    "Generate email for the various groups and option-params."

    ### the groups need to be further compressed. if the headers and
    ### body are the same across groups, then we can have multiple To:
    ### addresses. SMTPOutput holds the entire message body in memory,
    ### so if the body doesn't change, then it can be sent N times
    ### rather than rebuilding it each time.

    subpool = svn.core.svn_pool_create(self.pool)

    # build a renderer, tied to our output stream
    renderer = TextCommitRenderer(self.output)

    for (group, param_tuple), (params, paths) in self.groups.items():
      self.output.start(group, params)

      # generate the content for this group and set of params
      generate_content(renderer, self.cfg, self.repos, self.changelist,
                       group, params, paths, subpool)

      self.output.finish()
      svn.core.svn_pool_clear(subpool)

    svn.core.svn_pool_destroy(subpool)


class PropChange(Messenger):
  def __init__(self, pool, cfg, repos, author, propname, action):
    Messenger.__init__(self, pool, cfg, repos, 'propchange_subject_prefix')
    self.author = author
    self.propname = propname
    self.action = action

    # collect the set of groups and the unique sets of params for the options
    self.groups = { }
    for (group, params) in self.cfg.which_groups('', None):
      # turn the params into a hashable object and stash it away
      param_list = sorted(params.items())
      self.groups[group, tuple(param_list)] = params

    self.output.subject = 'r%d - %s' % (repos.rev, propname)

  def generate(self):
    actions = { 'A': 'added', 'M': 'modified', 'D': 'deleted' }
    for (group, param_tuple), params in self.groups.items():
      self.output.start(group, params)
      self.output.write('Author: %s\n'
                        'Revision: %s\n'
                        'Property Name: %s\n'
                        'Action: %s\n'
                        '\n'
                        % (self.author, self.repos.rev, self.propname,
                           actions.get(self.action, 'Unknown (\'%s\')' \
                                       % self.action)))
      if self.action == 'A' or self.action not in actions:
        self.output.write('Property value:\n')
        propvalue = self.repos.get_rev_prop(self.propname)
        self.output.write(propvalue)
      elif self.action == 'M':
        self.output.write('Property diff:\n')
        tempfile1 = tempfile.NamedTemporaryFile()
        tempfile1.write(sys.stdin.read())
        tempfile1.flush()
        tempfile2 = tempfile.NamedTemporaryFile()
        tempfile2.write(self.repos.get_rev_prop(self.propname))
        tempfile2.flush()
        self.output.run(self.cfg.get_diff_cmd(group, {
          'label_from' : 'old property value',
          'label_to' : 'new property value',
          'from' : tempfile1.name,
          'to' : tempfile2.name,
          }))
      self.output.finish()


def get_commondir(dirlist):
  """Figure out the common portion/parent (commondir) of all the paths
  in DIRLIST and return a tuple consisting of commondir, dirlist.  If
  a commondir is found, the dirlist returned is rooted in that
  commondir.  If no commondir is found, dirlist is returned unchanged,
  and commondir is the empty string."""
  if len(dirlist) < 2 or '/' in dirlist:
    commondir = ''
    newdirs = dirlist
  else:
    common = dirlist[0].split('/')
    for j in range(1, len(dirlist)):
      d = dirlist[j]
      parts = d.split('/')
      for i in range(len(common)):
        if i == len(parts) or common[i] != parts[i]:
          del common[i:]
          break
    commondir = '/'.join(common)
    if commondir:
      # strip the common portion from each directory
      l = len(commondir) + 1
      newdirs = [ ]
      for d in dirlist:
        if d == commondir:
          newdirs.append('.')
        else:
          newdirs.append(d[l:])
    else:
      # nothing in common, so reset the list of directories
      newdirs = dirlist

  return commondir, newdirs


class Lock(Messenger):
  def __init__(self, pool, cfg, repos, author, do_lock):
    self.author = author
    self.do_lock = do_lock

    Messenger.__init__(self, pool, cfg, repos,
                       (do_lock and 'lock_subject_prefix'
                        or 'unlock_subject_prefix'))

    # read all the locked paths from STDIN and strip off the trailing newlines
    self.dirlist = [x.rstrip() for x in sys.stdin.readlines()]

    # collect the set of groups and the unique sets of params for the options
    self.groups = { }
    for path in self.dirlist:
      for (group, params) in self.cfg.which_groups(path, None):
        # turn the params into a hashable object and stash it away
        param_list = sorted(params.items())
        # collect the set of paths belonging to this group
        if (group, tuple(param_list)) in self.groups:
          old_param, paths = self.groups[group, tuple(param_list)]
        else:
          paths = { }
        paths[path] = None
        self.groups[group, tuple(param_list)] = (params, paths)

    commondir, dirlist = get_commondir(self.dirlist)

    # compose the basic subject line. later, we can prefix it.
    dirlist.sort()
    dirlist = ' '.join(dirlist)
    if commondir:
      self.output.subject = '%s: %s' % (commondir, dirlist)
    else:
      self.output.subject = '%s' % (dirlist)

    # The lock comment is the same for all paths, so we can just pull
    # the comment for the first path in the dirlist and cache it.
    self.lock = svn.fs.svn_fs_get_lock(self.repos.fs_ptr,
                                       self.dirlist[0], self.pool)

  def generate(self):
    for (group, param_tuple), (params, paths) in self.groups.items():
      self.output.start(group, params)

      self.output.write('Author: %s\n'
                        '%s paths:\n' %
                        (self.author, self.do_lock and 'Locked' or 'Unlocked'))

      self.dirlist.sort()
      for dir in self.dirlist:
        self.output.write('   %s\n\n' % dir)

      if self.do_lock:
        self.output.write('Comment:\n%s\n' % (self.lock.comment or ''))

      self.output.finish()


class DiffSelections:
  def __init__(self, cfg, group, params):
    self.add = False
    self.copy = False
    self.delete = False
    self.modify = False

    gen_diffs = cfg.get('generate_diffs', group, params)

    ### Do a little dance for deprecated options.  Note that even if you
    ### don't have an option anywhere in your configuration file, it
    ### still gets returned as non-None.
    if len(gen_diffs):
      list = gen_diffs.split(" ")
      for item in list:
        if item == 'add':
          self.add = True
        if item == 'copy':
          self.copy = True
        if item == 'delete':
          self.delete = True
        if item == 'modify':
          self.modify = True
    else:
      self.add = True
      self.copy = True
      self.delete = True
      self.modify = True
      ### These options are deprecated
      suppress = cfg.get('suppress_deletes', group, params)
      if suppress == 'yes':
        self.delete = False
      suppress = cfg.get('suppress_adds', group, params)
      if suppress == 'yes':
        self.add = False


class DiffURLSelections:
  def __init__(self, cfg, group, params):
    self.cfg = cfg
    self.group = group
    self.params = params

  def _get_url(self, action, repos_rev, change):
    # The parameters for the URLs generation need to be placed in the
    # parameters for the configuration module, otherwise we may get
    # KeyError exceptions.
    params = self.params.copy()
    params['path'] = change.path and urllib_parse_quote(change.path) or None
    params['base_path'] = change.base_path and urllib_parse_quote(change.base_path) \
                          or None
    params['rev'] = repos_rev
    params['base_rev'] = change.base_rev

    return self.cfg.get("diff_%s_url" % action, self.group, params)

  def get_add_url(self, repos_rev, change):
    return self._get_url('add', repos_rev, change)

  def get_copy_url(self, repos_rev, change):
    return self._get_url('copy', repos_rev, change)

  def get_delete_url(self, repos_rev, change):
    return self._get_url('delete', repos_rev, change)

  def get_modify_url(self, repos_rev, change):
    return self._get_url('modify', repos_rev, change)

def generate_content(renderer, cfg, repos, changelist, group, params, paths,
                     pool):

  svndate = repos.get_rev_prop(svn.core.SVN_PROP_REVISION_DATE)
  ### pick a different date format?
  date = time.ctime(svn.core.secs_from_timestr(svndate, pool))

  diffsels = DiffSelections(cfg, group, params)
  diffurls = DiffURLSelections(cfg, group, params)

  show_nonmatching_paths = cfg.get('show_nonmatching_paths', group, params) \
      or 'yes'

  params_with_rev = params.copy()
  params_with_rev['rev'] = repos.rev
  commit_url = cfg.get('commit_url', group, params_with_rev)

  # figure out the lists of changes outside the selected path-space
  other_added_data = other_replaced_data = other_deleted_data = \
      other_modified_data = [ ]
  if len(paths) != len(changelist) and show_nonmatching_paths != 'no':
    other_added_data = generate_list('A', changelist, paths, False)
    other_replaced_data = generate_list('R', changelist, paths, False)
    other_deleted_data = generate_list('D', changelist, paths, False)
    other_modified_data = generate_list('M', changelist, paths, False)

  if len(paths) != len(changelist) and show_nonmatching_paths == 'yes':
    other_diffs = DiffGenerator(changelist, paths, False, cfg, repos, date,
                                group, params, diffsels, diffurls, pool)
  else:
    other_diffs = None

  data = _data(
    author=repos.author,
    date=date,
    rev=repos.rev,
    log=repos.get_rev_prop(svn.core.SVN_PROP_REVISION_LOG) or '',
    commit_url=commit_url,
    added_data=generate_list('A', changelist, paths, True),
    replaced_data=generate_list('R', changelist, paths, True),
    deleted_data=generate_list('D', changelist, paths, True),
    modified_data=generate_list('M', changelist, paths, True),
    show_nonmatching_paths=show_nonmatching_paths,
    other_added_data=other_added_data,
    other_replaced_data=other_replaced_data,
    other_deleted_data=other_deleted_data,
    other_modified_data=other_modified_data,
    diffs=DiffGenerator(changelist, paths, True, cfg, repos, date, group,
                        params, diffsels, diffurls, pool),
    other_diffs=other_diffs,
    )
  renderer.render(data)


def generate_list(changekind, changelist, paths, in_paths):
  if changekind == 'A':
    selection = lambda change: change.action == svn.repos.CHANGE_ACTION_ADD
  elif changekind == 'R':
    selection = lambda change: change.action == svn.repos.CHANGE_ACTION_REPLACE
  elif changekind == 'D':
    selection = lambda change: change.action == svn.repos.CHANGE_ACTION_DELETE
  elif changekind == 'M':
    selection = lambda change: change.action == svn.repos.CHANGE_ACTION_MODIFY

  items = [ ]
  for path, change in changelist:
    if selection(change) and (path in paths) == in_paths:
      item = _data(
        path=path,
        is_dir=change.item_kind == svn.core.svn_node_dir,
        props_changed=change.prop_changes,
        text_changed=change.text_changed,
        copied=(change.action == svn.repos.CHANGE_ACTION_ADD \
                or change.action == svn.repos.CHANGE_ACTION_REPLACE) \
               and change.base_path,
        base_path=remove_leading_slashes(change.base_path),
        base_rev=change.base_rev,
        )
      items.append(item)

  return items


class DiffGenerator:
  "This is a generator-like object returning DiffContent objects."

  def __init__(self, changelist, paths, in_paths, cfg, repos, date, group,
               params, diffsels, diffurls, pool):
    self.changelist = changelist
    self.paths = paths
    self.in_paths = in_paths
    self.cfg = cfg
    self.repos = repos
    self.date = date
    self.group = group
    self.params = params
    self.diffsels = diffsels
    self.diffurls = diffurls
    self.pool = pool

    self.diff = self.diff_url = None

    self.idx = 0

  def __nonzero__(self):
    # we always have some items
    return True

  def __getitem__(self, idx):
    while True:
      if self.idx == len(self.changelist):
        raise IndexError

      path, change = self.changelist[self.idx]
      self.idx = self.idx + 1

      diff = diff_url = None
      kind = None
      label1 = None
      label2 = None
      src_fname = None
      dst_fname = None
      binary = None
      singular = None
      content = None

      # just skip directories. they have no diffs.
      if change.item_kind == svn.core.svn_node_dir:
        continue

      # is this change in (or out of) the set of matched paths?
      if (path in self.paths) != self.in_paths:
        continue

      if change.base_rev != -1:
        svndate = self.repos.get_rev_prop(svn.core.SVN_PROP_REVISION_DATE,
                                          change.base_rev)
        ### pick a different date format?
        base_date = time.ctime(svn.core.secs_from_timestr(svndate, self.pool))
      else:
        base_date = ''

      # figure out if/how to generate a diff

      base_path = remove_leading_slashes(change.base_path)
      if change.action == svn.repos.CHANGE_ACTION_DELETE:
        # it was delete.
        kind = 'D'

        # get the diff url, if any is specified
        diff_url = self.diffurls.get_delete_url(self.repos.rev, change)

        # show the diff?
        if self.diffsels.delete:
          diff = svn.fs.FileDiff(self.repos.get_root(change.base_rev),
                                 base_path, None, None, self.pool)

          label1 = '%s\t%s\t(r%s)' % (base_path, self.date, change.base_rev)
          label2 = '/dev/null\t00:00:00 1970\t(deleted)'
          singular = True

      elif change.action == svn.repos.CHANGE_ACTION_ADD \
           or change.action == svn.repos.CHANGE_ACTION_REPLACE:
        if base_path and (change.base_rev != -1):

          # any diff of interest?
          if change.text_changed:
            # this file was copied and modified.
            kind = 'W'

            # get the diff url, if any is specified
            diff_url = self.diffurls.get_copy_url(self.repos.rev, change)

            # show the diff?
            if self.diffsels.modify:
              diff = svn.fs.FileDiff(self.repos.get_root(change.base_rev),
                                     base_path,
                                     self.repos.root_this, change.path,
                                     self.pool)
              label1 = '%s\t%s\t(r%s, copy source)' \
                       % (base_path, base_date, change.base_rev)
              label2 = '%s\t%s\t(r%s)' \
                       % (change.path, self.date, self.repos.rev)
              singular = False
          else:
            # this file was copied.
            kind = 'C'
            if self.diffsels.copy:
              diff = svn.fs.FileDiff(None, None, self.repos.root_this,
                                     change.path, self.pool)
              label1 = '/dev/null\t00:00:00 1970\t' \
                       '(empty, because file is newly added)'
              label2 = '%s\t%s\t(r%s, copy of r%s, %s)' \
                       % (change.path, self.date, self.repos.rev, \
                          change.base_rev, base_path)
              singular = False
        else:
          # the file was added.
          kind = 'A'

          # get the diff url, if any is specified
          diff_url = self.diffurls.get_add_url(self.repos.rev, change)

          # show the diff?
          if self.diffsels.add:
            diff = svn.fs.FileDiff(None, None, self.repos.root_this,
                                   change.path, self.pool)
            label1 = '/dev/null\t00:00:00 1970\t' \
                     '(empty, because file is newly added)'
            label2 = '%s\t%s\t(r%s)' \
                     % (change.path, self.date, self.repos.rev)
            singular = True

      elif not change.text_changed:
        # the text didn't change, so nothing to show.
        continue
      else:
        # a simple modification.
        kind = 'M'

        # get the diff url, if any is specified
        diff_url = self.diffurls.get_modify_url(self.repos.rev, change)

        # show the diff?
        if self.diffsels.modify:
          diff = svn.fs.FileDiff(self.repos.get_root(change.base_rev),
                                 base_path,
                                 self.repos.root_this, change.path,
                                 self.pool)
          label1 = '%s\t%s\t(r%s)' \
                   % (base_path, base_date, change.base_rev)
          label2 = '%s\t%s\t(r%s)' \
                   % (change.path, self.date, self.repos.rev)
          singular = False

      if diff:
        binary = diff.either_binary()
        if binary:
          content = src_fname = dst_fname = None
        else:
          src_fname, dst_fname = diff.get_files()
          try:
            content = DiffContent(self.cfg.get_diff_cmd(self.group, {
              'label_from' : label1,
              'label_to' : label2,
              'from' : src_fname,
              'to' : dst_fname,
              }))
          except OSError:
            # diff command does not exist, try difflib.unified_diff()
            content = DifflibDiffContent(label1, label2, src_fname, dst_fname)

      # return a data item for this diff
      return _data(
        path=change.path,
        base_path=base_path,
        base_rev=change.base_rev,
        diff=diff,
        diff_url=diff_url,
        kind=kind,
        label_from=label1,
        label_to=label2,
        from_fname=src_fname,
        to_fname=dst_fname,
        binary=binary,
        singular=singular,
        content=content,
        )

def _classify_diff_line(line, seen_change):
  # classify the type of line.
  first = line[:1]
  ltype = ''
  if first == '@':
    seen_change = True
    ltype = 'H'
  elif first == '-':
    if seen_change:
      ltype = 'D'
    else:
      ltype = 'F'
  elif first == '+':
    if seen_change:
      ltype = 'A'
    else:
      ltype = 'T'
  elif first == ' ':
    ltype = 'C'
  else:
    ltype = 'U'

  if line[-2] == '\r':
    line=line[0:-2] + '\n' # remove carriage return

  return line, ltype, seen_change


class DiffContent:
  "This is a generator-like object returning annotated lines of a diff."

  def __init__(self, cmd):
    self.seen_change = False

    # By default we choose to incorporate child stderr into the output
    self.pipe = subprocess.Popen(cmd, stdout=subprocess.PIPE,
                                 stderr=subprocess.STDOUT,
                                 close_fds=sys.platform != "win32")

  def __nonzero__(self):
    # we always have some items
    return True

  def __getitem__(self, idx):
    if self.pipe is None:
      raise IndexError

    line = self.pipe.stdout.readline()
    if not line:
      # wait on the child so we don't end up with a billion zombies
      self.pipe.wait()
      self.pipe = None
      raise IndexError

    line, ltype, self.seen_change = _classify_diff_line(line, self.seen_change)
    return _data(
      raw=line,
      text=line[1:-1],  # remove indicator and newline
      type=ltype,
      )

class DifflibDiffContent():
  "This is a generator-like object returning annotated lines of a diff."

  def __init__(self, label_from, label_to, from_file, to_file):
    import difflib
    self.seen_change = False
    fromlines = open(from_file, 'U').readlines()
    tolines = open(to_file, 'U').readlines()
    self.diff = difflib.unified_diff(fromlines, tolines,
                                     label_from, label_to)

  def __nonzero__(self):
    # we always have some items
    return True

  def __getitem__(self, idx):

    try:
      line = self.diff.next()
    except StopIteration:
      raise IndexError

    line, ltype, self.seen_change = _classify_diff_line(line, self.seen_change)
    return _data(
      raw=line,
      text=line[1:-1],  # remove indicator and newline
      type=ltype,
      )

class TextCommitRenderer:
  "This class will render the commit mail in plain text."

  def __init__(self, output):
    self.output = output

  def render(self, data):
    "Render the commit defined by 'data'."

    w = self.output.write

    w('Author: %s\nDate: %s\nNew Revision: %s\n' % (data.author,
                                                      data.date,
                                                      data.rev))

    if data.commit_url:
      w('URL: %s\n\n' % data.commit_url)
    else:
      w('\n')

    w('Log:\n%s\n\n' % data.log.strip())

    # print summary sections
    self._render_list('Added', data.added_data)
    self._render_list('Replaced', data.replaced_data)
    self._render_list('Deleted', data.deleted_data)
    self._render_list('Modified', data.modified_data)

    if data.other_added_data or data.other_replaced_data \
           or data.other_deleted_data or data.other_modified_data:
      if data.show_nonmatching_paths:
        w('\nChanges in other areas also in this revision:\n')
        self._render_list('Added', data.other_added_data)
        self._render_list('Replaced', data.other_replaced_data)
        self._render_list('Deleted', data.other_deleted_data)
        self._render_list('Modified', data.other_modified_data)
      else:
        w('and changes in other areas\n')

    self._render_diffs(data.diffs, '')
    if data.other_diffs:
      self._render_diffs(data.other_diffs,
                         '\nDiffs of changes in other areas also'
                         ' in this revision:\n')

  def _render_list(self, header, data_list):
    if not data_list:
      return

    w = self.output.write
    w(header + ':\n')
    for d in data_list:
      if d.is_dir:
        is_dir = '/'
      else:
        is_dir = ''
      if d.props_changed:
        if d.text_changed:
          props = '   (contents, props changed)'
        else:
          props = '   (props changed)'
      else:
        props = ''
      w('   %s%s%s\n' % (d.path, is_dir, props))
      if d.copied:
        if is_dir:
          text = ''
        elif d.text_changed:
          text = ', changed'
        else:
          text = ' unchanged'
        w('      - copied%s from r%d, %s%s\n'
          % (text, d.base_rev, d.base_path, is_dir))

  def _render_diffs(self, diffs, section_header):
    """Render diffs. Write the SECTION_HEADER if there are actually
    any diffs to render."""
    if not diffs:
      return
    w = self.output.write
    section_header_printed = False

    for diff in diffs:
      if not diff.diff and not diff.diff_url:
        continue
      if not section_header_printed:
        w(section_header)
        section_header_printed = True
      if diff.kind == 'D':
        w('\nDeleted: %s\n' % diff.base_path)
      elif diff.kind == 'A':
        w('\nAdded: %s\n' % diff.path)
      elif diff.kind == 'C':
        w('\nCopied: %s (from r%d, %s)\n'
          % (diff.path, diff.base_rev, diff.base_path))
      elif diff.kind == 'W':
        w('\nCopied and modified: %s (from r%d, %s)\n'
          % (diff.path, diff.base_rev, diff.base_path))
      else:
        # kind == 'M'
        w('\nModified: %s\n' % diff.path)

      if diff.diff_url:
        w('URL: %s\n' % diff.diff_url)

      if not diff.diff:
        continue

      w(SEPARATOR + '\n')

      if diff.binary:
        if diff.singular:
          w('Binary file. No diff available.\n')
        else:
          w('Binary file (source and/or target). No diff available.\n')
        continue

      for line in diff.content:
        w(line.raw)


class Repository:
  "Hold roots and other information about the repository."

  def __init__(self, repos_dir, rev, pool):
    self.repos_dir = repos_dir
    self.rev = rev
    self.pool = pool

    self.repos_ptr = svn.repos.open(repos_dir, pool)
    self.fs_ptr = svn.repos.fs(self.repos_ptr)

    self.roots = { }

    self.root_this = self.get_root(rev)

    self.author = self.get_rev_prop(svn.core.SVN_PROP_REVISION_AUTHOR)

  def get_rev_prop(self, propname, rev = None):
    if not rev:
      rev = self.rev
    return svn.fs.revision_prop(self.fs_ptr, rev, propname, self.pool)

  def get_root(self, rev):
    try:
      return self.roots[rev]
    except KeyError:
      pass
    root = self.roots[rev] = svn.fs.revision_root(self.fs_ptr, rev, self.pool)
    return root


class Config:

  # The predefined configuration sections. These are omitted from the
  # set of groups.
  _predefined = ('general', 'defaults', 'maps')

  def __init__(self, fname, repos, global_params):
    cp = configparser.ConfigParser()
    cp.read(fname)

    # record the (non-default) groups that we find
    self._groups = [ ]

    for section in cp.sections():
      if not hasattr(self, section):
        section_ob = _sub_section()
        setattr(self, section, section_ob)
        if section not in self._predefined:
          self._groups.append(section)
      else:
        section_ob = getattr(self, section)
      for option in cp.options(section):
        # get the raw value -- we use the same format for *our* interpolation
        value = cp.get(section, option, raw=1)
        setattr(section_ob, option, value)

    # be compatible with old format config files
    if hasattr(self.general, 'diff') and not hasattr(self.defaults, 'diff'):
      self.defaults.diff = self.general.diff
    if not hasattr(self, 'maps'):
      self.maps = _sub_section()

    # these params are always available, although they may be overridden
    self._global_params = global_params.copy()

    # prepare maps. this may remove sections from consideration as a group.
    self._prep_maps()

    # process all the group sections.
    self._prep_groups(repos)

  def is_set(self, option):
    """Return None if the option is not set; otherwise, its value is returned.

    The option is specified as a dotted symbol, such as 'general.mail_command'
    """
    ob = self
    for part in option.split('.'):
      if not hasattr(ob, part):
        return None
      ob = getattr(ob, part)
    return ob

  def get(self, option, group, params):
    "Get a config value with appropriate substitutions and value mapping."

    # find the right value
    value = None
    if group:
      sub = getattr(self, group)
      value = getattr(sub, option, None)
    if value is None:
      value = getattr(self.defaults, option, '')

    # parameterize it
    if params is not None:
      value = value % params

    # apply any mapper
    mapper = getattr(self.maps, option, None)
    if mapper is not None:
      value = mapper(value)

      # Apply any parameters that may now be available for
      # substitution that were not before the mapping.
      if value is not None and params is not None:
        value = value % params

    return value

  def get_diff_cmd(self, group, args):
    "Get a diff command as a list of argv elements."
    ### do some better splitting to enable quoting of spaces
    diff_cmd = self.get('diff', group, None).split()

    cmd = [ ]
    for part in diff_cmd:
      cmd.append(part % args)
    return cmd

  def _prep_maps(self):
    "Rewrite the [maps] options into callables that look up values."

    mapsections = []

    for optname, mapvalue in vars(self.maps).items():
      if mapvalue[:1] == '[':
        # a section is acting as a mapping
        sectname = mapvalue[1:-1]
        if not hasattr(self, sectname):
          raise UnknownMappingSection(sectname)
        # construct a lambda to look up the given value as an option name,
        # and return the option's value. if the option is not present,
        # then just return the value unchanged.
        setattr(self.maps, optname,
                lambda value,
                       sect=getattr(self, sectname): getattr(sect,
                                                             value.lower(),
                                                             value))
        # mark for removal when all optnames are done
        if sectname not in mapsections:
          mapsections.append(sectname)

      # elif test for other mapper types. possible examples:
      #   dbm:filename.db
      #   file:two-column-file.txt
      #   ldap:some-query-spec
      # just craft a mapper function and insert it appropriately

      else:
        raise UnknownMappingSpec(mapvalue)

    # remove each mapping section from consideration as a group
    for sectname in mapsections:
      self._groups.remove(sectname)


  def _prep_groups(self, repos):
    self._group_re = [ ]

    repos_dir = os.path.abspath(repos.repos_dir)

    # compute the default repository-based parameters. start with some
    # basic parameters, then bring in the regex-based params.
    self._default_params = self._global_params

    try:
      match = re.match(self.defaults.for_repos, repos_dir)
      if match:
        self._default_params = self._default_params.copy()
        self._default_params.update(match.groupdict())
    except AttributeError:
      # there is no self.defaults.for_repos
      pass

    # select the groups that apply to this repository
    for group in self._groups:
      sub = getattr(self, group)
      params = self._default_params
      if hasattr(sub, 'for_repos'):
        match = re.match(sub.for_repos, repos_dir)
        if not match:
          continue
        params = params.copy()
        params.update(match.groupdict())

      # if a matching rule hasn't been given, then use the empty string
      # as it will match all paths
      for_paths = getattr(sub, 'for_paths', '')
      exclude_paths = getattr(sub, 'exclude_paths', None)
      if exclude_paths:
        exclude_paths_re = re.compile(exclude_paths)
      else:
        exclude_paths_re = None

      # check search_logmsg re
      search_logmsg = getattr(sub, 'search_logmsg', None)
      if search_logmsg is not None:
        search_logmsg_re = re.compile(search_logmsg)
      else:
        search_logmsg_re = None

      self._group_re.append((group,
                             re.compile(for_paths),
                             exclude_paths_re,
                             params,
                             search_logmsg_re))

    # after all the groups are done, add in the default group
    try:
      self._group_re.append((None,
                             re.compile(self.defaults.for_paths),
                             None,
                             self._default_params,
                             None))
    except AttributeError:
      # there is no self.defaults.for_paths
      pass

  def which_groups(self, path, logmsg):
    "Return the path's associated groups."
    groups = []
    for group, pattern, exclude_pattern, repos_params, search_logmsg_re in self._group_re:
      match = pattern.match(path)
      if match:
        if exclude_pattern and exclude_pattern.match(path):
          continue
        params = repos_params.copy()
        params.update(match.groupdict())

        if search_logmsg_re is None:
          groups.append((group, params))
        else:
          if logmsg is None:
            logmsg = ''

          for match in search_logmsg_re.finditer(logmsg):
            # Add captured variables to (a copy of) params
            msg_params = params.copy()
            msg_params.update(match.groupdict())
            groups.append((group, msg_params))

    if not groups:
      groups.append((None, self._default_params))

    return groups


class _sub_section:
  pass

class _data:
  "Helper class to define an attribute-based hunk o' data."
  def __init__(self, **kw):
    vars(self).update(kw)

class MissingConfig(Exception):
  pass
class UnknownMappingSection(Exception):
  pass
class UnknownMappingSpec(Exception):
  pass
class UnknownSubcommand(Exception):
  pass


if __name__ == '__main__':
  def usage():
    scriptname = os.path.basename(sys.argv[0])
    sys.stderr.write(
"""USAGE: %s commit      REPOS REVISION [CONFIG-FILE]
       %s propchange  REPOS REVISION AUTHOR REVPROPNAME [CONFIG-FILE]
       %s propchange2 REPOS REVISION AUTHOR REVPROPNAME ACTION [CONFIG-FILE]
       %s lock        REPOS AUTHOR [CONFIG-FILE]
       %s unlock      REPOS AUTHOR [CONFIG-FILE]

If no CONFIG-FILE is provided, the script will first search for a mailer.conf
file in REPOS/conf/.  Failing that, it will search the directory in which
the script itself resides.

ACTION was added as a fifth argument to the post-revprop-change hook
in Subversion 1.2.0.  Its value is one of 'A', 'M' or 'D' to indicate
if the property was added, modified or deleted, respectively.

""" % (scriptname, scriptname, scriptname, scriptname, scriptname))
    sys.exit(1)

  # Command list:  subcommand -> number of arguments expected (not including
  #                              the repository directory and config-file)
  cmd_list = {'commit'     : 1,
              'propchange' : 3,
              'propchange2': 4,
              'lock'       : 1,
              'unlock'     : 1,
              }

  config_fname = None
  argc = len(sys.argv)
  if argc < 3:
    usage()

  cmd = sys.argv[1]
  repos_dir = svn.core.svn_path_canonicalize(sys.argv[2])
  try:
    expected_args = cmd_list[cmd]
  except KeyError:
    usage()

  if argc < (expected_args + 3):
    usage()
  elif argc > expected_args + 4:
    usage()
  elif argc == (expected_args + 4):
    config_fname = sys.argv[expected_args + 3]

  # Settle on a config file location, and open it.
  if config_fname is None:
    # Default to REPOS-DIR/conf/mailer.conf.
    config_fname = os.path.join(repos_dir, 'conf', 'mailer.conf')
    if not os.path.exists(config_fname):
      # Okay.  Look for 'mailer.conf' as a sibling of this script.
      config_fname = os.path.join(os.path.dirname(sys.argv[0]), 'mailer.conf')
  if not os.path.exists(config_fname):
    raise MissingConfig(config_fname)

  svn.core.run_app(main, cmd, config_fname, repos_dir,
                   sys.argv[3:3+expected_args])

# ------------------------------------------------------------------------
# TODO
#
# * add configuration options
#   - each group defines delivery info:
#     o whether to set Reply-To and/or Mail-Followup-To
#       (btw: it is legal do set Reply-To since this is the originator of the
#        mail; i.e. different from MLMs that munge it)
#   - each group defines content construction:
#     o max size of diff before trimming
#     o max size of entire commit message before truncation
#   - per-repository configuration
#     o extra config living in repos
#     o optional, non-mail log file
#     o look up authors (username -> email; for the From: header) in a
#       file(s) or DBM
# * get rid of global functions that should properly be class methods
