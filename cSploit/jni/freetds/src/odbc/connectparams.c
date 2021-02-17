/* FreeTDS - Library of routines accessing Sybase and Microsoft databases
 * Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005  Brian Bruns
 * Copyright (C) 2005-2011  Frediano Ziglio
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <config.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>

#include <freetds/odbc.h>
#include <freetds/string.h>
#include "replacements.h"

#ifdef DMALLOC
#include <dmalloc.h>
#endif

TDS_RCSID(var, "$Id: connectparams.c,v 1.94 2011-09-25 11:36:24 freddy77 Exp $");

#define ODBC_PARAM(p) static const char odbc_param_##p[] = #p;
ODBC_PARAM_LIST
#undef ODBC_PARAM

#ifdef _WIN32
#define ODBC_PARAM(p) odbc_param_##p,
static const char *odbc_param_names[] = {
	ODBC_PARAM_LIST
};
#undef ODBC_PARAM
#endif

#if !HAVE_SQLGETPRIVATEPROFILESTRING

/*
 * Last resort place to check for INI file. This is usually set at compile time
 * by build scripts.
 */
#ifndef SYS_ODBC_INI
#define SYS_ODBC_INI "/etc/odbc.ini"
#endif

/**
 * Call this to get the INI file containing Data Source Names.
 * @note rules for determining the location of ODBC config may be different 
 * then what you expect - at this time they differ from unixODBC 
 *
 * @return file opened or NULL if error
 * @retval 1 worked
 */
static FILE *tdoGetIniFileName(void);

/**
 * SQLGetPrivateProfileString
 *
 * PURPOSE
 *
 *  This is an implementation of a common MS API call. This implementation 
 *  should only be used if the ODBC sub-system/SDK does not have it.
 *  For example; unixODBC has its own so those using unixODBC should NOT be
 *  using this implementation because unixODBC;
 *  - provides caching of ODBC config data 
 *  - provides consistent interpretation of ODBC config data (i.e, location)
 *
 * ARGS
 *
 *  see ODBC documentation
 *                      
 * RETURNS
 *
 *  see ODBC documentation
 *
 * NOTES:
 *
 *  - the spec is not entirely implemented... consider this a lite version
 *  - rules for determining the location of ODBC config may be different then what you 
 *    expect see tdoGetIniFileName().
 *
 */
static int SQLGetPrivateProfileString(LPCSTR pszSection, LPCSTR pszEntry, LPCSTR pszDefault, LPSTR pRetBuffer, int nRetBuffer,
				      LPCSTR pszFileName);
#endif

#if defined(FILENAME_MAX) && FILENAME_MAX < 512
#undef FILENAME_MAX
#define FILENAME_MAX 512
#endif

static int
parse_server(TDS_ERRS *errs, char *server, TDSLOGIN * login)
{
	char *p = (char *) strchr(server, '\\');

	if (p) {
		if (!tds_dstr_copy(&login->instance_name, p+1)) {
			odbc_errs_add(errs, "HY001", NULL);
			return 0;
		}
		*p = 0;
	} else {
		p = (char *) strchr(server, ',');
		if (p && atoi(p+1) > 0) {
			login->port = atoi(p+1);
			*p = 0;
		}
	}

	if (TDS_SUCCEED(tds_lookup_host_set(server, &login->ip_addrs)))
		tds_dstr_copy(&login->server_host_name, server);

	return 1;
}

static int
myGetPrivateProfileString(const char *DSN, const char *key, char *buf)
{
	buf[0] = '\0';
	return SQLGetPrivateProfileString(DSN, key, "", buf, FILENAME_MAX, "odbc.ini");
}

/** 
 * Read connection information from given DSN
 * @param DSN           DSN name
 * @param login    where to store connection info
 * @return 1 if success 0 otherwhise
 */
int
odbc_get_dsn_info(TDS_ERRS *errs, const char *DSN, TDSLOGIN * login)
{
	char tmp[FILENAME_MAX];
	int freetds_conf_less = 1;

	/* use old servername */
	if (myGetPrivateProfileString(DSN, odbc_param_Servername, tmp) > 0) {
		freetds_conf_less = 0;
		tds_dstr_copy(&login->server_name, tmp);
		tds_read_conf_file(login, tmp);
		if (myGetPrivateProfileString(DSN, odbc_param_Server, tmp) > 0) {
			odbc_errs_add(errs, "HY000", "You cannot specify both SERVERNAME and SERVER");
			return 0;
		}
		if (myGetPrivateProfileString(DSN, odbc_param_Address, tmp) > 0) {
			odbc_errs_add(errs, "HY000", "You cannot specify both SERVERNAME and ADDRESS");
			return 0;
		}
	}

	/* search for server (compatible with ms one) */
	if (freetds_conf_less) {
		int address_specified = 0;

		if (myGetPrivateProfileString(DSN, odbc_param_Address, tmp) > 0) {
			address_specified = 1;
			/* TODO parse like MS */

			tds_lookup_host_set(tmp, &login->ip_addrs);
		}
		if (myGetPrivateProfileString(DSN, odbc_param_Server, tmp) > 0) {
			tds_dstr_copy(&login->server_name, tmp);
			if (!address_specified) {
				if (!parse_server(errs, tmp, login))
					return 0;
			}
		}
	}

	if (myGetPrivateProfileString(DSN, odbc_param_Port, tmp) > 0)
		tds_parse_conf_section(TDS_STR_PORT, tmp, login);

	if (myGetPrivateProfileString(DSN, odbc_param_TDS_Version, tmp) > 0)
		tds_parse_conf_section(TDS_STR_VERSION, tmp, login);

	if (myGetPrivateProfileString(DSN, odbc_param_Language, tmp) > 0)
		tds_parse_conf_section(TDS_STR_LANGUAGE, tmp, login);

	if (tds_dstr_isempty(&login->database)
	    && myGetPrivateProfileString(DSN, odbc_param_Database, tmp) > 0)
		tds_dstr_copy(&login->database, tmp);

	if (myGetPrivateProfileString(DSN, odbc_param_TextSize, tmp) > 0)
		tds_parse_conf_section(TDS_STR_TEXTSZ, tmp, login);

	if (myGetPrivateProfileString(DSN, odbc_param_PacketSize, tmp) > 0)
		tds_parse_conf_section(TDS_STR_BLKSZ, tmp, login);

	if (myGetPrivateProfileString(DSN, odbc_param_ClientCharset, tmp) > 0)
		tds_parse_conf_section(TDS_STR_CLCHARSET, tmp, login);

	if (myGetPrivateProfileString(DSN, odbc_param_DumpFile, tmp) > 0)
		tds_parse_conf_section(TDS_STR_DUMPFILE, tmp, login);

	if (myGetPrivateProfileString(DSN, odbc_param_DumpFileAppend, tmp) > 0)
		tds_parse_conf_section(TDS_STR_APPENDMODE, tmp, login);

	if (myGetPrivateProfileString(DSN, odbc_param_DebugFlags, tmp) > 0)
		tds_parse_conf_section(TDS_STR_DEBUGFLAGS, tmp, login);

	if (myGetPrivateProfileString(DSN, odbc_param_Encryption, tmp) > 0)
		tds_parse_conf_section(TDS_STR_ENCRYPTION, tmp, login);

	if (myGetPrivateProfileString(DSN, odbc_param_UseNTLMv2, tmp) > 0)
		tds_parse_conf_section(TDS_STR_USENTLMV2, tmp, login);

	if (myGetPrivateProfileString(DSN, odbc_param_REALM, tmp) > 0)
		tds_parse_conf_section(TDS_STR_REALM, tmp, login);

	if (myGetPrivateProfileString(DSN, odbc_param_ServerSPN, tmp) > 0)
		tds_parse_conf_section(TDS_STR_SPN, tmp, login);

	if (myGetPrivateProfileString(DSN, odbc_param_Trusted_Connection, tmp) > 0 && tds_config_boolean(odbc_param_Trusted_Connection, tmp, login)) {
		tds_dstr_copy(&login->user_name, "");
		tds_dstr_copy(&login->password, "");
	}

	if (myGetPrivateProfileString(DSN, odbc_param_MARS_Connection, tmp) > 0 && tds_config_boolean(odbc_param_MARS_Connection, tmp, login)) {
		login->mars = 1;
	}

	return 1;
}

/** 
 * Parse connection string and fill login according
 * @param connect_string     connect string
 * @param connect_string_end connect string end (pointer to char past last)
 * @param login         where to store connection info
 * @return 1 if success 0 otherwhise
 */
int
odbc_parse_connect_string(TDS_ERRS *errs, const char *connect_string, const char *connect_string_end, TDSLOGIN * login,
			  TDS_PARSED_PARAM *parsed_params)
{
	const char *p, *end;
	DSTR *dest_s, value;
	enum { CFG_DSN = 1, CFG_SERVER = 2, CFG_SERVERNAME = 4 };
	unsigned int cfgs = 0;	/* flags for indicate second parse of string */
	char option[24];
	int trusted = 0;

	if (parsed_params)
		memset(parsed_params, 0, sizeof(*parsed_params)*ODBC_PARAM_SIZE);

	tds_dstr_init(&value);
	for (p = connect_string; p < connect_string_end && *p;) {
		int num_param = -1;

		dest_s = NULL;

		/* handle empty options */
		while (p < connect_string_end && *p == ';')
			++p;

		/* parse option */
		end = (const char *) memchr(p, '=', connect_string_end - p);
		if (!end)
			break;

		/* account for spaces between ;'s. */
		while (p < end && *p == ' ')
			++p;

		if ((end - p) >= (int) sizeof(option))
			option[0] = 0;
		else {
			memcpy(option, p, end - p);
			option[end - p] = 0;
		}

		/* parse value */
		p = end + 1;
		if (*p == '{') {
			++p;
			/* search "};" */
			end = p;
			while ((end = (const char *) memchr(end, '}', connect_string_end - end)) != NULL) {
				if ((end + 1) != connect_string_end && end[1] == ';')
					break;
				++end;
			}
		} else {
			end = (const char *) memchr(p, ';', connect_string_end - p);
		}
		if (!end)
			end = connect_string_end;

		if (!tds_dstr_copyn(&value, p, end - p)) {
			odbc_errs_add(errs, "HY001", NULL);
			return 0;
		}

#define CHK_PARAM(p) (strcasecmp(option, odbc_param_##p) == 0 && (num_param=ODBC_PARAM_##p) >= 0)
		if (CHK_PARAM(Server)) {
			/* error if servername or DSN specified */
			if ((cfgs & (CFG_DSN|CFG_SERVERNAME)) != 0) {
				tds_dstr_free(&value);
				odbc_errs_add(errs, "HY000", "Only one between SERVER, SERVERNAME and DSN can be specified");
				return 0;
			}
			if (!cfgs) {
				dest_s = &login->server_name;
				/* not that safe cast but works -- freddy77 */
				if (!parse_server(errs, (char *) tds_dstr_cstr(&value), login)) {
					tds_dstr_free(&value);
					return 0;
				}
				cfgs = CFG_SERVER;
			}
		} else if (CHK_PARAM(Servername)) {
			if ((cfgs & (CFG_DSN|CFG_SERVER)) != 0) {
				tds_dstr_free(&value);
				odbc_errs_add(errs, "HY000", "Only one between SERVER, SERVERNAME and DSN can be specified");
				return 0;
			}
			if (!cfgs) {
				tds_dstr_dup(&login->server_name, &value);
				tds_read_conf_file(login, tds_dstr_cstr(&value));
				cfgs = CFG_SERVERNAME;
				p = connect_string;
				continue;
			}
		} else if (CHK_PARAM(DSN)) {
			if ((cfgs & (CFG_SERVER|CFG_SERVERNAME)) != 0) {
				tds_dstr_free(&value);
				odbc_errs_add(errs, "HY000", "Only one between SERVER, SERVERNAME and DSN can be specified");
				return 0;
			}
			if (!cfgs) {
				if (!odbc_get_dsn_info(errs, tds_dstr_cstr(&value), login)) {
					tds_dstr_free(&value);
					return 0;
				}
				cfgs = CFG_DSN;
				p = connect_string;
				continue;
			}
		} else if (CHK_PARAM(Database)) {
			dest_s = &login->database;
		} else if (CHK_PARAM(UID)) {
			dest_s = &login->user_name;
		} else if (CHK_PARAM(PWD)) {
			dest_s = &login->password;
		} else if (CHK_PARAM(APP)) {
			dest_s = &login->app_name;
		} else if (CHK_PARAM(WSID)) {
			dest_s = &login->client_host_name;
		} else if (CHK_PARAM(Language)) {
			tds_parse_conf_section(TDS_STR_LANGUAGE, tds_dstr_cstr(&value), login);
		} else if (CHK_PARAM(Port)) {
			tds_parse_conf_section(TDS_STR_PORT, tds_dstr_cstr(&value), login);
		} else if (CHK_PARAM(TDS_Version)) {
			tds_parse_conf_section(TDS_STR_VERSION, tds_dstr_cstr(&value), login);
		} else if (CHK_PARAM(TextSize)) {
			tds_parse_conf_section(TDS_STR_TEXTSZ, tds_dstr_cstr(&value), login);
		} else if (CHK_PARAM(PacketSize)) {
			tds_parse_conf_section(TDS_STR_BLKSZ, tds_dstr_cstr(&value), login);
		} else if (CHK_PARAM(ClientCharset)) {
			tds_parse_conf_section(TDS_STR_CLCHARSET, tds_dstr_cstr(&value), login);
		} else if (CHK_PARAM(DumpFile)) {
			tds_parse_conf_section(TDS_STR_DUMPFILE, tds_dstr_cstr(&value), login);
		} else if (CHK_PARAM(DumpFileAppend)) {
			tds_parse_conf_section(TDS_STR_APPENDMODE, tds_dstr_cstr(&value), login);
		} else if (CHK_PARAM(DebugFlags)) {
			tds_parse_conf_section(TDS_STR_DEBUGFLAGS, tds_dstr_cstr(&value), login);
		} else if (CHK_PARAM(Encryption)) {
			tds_parse_conf_section(TDS_STR_ENCRYPTION, tds_dstr_cstr(&value), login);
		} else if (CHK_PARAM(UseNTLMv2)) {
			tds_parse_conf_section(TDS_STR_USENTLMV2, tds_dstr_cstr(&value), login);
		} else if (CHK_PARAM(REALM)) {
			tds_parse_conf_section(TDS_STR_REALM, tds_dstr_cstr(&value), login);
		} else if (CHK_PARAM(ServerSPN)) {
			tds_parse_conf_section(TDS_STR_SPN, tds_dstr_cstr(&value), login);
		} else if (CHK_PARAM(Trusted_Connection)) {
			trusted = tds_config_boolean(option, tds_dstr_cstr(&value), login);
			tdsdump_log(TDS_DBG_INFO1, "trusted %s -> %d\n", tds_dstr_cstr(&value), trusted);
			num_param = -1;
			/* TODO odbc_param_Address field */
		} else if (CHK_PARAM(MARS_Connection)) {
			if (tds_config_boolean(option, tds_dstr_cstr(&value), login))
				login->mars = 1;
		}

		if (num_param >= 0 && parsed_params) {
			parsed_params[num_param].p = p;
			parsed_params[num_param].len = end - p;
		}

		/* copy to destination */
		if (dest_s) {
			DSTR tmp = *dest_s;
			*dest_s = value;
			value = tmp;
		}

		p = end;
		/* handle "" ";.." "};.." cases */
		if (p >= connect_string_end)
			break;
		if (*p == '}')
			++p;
		++p;
	}

	if (trusted) {
		if (parsed_params) {
			parsed_params[ODBC_PARAM_Trusted_Connection].p = "Yes";
			parsed_params[ODBC_PARAM_Trusted_Connection].len = 3;
			parsed_params[ODBC_PARAM_UID].p = NULL;
			parsed_params[ODBC_PARAM_PWD].p = NULL;
		}
		tds_dstr_copy(&login->user_name, "");
		tds_dstr_copy(&login->password, "");
	}

	tds_dstr_free(&value);
	return 1;
}

#ifdef _WIN32
int
odbc_build_connect_string(TDS_ERRS *errs, TDS_PARSED_PARAM *params, char **out)
{
	unsigned n;
	size_t len = 1;
	char *p;

	/* compute string size */
	for (n = 0; n < ODBC_PARAM_SIZE; ++n) {
		if (params[n].p)
			len += strlen(odbc_param_names[n]) + params[n].len + 2;
	}

	/* allocate */
	p = (char*) malloc(len);
	if (!p) {
		odbc_errs_add(errs, "HY001", NULL);
		return 0;
	}
	*out = p;

	/* build it */
	for (n = 0; n < ODBC_PARAM_SIZE; ++n) {
		if (params[n].p)
			p += sprintf(p, "%s=%.*s;", odbc_param_names[n], (int) params[n].len, params[n].p);
	}
	*p = 0;
	return 1;
}
#endif

#if !HAVE_SQLGETPRIVATEPROFILESTRING

#ifdef _WIN32
#  error There is something wrong  in configuration...
#endif

typedef struct
{
	LPCSTR entry;
	LPSTR buffer;
	int buffer_len;
	int ret_val;
	int found;
}
ProfileParam;

static void
tdoParseProfile(const char *option, const char *value, void *param)
{
	ProfileParam *p = (ProfileParam *) param;

	if (strcasecmp(p->entry, option) == 0) {
		tds_strlcpy(p->buffer, value, p->buffer_len);

		p->ret_val = strlen(p->buffer);
		p->found = 1;
	}
}

static int
SQLGetPrivateProfileString(LPCSTR pszSection, LPCSTR pszEntry, LPCSTR pszDefault, LPSTR pRetBuffer, int nRetBuffer,
			   LPCSTR pszFileName)
{
	FILE *hFile;
	ProfileParam param;

	tdsdump_log(TDS_DBG_FUNC, "SQLGetPrivateProfileString(%p, %p, %p, %p, %d, %p)\n", 
			pszSection, pszEntry, pszDefault, pRetBuffer, nRetBuffer, pszFileName);

	if (!pszSection) {
		/* spec says return list of all section names - but we will just return nothing */
		tdsdump_log(TDS_DBG_WARN, "WARNING: Functionality for NULL pszSection not implemented.\n");
		return 0;
	}

	if (!pszEntry) {
		/* spec says return list of all key names in section - but we will just return nothing */
		tdsdump_log(TDS_DBG_WARN, "WARNING: Functionality for NULL pszEntry not implemented.\n");
		return 0;
	}

	if (nRetBuffer < 1)
		tdsdump_log(TDS_DBG_WARN, "WARNING: No space to return a value because nRetBuffer < 1.\n");

	if (pszFileName && *pszFileName == '/')
		hFile = fopen(pszFileName, "r");
	else
		hFile = tdoGetIniFileName();

	if (hFile == NULL) {
		tdsdump_log(TDS_DBG_ERROR, "ERROR: Could not open configuration file\n");
		return 0;
	}

	param.entry = pszEntry;
	param.buffer = pRetBuffer;
	param.buffer_len = nRetBuffer;
	param.ret_val = 0;
	param.found = 0;

	pRetBuffer[0] = 0;
	tds_read_conf_section(hFile, pszSection, tdoParseProfile, &param);

	if (pszDefault && !param.found) {
		tds_strlcpy(pRetBuffer, pszDefault, nRetBuffer);

		param.ret_val = strlen(pRetBuffer);
	}

	fclose(hFile);
	return param.ret_val;
}

static FILE *
tdoGetIniFileName()
{
	FILE *ret = NULL;
	char *p;
	char *fn;

	/*
	 * First, try the ODBCINI environment variable
	 */
	if ((p = getenv("ODBCINI")) != NULL)
		ret = fopen(p, "r");

	/*
	 * Second, try the HOME environment variable
	 */
	if (!ret && (p = tds_get_homedir()) != NULL) {
		fn = NULL;
		if (asprintf(&fn, "%s/.odbc.ini", p) > 0) {
			ret = fopen(fn, "r");
			free(fn);
		}
		free(p);
	}

	/*
	 * As a last resort, try SYS_ODBC_INI
	 */
	if (!ret)
		ret = fopen(SYS_ODBC_INI, "r");

	return ret;
}

#endif /* !HAVE_SQLGETPRIVATEPROFILESTRING */

#ifdef UNIXODBC

/*
 * Begin BIG Hack.
 *  
 * We need these from odbcinstext.h but it wants to 
 * include <log.h> and <ini.h>, which are not in the 
 * standard include path.  XXX smurph
 * confirmed by unixODBC stuff, odbcinstext.h shouldn't be installed. freddy77
 */
#define     INI_MAX_LINE            1000
#define     INI_MAX_OBJECT_NAME     INI_MAX_LINE
#define     INI_MAX_PROPERTY_NAME   INI_MAX_LINE
#define     INI_MAX_PROPERTY_VALUE  INI_MAX_LINE

#define	ODBCINST_PROMPTTYPE_LABEL		0	/* readonly */
#define	ODBCINST_PROMPTTYPE_TEXTEDIT	1
#define	ODBCINST_PROMPTTYPE_LISTBOX		2
#define	ODBCINST_PROMPTTYPE_COMBOBOX	3
#define	ODBCINST_PROMPTTYPE_FILENAME	4
#define	ODBCINST_PROMPTTYPE_HIDDEN	    5

typedef struct tODBCINSTPROPERTY
{
	struct tODBCINSTPROPERTY *pNext;	/* pointer to next property, NULL if last property                                                                              */

	char szName[INI_MAX_PROPERTY_NAME + 1];	/* property name                                                                                                                                                */
	char szValue[INI_MAX_PROPERTY_VALUE + 1];	/* property value                                                                                                                                               */
	int nPromptType;	/* PROMPTTYPE_TEXTEDIT, PROMPTTYPE_LISTBOX, PROMPTTYPE_COMBOBOX, PROMPTTYPE_FILENAME    */
	char **aPromptData;	/* array of pointers terminated with a NULL value in array.                                                     */
	char *pszHelp;		/* help on this property (driver setups should keep it short)                                                   */
	void *pWidget;		/* CALLER CAN STORE A POINTER TO ? HERE                                                                                                 */
	int bRefresh;		/* app should refresh widget ie Driver Setup has changed aPromptData or szValue                 */
	void *hDLL;		/* for odbcinst internal use... only first property has valid one                                               */
}
ODBCINSTPROPERTY, *HODBCINSTPROPERTY;

/* 
 * End BIG Hack.
 */

int ODBCINSTGetProperties(HODBCINSTPROPERTY hLastProperty);

static const char *const aTDSver[] = {
	"",
	"4.2",
	"5.0",
	"7.0",
	"7.1",
	"7.2",
	"7.3",
	NULL
};

static const char *const aLanguage[] = {
	"us_english",
	NULL
};

static const char *const aEncryption[] = {
	TDS_STR_ENCRYPTION_OFF,
	TDS_STR_ENCRYPTION_REQUEST,
	TDS_STR_ENCRYPTION_REQUIRE,
	NULL
};

static const char *const aBoolean[] = {
	"Yes",
	"No",
	NULL
};

/*
static const char *aAuth[] = {
	"Server",
	"Domain",
	"Both",
	NULL
};
*/

static HODBCINSTPROPERTY
addProperty(HODBCINSTPROPERTY hLastProperty)
{
	hLastProperty->pNext = (HODBCINSTPROPERTY) calloc(1, sizeof(ODBCINSTPROPERTY));
	hLastProperty = hLastProperty->pNext;
	return hLastProperty;
}

static HODBCINSTPROPERTY
definePropertyString(HODBCINSTPROPERTY hLastProperty, const char *name, const char *value, const char *comment)
{
	hLastProperty = addProperty(hLastProperty);
	hLastProperty->nPromptType = ODBCINST_PROMPTTYPE_TEXTEDIT;
	tds_strlcpy(hLastProperty->szName, name, INI_MAX_PROPERTY_NAME);
	tds_strlcpy(hLastProperty->szValue, value, INI_MAX_PROPERTY_VALUE);
	hLastProperty->pszHelp = (char *) strdup(comment);
	return hLastProperty;
}

static HODBCINSTPROPERTY
definePropertyBoolean(HODBCINSTPROPERTY hLastProperty, const char *name, const char *value, const char *comment)
{
	hLastProperty = addProperty(hLastProperty);
	hLastProperty->nPromptType = ODBCINST_PROMPTTYPE_LISTBOX;
	hLastProperty->aPromptData = malloc(sizeof(aBoolean));
	memcpy(hLastProperty->aPromptData, aBoolean, sizeof(aBoolean));
	tds_strlcpy(hLastProperty->szName, name, INI_MAX_PROPERTY_NAME);
	tds_strlcpy(hLastProperty->szValue, value, INI_MAX_PROPERTY_VALUE);
	hLastProperty->pszHelp = (char *) strdup(comment);
	return hLastProperty;
}

static HODBCINSTPROPERTY
definePropertyHidden(HODBCINSTPROPERTY hLastProperty, const char *name, const char *value, const char *comment)
{
	hLastProperty = addProperty(hLastProperty);
	hLastProperty->nPromptType = ODBCINST_PROMPTTYPE_HIDDEN;
	tds_strlcpy(hLastProperty->szName, name, INI_MAX_PROPERTY_NAME);
	tds_strlcpy(hLastProperty->szValue, value, INI_MAX_PROPERTY_VALUE);
	hLastProperty->pszHelp = (char *) strdup(comment);
	return hLastProperty;
}

static HODBCINSTPROPERTY
definePropertyList(HODBCINSTPROPERTY hLastProperty, const char *name, const char *value, const void *list, int size, const char *comment)
{
	hLastProperty = addProperty(hLastProperty);
	hLastProperty->nPromptType = ODBCINST_PROMPTTYPE_LISTBOX;
	hLastProperty->aPromptData = malloc(size);
	memcpy(hLastProperty->aPromptData, list, size);
	tds_strlcpy(hLastProperty->szName, name, INI_MAX_PROPERTY_NAME);
	tds_strlcpy(hLastProperty->szValue, value, INI_MAX_PROPERTY_VALUE);
	hLastProperty->pszHelp = (char *) strdup(comment);
	return hLastProperty;
}

int
ODBCINSTGetProperties(HODBCINSTPROPERTY hLastProperty)
{
	hLastProperty = definePropertyString(hLastProperty, odbc_param_Servername, "", 
		"Name of FreeTDS connection to connect to.\n"
		"This server name refer to entry in freetds.conf file, not real server name.\n"
		"This property cannot be used with Server property.");

	hLastProperty = definePropertyString(hLastProperty, odbc_param_Server, "", 
		"Name of server to connect to.\n"
		"This should be the name of real server.\n"
		"This property cannot be used with Servername property.");

	hLastProperty = definePropertyString(hLastProperty, odbc_param_Address, "", 
		"The hostname or ip address of the server.");

	hLastProperty = definePropertyString(hLastProperty, odbc_param_Port, "1433", 
		"TCP/IP Port to connect to.");

	hLastProperty = definePropertyString(hLastProperty, odbc_param_Database, "", 
		"Default database.");

	hLastProperty = definePropertyList(hLastProperty, odbc_param_TDS_Version, "4.2", (void*) aTDSver, sizeof(aTDSver),
		"The TDS protocol version.\n"
		" 4.2 MSSQL 6.5 or Sybase < 10.x\n"
		" 5.0 Sybase >= 10.x\n"
		" 7.0 MSSQL 7\n"
		" 7.1 MSSQL 2000\n"
		" 7.2 MSSQL 2005\n"
		" 7.3 MSSQL 2008"
		);

	hLastProperty = definePropertyList(hLastProperty, odbc_param_Language, "us_english", (void*) aLanguage, sizeof(aLanguage),
		"The default language setting.");

	hLastProperty = definePropertyHidden(hLastProperty, odbc_param_TextSize, "", 
		"Text datatype limit.");

	/* ??? in odbc.ini ??? */
/*
	hLastProperty = definePropertyString(hLastProperty, odbc_param_UID, "", 
		"User ID (Beware of security issues).");

	hLastProperty = definePropertyString(hLastProperty, odbc_param_PWD, "", 
		"Password (Beware of security issues).");
*/

/*
	hLastProperty = definePropertyList(hLastProperty, odbc_param_Authentication, "Server", aAuth, sizeof(aAuth),
		"The server authentication mechanism.");

	hLastProperty = definePropertyString(hLastProperty, odbc_param_Domain, "", 
		"The default domain to use when using Domain Authentication.");
*/

	hLastProperty = definePropertyString(hLastProperty, odbc_param_PacketSize, "", 
		"Size of network packets.");

	hLastProperty = definePropertyString(hLastProperty, odbc_param_ClientCharset, "", 
		"The client character set name to convert application characters to UCS-2 in TDS 7.0 and higher.");

	hLastProperty = definePropertyString(hLastProperty, odbc_param_DumpFile, "",
		"Specifies the location of a tds dump file and turns on logging.");

	hLastProperty = definePropertyBoolean(hLastProperty, odbc_param_DumpFileAppend, "",
		"Appends dump file instead of overwriting it. Useful for debugging when many processes are active.");

	hLastProperty = definePropertyString(hLastProperty, odbc_param_DebugFlags, "", 
		"Sets granularity of logging. A set of bit that specify levels and informations. See table below for bit specification.");

	hLastProperty = definePropertyList(hLastProperty, odbc_param_Encryption, TDS_STR_ENCRYPTION_OFF, aEncryption, sizeof(aEncryption),
		"The encryption method.");

	return 1;
}

#endif
