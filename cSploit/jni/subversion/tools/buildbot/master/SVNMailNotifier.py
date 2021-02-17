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
import os
import urllib
import re

from email.Message import Message
from email.Utils import formatdate
from email.MIMEText import MIMEText

from twisted.internet import defer
from twisted.application import service

from buildbot.status.builder import FAILURE, SUCCESS, WARNINGS
from buildbot.status.mail import MailNotifier

class SVNMailNotifier(MailNotifier):
    """Implement custom status mails for the Subversion project"""

    def __init__(self, fromaddr, mode="all", categories=None, builders=None,
                 addLogs=False, relayhost="localhost",
                 subject="buildbot %(result)s in %(builder)s",
                 lookup=None, extraRecipients=[],
                 sendToInterestedUsers=True,
                 body="",
                 replytoaddr=""):
        """
        @type  body: string
        @param body: a string to be used as the body of the message.

        @type  replytoaddr: string
        @param replytoaddr: the email address to be used in the 'Reply-To' header.
        """

        self.body = body
        self.replytoaddr = replytoaddr

        # pass the rest of the parameters to our parent.
        MailNotifier.__init__(self, fromaddr, mode, categories, builders,
                              addLogs, relayhost, subject, lookup, extraRecipients,
                              sendToInterestedUsers)

    def buildMessage(self, name, build, results):
        if self.mode == "all":
            intro = "The Buildbot has finished a build of %s.\n" % name
        elif self.mode == "failing":
            intro = "The Buildbot has detected a failed build of %s.\n" % name
        else:
            intro = "The Buildbot has detected a new failure of %s.\n" % name

        # buildurl
        buildurl = self.status.getURLForThing(build)
# lgo: url's are already quoted now.
#       if buildurl:
#            buildurl = urllib.quote(buildurl, '/:')

        # buildboturl
        buildboturl = self.status.getBuildbotURL()
#        if url:
#            buildboturl = urllib.quote(url, '/:')

        # reason of build
        buildreason = build.getReason()

        # source stamp
        patch = None
        ss = build.getSourceStamp()
        if ss is None:
            source = "unavailable"
        else:
            if build.getChanges():
                revision = max([int(c.revision) for c in build.getChanges()])

            source = ""
            if ss.branch is None:
               ss.branch = "trunk"
            source += "[branch %s] " % ss.branch
            if revision:
                source += str(revision)
            else:
                source += "HEAD"
            if ss.patch is not None:
                source += " (plus patch)"

        # actual buildslave
        buildslave = build.getSlavename()

        # TODO: maybe display changes here? or in an attachment?

        # status
        t = build.getText()
        if t:
            t = ": " + " ".join(t)
        else:
            t = ""

        if results == SUCCESS:
            status = "Build succeeded!\n"
            res = "PASS"
        elif results == WARNINGS:
            status = "Build Had Warnings%s\n" % t
            res = "WARN"
        else:
            status = "BUILD FAILED%s\n" % t
            res = "FAIL"

        if build.getLogs():
            log = build.getLogs()[-1]
            laststep = log.getStep().getName()
            lastlog = log.getText()

            # only give me the last lines of the log files.
            lines = re.split('\n', lastlog)
            lastlog = ''
            for logline in lines[max(0, len(lines)-100):]:
                lastlog = lastlog + logline

        # TODO: it would be nice to provide a URL for the specific build
        # here. That involves some coordination with html.Waterfall .
        # Ideally we could do:
        #  helper = self.parent.getServiceNamed("html")
        #  if helper:
        #      url = helper.getURLForBuild(build)

        text = self.body % { 'result': res,
                             'builder': name,
                             'revision': revision,
                             'branch': ss.branch,
                             'blamelist': ",".join(build.getResponsibleUsers()),
                             'buildurl': buildurl,
                             'buildboturl': buildboturl,
                             'reason': buildreason,
                             'source': source,
                             'intro': intro,
                             'status': status,
                             'slave': buildslave,
                             'laststep': laststep,
                             'lastlog': lastlog,
                             }

        haveAttachments = False
        if ss.patch or self.addLogs:
            haveAttachments = True
            if not canDoAttachments:
                log.msg("warning: I want to send mail with attachments, "
                        "but this python is too old to have "
                        "email.MIMEMultipart . Please upgrade to python-2.3 "
                        "or newer to enable addLogs=True")

        if haveAttachments and canDoAttachments:
            m = MIMEMultipart()
            m.attach(MIMEText(text))
        else:
            m = Message()
            m.set_payload(text)

        m['Date'] = formatdate(localtime=True)
        m['Subject'] = self.subject % { 'result': res,
                                        'builder': name,
                                        'revision': revision,
                                        'branch': ss.branch
                                        }
        m['From'] = self.fromaddr
        # m['To'] is added later
        m['Reply-To'] = self.replytoaddr

        if ss.patch:
            a = MIMEText(patch)
            a.add_header('Content-Disposition', "attachment",
                         filename="source patch")
            m.attach(a)
        if self.addLogs:
            for log in build.getLogs():
                name = "%s.%s" % (log.getStep().getName(),
                                  log.getName())
                a = MIMEText(log.getText())
                a.add_header('Content-Disposition', "attachment",
                             filename=name)
                m.attach(a)

        # now, who is this message going to?
        dl = []
        recipients = self.extraRecipients[:]
        if self.sendToInterestedUsers and self.lookup:
            for u in build.getInterestedUsers():
                d = defer.maybeDeferred(self.lookup.getAddress, u)
                d.addCallback(recipients.append)
                dl.append(d)
        d = defer.DeferredList(dl)
        d.addCallback(self._gotRecipients, recipients, m)
        return d

