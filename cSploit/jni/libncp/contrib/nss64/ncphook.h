#ifndef __NCP_HOOK_H__
#define __NCP_HOOK_H__

#include <ntypes.h>
#include <nwtypes.h>

typedef unsigned long long nuint64;

LONG AllocateResourceTag(LONG NLMHandle, const unsigned char* description, LONG resourceType);
unsigned int GetNLMHandle(void);
void* Alloc(LONG len, nuint32 rTag);

#define PACKED	__attribute__((packed))

#define NCPVerbRTag	0x5650434E

struct ncpReqInfo {
	nuint8	reserved1[33]	PACKED;
	nuint16	conn		PACKED;
	nuint8	task		PACKED;
	nuint8	request[1]	PACKED;
};

struct ncpConnInfo {
	nuint8	reserved1[0x86]	PACKED;
	nuint32 bytesRead	PACKED;
	nuint32	bytesWritten	PACKED;
};

struct ncpSend {
	void	(*ReplyKeep)(struct ncpReqInfo*, nuint32 error, nuint32 fragCount, void* addr, nuint32 len, ...);
	void	(*ReplyDisgard)(struct ncpReqInfo*, nuint32 error, nuint32 fragCount, void* addr, nuint32 len, ...);
	void	(*ReplyKeepNoFragments)(struct ncpReqInfo*, nuint32 error);
	void*	(*GetReplyKeepBuffer)(struct ncpReqInfo*);
	void	(*ReplyKeepBufferFilledOut)(struct ncpReqInfo*, nuint32 len);
	void	(*ReplyKeepNoFragmentsWithStation)(nuint32 conn, nuint32 error);
	void	(*ReplyUsingAllocBuffer)(struct ncpReqInfo*, nuint32 error, void* base, nuint32 len);
	void	(*ReplyKeepWithBufferAndFreePtr)(struct ncpReqInfo*, nuint32 error, void* toFree, nuint32 fragCount, void* addr, nuint32 len, ...);
	void	(*ReplyReleaseWithFragments)(nuint32 conn, struct ncpReqInfo*, void (*fn)());
};

nuint32 HookNCPVerb(nuint32 rTag, nuint32 ncpVerb, void (*ncpFunc)(struct ncpReqInfo*, struct ncpSend*, nuint32, nuint32 reqLen,
		void* workspace, nuint32 workspaceLen));

nuint32 ReleaseNCPVerb(nuint32 rTag, nuint32 ncpVerb);

struct cacheblk {
	void*	address		PACKED;
	nuint8	y4[0x7B-0x04]	PACKED;
	nuint8	pinCount	PACKED;
};

void cacheUnpinned(struct cacheblk*);

struct ncpSendCacheCB {
	nuint8	y0[0x24-0x00]	PACKED;
	nuint32	connNum		PACKED;
};

struct largeReadControlFrag {
	nuint8			y0[0x04-0x00]	PACKED;
	void*			fragAddr	PACKED;
	struct cacheblk*	cacheBlock	PACKED;
	nuint32			fragLen		PACKED;
	nuint8			y10[0x14-0x10]	PACKED;
	nuint8			buffer[12]	PACKED;
};

struct largeReadControl {
	struct ncpReqInfo*	info		PACKED;
	nuint32			connNum		PACKED;
	struct ncpSend*		send		PACKED;
	nuint8			y0C[0x14-0x0C]	PACKED;
	nuint32			fragCount	PACKED;
	nuint32			y18		PACKED;
	struct largeReadControlFrag frags[0]	PACKED;
};

struct semaphore {
	void*	queue	PACKED;
	int	value	PACKED;
	void*	data	PACKED;
};

struct nxt {
	struct nxt*	next	PACKED;
};

struct nssVolume {
	nuint8	y[0x168-0x00]	PACKED;
	nuint32	flags		PACKED;
};

struct fh2 {
	nuint8			y0[0x08-0x00]	PACKED;
	unsigned long long	file_size	PACKED;
	nuint8			y10[0x14-0x10]	PACKED;
	nuint8			cacheShift	PACKED;
	nuint8			y15[0x1C-0x15]	PACKED;
	nuint32			opsCount	PACKED;
	nuint8			y20[0x38-0x20]	PACKED;
	struct fh2*		altFH		PACKED;
	nuint8			y3C[0x44-0x3C]	PACKED;
	struct nssVolume*	volume		PACKED;
	nuint8			y48[0x4C-0x48]	PACKED;
	nuint32			netwareFH	PACKED;
	nuint8			y50[0x60-0x50]	PACKED;
	struct semaphore	sema		PACKED;
	nuint8			y6C[0x94-0x6C]	PACKED;
	nuint8			x94		PACKED;
	nuint8			y95[0xD8-0x95]	PACKED;
	struct nxt		locks		PACKED;
};

struct fh {
	nuint8		y0[0x0C-0x00]		PACKED;
	struct fh2*	nssFH			PACKED;
	nuint8		y10[0x24-0x10]		PACKED;
	nuint32		accessMode		PACKED;
	nuint8		y28[0x2C-0x28]		PACKED;
	nuint8		x2C			PACKED;
	nuint8		x2D			PACKED;
	nuint8		y2E[0x38-0x2E]		PACKED;
	nuint32		nextReadAheadPage	PACKED;
};

struct zComnInfo {
	nuint32			errorcode	PACKED;
	nuint32			connection	PACKED;
	struct ncpConnInfo*	conn		PACKED;
	nuint32			reserved2	PACKED;
	nuint32			reserved3	PACKED;
};

struct zComnPosition {
	unsigned long long	offset		PACKED;
	nuint32			length		PACKED;
	void*			buffer		PACKED;
	nuint32			reserved1	PACKED;
	nuint32			written		PACKED;
};

struct zComnFile {
	nuint32		fileHandle	PACKED;
	struct fh*	nssHandle	PACKED;
};

struct nwconnection {
	nuint8				y0[0x180]	PACKED;
	nuint32				maxReadCacheBuf	PACKED;
	struct largeReadControl*	largeReadControl PACKED;
	nuint32				ncpDataSize	PACKED;
};

struct LB_async {
	nuint8				y0[0x0C-0x00]	PACKED;
	struct semaphore*		sema		PACKED;
	nuint8				y10[0x3C-0x10]	PACKED;
	nuint32				errorCode	PACKED;
	nuint32*			netwareFH	PACKED;
	struct cacheblk*		cacheBlock	PACKED;
	nuint8				y48[0x4C-0x48]	PACKED;
	nuint32				pageNumber	PACKED;
	nuint32				x50		PACKED;
	nuint8				x54		PACKED;
	nuint8				y55[0x6C-0x55]	PACKED;
	struct largeReadControlFrag*	frag		PACKED;
};

struct {
	nuint8	y0[0x54-0x00]		PACKED;
	nuint32	readYieldRequests	PACKED;
	nuint8	y58[0x5C-0x58]		PACKED;
	nuint32 readRequests		PACKED;
	nuint32	writeRequests		PACKED;
	nuint32	readBytes		PACKED;
	nuint32	writtenBytes		PACKED;
} Inst;

extern struct nwconnection* (**NW_connectionTable)[];
extern struct semaphore ReserveResource;
extern int PeriodicYieldCount;

extern void LBL_sSignal(struct semaphore*);
extern void LBL_sWait(struct semaphore*);
extern void LBL_cntSignal(struct semaphore*, int);
extern void LBL_cntWait(struct semaphore*, int);
extern void LBL_xSignal(struct semaphore*);
extern void LBL_xWait(struct semaphore*);

extern void mailInterrupt(void (*)(struct cacheblk*), struct cacheblk*);
extern struct cacheblk* fastReadCache(nuint32*, nuint32 block);
extern void asyncReadFileBlk(struct LB_async*, void (*)(struct LB_async*));
extern void cacheRelease(struct cacheblk*);
extern void asyncReadAhead(struct fh*);

extern void SetErrno(struct zComnInfo*, nuint32 nssErrorCode);

extern void CYieldIfNeeded(void);

extern void LB_freeAsyncio(struct LB_async*);
extern struct LB_async* LB_getAsyncio(void);

extern void COMN_Write(struct zComnInfo*, struct zComnPosition*, struct zComnFile*);
extern void COMN_SetDataSize(struct zComnInfo*, struct zComnFile*, unsigned long long offs, nuint32 flags);
extern struct fh* COMN_DoResolveFileHandle(struct zComnInfo*, struct zComnFile*);
extern void COMN_DoReleaseFileHandleIDP(struct zComnInfo*, struct zComnFile*);
extern int COMN_IsSharedByteRange(struct zComnInfo*, struct fh2*, unsigned long long offs, unsigned long long len);
extern int COMN_ReadSnapOrDontCopyToSnap(struct fh2*, unsigned int page);
extern void COMN_Release(struct fh2**);

#define zRR_READ_ACCESS			0x00000001

#define zERR_HARD_READ_ERROR		20101
#define zERR_NO_READ_PRIVILEGE		20861
#define zERR_IOLOCK_ERROR		20900

#define zATTR_COW			0x00040000

#define NCP_NSS64Verb		0xE5

#endif	/* __NCP_HOOK_H__ */

