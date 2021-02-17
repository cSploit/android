#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <mcheck.h>
#include <ncp/nwcalls.h>
#include <ncp/nwnet.h>

#if 0
static const char *testuser    = "ADMIN.SUN";
static const char *testpasswd  = "something";
static const char *testserver  = "testtree.sun.ac.za";
#else
static const char *testuser   = "ADMIN.VC.CVUT.CZ";
static const char *testpasswd = "aaaaa";
static const char *testserver = "VMWARE-NW6";
#endif

static NWDSCCODE
ndsfindusercontext (NWDSContextHandle ctx, const char *ousername)
{
  NWDSCCODE       nwerr = 0;
  int             found = 0;
  Buf_T         *filter = NULL;
  Buf_T            *buf = NULL;
  Filter_Cursor_T  *cur = NULL;
  nuint32            ih = NO_MORE_ITERATIONS;

  if ((nwerr = NWDSAllocBuf (DEFAULT_MESSAGE_LEN, &filter)))
    goto out;

  if ((nwerr = NWDSAllocBuf (DEFAULT_MESSAGE_LEN, &buf)))
    goto out;

  if ((nwerr = NWDSInitBuf (ctx, DSV_SEARCH_FILTER, filter)))
    goto out;

  if ((nwerr = NWDSAllocFilter (&cur)))
    goto out;

  if ((nwerr = NWDSAddFilterToken (cur, FTOK_LPAREN, NULL, 0)))
    goto out;

  if ((nwerr = NWDSAddFilterToken (cur, FTOK_ANAME, "Object Class", SYN_CI_STRING)))
    goto out;

  if ((nwerr = NWDSAddFilterToken (cur, FTOK_EQ, NULL, 0)))
    goto out;

  if ((nwerr = NWDSAddFilterToken (cur, FTOK_AVAL, "User", SYN_CLASS_NAME)))
    goto out;

  if ((nwerr = NWDSAddFilterToken (cur, FTOK_RPAREN, NULL, 0)))
    goto out;

  if ((nwerr = NWDSAddFilterToken (cur, FTOK_AND, NULL, 0)))
    goto out;

  if ((nwerr = NWDSAddFilterToken (cur, FTOK_LPAREN, NULL, 0)))
    goto out;

  if ((nwerr = NWDSAddFilterToken (cur, FTOK_ANAME, "CN", SYN_CI_STRING)))
    goto out;

  if ((nwerr = NWDSAddFilterToken (cur, FTOK_EQ, NULL, 0)))
    goto out;

  if ((nwerr = NWDSAddFilterToken (cur, FTOK_AVAL, (char *) ousername, SYN_CI_STRING)))
    goto out;

  if ((nwerr = NWDSAddFilterToken (cur, FTOK_RPAREN, NULL, 0)))
    goto out;

  if ((nwerr = NWDSAddFilterToken (cur, FTOK_END, NULL, 0)))
    goto out;

  if ((nwerr = NWDSPutFilter (ctx, filter, cur, NULL)))
    goto out;

  do
    {
      nuint32 numobjs, numattr, j;
      Object_Info_T objinfo;
      char name[MAX_DN_CHARS+1];

      printf ("searching ...\n");
      if ((nwerr = NWDSSearch (ctx,
              "[Root]",               /* Subtree to search */
              DS_SEARCH_SUBTREE,      /* Search scope is subtree */
              0,                      /* Disable dereferencing of aliases */
              filter,                 /* The filter, ObjectClass=User */
              DS_ATTRIBUTE_NAMES,     /* Info type to return */
              0,                      /* Attribute scope is false */
              NULL,                   /* Do not return any attributes */
              &ih,                    /* Iteration handle */
              0,
              NULL,
              buf)))
        break;

      if ((nwerr = NWDSGetObjectCount (ctx, buf, &numobjs)))
        break;

      for (j = 0; j < numobjs; j++)
        {
          if ((nwerr = NWDSGetObjectName (ctx, buf, name, &numattr, &objinfo)))
            continue;

          printf ("found: %s\n", name);
        }
    }
  while ((nwerr == 0) && (ih != NO_MORE_ITERATIONS));
  if (found == 0)
    nwerr = -601;

out:
  if (buf)
    NWDSFreeBuf (buf);
  if (filter)
    NWDSFreeBuf (filter);
  return nwerr;
}

int
main (int argc, char *argv[])
{
  NWDSCCODE nwerr;
  long lerr;
  u_int32_t          confidence = DCV_HIGH_CONF;
  u_int32_t             ctxflag = DCV_XLATE_STRINGS;
  NWDSContextHandle ctx;
  NWCONN_HANDLE conn;
  struct ncp_conn_spec connsp;
  static const u_int32_t add[] =
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };

  mtrace ();

  NWCallsInit (NULL, NULL);
  NWDSInitRequester ();

  if ((nwerr = NWDSCreateContextHandle (&ctx)))
    goto err;

  if ((nwerr = NWDSSetContext (ctx, DCK_FLAGS, &ctxflag)))
    goto err;

  if ((nwerr = NWDSSetContext (ctx, DCK_NAME_CONTEXT, "[Root]")))
    goto err;

  if ((nwerr = NWDSSetContext (ctx, DCK_CONFIDENCE, &confidence)))
    goto err;

  if ((nwerr = NWDSSetTransport (ctx, 16, add)))
    goto err;

  memset (&connsp, 0, sizeof (connsp));
  strcpy (connsp.server, testserver);
  if ((conn = ncp_open (&connsp, &lerr)) == NULL)
    {
      nwerr = ERR_NO_CONNECTION;
      goto err;
    }

#if 1
  if ((nwerr = nds_login_auth (conn, testuser, testpasswd)))
    goto err;
#endif

  if ((nwerr = NWDSAddConnection (ctx, conn)))
    goto err;

  printf("connected\n");
  ndsfindusercontext (ctx, "TEST");

  NWCCCloseConn (conn);
  NWDSFreeContext (ctx);
  return 0;

err:
  printf ("failed: %s\n", strnwerror (nwerr));
  return 1;
}



