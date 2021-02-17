Author: Adriano dos Santos Fernandes <adrianosf@uol.com.br>
Date: 2008-12-15

Before FB 2.5, filenames used in the connection string are always passed from the client to the
server without any conversion. On the server, those filenames are used with OS API functions without
any conversion, too. This creates the situation where filenames using non-ASCII characters do not
interoperate well when the client and the server are different OS or even same OS using different
codepages.

The problem is addressed in FB 2.5 in the following way:

The filename is considered, by default, to be on the OS codepage.

A new DPB is introduced, named isc_dpb_utf8_filename. Its meaning is to change the rule above, so FB
should consider the passed filename as being in UTF-8.

If a v2.5 (or newer) client is communicating with a remote server older than v2.5, and
isc_dpb_utf8_filename was used, the client converts the filename from UTF-8 to the client
codepage and passes that filename to the server. The client removes isc_dpb_utf8_filename DPB.
This guarantees backward compatibility when people are using the same codepage on the client and
server OS.

If a v2.5 (or newer) client is communicating with a v2.5 (or newer) server, and
isc_dpb_utf8_filename was not used, the client converts the filename from the OS codepage to
UTF-8 and inserts the isc_dpb_utf8_filename DPB. If isc_dpb_utf8_filename was used, the client just
passes the original filename withing the DPB to the server. So the client always passes to the server
an UTF-8 filename and the isc_dpb_utf8_filename DPB.

The filename received on the server is subject to the same rules above. But note that v2.5 client
may automatically convert the filename and insert the DPB. Clients older than v2.5 do not,
so the received filenames are going to be considered as on the server codepage. We again guarantee
backward compatibility when client and server codepage are the same.

The OS codepage used for conversions is:
- Windows: The Windows ANSI code page
- Others: UTF-8


Updated for 2.5 Beta 2: 2008-07-11

These rules are now valid for string DPB items too.
