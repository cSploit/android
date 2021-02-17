/* Automatically generated.  Do not edit */
/* See the mkopcodeh.awk script for details */
#define OP_Goto                                 1
#define OP_Gosub                                2
#define OP_Return                               3
#define OP_Yield                                4
#define OP_HaltIfNull                           5
#define OP_Halt                                 6
#define OP_Integer                              7
#define OP_Int64                                8
#define OP_Real                               130   /* same as TK_FLOAT    */
#define OP_String8                             94   /* same as TK_STRING   */
#define OP_String                               9
#define OP_Null                                10
#define OP_Blob                                11
#define OP_Variable                            12
#define OP_Move                                13
#define OP_Copy                                14
#define OP_SCopy                               15
#define OP_ResultRow                           16
#define OP_Concat                              91   /* same as TK_CONCAT   */
#define OP_Add                                 86   /* same as TK_PLUS     */
#define OP_Subtract                            87   /* same as TK_MINUS    */
#define OP_Multiply                            88   /* same as TK_STAR     */
#define OP_Divide                              89   /* same as TK_SLASH    */
#define OP_Remainder                           90   /* same as TK_REM      */
#define OP_CollSeq                             17
#define OP_Function                            18
#define OP_BitAnd                              82   /* same as TK_BITAND   */
#define OP_BitOr                               83   /* same as TK_BITOR    */
#define OP_ShiftLeft                           84   /* same as TK_LSHIFT   */
#define OP_ShiftRight                          85   /* same as TK_RSHIFT   */
#define OP_AddImm                              20
#define OP_MustBeInt                           21
#define OP_RealAffinity                        22
#define OP_ToText                             141   /* same as TK_TO_TEXT  */
#define OP_ToBlob                             142   /* same as TK_TO_BLOB  */
#define OP_ToNumeric                          143   /* same as TK_TO_NUMERIC*/
#define OP_ToInt                              144   /* same as TK_TO_INT   */
#define OP_ToReal                             145   /* same as TK_TO_REAL  */
#define OP_Eq                                  76   /* same as TK_EQ       */
#define OP_Ne                                  75   /* same as TK_NE       */
#define OP_Lt                                  79   /* same as TK_LT       */
#define OP_Le                                  78   /* same as TK_LE       */
#define OP_Gt                                  77   /* same as TK_GT       */
#define OP_Ge                                  80   /* same as TK_GE       */
#define OP_Permutation                         23
#define OP_Compare                             24
#define OP_Jump                                25
#define OP_And                                 69   /* same as TK_AND      */
#define OP_Or                                  68   /* same as TK_OR       */
#define OP_Not                                 19   /* same as TK_NOT      */
#define OP_BitNot                              93   /* same as TK_BITNOT   */
#define OP_Once                                26
#define OP_If                                  27
#define OP_IfNot                               28
#define OP_IsNull                              73   /* same as TK_ISNULL   */
#define OP_NotNull                             74   /* same as TK_NOTNULL  */
#define OP_Column                              29
#define OP_Affinity                            30
#define OP_MakeRecord                          31
#define OP_Count                               32
#define OP_Savepoint                           33
#define OP_AutoCommit                          34
#define OP_Transaction                         35
#define OP_ReadCookie                          36
#define OP_SetCookie                           37
#define OP_VerifyCookie                        38
#define OP_OpenRead                            39
#define OP_OpenWrite                           40
#define OP_OpenAutoindex                       41
#define OP_OpenEphemeral                       42
#define OP_SorterOpen                          43
#define OP_OpenPseudo                          44
#define OP_Close                               45
#define OP_SeekLt                              46
#define OP_SeekLe                              47
#define OP_SeekGe                              48
#define OP_SeekGt                              49
#define OP_Seek                                50
#define OP_NotFound                            51
#define OP_Found                               52
#define OP_IsUnique                            53
#define OP_NotExists                           54
#define OP_Sequence                            55
#define OP_NewRowid                            56
#define OP_Insert                              57
#define OP_InsertInt                           58
#define OP_Delete                              59
#define OP_ResetCount                          60
#define OP_SorterCompare                       61
#define OP_SorterData                          62
#define OP_RowKey                              63
#define OP_RowData                             64
#define OP_Rowid                               65
#define OP_NullRow                             66
#define OP_Last                                67
#define OP_SorterSort                          70
#define OP_Sort                                71
#define OP_Rewind                              72
#define OP_SorterNext                          81
#define OP_Prev                                92
#define OP_Next                                95
#define OP_SorterInsert                        96
#define OP_IdxInsert                           97
#define OP_IdxDelete                           98
#define OP_IdxRowid                            99
#define OP_IdxLT                              100
#define OP_IdxGE                              101
#define OP_Destroy                            102
#define OP_Clear                              103
#define OP_CreateIndex                        104
#define OP_CreateTable                        105
#define OP_ParseSchema                        106
#define OP_LoadAnalysis                       107
#define OP_DropTable                          108
#define OP_DropIndex                          109
#define OP_DropTrigger                        110
#define OP_IntegrityCk                        111
#define OP_RowSetAdd                          112
#define OP_RowSetRead                         113
#define OP_RowSetTest                         114
#define OP_Program                            115
#define OP_Param                              116
#define OP_FkCounter                          117
#define OP_FkIfZero                           118
#define OP_MemMax                             119
#define OP_IfPos                              120
#define OP_IfNeg                              121
#define OP_IfZero                             122
#define OP_AggStep                            123
#define OP_AggFinal                           124
#define OP_Checkpoint                         125
#define OP_JournalMode                        126
#define OP_Vacuum                             127
#define OP_IncrVacuum                         128
#define OP_Expire                             129
#define OP_TableLock                          131
#define OP_VBegin                             132
#define OP_VCreate                            133
#define OP_VDestroy                           134
#define OP_VOpen                              135
#define OP_VFilter                            136
#define OP_VColumn                            137
#define OP_VNext                              138
#define OP_VRename                            139
#define OP_VUpdate                            140
#define OP_Pagecount                          146
#define OP_MaxPgcnt                           147
#define OP_Trace                              148
#define OP_Noop                               149
#define OP_Explain                            150


/* Properties such as "out2" or "jump" that are specified in
** comments following the "case" for each opcode in the vdbe.c
** are encoded into bitvectors as follows:
*/
#define OPFLG_JUMP            0x0001  /* jump:  P2 holds jmp target */
#define OPFLG_OUT2_PRERELEASE 0x0002  /* out2-prerelease: */
#define OPFLG_IN1             0x0004  /* in1:   P1 is an input */
#define OPFLG_IN2             0x0008  /* in2:   P2 is an input */
#define OPFLG_IN3             0x0010  /* in3:   P3 is an input */
#define OPFLG_OUT2            0x0020  /* out2:  P2 is an output */
#define OPFLG_OUT3            0x0040  /* out3:  P3 is an output */
#define OPFLG_INITIALIZER {\
/*   0 */ 0x00, 0x01, 0x01, 0x04, 0x04, 0x10, 0x00, 0x02,\
/*   8 */ 0x02, 0x02, 0x02, 0x02, 0x02, 0x00, 0x00, 0x24,\
/*  16 */ 0x00, 0x00, 0x00, 0x24, 0x04, 0x05, 0x04, 0x00,\
/*  24 */ 0x00, 0x01, 0x01, 0x05, 0x05, 0x00, 0x00, 0x00,\
/*  32 */ 0x02, 0x00, 0x00, 0x00, 0x02, 0x10, 0x00, 0x00,\
/*  40 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x11,\
/*  48 */ 0x11, 0x11, 0x08, 0x11, 0x11, 0x11, 0x11, 0x02,\
/*  56 */ 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,\
/*  64 */ 0x00, 0x02, 0x00, 0x01, 0x4c, 0x4c, 0x01, 0x01,\
/*  72 */ 0x01, 0x05, 0x05, 0x15, 0x15, 0x15, 0x15, 0x15,\
/*  80 */ 0x15, 0x01, 0x4c, 0x4c, 0x4c, 0x4c, 0x4c, 0x4c,\
/*  88 */ 0x4c, 0x4c, 0x4c, 0x4c, 0x01, 0x24, 0x02, 0x01,\
/*  96 */ 0x08, 0x08, 0x00, 0x02, 0x01, 0x01, 0x02, 0x00,\
/* 104 */ 0x02, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,\
/* 112 */ 0x0c, 0x45, 0x15, 0x01, 0x02, 0x00, 0x01, 0x08,\
/* 120 */ 0x05, 0x05, 0x05, 0x00, 0x00, 0x00, 0x02, 0x00,\
/* 128 */ 0x01, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,\
/* 136 */ 0x01, 0x00, 0x01, 0x00, 0x00, 0x04, 0x04, 0x04,\
/* 144 */ 0x04, 0x04, 0x02, 0x02, 0x00, 0x00, 0x00,}
