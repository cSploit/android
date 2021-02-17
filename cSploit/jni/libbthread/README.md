libbthread
==========

library that provide some missing posix threading function to the bionic libc.

while i was porting tens of linux projects under the dSploit android app i found that
the bionic libc does not provide some POSIX thread functions like `pthread_cancel`, `pthread_testcancel` and so on.

so, i developed this library, which exploit some unused bits in the bionic thread structure.

there is many thing to develop, like support for deferred cancels, but basic thread cancellation works :smiley:

i hope that you find this library useful :wink: 

-- tux_mind

License
==========

Project is licensed under GNU LGPL v2.0 (Library General Public License)

pt-internal.h - is from The Android Open Source Project and licensed under Apache License, Version 2.0

building
========

```bash
$ autoreconf -i
$ export PATH="$PATH:/path/to/ndk/toolchains/llvm/prebuilt/linux-x86_64/bin"
$ target_host=aarch64-linux-android
$ export AR=${target_host}-ar
$ export AS=${target_host}-as
$ export CC=${target_host}21-clang
$ export CXX=${target_host}21-clang++
$ export LD=${target_host}-ld
$ export STRIP=${target_host}-strip
$ export CFLAGS="-fPIE -fPIC"
$ export LDFLAGS="-pie"
$ ./configure --host=${target_host}
$ make
```
