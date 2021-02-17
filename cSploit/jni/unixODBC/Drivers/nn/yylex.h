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
#ifndef _YYLEX_H
# define _YYLEX_H

typedef struct {
	int	idx;
	char*	name;
} keywd_ent_t;

enum {
	en_cmpop_eq,
	en_cmpop_ne,
	en_cmpop_gt,
	en_cmpop_lt,
	en_cmpop_ge,
	en_cmpop_le,

	en_logop_and,
	en_logop_or,
	en_logop_not
};

#endif
