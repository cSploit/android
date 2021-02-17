-------------------------
SuperClassic architecture
-------------------------


Firebird 2.5 offers a new working mode, also known as the SuperClassic architecture.

Generally speaking, it's some kind of a compromise between Classic and SuperServer
targeted to shift the scalability limits of the regular Classic Server and provide
slightly better performance, as well as some features priorly unique to SuperServer.

Architecturally, SuperClassic is implemented inside a single server process, with
user attachments being served by threads. Similarly to SuperServer, these worker
threads are shared, i.e. the server maintains a thread pool and never uses
more threads than a number of active user requests running at the moment.

However, unlike SuperServer which shares the page cache and metadata cache between
attachments, SuperClassic uses the model of private per-connection caches, similarly
to the regular Classic, thus inheriting its memory usage implications.

Being implemented inside a single process, SuperClassic offers some benefits as
compared with the regular Classic:

  * Less OS kernel resources are used (e.g. desktop heap on Windows), it should
    allow better scalability in terms of a number of user attachments

  * Better performance due to the local calls (instead of IPC) inside the lock
    manager and other in-process optimizations

  * Server can be safely shutdown as a whole, all active attachments are gracefully
    disconnected

  * It's possible to enumerate attached databases and users via the Services API

  * Security database attachment is shared, thus allowing faster user connections

The drawbacks are the following:

  * Somewhat ineffective memory usage (same as for Classic)

  * High lock table contention (due to page locks), so it requires careful tuning
    the lock manager configuration (again, same as for Classic)
 
  * Server crash affects all user attachments (same as for SuperServer)

  * It doesn't make much sense on 32-bit systems, because the total page cache size
    (for all attachments) must fit the 2GB process address space limit

Please note that SuperClassic opens the database file in the shared mode, allowing
other Classic, SuperClassic or Embedded processes to access the database file at the
same time.

On Windows, SuperClassic shares the same binary fb_inet_server.exe with Classic.
A new command line switch -m (multi-threaded) should be used for fb_inet_server.exe
to start SuperClassic as an application, or for instsvc.exe to install SuperClassic
as a service.

On POSIX, a new binary fb_smp_server is introduced to run the server in the
SuperClassic mode. It should be installed as a system daemon, [x]inetd is not required.
