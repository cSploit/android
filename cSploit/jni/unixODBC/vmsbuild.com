$! vmsbuild.com -- DCL procedure to build unixODBC on OpenVMS
$!
$ say := "write sys$OUTPUT"
$ whoami = f$parse(f$environment("PROCEDURE"),,,,"NO_CONCEAL")
$ cwd = f$parse(whoami,,,"DEVICE") + f$parse(whoami,,,"DIRECTORY") - ".][000000]" - "][" - ".]" - "]" + "]"
$ set default 'cwd'
$! 
$ basedir = cwd - "]"
$ define/translation=concealed ODBCSRC "''basedir'.]"
$!
$ if p1 .eqs. "" then $goto ERR_NOPARAMS
$!
$ includes = "ODBCSRC:[include],ODBCSRC:[extras],""/ODBCSRC/libltdl"""
$! /first_include requires CC 6.4 or later but avoids impossibly long /define qualifier
$ CFLAGS="/names=as_is/prefix=all/include=(" + includes + ")/first_include=(ODBCSRC:[include]vmsconfig.h)
$ LFLAGS="/nodebug/notrace"
$!
$ if p2 .eqs. "DEBUG"
$ then
$     CFLAGS = CFLAGS + "/noopt/debug"
$     LFLAGS = LFLAGS + "/debug/trace"
$ endif
$!
$ if p1 .eqs. "CLEAN" 
$ then
$   set default 'cwd'
$   d = f$parse(whoami,,,"DEVICE") + f$parse(whoami,,,"DIRECTORY") - ".][000000]" - "][" - ".]" - "]" + "]"
$   say "Removing all object files and listings"
$   delete/noconfirm [...]*.obj;*, *.lis;*
$   say "Removing all object libraries and linker maps"
$   delete/noconfirm [...]*.olb;*, *.map;*
$   goto all_done
$ endif
$!
$ if p1 .eqs. "COMPILE" .or. p1 .eqs. "ALL"
$ then
$   say "Compiling unixODBC"
$   if f$search("ODBCSRC:[vms]libodbcinst.olb") .eqs. "" then library/create ODBCSRC:[vms]libodbcinst.olb
$   if f$search("ODBCSRC:[vms]libodbc.olb") .eqs. "" then library/create ODBCSRC:[vms]libodbc.olb
$   if f$search("ODBCSRC:[vms]libodbcpsql.olb") .eqs. "" then library/create ODBCSRC:[vms]libodbcpsql.olb
$   call create_vmsconfig_h
$   call compile "ODBCSRC:[extras]" "*.c" "ODBCSRC:[vms]libodbcinst.olb" "ODBCSRC:[vms]libodbc.olb"
$   call compile "ODBCSRC:[ini]" "*.c" "ODBCSRC:[vms]libodbcinst.olb" "ODBCSRC:[vms]libodbc.olb"
$   call compile "ODBCSRC:[log]" "*.c" "ODBCSRC:[vms]libodbcinst.olb"
$   call compile "ODBCSRC:[lst]" "*.c" "ODBCSRC:[vms]libodbcinst.olb" "ODBCSRC:[vms]libodbc.olb"
$   call compile "ODBCSRC:[odbcinst]" "*.c" "ODBCSRC:[vms]libodbcinst.olb"
$   call compile "ODBCSRC:[drivermanager]" "*.c" "ODBCSRC:[vms]libodbc.olb"
$   call compile "ODBCSRC:[exe]" "*.c"
$   if f$getdvi("ODBCSRC:", "ACPTYPE") .eqs. "F11V5"
$   then
$       set process/parse=extended
$       call compile "ODBCSRC:[Drivers.Postgre7^.1]" "*.c" "ODBCSRC:[vms]libodbcpsql.olb"
$   else
$       call compile "ODBCSRC:[Drivers.Postgre7_1]" "*.c" "ODBCSRC:[vms]libodbcpsql.olb"
$   endif
$   set default 'cwd'
$!
$ endif
$!
$ if f$trnlnm("ODBC_LIBDIR").eqs.""
$ then
$     if f$search("ODBCSRC:[000000]odbclib.dir") .eqs. ""
$     then
$       create/directory/log ODBCSRC:[odbclib]
$     endif
$     libdir = f$parse("ODBCSRC:[odbclib]",,,,"NO_CONCEAL")  -".][000000"-"]["-"].;"+"]"
$     define/process ODBC_LIBDIR 'libdir'
$     say "Defining ODBC_LIBDIR as """ + f$trnlnm("ODBC_LIBDIR") + """
$ else
$     if f$search("ODBC_LIBDIR") .eqs. ""
$     then
$       create/directory/log ODBC_LIBDIR
$     endif
$ endif
$!
$! Build Shared objects
$!
$ if p1 .eqs. "LINK" .or. p1 .eqs. "ALL"
$ then
$   set default ODBCSRC:[vms]
$!
$   say "Linking libodbcinst.exe"
$   link 'LFLAGS' libodbcinst.olb/library,odbcinst_axp.opt/opt/share=libodbcinst.exe
$!
$   say "Linking libodbc.exe"
$   link 'LFLAGS' libodbc.olb/library,odbc_axp.opt/opt/share=libodbc.exe
$!
$!  The following kludge brought to you by the fact that on ODS-5 disks the C compiler 
$!  may create module names in upper or mixed case depending on how the archive was 
$!  unpacked.  Try lower case and if it's not there, assume uppper case.
$!
$   module = "psqlodbc"
$   module_exists == 0
$   call check_module "''module'" "libodbcpsql.olb"
$   if .not. module_exists then module = f$edit(module,"UPCASE")
$!
$   say "Linking libodbcpsql.exe"
$   link 'LFLAGS' libodbcpsql.olb/library/include="''module'",odbc2_axp.opt/opt/share=libodbcpsql.exe
$!
$   set default ODBCSRC:[exe]
$   say "Linking isql.exe, dltest.exe and odbc-config.exe"
$   link 'LFLAGS' isql.OBJ,SYS$INPUT/OPT
ODBCSRC:[vms]libodbc.exe/SHARE
$ eod
$!
$   link 'LFLAGS' dltest.OBJ,ODBCSRC:[vms]libodbcinst.olb/library
$   link 'LFLAGS' odbc-config.OBJ
$ endif
$!
$ if p1 .eqs. "INSTALL" .or. p1 .eqs. "ALL"
$ then
$!
$   copy/log ODBCSRC:[vms]libodbc*.exe ODBC_LIBDIR:
$   copy/log ODBCSRC:[vms]odbc_setup.com ODBC_LIBDIR:
$   copy/log ODBCSRC:[exe]isql.exe ODBC_LIBDIR:
$   copy/log ODBCSRC:[exe]dltest.exe ODBC_LIBDIR:
$   copy/log ODBCSRC:[exe]odbc-config.exe ODBC_LIBDIR:
$!
$! check for odbc.ini and odbcinst.ini in ODBC_LIBDIR
$! 
$   file = f$search ("ODBC_LIBDIR:odbc.ini")
$   if file .eqs. ""
$   then 
$     say "Creating ODBC_LIBDIR:odbc.ini"
$     create ODBC_LIBDIR:odbc.ini

$   endif
$!
$   file = f$search ("ODBC_LIBDIR:odbcinst.ini")
$   if file .eqs. ""
$   then 
$     say "Creating ODBC_LIBDIR:odbcinst.ini"
$     create ODBC_LIBDIR:odbcinst.ini

$   endif
$!
$   set file ODBC_LIBDIR:*.*/protection=(w:re)
$ endif
$!
$ all_done:
$ set default 'cwd'
$ exit (1)
$!
$ ERR_NOPARAMS:
$ say " "
$ say "The correct calling sequence is: "
$ say " "
$ say "$ @vmsbuild p1 [p2]
$ say " "
$ say "Where p1 is : "
$ say " "
$ say "    ALL      : COMPILE LINK, and INSTALL steps."
$ say "    CLEAN    : Remove all objects and object libraries."
$ say "    COMPILE  : Compile all source and produce object libraries"
$ say "    LINK     : Create shareable images"
$ say "    INSTALL  : Copy shareable images to ODBC_LIBDIR:"
$ say " "
$ say "and DEBUG may optionally be specified for p2 when p1 is COMPILE, LINK, or ALL"
$ say " "
$ exit 44
$!
$! compile subroutine will compile all p2 source files
$! and place the resulting object files in libraries
$! specified as p3 and/or p4
$!
$ compile : subroutine
$ set default 'p1'
$ loop:
$   file = f$search ("''p2'",1)
$   if file .eqs. "" then goto complete
$   filename = f$parse (file,,,"name")
$   if f$edit(filename,"UPCASE") .eqs. "INIOS2PROFILE" then goto loop
$   object = F$SEARCH ("''filename'.OBJ;*",2)
$   if object .eqs. ""
$   then
$      say "cc" + CFLAGS + " ''filename'.c"
$      on warning then continue
$      cc 'CFLAGS' 'filename'
$!     keep module names upper case to avoid insanity on ODS-5 volumes
$      if p3 .nes. "" then library/replace/log 'p3' 'f$edit(filename,"UPCASE")'
$      if p4 .nes. "" then library/replace/log 'p4' 'f$edit(filename,"UPCASE")'
$   endif
$   goto loop
$ complete:
$ set default 'cwd'
$ exit
$ endsubroutine ! compile
$
$!
$ create_vmsconfig_h : subroutine
$!
$ SEARCH/KEY=(POS:2,SIZE:8) ODBCSRC:[000000]CONFIGURE. "VERSION="/EXACT/OUTPUT=version.tmp
$ open/read version version.tmp
$ read version versionline
$ close version
$ delete/noconfirm/nolog version.tmp;*
$ versionstring = f$element(1, "=", f$edit(versionline, "COLLAPSE"))
$!
$ if f$search("ODBCSRC:[include]vmsconfig.h") .nes. "" then delete/noconfirm/nolog ODBCSRC:[include]vmsconfig.h;*
$ open/write vmsconfig ODBCSRC:[include]vmsconfig.h
$ write vmsconfig "/* auto-generated definitions for VMS port */
$ write vmsconfig "#define UNIXODBC"
$ write vmsconfig "#define HAVE_PWD_H 1"
$ write vmsconfig "#if __VMS_VER >= 70000000"
$ write vmsconfig "#define HAVE_STRCASECMP"
$ write vmsconfig "#define HAVE_STRNCASECMP"
$ write vmsconfig "#define HAVE_STDARG_H"
$ write vmsconfig "#define HAVE_STDLIB_H"
$ write vmsconfig "#define HAVE_STRING_H"
$ write vmsconfig "#define HAVE_STRINGS_H"
$ write vmsconfig "#define HAVE_DTIME"
$ write vmsconfig "#define HAVE_SYS_TIME_H"
$ write vmsconfig "#define HAVE_GETTIMEOFDAY"
$ write vmsconfig "#define HAVE_UNISTD_H"
$ write vmsconfig "#endif"
$ write vmsconfig "#define SIZEOF_LONG_INT 4"
$ write vmsconfig "#define UNIXODBC_SOURCE"
$ write vmsconfig "#define readonly __readonly"
$ write vmsconfig "#define DEFLIB_PATH ""ODBC_LIBDIR:[LIB]"""
$ write vmsconfig "#define SYSTEM_LIB_PATH ""ODBC_LIBDIR:[LIB]"""
$ write vmsconfig "#define SHLIBEXT "".exe"""
$ write vmsconfig "#define VERSION ""''versionstring'"""
$ write vmsconfig "#define PREFIX ""ODBC_LIBDIR:"""
$ write vmsconfig "#define EXEC_PREFIX ""ODBC_LIBDIR:[BIN]"""
$ write vmsconfig "#define BIN_PREFIX ""ODBC_LIBDIR:[BIN]"""
$ write vmsconfig "#define INCLUDE_PREFIX ""ODBC_LIBDIR:[INCLUDE]"""
$ write vmsconfig "#define LIB_PREFIX ""ODBC_LIBDIR:[ETC]"""
$ write vmsconfig "#define SYSTEM_FILE_PATH ""ODBC_LIBDIR:[ETC]"""
$ write vmsconfig "char* getvmsenv (char* symbol);"
$ close vmsconfig
$!
$ if f$search("ODBCSRC:[include]unixodbc_conf.h") .nes. "" then delete/noconfirm/nolog ODBCSRC:[include]unixodbc_conf.h;*
$ open/write unixodbcconfig ODBCSRC:[include]unixodbc_conf.h
$ write unixodbcconfig "/* auto-generated definitions for VMS port */
$ write unixodbcconfig "#ifndef HAVE_PWD_H"
$ write unixodbcconfig " #define HAVE_PWD_H"
$ write unixodbcconfig "#endif"
$ write unixodbcconfig "#ifndef SIZEOF_LONG_INT"
$ write unixodbcconfig " #define SIZEOF_LONG_INT 8"
$ write unixodbcconfig "#endif"
$ close unixodbcconfig
$ if f$search("ODBCSRC:[include]config.h") .nes. "" then delete/noconfirm/nolog ODBCSRC:[include]config.h;*
$ open/write config_h ODBCSRC:[include]config.h
$ write config_h "/* The real stuff is in vmsconfig.h */
$ close config_h
$ exit
$ endsubroutine ! create_vmsconfig_h
$!
$ check_module : subroutine
$!  Check for presence of a particular module in an object library.
$!  p1: module name, p2: object library
$ set noon
$ define/user_mode sys$output nl:
$ define/user_mode sys$error nl:
$ library/list/only="''p1'" 'p2'
$ if .not. $status 
$ then
$!    probably LBR$_KEYNOTFND; defer other errors until link time
$     module_exists == 0
$ else
$     module_exists == 1
$ endif
$ set on
$ exit
$ endsubroutine ! check_module
