dSploitd
======

a simple command dispatcher.

what it does
---------------
dSploitd receive your command requests, start them as children and report to you their output and exit code.
it allow you to send data to a child stdin.
a cool feature that dSploitd has it's the handlers.
handlers are plugins that pre-parse programs stdin and stdout. for example we can parse the output of nmap in search of useful infos and send it as binary format instead of send thousands of bytes to the dSploitd clients.


how it work
-------------

dSploitd has 6 kind of threads:

  - main: listen for new connection on the main socket.
  - connection: read messages from a connected client.
  - child: read process stdout and exit code.
  - sender: send messages to clients.
  - dispatcher: receive and dispatch messages from clients.
  - reaper: join every thread found in the graveyard.

`main`, `reaper`, `sender` and `dispatcher` are unique, only on thread of this kind is running.
on the other hand we have:
  - one `connection` thread for every connected client.
  - one `child` thread for every running process.
