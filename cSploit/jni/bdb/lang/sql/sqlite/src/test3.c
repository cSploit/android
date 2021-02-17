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
** Code for testing the btree.c module in SQLite.  This code
** is not included in the SQLite library.  It is used for automated
** testing of the SQLite library.
*/
#include "sqliteInt.h"
#include "btreeInt.h"
#include "tcl.h"
#include <stdlib.h>
#include <string.h>

static int t3_tcl_function_stub(
  void *NotUsed,
  Tcl_Interp *interp,    /* The TCL interpreter that invoked this command */
  int argc,              /* Number of arguments */
  const char **argv      /* Text of each argument */
){
  return TCL_OK;
}

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
    case SQLITE_LOCKED:     zName = "SQLITE_LOCKED";      break;
    default:                zName = "SQLITE_Unknown";     break;
  }
  return zName;
}

/*
** A bogus sqlite3 connection structure for use in the btree
** tests.
*/
static sqlite3 sDb;
static int nRefSqlite3 = 0;

/*
** Usage:   btree_open FILENAME NCACHE
**
** Open a new database
*/
static int btree_open(
  void *NotUsed,
  Tcl_Interp *interp,    /* The TCL interpreter that invoked this command */
  int argc,              /* Number of arguments */
  const char **argv      /* Text of each argument */
){
  Btree *pBt;
  int rc, nCache;
  char zBuf[100];
  int n;
  char *zFilename;
  if( argc!=3 ){
    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
       " FILENAME NCACHE FLAGS\"", 0);
    return TCL_ERROR;
  }
  if( Tcl_GetInt(interp, argv[2], &nCache) ) return TCL_ERROR;
  nRefSqlite3++;
  if( nRefSqlite3==1 ){
    sDb.pVfs = sqlite3_vfs_find(0);
    sDb.mutex = sqlite3MutexAlloc(SQLITE_MUTEX_RECURSIVE);
    sqlite3_mutex_enter(sDb.mutex);
  }
  n = (int)strlen(argv[1]);
  zFilename = sqlite3_malloc( n+2 );
  if( zFilename==0 ) return TCL_ERROR;
  memcpy(zFilename, argv[1], n+1);
  zFilename[n+1] = 0;
  rc = sqlite3BtreeOpen(sDb.pVfs, zFilename, &sDb, &pBt, 0, 
     SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_MAIN_DB);
  sqlite3_free(zFilename);
  if( rc!=SQLITE_OK ){
    Tcl_AppendResult(interp, errorName(rc), 0);
    return TCL_ERROR;
  }
  sqlite3BtreeSetCacheSize(pBt, nCache);
  sqlite3_snprintf(sizeof(zBuf), zBuf,"%p", pBt);
  Tcl_AppendResult(interp, zBuf, 0);
  return TCL_OK;
}

/*
** Usage:   btree_close ID
**
** Close the given database.
*/
static int btree_close(
  void *NotUsed,
  Tcl_Interp *interp,    /* The TCL interpreter that invoked this command */
  int argc,              /* Number of arguments */
  const char **argv      /* Text of each argument */
){
  Btree *pBt;
  int rc;
  if( argc!=2 ){
    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
       " ID\"", 0);
    return TCL_ERROR;
  }
  pBt = sqlite3TestTextToPtr(argv[1]);
  rc = sqlite3BtreeClose(pBt);
  if( rc!=SQLITE_OK ){
    Tcl_AppendResult(interp, errorName(rc), 0);
    return TCL_ERROR;
  }
  nRefSqlite3--;
  if( nRefSqlite3==0 ){
    sqlite3_mutex_leave(sDb.mutex);
    sqlite3_mutex_free(sDb.mutex);
    sDb.mutex = 0;
    sDb.pVfs = 0;
  }
  return TCL_OK;
}


/*
** Usage:   btree_begin_transaction ID
**
** Start a new transaction
*/
static int btree_begin_transaction(
  void *NotUsed,
  Tcl_Interp *interp,    /* The TCL interpreter that invoked this command */
  int argc,              /* Number of arguments */
  const char **argv      /* Text of each argument */
){
  Btree *pBt;
  int rc;
  if( argc!=2 ){
    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
       " ID\"", 0);
    return TCL_ERROR;
  }
  pBt = sqlite3TestTextToPtr(argv[1]);
  sqlite3BtreeEnter(pBt);
  rc = sqlite3BtreeBeginTrans(pBt, 1);
  sqlite3BtreeLeave(pBt);
  if( rc!=SQLITE_OK ){
    Tcl_AppendResult(interp, errorName(rc), 0);
    return TCL_ERROR;
  }
  return TCL_OK;
}

/*
** Usage:   btree_pager_stats ID
**
** Returns pager statistics
*/
static int btree_pager_stats(
  void *NotUsed,
  Tcl_Interp *interp,    /* The TCL interpreter that invoked this command */
  int argc,              /* Number of arguments */
  const char **argv      /* Text of each argument */
){
  Btree *pBt;
  int i;
  int *a;

  if( argc!=2 ){
    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
       " ID\"", 0);
    return TCL_ERROR;
  }
  pBt = sqlite3TestTextToPtr(argv[1]);
 
  /* Normally in this file, with a b-tree handle opened using the 
  ** [btree_open] command it is safe to call sqlite3BtreeEnter() directly.
  ** But this function is sometimes called with a btree handle obtained
  ** from an open SQLite connection (using [btree_from_db]). In this case
  ** we need to obtain the mutex for the controlling SQLite handle before
  ** it is safe to call sqlite3BtreeEnter().
  */
  sqlite3_mutex_enter(pBt->db->mutex);

  sqlite3BtreeEnter(pBt);
  a = sqlite3PagerStats(sqlite3BtreePager(pBt));
  for(i=0; i<11; i++){
    static char *zName[] = {
      "ref", "page", "max", "size", "state", "err",
      "hit", "miss", "ovfl", "read", "write"
    };
    char zBuf[100];
    Tcl_AppendElement(interp, zName[i]);
    sqlite3_snprintf(sizeof(zBuf), zBuf,"%d",a[i]);
    Tcl_AppendElement(interp, zBuf);
  }
  sqlite3BtreeLeave(pBt);

  /* Release the mutex on the SQLite handle that controls this b-tree */
  sqlite3_mutex_leave(pBt->db->mutex);
  return TCL_OK;
}

/*
** Usage:   btree_cursor ID TABLENUM WRITEABLE
**
** Create a new cursor.  Return the ID for the cursor.
*/
static int btree_cursor(
  void *NotUsed,
  Tcl_Interp *interp,    /* The TCL interpreter that invoked this command */
  int argc,              /* Number of arguments */
  const char **argv      /* Text of each argument */
){
  Btree *pBt;
  int iTable;
  BtCursor *pCur;
  int rc = SQLITE_OK;
  int wrFlag;
  char zBuf[30];

  if( argc!=4 ){
    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
       " ID TABLENUM WRITEABLE\"", 0);
    return TCL_ERROR;
  }
  pBt = sqlite3TestTextToPtr(argv[1]);
  if( Tcl_GetInt(interp, argv[2], &iTable) ) return TCL_ERROR;
  if( Tcl_GetBoolean(interp, argv[3], &wrFlag) ) return TCL_ERROR;
  pCur = (BtCursor *)ckalloc(sqlite3BtreeCursorSize());
  memset(pCur, 0, sqlite3BtreeCursorSize());
  sqlite3BtreeEnter(pBt);
#ifndef SQLITE_OMIT_SHARED_CACHE
  rc = sqlite3BtreeLockTable(pBt, iTable, wrFlag);
#endif
  if( rc==SQLITE_OK ){
    rc = sqlite3BtreeCursor(pBt, iTable, wrFlag, 0, pCur);
  }
  sqlite3BtreeLeave(pBt);
  if( rc ){
    ckfree((char *)pCur);
    Tcl_AppendResult(interp, errorName(rc), 0);
    return TCL_ERROR;
  }
  sqlite3_snprintf(sizeof(zBuf), zBuf,"%p", pCur);
  Tcl_AppendResult(interp, zBuf, 0);
  return SQLITE_OK;
}

/*
** Usage:   btree_close_cursor ID
**
** Close a cursor opened using btree_cursor.
*/
static int btree_close_cursor(
  void *NotUsed,
  Tcl_Interp *interp,    /* The TCL interpreter that invoked this command */
  int argc,              /* Number of arguments */
  const char **argv      /* Text of each argument */
){
  BtCursor *pCur;
  Btree *pBt;
  int rc;

  if( argc!=2 ){
    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
       " ID\"", 0);
    return TCL_ERROR;
  }
  pCur = sqlite3TestTextToPtr(argv[1]);
  pBt = pCur->pBtree;
  sqlite3BtreeEnter(pBt);
  rc = sqlite3BtreeCloseCursor(pCur);
  sqlite3BtreeLeave(pBt);
  ckfree((char *)pCur);
  if( rc ){
    Tcl_AppendResult(interp, errorName(rc), 0);
    return TCL_ERROR;
  }
  return SQLITE_OK;
}

/*
** Usage:   btree_next ID
**
** Move the cursor to the next entry in the table.  Return 0 on success
** or 1 if the cursor was already on the last entry in the table or if
** the table is empty.
*/
static int btree_next(
  void *NotUsed,
  Tcl_Interp *interp,    /* The TCL interpreter that invoked this command */
  int argc,              /* Number of arguments */
  const char **argv      /* Text of each argument */
){
  BtCursor *pCur;
  int rc;
  int res = 0;
  char zBuf[100];

  if( argc!=2 ){
    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
       " ID\"", 0);
    return TCL_ERROR;
  }
  pCur = sqlite3TestTextToPtr(argv[1]);
  sqlite3BtreeEnter(pCur->pBtree);
  rc = sqlite3BtreeNext(pCur, &res);
  sqlite3BtreeLeave(pCur->pBtree);
  if( rc ){
    Tcl_AppendResult(interp, errorName(rc), 0);
    return TCL_ERROR;
  }
  sqlite3_snprintf(sizeof(zBuf),zBuf,"%d",res);
  Tcl_AppendResult(interp, zBuf, 0);
  return SQLITE_OK;
}

/*
** Usage:   btree_first ID
**
** Move the cursor to the first entry in the table.  Return 0 if the
** cursor was left point to something and 1 if the table is empty.
*/
static int btree_first(
  void *NotUsed,
  Tcl_Interp *interp,    /* The TCL interpreter that invoked this command */
  int argc,              /* Number of arguments */
  const char **argv      /* Text of each argument */
){
  BtCursor *pCur;
  int rc;
  int res = 0;
  char zBuf[100];

  if( argc!=2 ){
    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
       " ID\"", 0);
    return TCL_ERROR;
  }
  pCur = sqlite3TestTextToPtr(argv[1]);
  sqlite3BtreeEnter(pCur->pBtree);
  rc = sqlite3BtreeFirst(pCur, &res);
  sqlite3BtreeLeave(pCur->pBtree);
  if( rc ){
    Tcl_AppendResult(interp, errorName(rc), 0);
    return TCL_ERROR;
  }
  sqlite3_snprintf(sizeof(zBuf),zBuf,"%d",res);
  Tcl_AppendResult(interp, zBuf, 0);
  return SQLITE_OK;
}

/*
** Usage:   btree_payload_size ID
**
** Return the number of bytes of payload
*/
static int btree_payload_size(
  void *NotUsed,
  Tcl_Interp *interp,    /* The TCL interpreter that invoked this command */
  int argc,              /* Number of arguments */
  const char **argv      /* Text of each argument */
){
  BtCursor *pCur;
  int n1;
  char zBuf[50];

  if( argc!=2 ){
    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
       " ID\"", 0);
    return TCL_ERROR;
  }
  pCur = sqlite3TestTextToPtr(argv[1]);
  sqlite3BtreeEnter(pCur->pBtree);

  /* The cursor may be in "require-seek" state. If this is the case, the
  ** call to BtreeDataSize() will fix it. */
  sqlite3BtreeDataSize(pCur, (u32*)&n1);
  sqlite3_snprintf(sizeof(zBuf),zBuf, "%d", (int)n1);
  Tcl_AppendResult(interp, zBuf, 0);
  return SQLITE_OK;
}

/*
** usage:   btree_from_db  DB-HANDLE
**
** This command returns the btree handle for the main database associated
** with the database-handle passed as the argument. Example usage:
**
** sqlite3 db test.db
** set bt [btree_from_db db]
*/
static int btree_from_db(
  void *NotUsed,
  Tcl_Interp *interp,    /* The TCL interpreter that invoked this command */
  int argc,              /* Number of arguments */
  const char **argv      /* Text of each argument */
){
  char zBuf[100];
  Tcl_CmdInfo info;
  sqlite3 *db;
  Btree *pBt;
  int iDb = 0;

  if( argc!=2 && argc!=3 ){
    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
       " DB-HANDLE ?N?\"", 0);
    return TCL_ERROR;
  }

  if( 1!=Tcl_GetCommandInfo(interp, argv[1], &info) ){
    Tcl_AppendResult(interp, "No such db-handle: \"", argv[1], "\"", 0);
    return TCL_ERROR;
  }
  if( argc==3 ){
    iDb = atoi(argv[2]);
  }

  db = *((sqlite3 **)info.objClientData);
  assert( db );

  pBt = db->aDb[iDb].pBt;
  sqlite3_snprintf(sizeof(zBuf), zBuf, "%p", pBt);
  Tcl_SetResult(interp, zBuf, TCL_VOLATILE);
  return TCL_OK;
}

/*
** Register commands with the TCL interpreter.
*/
int Sqlitetest3_Init(Tcl_Interp *interp){
  static struct {
     char *zName;
     Tcl_CmdProc *xProc;
  } aCmd[] = {
     { "btree_open",               (Tcl_CmdProc*)btree_open               },
     { "btree_close",              (Tcl_CmdProc*)btree_close              },
     { "btree_begin_transaction",  (Tcl_CmdProc*)btree_begin_transaction  },
     { "btree_pager_stats",        (Tcl_CmdProc*)btree_pager_stats        },
     { "btree_cursor",             (Tcl_CmdProc*)btree_cursor             },
     { "btree_close_cursor",       (Tcl_CmdProc*)btree_close_cursor       },
     { "btree_next",               (Tcl_CmdProc*)btree_next               },
     { "btree_eof",                (Tcl_CmdProc*)t3_tcl_function_stub     },
     { "btree_payload_size",       (Tcl_CmdProc*)btree_payload_size       },
     { "btree_first",              (Tcl_CmdProc*)btree_first              },
     { "btree_varint_test",        (Tcl_CmdProc*)t3_tcl_function_stub     },
     { "btree_from_db",            (Tcl_CmdProc*)btree_from_db            },
     { "btree_ismemdb",            (Tcl_CmdProc*)t3_tcl_function_stub     },
     { "btree_set_cache_size",     (Tcl_CmdProc*)t3_tcl_function_stub     }
  };
  int i;

  for(i=0; i<sizeof(aCmd)/sizeof(aCmd[0]); i++){
    Tcl_CreateCommand(interp, aCmd[i].zName, aCmd[i].xProc, 0, 0);
  }

  return TCL_OK;
}
