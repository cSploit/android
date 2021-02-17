[![Build Status](https://travis-ci.org/cSploit/daemon.svg)](https://travis-ci.org/cSploit/daemon)
cSploitd
======

a simple command dispatcher.

what it does
---------------
cSploitd receive your command requests, start them as children and report to you their output and exit code.
it allow you to send data to a child stdin.
a cool feature that cSploitd has it's the handlers.
handlers are plugins that pre-parse programs stdin and stdout.
for example we can parse the output of nmap in search of useful infos and send them alone,
saving thousands of bytes.


how it work
-------------

cSploitd has 6 kind of threads:

  - main: listen for new connection on the main socket.
  - connection reader: read messages from a connected client.
  - connection worker: process received messages.
  - connection writer: send messages to clients.
  - child: read process stdout, stderr and exit code.
  - reaper: join every thread found in the graveyard.

`main`, `reaper` are unique, only on thread of this kind is running.
on the other hand we have:
  - three `connection *` thread for every connected client.
  - one `child` thread for every running process.
