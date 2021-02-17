/*
** 2003 September 6
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
*************************************************************************
** This is the header file for information that is private to the
** VDBE.  This information used to all be at the top of the single
** source code file "vdbe.c".  When that file became too big (over
** 6000 lines long) it was split up into several smaller files and
** this header information was factored out.
*/
#ifndef _VDBEINT_H_
#define _VDBEINT_H_

/*
** SQL is translated into a sequence of instructions to be
** executed by a virtual machine.  Each instruction is an instance
** of the following structure.
*/
typedef struct VdbeOp Op;

/*
** Boolean values
*/
typedef unsigned char Bool;

/* Opaque type used by code in vdbesort.c */
typedef struct VdbeSorter VdbeSorter;

/* Opaque type used by the explainer */
typedef struct Explain Explain;

/*
** A cursor is a pointer into a single BTree within a database file.
** The cursor can seek to a BTree entry with a particular key, or
** loop over all entries of the Btree.  You can also insert new BTree
** entries or retrieve the key or data from the entry that the cursor
** is currently pointing to.
** 
** Every cursor that the virtual machine has open is represented by an
** instance of the following structure.
*/
struct VdbeCursor {
  BtCursor *pCursor;    /* The cursor structure of the backend */
  Btree *pBt;           /* Separate file holding temporary table */
  KeyInfo *pKeyInfo;    /* Info about index keys needed by index cursors */
  int iDb;              /* Index of cursor database in db->aDb[] (or -1) */
  int pseudoTableReg;   /* Register holding pseudotable content. */
  int nField;           /* Number of fields in the header */
  Bool zeroed;          /* True if zeroed out and ready for reuse */
  Bool rowidIsValid;    /* True if lastRowid is valid */
  Bool atFirst;         /* True if pointing to first entry */
  Bool useRandomRowid;  /* Generate new record numbers semi-randomly */
  Bool nullRow;         /* True if pointing to a row with no data */
  Bool deferredMoveto;  /* A call to sqlite3BtreeMoveto() is needed */
  Bool isTable;         /* True if a table requiring integer keys */
  Bool isIndex;         /* True if an index containing keys only - no data */
  Bool isOrdered;       /* True if the underlying table is BTREE_UNORDERED */
  Bool isSorter;        /* True if a new-style sorter */
  Bool multiPseudo;     /* Multi-register pseudo-cursor */
  sqlite3_vtab_cursor *pVtabCursor;  /* The cursor for a virtual table */
  const sqlite3_module *pModule;     /* Module for cursor pVtabCursor */
  i64 seqCount;         /* Sequence counter */
  i64 movetoTarget;     /* Argument to the deferred sqlite3BtreeMoveto() */
  i64 lastRowid;        /* Last rowid from a Next or NextIdx operation */
  VdbeSorter *pSorter;  /* Sorter object for OP_SorterOpen cursors */

  /* Result of last sqlite3BtreeMoveto() done by an OP_NotExists or 
  ** OP_IsUnique opcode on this cursor. */
  int seekResult;

  /* Cached information about the header for the data record that the
  ** cursor is currently pointing to.  Only valid if cacheStatus matches
  ** Vdbe.cacheCtr.  Vdbe.cacheCtr will never take on the value of
  ** CACHE_STALE and so setting cacheStatus=CACHE_STALE guarantees that
  ** the cache is out of date.
  **
  ** aRow might point to (ephemeral) data for the current row, or it might
  ** be NULL.
  */
  u32 cacheStatus;      /* Cache is valid if this matches Vdbe.cacheCtr */
  int payloadSize;      /* Total number of bytes in the record */
  u32 *aType;           /* Type values for all entries in the record */
  u32 *aOffset;         /* Cached offsets to the start of each columns data */
  u8 *aRow;             /* Data for the current row, if all on one page */
};
typedef struct VdbeCursor VdbeCursor;

/*
** When a sub-program is executed (OP_Program), a structure of this type
** is allocated to store the current value of the program counter, as
** well as the current memory cell array and various other frame specific
** values stored in the Vdbe struct. When the sub-program is finished, 
** these values are copied back to the Vdbe from the VdbeFrame structure,
** restoring the state of the VM to as it was before the sub-program
** began executing.
**
** The memory for a VdbeFrame object is allocated and managed by a memory
** cell in the parent (calling) frame. When the memory cell is deleted or
** overwritten, the VdbeFrame object is not freed immediately. Instead, it
** is linked into the Vdbe.pDelFrame list. The contents of the Vdbe.pDelFrame
** list is deleted when the VM is reset in VdbeHalt(). The reason for doing
** this instead of deleting the VdbeFrame immediately is to avoid recursive
** calls to sqlite3VdbeMemRelease() when the memory cells belonging to the
** child frame are released.
**
** The currently executing frame is stored in Vdbe.pFrame. Vdbe.pFrame is
** set to NULL if the currently executing frame is the main program.
*/
typedef struct VdbeFrame VdbeFrame;
struct VdbeFrame {
  Vdbe *v;                /* VM this frame belongs to */
  VdbeFrame *pParent;     /* Parent of this frame, or NULL if parent is main */
  Op *aOp;                /* Program instructions for parent frame */
  Mem *aMem;              /* Array of memory cells for parent frame */
  u8 *aOnceFlag;          /* Array of OP_Once flags for parent frame */
  VdbeCursor **apCsr;     /* Array of Vdbe cursors for parent frame */
  void *token;            /* Copy of SubProgram.token */
  i64 lastRowid;          /* Last insert rowid (sqlite3.lastRowid) */
  int nCursor;            /* Number of entries in apCsr */
  int pc;                 /* Program Counter in parent (calling) frame */
  int nOp;                /* Size of aOp array */
  int nMem;               /* Number of entries in aMem */
  int nOnceFlag;          /* Number of entries in aOnceFlag */
  int nChildMem;          /* Number of memory cells for child frame */
  int nChildCsr;          /* Number of cursors for child frame */
  int nChange;            /* Statement changes (Vdbe.nChanges)     */
};

#define VdbeFrameMem(p) ((Mem *)&((u8 *)p)[ROUND8(sizeof(VdbeFrame))])

/*
** A value for VdbeCursor.cacheValid that means the cache is always invalid.
*/
#define CACHE_STALE 0

/*
** Internally, the vdbe manipulates nearly all SQL values as Mem
** structures. Each Mem struct may cache multiple representations (string,
** integer etc.) of the same value.
*/
struct Mem {
  sqlite3 *db;        /* The associated database connection */
  char *z;            /* String or BLOB value */
  double r;           /* Real value */
  union {
    i64 i;              /* Integer value used when MEM_Int is set in flags */
    int nZero;          /* Used when bit MEM_Zero is set in flags */
    FuncDef *pDef;      /* Used only when flags==MEM_Agg */
    RowSet *pRowSet;    /* Used only when flags==MEM_RowSet */
    VdbeFrame *pFrame;  /* Used when flags==MEM_Frame */
  } u;
  int n;              /* Number of characters in string value, excluding '\0' */
  u16 flags;          /* Some combination of MEM_Null, MEM_Str, MEM_Dyn, etc. */
  u8  type;           /* One of SQLITE_NULL, SQLITE_TEXT, SQLITE_INTEGER, etc */
  u8  enc;            /* SQLITE_UTF8, SQLITE_UTF16BE, SQLITE_UTF16LE */
#ifdef SQLITE_DEBUG
  Mem *pScopyFrom;    /* This Mem is a shallow copy of pScopyFrom */
  void *pFiller;      /* So that sizeof(Mem) is a multiple of 8 */
#endif
  void (*xDel)(void *);  /* If not null, call this function to delete Mem.z */
  char *zMalloc;      /* Dynamic buffer allocated by sqlite3_malloc() */
};

/* One or more of the following flags are set to indicate the validOK
** representations of the value stored in the Mem struct.
**
** If the MEM_Null flag is set, then the value is an SQL NULL value.
** No other flags may be set in this case.
**
** If the MEM_Str flag is set then Mem.z points at a string representation.
** Usually this is encoded in the same unicode encoding as the main
** database (see below for exceptions). If the MEM_Term flag is also
** set, then the string is nul terminated. The MEM_Int and MEM_Real 
** flags may coexist with the MEM_Str flag.
*/
#define MEM_Null      0x0001   /* Value is NULL */
#define MEM_Str       0x0002   /* Value is a string */
#define MEM_Int       0x0004   /* Value is an integer */
#define MEM_Real      0x0008   /* Value is a real number */
#define MEM_Blob      0x0010   /* Value is a BLOB */
#define MEM_RowSet    0x0020   /* Value is a RowSet object */
#define MEM_Frame     0x0040   /* Value is a VdbeFrame object */
#define MEM_Invalid   0x0080   /* Value is undefined */
#define MEM_Cleared   0x0100   /* NULL set by OP_Null, not from data */
#define MEM_TypeMask  0x01ff   /* Mask of type bits */


/* Whenever Mem contains a valid string or blob representation, one of
** the following flags must be set to determine the memory management
** policy for Mem.z.  The MEM_Term flag tells us whether or not the
** string is \000 or \u0000 terminated
*/
#define MEM_Term      0x0200   /* String rep is nul terminated */
#define MEM_Dyn       0x0400   /* Need to call sqliteFree() on Mem.z */
#define MEM_Static    0x0800   /* Mem.z points to a static string */
#define MEM_Ephem     0x1000   /* Mem.z points to an ephemeral string */
#define MEM_Agg       0x2000   /* Mem.z points to an agg function context */
#define MEM_Zero      0x4000   /* Mem.i contains count of 0s appended to blob */
#ifdef SQLITE_OMIT_INCRBLOB
  #undef MEM_Zero
  #define MEM_Zero 0x0000
#endif

/*
** Clear any existing type flags from a Mem and replace them with f
*/
#define MemSetTypeFlag(p, f) \
   ((p)->flags = ((p)->flags&~(MEM_TypeMask|MEM_Zero))|f)

/*
** Return true if a memory cell is not marked as invalid.  This macro
** is for use inside assert() statements only.
*/
#ifdef SQLITE_DEBUG
#define memIsValid(M)  ((M)->flags & MEM_Invalid)==0
#endif


/* A VdbeFunc is just a FuncDef (defined in sqliteInt.h) that contains
** additional information about auxiliary information bound to arguments
** of the function.  This is used to implement the sqlite3_get_auxdata()
** and sqlite3_set_auxdata() APIs.  The "auxdata" is some auxiliary data
** that can be associated with a constant argument to a function.  This
** allows functions such as "regexp" to compile their constant regular
** expression argument once and reused the compiled code for multiple
** invocations.
*/
struct VdbeFunc {
  FuncDef *pFunc;               /* The definition of the function */
  int nAux;                     /* Number of entries allocated for apAux[] */
  struct AuxData {
    void *pAux;                   /* Aux data for the i-th argument */
    void (*xDelete)(void *);      /* Destructor for the aux data */
  } apAux[1];                   /* One slot for each function argument */
};

/*
** The "context" argument for a installable function.  A pointer to an
** instance of this structure is the first argument to the routines used
** implement the SQL functions.
**
** There is a typedef for this structure in sqlite.h.  So all routines,
** even the public interface to SQLite, can use a pointer to this structure.
** But this file is the only place where the internal details of this
** structure are known.
**
** This structure is defined inside of vdbeInt.h because it uses substructures
** (Mem) which are only defined there.
*/
struct sqlite3_context {
  FuncDef *pFunc;       /* Pointer to function information.  MUST BE FIRST */
  VdbeFunc *pVdbeFunc;  /* Auxilary data, if created. */
  Mem s;                /* The return value is stored here */
  Mem *pMem;            /* Memory cell used to store aggregate context */
  CollSeq *pColl;       /* Collating sequence */
  int isError;          /* Error code returned by the function. */
  int skipFlag;         /* Skip skip accumulator loading if true */
};

/*
** An Explain object accumulates indented output which is helpful
** in describing recursive data structures.
*/
struct Explain {
  Vdbe *pVdbe;       /* Attach the explanation to this Vdbe */
  StrAccum str;      /* The string being accumulated */
  int nIndent;       /* Number of elements in aIndent */
  u16 aIndent[100];  /* Levels of indentation */
  char zBase[100];   /* Initial space */
};

/* A bitfield type for use inside of structures.  Always follow with :N where
** N is the number of bits.
*/
typedef unsigned bft;  /* Bit Field Type */

/*
** An instance of the virtual machine.  This structure contains the complete
** state of the virtual machine.
**
** The "sqlite3_stmt" structure pointer that is returned by sqlite3_prepare()
** is really a pointer to an instance of this structure.
**
** The Vdbe.inVtabMethod variable is set to non-zero for the duration of
** any virtual table method invocations made by the vdbe program. It is
** set to 2 for xDestroy method calls and 1 for all other methods. This
** variable is used for two purposes: to allow xDestroy methods to execute
** "DROP TABLE" statements and to prevent some nasty side effects of
** malloc failure when SQLite is invoked recursively by a virtual table 
** method function.
*/
struct Vdbe {
  sqlite3 *db;            /* The database connection that owns this statement */
  Op *aOp;                /* Space to hold the virtual machine's program */
  Mem *aMem;              /* The memory locations */
  Mem **apArg;            /* Arguments to currently executing user function */
  Mem *aColName;          /* Column names to return */
  Mem *pResultSet;        /* Pointer to an array of results */
  int nMem;               /* Number of memory locations currently allocated */
  int nOp;                /* Number of instructions in the program */
  int nOpAlloc;           /* Number of slots allocated for aOp[] */
  int nLabel;             /* Number of labels used */
  int *aLabel;            /* Space to hold the labels */
  u16 nResColumn;         /* Number of columns in one row of the result set */
  int nCursor;            /* Number of slots in apCsr[] */
  u32 magic;              /* Magic number for sanity checking */
  char *zErrMsg;          /* Error message written here */
  Vdbe *pPrev,*pNext;     /* Linked list of VDBEs with the same Vdbe.db */
  VdbeCursor **apCsr;     /* One element of this array for each open cursor */
  Mem *aVar;              /* Values for the OP_Variable opcode. */
  char **azVar;           /* Name of variables */
  ynVar nVar;             /* Number of entries in aVar[] */
  ynVar nzVar;            /* Number of entries in azVar[] */
  u32 cacheCtr;           /* VdbeCursor row cache generation counter */
  int pc;                 /* The program counter */
  int rc;                 /* Value to return */
  u8 errorAction;         /* Recovery action to do in case of an error */
  u8 minWriteFileFormat;  /* Minimum file format for writable database files */
  bft explain:2;          /* True if EXPLAIN present on SQL command */
  bft inVtabMethod:2;     /* See comments above */
  bft changeCntOn:1;      /* True to update the change-counter */
  bft expired:1;          /* True if the VM needs to be recompiled */
  bft runOnlyOnce:1;      /* Automatically expire on reset */
  bft usesStmtJournal:1;  /* True if uses a statement journal */
  bft readOnly:1;         /* True for read-only statements */
  bft isPrepareV2:1;      /* True if prepared with prepare_v2() */
  bft doingRerun:1;       /* True if rerunning after an auto-reprepare */
  int nChange;            /* Number of db changes made since last reset */
  yDbMask btreeMask;      /* Bitmask of db->aDb[] entries referenced */
  yDbMask lockMask;       /* Subset of btreeMask that requires a lock */
  int iStatement;         /* Statement number (or 0 if has not opened stmt) */
  int aCounter[3];        /* Counters used by sqlite3_stmt_status() */
#ifndef SQLITE_OMIT_TRACE
  i64 startTime;          /* Time when query started - used for profiling */
#endif
  i64 nFkConstraint;      /* Number of imm. FK constraints this VM */
  i64 nStmtDefCons;       /* Number of def. constraints when stmt started */
  char *zSql;             /* Text of the SQL statement that generated this */
  void *pFree;            /* Free this when deleting the vdbe */
#ifdef SQLITE_DEBUG
  FILE *trace;            /* Write an execution trace here, if not NULL */
#endif
#ifdef SQLITE_ENABLE_TREE_EXPLAIN
  Explain *pExplain;      /* The explainer */
  char *zExplain;         /* Explanation of data structures */
#endif
  VdbeFrame *pFrame;      /* Parent frame */
  VdbeFrame *pDelFrame;   /* List of frame objects to free on VM reset */
  int nFrame;             /* Number of frames in pFrame list */
  u32 expmask;            /* Binding to these vars invalidates VM */
  SubProgram *pProgram;   /* Linked list of all sub-programs used by VM */
  int nOnceFlag;          /* Size of array aOnceFlag[] */
  u8 *aOnceFlag;          /* Flags for OP_Once */
};

/*
** The following are allowed values for Vdbe.magic
*/
#define VDBE_MAGIC_INIT     0x26bceaa5    /* Building a VDBE program */
#define VDBE_MAGIC_RUN      0xbdf20da3    /* VDBE is ready to execute */
#define VDBE_MAGIC_HALT     0x519c2973    /* VDBE has completed execution */
#define VDBE_MAGIC_DEAD     0xb606c3c8    /* The VDBE has been deallocated */

/*
** Function prototypes
*/
void sqlite3VdbeFreeCursor(Vdbe *, VdbeCursor*);
void sqliteVdbePopStack(Vdbe*,int);
int sqlite3VdbeCursorMoveto(VdbeCursor*);
#if defined(SQLITE_DEBUG) || defined(VDBE_PROFILE)
void sqlite3VdbePrintOp(FILE*, int, Op*);
#endif
u32 sqlite3VdbeSerialTypeLen(u32);
u32 sqlite3VdbeSerialType(Mem*, int);
u32 sqlite3VdbeSerialPut(unsigned char*, int, Mem*, int);
u32 sqlite3VdbeSerialGet(const unsigned char*, u32, Mem*);
void sqlite3VdbeDeleteAuxData(VdbeFunc*, int);

int sqlite2BtreeKeyCompare(BtCursor *, const void *, int, int, int *);
int sqlite3VdbeIdxKeyCompare(VdbeCursor*,UnpackedRecord*,int*);
int sqlite3VdbeIdxRowid(sqlite3*, BtCursor *, i64 *);
int sqlite3MemCompare(const Mem*, const Mem*, const CollSeq*);
int sqlite3VdbeExec(Vdbe*);
int sqlite3VdbeList(Vdbe*);
int sqlite3VdbeHalt(Vdbe*);
int sqlite3VdbeChangeEncoding(Mem *, int);
int sqlite3VdbeMemTooBig(Mem*);
int sqlite3VdbeMemCopy(Mem*, const Mem*);
void sqlite3VdbeMemShallowCopy(Mem*, const Mem*, int);
void sqlite3VdbeMemMove(Mem*, Mem*);
int sqlite3VdbeMemNulTerminate(Mem*);
int sqlite3VdbeMemSetStr(Mem*, const char*, int, u8, void(*)(void*));
void sqlite3VdbeMemSetInt64(Mem*, i64);
#ifdef SQLITE_OMIT_FLOATING_POINT
# define sqlite3VdbeMemSetDouble sqlite3VdbeMemSetInt64
#else
  void sqlite3VdbeMemSetDouble(Mem*, double);
#endif
void sqlite3VdbeMemSetNull(Mem*);
void sqlite3VdbeMemSetZeroBlob(Mem*,int);
void sqlite3VdbeMemSetRowSet(Mem*);
int sqlite3VdbeMemMakeWriteable(Mem*);
int sqlite3VdbeMemStringify(Mem*, int);
i64 sqlite3VdbeIntValue(Mem*);
int sqlite3VdbeMemIntegerify(Mem*);
double sqlite3VdbeRealValue(Mem*);
void sqlite3VdbeIntegerAffinity(Mem*);
int sqlite3VdbeMemRealify(Mem*);
int sqlite3VdbeMemNumerify(Mem*);
int sqlite3VdbeMemFromBtree(BtCursor*,int,int,int,Mem*);
void sqlite3VdbeMemRelease(Mem *p);
void sqlite3VdbeMemReleaseExternal(Mem *p);
#define VdbeMemRelease(X)  \
  if((X)->flags&(MEM_Agg|MEM_Dyn|MEM_RowSet|MEM_Frame)) \
    sqlite3VdbeMemReleaseExternal(X);
int sqlite3VdbeMemFinalize(Mem*, FuncDef*);
const char *sqlite3OpcodeName(int);
int sqlite3VdbeMemGrow(Mem *pMem, int n, int preserve);
int sqlite3VdbeCloseStatement(Vdbe *, int);
void sqlite3VdbeFrameDelete(VdbeFrame*);
int sqlite3VdbeFrameRestore(VdbeFrame *);
void sqlite3VdbeMemStoreType(Mem *pMem);
int sqlite3VdbeTransferError(Vdbe *p);

int sqlite3VdbeSorterInit(sqlite3 *, VdbeCursor *);
void sqlite3VdbeSorterClose(sqlite3 *, VdbeCursor *);
int sqlite3VdbeSorterRowkey(const VdbeCursor *, Mem *);
int sqlite3VdbeSorterNext(sqlite3 *, const VdbeCursor *, int *);
int sqlite3VdbeSorterRewind(sqlite3 *, const VdbeCursor *, int *);
int sqlite3VdbeSorterWrite(sqlite3 *, const VdbeCursor *, Mem *);
int sqlite3VdbeSorterCompare(const VdbeCursor *, Mem *, int *);

#if !defined(SQLITE_OMIT_SHARED_CACHE) && SQLITE_THREADSAFE>0
  void sqlite3VdbeEnter(Vdbe*);
  void sqlite3VdbeLeave(Vdbe*);
#else
# define sqlite3VdbeEnter(X)
# define sqlite3VdbeLeave(X)
#endif

#ifdef SQLITE_DEBUG
void sqlite3VdbeMemAboutToChange(Vdbe*,Mem*);
#endif

#ifndef SQLITE_OMIT_FOREIGN_KEY
int sqlite3VdbeCheckFk(Vdbe *, int);
#else
# define sqlite3VdbeCheckFk(p,i) 0
#endif

int sqlite3VdbeMemTranslate(Mem*, u8);
#ifdef SQLITE_DEBUG
  void sqlite3VdbePrintSql(Vdbe*);
  void sqlite3VdbeMemPrettyPrint(Mem *pMem, char *zBuf);
#endif
int sqlite3VdbeMemHandleBom(Mem *pMem);

#ifndef SQLITE_OMIT_INCRBLOB
  int sqlite3VdbeMemExpandBlob(Mem *);
  #define ExpandBlob(P) (((P)->flags&MEM_Zero)?sqlite3VdbeMemExpandBlob(P):0)
#else
  #define sqlite3VdbeMemExpandBlob(x) SQLITE_OK
  #define ExpandBlob(P) SQLITE_OK
#endif

#endif /* !defined(_VDBEINT_H_) */
