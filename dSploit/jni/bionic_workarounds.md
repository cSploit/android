Bionic libc workarounds
=======================

this is a file where you can find some workarounds to make standard C stuff
to work with the android ndk libc ( the bionic libc )

### 'struct lconv' has no member named 'decimal_point'

the bionic locale.h define a fake lconv struct ( FACEPALM ).
we have to hardcode things that can change with the 
LC_ALL / LANGUAGE / LANG envinroment variable.

```diff
-printf("hello workd 2%c0", lconv->decimal_point[0]);
+printf("hello workd 2%c0", '.');
```

### undefined reference to `pthread_cancel'

the bionic pthread implementation does not provide
`pthread_cancel`, `pthread_setcancelstate`, `pthread_setcanceltype`, 
`pthread_testcancel` functions.
note that pthread stands for POSIX threads, and POSIX means
Portable Operating System Interface for Unix.
so, another FACEPALM.

the solution it's quite ugly, we have to use signals to ask threads to cancel.
```diff
-pthread_testcancel();
+sigset_t waiting_mask;
+if(sigpending (&waiting_mask, SIGUSR1))
+  pthread_exit(0);
```
```diff
-pthread_cancel(thread_id);
+pthread_kill(thread_id,SIGUSR1);
```
> **NOTE:** remember to `#include <signals.h>` if not already included.

### undefined reference to `crypt_r'

bionic libc does not provede a crypt_r function as the glibc does.
we have to implent it.
look in [glibc_crypt](apr-util/crypto/glibc_crypt) for find my own implementation

### undefined reference to `crypt'

as above, but now it's a libc missing function...
look in [libc_crypt](apr-util/crypto/libc_crypt) for find my own implementation

