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
#ifndef 	_HERR_H
# define	_HERR_H

enum {
	en_00000 = 0,
	en_01000,
	en_01002,
	en_01004,
	en_01006,
	en_01S00,
	en_01S01,
	en_01S02,
	en_01S03,
	en_01S04,
	en_07001,
	en_07006,
	en_08001,
	en_08002,
	en_08003,
	en_08004,
	en_08007,
	en_08S01,
	en_0A000,
	en_21S01,
	en_21S02,
	en_22001,
	en_22003,
	en_22005,
	en_22008,
	en_22012,
	en_22026,
	en_23000,
	en_24000,
	en_25000,
	en_28000,
	en_34000,
	en_37000,
	en_3C000,
	en_40001,
	en_42000,
	en_70100,
	en_IM001,
	en_IM002,
	en_IM003,
	en_IM004,
	en_IM005,
	en_IM006,
	en_IM007,
	en_IM008,
	en_IM009,
	en_IM010,
	en_IM011,
	en_IM012,
	en_IM013,
	en_IM014,
	en_S0001,
	en_S0002,
	en_S0011,
	en_S0012,
	en_S0021,
	en_S0022,
	en_S0023,
	en_S1000,
	en_S1001,
	en_S1002,
	en_S1003,
	en_S1004,
	en_S1008,
	en_S1009,
	en_S1010,
	en_S1011,
	en_S1012,
	en_S1015,
	en_S1090,
	en_S1091,
	en_S1092,
	en_S1093,
	en_S1094,
	en_S1095,
	en_S1096,
	en_S1097,
	en_S1098,
	en_S1099,
	en_S1100,
	en_S1101,
	en_S1103,
	en_S1104,
	en_S1105,
	en_S1106,
	en_S1107,
	en_S1108,
	en_S1109,
	en_S1110,
	en_S1111,
	en_S1C00,
	en_S1T00
};

extern	void*	nnodbc_pusherr		(void* stack, int code, char* msg);
extern	void	nnodbc_poperr		(void* stack);
extern	int	nnodbc_errstkempty	(void* stack);
extern	int	nnodbc_getsqlstatcode	(void* stack);
extern	char*	nnodbc_getsqlstatstr	(void* stack);
extern	char*	nnodbc_getsqlstatmsg	(void* stack);
extern	int	nnodbc_getnativcode	(void* stack);
extern	char*	nnodbc_getnativemsg	(void* stack);
extern	void*	nnodbc_clearerr 	(void* stack);

# define	PUSHSQLERR(stack, stat) \
			stack = nnodbc_pusherr(stack, stat, 0)
# define	PUSHSYSERR(stack, code, msg) \
			stack = nnodbc_pusherr(stack, code, msg)

# define	UNSET_ERROR(stack) \
			nnodbc_errstkunset(stack)

# define	CLEAR_ERROR(stack) \
			stack = nnodbc_clearerr(stack)

# define	NNODBC_ERRHEAD		"[NetNews ODBC][NNODBC driver]"
#endif	/* _HERR_H */
