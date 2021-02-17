/*
** 2001 September 15
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
*************************************************************************
** Code for testing the pager.c module in SQLite.  This code
** is not included in the SQLite library.  It is used for automated
** testing of the SQLite library.
*/
#include "sqliteInt.h"
#include "tcl.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifndef SQLITE_OMIT_DISKIO
/*
** Interpret an SQLite error number
*/
static char *errorName(int rc){
  char *zName;
  switch( rc ){
    case SQLITE_OK:         zName = "SQLITE_OK";          break;
    case SQLITE_ERROR:      zName = "SQLITE_ERROR";       break;
    case SQLITE_PERM:       zName = "SQLITE_PERM";        break;
    case SQLITE_ABORT:      zName = "SQLITE_ABORT";       break;
    case SQLITE_BUSY:       zName = "SQLITE_BUSY";        break;
    case SQLITE_NOMEM:      zName = "SQLITE_NOMEM";       break;
    case SQLITE_READONLY:   zName = "SQLITE_READONLY";    break;
    case SQLITE_INTERRUPT:  zName = "SQLITE_INTERRUPT";   break;
    case SQLITE_IOERR:      zName = "SQLITE_IOERR";       break;
    case SQLITE_CORRUPT:    zName = "SQLITE_CORRUPT";     break;
    case SQLITE_FULL:       zName = "SQLITE_FULL";        break;
    case SQLITE_CANTOPEN:   zName = "SQLITE_CANTOPEN";    break;
    case SQLITE_PROTOCOL:   zName = "SQLITE_PROTOCOL";    break;
    case SQLITE_EMPTY:      zName = "SQLITE_EMPTY";       break;
    case SQLITE_SCHEMA:     zName = "SQLITE_SCHEMA";      break;
    case SQLITE_CONSTRAINT: zName = "SQLITE_CONSTRAINT";  break;
    case SQLITE_MISMATCH:   zName = "SQLITE_MISMATCH";    break;
    case SQLITE_MISUSE:     zName = "SQLITE_MISUSE";      break;
    case SQLITE_NOLFS:      zName = "SQLITE_NOLFS";       break;
    default:                zName = "SQLITE_Unknown";     break;
  }
  return zName;
}

/*
** Page size and reserved size used for testing.
*/
static int test_pagesize = 1024;
/*
** Usage:   fake_big_file  N  FILENAME
**
** Write a few bytes at the N megabyte point of FILENAME.  This will
** create a large file.  If the file was a valid SQLite database, then
** the next time the database is opened, SQLite will begin allocating
** new pages after N.  If N is 2096 or bigger, this will test the
** ability of SQLite to write to large files.
*/
static int fake_big_file(
  void *NotUsed,
  Tcl_Interp *interp,    /* The TCL interpreter that invoked this command */
  int argc,              /* Number of arguments */
  const char **argv      /* Text of each argument */
){
  sqlite3_vfs *pVfs;
  sqlite3_file *fd = 0;
  int rc;
  int n;
  i64 offset;
  char *zFile;
  int nFile;
  if( argc!=3 ){
    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
       " N-MEGABYTES FILE\"", 0);
    return TCL_ERROR;
  }
  if( Tcl_GetInt(interp, argv[1], &n) ) return TCL_ERROR;

  /*
   * This does not work with Berkeley DB. Making a large file does not cause
   * DB to skip the existing pages.
   */
  return TCL_ERROR;

  pVfs = sqlite3_vfs_find(0);
  nFile = (int)strlen(argv[2]);
  zFile = sqlite3_malloc( nFile+2 );
  if( zFile==0 ) return TCL_ERROR;
  memcpy(zFile, argv[2], nFile+1);
  zFile[nFile+1] = 0;
  rc = sqlite3OsOpenMalloc(pVfs, zFile, &fd, 
      (SQLITE_OPEN_CREATE|SQLITE_OPEN_READWRITE|SQLITE_OPEN_MAIN_DB), 0
  );
  if( rc ){
    Tcl_AppendResult(interp, "open failed: ", errorName(rc), 0);
    sqlite3_free(zFile);
    return TCL_ERROR;
  }
  offset = n;
  offset *= 1024*1024;
  rc = sqlite3OsWrite(fd, "Hello, World!", 14, offset);
  sqlite3OsCloseFree(fd);
  sqlite3_free(zFile);
  if( rc ){
    Tcl_AppendResult(interp, "write failed: ", errorName(rc), 0);
    return TCL_ERROR;
  }
  return TCL_OK;
}
#endif

/*
** sqlite3BitvecBuiltinTest SIZE PROGRAM
**
** Invoke the SQLITE_TESTCTRL_BITVEC_TEST operator on test_control.
** See comments on sqlite3BitvecBuiltinTest() for additional information.
*/
static int testBitvecBuiltinTest(
  void *NotUsed,
  Tcl_Interp *interp,    /* The TCL interpreter that invoked this command */
  int argc,              /* Number of arguments */
  const char **argv      /* Text of each argument */
){
  int sz, rc;
  int nProg = 0;
  int aProg[100];
  const char *z;
  if( argc!=3 ){
    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
                     " SIZE PROGRAM\"", (void*)0);
  }
  if( Tcl_GetInt(interp, argv[1], &sz) ) return TCL_ERROR;
  z = argv[2];
  while( nProg<99 && *z ){
    while( *z && !sqlite3Isdigit(*z) ){ z++; }
    if( *z==0 ) break;
    aProg[nProg++] = atoi(z);
    while( sqlite3Isdigit(*z) ){ z++; }
  }
  aProg[nProg] = 0;
  rc = sqlite3_test_control(SQLITE_TESTCTRL_BITVEC_TEST, sz, aProg);
  Tcl_SetObjResult(interp, Tcl_NewIntObj(rc));
  return TCL_OK;
}  

static int t2_tcl_function_stub(
  void *NotUsed,
  Tcl_Interp *interp,    /* The TCL interpreter that invoked this command */
  int argc,              /* Number of arguments */
  const char **argv      /* Text of each argument */
){
  return TCL_OK;
}

/*
** Register commands with the TCL interpreter.
*/
int Sqlitetest2_Init(Tcl_Interp *interp){
  static struct {
    char *zName;
    Tcl_CmdProc *xProc;
  } aCmd[] = {
    { "pager_open",              (Tcl_CmdProc*)t2_tcl_function_stub },
    { "pager_close",             (Tcl_CmdProc*)t2_tcl_function_stub },
    { "pager_commit",            (Tcl_CmdProc*)t2_tcl_function_stub },
    { "pager_rollback",          (Tcl_CmdProc*)t2_tcl_function_stub },
    { "pager_stmt_begin",        (Tcl_CmdProc*)t2_tcl_function_stub },
    { "pager_stmt_commit",       (Tcl_CmdProc*)t2_tcl_function_stub },
    { "pager_stmt_rollback",     (Tcl_CmdProc*)t2_tcl_function_stub },
    { "pager_stats",             (Tcl_CmdProc*)t2_tcl_function_stub },
    { "pager_pagecount",         (Tcl_CmdProc*)t2_tcl_function_stub },
    { "page_get",                (Tcl_CmdProc*)t2_tcl_function_stub },
    { "page_lookup",             (Tcl_CmdProc*)t2_tcl_function_stub },
    { "page_unref",              (Tcl_CmdProc*)t2_tcl_function_stub },
    { "page_read",               (Tcl_CmdProc*)t2_tcl_function_stub },
    { "page_write",              (Tcl_CmdProc*)t2_tcl_function_stub },
    { "page_number",             (Tcl_CmdProc*)t2_tcl_function_stub },
    { "pager_truncate",          (Tcl_CmdProc*)t2_tcl_function_stub },
#ifndef SQLITE_OMIT_DISKIO
    { "fake_big_file",           (Tcl_CmdProc*)fake_big_file       },
#endif
    { "sqlite3BitvecBuiltinTest",(Tcl_CmdProc*)testBitvecBuiltinTest     },
    { "sqlite3_test_control_pending_byte", (Tcl_CmdProc*)t2_tcl_function_stub },
  };
  int i;
  for(i=0; i<sizeof(aCmd)/sizeof(aCmd[0]); i++){
    Tcl_CreateCommand(interp, aCmd[i].zName, aCmd[i].xProc, 0, 0);
  }
#ifndef SQLITE_OMIT_WSD
  Tcl_LinkVar(interp, "sqlite_pending_byte",
     (char*)&sqlite3PendingByte, TCL_LINK_INT | TCL_LINK_READ_ONLY);
#endif
  return TCL_OK;
}
