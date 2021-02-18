libbthread
==========

library that provide some missing posix threading function to the bionic libc.

while i was porting tens of linux projects under the dSploit android app i found that
the bionic libc does not provide some POSIX thread functions like `pthread_cancel`, `pthread_testcancel` and so on.

so, i developed this library, which exploit some unused bits in the bionic thread structure.

there is many thing to develop, like support for deferred cancels, but basic thread cancellation works :smiley:

i hope that you find this library useful :wink: 

-- tux_mind
