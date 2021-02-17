/**
 * @file adddsn.c
 * DSN creation utility for Win32.
 *
 * $Id: adddsn.c,v 1.8 2013/01/11 12:19:55 chw Exp chw $
 *
 * Copyright (c) 2003-2013 Christian Werner <chw@ch-werner.de>
 *
 * See the file "license.terms" for information on usage
 * and redistribution of this file and for a
 * DISCLAIMER OF ALL WARRANTIES.
 */

#ifndef _WIN32
#error "only WIN32 supported"
#endif
#include <windows.h>
#include <sql.h>
#include <sqlext.h>
#include <odbcinst.h>
#include <winver.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

/**
 * Handler for ODBC installation error messages.
 * @param name name of API function for which to show error messages
 */

static BOOL
ProcessErrorMessages(char *name)
{
    WORD err = 1;
    DWORD code;
    char errmsg[301];
    WORD errlen, errmax = sizeof (errmsg) - 1;
    int rc;
    BOOL ret = FALSE;

    do {
	errmsg[0] = '\0';
	rc = SQLInstallerError(err, &code, errmsg, errmax, &errlen);
	if (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO) {
	    MessageBox(NULL, errmsg, name,
		       MB_ICONSTOP|MB_OK|MB_TASKMODAL|MB_SETFOREGROUND);
	    ret = TRUE;
	}
	err++;
    } while (rc != SQL_NO_DATA);
    return ret;
}

/**
 * Main function of DSN utility.
 * This is the Win32 GUI main entry point.
 * It (un)installs a DSN.
 *
 * Example usage:
 *
 *    add[sys]dsn "SQLite ODBC Driver" DSN=foobar;Database=C:/FOOBAR
 *    rem[sys]dsn "SQLite ODBC Driver" DSN=foobar
 */

int APIENTRY
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPSTR lpszCmdLine, int nCmdShow)
{
    char tmp[1024], *p, *drv, *cfg, *msg;
    int i, op;

    GetModuleFileName(NULL, tmp, sizeof (tmp));
    p = tmp;
    while (*p) {
	*p = tolower(*p);
	++p;
    }
    p = strrchr(tmp, '\\');
    if (p == NULL) {
	p = tmp;
    }
    op = ODBC_ADD_DSN;
    msg = "Adding DSN";
    if (strstr(p, "rem") != NULL) {
	msg = "Removing DSN";
	op = ODBC_REMOVE_DSN;
    }
    if (strstr(p, "sys") != NULL) {
	if (op == ODBC_REMOVE_DSN) {
	    op = ODBC_REMOVE_SYS_DSN;
	} else {
	    op = ODBC_ADD_SYS_DSN;
	}
    }
    strncpy(tmp, lpszCmdLine, sizeof (tmp));
    /* get driver argument */
    i = strspn(tmp, "\"");
    drv = tmp + i;
    if (i > 0) {
	i = strcspn(drv, "\"");
	drv[i] = '\0';
	cfg = drv + i + 1;
    } else {
	i = strcspn(drv, " \t");
	if (i > 0) {
	    drv[i] = '\0';
	    cfg = drv + i + 1;
	} else {
	    cfg = "\0\0";
	}
    }
    if (strlen(drv) == 0) {
	MessageBox(NULL, "No driver name given", msg,
		   MB_ICONERROR|MB_OK|MB_TASKMODAL|MB_SETFOREGROUND);
	exit(1);
    }
    i = strspn(cfg, " \t;");
    cfg += i;
    i = strlen(cfg);
    cfg[i + 1] = '\0';
    if (i > 0) {
	p = cfg;
	do {
	    p = strchr(p, ';');
	    if (p != NULL) {
		p[0] = '\0';
		p += 1;
	    }
	} while (p != NULL);
    }
    p = cfg;
    if (SQLConfigDataSource(NULL, (WORD) op, drv, cfg)) {
	exit(0);
    }
    ProcessErrorMessages(msg);
    exit(1);
}

