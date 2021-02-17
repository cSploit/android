/*
 * contrib/intarray/_int.h
 */
#ifndef ___INT_H__
#define ___INT_H__

#include "utils/array.h"

/* number ranges for compression */
#define MAXNUMRANGE 100

/* useful macros for accessing int4 arrays */
#define ARRPTR(x)  ( (int4 *) ARR_DATA_PTR(x) )
#define ARRNELEMS(x)  ArrayGetNItems(ARR_NDIM(x), ARR_DIMS(x))

/* reject arrays we can't handle; to wit, those containing nulls */
#define CHECKARRVALID(x) \
	do { \
		if (ARR_HASNULL(x) && array_contains_nulls(x)) \
			ereport(ERROR, \
					(errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED), \
					 errmsg("array must not contain nulls"))); \
	} while(0)

#define ARRISEMPTY(x)  (ARRNELEMS(x) == 0)

/* sort the elements of the array */
#define SORT(x) \
	do { \
		int		_nelems_ = ARRNELEMS(x); \
		if (_nelems_ > 1) \
			isort(ARRPTR(x), _nelems_); \
	} while(0)

/* sort the elements of the array and remove duplicates */
#define PREPAREARR(x) \
	do { \
		int		_nelems_ = ARRNELEMS(x); \
		if (_nelems_ > 1) \
			if (isort(ARRPTR(x), _nelems_)) \
				(x) = _int_unique(x); \
	} while(0)

/* "wish" function */
#define WISH_F(a,b,c) (double)( -(double)(((a)-(b))*((a)-(b))*((a)-(b)))*(c) )


/* bigint defines */
#define SIGLENINT  63			/* >122 => key will toast, so very slow!!! */
#define SIGLEN	( sizeof(int)*SIGLENINT )
#define SIGLENBIT (SIGLEN*BITS_PER_BYTE)

typedef char BITVEC[SIGLEN];
typedef char *BITVECP;

#define LOOPBYTE \
			for(i=0;i<SIGLEN;i++)

/* beware of multiple evaluation of arguments to these macros! */
#define GETBYTE(x,i) ( *( (BITVECP)(x) + (int)( (i) / BITS_PER_BYTE ) ) )
#define GETBITBYTE(x,i) ( (*((char*)(x)) >> (i)) & 0x01 )
#define CLRBIT(x,i)   GETBYTE(x,i) &= ~( 0x01 << ( (i) % BITS_PER_BYTE ) )
#define SETBIT(x,i)   GETBYTE(x,i) |=  ( 0x01 << ( (i) % BITS_PER_BYTE ) )
#define GETBIT(x,i) ( (GETBYTE(x,i) >> ( (i) % BITS_PER_BYTE )) & 0x01 )
#define HASHVAL(val) (((unsigned int)(val)) % SIGLENBIT)
#define HASH(sign, val) SETBIT((sign), HASHVAL(val))

/*
 * type of index key
 */
typedef struct
{
	int32		vl_len_;		/* varlena header (do not touch directly!) */
	int4		flag;
	char		data[1];
} GISTTYPE;

#define ALLISTRUE		0x04

#define ISALLTRUE(x)	( ((GISTTYPE*)x)->flag & ALLISTRUE )

#define GTHDRSIZE		(VARHDRSZ + sizeof(int4))
#define CALCGTSIZE(flag) ( GTHDRSIZE+(((flag) & ALLISTRUE) ? 0 : SIGLEN) )

#define GETSIGN(x)		( (BITVECP)( (char*)x+GTHDRSIZE ) )

/*
 * types for functions
 */
typedef ArrayType *(*formarray) (ArrayType *, ArrayType *);
typedef void (*formfloat) (ArrayType *, float *);

/*
 * useful functions
 */
bool		isort(int4 *a, int len);
ArrayType  *new_intArrayType(int num);
ArrayType  *copy_intArrayType(ArrayType *a);
ArrayType  *resize_intArrayType(ArrayType *a, int num);
int			internal_size(int *a, int len);
ArrayType  *_int_unique(ArrayType *a);
int32		intarray_match_first(ArrayType *a, int32 elem);
ArrayType  *intarray_add_elem(ArrayType *a, int32 elem);
ArrayType  *intarray_concat_arrays(ArrayType *a, ArrayType *b);
ArrayType  *int_to_intset(int32 elem);
bool		inner_int_overlap(ArrayType *a, ArrayType *b);
bool		inner_int_contains(ArrayType *a, ArrayType *b);
ArrayType  *inner_int_union(ArrayType *a, ArrayType *b);
ArrayType  *inner_int_inter(ArrayType *a, ArrayType *b);
void		rt__int_size(ArrayType *a, float *size);
void		gensign(BITVEC sign, int *a, int len);


/*****************************************************************************
 *			Boolean Search
 *****************************************************************************/

#define BooleanSearchStrategy	20

/*
 * item in polish notation with back link
 * to left operand
 */
typedef struct ITEM
{
	int2		type;
	int2		left;
	int4		val;
} ITEM;

typedef struct QUERYTYPE
{
	int32		vl_len_;		/* varlena header (do not touch directly!) */
	int4		size;			/* number of ITEMs */
	ITEM		items[1];		/* variable length array */
} QUERYTYPE;

#define HDRSIZEQT	offsetof(QUERYTYPE, items)
#define COMPUTESIZE(size)	( HDRSIZEQT + (size) * sizeof(ITEM) )
#define GETQUERY(x)  ( (x)->items )

/* "type" codes for ITEM */
#define END		0
#define ERR		1
#define VAL		2
#define OPR		3
#define OPEN	4
#define CLOSE	5

/* fmgr macros for QUERYTYPE objects */
#define DatumGetQueryTypeP(X)		  ((QUERYTYPE *) PG_DETOAST_DATUM(X))
#define DatumGetQueryTypePCopy(X)	  ((QUERYTYPE *) PG_DETOAST_DATUM_COPY(X))
#define PG_GETARG_QUERYTYPE_P(n)	  DatumGetQueryTypeP(PG_GETARG_DATUM(n))
#define PG_GETARG_QUERYTYPE_P_COPY(n) DatumGetQueryTypePCopy(PG_GETARG_DATUM(n))

bool		signconsistent(QUERYTYPE *query, BITVEC sign, bool calcnot);
bool		execconsistent(QUERYTYPE *query, ArrayType *array, bool calcnot);

bool		gin_bool_consistent(QUERYTYPE *query, bool *check);
bool		query_has_required_values(QUERYTYPE *query);

int			compASC(const void *a, const void *b);
int			compDESC(const void *a, const void *b);

/* sort, either ascending or descending */
#define QSORT(a, direction) \
	do { \
		int		_nelems_ = ARRNELEMS(a); \
		if (_nelems_ > 1) \
			qsort((void*) ARRPTR(a), _nelems_, sizeof(int4), \
				  (direction) ? compASC : compDESC ); \
	} while(0)

#endif   /* ___INT_H__ */
