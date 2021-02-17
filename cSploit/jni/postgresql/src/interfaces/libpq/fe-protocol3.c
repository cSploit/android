/*-------------------------------------------------------------------------
 *
 * fe-protocol3.c
 *	  functions that are specific to frontend/backend protocol version 3
 *
 * Portions Copyright (c) 1996-2011, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  src/interfaces/libpq/fe-protocol3.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres_fe.h"

#include <ctype.h>
#include <fcntl.h>

#include "libpq-fe.h"
#include "libpq-int.h"

#include "mb/pg_wchar.h"

#ifdef WIN32
#include "win32.h"
#else
#include <unistd.h>
#include <netinet/in.h>
#ifdef HAVE_NETINET_TCP_H
#include <netinet/tcp.h>
#endif
#include <arpa/inet.h>
#endif


/*
 * This macro lists the backend message types that could be "long" (more
 * than a couple of kilobytes).
 */
#define VALID_LONG_MESSAGE_TYPE(id) \
	((id) == 'T' || (id) == 'D' || (id) == 'd' || (id) == 'V' || \
	 (id) == 'E' || (id) == 'N' || (id) == 'A')


static void handleSyncLoss(PGconn *conn, char id, int msgLength);
static int	getRowDescriptions(PGconn *conn);
static int	getParamDescriptions(PGconn *conn);
static int	getAnotherTuple(PGconn *conn, int msgLength);
static int	getParameterStatus(PGconn *conn);
static int	getNotify(PGconn *conn);
static int	getCopyStart(PGconn *conn, ExecStatusType copytype);
static int	getReadyForQuery(PGconn *conn);
static void reportErrorPosition(PQExpBuffer msg, const char *query,
					int loc, int encoding);
static int build_startup_packet(const PGconn *conn, char *packet,
					 const PQEnvironmentOption *options);


/*
 * parseInput: if appropriate, parse input data from backend
 * until input is exhausted or a stopping state is reached.
 * Note that this function will NOT attempt to read more data from the backend.
 */
void
pqParseInput3(PGconn *conn)
{
	char		id;
	int			msgLength;
	int			avail;

	/*
	 * Loop to parse successive complete messages available in the buffer.
	 */
	for (;;)
	{
		/*
		 * Try to read a message.  First get the type code and length. Return
		 * if not enough data.
		 */
		conn->inCursor = conn->inStart;
		if (pqGetc(&id, conn))
			return;
		if (pqGetInt(&msgLength, 4, conn))
			return;

		/*
		 * Try to validate message type/length here.  A length less than 4 is
		 * definitely broken.  Large lengths should only be believed for a few
		 * message types.
		 */
		if (msgLength < 4)
		{
			handleSyncLoss(conn, id, msgLength);
			return;
		}
		if (msgLength > 30000 && !VALID_LONG_MESSAGE_TYPE(id))
		{
			handleSyncLoss(conn, id, msgLength);
			return;
		}

		/*
		 * Can't process if message body isn't all here yet.
		 */
		msgLength -= 4;
		avail = conn->inEnd - conn->inCursor;
		if (avail < msgLength)
		{
			/*
			 * Before returning, enlarge the input buffer if needed to hold
			 * the whole message.  This is better than leaving it to
			 * pqReadData because we can avoid multiple cycles of realloc()
			 * when the message is large; also, we can implement a reasonable
			 * recovery strategy if we are unable to make the buffer big
			 * enough.
			 */
			if (pqCheckInBufferSpace(conn->inCursor + (size_t) msgLength,
									 conn))
			{
				/*
				 * XXX add some better recovery code... plan is to skip over
				 * the message using its length, then report an error. For the
				 * moment, just treat this like loss of sync (which indeed it
				 * might be!)
				 */
				handleSyncLoss(conn, id, msgLength);
			}
			return;
		}

		/*
		 * NOTIFY and NOTICE messages can happen in any state; always process
		 * them right away.
		 *
		 * Most other messages should only be processed while in BUSY state.
		 * (In particular, in READY state we hold off further parsing until
		 * the application collects the current PGresult.)
		 *
		 * However, if the state is IDLE then we got trouble; we need to deal
		 * with the unexpected message somehow.
		 *
		 * ParameterStatus ('S') messages are a special case: in IDLE state we
		 * must process 'em (this case could happen if a new value was adopted
		 * from config file due to SIGHUP), but otherwise we hold off until
		 * BUSY state.
		 */
		if (id == 'A')
		{
			if (getNotify(conn))
				return;
		}
		else if (id == 'N')
		{
			if (pqGetErrorNotice3(conn, false))
				return;
		}
		else if (conn->asyncStatus != PGASYNC_BUSY)
		{
			/* If not IDLE state, just wait ... */
			if (conn->asyncStatus != PGASYNC_IDLE)
				return;

			/*
			 * Unexpected message in IDLE state; need to recover somehow.
			 * ERROR messages are displayed using the notice processor;
			 * ParameterStatus is handled normally; anything else is just
			 * dropped on the floor after displaying a suitable warning
			 * notice.	(An ERROR is very possibly the backend telling us why
			 * it is about to close the connection, so we don't want to just
			 * discard it...)
			 */
			if (id == 'E')
			{
				if (pqGetErrorNotice3(conn, false /* treat as notice */ ))
					return;
			}
			else if (id == 'S')
			{
				if (getParameterStatus(conn))
					return;
			}
			else
			{
				pqInternalNotice(&conn->noticeHooks,
						"message type 0x%02x arrived from server while idle",
								 id);
				/* Discard the unexpected message */
				conn->inCursor += msgLength;
			}
		}
		else
		{
			/*
			 * In BUSY state, we can process everything.
			 */
			switch (id)
			{
				case 'C':		/* command complete */
					if (pqGets(&conn->workBuffer, conn))
						return;
					if (conn->result == NULL)
					{
						conn->result = PQmakeEmptyPGresult(conn,
														   PGRES_COMMAND_OK);
						if (!conn->result)
							return;
					}
					strncpy(conn->result->cmdStatus, conn->workBuffer.data,
							CMDSTATUS_LEN);
					conn->asyncStatus = PGASYNC_READY;
					break;
				case 'E':		/* error return */
					if (pqGetErrorNotice3(conn, true))
						return;
					conn->asyncStatus = PGASYNC_READY;
					break;
				case 'Z':		/* backend is ready for new query */
					if (getReadyForQuery(conn))
						return;
					conn->asyncStatus = PGASYNC_IDLE;
					break;
				case 'I':		/* empty query */
					if (conn->result == NULL)
					{
						conn->result = PQmakeEmptyPGresult(conn,
														   PGRES_EMPTY_QUERY);
						if (!conn->result)
							return;
					}
					conn->asyncStatus = PGASYNC_READY;
					break;
				case '1':		/* Parse Complete */
					/* If we're doing PQprepare, we're done; else ignore */
					if (conn->queryclass == PGQUERY_PREPARE)
					{
						if (conn->result == NULL)
						{
							conn->result = PQmakeEmptyPGresult(conn,
														   PGRES_COMMAND_OK);
							if (!conn->result)
								return;
						}
						conn->asyncStatus = PGASYNC_READY;
					}
					break;
				case '2':		/* Bind Complete */
				case '3':		/* Close Complete */
					/* Nothing to do for these message types */
					break;
				case 'S':		/* parameter status */
					if (getParameterStatus(conn))
						return;
					break;
				case 'K':		/* secret key data from the backend */

					/*
					 * This is expected only during backend startup, but it's
					 * just as easy to handle it as part of the main loop.
					 * Save the data and continue processing.
					 */
					if (pqGetInt(&(conn->be_pid), 4, conn))
						return;
					if (pqGetInt(&(conn->be_key), 4, conn))
						return;
					break;
				case 'T':		/* Row Description */
					if (conn->result == NULL ||
						conn->queryclass == PGQUERY_DESCRIBE)
					{
						/* First 'T' in a query sequence */
						if (getRowDescriptions(conn))
							return;

						/*
						 * If we're doing a Describe, we're ready to pass the
						 * result back to the client.
						 */
						if (conn->queryclass == PGQUERY_DESCRIBE)
							conn->asyncStatus = PGASYNC_READY;
					}
					else
					{
						/*
						 * A new 'T' message is treated as the start of
						 * another PGresult.  (It is not clear that this is
						 * really possible with the current backend.) We stop
						 * parsing until the application accepts the current
						 * result.
						 */
						conn->asyncStatus = PGASYNC_READY;
						return;
					}
					break;
				case 'n':		/* No Data */

					/*
					 * NoData indicates that we will not be seeing a
					 * RowDescription message because the statement or portal
					 * inquired about doesn't return rows.
					 *
					 * If we're doing a Describe, we have to pass something
					 * back to the client, so set up a COMMAND_OK result,
					 * instead of TUPLES_OK.  Otherwise we can just ignore
					 * this message.
					 */
					if (conn->queryclass == PGQUERY_DESCRIBE)
					{
						if (conn->result == NULL)
						{
							conn->result = PQmakeEmptyPGresult(conn,
														   PGRES_COMMAND_OK);
							if (!conn->result)
								return;
						}
						conn->asyncStatus = PGASYNC_READY;
					}
					break;
				case 't':		/* Parameter Description */
					if (getParamDescriptions(conn))
						return;
					break;
				case 'D':		/* Data Row */
					if (conn->result != NULL &&
						conn->result->resultStatus == PGRES_TUPLES_OK)
					{
						/* Read another tuple of a normal query response */
						if (getAnotherTuple(conn, msgLength))
							return;
					}
					else if (conn->result != NULL &&
							 conn->result->resultStatus == PGRES_FATAL_ERROR)
					{
						/*
						 * We've already choked for some reason.  Just discard
						 * tuples till we get to the end of the query.
						 */
						conn->inCursor += msgLength;
					}
					else
					{
						/* Set up to report error at end of query */
						printfPQExpBuffer(&conn->errorMessage,
										  libpq_gettext("server sent data (\"D\" message) without prior row description (\"T\" message)\n"));
						pqSaveErrorResult(conn);
						/* Discard the unexpected message */
						conn->inCursor += msgLength;
					}
					break;
				case 'G':		/* Start Copy In */
					if (getCopyStart(conn, PGRES_COPY_IN))
						return;
					conn->asyncStatus = PGASYNC_COPY_IN;
					break;
				case 'H':		/* Start Copy Out */
					if (getCopyStart(conn, PGRES_COPY_OUT))
						return;
					conn->asyncStatus = PGASYNC_COPY_OUT;
					conn->copy_already_done = 0;
					break;
				case 'W':		/* Start Copy Both */
					if (getCopyStart(conn, PGRES_COPY_BOTH))
						return;
					conn->asyncStatus = PGASYNC_COPY_BOTH;
					conn->copy_already_done = 0;
					break;
				case 'd':		/* Copy Data */

					/*
					 * If we see Copy Data, just silently drop it.	This would
					 * only occur if application exits COPY OUT mode too
					 * early.
					 */
					conn->inCursor += msgLength;
					break;
				case 'c':		/* Copy Done */

					/*
					 * If we see Copy Done, just silently drop it.	This is
					 * the normal case during PQendcopy.  We will keep
					 * swallowing data, expecting to see command-complete for
					 * the COPY command.
					 */
					break;
				default:
					printfPQExpBuffer(&conn->errorMessage,
									  libpq_gettext(
													"unexpected response from server; first received character was \"%c\"\n"),
									  id);
					/* build an error result holding the error message */
					pqSaveErrorResult(conn);
					/* not sure if we will see more, so go to ready state */
					conn->asyncStatus = PGASYNC_READY;
					/* Discard the unexpected message */
					conn->inCursor += msgLength;
					break;
			}					/* switch on protocol character */
		}
		/* Successfully consumed this message */
		if (conn->inCursor == conn->inStart + 5 + msgLength)
		{
			/* Normal case: parsing agrees with specified length */
			conn->inStart = conn->inCursor;
		}
		else
		{
			/* Trouble --- report it */
			printfPQExpBuffer(&conn->errorMessage,
							  libpq_gettext("message contents do not agree with length in message type \"%c\"\n"),
							  id);
			/* build an error result holding the error message */
			pqSaveErrorResult(conn);
			conn->asyncStatus = PGASYNC_READY;
			/* trust the specified message length as what to skip */
			conn->inStart += 5 + msgLength;
		}
	}
}

/*
 * handleSyncLoss: clean up after loss of message-boundary sync
 *
 * There isn't really a lot we can do here except abandon the connection.
 */
static void
handleSyncLoss(PGconn *conn, char id, int msgLength)
{
	printfPQExpBuffer(&conn->errorMessage,
					  libpq_gettext(
	"lost synchronization with server: got message type \"%c\", length %d\n"),
					  id, msgLength);
	/* build an error result holding the error message */
	pqSaveErrorResult(conn);
	conn->asyncStatus = PGASYNC_READY;	/* drop out of GetResult wait loop */

	pqsecure_close(conn);
	closesocket(conn->sock);
	conn->sock = -1;
	conn->status = CONNECTION_BAD;		/* No more connection to backend */
}

/*
 * parseInput subroutine to read a 'T' (row descriptions) message.
 * We'll build a new PGresult structure (unless called for a Describe
 * command for a prepared statement) containing the attribute data.
 * Returns: 0 if completed message, EOF if not enough data yet.
 *
 * Note that if we run out of data, we have to release the partially
 * constructed PGresult, and rebuild it again next time.  Fortunately,
 * that shouldn't happen often, since 'T' messages usually fit in a packet.
 */
static int
getRowDescriptions(PGconn *conn)
{
	PGresult   *result;
	int			nfields;
	int			i;

	/*
	 * When doing Describe for a prepared statement, there'll already be a
	 * PGresult created by getParamDescriptions, and we should fill data into
	 * that.  Otherwise, create a new, empty PGresult.
	 */
	if (conn->queryclass == PGQUERY_DESCRIBE)
	{
		if (conn->result)
			result = conn->result;
		else
			result = PQmakeEmptyPGresult(conn, PGRES_COMMAND_OK);
	}
	else
		result = PQmakeEmptyPGresult(conn, PGRES_TUPLES_OK);
	if (!result)
		goto failure;

	/* parseInput already read the 'T' label and message length. */
	/* the next two bytes are the number of fields */
	if (pqGetInt(&(result->numAttributes), 2, conn))
		goto failure;
	nfields = result->numAttributes;

	/* allocate space for the attribute descriptors */
	if (nfields > 0)
	{
		result->attDescs = (PGresAttDesc *)
			pqResultAlloc(result, nfields * sizeof(PGresAttDesc), TRUE);
		if (!result->attDescs)
			goto failure;
		MemSet(result->attDescs, 0, nfields * sizeof(PGresAttDesc));
	}

	/* result->binary is true only if ALL columns are binary */
	result->binary = (nfields > 0) ? 1 : 0;

	/* get type info */
	for (i = 0; i < nfields; i++)
	{
		int			tableid;
		int			columnid;
		int			typid;
		int			typlen;
		int			atttypmod;
		int			format;

		if (pqGets(&conn->workBuffer, conn) ||
			pqGetInt(&tableid, 4, conn) ||
			pqGetInt(&columnid, 2, conn) ||
			pqGetInt(&typid, 4, conn) ||
			pqGetInt(&typlen, 2, conn) ||
			pqGetInt(&atttypmod, 4, conn) ||
			pqGetInt(&format, 2, conn))
		{
			goto failure;
		}

		/*
		 * Since pqGetInt treats 2-byte integers as unsigned, we need to
		 * coerce these results to signed form.
		 */
		columnid = (int) ((int16) columnid);
		typlen = (int) ((int16) typlen);
		format = (int) ((int16) format);

		result->attDescs[i].name = pqResultStrdup(result,
												  conn->workBuffer.data);
		if (!result->attDescs[i].name)
			goto failure;
		result->attDescs[i].tableid = tableid;
		result->attDescs[i].columnid = columnid;
		result->attDescs[i].format = format;
		result->attDescs[i].typid = typid;
		result->attDescs[i].typlen = typlen;
		result->attDescs[i].atttypmod = atttypmod;

		if (format != 1)
			result->binary = 0;
	}

	/* Success! */
	conn->result = result;
	return 0;

failure:

	/*
	 * Discard incomplete result, unless it's from getParamDescriptions.
	 *
	 * Note that if we hit a bufferload boundary while handling the
	 * describe-statement case, we'll forget any PGresult space we just
	 * allocated, and then reallocate it on next try.  This will bloat the
	 * PGresult a little bit but the space will be freed at PQclear, so it
	 * doesn't seem worth trying to be smarter.
	 */
	if (result != conn->result)
		PQclear(result);
	return EOF;
}

/*
 * parseInput subroutine to read a 't' (ParameterDescription) message.
 * We'll build a new PGresult structure containing the parameter data.
 * Returns: 0 if completed message, EOF if not enough data yet.
 *
 * Note that if we run out of data, we have to release the partially
 * constructed PGresult, and rebuild it again next time.  Fortunately,
 * that shouldn't happen often, since 't' messages usually fit in a packet.
 */
static int
getParamDescriptions(PGconn *conn)
{
	PGresult   *result;
	int			nparams;
	int			i;

	result = PQmakeEmptyPGresult(conn, PGRES_COMMAND_OK);
	if (!result)
		goto failure;

	/* parseInput already read the 't' label and message length. */
	/* the next two bytes are the number of parameters */
	if (pqGetInt(&(result->numParameters), 2, conn))
		goto failure;
	nparams = result->numParameters;

	/* allocate space for the parameter descriptors */
	if (nparams > 0)
	{
		result->paramDescs = (PGresParamDesc *)
			pqResultAlloc(result, nparams * sizeof(PGresParamDesc), TRUE);
		if (!result->paramDescs)
			goto failure;
		MemSet(result->paramDescs, 0, nparams * sizeof(PGresParamDesc));
	}

	/* get parameter info */
	for (i = 0; i < nparams; i++)
	{
		int			typid;

		if (pqGetInt(&typid, 4, conn))
			goto failure;
		result->paramDescs[i].typid = typid;
	}

	/* Success! */
	conn->result = result;
	return 0;

failure:
	PQclear(result);
	return EOF;
}

/*
 * parseInput subroutine to read a 'D' (row data) message.
 * We add another tuple to the existing PGresult structure.
 * Returns: 0 if completed message, EOF if error or not enough data yet.
 *
 * Note that if we run out of data, we have to suspend and reprocess
 * the message after more data is received.  We keep a partially constructed
 * tuple in conn->curTuple, and avoid reallocating already-allocated storage.
 */
static int
getAnotherTuple(PGconn *conn, int msgLength)
{
	PGresult   *result = conn->result;
	int			nfields = result->numAttributes;
	PGresAttValue *tup;
	int			tupnfields;		/* # fields from tuple */
	int			vlen;			/* length of the current field value */
	int			i;

	/* Allocate tuple space if first time for this data message */
	if (conn->curTuple == NULL)
	{
		conn->curTuple = (PGresAttValue *)
			pqResultAlloc(result, nfields * sizeof(PGresAttValue), TRUE);
		if (conn->curTuple == NULL)
			goto outOfMemory;
		MemSet(conn->curTuple, 0, nfields * sizeof(PGresAttValue));
	}
	tup = conn->curTuple;

	/* Get the field count and make sure it's what we expect */
	if (pqGetInt(&tupnfields, 2, conn))
		return EOF;

	if (tupnfields != nfields)
	{
		/* Replace partially constructed result with an error result */
		printfPQExpBuffer(&conn->errorMessage,
				 libpq_gettext("unexpected field count in \"D\" message\n"));
		pqSaveErrorResult(conn);
		/* Discard the failed message by pretending we read it */
		conn->inCursor = conn->inStart + 5 + msgLength;
		return 0;
	}

	/* Scan the fields */
	for (i = 0; i < nfields; i++)
	{
		/* get the value length */
		if (pqGetInt(&vlen, 4, conn))
			return EOF;
		if (vlen == -1)
		{
			/* null field */
			tup[i].value = result->null_field;
			tup[i].len = NULL_LEN;
			continue;
		}
		if (vlen < 0)
			vlen = 0;
		if (tup[i].value == NULL)
		{
			bool		isbinary = (result->attDescs[i].format != 0);

			tup[i].value = (char *) pqResultAlloc(result, vlen + 1, isbinary);
			if (tup[i].value == NULL)
				goto outOfMemory;
		}
		tup[i].len = vlen;
		/* read in the value */
		if (vlen > 0)
			if (pqGetnchar((char *) (tup[i].value), vlen, conn))
				return EOF;
		/* we have to terminate this ourselves */
		tup[i].value[vlen] = '\0';
	}

	/* Success!  Store the completed tuple in the result */
	if (!pqAddTuple(result, tup))
		goto outOfMemory;
	/* and reset for a new message */
	conn->curTuple = NULL;

	return 0;

outOfMemory:

	/*
	 * Replace partially constructed result with an error result. First
	 * discard the old result to try to win back some memory.
	 */
	pqClearAsyncResult(conn);
	printfPQExpBuffer(&conn->errorMessage,
					  libpq_gettext("out of memory for query result\n"));
	pqSaveErrorResult(conn);

	/* Discard the failed message by pretending we read it */
	conn->inCursor = conn->inStart + 5 + msgLength;
	return 0;
}


/*
 * Attempt to read an Error or Notice response message.
 * This is possible in several places, so we break it out as a subroutine.
 * Entry: 'E' or 'N' message type and length have already been consumed.
 * Exit: returns 0 if successfully consumed message.
 *		 returns EOF if not enough data.
 */
int
pqGetErrorNotice3(PGconn *conn, bool isError)
{
	PGresult   *res = NULL;
	PQExpBufferData workBuf;
	char		id;
	const char *val;
	const char *querytext = NULL;
	int			querypos = 0;

	/*
	 * Since the fields might be pretty long, we create a temporary
	 * PQExpBuffer rather than using conn->workBuffer.	workBuffer is intended
	 * for stuff that is expected to be short.	We shouldn't use
	 * conn->errorMessage either, since this might be only a notice.
	 */
	initPQExpBuffer(&workBuf);

	/*
	 * Make a PGresult to hold the accumulated fields.	We temporarily lie
	 * about the result status, so that PQmakeEmptyPGresult doesn't uselessly
	 * copy conn->errorMessage.
	 */
	res = PQmakeEmptyPGresult(conn, PGRES_EMPTY_QUERY);
	if (!res)
		goto fail;
	res->resultStatus = isError ? PGRES_FATAL_ERROR : PGRES_NONFATAL_ERROR;

	/*
	 * Read the fields and save into res.
	 */
	for (;;)
	{
		if (pqGetc(&id, conn))
			goto fail;
		if (id == '\0')
			break;				/* terminator found */
		if (pqGets(&workBuf, conn))
			goto fail;
		pqSaveMessageField(res, id, workBuf.data);
	}

	/*
	 * Now build the "overall" error message for PQresultErrorMessage.
	 *
	 * Also, save the SQLSTATE in conn->last_sqlstate.
	 */
	resetPQExpBuffer(&workBuf);
	val = PQresultErrorField(res, PG_DIAG_SEVERITY);
	if (val)
		appendPQExpBuffer(&workBuf, "%s:  ", val);
	val = PQresultErrorField(res, PG_DIAG_SQLSTATE);
	if (val)
	{
		if (strlen(val) < sizeof(conn->last_sqlstate))
			strcpy(conn->last_sqlstate, val);
		if (conn->verbosity == PQERRORS_VERBOSE)
			appendPQExpBuffer(&workBuf, "%s: ", val);
	}
	val = PQresultErrorField(res, PG_DIAG_MESSAGE_PRIMARY);
	if (val)
		appendPQExpBufferStr(&workBuf, val);
	val = PQresultErrorField(res, PG_DIAG_STATEMENT_POSITION);
	if (val)
	{
		if (conn->verbosity != PQERRORS_TERSE && conn->last_query != NULL)
		{
			/* emit position as a syntax cursor display */
			querytext = conn->last_query;
			querypos = atoi(val);
		}
		else
		{
			/* emit position as text addition to primary message */
			/* translator: %s represents a digit string */
			appendPQExpBuffer(&workBuf, libpq_gettext(" at character %s"),
							  val);
		}
	}
	else
	{
		val = PQresultErrorField(res, PG_DIAG_INTERNAL_POSITION);
		if (val)
		{
			querytext = PQresultErrorField(res, PG_DIAG_INTERNAL_QUERY);
			if (conn->verbosity != PQERRORS_TERSE && querytext != NULL)
			{
				/* emit position as a syntax cursor display */
				querypos = atoi(val);
			}
			else
			{
				/* emit position as text addition to primary message */
				/* translator: %s represents a digit string */
				appendPQExpBuffer(&workBuf, libpq_gettext(" at character %s"),
								  val);
			}
		}
	}
	appendPQExpBufferChar(&workBuf, '\n');
	if (conn->verbosity != PQERRORS_TERSE)
	{
		if (querytext && querypos > 0)
			reportErrorPosition(&workBuf, querytext, querypos,
								conn->client_encoding);
		val = PQresultErrorField(res, PG_DIAG_MESSAGE_DETAIL);
		if (val)
			appendPQExpBuffer(&workBuf, libpq_gettext("DETAIL:  %s\n"), val);
		val = PQresultErrorField(res, PG_DIAG_MESSAGE_HINT);
		if (val)
			appendPQExpBuffer(&workBuf, libpq_gettext("HINT:  %s\n"), val);
		val = PQresultErrorField(res, PG_DIAG_INTERNAL_QUERY);
		if (val)
			appendPQExpBuffer(&workBuf, libpq_gettext("QUERY:  %s\n"), val);
		val = PQresultErrorField(res, PG_DIAG_CONTEXT);
		if (val)
			appendPQExpBuffer(&workBuf, libpq_gettext("CONTEXT:  %s\n"), val);
	}
	if (conn->verbosity == PQERRORS_VERBOSE)
	{
		const char *valf;
		const char *vall;

		valf = PQresultErrorField(res, PG_DIAG_SOURCE_FILE);
		vall = PQresultErrorField(res, PG_DIAG_SOURCE_LINE);
		val = PQresultErrorField(res, PG_DIAG_SOURCE_FUNCTION);
		if (val || valf || vall)
		{
			appendPQExpBufferStr(&workBuf, libpq_gettext("LOCATION:  "));
			if (val)
				appendPQExpBuffer(&workBuf, libpq_gettext("%s, "), val);
			if (valf && vall)	/* unlikely we'd have just one */
				appendPQExpBuffer(&workBuf, libpq_gettext("%s:%s"),
								  valf, vall);
			appendPQExpBufferChar(&workBuf, '\n');
		}
	}

	/*
	 * Either save error as current async result, or just emit the notice.
	 */
	if (isError)
	{
		res->errMsg = pqResultStrdup(res, workBuf.data);
		if (!res->errMsg)
			goto fail;
		pqClearAsyncResult(conn);
		conn->result = res;
		appendPQExpBufferStr(&conn->errorMessage, workBuf.data);
	}
	else
	{
		/* We can cheat a little here and not copy the message. */
		res->errMsg = workBuf.data;
		if (res->noticeHooks.noticeRec != NULL)
			(*res->noticeHooks.noticeRec) (res->noticeHooks.noticeRecArg, res);
		PQclear(res);
	}

	termPQExpBuffer(&workBuf);
	return 0;

fail:
	PQclear(res);
	termPQExpBuffer(&workBuf);
	return EOF;
}

/*
 * Add an error-location display to the error message under construction.
 *
 * The cursor location is measured in logical characters; the query string
 * is presumed to be in the specified encoding.
 */
static void
reportErrorPosition(PQExpBuffer msg, const char *query, int loc, int encoding)
{
#define DISPLAY_SIZE	60		/* screen width limit, in screen cols */
#define MIN_RIGHT_CUT	10		/* try to keep this far away from EOL */

	char	   *wquery;
	int			slen,
				cno,
				i,
			   *qidx,
			   *scridx,
				qoffset,
				scroffset,
				ibeg,
				iend,
				loc_line;
	bool		mb_encoding,
				beg_trunc,
				end_trunc;

	/* Convert loc from 1-based to 0-based; no-op if out of range */
	loc--;
	if (loc < 0)
		return;

	/* Need a writable copy of the query */
	wquery = strdup(query);
	if (wquery == NULL)
		return;					/* fail silently if out of memory */

	/*
	 * Each character might occupy multiple physical bytes in the string, and
	 * in some Far Eastern character sets it might take more than one screen
	 * column as well.	We compute the starting byte offset and starting
	 * screen column of each logical character, and store these in qidx[] and
	 * scridx[] respectively.
	 */

	/* we need a safe allocation size... */
	slen = strlen(wquery) + 1;

	qidx = (int *) malloc(slen * sizeof(int));
	if (qidx == NULL)
	{
		free(wquery);
		return;
	}
	scridx = (int *) malloc(slen * sizeof(int));
	if (scridx == NULL)
	{
		free(qidx);
		free(wquery);
		return;
	}

	/* We can optimize a bit if it's a single-byte encoding */
	mb_encoding = (pg_encoding_max_length(encoding) != 1);

	/*
	 * Within the scanning loop, cno is the current character's logical
	 * number, qoffset is its offset in wquery, and scroffset is its starting
	 * logical screen column (all indexed from 0).	"loc" is the logical
	 * character number of the error location.	We scan to determine loc_line
	 * (the 1-based line number containing loc) and ibeg/iend (first character
	 * number and last+1 character number of the line containing loc). Note
	 * that qidx[] and scridx[] are filled only as far as iend.
	 */
	qoffset = 0;
	scroffset = 0;
	loc_line = 1;
	ibeg = 0;
	iend = -1;					/* -1 means not set yet */

	for (cno = 0; wquery[qoffset] != '\0'; cno++)
	{
		char		ch = wquery[qoffset];

		qidx[cno] = qoffset;
		scridx[cno] = scroffset;

		/*
		 * Replace tabs with spaces in the writable copy.  (Later we might
		 * want to think about coping with their variable screen width, but
		 * not today.)
		 */
		if (ch == '\t')
			wquery[qoffset] = ' ';

		/*
		 * If end-of-line, count lines and mark positions. Each \r or \n
		 * counts as a line except when \r \n appear together.
		 */
		else if (ch == '\r' || ch == '\n')
		{
			if (cno < loc)
			{
				if (ch == '\r' ||
					cno == 0 ||
					wquery[qidx[cno - 1]] != '\r')
					loc_line++;
				/* extract beginning = last line start before loc. */
				ibeg = cno + 1;
			}
			else
			{
				/* set extract end. */
				iend = cno;
				/* done scanning. */
				break;
			}
		}

		/* Advance */
		if (mb_encoding)
		{
			int			w;

			w = pg_encoding_dsplen(encoding, &wquery[qoffset]);
			/* treat any non-tab control chars as width 1 */
			if (w <= 0)
				w = 1;
			scroffset += w;
			qoffset += pg_encoding_mblen(encoding, &wquery[qoffset]);
		}
		else
		{
			/* We assume wide chars only exist in multibyte encodings */
			scroffset++;
			qoffset++;
		}
	}
	/* Fix up if we didn't find an end-of-line after loc */
	if (iend < 0)
	{
		iend = cno;				/* query length in chars, +1 */
		qidx[iend] = qoffset;
		scridx[iend] = scroffset;
	}

	/* Print only if loc is within computed query length */
	if (loc <= cno)
	{
		/* If the line extracted is too long, we truncate it. */
		beg_trunc = false;
		end_trunc = false;
		if (scridx[iend] - scridx[ibeg] > DISPLAY_SIZE)
		{
			/*
			 * We first truncate right if it is enough.  This code might be
			 * off a space or so on enforcing MIN_RIGHT_CUT if there's a wide
			 * character right there, but that should be okay.
			 */
			if (scridx[ibeg] + DISPLAY_SIZE >= scridx[loc] + MIN_RIGHT_CUT)
			{
				while (scridx[iend] - scridx[ibeg] > DISPLAY_SIZE)
					iend--;
				end_trunc = true;
			}
			else
			{
				/* Truncate right if not too close to loc. */
				while (scridx[loc] + MIN_RIGHT_CUT < scridx[iend])
				{
					iend--;
					end_trunc = true;
				}

				/* Truncate left if still too long. */
				while (scridx[iend] - scridx[ibeg] > DISPLAY_SIZE)
				{
					ibeg++;
					beg_trunc = true;
				}
			}
		}

		/* truncate working copy at desired endpoint */
		wquery[qidx[iend]] = '\0';

		/* Begin building the finished message. */
		i = msg->len;
		appendPQExpBuffer(msg, libpq_gettext("LINE %d: "), loc_line);
		if (beg_trunc)
			appendPQExpBufferStr(msg, "...");

		/*
		 * While we have the prefix in the msg buffer, compute its screen
		 * width.
		 */
		scroffset = 0;
		for (; i < msg->len; i += pg_encoding_mblen(encoding, &msg->data[i]))
		{
			int			w = pg_encoding_dsplen(encoding, &msg->data[i]);

			if (w <= 0)
				w = 1;
			scroffset += w;
		}

		/* Finish up the LINE message line. */
		appendPQExpBufferStr(msg, &wquery[qidx[ibeg]]);
		if (end_trunc)
			appendPQExpBufferStr(msg, "...");
		appendPQExpBufferChar(msg, '\n');

		/* Now emit the cursor marker line. */
		scroffset += scridx[loc] - scridx[ibeg];
		for (i = 0; i < scroffset; i++)
			appendPQExpBufferChar(msg, ' ');
		appendPQExpBufferChar(msg, '^');
		appendPQExpBufferChar(msg, '\n');
	}

	/* Clean up. */
	free(scridx);
	free(qidx);
	free(wquery);
}


/*
 * Attempt to read a ParameterStatus message.
 * This is possible in several places, so we break it out as a subroutine.
 * Entry: 'S' message type and length have already been consumed.
 * Exit: returns 0 if successfully consumed message.
 *		 returns EOF if not enough data.
 */
static int
getParameterStatus(PGconn *conn)
{
	PQExpBufferData valueBuf;

	/* Get the parameter name */
	if (pqGets(&conn->workBuffer, conn))
		return EOF;
	/* Get the parameter value (could be large) */
	initPQExpBuffer(&valueBuf);
	if (pqGets(&valueBuf, conn))
	{
		termPQExpBuffer(&valueBuf);
		return EOF;
	}
	/* And save it */
	pqSaveParameterStatus(conn, conn->workBuffer.data, valueBuf.data);
	termPQExpBuffer(&valueBuf);
	return 0;
}


/*
 * Attempt to read a Notify response message.
 * This is possible in several places, so we break it out as a subroutine.
 * Entry: 'A' message type and length have already been consumed.
 * Exit: returns 0 if successfully consumed Notify message.
 *		 returns EOF if not enough data.
 */
static int
getNotify(PGconn *conn)
{
	int			be_pid;
	char	   *svname;
	int			nmlen;
	int			extralen;
	PGnotify   *newNotify;

	if (pqGetInt(&be_pid, 4, conn))
		return EOF;
	if (pqGets(&conn->workBuffer, conn))
		return EOF;
	/* must save name while getting extra string */
	svname = strdup(conn->workBuffer.data);
	if (!svname)
		return EOF;
	if (pqGets(&conn->workBuffer, conn))
	{
		free(svname);
		return EOF;
	}

	/*
	 * Store the strings right after the PQnotify structure so it can all be
	 * freed at once.  We don't use NAMEDATALEN because we don't want to tie
	 * this interface to a specific server name length.
	 */
	nmlen = strlen(svname);
	extralen = strlen(conn->workBuffer.data);
	newNotify = (PGnotify *) malloc(sizeof(PGnotify) + nmlen + extralen + 2);
	if (newNotify)
	{
		newNotify->relname = (char *) newNotify + sizeof(PGnotify);
		strcpy(newNotify->relname, svname);
		newNotify->extra = newNotify->relname + nmlen + 1;
		strcpy(newNotify->extra, conn->workBuffer.data);
		newNotify->be_pid = be_pid;
		newNotify->next = NULL;
		if (conn->notifyTail)
			conn->notifyTail->next = newNotify;
		else
			conn->notifyHead = newNotify;
		conn->notifyTail = newNotify;
	}

	free(svname);
	return 0;
}

/*
 * getCopyStart - process CopyInResponse, CopyOutResponse or
 * CopyBothResponse message
 *
 * parseInput already read the message type and length.
 */
static int
getCopyStart(PGconn *conn, ExecStatusType copytype)
{
	PGresult   *result;
	int			nfields;
	int			i;

	result = PQmakeEmptyPGresult(conn, copytype);
	if (!result)
		goto failure;

	if (pqGetc(&conn->copy_is_binary, conn))
		goto failure;
	result->binary = conn->copy_is_binary;
	/* the next two bytes are the number of fields	*/
	if (pqGetInt(&(result->numAttributes), 2, conn))
		goto failure;
	nfields = result->numAttributes;

	/* allocate space for the attribute descriptors */
	if (nfields > 0)
	{
		result->attDescs = (PGresAttDesc *)
			pqResultAlloc(result, nfields * sizeof(PGresAttDesc), TRUE);
		if (!result->attDescs)
			goto failure;
		MemSet(result->attDescs, 0, nfields * sizeof(PGresAttDesc));
	}

	for (i = 0; i < nfields; i++)
	{
		int			format;

		if (pqGetInt(&format, 2, conn))
			goto failure;

		/*
		 * Since pqGetInt treats 2-byte integers as unsigned, we need to
		 * coerce these results to signed form.
		 */
		format = (int) ((int16) format);
		result->attDescs[i].format = format;
	}

	/* Success! */
	conn->result = result;
	return 0;

failure:
	PQclear(result);
	return EOF;
}

/*
 * getReadyForQuery - process ReadyForQuery message
 */
static int
getReadyForQuery(PGconn *conn)
{
	char		xact_status;

	if (pqGetc(&xact_status, conn))
		return EOF;
	switch (xact_status)
	{
		case 'I':
			conn->xactStatus = PQTRANS_IDLE;
			break;
		case 'T':
			conn->xactStatus = PQTRANS_INTRANS;
			break;
		case 'E':
			conn->xactStatus = PQTRANS_INERROR;
			break;
		default:
			conn->xactStatus = PQTRANS_UNKNOWN;
			break;
	}

	return 0;
}

/*
 * getCopyDataMessage - fetch next CopyData message, process async messages
 *
 * Returns length word of CopyData message (> 0), or 0 if no complete
 * message available, -1 if end of copy, -2 if error.
 */
static int
getCopyDataMessage(PGconn *conn)
{
	char		id;
	int			msgLength;
	int			avail;

	for (;;)
	{
		/*
		 * Do we have the next input message?  To make life simpler for async
		 * callers, we keep returning 0 until the next message is fully
		 * available, even if it is not Copy Data.
		 */
		conn->inCursor = conn->inStart;
		if (pqGetc(&id, conn))
			return 0;
		if (pqGetInt(&msgLength, 4, conn))
			return 0;
		if (msgLength < 4)
		{
			handleSyncLoss(conn, id, msgLength);
			return -2;
		}
		avail = conn->inEnd - conn->inCursor;
		if (avail < msgLength - 4)
		{
			/*
			 * Before returning, enlarge the input buffer if needed to hold
			 * the whole message.  See notes in parseInput.
			 */
			if (pqCheckInBufferSpace(conn->inCursor + (size_t) msgLength - 4,
									 conn))
			{
				/*
				 * XXX add some better recovery code... plan is to skip over
				 * the message using its length, then report an error. For the
				 * moment, just treat this like loss of sync (which indeed it
				 * might be!)
				 */
				handleSyncLoss(conn, id, msgLength);
				return -2;
			}
			return 0;
		}

		/*
		 * If it's a legitimate async message type, process it.  (NOTIFY
		 * messages are not currently possible here, but we handle them for
		 * completeness.)  Otherwise, if it's anything except Copy Data,
		 * report end-of-copy.
		 */
		switch (id)
		{
			case 'A':			/* NOTIFY */
				if (getNotify(conn))
					return 0;
				break;
			case 'N':			/* NOTICE */
				if (pqGetErrorNotice3(conn, false))
					return 0;
				break;
			case 'S':			/* ParameterStatus */
				if (getParameterStatus(conn))
					return 0;
				break;
			case 'd':			/* Copy Data, pass it back to caller */
				return msgLength;
			default:			/* treat as end of copy */
				return -1;
		}

		/* Drop the processed message and loop around for another */
		conn->inStart = conn->inCursor;
	}
}

/*
 * PQgetCopyData - read a row of data from the backend during COPY OUT
 * or COPY BOTH
 *
 * If successful, sets *buffer to point to a malloc'd row of data, and
 * returns row length (always > 0) as result.
 * Returns 0 if no row available yet (only possible if async is true),
 * -1 if end of copy (consult PQgetResult), or -2 if error (consult
 * PQerrorMessage).
 */
int
pqGetCopyData3(PGconn *conn, char **buffer, int async)
{
	int			msgLength;

	for (;;)
	{
		/*
		 * Collect the next input message.	To make life simpler for async
		 * callers, we keep returning 0 until the next message is fully
		 * available, even if it is not Copy Data.
		 */
		msgLength = getCopyDataMessage(conn);
		if (msgLength < 0)
		{
			/*
			 * On end-of-copy, exit COPY_OUT or COPY_BOTH mode and let caller
			 * read status with PQgetResult().	The normal case is that it's
			 * Copy Done, but we let parseInput read that.	If error, we
			 * expect the state was already changed.
			 */
			if (msgLength == -1)
				conn->asyncStatus = PGASYNC_BUSY;
			return msgLength;	/* end-of-copy or error */
		}
		if (msgLength == 0)
		{
			/* Don't block if async read requested */
			if (async)
				return 0;
			/* Need to load more data */
			if (pqWait(TRUE, FALSE, conn) ||
				pqReadData(conn) < 0)
				return -2;
			continue;
		}

		/*
		 * Drop zero-length messages (shouldn't happen anyway).  Otherwise
		 * pass the data back to the caller.
		 */
		msgLength -= 4;
		if (msgLength > 0)
		{
			*buffer = (char *) malloc(msgLength + 1);
			if (*buffer == NULL)
			{
				printfPQExpBuffer(&conn->errorMessage,
								  libpq_gettext("out of memory\n"));
				return -2;
			}
			memcpy(*buffer, &conn->inBuffer[conn->inCursor], msgLength);
			(*buffer)[msgLength] = '\0';		/* Add terminating null */

			/* Mark message consumed */
			conn->inStart = conn->inCursor + msgLength;

			return msgLength;
		}

		/* Empty, so drop it and loop around for another */
		conn->inStart = conn->inCursor;
	}
}

/*
 * PQgetline - gets a newline-terminated string from the backend.
 *
 * See fe-exec.c for documentation.
 */
int
pqGetline3(PGconn *conn, char *s, int maxlen)
{
	int			status;

	if (conn->sock < 0 ||
		conn->asyncStatus != PGASYNC_COPY_OUT ||
		conn->copy_is_binary)
	{
		printfPQExpBuffer(&conn->errorMessage,
					  libpq_gettext("PQgetline: not doing text COPY OUT\n"));
		*s = '\0';
		return EOF;
	}

	while ((status = PQgetlineAsync(conn, s, maxlen - 1)) == 0)
	{
		/* need to load more data */
		if (pqWait(TRUE, FALSE, conn) ||
			pqReadData(conn) < 0)
		{
			*s = '\0';
			return EOF;
		}
	}

	if (status < 0)
	{
		/* End of copy detected; gin up old-style terminator */
		strcpy(s, "\\.");
		return 0;
	}

	/* Add null terminator, and strip trailing \n if present */
	if (s[status - 1] == '\n')
	{
		s[status - 1] = '\0';
		return 0;
	}
	else
	{
		s[status] = '\0';
		return 1;
	}
}

/*
 * PQgetlineAsync - gets a COPY data row without blocking.
 *
 * See fe-exec.c for documentation.
 */
int
pqGetlineAsync3(PGconn *conn, char *buffer, int bufsize)
{
	int			msgLength;
	int			avail;

	if (conn->asyncStatus != PGASYNC_COPY_OUT)
		return -1;				/* we are not doing a copy... */

	/*
	 * Recognize the next input message.  To make life simpler for async
	 * callers, we keep returning 0 until the next message is fully available
	 * even if it is not Copy Data.  This should keep PQendcopy from blocking.
	 * (Note: unlike pqGetCopyData3, we do not change asyncStatus here.)
	 */
	msgLength = getCopyDataMessage(conn);
	if (msgLength < 0)
		return -1;				/* end-of-copy or error */
	if (msgLength == 0)
		return 0;				/* no data yet */

	/*
	 * Move data from libpq's buffer to the caller's.  In the case where a
	 * prior call found the caller's buffer too small, we use
	 * conn->copy_already_done to remember how much of the row was already
	 * returned to the caller.
	 */
	conn->inCursor += conn->copy_already_done;
	avail = msgLength - 4 - conn->copy_already_done;
	if (avail <= bufsize)
	{
		/* Able to consume the whole message */
		memcpy(buffer, &conn->inBuffer[conn->inCursor], avail);
		/* Mark message consumed */
		conn->inStart = conn->inCursor + avail;
		/* Reset state for next time */
		conn->copy_already_done = 0;
		return avail;
	}
	else
	{
		/* We must return a partial message */
		memcpy(buffer, &conn->inBuffer[conn->inCursor], bufsize);
		/* The message is NOT consumed from libpq's buffer */
		conn->copy_already_done += bufsize;
		return bufsize;
	}
}

/*
 * PQendcopy
 *
 * See fe-exec.c for documentation.
 */
int
pqEndcopy3(PGconn *conn)
{
	PGresult   *result;

	if (conn->asyncStatus != PGASYNC_COPY_IN &&
		conn->asyncStatus != PGASYNC_COPY_OUT)
	{
		printfPQExpBuffer(&conn->errorMessage,
						  libpq_gettext("no COPY in progress\n"));
		return 1;
	}

	/* Send the CopyDone message if needed */
	if (conn->asyncStatus == PGASYNC_COPY_IN)
	{
		if (pqPutMsgStart('c', false, conn) < 0 ||
			pqPutMsgEnd(conn) < 0)
			return 1;

		/*
		 * If we sent the COPY command in extended-query mode, we must issue a
		 * Sync as well.
		 */
		if (conn->queryclass != PGQUERY_SIMPLE)
		{
			if (pqPutMsgStart('S', false, conn) < 0 ||
				pqPutMsgEnd(conn) < 0)
				return 1;
		}
	}

	/*
	 * make sure no data is waiting to be sent, abort if we are non-blocking
	 * and the flush fails
	 */
	if (pqFlush(conn) && pqIsnonblocking(conn))
		return 1;

	/* Return to active duty */
	conn->asyncStatus = PGASYNC_BUSY;
	resetPQExpBuffer(&conn->errorMessage);

	/*
	 * Non blocking connections may have to abort at this point.  If everyone
	 * played the game there should be no problem, but in error scenarios the
	 * expected messages may not have arrived yet.	(We are assuming that the
	 * backend's packetizing will ensure that CommandComplete arrives along
	 * with the CopyDone; are there corner cases where that doesn't happen?)
	 */
	if (pqIsnonblocking(conn) && PQisBusy(conn))
		return 1;

	/* Wait for the completion response */
	result = PQgetResult(conn);

	/* Expecting a successful result */
	if (result && result->resultStatus == PGRES_COMMAND_OK)
	{
		PQclear(result);
		return 0;
	}

	/*
	 * Trouble. For backwards-compatibility reasons, we issue the error
	 * message as if it were a notice (would be nice to get rid of this
	 * silliness, but too many apps probably don't handle errors from
	 * PQendcopy reasonably).  Note that the app can still obtain the error
	 * status from the PGconn object.
	 */
	if (conn->errorMessage.len > 0)
	{
		/* We have to strip the trailing newline ... pain in neck... */
		char		svLast = conn->errorMessage.data[conn->errorMessage.len - 1];

		if (svLast == '\n')
			conn->errorMessage.data[conn->errorMessage.len - 1] = '\0';
		pqInternalNotice(&conn->noticeHooks, "%s", conn->errorMessage.data);
		conn->errorMessage.data[conn->errorMessage.len - 1] = svLast;
	}

	PQclear(result);

	return 1;
}


/*
 * PQfn - Send a function call to the POSTGRES backend.
 *
 * See fe-exec.c for documentation.
 */
PGresult *
pqFunctionCall3(PGconn *conn, Oid fnid,
				int *result_buf, int *actual_result_len,
				int result_is_int,
				const PQArgBlock *args, int nargs)
{
	bool		needInput = false;
	ExecStatusType status = PGRES_FATAL_ERROR;
	char		id;
	int			msgLength;
	int			avail;
	int			i;

	/* PQfn already validated connection state */

	if (pqPutMsgStart('F', false, conn) < 0 ||	/* function call msg */
		pqPutInt(fnid, 4, conn) < 0 ||	/* function id */
		pqPutInt(1, 2, conn) < 0 ||		/* # of format codes */
		pqPutInt(1, 2, conn) < 0 ||		/* format code: BINARY */
		pqPutInt(nargs, 2, conn) < 0)	/* # of args */
	{
		pqHandleSendFailure(conn);
		return NULL;
	}

	for (i = 0; i < nargs; ++i)
	{							/* len.int4 + contents	   */
		if (pqPutInt(args[i].len, 4, conn))
		{
			pqHandleSendFailure(conn);
			return NULL;
		}
		if (args[i].len == -1)
			continue;			/* it's NULL */

		if (args[i].isint)
		{
			if (pqPutInt(args[i].u.integer, args[i].len, conn))
			{
				pqHandleSendFailure(conn);
				return NULL;
			}
		}
		else
		{
			if (pqPutnchar((char *) args[i].u.ptr, args[i].len, conn))
			{
				pqHandleSendFailure(conn);
				return NULL;
			}
		}
	}

	if (pqPutInt(1, 2, conn) < 0)		/* result format code: BINARY */
	{
		pqHandleSendFailure(conn);
		return NULL;
	}

	if (pqPutMsgEnd(conn) < 0 ||
		pqFlush(conn))
	{
		pqHandleSendFailure(conn);
		return NULL;
	}

	for (;;)
	{
		if (needInput)
		{
			/* Wait for some data to arrive (or for the channel to close) */
			if (pqWait(TRUE, FALSE, conn) ||
				pqReadData(conn) < 0)
				break;
		}

		/*
		 * Scan the message. If we run out of data, loop around to try again.
		 */
		needInput = true;

		conn->inCursor = conn->inStart;
		if (pqGetc(&id, conn))
			continue;
		if (pqGetInt(&msgLength, 4, conn))
			continue;

		/*
		 * Try to validate message type/length here.  A length less than 4 is
		 * definitely broken.  Large lengths should only be believed for a few
		 * message types.
		 */
		if (msgLength < 4)
		{
			handleSyncLoss(conn, id, msgLength);
			break;
		}
		if (msgLength > 30000 && !VALID_LONG_MESSAGE_TYPE(id))
		{
			handleSyncLoss(conn, id, msgLength);
			break;
		}

		/*
		 * Can't process if message body isn't all here yet.
		 */
		msgLength -= 4;
		avail = conn->inEnd - conn->inCursor;
		if (avail < msgLength)
		{
			/*
			 * Before looping, enlarge the input buffer if needed to hold the
			 * whole message.  See notes in parseInput.
			 */
			if (pqCheckInBufferSpace(conn->inCursor + (size_t) msgLength,
									 conn))
			{
				/*
				 * XXX add some better recovery code... plan is to skip over
				 * the message using its length, then report an error. For the
				 * moment, just treat this like loss of sync (which indeed it
				 * might be!)
				 */
				handleSyncLoss(conn, id, msgLength);
				break;
			}
			continue;
		}

		/*
		 * We should see V or E response to the command, but might get N
		 * and/or A notices first. We also need to swallow the final Z before
		 * returning.
		 */
		switch (id)
		{
			case 'V':			/* function result */
				if (pqGetInt(actual_result_len, 4, conn))
					continue;
				if (*actual_result_len != -1)
				{
					if (result_is_int)
					{
						if (pqGetInt(result_buf, *actual_result_len, conn))
							continue;
					}
					else
					{
						if (pqGetnchar((char *) result_buf,
									   *actual_result_len,
									   conn))
							continue;
					}
				}
				/* correctly finished function result message */
				status = PGRES_COMMAND_OK;
				break;
			case 'E':			/* error return */
				if (pqGetErrorNotice3(conn, true))
					continue;
				status = PGRES_FATAL_ERROR;
				break;
			case 'A':			/* notify message */
				/* handle notify and go back to processing return values */
				if (getNotify(conn))
					continue;
				break;
			case 'N':			/* notice */
				/* handle notice and go back to processing return values */
				if (pqGetErrorNotice3(conn, false))
					continue;
				break;
			case 'Z':			/* backend is ready for new query */
				if (getReadyForQuery(conn))
					continue;
				/* consume the message and exit */
				conn->inStart += 5 + msgLength;
				/* if we saved a result object (probably an error), use it */
				if (conn->result)
					return pqPrepareAsyncResult(conn);
				return PQmakeEmptyPGresult(conn, status);
			case 'S':			/* parameter status */
				if (getParameterStatus(conn))
					continue;
				break;
			default:
				/* The backend violates the protocol. */
				printfPQExpBuffer(&conn->errorMessage,
								  libpq_gettext("protocol error: id=0x%x\n"),
								  id);
				pqSaveErrorResult(conn);
				/* trust the specified message length as what to skip */
				conn->inStart += 5 + msgLength;
				return pqPrepareAsyncResult(conn);
		}
		/* Completed this message, keep going */
		/* trust the specified message length as what to skip */
		conn->inStart += 5 + msgLength;
		needInput = false;
	}

	/*
	 * We fall out of the loop only upon failing to read data.
	 * conn->errorMessage has been set by pqWait or pqReadData. We want to
	 * append it to any already-received error message.
	 */
	pqSaveErrorResult(conn);
	return pqPrepareAsyncResult(conn);
}


/*
 * Construct startup packet
 *
 * Returns a malloc'd packet buffer, or NULL if out of memory
 */
char *
pqBuildStartupPacket3(PGconn *conn, int *packetlen,
					  const PQEnvironmentOption *options)
{
	char	   *startpacket;

	*packetlen = build_startup_packet(conn, NULL, options);
	startpacket = (char *) malloc(*packetlen);
	if (!startpacket)
		return NULL;
	*packetlen = build_startup_packet(conn, startpacket, options);
	return startpacket;
}

/*
 * Build a startup packet given a filled-in PGconn structure.
 *
 * We need to figure out how much space is needed, then fill it in.
 * To avoid duplicate logic, this routine is called twice: the first time
 * (with packet == NULL) just counts the space needed, the second time
 * (with packet == allocated space) fills it in.  Return value is the number
 * of bytes used.
 */
static int
build_startup_packet(const PGconn *conn, char *packet,
					 const PQEnvironmentOption *options)
{
	int			packet_len = 0;
	const PQEnvironmentOption *next_eo;
	const char *val;

	/* Protocol version comes first. */
	if (packet)
	{
		ProtocolVersion pv = htonl(conn->pversion);

		memcpy(packet + packet_len, &pv, sizeof(ProtocolVersion));
	}
	packet_len += sizeof(ProtocolVersion);

	/* Add user name, database name, options */

#define ADD_STARTUP_OPTION(optname, optval) \
	do { \
		if (packet) \
			strcpy(packet + packet_len, optname); \
		packet_len += strlen(optname) + 1; \
		if (packet) \
			strcpy(packet + packet_len, optval); \
		packet_len += strlen(optval) + 1; \
	} while(0)

	if (conn->pguser && conn->pguser[0])
		ADD_STARTUP_OPTION("user", conn->pguser);
	if (conn->dbName && conn->dbName[0])
		ADD_STARTUP_OPTION("database", conn->dbName);
	if (conn->replication && conn->replication[0])
		ADD_STARTUP_OPTION("replication", conn->replication);
	if (conn->pgoptions && conn->pgoptions[0])
		ADD_STARTUP_OPTION("options", conn->pgoptions);
	if (conn->send_appname)
	{
		/* Use appname if present, otherwise use fallback */
		val = conn->appname ? conn->appname : conn->fbappname;
		if (val && val[0])
			ADD_STARTUP_OPTION("application_name", val);
	}

	if (conn->client_encoding_initial && conn->client_encoding_initial[0])
		ADD_STARTUP_OPTION("client_encoding", conn->client_encoding_initial);

	/* Add any environment-driven GUC settings needed */
	for (next_eo = options; next_eo->envName; next_eo++)
	{
		if ((val = getenv(next_eo->envName)) != NULL)
		{
			if (pg_strcasecmp(val, "default") != 0)
				ADD_STARTUP_OPTION(next_eo->pgName, val);
		}
	}

	/* Add trailing terminator */
	if (packet)
		packet[packet_len] = '\0';
	packet_len++;

	return packet_len;
}
