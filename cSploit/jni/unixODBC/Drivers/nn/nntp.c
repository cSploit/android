/**
    Copyright (C) 1995, 1996 by Ke Jin <kejin@visigenic.com>
	Enhanced for unixODBC (1999) by Peter Harvey <pharvey@codebydesign.com>
	
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
**/

#include <config.h>
#include	<nnconfig.h>

#include    <string.h>
#include	<errno.h>
#include	<nntp.h>

# include	"nntp.ci"

#ifndef WINSOCK

# include	<sys/types.h>
# include	<sys/socket.h>
# include	<netdb.h>
# include	<netinet/in.h>

#else

# include	<winsock.h>

#endif

#include	<nntp.h>

typedef struct {
	long	article_num;
	long	data;
} nntp_xhdridx_t;

typedef struct {
	char*		header;
	long		start;
	long		end;
	int		count;
	nntp_xhdridx_t* idxs;
	char*	buf;
} nntp_xhdrinfo_t;

typedef struct {
	void*	sin;
	void*	sout;
	int	postok;
	int	code;

	/* group info */
	long	start;
	long	end;
	int	count;

	/* access mode */
	int	mode;
} nntp_cndes_t;

void*	nntp_connect( char*	server )
{
	int		sock, err;
	char		msgbuf[128];
	struct sockaddr_in
			srvaddr;
	nntp_cndes_t*	cndes;

#ifdef	WINSOCK
	WSADATA winsock_data;

# ifndef	WINSOCKVERSION
#  define	WINSOCKVERSION	( 0x0101 )		/* default version 1.1 */
# endif

	if( WSAStartup((WORD)WINSOCKVERSION, &winsock_data) )
		return 0;	/* fail to init winsock */
#endif

	if( atoi(server) > 0 )
	/* an IP address */
	{
		srvaddr.sin_family = AF_INET;
		srvaddr.sin_addr.s_addr = inet_addr(server);
	}
	else
	/* a domain name */
	{
		struct hostent* ph;

		if( ! (ph = gethostbyname( server )) )
			return 0;

		srvaddr.sin_family = ph->h_addrtype;
		memcpy( (char*)&srvaddr.sin_addr,
			(char*)ph->h_addr, ph->h_length);
	}

	cndes = (nntp_cndes_t*)MEM_ALLOC(sizeof(nntp_cndes_t));

	if( ! cndes )
		return 0;

	srvaddr.sin_port = htons(119);

	if( (sock = socket( AF_INET, SOCK_STREAM, 0)) == -1 )
	{
		MEM_FREE(cndes);
		return 0;
	}

	if( connect( sock, (struct sockaddr*)&srvaddr, sizeof(srvaddr) ) == -1 )
	{
		SOCK_CLOSE( sock );
		MEM_FREE(cndes);

		return 0;
	}

	if( ! (cndes->sin = SOCK_FDOPEN( sock, "r")) )
	{
		SOCK_CLOSE( sock );
		MEM_FREE(cndes);

		return 0;
	}

	if( ! (cndes->sout = SOCK_FDOPEN( sock, "w")) )
	{
		SOCK_FCLOSE( cndes->sin );
		MEM_FREE(cndes);

		return 0;
	}

	if( ! SOCK_FGETS(msgbuf, sizeof(msgbuf),  cndes->sin ) )
	{
		SOCK_FCLOSE( cndes->sin );
		SOCK_FCLOSE( cndes->sout );
		MEM_FREE(cndes);

		return 0;
	}

	/* a patch from Julia Anne Case <julie@magenet.com> */
	SOCK_FPUTS("MODE READER\r\n", cndes->sout);
	if( SOCK_FFLUSH(cndes->sout) == EOF )
		return 0;

	if( ! SOCK_FGETS(msgbuf, sizeof(msgbuf),  cndes->sin ) )
	{
		SOCK_FCLOSE( cndes->sin );
		SOCK_FCLOSE( cndes->sout );
		MEM_FREE(cndes);

		return 0;
	}

	switch( atoi( msgbuf ) )
	{
		case NNTP_POST_CONN_OK:
			cndes->postok = 1;
			break;

		case NNTP_READ_CONN_OK:
			cndes->postok = 0;
			break;

		default:
			SOCK_FCLOSE( cndes->sin );
			SOCK_FCLOSE( cndes->sout );
			MEM_FREE(cndes);
			return 0;
	}

	cndes->code = 0;

	/* group info */
	cndes->start= 0L;
	cndes->end  = 0L;
	cndes->count= 0;
	cndes->mode = 0;

	return (void*)cndes;
}

void	nntp_close(void* hcndes)
{
	nntp_cndes_t*	pcndes = (nntp_cndes_t*)hcndes;
	char		msgbuf[128];

	SOCK_FPUTS( "QUIT\r\n", pcndes->sout );
	SOCK_FFLUSH( pcndes->sout );

	SOCK_FGETS(msgbuf, sizeof(msgbuf), pcndes->sin );

	SOCK_FCLOSE( pcndes->sin );
	SOCK_FCLOSE( pcndes->sout );

	MEM_FREE( hcndes );

#ifdef	WINSOCK
	WSACleanup();
#endif
}

int	nntp_errcode( void* hcndes )
{
	if( ! hcndes )
		return -1;

	return ((nntp_cndes_t*)hcndes)->code;
}

char*	nntp_errmsg( void* hcndes )
{
	int	i, errcode;

	errcode = nntp_errcode( hcndes );

	switch( errcode )
	{
		case 0:
			return 0;

		case -1:
			return strerror( errno );

		default:
			break;
	}

	for(i=0;i<sizeof(nntp_msg)/sizeof(nntp_msg[0]);i++)
	{
		if( nntp_msg[i].code == errcode )
			return nntp_msg[i].msg;
	}

	return 0;
}

int	nntp_postok( void* hcndes )
{
	return ((nntp_cndes_t*)hcndes)->postok;
}

int	nntp_group( void* hcndes, char* grpnam )
{
	nntp_cndes_t*	pcndes	= hcndes;
	char	response[64];
	int	code;

	pcndes->code = -1;	/* system error */

	SOCK_FPRINTF( pcndes->sout, "GROUP %s\r\n", grpnam);
	if( SOCK_FFLUSH( pcndes->sout ) == EOF )
		return -1;

	if( ! SOCK_FGETS( response, sizeof( response ), pcndes->sin ) )
		return -1;

	code = atoi(response);

	if( code != NNTP_GROUP_OK )
	{
		pcndes->code = code;

		return -1;
	}

	sscanf( response, "%d%d%ld%ld",
		&code, &(pcndes->count),
		&(pcndes->start), &(pcndes->end));

	pcndes->code = 0;

	return 0;
}

char*	nntp_body( void* hcndes, long msgnum, char* msgid)
{
	nntp_cndes_t*	pcndes = hcndes;
	char	tmsgbuf[128];
	int	code;
	char*	body;
	int	totsize, freesize, offset;

	pcndes->code = -1;

	if( msgnum > 0 )
		SOCK_FPRINTF(pcndes->sout, "BODY %ld\r\n", msgnum );
	else if( msgid )
		SOCK_FPRINTF(pcndes->sout, "BODY %s\r\n", msgid );
	else
	{
		SOCK_FPUTS("BODY\r\n", pcndes->sout);
	}

	if( SOCK_FFLUSH( pcndes->sout ) == EOF )
		return 0;

	if( ! SOCK_FGETS(tmsgbuf, sizeof(tmsgbuf), pcndes->sin ) )
		return 0;

	code = atoi(tmsgbuf);

	if( code != NNTP_BODY_OK )
	{
		pcndes->code = code;
		return 0;
	}

	body = MEM_ALLOC(BODY_CHUNK_SIZE);

	if( !body )
		abort();

	totsize = freesize = BODY_CHUNK_SIZE;
	offset = 0;

	for(;;)
	{
		if( freesize <= BODY_CHUNK_SIZE/2 )
		{
			totsize += BODY_CHUNK_SIZE;
			freesize += BODY_CHUNK_SIZE;
			body = MEM_REALLOC(body, totsize);

			if( ! body )
				abort();
		}

		if( ! SOCK_FGETS( body + offset, freesize, pcndes->sin ) )
			return 0;

		if( STREQ( body+offset, ".\r\n") )
		{
			*(body+offset) = 0;
			break;
		}

		/* strip off CR i.e '\r' */
		offset += STRLEN(body+offset) - 1;
		freesize = totsize - offset;
		body[offset-1] = '\n';
	}

	return body;
}

int	nntp_next( void* hcndes )
{
	nntp_cndes_t*	pcndes = hcndes;
	char	tmsgbuf[128];
	int	code;

	pcndes->code = -1;

	SOCK_FPUTS("NEXT\r\n", pcndes->sout);

	if( SOCK_FFLUSH( pcndes->sout ) == EOF )
		return -1;

	if( ! SOCK_FGETS( tmsgbuf, sizeof(tmsgbuf), pcndes->sin) )
		return -1;

	pcndes->code = atoi(tmsgbuf);

	switch( pcndes->code )
	{
		case NNTP_END_OF_GROUP:
			return 100;

		case NNTP_NEXT_OK:
			return 0;

		default:
			break;
	}

	return -1;
}

int	nntp_last( void* hcndes )
{
	nntp_cndes_t*	pcndes = hcndes;

	char	tmsgbuf[128];
	int	code, sock;
	char*	body;
	int	size;

	pcndes->code = -1;

	SOCK_FPUTS("LAST\r\n", pcndes->sout);

	if( SOCK_FFLUSH( pcndes->sout ) == EOF )
		return -1;

	if( ! SOCK_FGETS( tmsgbuf, sizeof(tmsgbuf), pcndes->sin) )
		return -1;

	pcndes->code = atoi(tmsgbuf);

	switch( pcndes->code )
	{
		case NNTP_TOP_OF_GROUP:
			return 100;

		case NNTP_LAST_OK:
			return 0;

		default:
			break;
	}

	return -1;
}

static	int	nntp_xhdr( void* hcndes, nntp_xhdrinfo_t* xhdr_data)
{
	nntp_cndes_t*	pcndes = hcndes;
	char		tbuf[128];
	int		totsize, freesize;
	int		flag;
	char*		ptr;

	pcndes->code = -1;

	xhdr_data->count = 0;

	SOCK_FPRINTF( pcndes->sout, "XHDR %s %ld-%ld\r\n",
		xhdr_data->header, xhdr_data->start, xhdr_data->end );

	if( SOCK_FFLUSH( pcndes->sout ) == EOF )
		return -1;

	if( ! SOCK_FGETS( tbuf, sizeof(tbuf), pcndes->sin ) )
		return -1;

	pcndes->code = atoi(tbuf);

	if( pcndes->code != NNTP_XHDR_OK )
		return -1;

	flag = upper_strneq(xhdr_data->header, "lines", 6);

	if( flag )
	{
		xhdr_data->buf = 0;
	}
	else
	{
		totsize = freesize = XHDR_CHUNK_SIZE;

		xhdr_data->buf = MEM_ALLOC(totsize);

		if( ! xhdr_data->buf )
			return -1;

		ptr = xhdr_data->buf;
	}

	for(xhdr_data->count=0;;xhdr_data->count++)
	{
		int	num;

		if( flag )
		{
			if( ! SOCK_FGETS(tbuf, sizeof(tbuf), pcndes->sin) )
				return -1;

			if( STRNEQ(tbuf, ".\r\n", 3) )
				break;

			sscanf(tbuf, "%ld%ld",
				&(xhdr_data->idxs[xhdr_data->count].article_num),
				&(xhdr_data->idxs[xhdr_data->count].data) );
		}
		else
		{
			if( freesize < XHDR_CHUNK_SIZE/2 )
			{
				int	offset;

				totsize += XHDR_CHUNK_SIZE;
				freesize += XHDR_CHUNK_SIZE;

				offset = (int)(ptr - xhdr_data->buf);

				xhdr_data->buf = MEM_REALLOC( xhdr_data->buf, totsize );

				if( ! xhdr_data->buf )
					return -1;

				ptr = xhdr_data->buf + offset;
			}

			if( ! SOCK_FGETS(ptr, freesize, pcndes->sin ) )
				return -1;

			if( STRNEQ(ptr, ".\r\n", 3) )
				break;

			sscanf( ptr, "%ld%n",
				&(xhdr_data->idxs[xhdr_data->count].article_num),
				&num);

			if( STREQ( ptr + num + 1, "(none)\r\n") )
			{
				xhdr_data->idxs[xhdr_data->count].data = 0;

				ptr += (num + 1);
			}
			else
			{
				xhdr_data->idxs[xhdr_data->count].data =
						ptr - xhdr_data->buf + num + 1;

				ptr += (STRLEN(ptr) - 1);
			}

			ptr[-1] = 0;

			freesize = totsize - (int)(ptr - xhdr_data->buf);
		}
	}

	return 0;
}

typedef struct {
	void*			hcndes;
	char			header[20];

	nntp_xhdrinfo_t*	hdrinfo;
	long			position;
	long			last;
} nntp_header_t;

void*	nntp_openheader(void* hcndes, char* header, long* tmin, long* tmax)
{
	nntp_cndes_t*	pcndes = hcndes;
	nntp_header_t*	hd;
	long		start;

	pcndes->code = -1;

	hd = (nntp_header_t*)MEM_ALLOC(sizeof(nntp_header_t));

	if( !hd )
		return 0;

	hd->hcndes = hcndes;
	STRCPY(hd->header, header);
	hd->last = pcndes->end;

	hd->hdrinfo = (nntp_xhdrinfo_t*)MEM_ALLOC(sizeof(nntp_xhdrinfo_t));

	if( !hd->hdrinfo )
	{
		MEM_FREE( hd );
		return 0;
	}

	start = pcndes->start;

	if( *tmax < *tmin )
	{
#ifndef MAX_SIGNED_LONG
# define MAX_SIGNED_LONG	( (-1UL) >> 1 )
#endif

		if( start < *tmax || start > *tmin )
			*tmin = start;

		*tmax = MAX_SIGNED_LONG;
	}

	if( *tmin < start )
		*tmin = start;

	if( *tmin == MAX_SIGNED_LONG )
		*tmin = *tmax = 0L;

	hd->hdrinfo->header = hd->header;
	hd->hdrinfo->start  = 0L;
	hd->hdrinfo->end    = *tmin - 1L;
	hd->hdrinfo->count  = 0;
	hd->hdrinfo->idxs   = (nntp_xhdridx_t*)MEM_ALLOC(
				sizeof(nntp_xhdridx_t)*NNTP_HEADER_CHUNK);

	if( ! hd->hdrinfo->idxs )
	{
		MEM_FREE( hd->hdrinfo );
		MEM_FREE( hd );
		return 0;
	}

	hd->hdrinfo->buf = 0;
	hd->position = 0;

	return hd;
}

void	nntp_closeheader(void* hh)
{
	nntp_header_t*	ph = hh;

	if( !hh )
		return;

	if( ph->hdrinfo )
	{
		MEM_FREE( ph->hdrinfo->idxs );
		MEM_FREE( ph->hdrinfo->buf );
		MEM_FREE( ph->hdrinfo );
	}

	MEM_FREE( hh );
}

int	nntp_fetchheader( void* hh, long* artnum, long* data, void* hrh)
{
	nntp_header_t*	ph = hh;
	nntp_header_t*	prh= hrh;	/* reference header */
	nntp_cndes_t*	pcndes;
	long		position;
	long		ldata;

	if(! hh )
		return -1;

	pcndes = ph->hcndes;
	position = ph->position;

	pcndes->code = -1;

	if( ph->hdrinfo->start >= ph->last )
			return 100;

	if( prh )
	{
		if( ph->hdrinfo->end != prh->hdrinfo->end )
		{
			MEM_FREE(ph->hdrinfo->buf);
			ph->hdrinfo->buf = 0;

			ph->hdrinfo->start = prh->hdrinfo->start;
			ph->hdrinfo->end   = prh->hdrinfo->end;

			if( nntp_xhdr( pcndes, ph->hdrinfo ) )
			return -1;
		}

		position = ph->position = prh->position-1;
	}
	else if( ph->hdrinfo->count == position )
	{
		MEM_FREE(ph->hdrinfo->buf);
		ph->hdrinfo->buf = 0;

		for(;;)
		{
			ph->hdrinfo->start = ph->hdrinfo->end + 1;
			ph->hdrinfo->end   = ph->hdrinfo->end + NNTP_HEADER_CHUNK;
			ph->hdrinfo->count = 0;
			position = ph->position = 0;

			if( ph->hdrinfo->start > ph->last )
				return 100;

			if( nntp_xhdr( pcndes, ph->hdrinfo ) )
				return -1;

			if( ph->hdrinfo->count )
				break;
		}
	}

	if( artnum )
		*artnum = (ph->hdrinfo->idxs)[position].article_num;

	ldata = (ph->hdrinfo->idxs)[position].data;

	if( ldata )
		ldata = (long)(ph->hdrinfo->buf) + ldata;

	if( data )
		*data = ldata;

	ph->position ++;

	return 0;
}

int	nntp_start_post(void* hcndes)
{
	nntp_cndes_t*	pcndes = hcndes;
	char		msgbuf[128];
	int		i;

	pcndes->code = -1;

	if( ! nntp_postok(hcndes) )
	{
		pcndes->code = NNTP_CANNOT_POST;
		return -1;
	}

	SOCK_FPUTS( "POST\r\n", pcndes->sout );
	if( SOCK_FFLUSH( pcndes->sout ) == EOF )
		return -1;

	if( ! SOCK_FGETS(msgbuf, sizeof(msgbuf), pcndes->sin) )
		return -1;

	pcndes->code = atoi(msgbuf);

	if(pcndes->code != NNTP_POSTING )
		return -1;

	return 0;
}

int	nntp_send_head(void* hcndes, char* head_name, char* head)
{
	nntp_cndes_t*	pcndes = hcndes;
	int		i;

	for(i=0;head[i];i++)
	/* to make sure there is no new line char in a head string
	 * here, head must be pointed to a writeable area and don't
	 * care whether it will be overwritten after posting */
	{
		if( head[i] == '\n' )
		{
			head[i] = 0;
			break;
		}
	}

	SOCK_FPRINTF( pcndes->sout, "%s: %s\n",
		head_name, head);

	return 0;
}

int	nntp_end_head(void* hcndes)
{
	nntp_cndes_t*	pcndes = hcndes;

	SOCK_FPUTS("\n", pcndes->sout);

	return 0;
}

int	nntp_send_body(void* hcndes, char* body)
{
	nntp_cndes_t*	pcndes = hcndes;
	char*		ptr;

	for(ptr=body;*ptr;ptr++)
	/* we require the article body buffer is located in a
	 * writeable area and can be overwritten during the
	 * post action.
	 */
	{
		if( *ptr == '\n' )
		{
			if( STRNEQ( ptr, "\n.\n", 3)
			 || STRNEQ( ptr, "\n.\r\n", 4 ) )
			{
				*ptr = 0;
				break;
			}
		}
	}

	SOCK_FPUTS(body, pcndes->sout);

	return 0;
}

int	nntp_end_post(void* hcndes)
{
	nntp_cndes_t*	pcndes = hcndes;
	char		msgbuf[128];

	pcndes->code = -1;

	SOCK_FPUTS("\r\n.\r\n", pcndes->sout);

	if( SOCK_FFLUSH(pcndes->sout) == EOF )
		return -1;

	if( ! SOCK_FGETS(msgbuf, sizeof(msgbuf), pcndes->sin) )
		return -1;

	pcndes->code = atoi(msgbuf);

	if( pcndes->code != NNTP_POST_OK )
		return -1;

	return 0;
}

int	nntp_cancel(
		void*	hcndes,
		char*	group,
		char*	sender,
		char*	from,
		char*	msgid )
{
	char	msgbuf[128];

	if( ! from )
		from = "(none)";

	sprintf( msgbuf, "cancel %s", msgid);

	if( nntp_start_post(hcndes)
	 || nntp_send_head(hcndes, "Newsgroups", group)
	 || ( sender && nntp_send_head(hcndes, "Sender", sender) )
	 || nntp_send_head(hcndes, "From", from)
	 || nntp_send_head(hcndes, "Control", msgbuf)
	 || nntp_end_head(hcndes)
	 || nntp_end_post(hcndes) )
		return -1;

	return 0;
}

void	nntp_setaccmode(void* hcndes, int mode)
{
	nntp_cndes_t*	pcndes = hcndes;

	pcndes->mode = mode;
}

int	nntp_getaccmode(void* hcndes )
{
	nntp_cndes_t*	pcndes = hcndes;

	return pcndes->mode;
}
