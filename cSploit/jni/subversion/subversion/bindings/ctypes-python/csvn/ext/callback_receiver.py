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
Some helper classes for converting c-style functions
which accept callbacks into Pythonic generator
functions.
"""

from threading import Thread, Lock, Condition, currentThread
import sys, traceback, weakref

class CallbackReceiver(Thread):
    """A thread which collects input from callbacks and
       sends them back to the main thread via an iterator.

       Classes which inherit from this class should define
       a 'collect' method which dispatches results to the
       iterator via the 'send' method."""

    def __init__(self, *args, **kwargs):
        """Create a new callback iterator"""
        Thread.__init__(self)
        self.args = args
        self.kwargs = kwargs
        self.lock = Lock()
        self.result_produced = Condition(self.lock)
        self.result_consumed = Condition(self.lock)
        self.result = None
        self.done = False
        self.consumed = False
        self.exception = None

    def run(self, *args, **kwargs):
        try:
            try:
                self.collect(*self.args, **self.kwargs)
            except:

                # Looks like the child thread bombed out!
                # Find out what happened.
                tb = traceback.format_exception(*sys.exc_info())
                self.exception = CollectionError(
                    "Exception thrown by collect method\n"
                    "%s" % "".join(tb))

        finally:

            # Wake up the parent, and tell 'im we're all
            # done our work
            self.done = True
            try:
                self.lock.acquire()
                self.result_produced.notify()
            finally:
                self.lock.release()


    def send(self, result):
        """Send a new result to the iterator"""
        try:
            self.lock.acquire()

            # Save the result
            self.result = result
            self.consumed = False

            # A new result is ready!
            self.result_produced.notify()

            # Wait for the iterator to pickup the result
            while not self.done and self.consumed is not True:
                self.result_consumed.wait(0.5)

                # If the calling thread died, we should stop.
                if not self.calling_thread.isAlive():
                    self.done = True
        finally:
            self.lock.release()

    def shutdown(self, _):
        """Shut down the child thread"""

        # Tell the child thread to wake up (and die) if it has been
        # waiting for us to consume some data
        self.done = True
        try:
            self.lock.acquire()
            self.result_consumed.notify()
        finally:
            self.lock.release()

    def __iter__(self):
        """Iterate over the callback results"""

        # Save a reference to the current thread
        self.calling_thread = currentThread()

        # Setup the iterator
        iterator = iter(_CallbackResultIterator(self))

        # Cleanup: When the iterator goes out of scope, shut
        #          down the collector thread.
        self.cleanup = weakref.ref(iterator, self.shutdown)

        return iterator

class _CallbackResultIterator(object):

    def __init__(self, receiver):
        self.receiver = receiver

    def __iter__(self):

        # Start up receiver thread, and
        # wait for the first result
        self.receiver.done = False
        try:
            self.receiver.lock.acquire()
            self.receiver.start()
            if not self.receiver.done:
                self.receiver.result_produced.wait()
        finally:
            self.receiver.lock.release()

        # Return the first result. Only Python 2.5 supports 'yield'
        # inside a try-finally block, so we jump through some hoops here
        # to avoid that case.
        if not self.receiver.done and self.receiver.result is not None:
            yield self.receiver.result

            try:
                self.receiver.lock.acquire()
                self.receiver.consumed = True
                self.receiver.result_consumed.notify()
            finally:
                self.receiver.lock.release()

        # Grab any further results
        while not self.receiver.done:

            # Wait for a result to be produced
            try:
                self.receiver.lock.acquire()
                self.receiver.result_produced.wait()
            finally:
                self.receiver.lock.release()

            # Look like a result is ready. Consume it
            if not self.receiver.done and self.receiver.result is not None:
                yield self.receiver.result

                # Notify the child thread that we've consumed their output
                try:
                    self.receiver.lock.acquire()
                    self.receiver.consumed = True
                    self.receiver.result_consumed.notify()
                finally:
                    self.receiver.lock.release()

        # If the child thread raised an exception, re-raise it
        if self.receiver.exception:
            raise self.receiver.exception

        # Wait for the thread to exit before returning
        self.receiver.join()

class CollectionError(Exception):
    pass
