#include "ncphook.h"
#include <stddef.h>
#include <signal.h>
#include <nwadv.h>
#include <nwthread.h>
#include <string.h>
#include <stdio.h>

static unsigned char NSS2NCPErrorTable[5000];

static void InitNSS2NCPErrorTable(void) {
	memset(NSS2NCPErrorTable, 0xFF, sizeof(NSS2NCPErrorTable));
	
	NSS2NCPErrorTable[    0] = 0x96;
	NSS2NCPErrorTable[    1] = 0xFD;
	NSS2NCPErrorTable[    2] = 0xFD;
	NSS2NCPErrorTable[    4] = 0x77;
	NSS2NCPErrorTable[    7] = 0xFB;
	NSS2NCPErrorTable[   99] = 0xFC;
	NSS2NCPErrorTable[  101] = 0x83;
	NSS2NCPErrorTable[  102] = 0x83;
	NSS2NCPErrorTable[  103] = 0x01;
	NSS2NCPErrorTable[  104] = 0x01;
	NSS2NCPErrorTable[  300] = 0x9C;
	NSS2NCPErrorTable[  400] = 0x9C;
	NSS2NCPErrorTable[  401] = 0x88;
	NSS2NCPErrorTable[  402] = 0x9B;
	NSS2NCPErrorTable[  404] = 0x9E;
	NSS2NCPErrorTable[  405] = 0x9C;
	NSS2NCPErrorTable[  406] = 0x9C;
	NSS2NCPErrorTable[  408] = 0x9C;
	NSS2NCPErrorTable[  409] = 0x9C;
	NSS2NCPErrorTable[  410] = 0x9C;
	NSS2NCPErrorTable[  411] = 0x98;
	NSS2NCPErrorTable[  417] = 0xA0;
	NSS2NCPErrorTable[  421] = 0x01;
	NSS2NCPErrorTable[  438] = 0x82;
	NSS2NCPErrorTable[  439] = 0x9D;
	NSS2NCPErrorTable[  440] = 0x9C;
	NSS2NCPErrorTable[  444] = 0x9C;
	NSS2NCPErrorTable[  445] = 0x2D;
	NSS2NCPErrorTable[  499] = 0x9C;
	NSS2NCPErrorTable[  500] = 0x8E;
	NSS2NCPErrorTable[  501] = 0x8D;
	NSS2NCPErrorTable[  502] = 0x90;
	NSS2NCPErrorTable[  503] = 0x8F;
	NSS2NCPErrorTable[  504] = 0x92;
	NSS2NCPErrorTable[  505] = 0x91;
	NSS2NCPErrorTable[  506] = 0x8B;
	NSS2NCPErrorTable[  507] = 0xA4;
	NSS2NCPErrorTable[  508] = 0x9A;
	NSS2NCPErrorTable[  550] = 0xBE;
	NSS2NCPErrorTable[  601] = 0xCF;
	NSS2NCPErrorTable[  650] = 0x17;
	NSS2NCPErrorTable[  651] = 0x11;
	NSS2NCPErrorTable[  652] = 0x18;
	NSS2NCPErrorTable[  653] = 0x95;
	NSS2NCPErrorTable[  654] = 0x95;
	NSS2NCPErrorTable[  700] = 0xBF;
	NSS2NCPErrorTable[  701] = 0xBF;
	NSS2NCPErrorTable[  702] = 0xBF;
	NSS2NCPErrorTable[  703] = 0xBF;
	NSS2NCPErrorTable[  800] = 0x98;
	NSS2NCPErrorTable[  801] = 0x98;
	NSS2NCPErrorTable[  804] = 0x78;
	NSS2NCPErrorTable[  850] = 0x8C;
	NSS2NCPErrorTable[  851] = 0x84;
	NSS2NCPErrorTable[  856] = 0xFE;
	NSS2NCPErrorTable[  857] = 0x9C;
	NSS2NCPErrorTable[  859] = 0xA8;
	NSS2NCPErrorTable[  860] = 0x94;
	NSS2NCPErrorTable[  861] = 0x93;
	NSS2NCPErrorTable[  862] = 0x8A;
	NSS2NCPErrorTable[  863] = 0x8A;
	NSS2NCPErrorTable[  867] = 0xFC;
	NSS2NCPErrorTable[  868] = 0x8E;
	NSS2NCPErrorTable[  869] = 0x85;
	NSS2NCPErrorTable[  870] = 0x84;
	NSS2NCPErrorTable[  900] = 0xA2;
	NSS2NCPErrorTable[  901] = 0x80;
	NSS2NCPErrorTable[  903] = 0xFE;
	NSS2NCPErrorTable[  905] = 0x80;
	NSS2NCPErrorTable[  906] = 0x80;
	NSS2NCPErrorTable[  907] = 0x80;
	NSS2NCPErrorTable[  908] = 0x80;
	NSS2NCPErrorTable[ 1303] = 0x01;
	NSS2NCPErrorTable[ 1503] = 0x01;
	NSS2NCPErrorTable[ 2000] = 0xA5;
}

static LONG NSS2NCPError(nuint32 ec) {
	if (ec) {
		if (ec < 20000 || ec >= 20000 + sizeof(NSS2NCPErrorTable)) {
			return 0xFF;
		}
		return NSS2NCPErrorTable[ec - 20000];
	}
	return 0;
}

static nuint32 nlmHandle;
static nuint32 NCPRTag;
//static nuint32 AllocRTag;
static int ncpHooked = 0;

static inline void cachePut(struct cacheblk* cacheBlock) {
	if (!--cacheBlock->pinCount) {
		cacheUnpinned(cacheBlock);
	}
}

static inline void releaseReadReply(struct largeReadControl* lrc) {
	struct largeReadControlFrag* lrcf;
	unsigned int i;

	lrcf = lrc->frags + 1;
	for (i = lrc->fragCount; --i; ) {
		struct cacheblk* cacheBlock;

		cacheBlock = lrcf->cacheBlock;
		if (cacheBlock) {
			cachePut(cacheBlock);
		}
		lrcf++;
	}
	return;
}

static void sendDone(struct ncpSendCacheCB* sendCb) {
	struct largeReadControlFrag* lrcf;
	struct largeReadControl* lrc;
	unsigned int i;

	lrc = (**NW_connectionTable)[sendCb->connNum]->largeReadControl;
	lrcf = lrc->frags + 1;
	for (i = lrc->fragCount; --i; ) {
		mailInterrupt(cachePut, lrcf->cacheBlock);
		lrcf++;
	}
	return;
}
	
static inline void __LBL_sSignal(struct semaphore* sem) {
	if (!--sem->value && sem->queue) {
		LBL_sSignal(sem);
	}
}

static inline void readAsyncCallback(struct LB_async* req) {
	struct largeReadControlFrag* lrcf;

	lrcf = req->frag;
	if (req->errorCode) {
		lrcf->fragAddr = NULL;
		lrcf->cacheBlock = NULL;
		SetErrno(req->sema->data, req->errorCode);
	} else {
		struct cacheblk* cacheBlock = req->cacheBlock;

		cacheBlock->pinCount++;
		cacheRelease(cacheBlock);
		lrcf->fragAddr = cacheBlock->address + (nuint32)lrcf->fragAddr;
		lrcf->cacheBlock = cacheBlock;
	}
	__LBL_sSignal(req->sema);
	LB_freeAsyncio(req);
}

static inline void __LBL_sWait(struct semaphore* sem) {
	if (sem->value) {
		if (sem->value < 0 || sem->queue) {
			LBL_sWait(sem);
		} else {
			sem->value++;
		}
	} else {
		sem->value = 1;
	}
}

static inline void __LBL_cntSignal(struct semaphore* sem, int val) {
	sem->value += val;
	if (sem->queue) {
		LBL_cntSignal(sem, val);
	}
}

static inline void __LBL_cntWait(struct semaphore* sem, int val) {
	if (sem->value < val) {
		LBL_cntWait(sem, val);
	} else {
		sem->value -= val;
	}
}

static inline int __LBL_xIsLocked(struct semaphore* sem) {
	return sem->value;
}

static inline void __LBL_xSignal(struct semaphore* sem) {
	sem->value = 0;
	if (sem->queue) {
		LBL_xSignal(sem);
	}
}

static inline void __LBL_xWait(struct semaphore* sem) {
	if (sem->value) {
		LBL_xWait(sem);
	} else {
		sem->value = -1;
	}
}

static nuint32 nssReadEnter;
static nuint32 nssReadExit;
static nuint32 nssGetSizeEnter;
static nuint32 nssGetSizeExit;

static inline unsigned long long shr(unsigned long long val, unsigned int shift) {
	return val >> shift;
}

void Case72(struct ncpReqInfo* info, struct ncpSend* send, nuint32 unk, nuint32 reqLen,
		void* workspace, nuint32 workspaceLen);

struct fhOffsLen {
	nuint16	filehandle_l	PACKED;
	nuint32 filehandle_h	PACKED;
	nuint16 rsvd		PACKED;
	unsigned long long	offset	PACKED;
	unsigned long long	length	PACKED;
	nuint8  data[1];
};

struct fhReq {
	nuint16 filehandle_l	PACKED;
	nuint32 filehandle_h	PACKED;
	nuint16 rsvd		PACKED;
};

static void NWSARead(struct ncpReqInfo* info, struct ncpSend* send, struct fhOffsLen* rq,
		void* workspace, nuint32 workspaceLen) {
	struct fh* nssFD;
	nuint32 connNum;
	struct zComnFile zFile;
	struct zComnInfo zInfo;
	
	nssReadEnter++;
	zFile.fileHandle = rq->filehandle_h;
	zFile.nssHandle = 0;
	Inst.readRequests++;
	connNum = info->conn;
	zInfo.errorcode = 0;
	zInfo.connection = connNum;
	zInfo.conn = NULL;
	zInfo.reserved2 = 0;
	zInfo.reserved3 = 2;
	__LBL_cntWait(&ReserveResource, 32);
	if (zFile.nssHandle == NULL) {
		nssFD = COMN_DoResolveFileHandle(&zInfo, &zFile);
	} else {
		nssFD = zFile.nssHandle;
	}
	if (nssFD) {
		if ((nssFD->accessMode & zRR_READ_ACCESS) == 0) {
			SetErrno(&zInfo, zERR_NO_READ_PRIVILEGE);
		} else {
			unsigned long long file_offset;
			nuint32 read_length;
			unsigned long long file_size;
			unsigned long long requestEnd;
			unsigned long long pageNumber;
			nuint32 firstByte;
			struct largeReadControl* lrc;
			nuint32 was_read;
			struct largeReadControlFrag* lrcf_base;
			nuint8 cacheShift;
			struct nwconnection* conn;
			nuint32 pagesRead;
			nuint32 cacheBlockSize;
			struct fh2* nssFH2;
			struct fh2* nssFH1;
			struct semaphore readDoneSema;
			struct nxt* fh;
			struct largeReadControlFrag* lrcf;
			nuint32 readBytes;

			file_offset = rq->offset;
			read_length = rq->length > 65536 ? 65536 : rq->length;

			nssFH1 = nssFD->nssFH;

			__LBL_sWait(&nssFH1->sema);
			if (((nssFH1->volume->flags & zATTR_COW) == 0) || ((nssFD->x2C & 4) == 0) || ((nssFH2 = nssFH1->altFH) == NULL)) {
				nssFH2 = NULL;
				file_size = nssFH1->file_size;
			} else {
				__LBL_sWait(&nssFH2->sema);
				file_size = nssFH2->file_size;
			}
			if (file_size <= file_offset) {
				__LBL_cntSignal(&ReserveResource, 32);
				if (nssFH2) {
					__LBL_sSignal(&nssFH2->sema);
				}
				__LBL_sSignal(&nssFH1->sema);
				*(nuint64*)workspace = 0;
				send->ReplyKeep(info, 0, 1, workspace, 8);
				goto quit;
			}
			if (file_size < file_offset + read_length) {
				read_length = file_size - file_offset;
			}
			cacheShift = nssFH1->cacheShift;
			cacheBlockSize = 1 << cacheShift;
			firstByte = file_offset & (cacheBlockSize - 1);
//			asm __volatile__ ("  int $3\n");
			pageNumber = shr(file_offset, cacheShift);
			readBytes = cacheBlockSize - firstByte;
			requestEnd = file_offset + read_length - 1;
			requestEnd = shr(requestEnd, cacheShift) - pageNumber;
			conn = (**NW_connectionTable)[zInfo.connection];
			if (requestEnd > conn->maxReadCacheBuf) {
				read_length = (conn->maxReadCacheBuf - 1) << cacheShift;
			}
			if (read_length > conn->ncpDataSize - 8) {
				read_length = conn->ncpDataSize - 8;
			}
			fh = &nssFH1->locks;
			if (fh->next != fh) {
				if (COMN_IsSharedByteRange(&zInfo, nssFH1, file_offset, read_length)) {
					SetErrno(&zInfo, zERR_IOLOCK_ERROR);
					if (nssFH2) {
						__LBL_sSignal(&nssFH2->sema);
					}
					__LBL_sSignal(&nssFH1->sema);
					goto errexit;
				}
			}
			lrc = conn->largeReadControl;
			lrc->info = info;
			lrc->connNum = connNum;
			lrc->send = send;
			lrcf_base = lrc->frags;
			lrcf_base->cacheBlock = NULL;
			lrcf_base->fragLen = 8;
			lrcf_base->fragAddr = lrcf_base->buffer;
			*(unsigned long long*)(lrcf_base->buffer) = read_length;
			was_read = 0;
			readDoneSema.queue = 0;
			readDoneSema.value = 0;
			readDoneSema.data = &zInfo;
			pagesRead = 0;
			lrcf = lrcf_base + 1;
			while (1) {
				struct fh2* nssFH;
				struct cacheblk* cacheBlock;

				if (readBytes > read_length) {
					readBytes = read_length;
				}
				if (!--PeriodicYieldCount) {
					++Inst.readYieldRequests;
					PeriodicYieldCount = 16;
					CYieldIfNeeded();
				}
				if (nssFH2 && COMN_ReadSnapOrDontCopyToSnap(nssFH1, pageNumber)) {
					nssFH = nssFH2;
				} else {
			    		nssFH = nssFH1;
				}
				cacheBlock = fastReadCache(&nssFH->netwareFH, pageNumber);
				if (cacheBlock) {
					cacheBlock->pinCount++;
					cacheRelease(cacheBlock);
					lrcf->cacheBlock = cacheBlock;
					lrcf->fragAddr = cacheBlock->address + firstByte;
				} else {
					struct LB_async* req;

					readDoneSema.value++;
					req = LB_getAsyncio();
					req->errorCode = 0;
					req->netwareFH = &nssFH->netwareFH;
					req->cacheBlock = NULL;
					req->x50 = 0;
					req->x54 = 0;
					req->frag = lrcf;
					req->sema = &readDoneSema;
					req->pageNumber = pageNumber;
					lrcf->fragAddr = (void*)firstByte;
					asyncReadFileBlk(req, readAsyncCallback);
				}
				lrcf->fragLen = readBytes;
				lrcf++;
				was_read += readBytes;
				read_length -= readBytes;
				if (read_length == 0) {
					break;
				}
				pageNumber++;
				pagesRead++;
				firstByte = 0;
				readBytes = 1 << nssFH1->x94;
			}
			lrc->fragCount = pagesRead + 2;
			++nssFH1->opsCount;
			if (__LBL_xIsLocked(&readDoneSema)) {
				__LBL_xWait(&readDoneSema);
				__LBL_xSignal(&readDoneSema);
			}
			__LBL_cntSignal(&ReserveResource, 32);
			if (nssFH2) {
				__LBL_sSignal(&nssFH2->sema);
			}
			__LBL_sSignal(&nssFH1->sema);
			if (zInfo.errorcode) {
				COMN_Release(&nssFH1);
				/* __LBL_cntSignal(&ReserveResource, 32); */
				releaseReadReply(lrc);
				send->ReplyKeepNoFragments(info, NSS2NCPError(zERR_HARD_READ_ERROR));
				goto quit;
			}
			Inst.readBytes += was_read;
			if (zInfo.conn) {
				zInfo.conn->bytesRead += was_read;
			}
//			asm __volatile__ ("	int $3\n");
			send->ReplyReleaseWithFragments(connNum, info, sendDone);
			if (pagesRead == pageNumber - nssFD->nextReadAheadPage) {
				if (pagesRead) {
					nssFD->x2D >>= pagesRead;
					nssFD->nextReadAheadPage = pageNumber + 1;
				}
				if (nssFD->x2D == 0) {
					asyncReadAhead(nssFD);
				}
			}
			COMN_Release(&nssFH1);
			goto quit;
		}
	}
errexit:;		
	__LBL_cntSignal(&ReserveResource, 32);
	send->ReplyKeepNoFragments(info, NSS2NCPError(zInfo.errorcode));
quit:;	
	COMN_DoReleaseFileHandleIDP(&zInfo, &zFile);
	nssReadExit++;
	return;
}

static void NWSAGetSize(struct ncpReqInfo* info, struct ncpSend* send, struct fhReq* rq,
		void* workspace, nuint32 workspaceLen) {
	struct fh* nssFD;
	nuint32 connNum;
	struct zComnFile zFile;
	struct zComnInfo zInfo;
	
	nssGetSizeEnter++;
	zFile.fileHandle = rq->filehandle_h;
	zFile.nssHandle = 0;
	connNum = info->conn;
	zInfo.errorcode = 0;
	zInfo.connection = connNum;
	zInfo.conn = NULL;
	zInfo.reserved2 = 0;
	zInfo.reserved3 = 2;
	__LBL_cntWait(&ReserveResource, 32);
	if (zFile.nssHandle == NULL) {
		nssFD = COMN_DoResolveFileHandle(&zInfo, &zFile);
	} else {
		nssFD = zFile.nssHandle;
	}
	if (nssFD) {
		struct fh2* nssFH2;
		struct fh2* nssFH1;

		nssFH1 = nssFD->nssFH;

		__LBL_sWait(&nssFH1->sema);
		if (((nssFH1->volume->flags & zATTR_COW) == 0) || ((nssFD->x2C & 4) == 0) || ((nssFH2 = nssFH1->altFH) == NULL)) {
			nssFH2 = NULL;
			*(nuint64*)workspace = nssFH1->file_size;
		} else {
			__LBL_sWait(&nssFH2->sema);
			*(nuint64*)workspace = nssFH2->file_size;
		}
		__LBL_cntSignal(&ReserveResource, 32);
		if (nssFH2) {
			__LBL_sSignal(&nssFH2->sema);
		}
		__LBL_sSignal(&nssFH1->sema);
		send->ReplyKeep(info, 0, 1, workspace, 8);
	} else {
		__LBL_cntSignal(&ReserveResource, 32);
		send->ReplyKeepNoFragments(info, NSS2NCPError(zInfo.errorcode));
	}
	COMN_DoReleaseFileHandleIDP(&zInfo, &zFile);
	nssGetSizeExit++;
	return;
}

static void nss64NCP(struct ncpReqInfo* i, struct ncpSend* ncpsend, nuint32 unknown, nuint32 reqLen,
		void* workspace, nuint32 workspaceLen) {
	nuint32 err;
	struct nwconnection* conn;
		
	conn = (**NW_connectionTable)[i->conn];
	/* Why this happens?! Netware's NCP write suffers from this bug too - you can force it to
	   write random parts of memory to disk by issuing write command which says that
	   60KB of data is going to be written... MTU..60000 bytes will contain random garbage */
	/* We prevent this by truncating request */
	err = 0x7E;
//	asm __volatile__ ("  int $3\n");
	if (reqLen < offsetof(struct ncpReqInfo, request) + 2) {
		goto errquit;
	}
	reqLen -= offsetof(struct ncpReqInfo, request) - 2;
	if (reqLen > conn->ncpDataSize - 8) {
		if (i->request[1] == 2) {
			reqLen = conn->ncpDataSize - 8;
		} else {
			goto errquit;
		}
	}
	switch (i->request[1]) {
		case 1:
			if (reqLen < 8 + 8 + 8) {
				break;
			}
			{
				struct fhOffsLen* rq = (struct fhOffsLen*)(i->request + 2);

				err = 0xFF;
				if (rq->rsvd != 0) {
					break;
				}
				err = 0x84;
				if (((rq->filehandle_l - 1U) & 0xFFFF) != (rq->filehandle_h & 0xFFFF)) {
					break;
				}
				if (!(rq->filehandle_h & 0x80000000U)) {
					/* only NSS handles allowed... We have to do some fallback here... */
					break;
				}
				NWSARead(i, ncpsend, rq, workspace, workspaceLen);
			}
			return;
		case 2:
			if (reqLen < 8 + 8 + 8) {
				break;
			}
			reqLen -= 24;
			{
				struct zComnInfo info;
				struct zComnFile file;
				struct zComnPosition pos;
				struct fhOffsLen* rq = (struct fhOffsLen*)(i->request + 2);

//				asm __volatile__ ("	int $3\n");

				if (rq->length > reqLen) {
					rq->length = reqLen;
				}
				err = 0xFF;
				if (rq->rsvd != 0) {
					break;
				}
				err = 0x84;
				if (((rq->filehandle_l - 1U) & 0xFFFF) != (rq->filehandle_h & 0xFFFF)) {
					break;
				}
				if (!(rq->filehandle_h & 0x80000000U)) {
					/* only NSS handles allowed... We have to do some fallback here... */
					break;
				}
				Inst.writeRequests++;
				info.errorcode = 0;
				info.connection = i->conn;
				info.conn = NULL;
				info.reserved2 = 0;
				info.reserved3 = 2;
				file.fileHandle = rq->filehandle_h;
				file.nssHandle = NULL;
				pos.written = 0;
				if (rq->length) {
					pos.offset = rq->offset;
					pos.length = rq->length;
					pos.buffer = rq->data;
					pos.reserved1 = 0;
					COMN_Write(&info, &pos, &file);
					Inst.writtenBytes += pos.written;
					if (info.conn) {
						info.conn->bytesWritten += pos.written;
					}
				} else {
					COMN_SetDataSize(&info, &file, rq->offset, 0);
				}
				if (info.errorcode) {
					ncpsend->ReplyKeepNoFragments(i, NSS2NCPError(info.errorcode));
				} else {
					unsigned long long len = pos.written;
					ncpsend->ReplyKeep(i, 0, 1, &len, sizeof(len));
				}
				COMN_DoReleaseFileHandleIDP(&info, &file);
			}
			return;
		case 3:
			if (reqLen < 8) {
				break;
			}
			{
				struct fhReq* rq = (struct fhReq*)(i->request + 2);

				err = 0xFF;
				if (rq->rsvd != 0) {
					break;
				}
				err = 0x84;
				if (((rq->filehandle_l - 1U) & 0xFFFF) != (rq->filehandle_h & 0xFFFF)) {
					break;
				}
				if (!(rq->filehandle_h & 0x80000000U)) {
					/* only NSS handles allowed... We have to do some fallback here... */
					break;
				}
				NWSAGetSize(i, ncpsend, rq, workspace, workspaceLen);
			}
			return;
		default:
			break;
	}
errquit:;
	ncpsend->ReplyKeepNoFragments(i, err);
}

static void termFn(int dummy) {
	if (ncpHooked) {
		ncpHooked = 0;
		ReleaseNCPVerb(NCPRTag, NCP_NSS64Verb);
	}
}

int main(int argc, char* argv[]) {
	int err;
	
	signal(SIGINT, termFn);
	signal(SIGTERM, termFn);

	InitNSS2NCPErrorTable();

	nlmHandle = GetNLMHandle();
//	AllocRTag = AllocateResourceTag(nlmHandle, "Buffer memory", AllocSignature);
	NCPRTag = AllocateResourceTag(nlmHandle, "64-bit Linux NCP calls", NCPVerbRTag);
	err = HookNCPVerb(NCPRTag, NCP_NSS64Verb, nss64NCP);
	if (err) {
		printf("Cannot hook NCP 0x%02X, error = %u!\r\n", NCP_NSS64Verb, err);
		return 0;
	}
	ncpHooked = 1;
	ExitThread(TSR_THREAD, 0);
}
