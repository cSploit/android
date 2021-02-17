/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */

#ifndef _DB_PAGE_H_
#define	_DB_PAGE_H_

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * DB page formats.
 *
 * !!!
 * This implementation requires that values within the following structures
 * NOT be padded -- note, ANSI C permits random padding within structures.
 * If your compiler pads randomly you can just forget ever making DB run on
 * your system.  In addition, no data type can require larger alignment than
 * its own size, e.g., a 4-byte data element may not require 8-byte alignment.
 *
 * Note that key/data lengths are often stored in db_indx_t's -- this is
 * not accidental, nor does it limit the key/data size.  If the key/data
 * item fits on a page, it's guaranteed to be small enough to fit into a
 * db_indx_t, and storing it in one saves space.
 */

#define	PGNO_INVALID	0	/* Invalid page number in any database. */
#define	PGNO_BASE_MD	0	/* Base database: metadata page number. */

/* Page types. */
#define	P_INVALID	0	/* Invalid page type. */
#define	__P_DUPLICATE	1	/* Duplicate. DEPRECATED in 3.1 */
#define	P_HASH_UNSORTED	2	/* Hash pages created pre 4.6. DEPRECATED */
#define	P_IBTREE	3	/* Btree internal. */
#define	P_IRECNO	4	/* Recno internal. */
#define	P_LBTREE	5	/* Btree leaf. */
#define	P_LRECNO	6	/* Recno leaf. */
#define	P_OVERFLOW	7	/* Overflow. */
#define	P_HASHMETA	8	/* Hash metadata page. */
#define	P_BTREEMETA	9	/* Btree metadata page. */
#define	P_QAMMETA	10	/* Queue metadata page. */
#define	P_QAMDATA	11	/* Queue data page. */
#define	P_LDUP		12	/* Off-page duplicate leaf. */
#define	P_HASH		13	/* Sorted hash page. */
#define	P_HEAPMETA	14	/* Heap metadata page. */
#define	P_HEAP		15	/* Heap data page. */
#define	P_IHEAP		16	/* Heap internal. */
#define	P_PAGETYPE_MAX	17
/* Flag to __db_new */
#define	P_DONTEXTEND	0x8000	/* Don't allocate if there are no free pages. */

/*
 * When we create pages in mpool, we ask mpool to clear some number of bytes
 * in the header.  This number must be at least as big as the regular page
 * headers and cover enough of the btree and hash meta-data pages to obliterate
 * the page type.
 */
#define	DB_PAGE_DB_LEN		32
#define	DB_PAGE_QUEUE_LEN	0

/************************************************************************
 GENERIC METADATA PAGE HEADER
 *
 * !!!
 * The magic and version numbers have to be in the same place in all versions
 * of the metadata page as the application may not have upgraded the database.
 ************************************************************************/
typedef struct _dbmeta33 {
	DB_LSN	  lsn;		/* 00-07: LSN. */
	db_pgno_t pgno;		/* 08-11: Current page number. */
	u_int32_t magic;	/* 12-15: Magic number. */
	u_int32_t version;	/* 16-19: Version. */
	u_int32_t pagesize;	/* 20-23: Pagesize. */
	u_int8_t  encrypt_alg;	/*    24: Encryption algorithm. */
	u_int8_t  type;		/*    25: Page type. */
#define	DBMETA_CHKSUM		0x01
#define	DBMETA_PART_RANGE	0x02
#define	DBMETA_PART_CALLBACK	0x04
	u_int8_t  metaflags;	/* 26: Meta-only flags */
	u_int8_t  unused1;	/* 27: Unused. */
	u_int32_t free;		/* 28-31: Free list page number. */
	db_pgno_t last_pgno;	/* 32-35: Page number of last page in db. */
	u_int32_t nparts;	/* 36-39: Number of partitions. */
	u_int32_t key_count;	/* 40-43: Cached key count. */
	u_int32_t record_count;	/* 44-47: Cached record count. */
	u_int32_t flags;	/* 48-51: Flags: unique to each AM. */
				/* 52-71: Unique file ID. */
	u_int8_t  uid[DB_FILE_ID_LEN];
} DBMETA33, DBMETA;


/************************************************************************
 BTREE METADATA PAGE LAYOUT
 ************************************************************************/
typedef struct _btmeta33 {
#define	BTM_DUP		0x001	/*	  Duplicates. */
#define	BTM_RECNO	0x002	/*	  Recno tree. */
#define	BTM_RECNUM	0x004	/*	  Btree: maintain record count. */
#define	BTM_FIXEDLEN	0x008	/*	  Recno: fixed length records. */
#define	BTM_RENUMBER	0x010	/*	  Recno: renumber on insert/delete. */
#define	BTM_SUBDB	0x020	/*	  Subdatabases. */
#define	BTM_DUPSORT	0x040	/*	  Duplicates are sorted. */
#define	BTM_COMPRESS	0x080	/*	  Compressed. */
#define	BTM_MASK	0x0ff
	DBMETA	dbmeta;		/* 00-71: Generic meta-data header. */

	u_int32_t unused1;	/* 72-75: Unused space. */
	u_int32_t minkey;	/* 76-79: Btree: Minkey. */
	u_int32_t re_len;	/* 80-83: Recno: fixed-length record length. */
	u_int32_t re_pad;	/* 84-87: Recno: fixed-length record pad. */
	u_int32_t root;		/* 88-91: Root page. */
	u_int32_t blob_threshold;
				/* 92-95: Minimum blob file size. */
	u_int32_t blob_file_lo;	/* 96-99: Blob file dir id lo. */
	u_int32_t blob_file_hi;	/* 100-103: Blob file dir id hi. */
	u_int32_t blob_sdb_lo;	/* 104-107: Blob sdb dir id lo */
	u_int32_t blob_sdb_hi;	/* 108-111: Blob sdb dir id hi */
	u_int32_t unused2[87];	/* 112-459: Unused space. */
	u_int32_t crypto_magic;		/* 460-463: Crypto magic number */
	u_int32_t trash[3];		/* 464-475: Trash space - Do not use */
	u_int8_t iv[DB_IV_BYTES];	/* 476-495: Crypto IV */
	u_int8_t chksum[DB_MAC_KEY];	/* 496-511: Page chksum */

	/*
	 * Minimum page size is 512.
	 */
} BTMETA33, BTMETA;

/************************************************************************
 HASH METADATA PAGE LAYOUT
 ************************************************************************/
typedef struct _hashmeta33 {
#define	DB_HASH_DUP	0x01	/*	  Duplicates. */
#define	DB_HASH_SUBDB	0x02	/*	  Subdatabases. */
#define	DB_HASH_DUPSORT	0x04	/*	  Duplicates are sorted. */
	DBMETA dbmeta;		/* 00-71: Generic meta-data page header. */

	u_int32_t max_bucket;	/* 72-75: ID of Maximum bucket in use */
	u_int32_t high_mask;	/* 76-79: Modulo mask into table */
	u_int32_t low_mask;	/* 80-83: Modulo mask into table lower half */
	u_int32_t ffactor;	/* 84-87: Fill factor */
	u_int32_t nelem;	/* 88-91: Number of keys in hash table */
	u_int32_t h_charkey;	/* 92-95: Value of hash(CHARKEY) */
#define	NCACHED	32		/* number of spare points */
				/* 96-223: Spare pages for overflow */
	u_int32_t spares[NCACHED];
	u_int32_t blob_threshold;
				/* 224-227: Minimum blob file size. */
	u_int32_t blob_file_lo;	/* 228-231: Blob file dir id lo. */
	u_int32_t blob_file_hi;	/* 232-235: Blob file dir id hi. */
	u_int32_t blob_sdb_lo;	/* 236-239: Blob sdb dir id lo. */
	u_int32_t blob_sdb_hi;	/* 240-243: Blob sdb dir id hi. */
	u_int32_t unused[54];	/* 244-459: Unused space */
	u_int32_t crypto_magic;	/* 460-463: Crypto magic number */
	u_int32_t trash[3];	/* 464-475: Trash space - Do not use */
	u_int8_t iv[DB_IV_BYTES];	/* 476-495: Crypto IV */
	u_int8_t chksum[DB_MAC_KEY];	/* 496-511: Page chksum */

	/*
	 * Minimum page size is 512.
	 */
} HMETA33, HMETA;

/************************************************************************
 HEAP METADATA PAGE LAYOUT
*************************************************************************/
/*
 * Heap Meta data page structure
 *
 */
typedef struct _heapmeta {
	DBMETA    dbmeta;		/* 00-71: Generic meta-data header. */

	db_pgno_t curregion;		/* 72-75: Current region pgno. */
	u_int32_t nregions;		/* 76-79: Number of regions. */
	u_int32_t gbytes;		/* 80-83: GBytes for fixed size heap. */
	u_int32_t bytes;		/* 84-87: Bytes for fixed size heap. */
	u_int32_t region_size;		/* 88-91: Max region size. */
	u_int32_t blob_threshold;	/* 92-95: Minimum blob file size. */
	u_int32_t blob_file_lo;		/* 96-97: Blob file dir id lo. */
	u_int32_t blob_file_hi;		/* 98-101: Blob file dir id hi. */
	u_int32_t unused2[89];		/* 102-459: Unused space.*/
	u_int32_t crypto_magic;		/* 460-463: Crypto magic number */
	u_int32_t trash[3];		/* 464-475: Trash space - Do not use */
	u_int8_t  iv[DB_IV_BYTES];	/* 476-495: Crypto IV */
	u_int8_t  chksum[DB_MAC_KEY];	/* 496-511: Page chksum */


	/*
	 * Minimum page size is 512.
	 */
} HEAPMETA;
		
/************************************************************************
 QUEUE METADATA PAGE LAYOUT
 ************************************************************************/
/*
 * QAM Meta data page structure
 *
 */
typedef struct _qmeta33 {
	DBMETA    dbmeta;	/* 00-71: Generic meta-data header. */

	u_int32_t first_recno;	/* 72-75: First not deleted record. */
	u_int32_t cur_recno;	/* 76-79: Next recno to be allocated. */
	u_int32_t re_len;	/* 80-83: Fixed-length record length. */
	u_int32_t re_pad;	/* 84-87: Fixed-length record pad. */
	u_int32_t rec_page;	/* 88-91: Records Per Page. */
	u_int32_t page_ext;	/* 92-95: Pages per extent */

	u_int32_t unused[91];	/* 96-459: Unused space */
	u_int32_t crypto_magic;	/* 460-463: Crypto magic number */
	u_int32_t trash[3];	/* 464-475: Trash space - Do not use */
	u_int8_t iv[DB_IV_BYTES];	/* 476-495: Crypto IV */
	u_int8_t chksum[DB_MAC_KEY];	/* 496-511: Page chksum */
	/*
	 * Minimum page size is 512.
	 */
} QMETA33, QMETA;

/*
 * DBMETASIZE is a constant used by __db_file_setup and DB->verify
 * as a buffer which is guaranteed to be larger than any possible
 * metadata page size and smaller than any disk sector.
 */
#define	DBMETASIZE	512

/************************************************************************
 BTREE/HASH MAIN PAGE LAYOUT
 ************************************************************************/
/*
 *	+-----------------------------------+
 *	|    lsn    |   pgno    | prev pgno |
 *	+-----------------------------------+
 *	| next pgno |  entries  | hf offset |
 *	+-----------------------------------+
 *	|   level   |   type    |   chksum  |
 *	+-----------------------------------+
 *	|    iv     |   index   | free -->  |
 *	+-----------+-----------------------+
 *	|	 F R E E A R E A            |
 *	+-----------------------------------+
 *	|              <-- free |   item    |
 *	+-----------------------------------+
 *	|   item    |   item    |   item    |
 *	+-----------------------------------+
 *
 * sizeof(PAGE) == 26 bytes + possibly 20 bytes of checksum and possibly
 * 16 bytes of IV (+ 2 bytes for alignment), and the following indices
 * are guaranteed to be two-byte aligned.  If we aren't doing crypto or
 * checksumming the bytes are reclaimed for data storage.
 *
 * For hash and btree leaf pages, index items are paired, e.g., inp[0] is the
 * key for inp[1]'s data.  All other types of pages only contain single items.
 */
typedef struct __pg_chksum {
	u_int8_t	unused[2];		/* 26-27: For alignment */
	u_int8_t	chksum[4];		/* 28-31: Checksum */
} PG_CHKSUM;

typedef struct __pg_crypto {
	u_int8_t	unused[2];		/* 26-27: For alignment */
	u_int8_t	chksum[DB_MAC_KEY];	/* 28-47: Checksum */
	u_int8_t	iv[DB_IV_BYTES];	/* 48-63: IV */
	/* !!!
	 * Must be 16-byte aligned for crypto
	 */
} PG_CRYPTO;

typedef struct _db_page {
	DB_LSN	  lsn;		/* 00-07: Log sequence number. */
	db_pgno_t pgno;		/* 08-11: Current page number. */
	db_pgno_t prev_pgno;	/* 12-15: Previous page number. */
	db_pgno_t next_pgno;	/* 16-19: Next page number. */
	db_indx_t entries;	/* 20-21: Number of items on the page. */
	db_indx_t hf_offset;	/* 22-23: High free byte page offset. */

	/*
	 * The btree levels are numbered from the leaf to the root, starting
	 * with 1, so the leaf is level 1, its parent is level 2, and so on.
	 * We maintain this level on all btree pages, but the only place that
	 * we actually need it is on the root page.  It would not be difficult
	 * to hide the byte on the root page once it becomes an internal page,
	 * so we could get this byte back if we needed it for something else.
	 */
#define	LEAFLEVEL	  1
#define	MAXBTREELEVEL	255
	u_int8_t  level;	/*    24: Btree tree level. */
	u_int8_t  type;		/*    25: Page type. */
} PAGE;

/*
 * With many compilers sizeof(PAGE) == 28, while SIZEOF_PAGE == 26.
 * We add in other things directly after the page header and need
 * the SIZEOF_PAGE.  When giving the sizeof(), many compilers will
 * pad it out to the next 4-byte boundary.
 */
#define	SIZEOF_PAGE	26
/*
 * !!!
 * DB_AM_ENCRYPT always implies DB_AM_CHKSUM so that must come first.
 */
#define	P_INP(dbp, pg)							\
	((db_indx_t *)((u_int8_t *)(pg) + SIZEOF_PAGE +			\
	(F_ISSET((dbp), DB_AM_ENCRYPT) ? sizeof(PG_CRYPTO) :		\
	(F_ISSET((dbp), DB_AM_CHKSUM) ? sizeof(PG_CHKSUM) : 0))))

#define	P_IV(dbp, pg)							\
	(F_ISSET((dbp), DB_AM_ENCRYPT) ? ((u_int8_t *)(pg) +		\
	SIZEOF_PAGE + SSZA(PG_CRYPTO, iv))				\
	: NULL)

#define	P_CHKSUM(dbp, pg)						\
	(F_ISSET((dbp), DB_AM_ENCRYPT) ? ((u_int8_t *)(pg) +		\
	SIZEOF_PAGE + SSZA(PG_CRYPTO, chksum)) :			\
	(F_ISSET((dbp), DB_AM_CHKSUM) ? ((u_int8_t *)(pg) +		\
	SIZEOF_PAGE + SSZA(PG_CHKSUM, chksum))				\
	: NULL))

/* PAGE element macros. */
#define	LSN(p)		(((PAGE *)p)->lsn)
#define	PGNO(p)		(((PAGE *)p)->pgno)
#define	PREV_PGNO(p)	(((PAGE *)p)->prev_pgno)
#define	NEXT_PGNO(p)	(((PAGE *)p)->next_pgno)
#define	NUM_ENT(p)	(((PAGE *)p)->entries)
#define	HOFFSET(p)	(((PAGE *)p)->hf_offset)
#define	LEVEL(p)	(((PAGE *)p)->level)
#define	TYPE(p)		(((PAGE *)p)->type)

/************************************************************************
 HEAP PAGE LAYOUT
 ************************************************************************/
#define HEAPPG_NORMAL	26
#define HEAPPG_CHKSUM	48
#define HEAPPG_SEC	64

/*
 *	+0-----------2------------4-----------6-----------7+
 *	|                        lsn                       |
 *	+-------------------------+------------------------+
 *	|           pgno          |         unused0        |
 *      +-------------+-----------+-----------+------------+
 *	|  high_indx  | free_indx |  entries  |  hf offset |
 *	+-------+-----+-----------+-----------+------------+
 *	|unused2|type |  unused3  |      ...chksum...      |
 *	+-------+-----+-----------+------------------------+
 *	|  ...iv...   |   offset table / free space map    |
 *	+-------------+------------------------------------+
 *	|free->	 	F R E E A R E A                    |
 *	+--------------------------------------------------+
 *	|                <-- free |          item          |
 *	+-------------------------+------------------------+
 *	|           item          |          item          |
 *	+-------------------------+------------------------+
 *
 * The page layout of both heap internal and data pages.  If not using
 * crypto, iv will be overwritten with data.  If not using checksumming,
 * unused3 and chksum will also be overwritten with data and data will start at
 * 26.  Note that this layout lets us re-use a lot of the PAGE element macros
 * defined above.
 */
typedef struct _heappg {
	DB_LSN lsn;		/* 00-07: Log sequence number. */
	db_pgno_t pgno;		/* 08-11: Current page number. */
	u_int32_t high_pgno;	/* 12-15: Highest page in region. */
	u_int16_t high_indx;	/* 16-17: Highest index in the offset table. */
	db_indx_t free_indx;	/* 18-19: First available index. */
	db_indx_t entries;	/* 20-21: Number of items on the page. */
	db_indx_t hf_offset;	/* 22-23: High free byte page offset. */
	u_int8_t unused2[1];	/*    24: Unused. */
	u_int8_t type;		/*    25: Page type. */
	u_int8_t unused3[2];    /* 26-27: Never used, just checksum alignment. */
	u_int8_t  chksum[DB_MAC_KEY]; /* 28-47: Checksum */
	u_int8_t  iv[DB_IV_BYTES]; /* 48-63: IV */
} HEAPPG;

/* Define first possible data page for heap, 0 is metapage, 1 is region page */
#define FIRST_HEAP_RPAGE 1 
#define FIRST_HEAP_DPAGE 2

typedef struct __heaphdr {
#define HEAP_RECSPLIT 0x01 /* Heap data record is split */
#define HEAP_RECFIRST 0x02 /* First piece of a split record */
#define HEAP_RECLAST  0x04 /* Last piece of a split record */
#define HEAP_RECBLOB  0x08 /* Record refers to a blob */
	u_int8_t flags;		/* 00: Flags describing record. */
	u_int8_t unused;	/* 01: Padding. */
	u_int16_t size;		/* 02-03: The size of the stored data piece. */
} HEAPHDR;

typedef struct __heaphdrsplt {
	HEAPHDR std_hdr;	/* 00-03: The standard data header */
	u_int32_t tsize;	/* 04-07: Total record size, 1st piece only */
	db_pgno_t nextpg;	/* 08-11: RID.pgno of the next record piece */
	db_indx_t nextindx;	/* 12-13: RID.indx of the next record piece */
	u_int16_t unused;	/* 14-15: Padding. */
} HEAPSPLITHDR;

/*
 * HEAPBLOB, the blob database record for heap.
 * Saving bytes is not a concern for the blob record type - if too many
 * fit onto a single page, then we're likely to introduce unnecessary
 * contention for blobs. Using blobs implies storing large items, thus slightly
 * more per-item overhead is acceptable.
 * If this proves untrue, the crypto section of the record could be optional.
 * encoding, lsn, encryption, and checksum fields are unused at the moment, but
 * included to make adding those features easier.
 */
typedef struct _heapblob {
	HEAPHDR std_hdr;		/* 00-03: The standard data header */
	u_int8_t  encoding;		/*    04: Encoding of blob file. */
	u_int8_t  unused[7];		/* 05-11: Padding, unused. */
	u_int8_t  chksum[DB_MAC_KEY];	/* 12-31: Checksum */
	u_int8_t  iv[DB_IV_BYTES];	/* 32-47: IV */
	DB_LSN    lsn;			/* 48-55: LSN for blob file update. */
	u_int64_t id;			/* 56-63: Blob file identifier. */
	u_int64_t size;			/* 64-71: Blob file size. */
	u_int64_t file_id;		/* 72-80: File directory. */
} HEAPBLOBHDR, HEAPBLOBHDR60P1;

#define HEAP_HDRSIZE(hdr) 					\
	(F_ISSET((hdr), HEAP_RECSPLIT) ? sizeof(HEAPSPLITHDR) :	\
	sizeof(HEAPHDR))

#define HEAPBLOBREC_SIZE		(sizeof(HEAPBLOBHDR))
#define HEAPBLOBREC_DSIZE		(sizeof(HEAPBLOBHDR) - sizeof(HEAPHDR))
#define HEAPBLOBREC_DATA(p)		(((u_int8_t *)p) + sizeof(HEAPHDR))

#define HEAPPG_SZ(dbp)			       			\
	(F_ISSET((dbp), DB_AM_ENCRYPT) ? HEAPPG_SEC :		\
	F_ISSET((dbp), DB_AM_CHKSUM) ? HEAPPG_CHKSUM : HEAPPG_NORMAL)

/* Each byte in the bitmap describes 4 pages (2 bits per page.) */
#define HEAP_REGION_COUNT(dbp, size) (((size) - HEAPPG_SZ(dbp)) * 4)
#define HEAP_DEFAULT_REGION_MAX(dbp)				\
	(HEAP_REGION_COUNT(dbp, (u_int32_t)8 * 1024))
#define	HEAP_REGION_SIZE(dbp)	(((HEAP*) (dbp)->heap_internal)->region_size)

/* Figure out which region a given page belongs to. */
#define HEAP_REGION_PGNO(dbp, p) 				\
	((((p) - 1) / (HEAP_REGION_SIZE(dbp) + 1)) * 		\
	(HEAP_REGION_SIZE(dbp) + 1) + 1)
/* Translate a region pgno to region number */
#define HEAP_REGION_NUM(dbp, pgno)				\
	((((pgno) - 1) / (HEAP_REGION_SIZE((dbp)) + 1)) + 1)
/* 
 * Given an internal heap page and page number relative to that page, return the
 * bits from map describing free space on the nth page.  Each byte in the map
 * describes 4 pages. Point at the correct byte and mask the correct 2 bits.
 */
#define HEAP_SPACE(dbp, pg, n)					\
	(HEAP_SPACEMAP((dbp), (pg))[(n) / 4] >> (2 * ((n) % 4)) & 3)
      
#define HEAP_SETSPACE(dbp, pg, n, b) do {				\
	HEAP_SPACEMAP((dbp), (pg))[(n) / 4] &= ~(3 << (2 * ((n) % 4))); \
	HEAP_SPACEMAP((dbp), (pg))[(n) / 4] |= ((b & 3) << (2 * ((n) % 4))); \
} while (0)
		
/* Return the bitmap describing free space on heap data pages. */
#define HEAP_SPACEMAP(dbp, pg) ((u_int8_t *)P_INP((dbp), (pg)))

/* Return the offset table for a heap data page. */
#define HEAP_OFFSETTBL(dbp, pg) P_INP((dbp), (pg))

/* 
 * Calculate the % of a page a given size occupies and translate that to the
 * corresponding bitmap value. 
 */
#define HEAP_CALCSPACEBITS(dbp, sz, space) do {			\
	(space) = 100 * (sz) / (dbp)->pgsize;			\
	if ((space) <= HEAP_PG_FULL_PCT)			\
		(space) = HEAP_PG_FULL;				\
	else if ((space) <= HEAP_PG_GT66_PCT)			\
		(space) = HEAP_PG_GT66;				\
	else if ((space) <= HEAP_PG_GT33_PCT)			\
		(space) = HEAP_PG_GT33;				\
	else							\
		(space) = HEAP_PG_LT33;				\
} while (0)
	
/* Return the amount of free space on a heap data page. */
#define HEAP_FREESPACE(dbp, p)                                  \
	(HOFFSET(p) - HEAPPG_SZ(dbp) -				\
	(NUM_ENT(p) == 0 ? 0 : ((HEAP_HIGHINDX(p) + 1) * sizeof(db_indx_t))))

/* The maximum amount of data that can fit on an empty heap data page. */
#define HEAP_MAXDATASIZE(dbp)					\
	((dbp)->pgsize - HEAPPG_SZ(dbp) - sizeof(db_indx_t))

#define HEAP_FREEINDX(p)	(((HEAPPG *)p)->free_indx)
#define HEAP_HIGHINDX(p)	(((HEAPPG *)p)->high_indx)

/* True if we have a page that deals with heap */
#define HEAPTYPE(h)                                           \
    (TYPE(h) == P_HEAPMETA || TYPE(h) == P_HEAP || TYPE(h) == P_IHEAP)

/************************************************************************
 QUEUE MAIN PAGE LAYOUT
 ************************************************************************/
/*
 * Sizes of page below.  Used to reclaim space if not doing
 * crypto or checksumming.  If you change the QPAGE below you
 * MUST adjust this too.
 */
#define	QPAGE_NORMAL	28
#define	QPAGE_CHKSUM	48
#define	QPAGE_SEC	64

typedef struct _qpage {
	DB_LSN	  lsn;		/* 00-07: Log sequence number. */
	db_pgno_t pgno;		/* 08-11: Current page number. */
	u_int32_t unused0[3];	/* 12-23: Unused. */
	u_int8_t  unused1[1];	/*    24: Unused. */
	u_int8_t  type;		/*    25: Page type. */
	u_int8_t  unused2[2];	/* 26-27: Unused. */
	u_int8_t  chksum[DB_MAC_KEY]; /* 28-47: Checksum */
	u_int8_t  iv[DB_IV_BYTES]; /* 48-63: IV */
} QPAGE;

#define	QPAGE_SZ(dbp)						\
	(F_ISSET((dbp), DB_AM_ENCRYPT) ? QPAGE_SEC :		\
	F_ISSET((dbp), DB_AM_CHKSUM) ? QPAGE_CHKSUM : QPAGE_NORMAL)
/*
 * !!!
 * The next_pgno and prev_pgno fields are not maintained for btree and recno
 * internal pages.  Doing so only provides a minor performance improvement,
 * it's hard to do when deleting internal pages, and it increases the chance
 * of deadlock during deletes and splits because we have to re-link pages at
 * more than the leaf level.
 *
 * !!!
 * The btree/recno access method needs db_recno_t bytes of space on the root
 * page to specify how many records are stored in the tree.  (The alternative
 * is to store the number of records in the meta-data page, which will create
 * a second hot spot in trees being actively modified, or recalculate it from
 * the BINTERNAL fields on each access.)  Overload the PREV_PGNO field.
 */
#define	RE_NREC(p)							\
	((TYPE(p) == P_IBTREE || TYPE(p) == P_IRECNO) ?	PREV_PGNO(p) :	\
	(db_pgno_t)(TYPE(p) == P_LBTREE ? NUM_ENT(p) / 2 : NUM_ENT(p)))
#define	RE_NREC_ADJ(p, adj)						\
	PREV_PGNO(p) += adj;
#define	RE_NREC_SET(p, num)						\
	PREV_PGNO(p) = (num);

/*
 * Initialize a page.
 *
 * !!!
 * Don't modify the page's LSN, code depends on it being unchanged after a
 * P_INIT call.
 */
#define	P_INIT(pg, pg_size, n, pg_prev, pg_next, btl, pg_type) do {	\
	PGNO(pg) = (n);							\
	PREV_PGNO(pg) = (pg_prev);					\
	NEXT_PGNO(pg) = (pg_next);					\
	NUM_ENT(pg) = (0);						\
	HOFFSET(pg) = (db_indx_t)(pg_size);				\
	LEVEL(pg) = (btl);						\
	TYPE(pg) = (pg_type);						\
} while (0)

/* Page header length (offset to first index). */
#define	P_OVERHEAD(dbp)	P_TO_UINT16(P_INP(dbp, 0))

/* First free byte. */
#define	LOFFSET(dbp, pg)						\
    (P_OVERHEAD(dbp) + NUM_ENT(pg) * sizeof(db_indx_t))

/* Free space on a regular page. */
#define	P_FREESPACE(dbp, pg)	(HOFFSET(pg) - LOFFSET(dbp, pg))

/* Get a pointer to the bytes at a specific index. */
#define	P_ENTRY(dbp, pg, indx)	((u_int8_t *)pg + P_INP(dbp, pg)[indx])

/************************************************************************
 OVERFLOW PAGE LAYOUT
 ************************************************************************/

/*
 * Overflow items are referenced by HOFFPAGE and BOVERFLOW structures, which
 * store a page number (the first page of the overflow item) and a length
 * (the total length of the overflow item).  The overflow item consists of
 * some number of overflow pages, linked by the next_pgno field of the page.
 * A next_pgno field of PGNO_INVALID flags the end of the overflow item.
 *
 * Overflow page overloads:
 *	The amount of overflow data stored on each page is stored in the
 *	hf_offset field.
 *
 *	Before 4.3 the implementation reference counted overflow items as it
 *	once was possible for them to be promoted onto btree internal pages.
 *	The reference count is stored in the entries field. 
 */
#define	OV_LEN(p)	(((PAGE *)p)->hf_offset)
#define	OV_REF(p)	(((PAGE *)p)->entries)

/* Maximum number of bytes that you can put on an overflow page. */
#define	P_MAXSPACE(dbp, psize)	((psize) - P_OVERHEAD(dbp))

/* Free space on an overflow page. */
#define	P_OVFLSPACE(dbp, psize, pg)	(P_MAXSPACE(dbp, psize) - HOFFSET(pg))

/************************************************************************
 HASH PAGE LAYOUT
 ************************************************************************/

/* Each index references a group of bytes on the page. */
#define	H_KEYDATA	1	/* Key/data item. */
#define	H_DUPLICATE	2	/* Duplicate key/data item. */
#define	H_OFFPAGE	3	/* Overflow key/data item. */
#define	H_OFFDUP	4	/* Overflow page of duplicates. */
#define	H_BLOB		5	/* Blob file data item. */

/*
 * !!!
 * Items on hash pages are (potentially) unaligned, so we can never cast the
 * (page + offset) pointer to an HKEYDATA, HOFFPAGE or HOFFDUP structure, as
 * we do with B+tree on-page structures.  Because we frequently want the type
 * field, it requires no alignment, and it's in the same location in all three
 * structures, there's a pair of macros.
 */
#define	HPAGE_PTYPE(p)		(*(u_int8_t *)p)
#define	HPAGE_TYPE(dbp, pg, indx)	(*P_ENTRY(dbp, pg, indx))

/*
 * The first and second types are H_KEYDATA and H_DUPLICATE, represented
 * by the HKEYDATA structure:
 *
 *	+-----------------------------------+
 *	|    type   | key/data ...          |
 *	+-----------------------------------+
 *
 * For duplicates, the data field encodes duplicate elements in the data
 * field:
 *
 *	+---------------------------------------------------------------+
 *	|    type   | len1 | element1 | len1 | len2 | element2 | len2   |
 *	+---------------------------------------------------------------+
 *
 * Thus, by keeping track of the offset in the element, we can do both
 * backward and forward traversal.
 */
typedef struct _hkeydata {
	u_int8_t  type;		/*    00: Page type. */
	u_int8_t  data[1];	/* Variable length key/data item. */
} HKEYDATA;
#define	HKEYDATA_DATA(p)	(((u_int8_t *)p) + SSZA(HKEYDATA, data))

/*
 * The length of any HKEYDATA item. Note that indx is an element index,
 * not a PAIR index.
 */
#define	LEN_HITEM(dbp, pg, pgsize, indx)				\
	(((indx) == 0 ? (pgsize) :					\
	(P_INP(dbp, pg)[(indx) - 1])) - (P_INP(dbp, pg)[indx]))

#define	LEN_HKEYDATA(dbp, pg, psize, indx)				\
	(db_indx_t)(LEN_HITEM(dbp, pg, psize, indx) - HKEYDATA_SIZE(0))

/*
 * Page space required to add a new HKEYDATA item to the page, with and
 * without the index value.
 */
#define	HKEYDATA_SIZE(len)						\
	((len) + SSZA(HKEYDATA, data))
#define	HKEYDATA_PSIZE(len)						\
	(HKEYDATA_SIZE(len) + sizeof(db_indx_t))

/* Put a HKEYDATA item at the location referenced by a page entry. */
#define	PUT_HKEYDATA(pe, kd, len, etype) {				\
	((HKEYDATA *)(pe))->type = etype;				\
	memcpy((u_int8_t *)(pe) + sizeof(u_int8_t), kd, len);		\
}

/*
 * Macros the describe the page layout in terms of key-data pairs.
 */
#define	H_NUMPAIRS(pg)			(NUM_ENT(pg) / 2)
#define	H_KEYINDEX(indx)		(indx)
#define	H_DATAINDEX(indx)		((indx) + 1)
#define	H_PAIRKEY(dbp, pg, indx)	P_ENTRY(dbp, pg, H_KEYINDEX(indx))
#define	H_PAIRDATA(dbp, pg, indx)	P_ENTRY(dbp, pg, H_DATAINDEX(indx))
#define	H_PAIRSIZE(dbp, pg, psize, indx)				\
	(LEN_HITEM(dbp, pg, psize, H_KEYINDEX(indx)) +			\
	LEN_HITEM(dbp, pg, psize, H_DATAINDEX(indx)))
#define	LEN_HDATA(dbp, p, psize, indx)					\
    LEN_HKEYDATA(dbp, p, psize, H_DATAINDEX(indx))
#define	LEN_HKEY(dbp, p, psize, indx)					\
    LEN_HKEYDATA(dbp, p, psize, H_KEYINDEX(indx))

/*
 * The third type is the H_OFFPAGE, represented by the HOFFPAGE structure:
 */
typedef struct _hoffpage {
	u_int8_t  type;		/*    00: Page type and delete flag. */
	u_int8_t  unused[3];	/* 01-03: Padding, unused. */
	db_pgno_t pgno;		/* 04-07: Offpage page number. */
	u_int32_t tlen;		/* 08-11: Total length of item. */
} HOFFPAGE;

#define	HOFFPAGE_PGNO(p)	(((u_int8_t *)p) + SSZ(HOFFPAGE, pgno))
#define	HOFFPAGE_TLEN(p)	(((u_int8_t *)p) + SSZ(HOFFPAGE, tlen))

/*
 * Page space required to add a new HOFFPAGE item to the page, with and
 * without the index value.
 */
#define	HOFFPAGE_SIZE		(sizeof(HOFFPAGE))
#define	HOFFPAGE_PSIZE		(HOFFPAGE_SIZE + sizeof(db_indx_t))

/*
 * The fourth type is H_OFFDUP represented by the HOFFDUP structure:
 */
typedef struct _hoffdup {
	u_int8_t  type;		/*    00: Page type and delete flag. */
	u_int8_t  unused[3];	/* 01-03: Padding, unused. */
	db_pgno_t pgno;		/* 04-07: Offpage page number. */
} HOFFDUP;
#define	HOFFDUP_PGNO(p)		(((u_int8_t *)p) + SSZ(HOFFDUP, pgno))

/*
 * Page space required to add a new HOFFDUP item to the page, with and
 * without the index value.
 */
#define	HOFFDUP_SIZE		(sizeof(HOFFDUP))

/*
 * The fifth type is the H_BLOB, represented by the HBLOB structure.
 * Saving bytes is not a concern for the blob record type - if too many
 * fit onto a single page, then we're likely to introduce unnecessary
 * contention for blobs. Using blobs implies storing large items, thus slightly
 * more per-item overhead is acceptable.
 * If this proves untrue, the crypto section of the record could be optional.
 * encoding, encryption, and checksum fields are unused at the moment, but
 * included to make adding those features easier.
 */
typedef struct _hblob {
	u_int8_t  type;			/*    00: Page type and delete flag. */
	u_int8_t  encoding;		/*    01: Encoding of blob file. */
	u_int8_t  unused[10];		/* 02-11: Padding, unused. */
	u_int8_t  chksum[DB_MAC_KEY];	/* 12-31: Checksum */
	u_int8_t  iv[DB_IV_BYTES];	/* 32-47: IV */
	u_int64_t id;			/* 48-55: Blob file identifier. */
	u_int64_t size;			/* 56-63: Blob file size. */
	u_int64_t file_id;		/* 64-71: File directory. */
	u_int64_t sdb_id;		/* 72-79: Subdb that owns this blob. */
} HBLOB, HBLOB60P1;

#define	HBLOB_ID(p)	(((u_int8_t *)p) + SSZ(HBLOB, id))
#define	HBLOB_FILE_ID(p)	(((u_int8_t *)p) + SSZ(HBLOB, file_id))

/*
 * Return a off_t version of the u_int64_t blob size.
 * Since off_t can be a 32 or 64 integer on different systems, this macro
 * is used to catch cases of overflow.
 */
#define	GET_BLOB_SIZE(e, p, o, ret)	do {				\
	DB_ASSERT((e), sizeof(o) <= 8);					\
	if (sizeof(o) == 8) {						\
		(o) = (off_t)(p).size;					\
	} else {							\
		if ((p).size > INT_MAX) {				\
			__db_errx((e), DB_STR("0768",			\
			    "Blob size overflow."));			\
			(ret) = EINVAL;					\
		}							\
		(o) = (int32_t)(p).size;				\
	}								\
} while (0);

#define	SET_BLOB_FIELD(p, v, type, field)	do {			\
	u_int64_t tmp;							\
	tmp = (u_int64_t)(v);						\
	memcpy((u_int8_t *)(p) + SSZ(type, field),			\
	    &tmp, sizeof(u_int64_t));					\
} while (0);

#define	SET_BLOB_ID(p, v, type)						\
    SET_BLOB_FIELD(p, v, type, id)

#define	SET_BLOB_SIZE(p, v, type)					\
    SET_BLOB_FIELD(p, v, type, size)

#define	SET_BLOB_FILE_ID(p, v, type)					\
    SET_BLOB_FIELD(p, v, type, file_id)

#define	SET_BLOB_SDB_ID(p, v, type)					\
    SET_BLOB_FIELD(p, v, type, sdb_id)

/*
 * Page space required to add a new HBLOB item to the page, with and
 * without the index value.
 */
#define	HBLOB_SIZE		(sizeof(HBLOB))
#define	HBLOB_DSIZE		(sizeof(HBLOB) - SSZA(HKEYDATA, data))
#define	HBLOB_PSIZE		(HBLOB_SIZE + sizeof(db_indx_t))


/************************************************************************
 BTREE PAGE LAYOUT
 ************************************************************************/

/* Each index references a group of bytes on the page. */
#define	B_KEYDATA	1	/* Key/data item. */
#define	B_DUPLICATE	2	/* Duplicate key/data item. */
#define	B_OVERFLOW	3	/* Overflow key/data item. */
#define	B_BLOB		4	/* Blob file key/data item. */

/*
 * We have to store a deleted entry flag in the page.   The reason is complex,
 * but the simple version is that we can't delete on-page items referenced by
 * a cursor -- the return order of subsequent insertions might be wrong.  The
 * delete flag is an overload of the top bit of the type byte.
 */
#define	B_DELETE	(0x80)
#define	B_DCLR(t)	(t) &= ~B_DELETE
#define	B_DSET(t)	(t) |= B_DELETE
#define	B_DISSET(t)	((t) & B_DELETE)

#define	B_TYPE(t)		((t) & ~B_DELETE)
#define	B_TSET(t, type)	((t) = B_TYPE(type))
#define	B_TSET_DELETED(t, type) ((t) = (type) | B_DELETE)

/*
 * The first type is B_KEYDATA, represented by the BKEYDATA structure:
 */
typedef struct _bkeydata {
	db_indx_t len;		/* 00-01: Key/data item length. */
	u_int8_t  type;		/*    02: Page type AND DELETE FLAG. */
	u_int8_t  data[1];	/* Variable length key/data item. */
} BKEYDATA;

/* Get a BKEYDATA item for a specific index. */
#define	GET_BKEYDATA(dbp, pg, indx)					\
	((BKEYDATA *)P_ENTRY(dbp, pg, indx))

/*
 * Page space required to add a new BKEYDATA item to the page, with and
 * without the index value.  The (u_int16_t) cast avoids warnings: DB_ALIGN
 * casts to uintmax_t, the cast converts it to a small integral type so we
 * don't get complaints when we assign the final result to an integral type
 * smaller than uintmax_t.
 */
#define	BKEYDATA_SIZE(len)						\
	(u_int16_t)DB_ALIGN((len) + SSZA(BKEYDATA, data), sizeof(u_int32_t))
#define	BKEYDATA_PSIZE(len)						\
	(BKEYDATA_SIZE(len) + sizeof(db_indx_t))

/*
 * The second and third types are B_DUPLICATE and B_OVERFLOW, represented
 * by the BOVERFLOW structure.
 */
typedef struct _boverflow {
	db_indx_t unused1;	/* 00-01: Padding, unused. */
	u_int8_t  type;		/*    02: Page type AND DELETE FLAG. */
	u_int8_t  unused2;	/*    03: Padding, unused. */
	db_pgno_t pgno;		/* 04-07: Next page number. */
	u_int32_t tlen;		/* 08-11: Total length of item. */
} BOVERFLOW;

/*
 * The fourth type is the B_BLOB, represented by the BBLOB structure.
 * Saving bytes is not a concern for the blob record type - if too many
 * fit onto a single page, then we're likely to introduce unnecessary
 * contention for blobs. Using blobs implies storing large items, thus slightly
 * more per-item overhead is acceptable.
 * The len field is set to BBLOB_DSIZE, so that a B_BLOB can be treated just
 * like a B_KEYDATA for the purposes of moving items between or on a page.
 * If this proves untrue, the crypto section of the record could be optional.
 * encoding, lsn, encryption, and checksum fields are unused at the moment, but
 * included to make adding those features easier.
 */
typedef struct _bblob {
	db_indx_t len;			/* 00-01: BBLOB_DSIZE. */
	u_int8_t  type;			/*    02: Page type and delete flag. */
	u_int8_t  encoding;		/*    03: Encoding of blob file. */
	u_int8_t  unused[8];		/* 04-11: Padding, unused. */
	u_int8_t  chksum[DB_MAC_KEY];	/* 12-31: Checksum */
	u_int8_t  iv[DB_IV_BYTES];	/* 32-47: IV */
	u_int64_t id;			/* 48-55: Blob file identifier. */
	u_int64_t size;			/* 56-63: Blob file size. */
	u_int64_t file_id;		/* 64-71: File directory. */
	u_int64_t sdb_id;		/* 72-79: Subdb that owns this blob. */
} BBLOB, BBLOB60P1;
#define	BBLOB_DATA(p)	((u_int8_t *)((BKEYDATA *)p)->data)

/* Get a BOVERFLOW item for a specific index. */
#define	GET_BOVERFLOW(dbp, pg, indx)					\
	((BOVERFLOW *)P_ENTRY(dbp, pg, indx))

/*
 * Page space required to add a new BOVERFLOW item to the page, with and
 * without the index value.
 */
#define	BOVERFLOW_SIZE							\
	((u_int16_t)DB_ALIGN(sizeof(BOVERFLOW), sizeof(u_int32_t)))
#define	BOVERFLOW_PSIZE							\
	(BOVERFLOW_SIZE + sizeof(db_indx_t))

/*
 * Page space required to add a new BBLOB item to the page, with and
 * without the index value.  BBLOB_DSIZE is used so that a B_BLOB item
 * can be treated just like a B_KEYDATA for the purposes of moving items
 * between or on a page, such as when doing compaction.
 */
#define	BBLOB_SIZE							\
	((u_int16_t)DB_ALIGN(sizeof(BBLOB), sizeof(u_int32_t)))
#define	BBLOB_DSIZE							\
	(BBLOB_SIZE - SSZA(BKEYDATA, data))
#define	BBLOB_PSIZE							\
	(BBLOB_SIZE + sizeof(db_indx_t))

#define	BITEM_SIZE(bk)							\
	(B_TYPE((bk)->type) == B_KEYDATA ? BKEYDATA_SIZE((bk)->len) :	\
	(B_TYPE((bk)->type) == B_BLOB ? BBLOB_SIZE : BOVERFLOW_SIZE))

#define	BITEM_PSIZE(bk)							\
	(B_TYPE((bk)->type) == B_KEYDATA ? BKEYDATA_PSIZE((bk)->len) :	\
	(B_TYPE((bk)->type) == B_BLOB ? BBLOB_PSIZE : BOVERFLOW_PSIZE))

/*
 * Btree leaf and hash page layouts group indices in sets of two, one for the
 * key and one for the data.  Everything else does it in sets of one to save
 * space.  Use the following macros so that it's real obvious what's going on.
 */
#define	O_INDX	1
#define	P_INDX	2

/************************************************************************
 BTREE INTERNAL PAGE LAYOUT
 ************************************************************************/

/*
 * Btree internal entry.
 */
typedef struct _binternal {
	db_indx_t  len;		/* 00-01: Key/data item length. */
	u_int8_t   type;	/*    02: Page type AND DELETE FLAG. */
	u_int8_t   unused;	/*    03: Padding, unused. */
	db_pgno_t  pgno;	/* 04-07: Page number of referenced page. */
	db_recno_t nrecs;	/* 08-11: Subtree record count. */
	u_int8_t   data[1];	/* Variable length key item. */
} BINTERNAL;

/* Get a BINTERNAL item for a specific index. */
#define	GET_BINTERNAL(dbp, pg, indx)					\
	((BINTERNAL *)P_ENTRY(dbp, pg, indx))

/*
 * Page space required to add a new BINTERNAL item to the page, with and
 * without the index value.
 */
#define	BINTERNAL_SIZE(len)						\
	(u_int16_t)DB_ALIGN((len) + SSZA(BINTERNAL, data), sizeof(u_int32_t))
#define	BINTERNAL_PSIZE(len)						\
	(BINTERNAL_SIZE(len) + sizeof(db_indx_t))

/************************************************************************
 RECNO INTERNAL PAGE LAYOUT
 ************************************************************************/

/*
 * The recno internal entry.
 */
typedef struct _rinternal {
	db_pgno_t  pgno;	/* 00-03: Page number of referenced page. */
	db_recno_t nrecs;	/* 04-07: Subtree record count. */
} RINTERNAL;

/* Get a RINTERNAL item for a specific index. */
#define	GET_RINTERNAL(dbp, pg, indx)					\
	((RINTERNAL *)P_ENTRY(dbp, pg, indx))

/*
 * Page space required to add a new RINTERNAL item to the page, with and
 * without the index value.
 */
#define	RINTERNAL_SIZE							\
	(u_int16_t)DB_ALIGN(sizeof(RINTERNAL), sizeof(u_int32_t))
#define	RINTERNAL_PSIZE							\
	(RINTERNAL_SIZE + sizeof(db_indx_t))

typedef struct __pglist {
	db_pgno_t pgno, next_pgno;
	DB_LSN lsn;
} db_pglist_t;

#if defined(__cplusplus)
}
#endif

#endif /* !_DB_PAGE_H_ */
