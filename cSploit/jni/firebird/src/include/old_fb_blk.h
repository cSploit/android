#ifndef INCLUDE_FB_BLK
#define INCLUDE_FB_BLK

#define INCLUDE_OLD_FB_BLK


typedef struct blk
{
	UCHAR	blk_type;
	UCHAR	blk_pool_id;
	ULONG	blk_length;
} *BLK;


#endif	/* INCLUDE_FB_BLK */

