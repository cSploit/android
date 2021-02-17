/*! 
 *  \file
 *          
 *          This file contains constants and prototypes required to compile an
 *          Auto Test library/plugin. This is used by the unixODBC-Test and
 *          unixODBC-GUI-Qt projects.
 *  
 *  \note
 *  
 *          The contents of this file must be consistent with MS version so as to
 *          maintain source code portability. This should allow (for example)
 *          Auto Tests to be compiled for all platforms without source changes.
 *  
 */
#ifndef AUTOTEST_H
#define AUTOTEST_H

/* standard C stuff... */
#include <stdlib.h>
#include <string.h>

/* platform specific... */
#ifdef _WINDOWS
    #include <windows.h>
#endif

/* standard ODBC stuff... */
#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef WIN32
    #ifdef PATH_MAX
        #define _MAX_PATH           PATH_MAX
    #else
        #define _MAX_PATH           256
    #endif
#endif

extern  HINSTANCE           hLoadedInst;
    
/*----------------------------------------------------------------------------------
		Defines and Macros
----------------------------------------------------------------------------------*/

#define TEST_ABORTED						(-1)

#define AUTO_MAX_TEST_NAME				    35
#define AUTO_MAX_TESTCASE_NAME		        35
#define AUTO_MAX_TESTDESC_NAME		        75

#define MAXFLUSH		 	 			    300
#define MAX_USER_INFO	  				    50
#define MAX_KEYWORD_LEN	 			        149

/*  */
#ifdef WIN32
    #define EXTFUNCDECL                     _stdcall
    #define EXTFUN                          _stdcall
    #define MY_EXPORT                       __declspec(dllexport)
#else
    #define EXTFUNCDECL
    #define EXTFUN							
    #define MY_EXPORT 
#endif

#define InitTest(lps)															\
{ 	lps->cErrors=0; }
#define AbortTest(lps)															\
{ lps->cErrors=TEST_ABORTED; }

#define     AllocateMemory(cb)	            (calloc(cb,1))
#define     ReleaseMemory(lp)		        (free(lp))

#define NumItems(s) (sizeof(s) / sizeof(s[0]))

/* Following will access bit number pos in a bit array and return */
/*		TRUE if it is set, FALSE if it is not */
#define CQBITS (sizeof(unsigned int) * 8)
#define getqbit(lpa, pos)	\
	(lpa[((pos) / CQBITS)] & (1 << ((pos) - (CQBITS * ((pos) / CQBITS)))))
#define GETBIT(p1,p2) getqbit(p1,(p2)-1)

/*
 * Message box defines
 */

#ifndef WIN32
    #define MB_OK                               (0x0000)
    #define MB_ABORTRETRYIGNORE                 (0x0001)
    #define MB_OKCANCEL                         (0x0002)
    #define MB_RETRYCANCEL                      (0x0003)
    #define MB_YESNO                            (0x0004)
    #define MB_YESNOCANCEL                      (0x0005)
    
    #define MB_ICONEXCLAMATION                  (0x0000)
    #define MB_ICONWARNING                      MB_ICONEXCLAMATION
    #define MB_ICONINFORMATION                  (0x0010)
    #define MB_ICONASTERISK                     MB_ICONINFORMATION
    #define MB_ICONQUESTION                     (0x0020)
    #define MB_ICONSTOP                         (0x0030)
    #define MB_ICONERROR                        MB_ICONSTOP
    #define MB_ICONHAND                         MB_ICONSTOP
    
    #define MB_DEFBUTTON1                       (0x0000)
    #define MB_DEFBUTTON2                       (0x0100)
    #define MB_DEFBUTTON3                       (0x0200)
    #define MB_DEFBUTTON4                       (0x0300)
    
    #define MB_APPMODAL                         (0x0000)
    #define MB_SYSTEMMODAL                      (0x1000)
    #define MB_TASKMODAL                        (0x2000)
    
    #define MB_DEFAULT_DESKTOP_ONLY             (0x0000)
    #define MB_HELP                             (0x0000)
    #define MB_RIGHT                            (0x0000)
    #define MB_RTLREADING                       (0x0000)
    #define MB_SETFOREGROUND                    (0x0000)
    #define MB_TOPMOST                          (0x0000)
    #define MB_SERVICE_NOTIFICATION             (0x0000)
    #define MB_SERVICE_NOTIFICATION_NT3X        (0x0000)
#endif

/*! 
    This structure contains the information found in the .INI file for a 
    data source.  The filled out structure is in turn passed to AutoTestFunc
	to drive the individual tests.
*/
typedef struct tagSERVERINFO {
	HWND	 		hwnd;								/* Output edit window */
	CHAR   		    szLogFile[_MAX_PATH];		        /* Output log file */
	HENV 			henv;								/* .EXE's henv */
	HDBC 			hdbc;								/* .EXE's hdbc */
	HSTMT			hstmt;							    /* .EXE's hstmt */

	/* The following items are gathered from the .INI file and may be defined */
	/*		via the "Manage Test Sources" menu item from ODBC Test */
	CHAR 			szSource[SQL_MAX_DSN_LENGTH+1];
	CHAR 			szValidServer0[SQL_MAX_DSN_LENGTH+1];
	CHAR 			szValidLogin0[MAX_USER_INFO+1];
	CHAR 			szValidPassword0[MAX_USER_INFO+1];
	CHAR			szKeywords[MAX_KEYWORD_LEN+1];

	/* Following are used for run-time */
	UINT FAR * 	rglMask;  						    /* Run test mask */
	int  		failed;							    /* Track failures on a test case basis */
	int  		cErrors;						    /* Count of errors */
	BOOL 		fDebug;							    /* TRUE if debugging is to be enabled */
	BOOL 		fScreen;						    /* TRUE if test output goes to screen */
	BOOL 		fLog;							    /* TRUE if test output goes to log */
	BOOL 		fIsolate;						    /* TRUE to isolate output */
	UDWORD		vCursorLib;						    /* Value for SQL_ODBC_CURSOR on SQLSetConnectOption */
	HINSTANCE   hLoadedInst;					    /* Instance handle of loaded test */

	/* Following are used for buffering output to edit window */
	CHAR			szBuff[MAXFLUSH];				/* Hold temporary results */
	UINT			cBuff;							/* Number of TCHARs in szBuff */
	} SERVERINFO;
typedef SERVERINFO FAR * lpSERVERINFO;


BOOL EXTFUNCDECL FAR szLogPrintf(lpSERVERINFO lps, BOOL fForce, LPTSTR szFmt, ...);
int EXTFUNCDECL FAR szMessageBox(HWND hwnd, UINT style, LPTSTR szTitle, LPTSTR szFmt, ...);
LPTSTR EXTFUN GetRCString(HINSTANCE hInst, LPTSTR buf, int cbbuf, UINT ids);

#ifdef __cplusplus
}
#endif

#endif


