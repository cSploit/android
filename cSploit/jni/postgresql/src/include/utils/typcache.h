/*-------------------------------------------------------------------------
 *
 * typcache.h
 *	  Type cache definitions.
 *
 * The type cache exists to speed lookup of certain information about data
 * types that is not directly available from a type's pg_type row.
 *
 * Portions Copyright (c) 1996-2011, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/utils/typcache.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef TYPCACHE_H
#define TYPCACHE_H

#include "access/tupdesc.h"
#include "fmgr.h"


/* TypeCacheEnumData is an opaque struct known only within typcache.c */
struct TypeCacheEnumData;

typedef struct TypeCacheEntry
{
	/* typeId is the hash lookup key and MUST BE FIRST */
	Oid			type_id;		/* OID of the data type */

	/* some subsidiary information copied from the pg_type row */
	int16		typlen;
	bool		typbyval;
	char		typalign;
	char		typtype;
	Oid			typrelid;

	/*
	 * Information obtained from opfamily entries
	 *
	 * These will be InvalidOid if no match could be found, or if the
	 * information hasn't yet been requested.  Also note that for array and
	 * composite types, typcache.c checks that the contained types are
	 * comparable or hashable before allowing eq_opr etc to become set.
	 */
	Oid			btree_opf;		/* the default btree opclass' family */
	Oid			btree_opintype; /* the default btree opclass' opcintype */
	Oid			hash_opf;		/* the default hash opclass' family */
	Oid			hash_opintype;	/* the default hash opclass' opcintype */
	Oid			eq_opr;			/* the equality operator */
	Oid			lt_opr;			/* the less-than operator */
	Oid			gt_opr;			/* the greater-than operator */
	Oid			cmp_proc;		/* the btree comparison function */
	Oid			hash_proc;		/* the hash calculation function */

	/*
	 * Pre-set-up fmgr call info for the equality operator, the btree
	 * comparison function, and the hash calculation function.	These are kept
	 * in the type cache to avoid problems with memory leaks in repeated calls
	 * to functions such as array_eq, array_cmp, hash_array.  There is not
	 * currently a need to maintain call info for the lt_opr or gt_opr.
	 */
	FmgrInfo	eq_opr_finfo;
	FmgrInfo	cmp_proc_finfo;
	FmgrInfo	hash_proc_finfo;

	/*
	 * Tuple descriptor if it's a composite type (row type).  NULL if not
	 * composite or information hasn't yet been requested.  (NOTE: this is a
	 * reference-counted tupledesc.)
	 */
	TupleDesc	tupDesc;

	/* Private data, for internal use of typcache.c only */
	int			flags;			/* flags about what we've computed */

	/*
	 * Private information about an enum type.	NULL if not enum or
	 * information hasn't been requested.
	 */
	struct TypeCacheEnumData *enumData;
} TypeCacheEntry;

/* Bit flags to indicate which fields a given caller needs to have set */
#define TYPECACHE_EQ_OPR			0x0001
#define TYPECACHE_LT_OPR			0x0002
#define TYPECACHE_GT_OPR			0x0004
#define TYPECACHE_CMP_PROC			0x0008
#define TYPECACHE_HASH_PROC			0x0010
#define TYPECACHE_EQ_OPR_FINFO		0x0020
#define TYPECACHE_CMP_PROC_FINFO	0x0040
#define TYPECACHE_HASH_PROC_FINFO	0x0080
#define TYPECACHE_TUPDESC			0x0100
#define TYPECACHE_BTREE_OPFAMILY	0x0200
#define TYPECACHE_HASH_OPFAMILY		0x0400

extern TypeCacheEntry *lookup_type_cache(Oid type_id, int flags);

extern TupleDesc lookup_rowtype_tupdesc(Oid type_id, int32 typmod);

extern TupleDesc lookup_rowtype_tupdesc_noerror(Oid type_id, int32 typmod,
							   bool noError);

extern TupleDesc lookup_rowtype_tupdesc_copy(Oid type_id, int32 typmod);

extern void assign_record_type_typmod(TupleDesc tupDesc);

extern int	compare_values_of_enum(TypeCacheEntry *tcache, Oid arg1, Oid arg2);

#endif   /* TYPCACHE_H */
