/*
** 2003 April 6
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
*************************************************************************
** This file contains code used to implement the ATTACH and DETACH commands.
*/
#include "sqliteInt.h"

#ifndef SQLITE_OMIT_ATTACH
/*
** Resolve an expression that was part of an ATTACH or DETACH statement. This
** is slightly different from resolving a normal SQL expression, because simple
** identifiers are treated as strings, not possible column names or aliases.
**
** i.e. if the parser sees:
**
**     ATTACH DATABASE abc AS def
**
** it treats the two expressions as literal strings 'abc' and 'def' instead of
** looking for columns of the same name.
**
** This only applies to the root node of pExpr, so the statement:
**
**     ATTACH DATABASE abc||def AS 'db2'
**
** will fail because neither abc or def can be resolved.
*/
static int resolveAttachExpr(NameContext *pName, Expr *pExpr)
{
  int rc = SQLITE_OK;
  if( pExpr ){
    if( pExpr->op!=TK_ID ){
      rc = sqlite3ResolveExprNames(pName, pExpr);
      if( rc==SQLITE_OK && !sqlite3ExprIsConstant(pExpr) ){
        sqlite3ErrorMsg(pName->pParse, "invalid name: \"%s\"", pExpr->u.zToken);
        return SQLITE_ERROR;
      }
    }else{
      pExpr->op = TK_STRING;
    }
  }
  return rc;
}

/*
** An SQL user-function registered to do the work of an ATTACH statement. The
** three arguments to the function come directly from an attach statement:
**
**     ATTACH DATABASE x AS y KEY z
**
**     SELECT sqlite_attach(x, y, z)
**
** If the optional "KEY z" syntax is omitted, an SQL NULL is passed as the
** third argument.
*/
static void attachFunc(
  sqlite3_context *context,
  int NotUsed,
  sqlite3_value **argv
){
  int i;
  int rc = 0;
  sqlite3 *db = sqlite3_context_db_handle(context);
  const char *zName;
  const char *zFile;
  char *zPath = 0;
  char *zErr = 0;
  unsigned int flags;
  Db *aNew;
  char *zErrDyn = 0;
  sqlite3_vfs *pVfs;

  UNUSED_PARAMETER(NotUsed);

  zFile = (const char *)sqlite3_value_text(argv[0]);
  zName = (const char *)sqlite3_value_text(argv[1]);
  if( zFile==0 ) zFile = "";
  if( zName==0 ) zName = "";

  /* Check for the following errors:
  **
  **     * Too many attached databases,
  **     * Transaction currently open
  **     * Specified database name already being used.
  */
  if( db->nDb>=db->aLimit[SQLITE_LIMIT_ATTACHED]+2 ){
    zErrDyn = sqlite3MPrintf(db, "too many attached databases - max %d", 
      db->aLimit[SQLITE_LIMIT_ATTACHED]
    );
    goto attach_error;
  }
  if( !db->autoCommit ){
    zErrDyn = sqlite3MPrintf(db, "cannot ATTACH database within transaction");
    goto attach_error;
  }
  for(i=0; i<db->nDb; i++){
    char *z = db->aDb[i].zName;
    assert( z && zName );
    if( sqlite3StrICmp(z, zName)==0 ){
      zErrDyn = sqlite3MPrintf(db, "database %s is already in use", zName);
      goto attach_error;
    }
  }

  /* Allocate the new entry in the db->aDb[] array and initialize the schema
  ** hash tables.
  */
  if( db->aDb==db->aDbStatic ){
    aNew = sqlite3DbMallocRaw(db, sizeof(db->aDb[0])*3 );
    if( aNew==0 ) return;
    memcpy(aNew, db->aDb, sizeof(db->aDb[0])*2);
  }else{
    aNew = sqlite3DbRealloc(db, db->aDb, sizeof(db->aDb[0])*(db->nDb+1) );
    if( aNew==0 ) return;
  }
  db->aDb = aNew;
  aNew = &db->aDb[db->nDb];
  memset(aNew, 0, sizeof(*aNew));

  /* Open the database file. If the btree is successfully opened, use
  ** it to obtain the database schema. At this point the schema may
  ** or may not be initialized.
  */
  flags = db->openFlags;
  rc = sqlite3ParseUri(db->pVfs->zName, zFile, &flags, &pVfs, &zPath, &zErr);
  if( rc!=SQLITE_OK ){
    if( rc==SQLITE_NOMEM ) db->mallocFailed = 1;
    sqlite3_result_error(context, zErr, -1);
    sqlite3_free(zErr);
    return;
  }
  assert( pVfs );
  flags |= SQLITE_OPEN_MAIN_DB;
  rc = sqlite3BtreeOpen(pVfs, zPath, db, &aNew->pBt, 0, flags);
  sqlite3_free( zPath );
  db->nDb++;
  if( rc==SQLITE_CONSTRAINT ){
    rc = SQLITE_ERROR;
    zErrDyn = sqlite3MPrintf(db, "database is already attached");
  }else if( rc==SQLITE_OK ){
    Pager *pPager;
    aNew->pSchema = sqlite3SchemaGet(db, aNew->pBt);
    if( !aNew->pSchema ){
      rc = SQLITE_NOMEM;
    }else if( aNew->pSchema->file_format && aNew->pSchema->enc!=ENC(db) ){
      zErrDyn = sqlite3MPrintf(db, 
        "attached databases must use the same text encoding as main database");
      rc = SQLITE_ERROR;
    }
    pPager = sqlite3BtreePager(aNew->pBt);
    sqlite3PagerLockingMode(pPager, db->dfltLockMode);
    sqlite3BtreeSecureDelete(aNew->pBt,
                             sqlite3BtreeSecureDelete(db->aDb[0].pBt,-1) );
  }
  aNew->safety_level = 3;
  aNew->zName = sqlite3DbStrDup(db, zName);
  if( rc==SQLITE_OK && aNew->zName==0 ){
    rc = SQLITE_NOMEM;
  }


#ifdef SQLITE_HAS_CODEC
  if( rc==SQLITE_OK ){
    extern int sqlite3CodecAttach(sqlite3*, int, const void*, int);
    extern void sqlite3CodecGetKey(sqlite3*, int, void**, int*);
    int nKey;
    char *zKey;
    int t = sqlite3_value_type(argv[2]);
    switch( t ){
      case SQLITE_INTEGER:
      case SQLITE_FLOAT:
        zErrDyn = sqlite3DbStrDup(db, "Invalid key value");
        rc = SQLITE_ERROR;
        break;
        
      case SQLITE_TEXT:
      case SQLITE_BLOB:
        nKey = sqlite3_value_bytes(argv[2]);
        zKey = (char *)sqlite3_value_blob(argv[2]);
        rc = sqlite3CodecAttach(db, db->nDb-1, zKey, nKey);
        break;

      case SQLITE_NULL:
        /* No key specified.  Use the key from the main database */
        sqlite3CodecGetKey(db, 0, (void**)&zKey, &nKey);
        if( nKey>0 || sqlite3BtreeGetReserve(db->aDb[0].pBt)>0 ){
          rc = sqlite3CodecAttach(db, db->nDb-1, zKey, nKey);
        }
        break;
    }
  }
#endif

  /* If the file was opened successfully, read the schema for the new database.
  ** If this fails, or if opening the file failed, then close the file and 
  ** remove the entry from the db->aDb[] array. i.e. put everything back the way
  ** we found it.
  */
  if( rc==SQLITE_OK ){
    sqlite3BtreeEnterAll(db);
    rc = sqlite3Init(db, &zErrDyn);
    sqlite3BtreeLeaveAll(db);
  }
  if( rc ){
    int iDb = db->nDb - 1;
    assert( iDb>=2 );
    if( db->aDb[iDb].pBt ){
      sqlite3BtreeClose(db->aDb[iDb].pBt);
      db->aDb[iDb].pBt = 0;
      db->aDb[iDb].pSchema = 0;
    }
    sqlite3ResetAllSchemasOfConnection(db);
    db->nDb = iDb;
    if( rc==SQLITE_NOMEM || rc==SQLITE_IOERR_NOMEM ){
      db->mallocFailed = 1;
      sqlite3DbFree(db, zErrDyn);
      zErrDyn = sqlite3MPrintf(db, "out of memory");
    }else if( zErrDyn==0 ){
      zErrDyn = sqlite3MPrintf(db, "unable to open database: %s", zFile);
    }
    goto attach_error;
  }
  
  return;

attach_error:
  /* Return an error if we get here */
  if( zErrDyn ){
    sqlite3_result_error(context, zErrDyn, -1);
    sqlite3DbFree(db, zErrDyn);
  }
  if( rc ) sqlite3_result_error_code(context, rc);
}

/*
** An SQL user-function registered to do the work of an DETACH statement. The
** three arguments to the function come directly from a detach statement:
**
**     DETACH DATABASE x
**
**     SELECT sqlite_detach(x)
*/
static void detachFunc(
  sqlite3_context *context,
  int NotUsed,
  sqlite3_value **argv
){
  const char *zName = (const char *)sqlite3_value_text(argv[0]);
  sqlite3 *db = sqlite3_context_db_handle(context);
  int i;
  Db *pDb = 0;
  char zErr[128];

  UNUSED_PARAMETER(NotUsed);

  if( zName==0 ) zName = "";
  for(i=0; i<db->nDb; i++){
    pDb = &db->aDb[i];
    if( pDb->pBt==0 ) continue;
    if( sqlite3StrICmp(pDb->zName, zName)==0 ) break;
  }

  if( i>=db->nDb ){
    sqlite3_snprintf(sizeof(zErr),zErr, "no such database: %s", zName);
    goto detach_error;
  }
  if( i<2 ){
    sqlite3_snprintf(sizeof(zErr),zErr, "cannot detach database %s", zName);
    goto detach_error;
  }
  if( !db->autoCommit ){
    sqlite3_snprintf(sizeof(zErr), zErr,
                     "cannot DETACH database within transaction");
    goto detach_error;
  }
  if( sqlite3BtreeIsInReadTrans(pDb->pBt) || sqlite3BtreeIsInBackup(pDb->pBt) ){
    sqlite3_snprintf(sizeof(zErr),zErr, "database %s is locked", zName);
    goto detach_error;
  }

  sqlite3BtreeClose(pDb->pBt);
  pDb->pBt = 0;
  pDb->pSchema = 0;
  sqlite3ResetAllSchemasOfConnection(db);
  return;

detach_error:
  sqlite3_result_error(context, zErr, -1);
}

/*
** This procedure generates VDBE code for a single invocation of either the
** sqlite_detach() or sqlite_attach() SQL user functions.
*/
static void codeAttach(
  Parse *pParse,       /* The parser context */
  int type,            /* Either SQLITE_ATTACH or SQLITE_DETACH */
  FuncDef const *pFunc,/* FuncDef wrapper for detachFunc() or attachFunc() */
  Expr *pAuthArg,      /* Expression to pass to authorization callback */
  Expr *pFilename,     /* Name of database file */
  Expr *pDbname,       /* Name of the database to use internally */
  Expr *pKey           /* Database key for encryption extension */
){
  int rc;
  NameContext sName;
  Vdbe *v;
  sqlite3* db = pParse->db;
  int regArgs;

  memset(&sName, 0, sizeof(NameContext));
  sName.pParse = pParse;

  if( 
      SQLITE_OK!=(rc = resolveAttachExpr(&sName, pFilename)) ||
      SQLITE_OK!=(rc = resolveAttachExpr(&sName, pDbname)) ||
      SQLITE_OK!=(rc = resolveAttachExpr(&sName, pKey))
  ){
    pParse->nErr++;
    goto attach_end;
  }

#ifndef SQLITE_OMIT_AUTHORIZATION
  if( pAuthArg ){
    char *zAuthArg;
    if( pAuthArg->op==TK_STRING ){
      zAuthArg = pAuthArg->u.zToken;
    }else{
      zAuthArg = 0;
    }
    rc = sqlite3AuthCheck(pParse, type, zAuthArg, 0, 0);
    if(rc!=SQLITE_OK ){
      goto attach_end;
    }
  }
#endif /* SQLITE_OMIT_AUTHORIZATION */


  v = sqlite3GetVdbe(pParse);
  regArgs = sqlite3GetTempRange(pParse, 4);
  sqlite3ExprCode(pParse, pFilename, regArgs);
  sqlite3ExprCode(pParse, pDbname, regArgs+1);
  sqlite3ExprCode(pParse, pKey, regArgs+2);

  assert( v || db->mallocFailed );
  if( v ){
    sqlite3VdbeAddOp3(v, OP_Function, 0, regArgs+3-pFunc->nArg, regArgs+3);
    assert( pFunc->nArg==-1 || (pFunc->nArg&0xff)==pFunc->nArg );
    sqlite3VdbeChangeP5(v, (u8)(pFunc->nArg));
    sqlite3VdbeChangeP4(v, -1, (char *)pFunc, P4_FUNCDEF);

    /* Code an OP_Expire. For an ATTACH statement, set P1 to true (expire this
    ** statement only). For DETACH, set it to false (expire all existing
    ** statements).
    */
    sqlite3VdbeAddOp1(v, OP_Expire, (type==SQLITE_ATTACH));
  }
  
attach_end:
  sqlite3ExprDelete(db, pFilename);
  sqlite3ExprDelete(db, pDbname);
  sqlite3ExprDelete(db, pKey);
}

/*
** Called by the parser to compile a DETACH statement.
**
**     DETACH pDbname
*/
void sqlite3Detach(Parse *pParse, Expr *pDbname){
  static const FuncDef detach_func = {
    1,                /* nArg */
    SQLITE_UTF8,      /* iPrefEnc */
    0,                /* flags */
    0,                /* pUserData */
    0,                /* pNext */
    detachFunc,       /* xFunc */
    0,                /* xStep */
    0,                /* xFinalize */
    "sqlite_detach",  /* zName */
    0,                /* pHash */
    0                 /* pDestructor */
  };
  codeAttach(pParse, SQLITE_DETACH, &detach_func, pDbname, 0, 0, pDbname);
}

/*
** Called by the parser to compile an ATTACH statement.
**
**     ATTACH p AS pDbname KEY pKey
*/
void sqlite3Attach(Parse *pParse, Expr *p, Expr *pDbname, Expr *pKey){
  static const FuncDef attach_func = {
    3,                /* nArg */
    SQLITE_UTF8,      /* iPrefEnc */
    0,                /* flags */
    0,                /* pUserData */
    0,                /* pNext */
    attachFunc,       /* xFunc */
    0,                /* xStep */
    0,                /* xFinalize */
    "sqlite_attach",  /* zName */
    0,                /* pHash */
    0                 /* pDestructor */
  };
  codeAttach(pParse, SQLITE_ATTACH, &attach_func, p, p, pDbname, pKey);
}
#endif /* SQLITE_OMIT_ATTACH */

/*
** Initialize a DbFixer structure.  This routine must be called prior
** to passing the structure to one of the sqliteFixAAAA() routines below.
**
** The return value indicates whether or not fixation is required.  TRUE
** means we do need to fix the database references, FALSE means we do not.
*/
int sqlite3FixInit(
  DbFixer *pFix,      /* The fixer to be initialized */
  Parse *pParse,      /* Error messages will be written here */
  int iDb,            /* This is the database that must be used */
  const char *zType,  /* "view", "trigger", or "index" */
  const Token *pName  /* Name of the view, trigger, or index */
){
  sqlite3 *db;

  if( NEVER(iDb<0) || iDb==1 ) return 0;
  db = pParse->db;
  assert( db->nDb>iDb );
  pFix->pParse = pParse;
  pFix->zDb = db->aDb[iDb].zName;
  pFix->pSchema = db->aDb[iDb].pSchema;
  pFix->zType = zType;
  pFix->pName = pName;
  return 1;
}

/*
** The following set of routines walk through the parse tree and assign
** a specific database to all table references where the database name
** was left unspecified in the original SQL statement.  The pFix structure
** must have been initialized by a prior call to sqlite3FixInit().
**
** These routines are used to make sure that an index, trigger, or
** view in one database does not refer to objects in a different database.
** (Exception: indices, triggers, and views in the TEMP database are
** allowed to refer to anything.)  If a reference is explicitly made
** to an object in a different database, an error message is added to
** pParse->zErrMsg and these routines return non-zero.  If everything
** checks out, these routines return 0.
*/
int sqlite3FixSrcList(
  DbFixer *pFix,       /* Context of the fixation */
  SrcList *pList       /* The Source list to check and modify */
){
  int i;
  const char *zDb;
  struct SrcList_item *pItem;

  if( NEVER(pList==0) ) return 0;
  zDb = pFix->zDb;
  for(i=0, pItem=pList->a; i<pList->nSrc; i++, pItem++){
    if( pItem->zDatabase && sqlite3StrICmp(pItem->zDatabase, zDb) ){
      sqlite3ErrorMsg(pFix->pParse,
         "%s %T cannot reference objects in database %s",
         pFix->zType, pFix->pName, pItem->zDatabase);
      return 1;
    }
    sqlite3DbFree(pFix->pParse->db, pItem->zDatabase);
    pItem->zDatabase = 0;
    pItem->pSchema = pFix->pSchema;
#if !defined(SQLITE_OMIT_VIEW) || !defined(SQLITE_OMIT_TRIGGER)
    if( sqlite3FixSelect(pFix, pItem->pSelect) ) return 1;
    if( sqlite3FixExpr(pFix, pItem->pOn) ) return 1;
#endif
  }
  return 0;
}
#if !defined(SQLITE_OMIT_VIEW) || !defined(SQLITE_OMIT_TRIGGER)
int sqlite3FixSelect(
  DbFixer *pFix,       /* Context of the fixation */
  Select *pSelect      /* The SELECT statement to be fixed to one database */
){
  while( pSelect ){
    if( sqlite3FixExprList(pFix, pSelect->pEList) ){
      return 1;
    }
    if( sqlite3FixSrcList(pFix, pSelect->pSrc) ){
      return 1;
    }
    if( sqlite3FixExpr(pFix, pSelect->pWhere) ){
      return 1;
    }
    if( sqlite3FixExpr(pFix, pSelect->pHaving) ){
      return 1;
    }
    pSelect = pSelect->pPrior;
  }
  return 0;
}
int sqlite3FixExpr(
  DbFixer *pFix,     /* Context of the fixation */
  Expr *pExpr        /* The expression to be fixed to one database */
){
  while( pExpr ){
    if( ExprHasAnyProperty(pExpr, EP_TokenOnly) ) break;
    if( ExprHasProperty(pExpr, EP_xIsSelect) ){
      if( sqlite3FixSelect(pFix, pExpr->x.pSelect) ) return 1;
    }else{
      if( sqlite3FixExprList(pFix, pExpr->x.pList) ) return 1;
    }
    if( sqlite3FixExpr(pFix, pExpr->pRight) ){
      return 1;
    }
    pExpr = pExpr->pLeft;
  }
  return 0;
}
int sqlite3FixExprList(
  DbFixer *pFix,     /* Context of the fixation */
  ExprList *pList    /* The expression to be fixed to one database */
){
  int i;
  struct ExprList_item *pItem;
  if( pList==0 ) return 0;
  for(i=0, pItem=pList->a; i<pList->nExpr; i++, pItem++){
    if( sqlite3FixExpr(pFix, pItem->pExpr) ){
      return 1;
    }
  }
  return 0;
}
#endif

#ifndef SQLITE_OMIT_TRIGGER
int sqlite3FixTriggerStep(
  DbFixer *pFix,     /* Context of the fixation */
  TriggerStep *pStep /* The trigger step be fixed to one database */
){
  while( pStep ){
    if( sqlite3FixSelect(pFix, pStep->pSelect) ){
      return 1;
    }
    if( sqlite3FixExpr(pFix, pStep->pWhere) ){
      return 1;
    }
    if( sqlite3FixExprList(pFix, pStep->pExprList) ){
      return 1;
    }
    pStep = pStep->pNext;
  }
  return 0;
}
#endif
