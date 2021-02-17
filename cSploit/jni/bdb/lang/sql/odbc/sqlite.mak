# VC++ 6.0 Makefile for SQLite 2.8.17

#### The toplevel directory of the source tree.  This is the directory
#    that contains this "Makefile.in" and the "configure.in" script.

TOP = ..\sqlite

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

#### The suffix to add to executable files.  ".exe" for windows.
#    Nothing for unix.

EXE = .exe

#### C Compile and options for use in building executables that 
#    will run on the target platform.  This is usually the same
#    as BCC, unless you are cross-compiling.

TCC = cl -Gs -GX -D_WIN32 -DOS_WIN=1 -nologo -Zi

#### Should the database engine assume text is coded as UTF-8 or iso8859?

!IF "$(ENCODING)" != "UTF8"
ENCODING = ISO8859
!ENDIF

# You should not have to change anything below this line
###############################################################################

# This is how we compile

TCCX = $(TCC) $(OPTS) -DWIN32=1 -DTHREADSAFE=1 -I. -I$(TOP)/src

TCCXD = $(TCCX) -D_DLL

# Object files for the SQLite library.

LIBOBJ = attach.obj auth.obj btree.obj btree_rb.obj build.obj copy.obj \
         date.obj delete.obj expr.obj func.obj hash.obj insert.obj \
         main.obj os.obj pager.obj parse.obj pragma.obj printf.obj \
         random.obj select.obj \
	 table.obj tokenize.obj trigger.obj update.obj util.obj \
         vacuum.obj vdbe.obj vdbeaux.obj where.obj \
	 encode.obj opcodes.obj

# All of the source code files.

SRC = \
  $(TOP)/src/attach.c \
  $(TOP)/src/auth.c \
  $(TOP)/src/btree.c \
  $(TOP)/src/btree.h \
  $(TOP)/src/btree_rb.c \
  $(TOP)/src/build.c \
  $(TOP)/src/copy.c \
  $(TOP)/src/date.c \
  $(TOP)/src/delete.c \
  $(TOP)/src/expr.c \
  $(TOP)/src/hash.c \
  $(TOP)/src/hash.h \
  $(TOP)/src/insert.c \
  $(TOP)/src/main.c \
  $(TOP)/src/os.c \
  $(TOP)/src/pager.c \
  $(TOP)/src/pager.h \
  $(TOP)/src/parse.y \
  $(TOP)/src/pragma.c \
  $(TOP)/src/printf.c \
  $(TOP)/src/random.c \
  $(TOP)/src/select.c \
  $(TOP)/src/shell.c \
  $(TOP)/src/sqlite.h.in \
  $(TOP)/src/sqliteInt.h \
  $(TOP)/src/table.c \
  $(TOP)/src/func.c \
  $(TOP)/src/tclsqlite.c \
  $(TOP)/src/tokenize.c \
  $(TOP)/src/trigger.c \
  $(TOP)/src/update.c \
  $(TOP)/src/util.c \
  $(TOP)/src/vacuum.c \
  $(TOP)/src/vdbe.c \
  $(TOP)/src/vdbe.h \
  $(TOP)/src/vdbeaux.c \
  $(TOP)/src/vdbeInt.h \
  $(TOP)/src/where.c \
  $(TOP)/src/encode.c

# Header files used by all library source files.

HDR = \
   sqlite.h  \
   $(TOP)/src/btree.h \
   config.h \
   $(TOP)/src/hash.h \
   opcodes.h \
   $(TOP)/src/os.h \
   $(TOP)/src/sqliteInt.h  \
   $(TOP)/src/vdbe.h  \
   parse.h

# This is the default Makefile target.  The objects listed here
# are what get build when you type just "make" with no arguments.

all:	sqlite.h config.h sqlite.dll libsqlite.lib sqlite.exe

sqlite.dll:	$(LIBOBJ) sqlite.def
	echo #include "sqlite.h" > version.c
	echo const char sqlite_version[] = SQLITE_VERSION; >> version.c
	echo #ifdef SQLITE_UTF8 >> version.c
	echo const char sqlite_encoding[] = "UTF-8"; >> version.c
	echo #else >> version.c
	echo const char sqlite_encoding[] = "iso8859"; >> version.c
	echo #endif >> version.c
	$(TCCX) -c version.c
	link -release -nodefaultlib -dll msvcrt.lib kernel32.lib \
	    -def:$(TOP)\sqlite.def -out:$@ $(LIBOBJ)
	lib sqlite.lib version.obj

libsqlite.lib:	$(LIBOBJ)
	lib -out:$@ $(LIBOBJ)

sqlite.exe:	sqlite.dll
	$(TCCX) -o $@ $(TOP)/src/shell.c sqlite.lib

# Rules to build the LEMON compiler generator

lemon:	$(TOP)/tool/lemon.c $(TOP)/tool/lempar.c
	$(BCC) -o lemon $(TOP)/tool/lemon.c
	copy $(TOP)\tool\lempar.c .

attach.obj:	$(TOP)/src/attach.c $(HDR)
	$(TCCXD) -c $(TOP)/src/attach.c

auth.obj:	$(TOP)/src/auth.c $(HDR)
	$(TCCXD) -c $(TOP)/src/auth.c

btree.obj:	$(TOP)/src/btree.c $(HDR) $(TOP)/src/pager.h
	$(TCCXD) -c $(TOP)/src/btree.c

btree_rb.obj:	$(TOP)/src/btree_rb.c $(HDR)
	$(TCCXD) -c $(TOP)/src/btree_rb.c

build.obj:	$(TOP)/src/build.c $(HDR)
	$(TCCXD) -c $(TOP)/src/build.c

copy.obj:	$(TOP)/src/copy.c $(HDR)
	$(TCCXD) -c $(TOP)/src/copy.c

main.obj:	$(TOP)/src/main.c $(HDR)
	$(TCCXD) -c $(TOP)/src/main.c

pager.obj:	$(TOP)/src/pager.c $(HDR) $(TOP)/src/pager.h
	$(TCCXD) -c $(TOP)/src/pager.c

pragma.obj:	$(TOP)/src/pragma.c $(HDR)
	$(TCCXD) -c $(TOP)/src/pragma.c

os.obj:	$(TOP)/src/os.c $(HDR)
	$(TCCXD) -c $(TOP)/src/os.c

parse.obj:	parse.c $(HDR)
	$(TCCXD) -c parse.c

parse.h:	parse.c

parse.c:	$(TOP)/src/parse.y lemon
	copy $(TOP)\src\parse.y .
	.\lemon parse.y

sqlite.h:	$(TOP)/src/sqlite.h.in
	..\fixup < $(TOP)\src\sqlite.h.in > sqlite.h \
	    --VERS-- @$(TOP)\VERSION --ENCODING-- $(ENCODING)

config.h:
	echo #include "stdio.h" >temp.c
	echo int main(){printf( >>temp.c
	echo "#define SQLITE_PTR_SZ %d\n",sizeof(char*)); >>temp.c
	echo exit(0);} >>temp.c
	$(BCC) -o temp temp.c
	.\temp >config.h
	@del temp.*

opcodes.h:	$(TOP)/src/vdbe.c
	..\mkopc <$(TOP)/src/vdbe.c

tokenize.obj:	$(TOP)/src/tokenize.c $(HDR)
	$(TCCXD) -c $(TOP)/src/tokenize.c

util.obj:	$(TOP)/src/util.c $(HDR)
	$(TCCXD) -c $(TOP)/src/util.c

vdbe.obj:	$(TOP)/src/vdbe.c $(HDR)
	$(TCCXD) -c $(TOP)/src/vdbe.c

vdbeaux.obj:	$(TOP)/src/vdbeaux.c $(HDR)
	$(TCCXD) -c $(TOP)/src/vdbeaux.c

where.obj:	$(TOP)/src/where.c $(HDR)
	$(TCCXD) -c $(TOP)/src/where.c

date.obj:	$(TOP)/src/date.c $(HDR)
	$(TCCXD) -c $(TOP)/src/date.c

delete.obj:	$(TOP)/src/delete.c $(HDR)
	$(TCCXD) -c $(TOP)/src/delete.c

expr.obj:	$(TOP)/src/expr.c $(HDR)
	$(TCCXD) -c $(TOP)/src/expr.c

hash.obj:	$(TOP)/src/hash.c $(HDR)
	$(TCCXD) -c $(TOP)/src/hash.c

insert.obj:	$(TOP)/src/insert.c $(HDR)
	$(TCCXD) -c $(TOP)/src/insert.c

random.obj:	$(TOP)/src/random.c $(HDR)
	$(TCCXD) -c $(TOP)/src/random.c

select.obj:	$(TOP)/src/select.c $(HDR)
	$(TCCXD) -c $(TOP)/src/select.c

table.obj:	$(TOP)/src/table.c $(HDR)
	$(TCCXD) -c $(TOP)/src/table.c

func.obj:	$(TOP)/src/func.c $(HDR)
	$(TCCXD) -c $(TOP)/src/func.c

update.obj:	$(TOP)/src/update.c $(HDR)
	$(TCCXD) -c $(TOP)/src/update.c

printf.obj:	$(TOP)/src/printf.c $(HDR)
	$(TCCXD) -c $(TOP)/src/printf.c

encode.obj:	$(TOP)/src/encode.c $(HDR)
	$(TCCXD) -c $(TOP)/src/encode.c

trigger.obj:	$(TOP)/src/trigger.c $(HDR)
	$(TCCXD) -c $(TOP)/src/trigger.c

opcodes.obj:	$(TOP)/opcodes.c $(HDR)
	$(TCCXD) -c $(TOP)/opcodes.c

vacuum.obj:	$(TOP)/src/vacuum.c $(HDR)
	$(TCCXD) -c $(TOP)/src/vacuum.c

sqlite.def:	sqlite.h
	echo LIBRARY SQLITE > sqlite.def
	echo DESCRIPTION 'SQLite Library' >> sqlite.def
	echo EXPORTS >> sqlite.def
	echo sqlite_open >> sqlite.def
	echo sqlite_close >> sqlite.def
	echo sqlite_exec >> sqlite.def
	echo sqlite_last_insert_rowid >> sqlite.def
	echo sqlite_changes >> sqlite.def
	echo sqlite_error_string >> sqlite.def
	echo sqlite_interrupt >> sqlite.def
	echo sqlite_complete >> sqlite.def
	echo sqlite_busy_handler >> sqlite.def
	echo sqlite_busy_timeout >> sqlite.def
	echo sqlite_get_table >> sqlite.def
	echo sqlite_free_table >> sqlite.def
	echo sqlite_exec_printf >> sqlite.def
	echo sqlite_exec_vprintf >> sqlite.def
	echo sqlite_get_table_printf >> sqlite.def
	echo sqlite_get_table_vprintf >> sqlite.def
	echo sqlite_freemem >> sqlite.def
	echo sqlite_libversion >> sqlite.def
	echo sqlite_libencoding >> sqlite.def
	echo sqlite_create_function >> sqlite.def
	echo sqlite_create_aggregate >> sqlite.def
	echo sqlite_function_type >> sqlite.def
	echo sqlite_set_result_string >> sqlite.def
	echo sqlite_set_result_int >> sqlite.def
	echo sqlite_set_result_double >> sqlite.def
	echo sqlite_set_result_error >> sqlite.def
	echo sqlite_user_data >> sqlite.def
	echo sqlite_aggregate_context >> sqlite.def
	echo sqlite_aggregate_count >> sqlite.def
	echo sqlite_mprintf >> sqlite.def
	echo sqliteStrICmp >> sqlite.def
	echo sqlite_set_authorizer >> sqlite.def
	echo sqlite_trace >> sqlite.def
	echo sqlite_compile >> sqlite.def
	echo sqlite_step >> sqlite.def
	echo sqlite_finalize >> sqlite.def
	echo sqlite_vmprintf >> sqlite.def
	echo sqliteOsFileExists >> sqlite.def
	echo sqliteIsNumber >> sqlite.def
	echo sqliteStrNICmp >> sqlite.def
	echo sqlite_encode_binary >> sqlite.def
	echo sqlite_decode_binary >> sqlite.def

clean:	
	del *.obj
	del *.pdb
	del *.dll
	del *.lib
	del *.exe
	del sqlite.h
	del opcodes.h
	del opcodes.c
	del config.h
	del parse.h
	del parse.c
