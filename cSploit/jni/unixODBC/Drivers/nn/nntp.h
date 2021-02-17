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
#ifndef _NNTP_H
# define _NNTP_H

/* part of useful nntp response code */
# define NNTP_POST_CONN_OK	200
# define NNTP_READ_CONN_OK	201
# define NNTP_SLAVE_CONN_OK	202
# define NNTP_QUIT_OK		205

# define NNTP_GROUP_OK		211
# define NNTP_LIST_OK		215

# define NNTP_ARTICLE_OK	220
# define NNTP_HEAD_OK		221
# define NNTP_XHDR_OK		NNTP_HEAD_OK
# define NNTP_BODY_OK		222
# define NNTP_NEXT_OK		223
# define NNTP_LAST_OK		NNTP_NEXT_OK

# define NNTP_NEWMSG_OK 	230
# define NNTP_NEWGRP_OK 	231

# define NNTP_POST_OK		240

# define NNTP_POSTING		340

# define NNTP_SERVICE_DOWN	400
# define NNTP_INVALID_GROUP	411
# define NNTP_NOT_IN_GROUP	412
# define NNTP_NOT_IN_ARTICLE	420
# define NNTP_END_OF_GROUP	421
# define NNTP_TOP_OF_GROUP	422
# define NNTP_INVALID_MSGNUM	423
# define NNTP_INVALID_MSGID	430
# define NNTP_UNWANTED_MSG	435
# define NNTP_REJECTED_MSG	437
# define NNTP_CANNOT_POST	440
# define NNTP_POSTING_FAILED	441

# define BODY_CHUNK_SIZE	4096
# define XHDR_CHUNK_SIZE	4096

# define NNTP_HEADER_CHUNK	128

extern void*	nntp_connect	( char* server );
extern void	nntp_close	( void* hcndes );
extern int	nntp_group	( void* hcndes, char* grp);
extern int	nntp_next	( void* hcndes );
extern int	nntp_last	( void* hcndes );
extern int	nntp_errcode	( void* hcndes );
extern char*	nntp_errmsg	( void* hcndes );

extern void*	nntp_openheader ( void* hcndes, char* header, long* tmin, long* tmax );
extern int	nntp_fetchheader( void* hhead, long* article_num, long* data, void* hrh );
extern void	nntp_closeheader( void* hhead );

extern char*	nntp_body	( void* hcndes, long artnum, char* artid);

extern int	nntp_start_post ( void* hcndes );
extern int	nntp_send_head	( void* hcndes, char* header_name, char* header );
extern int	nntp_end_head	( void* hcndes );
extern int	nntp_send_body	( void* hcndes, char* body);
extern int	nntp_end_post	( void* hcndes );

extern int	nntp_cancel	( void* hcndes, char* group, char* sender,
				   char* from, char* msgid);

extern int	nntp_getaccmode( void* hcndes );
extern void	nntp_setaccmode( void* hcndes, int mode );

#endif
