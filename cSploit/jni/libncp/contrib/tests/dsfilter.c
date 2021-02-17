/* p.c */
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <ncp/nwcalls.h>
#include <ncp/nwnet.h>

static NWDSCCODE test1(Filter_Cursor_T* cur) {
	NWDSCCODE dserr;

	if ((dserr = NWDSAddFilterToken (cur, FTOK_BASECLS, NULL, 0)))
		return dserr;

	if ((dserr = NWDSAddFilterToken (cur, FTOK_ANAME, "User", SYN_CLASS_NAME)))
		return dserr;

	if ((dserr = NWDSAddFilterToken (cur, FTOK_AND, NULL, 0)))
		return dserr;

	if ((dserr = NWDSAddFilterToken (cur, FTOK_ANAME, "CN", SYN_CI_STRING)))
		return dserr;

	if ((dserr = NWDSAddFilterToken (cur, FTOK_EQ, NULL, 0)))
		return dserr;

	if ((dserr = NWDSAddFilterToken (cur, FTOK_AVAL, "ADMIN", SYN_CI_STRING)))
		return dserr;

	if ((dserr = NWDSAddFilterToken (cur, FTOK_END, NULL, 0)))
		return dserr;
		
	return 0;
}

static NWDSCCODE test2(Filter_Cursor_T* cur) {
	NWDSCCODE dserr;

	if ((dserr = NWDSAddFilterToken (cur, FTOK_LPAREN, NULL, 0)))
		return dserr;

	if ((dserr = NWDSAddFilterToken (cur, FTOK_ANAME, "Object Class", SYN_CI_STRING)))
		return dserr;

	if ((dserr = NWDSAddFilterToken (cur, FTOK_EQ, NULL, 0)))
		return dserr;

	if ((dserr = NWDSAddFilterToken (cur, FTOK_AVAL, "User", SYN_CLASS_NAME)))
		return dserr;

	if ((dserr = NWDSAddFilterToken (cur, FTOK_RPAREN, NULL, 0)))
		return dserr;

	if ((dserr = NWDSAddFilterToken (cur, FTOK_AND, NULL, 0)))
		return dserr;

	if ((dserr = NWDSAddFilterToken (cur, FTOK_LPAREN, NULL, 0)))
		return dserr;

	if ((dserr = NWDSAddFilterToken (cur, FTOK_ANAME, "CN", SYN_CI_STRING)))
		return dserr;

	if ((dserr = NWDSAddFilterToken (cur, FTOK_EQ, NULL, 0)))
		return dserr;

	if ((dserr = NWDSAddFilterToken (cur, FTOK_AVAL, "ADMIN", SYN_CI_STRING)))
		return dserr;

	if ((dserr = NWDSAddFilterToken (cur, FTOK_RPAREN, NULL, 0)))
		return dserr;

	if ((dserr = NWDSAddFilterToken (cur, FTOK_END, NULL, 0)))
		return dserr;
		
	return 0;
}

static int searchtest(const char* testname, NWDSCCODE (*fn)(Filter_Cursor_T*))
{
	NWDSContextHandle	ctx;
	u_int32_t		confidence = DCV_HIGH_CONF;
	u_int32_t		ctxflag = DCV_XLATE_STRINGS;
	NWDSCCODE		dserr = 0;
	Buf_T			*filter = NULL;
	Buf_T			*buf = NULL;
	Filter_Cursor_T		*cur = NULL;

	static const u_int32_t add[] =
		{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };

	NWCallsInit (NULL, NULL);
	NWDSInitRequester ();

	if ((dserr = NWDSCreateContextHandle (&ctx)))
		goto err;

	if ((dserr = NWDSSetContext (ctx, DCK_FLAGS, &ctxflag)))
		goto err;

	if ((dserr = NWDSSetContext (ctx, DCK_NAME_CONTEXT, "[Root]")))
		goto err;

	if ((dserr = NWDSSetContext (ctx, DCK_CONFIDENCE, &confidence)))
		goto err;

	if ((dserr = NWDSSetTransport (ctx, 16, add)))
		goto err;

	if ((dserr = NWDSAllocBuf (DEFAULT_MESSAGE_LEN, &filter)))
		goto err;

	if ((dserr = NWDSAllocBuf (DEFAULT_MESSAGE_LEN, &buf)))
		goto err;

	if ((dserr = NWDSInitBuf (ctx, DSV_SEARCH_FILTER, filter)))
		goto err;

	if ((dserr = NWDSAllocFilter (&cur)))
		goto err;

	if ((dserr = fn(cur)))
		goto err;

	if ((dserr = NWDSPutFilter (ctx, filter, cur, NULL)))
		goto err;

	printf("%s: passed\n", testname);
	return 0;

err:
	printf("%s: failed: %s\n", testname, strnwerror (dserr));
	return 1;
}

int main(int argc, char* argv[]) {
	int r;
	
	r = searchtest("test1", test1);
	if (r)
		return r;
	r = searchtest("test2", test2);
	if (r)
		return r;
		
	printf("OK\n");
	return 0;
}
