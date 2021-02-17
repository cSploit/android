XNET - new local protocol implementation (win32)
------------------------------------------------

Firebird 2.0 has replaced the former (often referred to as IPC or IPServer)
implementation of the local transport protocol with a new one, named XNET.

It serves exactly the same goal - provide an efficient way to connect to the
local server (without a remote node name in the connection string) - but it's
implemented differently, in order to address the known issues with the old
protocol. Generally speaking, both implementations use shared memory for
inter-process communication, but XNET eliminates usage of window messages to
deliver attachment requests and it also implements another synchronization
logic.

Advantages of the XNET protocol over IPServer:

  - it works with Classic Server
  - it works for non-interactive services and terminal sessions
  - it doesn't lock up when using a few connections simultaneously

From the performance point of view, they should behave similarly, although
XNET is expected to be slightly faster.

As for disadvantages, there's only one - implementations are not compatible
with each other. It means that your fbclient.dll version should match the
version of the used server binaries (fbserver.exe or fb_inet_server.exe),
otherwise you won't be able to establish a local connection (a TCP localhost
loopback will do the trick, of course).
