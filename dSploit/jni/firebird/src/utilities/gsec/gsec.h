/*
 *	PROGRAM:	Security data base manager
 *	MODULE:		gsec.h
 *	DESCRIPTION:	Header file for the GSEC program
 *
 * The contents of this file are subject to the Interbase Public
 * License Version 1.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy
 * of the License at http://www.Inprise.com/IPL.html
 *
 * Software distributed under the License is distributed on an
 * "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either express
 * or implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code was created by Inprise Corporation
 * and its predecessors. Portions created by Inprise Corporation are
 * Copyright (C) Inprise Corporation.
 *
 * All Rights Reserved.
 * Contributor(s): ______________________________________.
 */

#ifndef UTILITIES_GSEC_H
#define UTILITIES_GSEC_H

#include "../jrd/ThreadData.h"
#include "../jrd/jrd_pwd.h"
#include "../jrd/constants.h"

const USHORT GSEC_MSG_FAC	= 18;
const int MSG_LENGTH		= 128;
//#define QUERY_LENGTH	256

/* structure to hold information from the command line, including the
   operation to perform and any parameters entered (sizes are determined
   by the size of the fields in the USERS relation in USERINFO.GDB) */

const int ADD_OPER		= 1;
const int DEL_OPER		= 2;
const int DIS_OPER		= 3;
const int MOD_OPER		= 4;
const int QUIT_OPER		= 5;
const int HELP_OPER		= 6;
//const int VERSION_OPER	= 7;
const int MAP_SET_OPER	= 8;
const int MAP_DROP_OPER	= 9;
const int OLD_DIS_OPER	= 10;

const int USER_NAME_LEN	= 133;
const int ALT_NAME_LEN	= 129;
const int NAME_LEN		= 33;
//const int PASS_LEN		= MAX_PASSWORD_LENGTH + 1;
const int _SERVER_LEN	= 128;
const int DATABASE_LEN  = _SERVER_LEN + MAXPATHLEN;

struct internal_user_data
{
	int		operation;						// what's to be done
	TEXT	user_name [USER_NAME_LEN];		// the user's name
	bool	user_name_entered;				// user name entered flag
	int		uid;							// the user's id
	bool	uid_entered;					// UID entered flag
	bool	uid_specified;					// UID specified flag
	int		gid;							// the user's group id
	bool	gid_entered;					// GID entered flag
	bool	gid_specified;					// GID specified flag
	TEXT	sys_user_name [ALT_NAME_LEN];	// the sys_user's name
	bool	sys_user_entered;				// sys_user entered flag
	bool	sys_user_specified;				// sys_user specified flag
	TEXT	group_name [ALT_NAME_LEN];		// the group name
	bool	group_name_entered;				// group_name entered flag
	bool	group_name_specified;			// group_name specified flag
	TEXT	password [NAME_LEN];			// the user's password
	bool	password_entered;				// password entered flag
	bool	password_specified;				// password specified flag
	TEXT	first_name [NAME_LEN];			// the user's first name
	bool	first_name_entered;				// first name entered flag
	bool	first_name_specified;			// first name specified flag
	TEXT	middle_name [NAME_LEN];			// the user's middle name
	bool	middle_name_entered;			// middle name entered flag
	bool	middle_name_specified;			// middle name specified flag
	TEXT	last_name [NAME_LEN];			// the user's last name
	bool	last_name_entered;				// last name entered flag
	bool	last_name_specified;			// last name specified flag
	TEXT	dba_user_name [USER_NAME_LEN];	// the user's name
	bool	dba_user_name_entered;			// user name entered flag
	bool	dba_user_name_specified;		// database specified flag
	TEXT	dba_trust_user_name [USER_NAME_LEN];	// the trusted dba user's name
	bool	dba_trust_user_name_entered;	// trusted dba user name entered flag
	bool	dba_trust_user_name_specified;	// trusted dba user name specified flag
	bool	trusted_role;					// use trusted role to authenticate
	TEXT	dba_password [NAME_LEN];		// the user's name
	bool	dba_password_entered;			// user name entered flag
	bool	dba_password_specified;			// database specified flag
	TEXT	sql_role_name [NAME_LEN];		// the user's role
	bool	sql_role_name_entered;			// role entered flag
	bool	sql_role_name_specified;		// role specified flag
	TEXT	database_name [DATABASE_LEN];	// database to manage name
	bool	database_name_entered;			// database entered flag
	bool	database_name_specified;		// database specified flag
#ifdef TRUSTED_AUTH
	bool	trusted_auth;					// use trusted authentication
#endif
	int		admin;							// the user is admin of security DB
	bool	admin_entered;					// admin entered flag
	bool	admin_specified;				// admin specified flag

	// force NULLs in this ugly structure to avoid foolish bugs
	internal_user_data()
	{
		memset(this, 0, sizeof *this);
	}
};

namespace Firebird
{
	class UtilSvc;
}

class tsec : public ThreadData
{
public:
	explicit tsec(Firebird::UtilSvc* uf)
		: ThreadData(ThreadData::tddSEC), utilSvc(uf),
		tsec_user_data(0), tsec_exit_code(0), tsec_throw(false),
		tsec_interactive(false), tsec_sw_version(false)
	{
	}

	Firebird::UtilSvc*	utilSvc;
	internal_user_data*	tsec_user_data;
	int					tsec_exit_code;
	bool				tsec_throw;
	bool				tsec_interactive;
	bool				tsec_sw_version;

	static inline tsec* getSpecific()
	{
		return (tsec*) ThreadData::getSpecific();
	}
	static inline void putSpecific(tsec* tdsec)
	{
		tdsec->ThreadData::putSpecific();
	}
	static inline void restoreSpecific()
	{
		ThreadData::restoreSpecific();
	}
};



const USHORT GsecMsg0	= 0;	// empty message
const USHORT GsecMsg1	= 1;	// "GSEC> "  (the prompt)
const USHORT GsecMsg2	= 2;	// gsec  (lower case version of the prompt for help display)
const USHORT GsecMsg3	= 3;	// ADD          add user
const USHORT GsecMsg4	= 4;	// DELETE       delete user
const USHORT GsecMsg5	= 5;	// DISPLAY      display user(s)
const USHORT GsecMsg6	= 6;	// MODIFY       modify user
const USHORT GsecMsg7	= 7;	// PW           user's password
const USHORT GsecMsg8	= 8;	// UID          user's ID
const USHORT GsecMsg9	= 9;	// GID          user's group ID
const USHORT GsecMsg10	= 10;	// PROJ         user's project name
const USHORT GsecMsg11	= 11;	// ORG          user's organization name
const USHORT GsecMsg12	= 12;	// FNAME        user's first name
const USHORT GsecMsg13	= 13;	// MNAME        user's middle name/initial
const USHORT GsecMsg14	= 14;	// LNAME        user's last name
const USHORT GsecMsg15	= 15;	// gsec - unable to open database
const USHORT GsecMsg16	= 16;	// gsec - error in switch specifications
const USHORT GsecMsg17	= 17;	// gsec - no operation specified
const USHORT GsecMsg18	= 18;	// gsec - no user name specified
const USHORT GsecMsg19	= 19;	// gsec - add record error
const USHORT GsecMsg20	= 20;	// gsec - modify record error
const USHORT GsecMsg21	= 21;	// gsec - find/modify record error
const USHORT GsecMsg22	= 22;	// gsec - record not found for user:
const USHORT GsecMsg23	= 23;	// gsec - delete record error
const USHORT GsecMsg24	= 24;	// gsec - find/delete record error
const USHORT GsecMsg25	= 25;	// users defined for node
const USHORT GsecMsg26	= 26;	//     user name     uid   gid     project   organization       full name
const USHORT GsecMsg27	= 27;	// ---------------- ----- ----- ------------ ------------ --------------------
const USHORT GsecMsg28	= 28;	// gsec - find/display record error
const USHORT GsecMsg29	= 29;	// gsec - invalid parameter, no switch defined
const USHORT GsecMsg30	= 30;	// gsec - operation already specified
const USHORT GsecMsg31	= 31;	// gsec - password already specified
const USHORT GsecMsg32	= 32;	// gsec - uid already specified
const USHORT GsecMsg33	= 33;	// gsec - gid already specified
const USHORT GsecMsg34	= 34;	// gsec - project already specified
const USHORT GsecMsg35	= 35;	// gsec - organization already specified
const USHORT GsecMsg36	= 36;	// gsec - first_name already specified
const USHORT GsecMsg37	= 37;	// gsec - middle_name already specified
const USHORT GsecMsg38	= 38;	// gsec - last_name already specified
const USHORT GsecMsg39	= 39;	// gsec version
const USHORT GsecMsg40	= 40;	// gsec - invalid switch specified
const USHORT GsecMsg41	= 41;	// gsec - ambiguous switch specified
const USHORT GsecMsg42	= 42;	// gsec - no operation specified for parameters
const USHORT GsecMsg43	= 43;	// gsec - no parameters allowed for this operation
const USHORT GsecMsg44	= 44;	// gsec - incompatible switches specified
const USHORT GsecMsg45	= 45;	// gsec utility - maintains user password database"
const USHORT GsecMsg46	= 46;	// command line usage:
const USHORT GsecMsg47	= 47;	// <command> [ <parameter> ... ]
const USHORT GsecMsg48	= 48;	// interactive usage:
const USHORT GsecMsg49	= 49;	// available commands:
const USHORT GsecMsg50	= 50;	// adding a new user:
const USHORT GsecMsg51	= 51;	// add <name> [ <parameter> ... ]
const USHORT GsecMsg52	= 52;	// deleting a current user:
const USHORT GsecMsg53	= 53;	// delete <name>
const USHORT GsecMsg54	= 54;	// displaying all users:
const USHORT GsecMsg55	= 55;	// display
const USHORT GsecMsg56	= 56;	// displaying one user:
const USHORT GsecMsg57	= 57;	// display <name>
const USHORT GsecMsg58	= 58;	// modifying a user's parameters:
const USHORT GsecMsg59	= 59;	// modify <name> <parameter> [ <parameter> ... ]
const USHORT GsecMsg60	= 60;	// help:
const USHORT GsecMsg61	= 61;	// ? (interactive only)
const USHORT GsecMsg62	= 62;	// help
const USHORT GsecMsg63	= 63;	// quit interactive session:
const USHORT GsecMsg64	= 64;	// quit (interactive only)
const USHORT GsecMsg65	= 65;	// available parameters:
const USHORT GsecMsg66	= 66;	// -pw <password>
const USHORT GsecMsg67	= 67;	// -uid <uid>
const USHORT GsecMsg68	= 68;	// -gid <uid>
const USHORT GsecMsg69	= 69;	// -proj <projectname>
const USHORT GsecMsg70	= 70;	// -org <organizationname>
const USHORT GsecMsg71	= 71;	// -fname <firstname>
const USHORT GsecMsg72	= 72;	// -mname <middlename>
const USHORT GsecMsg73	= 73;	// -lname <lastname>
const USHORT GsecMsg74	= 74;	// gsec - memory allocation error
const USHORT GsecMsg75	= 75;	// gsec error
const USHORT GsecMsg76	= 76;	// invalid user name (maximum 31 bytes allowed)
const USHORT GsecMsg77	= 77;	// invalid password (maximum 16 bytes allowed)
const USHORT GsecMsg78	= 78;	// gsec - database already specified
const USHORT GsecMsg79	= 79;	// gsec - database administrator name already specified
const USHORT GsecMsg80	= 80;	// gsec - database administrator password already specified
const USHORT GsecMsg81	= 81;	// gsec - SQL role name already specified
const USHORT GsecMsg82	= 82;	// [ <options ... ]
const USHORT GsecMsg83	= 83;	// available options:
const USHORT GsecMsg84	= 84;	// -user <database administrator name>
const USHORT GsecMsg85	= 85;	// -password <database administrator password>
const USHORT GsecMsg86	= 86;	// -role <database administrator SQL role name>
const USHORT GsecMsg87	= 87;	// -database <security database>
const USHORT GsecMsg88	= 88;	// -z
const USHORT GsecMsg89	= 89;	// displaying version number:
const USHORT GsecMsg90	= 90;	// z (interactive only)
const USHORT GsecMsg91	= 91;	// -trusted (use trusted authentication)
const USHORT GsecMsg92	= 92;	// invalid switch specified in interactive mode
const USHORT GsecMsg93	= 93;	// error closing security database
const USHORT GsecMsg94	= 94;	// error releasing request in security database
const USHORT GsecMsg95	= 95;	// -fe(tch_password) fetch password from file
const USHORT GsecMsg96	= 96;	// error fetching password from file
const USHORT GsecMsg97	= 97;	// error changing AUTO ADMINS MAPPING in security database
const USHORT GsecMsg98	= 98;	// changing admins mapping to SYSDBA:\n
const USHORT GsecMsg99	= 99;	// invalid parameter for -MAPPING, only SET or DROP is accepted
const USHORT GsecMsg100	= 100;	// -ma(pping) {set|drop}
const USHORT GsecMsg101 = 101;	// use gsec -? to get help
const USHORT GsecMsg102 = 102;	// -adm(in) {yes|no}
const USHORT GsecMsg103	= 103;	// invalid parameter for -ADMIN, only YES or NO is accepted

#endif // UTILITIES_GSEC_H

