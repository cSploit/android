#!/usr/bin/env python
# -*- coding: utf-8 -*-
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
# ====================================================================

"""Usage: svn2feed.py [OPTION...] REPOS-PATH

Generate an RSS 2.0 or Atom 1.0 feed file containing commit
information for the Subversion repository located at REPOS-PATH.  Once
the maximum number of items is reached, older elements are removed.
The item title is the revision number, and the item description
contains the author, date, log messages and changed paths.

Options:

 -h, --help             Show this help message.

 -F, --format=FORMAT    Required option.  FORMAT must be one of:
                            'rss'  (RSS 2.0)
                            'atom' (Atom 1.0)
                        to select the appropriate feed format.

 -f, --feed-file=PATH   Store the feed in the file located at PATH, which will
                        be created if it does not exist, or overwritten if it
                        does.  If not provided, the script will store the feed
                        in the current working directory, in a file named
                        REPOS_NAME.rss or REPOS_NAME.atom (where REPOS_NAME is
                        the basename of the REPOS_PATH command-line argument,
                        and the file extension depends on the selected
                        format).

 -r, --revision=X[:Y]   Subversion revision (or revision range) to generate
                        info for.  If not provided, info for the single
                        youngest revision in the repository will be generated.

 -m, --max-items=N      Keep only N items in the feed file.  By default,
                        20 items are kept.

 -u, --item-url=URL     Use URL as the basis for generating feed item links.
                        This value is appended with '?rev=REV_NUMBER' to form
                        the actual item links.

 -U, --feed-url=URL     Use URL as the global link associated with the feed.

 -P, --svn-path=DIR     Look in DIR for the svnlook binary.  If not provided,
                        svnlook must be on the PATH.
"""

# TODO:
# --item-url should support arbitrary formatting of the revision number,
#   to be useful with web viewers other than ViewVC.
# Rather more than intended is being cached in the pickle file. Instead of
#   only old items being drawn from the pickle, all the global feed metadata
#   is actually set only on initial feed creation, and thereafter simply
#   re-used from the pickle each time.

# $HeadURL: http://svn.apache.org/repos/asf/subversion/branches/1.7.x/tools/hook-scripts/svn2feed.py $
# $LastChangedDate: 2009-11-16 19:07:17 +0000 (Mon, 16 Nov 2009) $
# $LastChangedBy: hwright $
# $LastChangedRevision: 880911 $

import sys

# Python 2.4 is required for subprocess
if sys.version_info < (2, 4):
    sys.stderr.write("Error: Python 2.4 or higher required.\n")
    sys.stderr.flush()
    sys.exit(1)

import getopt
import os
import subprocess
try:
  # Python <3.0
  import cPickle as pickle
except ImportError:
  # Python >=3.0
  import pickle
import datetime
import time

def usage_and_exit(errmsg=None):
    """Print a usage message, plus an ERRMSG (if provided), then exit.
    If ERRMSG is provided, the usage message is printed to stderr and
    the script exits with a non-zero error code.  Otherwise, the usage
    message goes to stdout, and the script exits with a zero
    errorcode."""
    if errmsg is None:
        stream = sys.stdout
    else:
        stream = sys.stderr
    stream.write("%s\n" % __doc__)
    stream.flush()
    if errmsg:
        stream.write("\nError: %s\n" % errmsg)
	stream.flush()
        sys.exit(2)
    sys.exit(0)

def check_url(url, opt):
    """Verify that URL looks like a valid URL or option OPT."""
    if not (url.startswith('https://') \
            or url.startswith('http://') \
            or url.startswith('file://')):
      usage_and_exit("svn2feed.py: Invalid url '%s' is specified for " \
                     "'%s' option" % (url, opt))


class Svn2Feed:
    def __init__(self, svn_path, repos_path, item_url, feed_file,
                 max_items, feed_url):
        self.repos_path = repos_path
        self.item_url = item_url
        self.feed_file = feed_file
        self.max_items = max_items
        self.feed_url = feed_url
        self.svnlook_cmd = 'svnlook'
        if svn_path is not None:
            self.svnlook_cmd = os.path.join(svn_path, 'svnlook')
        self.feed_title = ("%s's Subversion Commits Feed"
                % (os.path.basename(os.path.abspath(self.repos_path))))
        self.feed_desc = "The latest Subversion commits"

    def _get_item_dict(self, revision):
        revision = str(revision)

        cmd = [self.svnlook_cmd, 'info', '-r', revision, self.repos_path]
        proc = subprocess.Popen(cmd, stdout=subprocess.PIPE)
        proc.wait()
        info_lines = proc.stdout.readlines()

        cmd = [self.svnlook_cmd, 'changed', '-r', revision, self.repos_path]
        proc = subprocess.Popen(cmd, stdout=subprocess.PIPE)
        proc.wait()
        changed_data = proc.stdout.readlines()

        desc = ("\nRevision: %s\nLog: %sModified: \n%s"
                % (revision, info_lines[3], changed_data))

        item_dict = {
            'author': info_lines[0].strip('\n'),
            'title': "Revision %s" % revision,
            'link': self.item_url and "%s?rev=%s" % (self.item_url, revision),
            'date': self._format_updated_ts(info_lines[1]),
            'description': "<pre>" + desc + "</pre>",
            }

        return item_dict

    def _format_updated_ts(self, revision_ts):

        # Get "2006-08-10 20:17:08" from
        # "2006-07-28 20:17:18 +0530 (Fri, 28 Jul 2006)
        date = revision_ts[0:19]
        epoch = time.mktime(time.strptime(date, "%Y-%m-%d %H:%M:%S"))
        return time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime(epoch))


class Svn2RSS(Svn2Feed):
    def __init__(self, svn_path, repos_path, item_url, feed_file,
                 max_items, feed_url):
        Svn2Feed.__init__(self, svn_path, repos_path, item_url, feed_file,
                          max_items, feed_url)
        try:
            import PyRSS2Gen
        except ImportError:
            sys.stderr.write("""
Error: Required PyRSS2Gen module not found.  You can download the PyRSS2Gen
module from:

    http://www.dalkescientific.com/Python/PyRSS2Gen.html

""")
            sys.exit(1)
        self.PyRSS2Gen = PyRSS2Gen

        (file, ext) = os.path.splitext(self.feed_file)
        self.pickle_file = file + ".pickle"
        if os.path.exists(self.pickle_file):
            self.rss = pickle.load(open(self.pickle_file, "r"))
        else:
            self.rss = self.PyRSS2Gen.RSS2(
                    title = self.feed_title,
                    link = self.feed_url,
                    description = self.feed_desc,
                    lastBuildDate = datetime.datetime.now(),
                    items = [])

    @staticmethod
    def get_default_file_extension():
        return ".rss"

    def add_revision_item(self, revision):
        rss_item = self._make_rss_item(revision)
        self.rss.items.insert(0, rss_item)
        if len(self.rss.items) > self.max_items:
            del self.rss.items[self.max_items:]

    def write_output(self):
        s = pickle.dumps(self.rss)
        f = open(self.pickle_file, "w")
        f.write(s)
        f.close()

        f = open(self.feed_file, "w")
        self.rss.write_xml(f)
        f.close()

    def _make_rss_item(self, revision):
        info = self._get_item_dict(revision)

        rss_item = self.PyRSS2Gen.RSSItem(
                author = info['author'],
                title = info['title'],
                link = info['link'],
                description = info['description'],
                guid = self.PyRSS2Gen.Guid(info['link']),
                pubDate = info['date'])
        return rss_item


class Svn2Atom(Svn2Feed):
    def __init__(self, svn_path, repos_path, item_url, feed_file,
                 max_items, feed_url):
        Svn2Feed.__init__(self, svn_path, repos_path, item_url, feed_file,
                          max_items, feed_url)
        from xml.dom import getDOMImplementation
        self.dom_impl = getDOMImplementation()

        self.pickle_file = self.feed_file + ".pickle"
        if os.path.exists(self.pickle_file):
            self.document = pickle.load(open(self.pickle_file, "r"))
            self.feed = self.document.getElementsByTagName('feed')[0]
        else:
            self._init_atom_document()

    @staticmethod
    def get_default_file_extension():
        return ".atom"

    def add_revision_item(self, revision):
        item = self._make_atom_item(revision)

        total = 0
        for childNode in self.feed.childNodes:
            if childNode.nodeName == 'entry':
                if total == 0:
                    self.feed.insertBefore(item, childNode)
                    total += 1
                total += 1
                if total > self.max_items:
                    self.feed.removeChild(childNode)
        if total == 0:
            self.feed.appendChild(item)

    def write_output(self):
        s = pickle.dumps(self.document)
        f = open(self.pickle_file, "w")
        f.write(s)
        f.close()

        f = open(self.feed_file, "w")
        f.write(self.document.toxml())
        f.close()

    def _make_atom_item(self, revision):
        info = self._get_item_dict(revision)

        doc = self.document
        entry = doc.createElement("entry")

        id = doc.createElement("id")
        entry.appendChild(id)
        id.appendChild(doc.createTextNode(info['link']))

        title = doc.createElement("title")
        entry.appendChild(title)
        title.appendChild(doc.createTextNode(info['title']))

        updated = doc.createElement("updated")
        entry.appendChild(updated)
        updated.appendChild(doc.createTextNode(info['date']))

        link = doc.createElement("link")
        entry.appendChild(link)
        link.setAttribute("href", info['link'])

        summary = doc.createElement("summary")
        entry.appendChild(summary)
        summary.appendChild(doc.createTextNode(info['description']))

        author = doc.createElement("author")
        entry.appendChild(author)
        aname = doc.createElement("name")
        author.appendChild(aname)
        aname.appendChild(doc.createTextNode(info['author']))

        return entry

    def _init_atom_document(self):
        doc = self.document = self.dom_impl.createDocument(None, None, None)
        feed = self.feed = doc.createElement("feed")
        doc.appendChild(feed)

        feed.setAttribute("xmlns", "http://www.w3.org/2005/Atom")

        title = doc.createElement("title")
        feed.appendChild(title)
        title.appendChild(doc.createTextNode(self.feed_title))

        id = doc.createElement("id")
        feed.appendChild(id)
        id.appendChild(doc.createTextNode(self.feed_url))

        updated = doc.createElement("updated")
        feed.appendChild(updated)
        now = datetime.datetime.now()
        updated.appendChild(doc.createTextNode(self._format_date(now)))

        link = doc.createElement("link")
        feed.appendChild(link)
        link.setAttribute("href", self.feed_url)

        author = doc.createElement("author")
        feed.appendChild(author)
        aname = doc.createElement("name")
        author.appendChild(aname)
        aname.appendChild(doc.createTextNode("subversion"))

    def _format_date(self, dt):
        """ input date must be in GMT """
        return ("%04d-%02d-%02dT%02d:%02d:%02d.%02dZ"
                % (dt.year, dt.month, dt.day, dt.hour, dt.minute,
                   dt.second, dt.microsecond))


def main():
    # Parse the command-line options and arguments.
    try:
        opts, args = getopt.gnu_getopt(sys.argv[1:], "hP:r:u:f:m:U:F:",
                                       ["help",
                                        "svn-path=",
                                        "revision=",
                                        "item-url=",
                                        "feed-file=",
                                        "max-items=",
                                        "feed-url=",
                                        "format=",
                                        ])
    except getopt.GetoptError, msg:
        usage_and_exit(msg)

    # Make sure required arguments are present.
    if len(args) != 1:
        usage_and_exit("You must specify a repository path.")
    repos_path = os.path.abspath(args[0])

    # Now deal with the options.
    max_items = 20
    commit_rev = svn_path = None
    item_url = feed_url = None
    feed_file = None
    feedcls = None
    feed_classes = { 'rss': Svn2RSS, 'atom': Svn2Atom }

    for opt, arg in opts:
        if opt in ("-h", "--help"):
            usage_and_exit()
        elif opt in ("-P", "--svn-path"):
            svn_path = arg
        elif opt in ("-r", "--revision"):
            commit_rev = arg
        elif opt in ("-u", "--item-url"):
            item_url = arg
            check_url(item_url, opt)
        elif opt in ("-f", "--feed-file"):
            feed_file = arg
        elif opt in ("-m", "--max-items"):
            try:
               max_items = int(arg)
            except ValueError, msg:
               usage_and_exit("Invalid value '%s' for --max-items." % (arg))
            if max_items < 1:
               usage_and_exit("Value for --max-items must be a positive "
                              "integer.")
        elif opt in ("-U", "--feed-url"):
            feed_url = arg
            check_url(feed_url, opt)
        elif opt in ("-F", "--format"):
            try:
                feedcls = feed_classes[arg]
            except KeyError:
                usage_and_exit("Invalid value '%s' for --format." % arg)

    if feedcls is None:
        usage_and_exit("Option -F [--format] is required.")

    if item_url is None:
        usage_and_exit("Option -u [--item-url] is required.")

    if feed_url is None:
        usage_and_exit("Option -U [--feed-url] is required.")

    if commit_rev is None:
        svnlook_cmd = 'svnlook'
        if svn_path is not None:
            svnlook_cmd = os.path.join(svn_path, 'svnlook')
        cmd = [svnlook_cmd, 'youngest', repos_path]
        proc = subprocess.Popen(cmd, stdout=subprocess.PIPE)
        proc.wait()
        cmd_out = proc.stdout.readlines()
        try:
            revisions = [int(cmd_out[0])]
        except IndexError, msg:
            usage_and_exit("svn2feed.py: Invalid value '%s' for " \
                           "REPOS-PATH" % (repos_path))
    else:
        try:
            rev_range = commit_rev.split(':')
            len_rev_range = len(rev_range)
            if len_rev_range == 1:
                revisions = [int(commit_rev)]
            elif len_rev_range == 2:
                start, end = rev_range
                start = int(start)
                end = int(end)
                if (start > end):
                    tmp = start
                    start = end
                    end = tmp
                revisions = list(range(start, end + 1)[-max_items:])
            else:
                raise ValueError()
        except ValueError, msg:
            usage_and_exit("svn2feed.py: Invalid value '%s' for --revision." \
                           % (commit_rev))

    if feed_file is None:
        feed_file = (os.path.basename(repos_path) +
                     feedcls.get_default_file_extension())

    feed = feedcls(svn_path, repos_path, item_url, feed_file, max_items,
                   feed_url)
    for revision in revisions:
        feed.add_revision_item(revision)
    feed.write_output()


if __name__ == "__main__":
    main()
