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
#ifndef _NNCOL_H
# define _NNCOL_H

# define	PRIV_NONE	0
# define	PRIV_REFER	1
# define	PRIV_SELECT	2
# define	PRIV_INSERT	3
# define	PRIV_UPDATE	4

enum	{
	en_article_num	= 0,

	en_newsgroups,
	en_subject,
	en_from,
	en_sender,
	en_organization,
	en_summary,
	en_keywords,
	en_expires,
	en_msgid,
	en_references,
	en_followup_to,
	en_reply_to,
	en_distribution,

	en_xref,
	en_host,
	en_date,
	en_path,

	en_x_newsreader,

	en_lines,

	en_body,

	en_sql_count,
	en_sql_qstr,
	en_sql_num,
	en_sql_date
};

enum {
	en_t_null,
	en_t_num,
	en_t_str,
	en_t_date,
	en_t_text
};

char*	nnsql_getcolnamebyidx(int idx);
int	nnsql_getcolidxbyname(char* name);
void*	nnsql_getcoldescbyidx(int idx);
char*	nnsql_getcolnamebydesc(void* desc);
int	nnsql_getcoldatetypebydesc(void* desc);
int	nnsql_getcolnullablebydesc(void* desc);

#endif
