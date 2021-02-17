# VC++ 6.0 Makefile for SQLite 3.6.22

#### The toplevel directory of the source tree.
#

TOP = ..\sqlite-3.6.22

#### C Compiler and options for use in building executables that
#    will run on the platform that is doing the build.

BCC = cl -Gs -GX -D_WIN32 -nologo -Zi

#### Leave MEMORY_DEBUG undefined for maximum speed.  Use MEMORY_DEBUG=1
#    to check for memory leaks.  Use MEMORY_DEBUG=2 to print a log of all
#    malloc()s and free()s in order to track down memory leaks.
#    
#    SQLite uses some expensive assert() statements in the inner loop.
#    You can make the library go almost twice as fast if you compile
#    with -DNDEBUG=1

#OPTS = -DMEMORY_DEBUG=2
#OPTS = -DMEMORY_DEBUG=1
#OPTS = 
OPTS = -DNDEBUG=1

#### C Compile and options for use in building executables that 
#    will run on the target platform.  This is usually the same
#    as BCC, unless you are cross-compiling.

TCC = cl -Gs -GX -D_WIN32 -nologo -Zi -DOS_WIN=1

# You should not have to change anything below this line
###############################################################################

# This is how we compile

TCCX = $(TCC) $(OPTS) -DWIN32=1 -DTHREADSAFE=1 -DSQLITE_OS_WIN=1 \
	-DSQLITE_ENABLE_COLUMN_METADATA=1 -DSQLITE_SOUNDEX=1 \
	-DSQLITE_OMIT_LOAD_EXTENSION=1 -I. -I$(TOP)

TCCXD = $(TCCX) -D_DLL

# Object files for the SQLite library.

LIBOBJ = sqlite3.obj

# This is the default Makefile target.  The objects listed here
# are what get build when you type just "make" with no arguments.

all:	libsqlite3.lib sqlite3.exe

libsqlite3.lib:	$(LIBOBJ)
	lib -out:$@ $(LIBOBJ)

sqlite3.obj:	sqlite3.c $(TOP)\sqlite3.h
	$(TCCXD) -c sqlite3.c

sqlite3.exe:		$(TOP)\shell.c sqlite3.c $(TOP)\sqlite3.h
	$(TCCX) -o $@ shell.c sqlite3.c

clean:
	del *.obj
	del *.pdb
	del *.lib
	del *.exe

