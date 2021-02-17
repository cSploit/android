$! FreeTDS - Library of routines accessing Sybase and Microsoft databases
$! Copyright (C) 2003  Craig A. Berry   craigberry@mac.com      1-FEB-2003
$! 
$! This library is free software; you can redistribute it and/or
$! modify it under the terms of the GNU Library General Public
$! License as published by the Free Software Foundation; either
$! version 2 of the License, or (at your option) any later version.
$! 
$! This library is distributed in the hope that it will be useful,
$! but WITHOUT ANY WARRANTY; without even the implied warranty of
$! MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
$! Library General Public License for more details.
$! 
$! You should have received a copy of the GNU Library General Public
$! License along with this library; if not, write to the
$! Free Software Foundation, Inc., 59 Temple Place - Suite 330,
$! Boston, MA 02111-1307, USA.
$!
$! $Id: configure.com,v 1.6 2008-02-28 23:27:14 jklowden Exp $
$!
$! CONFIGURE.COM -- run from top level source directory as @[.vms]configure
$!
$! Checks for C library functions and applies its findings to 
$! description file template and config.h.  Much of this cribbed
$! from Perl's configure.com, largely the work of Peter Prymmer.
$!
$ SAY := "write sys$output"
$!
$ SEARCH/KEY=(POS:2,SIZE:8) SYS$DISK:[]Configure. "VERSION="/EXACT/OUTPUT=version.tmp
$ open/read version version.tmp
$ read version versionline
$ close version
$ delete/noconfirm/nolog version.tmp;*
$ versionstring = f$element(1, "=", f$edit(versionline, "COLLAPSE"))
$ gosub check_crtl
$!
$! The system-supplied iconv() is fine, but unless the internationalization
$! kit has been installed, we may not have the conversions we need.  Check
$! for their presence and use the homegrown iconv() if necessary.
$!
$ IF -
    "FALSE" - ! native iconv() buggy, don't use for now
    .AND. F$SEARCH("SYS$I18N_ICONV:UCS-2_ISO8859-1.ICONV") .NES. "" -
    .AND. F$SEARCH("SYS$I18N_ICONV:ISO8859-1_UCS-2.ICONV") .NES. "" -
    .AND. F$SEARCH("SYS$I18N_ICONV:UTF-8_ISO8859-1.ICONV") .NES. "" -
    .AND. F$SEARCH("SYS$I18N_ICONV:ISO8859-1_UTF-8.ICONV") .NES. ""
$ THEN
$   d_have_iconv = "1"
$   SAY "Using system-supplied iconv()"
$ ELSE
$   d_have_iconv = "0"
$   SAY "Using replacement iconv()"
$ ENDIF
$!
$! Generate config.h
$!
$ open/write vmsconfigtmp vmsconfigtmp.com
$ write vmsconfigtmp "$ define/user_mode/nolog SYS$OUTPUT _NLA0:"
$ write vmsconfigtmp "$ edit/tpu/nodisplay/noinitialization -"
$ write vmsconfigtmp "/section=sys$library:eve$section.tpu$section -"
$ write vmsconfigtmp "/command=sys$input/output=[.include]config.h [.vms]config_h.vms"
$ write vmsconfigtmp "input_file := GET_INFO (COMMAND_LINE, ""file_name"");"
$ write vmsconfigtmp "main_buffer:= CREATE_BUFFER (""main"", input_file);"
$ write vmsconfigtmp "POSITION (BEGINNING_OF (main_buffer));"
$ write vmsconfigtmp "eve_global_replace(""@D_ASPRINTF@"",""''d_asprintf'"");"
$ write vmsconfigtmp "POSITION (BEGINNING_OF (main_buffer));"
$ write vmsconfigtmp "eve_global_replace(""@D_VASPRINTF@"",""''d_vasprintf'"");"
$ write vmsconfigtmp "POSITION (BEGINNING_OF (main_buffer));"
$ write vmsconfigtmp "eve_global_replace(""@D_STRTOK_R@"",""''d_strtok_r'"");"
$ write vmsconfigtmp "POSITION (BEGINNING_OF (main_buffer));"
$ write vmsconfigtmp "eve_global_replace(""@VERSION@"",""""""''versionstring'"""""");"
$ write vmsconfigtmp "POSITION (BEGINNING_OF (main_buffer));"
$ write vmsconfigtmp "eve_global_replace(""@D_HAVE_ICONV@"",""''d_have_iconv'"");"
$ write vmsconfigtmp "POSITION (BEGINNING_OF (main_buffer));"
$ write vmsconfigtmp "eve_global_replace(""@D_SNPRINTF@"",""''d_snprintf'"");"
$ write vmsconfigtmp "out_file := GET_INFO (COMMAND_LINE, ""output_file"");"
$ write vmsconfigtmp "WRITE_FILE (main_buffer, out_file);"
$ write vmsconfigtmp "quit;"
$ write vmsconfigtmp "$ exit"
$ close vmsconfigtmp
$ @vmsconfigtmp.com
$ delete/noconfirm/nolog vmsconfigtmp.com;
$!
$! Generate descrip.mms from template
$!
$ if d_asprintf .eqs. "1" 
$ then
$   asprintfobj = " "
$ else
$   asprintfobj = "[.src.replacements]asprintf$(OBJ),"
$ endif
$!
$ if d_vasprintf .eqs. "1" 
$ then
$   vasprintfobj = " "
$ else
$   vasprintfobj = "[.src.replacements]vasprintf$(OBJ),"
$ endif
$!
$ if d_strtok_r .eqs. "1" 
$ then
$   strtok_robj = " "
$ else
$   strtok_robj = "[.src.replacements]strtok_r$(OBJ),"
$ endif
$!
$ if d_have_iconv .eqs. "1" 
$ then
$   libiconvobj = " "
$ else
$   libiconvobj = "[.src.replacements]libiconv$(OBJ),"
$   copy/noconfirm/nolog [.src.replacements]iconv.c [.src.replacements]libiconv.c
$ endif
$!
$ if d_snprintf .eqs. "1" 
$ then
$   snprintfobj = " "
$ else
$   snprintfobj = "[.src.replacements]snprintf$(OBJ),"
$ endif
$!
$ if P1 .eqs. "--disable-thread-safe"
$ then
$   enable_thread_safe = " "
$ else
$   enable_thread_safe = "ENABLE_THREAD_SAFE = 1"
$ endif
$!
$ open/write vmsconfigtmp vmsconfigtmp.com
$ write vmsconfigtmp "$ define/user_mode/nolog SYS$OUTPUT _NLA0:"
$ write vmsconfigtmp "$ edit/tpu/nodisplay/noinitialization -"
$ write vmsconfigtmp "/section=sys$library:eve$section.tpu$section -"
$ write vmsconfigtmp "/command=sys$input/output=[]descrip.mms [.vms]descrip_mms.template"
$ write vmsconfigtmp "input_file := GET_INFO (COMMAND_LINE, ""file_name"");"
$ write vmsconfigtmp "main_buffer:= CREATE_BUFFER (""main"", input_file);"
$ write vmsconfigtmp "POSITION (BEGINNING_OF (main_buffer));"
$ write vmsconfigtmp "eve_global_replace(""@ASPRINTFOBJ@"",""''asprintfobj'"");"
$ write vmsconfigtmp "POSITION (BEGINNING_OF (main_buffer));"
$ write vmsconfigtmp "eve_global_replace(""@VASPRINTFOBJ@"",""''vasprintfobj'"");"
$ write vmsconfigtmp "POSITION (BEGINNING_OF (main_buffer));"
$ write vmsconfigtmp "eve_global_replace(""@STRTOK_ROBJ@"",""''strtok_robj'"");"
$ write vmsconfigtmp "POSITION (BEGINNING_OF (main_buffer));"
$ write vmsconfigtmp "eve_global_replace(""@LIBICONVOBJ@"",""''libiconvobj'"");"
$ write vmsconfigtmp "POSITION (BEGINNING_OF (main_buffer));"
$ write vmsconfigtmp "eve_global_replace(""@SNPRINTFOBJ@"",""''snprintfobj'"");"
$ write vmsconfigtmp "POSITION (BEGINNING_OF (main_buffer));"
$ write vmsconfigtmp "eve_global_replace(""@ENABLE_THREAD_SAFE@"",""''enable_thread_safe'"");"
$ write vmsconfigtmp "out_file := GET_INFO (COMMAND_LINE, ""output_file"");"
$ write vmsconfigtmp "WRITE_FILE (main_buffer, out_file);"
$ write vmsconfigtmp "quit;"
$ write vmsconfigtmp "$ exit"
$ close vmsconfigtmp
$ @vmsconfigtmp.com
$ delete/noconfirm/nolog vmsconfigtmp.com;
$!
$! C99 requires t, z, and j modifiers to decimal format specifiers
$! but the HP compiler doesn't handle them, so replace the one
$! use of %td with %ld.
$!
$ open/write vmsbsqldbtmp vmsbsqldbtmp.com
$ write vmsbsqldbtmp "$ define/user_mode/nolog SYS$OUTPUT _NLA0:"
$ write vmsbsqldbtmp "$ edit/tpu/nodisplay/noinitialization -"
$ write vmsbsqldbtmp "/section=sys$library:eve$section.tpu$section -"
$ write vmsbsqldbtmp "/command=sys$input/output=[.src.apps]bsqldb.c [.src.apps]bsqldb.c"
$ write vmsbsqldbtmp "input_file := GET_INFO (COMMAND_LINE, ""file_name"");"
$ write vmsbsqldbtmp "main_buffer:= CREATE_BUFFER (""main"", input_file);"
$ write vmsbsqldbtmp "POSITION (BEGINNING_OF (main_buffer));"
$ write vmsbsqldbtmp "eve_global_replace("" %td "","" %ld "");"
$ write vmsbsqldbtmp "out_file := GET_INFO (COMMAND_LINE, ""output_file"");"
$ write vmsbsqldbtmp "WRITE_FILE (main_buffer, out_file);"
$ write vmsbsqldbtmp "quit;"
$ write vmsbsqldbtmp "$ exit"
$ close vmsbsqldbtmp
$ @vmsbsqldbtmp.com
$ delete/noconfirm/nolog vmsbsqldbtmp.com;
$!
$ Say ""
$ Say "Configuration complete; run MMK or MMS to build."
$ EXIT
$!
$ CHECK_CRTL:
$!
$ OS := "open/write CONFIG []try.c"
$ WS := "write CONFIG"
$ CS := "close CONFIG"
$ DS := "delete/nolog/noconfirm []try.*;*"
$ good_compile = %X10B90001
$ good_link = %X10000001
$ tmp = "" ! null string default
$!
$! Check for asprintf
$!
$ OS
$ WS "#include <stdio.h>"
$ WS "#include <stdlib.h>"
$ WS "int main()"
$ WS "{"
$ WS "char *ptr;
$ WS "asprintf(&ptr,""%d"",1);"
$ WS "exit(0);"
$ WS "}"
$ CS
$ tmp = "asprintf"
$ GOSUB inlibc
$ d_asprintf == tmp
$!
$!
$! Check for vasprintf
$!
$ OS
$ WS "#include <stdarg.h>"
$ WS "#include <stdio.h>"
$ WS "#include <stdlib.h>"
$ WS "int main()"
$ WS "{"
$ WS "char *ptr;
$ WS "vasprintf(&ptr,""%d,%d"",1,2);"
$ WS "exit(0);"
$ WS "}"
$ CS
$ tmp = "vasprintf"
$ GOSUB inlibc
$ d_vasprintf == tmp
$!
$!
$!
$! Check for strtok_r
$!
$ OS
$ WS "#include <stdlib.h>"
$ WS "#include <string.h>"
$ WS "int main()"
$ WS "{"
$ WS "char *word, *brkt, mystr[4];"
$ WS "strcpy(mystr,""1^2"");"
$ WS "word = strtok_r(mystr, ""^"", &brkt);"
$ WS "exit(0);"
$ WS "}"
$ CS
$ tmp = "strtok_r"
$ GOSUB inlibc
$ d_strtok_r == tmp
$!
$!
$! Check for snprintf
$!
$ OS
$ WS "#include <stdarg.h>"
$ WS "#include <stdio.h>"
$ WS "#include <stdlib.h>"
$ WS "int main()"
$ WS "{"
$ WS "char ptr[15];"
$ WS "snprintf((char*)&ptr,sizeof(ptr),""%d,%d"",1,2);"
$ WS "exit(0);"
$ WS "}"
$ CS
$ tmp = "snprintf"
$ GOSUB inlibc
$ d_snprintf == tmp
$!
$ DS
$ RETURN
$!
$!********************
$inlibc: 
$ GOSUB link_ok
$ IF compile_status .EQ. good_compile .AND. link_status .EQ. good_link
$ THEN
$   say "''tmp'() found."
$   tmp = "1"
$ ELSE
$   say "''tmp'() NOT found."
$   tmp = "0"
$ ENDIF
$ RETURN
$!
$!: define a shorthand compile call
$compile:
$ GOSUB link_ok
$just_mcr_it:
$ IF compile_status .EQ. good_compile .AND. link_status .EQ. good_link
$ THEN
$   OPEN/WRITE CONFIG []try.out
$   DEFINE/USER_MODE SYS$ERROR CONFIG
$   DEFINE/USER_MODE  SYS$OUTPUT CONFIG
$   MCR []try.exe
$   CLOSE CONFIG
$   OPEN/READ CONFIG []try.out
$   READ CONFIG tmp
$   CLOSE CONFIG
$   DELETE/NOLOG/NOCONFIRM []try.out;
$   DELETE/NOLOG/NOCONFIRM []try.exe;
$ ELSE
$   tmp = "" ! null string default
$ ENDIF
$ RETURN
$!
$link_ok:
$ GOSUB compile_ok
$ DEFINE/USER_MODE SYS$ERROR _NLA0:
$ DEFINE/USER_MODE SYS$OUTPUT _NLA0:
$ SET NOON
$ LINK try.obj
$ link_status = $status
$ SET ON
$ IF F$SEARCH("try.obj") .NES. "" THEN DELETE/NOLOG/NOCONFIRM try.obj;
$ RETURN
$!
$!: define a shorthand compile call for compilations that should be ok.
$compile_ok:
$ DEFINE/USER_MODE SYS$ERROR _NLA0:
$ DEFINE/USER_MODE SYS$OUTPUT _NLA0:
$ SET NOON
$ CC try.c
$ compile_status = $status
$ SET ON
$ DELETE/NOLOG/NOCONFIRM try.c;
$ RETURN
$!
$beyond_compile_ok:
$!
