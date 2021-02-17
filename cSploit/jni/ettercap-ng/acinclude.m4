
dnl $Id: acinclude.m4,v 1.21 2004/09/17 19:38:34 alor Exp $

dnl
dnl EC_MESSAGE(MESSAGE)
dnl

AC_DEFUN([EC_MESSAGE],
[
   AC_MSG_RESULT()
   AC_MSG_RESULT(${SB}$1...${EB})
   AC_MSG_RESULT()
])

dnl
dnl EC_CHECK_OPTION(STRING, VAR)
dnl

AC_DEFUN([EC_CHECK_OPTION],
[
   echo "$1 ${SB}$2${EB}"
])


dnl
dnl EC_CHECK_FUNC(lib, func, ldflags, libs, action-if-found, action-if-not-found)
dnl

AC_DEFUN([EC_CHECK_FUNC],
[
   OLDLDFLAGS="${LDFLAGS}"
   OLDLIBS="${LIBS}"
   LDFLAGS="$3"
   LIBS="$4"
   AC_CHECK_LIB($1, $2, $5, $6)
   LDFLAGS="${OLDLDFLAGS}"
   LIBS="${OLDLIBS}"

])

dnl
dnl EC_PTHREAD_CHECK()
dnl

AC_DEFUN([EC_PTHREAD_CHECK],
[
   AC_SEARCH_LIBS(pthread_create, c_r pthread,,)
   
   if test "$OS" = "SOLARIS"; then
      AC_SEARCH_LIBS(_getfp, pthread,,)
   elif test "$OS" != "MACOSX" -a "$OS" != "WINDOWS"; then
      AC_MSG_CHECKING(whether $CC accepts -pthread)
      LDFLAGS_store="$LDFLAGS"
      LDFLAGS="$LDFLAGS -pthread"
      AC_TRY_LINK([
         #include <pthread.h>
         ],
         [
            int main(int argc, char **argv)
            {
               pthread_create(NULL, NULL, NULL, NULL);
               return 0;
            }
         ],
         [AC_MSG_RESULT(yes)],
         [AC_MSG_RESULT(no)
            LDFLAGS="$LDFLAGS_store"
            AC_MSG_WARN(***************************);
            AC_MSG_WARN(* PTHREAD ARE REQUIRED !! *);
            AC_MSG_WARN(***************************);
            exit
         ])
      unset LDFLAGS_store
   fi

])


dnl
dnl EC_WINDOWS_KERNEL()
dnl

AC_DEFUN([EC_WINDOWS_KERNEL],
[
   AC_MSG_CHECKING(Windows kernel version)
   tech=`uname | cut -f2 -d"_" | cut -f1 -d"-"`
   major=`uname | cut -f2 -d"-" | cut -f1 -d"."`
   minor=`uname | cut -f2 -d"-" | cut -f2 -d"."`
   AC_MSG_RESULT($tech $major.$minor)
   if test "$tech" != "NT"; then
      ac_ec_windows_version="-DWIN9X"
   elif test "$major$minor" -lt 50; then
      ac_ec_windows_version="-DWINNT"
   else
      ac_ec_windows_version="-DWIN2K_XP"
   fi
])

dnl
dnl EC_CYGWIN_KERNEL()
dnl

AC_DEFUN([EC_CYGWIN_KERNEL],
[
   AC_MSG_CHECKING(Cygwin dll version)
   uname=`uname -r | cut -f1 -d"("`
   major=`uname -r | cut -f1 -d"(" | cut -f1 -d"."`
   minor=`uname -r | cut -f1 -d"(" | cut -f2 -d"."`
   AC_MSG_RESULT($uname)
   if test "$major$minor" -lt 13; then
      AC_MSG_WARN(****************************);
      AC_MSG_WARN(* Cygwin 1.3.x REQUIRED !! *);
      AC_MSG_WARN(****************************);
      exit;
   fi
])

dnl
dnl EC_MINGW_KERNEL()
dnl

AC_DEFUN([EC_MINGW_KERNEL],
[
   AC_MSG_CHECKING(MingW32 version)
   uname=`uname -r | cut -f1 -d"("`
   major=`uname -r | cut -f1 -d"(" | cut -f1 -d"."`
   minor=`uname -r | cut -f1 -d"(" | cut -f2 -d"."`
   AC_MSG_RESULT($uname)
   if test "$major$minor" -lt 10; then
      AC_MSG_WARN(*****************************);
      AC_MSG_WARN(* MingW32 1.0.x REQUIRED !! *);
      AC_MSG_WARN(*****************************);
      exit;
   fi
])


dnl
dnl EC_GCC_MACRO()
dnl
dnl check if the compiler support __VA_ARGS__ in macro declarations
dnl

AC_DEFUN([EC_GCC_MACRO],
[
   AC_MSG_CHECKING(if your compiler supports __VA_ARGS__ in macro declarations)
   
   AC_TRY_RUN([
   
      #include <stdio.h>

      #define EXECUTE(x, ...) do{ if (x != NULL) x( __VA_ARGS__ ); }while(0)

      void foo() { }
      
      int main(int argc, char **argv)
      {
         EXECUTE(foo);
         return 0;
      } 
   ],
   [ AC_MSG_RESULT(yes) ],
   [ AC_MSG_RESULT(no) 
     AC_ERROR(please use gcc >= 3.2.x)
   ],
     AC_MSG_RESULT(unkown when cross-compiling)
   )
])

dnl
dnl EC_NS_GET()
dnl
dnl   returns  HAVE_NS_GET
dnl

AH_TEMPLATE(HAVE_NS_GET, [nameser NS_GET32])

AC_DEFUN([EC_NS_GET],
[
   AC_CHECK_HEADERS(arpa/nameser.h)
   AC_MSG_CHECKING(for NS_GET32)
   AC_TRY_RUN([
      #include <arpa/nameser.h>

      int main()
      {
         int i;
         char *p = "\x01\x02\x03\x04";
         NS_GET32(i, p);

         return 0;
      }
   ],
   [  AC_MSG_RESULT(yes)
      AC_DEFINE(HAVE_NS_GET,1) ],
   [  AC_MSG_RESULT(no); ]
   )

])

dnl
dnl EC_RESOLVE_CHECK()
dnl

AC_DEFUN([EC_RESOLVE_CHECK],
[
   AC_SEARCH_LIBS(dn_expand, resolv c,
      [
         AC_MSG_CHECKING(for additional -lresolv needed by dn_expand)
         AC_TRY_LINK([
               #include <sys/types.h>
               #include <netinet/in.h>
               #include <arpa/nameser.h>
               #include <resolv.h>
            ],
            [
               int main(int argc, char **argv)
               {
                  char *q;
                  char p[NS_MAXDNAME];

                  dn_expand(q, q, q, p, sizeof(p));
               } 
            ],
            [AC_MSG_RESULT(not needed)],
            [AC_MSG_RESULT(needed)
             LIBS="$LIBS -lresolv"]
         )
         AM_CONDITIONAL(HAVE_DN_EXPAND, true) ac_ec_dns=yes 
      ],
      [AM_CONDITIONAL(HAVE_DN_EXPAND, false) ac_ec_dns=no])

])

dnl
dnl EC_MINGW_SPECIAL_MAKEFILE()
dnl

AC_DEFUN([EC_MINGW_SPECIAL_MAKEFILE],
[
   EC_MESSAGE(Checking for required libraries)

directories="../winpcap/lib ../winpcap/include
             ../libnet/lib ../libnet/include
             ../pthreads/lib ../pthreads/include
             ../gw32c/lib ../gw32c/include
             ../libiconv/lib ../libiconv/include
             ../zlib/lib ../zlib/include
             ../regex/lib ../regex/include
             ../openssl/lib ../openssl/include
             ../gtk/lib ../gtk/include"

   for i in `echo $directories`; do
      echo -n "Searching for $i "
      if ! test -d $i; then
         echo "not found !"
         echo
         echo "Please read the README.PLATFORM for the instructions"
         echo "for the installation under MingW32"
         exit
      fi
      echo " ok."
   done
  
   EC_MESSAGE(Writing output files)
   dnl under mingw there is a special makefile
   dnl make the final adjustment
   sed -e "s/@VERSION@/${VERSION}/" < Makefile.mingw.in > Makefile
   mkdir lib
])

dnl vim:ts=3:expandtab
