/**
 * @file inst.c
 * SQLite ODBC Driver installer/uninstaller for WIN32
 *
 * $Id: inst.c,v 1.19 2013/01/22 16:37:30 chw Exp chw $
 *
 * Copyright (c) 2001-2013 Christian Werner <chw@ch-werner.de>
 *
 * See the file "license.terms" for information on usage
 * and redistribution of this file and for a
 * DISCLAIMER OF ALL WARRANTIES.
 */

#include <windows.h>
#include <sql.h>
#include <sqlext.h>
#include <odbcinst.h>
#include <winver.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

#ifdef SEEEXT
#define SEESTR " (SEE)"
#define SEESTR2 "SEE "
#else
#define SEEEXT ""
#define SEESTR ""
#define SEESTR2 ""
#endif

#define NUMDRVS 4
static char *DriverName[NUMDRVS] = {
    "SQLite ODBC Driver",
    "SQLite ODBC (UTF-8) Driver",
    "SQLite3 ODBC Driver" SEESTR,
    "SQLite4 ODBC Driver"
};
static char *DSName[NUMDRVS] = {
    "SQLite Datasource",
    "SQLite UTF-8 Datasource",
    "SQLite3 " SEESTR2 "Datasource",
    "SQLite4 Datasource"
};
static char *DriverDLL[NUMDRVS] = {
    "sqliteodbc.dll",
    "sqliteodbcu.dll",
    "sqlite3odbc" SEEEXT ".dll",
    "sqlite4odbc.dll"
};
#ifdef WITH_SQLITE_DLLS
static char *EngineDLL[NUMDRVS] = {
    "sqlite.dll",
    "sqliteu.dll",
    "sqlite3.dll",
    "sqlite4.dll"
};
#endif

static int quiet = 0;
static int nosys = 0;

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
 * Copy or delete SQLite3 module DLLs
 * @param dllname file name of driver DLL
 * @param path install directory for modules
 * @param del flag, when true, delete DLLs in install directory
 */

static BOOL
CopyOrDelModules(char *dllname, char *path, BOOL del)
{
    char firstpat[MAX_PATH];
    WIN32_FIND_DATA fdata;
    HANDLE h;
    DWORD err;

    if (strncmp(dllname, "sqlite3", 7)) {
	return TRUE;
    }
    firstpat[0] = '\0';
    if (del) {
	strcpy(firstpat, path);
	strcat(firstpat, "\\");
    }
    strcat(firstpat, "sqlite3_mod*.dll");
    h = FindFirstFile(firstpat, &fdata);
    if (h == INVALID_HANDLE_VALUE) {
	return TRUE;
    }
    do {
	if (del) {
	    DeleteFile(fdata.cFileName);
	} else {
	    char buf[1024];

	    sprintf(buf, "%s\\%s", path, fdata.cFileName);
	    if (!CopyFile(fdata.cFileName, buf, 0)) {
		sprintf(buf, "Copy %s to %s failed", fdata.cFileName, path);
		MessageBox(NULL, buf, "CopyFile",
			   MB_ICONSTOP|MB_OK|MB_TASKMODAL|MB_SETFOREGROUND); 
		FindClose(h);
		return FALSE;
	    }
	}
    } while (FindNextFile(h, &fdata));
    err = GetLastError();
    FindClose(h);
    return err == ERROR_NO_MORE_FILES;
}

/**
 * Driver installer/uninstaller.
 * @param remove true for uninstall
 * @param drivername print name of driver
 * @param dllname file name of driver DLL
 * @param dll2name file name of additional DLL
 * @param dsname name for data source
 */

static BOOL
InUn(int remove, char *drivername, char *dllname, char *dll2name, char *dsname)
{
    char path[301], driver[300], attr[300], inst[400], inst2[400];
    WORD pathmax = sizeof (path) - 1, pathlen;
    DWORD usecnt, mincnt;

    if (SQLInstallDriverManager(path, pathmax, &pathlen)) {
	char *p;

	sprintf(driver, "%s;Driver=%s;Setup=%s;",
		drivername, dllname, dllname);
	p = driver;
	while (*p) {
	    if (*p == ';') {
		*p = '\0';
	    }
	    ++p;
	}
	usecnt = 0;
	SQLInstallDriverEx(driver, NULL, path, pathmax, &pathlen,
			   ODBC_INSTALL_INQUIRY, &usecnt);
	sprintf(driver, "%s;Driver=%s\\%s;Setup=%s\\%s;",
		drivername, path, dllname, path, dllname);
	p = driver;
	while (*p) {
	    if (*p == ';') {
		*p = '\0';
	    }
	    ++p;
	}
	sprintf(inst, "%s\\%s", path, dllname);
	if (dll2name) {
	    sprintf(inst2, "%s\\%s", path, dll2name);
	}
	if (!remove && usecnt > 0) {
	    /* first install try: copy over driver dll, keeping DSNs */
	    if (GetFileAttributes(dllname) != INVALID_FILE_ATTRIBUTES &&
		CopyFile(dllname, inst, 0) &&
		CopyOrDelModules(dllname, path, 0)) {
		if (dll2name != NULL) {
		    CopyFile(dll2name, inst2, 0);
		}
		return TRUE;
	    }
	}
	mincnt = remove ? 1 : 0;
	while (usecnt != mincnt) {
	    if (!SQLRemoveDriver(driver, TRUE, &usecnt)) {
		break;
	    }
	}
	if (remove) {
	    if (!SQLRemoveDriver(driver, TRUE, &usecnt)) {
		ProcessErrorMessages("SQLRemoveDriver");
		return FALSE;
	    }
	    if (!usecnt) {
		char buf[512];

		DeleteFile(inst);
		/* but keep inst2 */
		CopyOrDelModules(dllname, path, 1);
		if (!quiet) {
		    sprintf(buf, "%s uninstalled.", drivername);
		    MessageBox(NULL, buf, "Info",
			       MB_ICONINFORMATION|MB_OK|MB_TASKMODAL|
			       MB_SETFOREGROUND);
		}
	    }
	    if (nosys) {
		goto done;
	    }
	    sprintf(attr, "DSN=%s;Database=sqlite.db;", dsname);
	    p = attr;
	    while (*p) {
		if (*p == ';') {
		    *p = '\0';
		}
		++p;
	    }
	    SQLConfigDataSource(NULL, ODBC_REMOVE_SYS_DSN, drivername, attr);
	    goto done;
	}
	if (GetFileAttributes(dllname) == INVALID_FILE_ATTRIBUTES) {
	    return FALSE;
	}
	if (!CopyFile(dllname, inst, 0)) {
	    char buf[512];

	    sprintf(buf, "Copy %s to %s failed", dllname, inst);
	    MessageBox(NULL, buf, "CopyFile",
		       MB_ICONSTOP|MB_OK|MB_TASKMODAL|MB_SETFOREGROUND); 
	    return FALSE;
	}
	if (dll2name != NULL && !CopyFile(dll2name, inst2, 0)) {
	    char buf[512];

	    sprintf(buf, "Copy %s to %s failed", dll2name, inst2);
	    MessageBox(NULL, buf, "CopyFile",
		       MB_ICONSTOP|MB_OK|MB_TASKMODAL|MB_SETFOREGROUND); 
	    /* but go on hoping that an SQLite engine is in place */
	}
	if (!CopyOrDelModules(dllname, path, 0)) {
	    return FALSE;
	}
	if (!SQLInstallDriverEx(driver, path, path, pathmax, &pathlen,
				ODBC_INSTALL_COMPLETE, &usecnt)) {
	    ProcessErrorMessages("SQLInstallDriverEx");
	    return FALSE;
	}
	if (nosys) {
	    goto done;
	}
	sprintf(attr, "DSN=%s;Database=sqlite.db;", dsname);
	p = attr;
	while (*p) {
	    if (*p == ';') {
		*p = '\0';
	    }
	    ++p;
	}
	SQLConfigDataSource(NULL, ODBC_REMOVE_SYS_DSN, drivername, attr);
	if (!SQLConfigDataSource(NULL, ODBC_ADD_SYS_DSN, drivername, attr)) {
	    ProcessErrorMessages("SQLConfigDataSource");
	    return FALSE;
	}
    } else {
	ProcessErrorMessages("SQLInstallDriverManager");
	return FALSE;
    }
done:
    return TRUE;
}

/**
 * Main function of installer/uninstaller.
 * This is the Win32 GUI main entry point.
 * It (un)registers the ODBC driver(s) and deletes or
 * copies the driver DLL(s) to the system folder.
 */

int APIENTRY
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPSTR lpszCmdLine, int nCmdShow)
{
    char path[300], *p;
    int i, remove;
    BOOL ret[3];

    GetModuleFileName(NULL, path, sizeof (path));
    p = path;
    while (*p) {
	*p = tolower(*p);
	++p;
    }
    p = strrchr(path, '\\');
    if (p == NULL) {
	p = path;
    } else {
	*p = '\0';
	++p;
	SetCurrentDirectory(path);
    }
    remove = strstr(p, "uninst") != NULL;
    quiet = strstr(p, "instq") != NULL;
    nosys = strstr(p, "nosys") != NULL;
    for (i = 0; i < NUMDRVS; i++) {
#ifdef WITH_SQLITE_DLLS
	p = EngineDLL[i];
#else
	p = NULL;
#endif
	ret[i] = InUn(remove, DriverName[i], DriverDLL[i], p, DSName[i]);
    }
    for (i = 1; i < NUMDRVS; i++) {
	ret[0] = ret[0] || ret[i];
    }
    if (!remove && ret[0]) {
	if (!quiet) {
	    MessageBox(NULL, "SQLite ODBC Driver(s) installed.", "Info",
		       MB_ICONINFORMATION|MB_OK|MB_TASKMODAL|MB_SETFOREGROUND);
	}
    }
    exit(0);
}

