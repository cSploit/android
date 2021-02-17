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
#ifndef _YYERR_H
# define	_YYERR_H

# include	<nnsql.h>

# define	NNSQL_ERRHEAD		"[NetNewSQL][Dynamic SQL parser]"

# define	INVALID_COLUMN_NAME			200
# define	NOT_SUPPORT_UPDATEABLE_CURSOR		201
# define	NOT_SUPPORT_DISTINCT_SELECT		202
# define	NOT_SUPPORT_MULTITABLE_QUERY		203
# define	NOT_SUPPORT_GROUP_CLAUSE		204
# define	NOT_SUPPORT_HAVING_CLAUSE		205
# define	NOT_SUPPORT_ORDER_CLAUSE		206
# define	NOT_SUPPORT_DDL_DCL			207
# define	NOT_SUPPORT_UPDATE			208
# define	NOT_SUPPORT_POSITION_DELETE		209
# define	NOT_SUPPORT_SPROC			210
# define	TOO_MANY_COLUMNS			211
# define	VARIABLE_IN_SELECT_LIST 		212
# define	VARIABLE_IN_TABLE_LIST			213
# define	UNSEARCHABLE_ATTR			214
# define	INSERT_VALUE_LESS			215
# define	INSERT_VALUE_MORE			216
# define	POST_WITHOUT_BODY			217
# define	NO_POST_PRIVILEGE			218
# define	NO_INSERT_PRIVILEGE			219
# define	NO_DELETE_PRIVILEGE			220
# define	NO_DELETE_ANY_PRIVILEGE 		221
# define	TYPE_VIOLATION				222

# define	PARSER_ERROR				256

#endif
