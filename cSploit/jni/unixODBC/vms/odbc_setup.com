$ whoami = f$parse(f$environment("PROCEDURE"),,,,"NO_CONCEAL")
$ whereami = f$parse(whoami,,,"DEVICE") + f$parse(whoami,,,"DIRECTORY") - ".][000000]" - "][" - ".]" - "]" + "]"
$ define ODBC_LIBDIR 'whereami'
$!
$ file_loop:
$   file = f$search("ODBC_LIBDIR:*ODBC*.EXE")
$   if file .eqs. "" then goto file_loop_end
$   image_name = f$parse(file,,,"NAME")
$   define 'f$edit(image_name,"UPCASE")' "ODBC_LIBDIR:''image_name'.EXE"
$   goto file_loop
$ file_loop_end:
$!
$ isql :== $ODBC_LIBDIR:ISQL.EXE
$ exit
