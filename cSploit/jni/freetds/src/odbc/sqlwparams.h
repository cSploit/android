#if 0
# ODBC_FUNC(SQLTest, (P(SQLSMALLINT, x), PCHAR(y) WIDE))
#endif

#undef WIDE
#undef P
#undef PCHAR
#undef PCHARIN
#undef PCHAROUT

#ifdef ENABLE_ODBC_WIDE
#  define WIDE , int wide
#  define PCHAR(a) ODBC_CHAR* a
#else
#  define WIDE
#  define PCHAR(a) SQLCHAR* a
#endif

#define P(a,b) a b
#define PCHARIN(n,t) PCHAR(sz ## n), P(t, cb ## n)
#define PCHAROUT(n,t) PCHAR(sz ## n), P(t, cb ## n ## Max), P(t FAR*, pcb ## n)

#define ODBC_FUNC(name, params) \
	static SQLRETURN _ ## name params

