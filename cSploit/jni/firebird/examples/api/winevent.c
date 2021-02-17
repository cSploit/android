/*
 * Program type: API
 * 
 * EVENTS.C -- Example events program 
 *
 *      Description:    This program does an asynchronous event wait
 *                      on a trigger set up in the sales table of
 *                      employee.fdb.  
 *                      Somebody must add a new sales order to alert
 *                      this program.  That role can be accomplished 
 *                      by running the api16t example program
 *                      after starting this program.
 *
 *        Note: The system administrator needs to create the account
 *        "guest" with password "guest" on the server before this
 *        example can be run.
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

#include <windows.h>
#include <windowsx.h>
#include <ibase.h>
#include <stdio.h>
#include <string.h>

/*
** Local Defines
*/
#define USER     "guest"
#define PASSWORD "guest"
#define DATABASE "employee.fdb"

#define IDM_EXIT        1
#define WM_DB_EVENT     WM_USER + 1

/*
** This is an event block defined to keep track of all the parameters
** used to queue and proecess events.  A pointer to this structure
** will be passed as the user argument to gds_que_events.
*/
typedef struct _EventBlk {
    char                *EventBuf;
    char                *ResultBuf;
    short               length;
    long                EventId;
    char                *EventName;
    isc_callback        lpAstProc;
    isc_db_handle       DB; 
    HWND                hWnd;
    struct _EventBlk    *NextBlk;
} EVENTBLK;


/*
** GLOBAL VARIABLES 
*/
char            szAppName [] = "Events";
HINSTANCE       hInstance;
ISC_STATUS_ARRAY status;


/* 
** FUNCTION PROTOTYPES 
*/

LRESULT CALLBACK _export WndProc (HWND, UINT, WPARAM, LPARAM) ;
HWND                    InitApplication(int nCmdShow, HINSTANCE hPrevInstance);
int                     InitEvent(EVENTBLK *lpEvent, long *DB,
                                  HWND hWnd, char *event);
void                    ReleaseEvents(EVENTBLK *lpEvent);

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  		/* __cplusplus */
void                    AstRoutine(EVENTBLK *result, short length,
                                   char *updated);
#ifdef __cplusplus
}
#endif  		/* __cplusplus */

void                    WinPrint(char*);
int                     CHK_ERR(long *gds__status);


/*****************************************************************
 *
 * WinMain
 *
 *****************************************************************
 *
 * Functional description
 *      Description:    Setup the dpb with the username and password
 *			and attach to the database, employee.fdb.
 *                      If the attach was successful, allocate an event
 *                      control block and register interest in the
 *			event "new_order".
 *      
 *
 ****************************************************************/
int WINAPI WinMain (HINSTANCE hInst, HINSTANCE hPrevInstance,
                    LPSTR lpszCmdLine, int nCmdShow)
{
    MSG                 msg;
    isc_db_handle       DB = NULL;
    HWND                hWnd;
    EVENTBLK            *lpEventNewOrder = NULL;
    char                dpb[48];
    int                 i = 0, len;


    hInstance = hInst; 
    hWnd = InitApplication(nCmdShow, hPrevInstance);

    if (!hWnd)
    {
        WinPrint("Unable to initialize main window");
        /* Get rid of warning for both Borland and Microsoft */
        lpszCmdLine = lpszCmdLine;
        return FALSE;
    }

    /* Format the dpb with the user name a password */
    dpb[i++] = isc_dpb_version1;
    
    dpb[i++] = isc_dpb_user_name;
    len = strlen (USER);
    dpb[i++] = (char) len;
    strncpy(&(dpb[i]), USER, len);
    i += len;
    
    dpb[i++] = isc_dpb_password;
    len = strlen (PASSWORD);
    dpb[i++] = len;
    strncpy(&(dpb[i]), PASSWORD, len);
    i += len;

    isc_attach_database(status, 0, DATABASE, &(DB), i, dpb);
         
    /* If the attach was successful, initialize the event handlers */
    if (!CHK_ERR(status))
    {
        /* Allocate our event control block to hold all the event parameters */
        lpEventNewOrder = (EVENTBLK *) GlobalAllocPtr(
                                        GMEM_MOVEABLE | GMEM_ZEROINIT,
                                        sizeof(EVENTBLK));

        /* Register interest in the "new_order" event */
        InitEvent(lpEventNewOrder, (long *)DB, hWnd, "new_order");
    }

    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    isc_detach_database(status, &DB);

    /* Release the allocated event blocks */
    ReleaseEvents(lpEventNewOrder);

    return msg.wParam;
}

/*****************************************************************
 *
 * InitApplication
 *
 *****************************************************************
 *
 * Functional description:
 *      Registers the window class and displays the main window.
 * Returns:
 *      window handle on success, FALSE otherwise
 *
 *****************************************************************/
HWND InitApplication (int nCmdShow, HINSTANCE hPrevInstance)
{
    WNDCLASS    wndclass;
    HWND        hWnd;

    if (!hPrevInstance)
    {
        wndclass.style         = CS_HREDRAW | CS_VREDRAW;
        wndclass.lpfnWndProc   = WndProc;
        wndclass.cbClsExtra    = 0;
        wndclass.cbWndExtra    = 0;
        wndclass.hInstance     = hInstance;
        wndclass.hIcon         = LoadIcon (NULL, IDI_APPLICATION);
        wndclass.hCursor       = LoadCursor (NULL, IDC_ARROW);
        wndclass.hbrBackground = (HBRUSH)GetStockObject (WHITE_BRUSH);
        wndclass.lpszMenuName  = szAppName;
        wndclass.lpszClassName = szAppName;

        if (!RegisterClass(&wndclass))
            return NULL;
    }

    if ((hWnd = CreateWindow(szAppName, "Event Test",
                             WS_OVERLAPPEDWINDOW,
                             CW_USEDEFAULT, CW_USEDEFAULT,
                             CW_USEDEFAULT, CW_USEDEFAULT,
                             NULL, NULL, hInstance, NULL)) == NULL )
        return NULL;


    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    return hWnd;
}


/*****************************************************************
 *
 * InitEvents
 *
 *****************************************************************
 *
 * Functional description:
 *      Initialize the event_block, and queue interest in a particular
 *      event.
 * Returns:
 *      TRUE if success, FALSE if que_events failed.
 *
 *****************************************************************/
int InitEvent (EVENTBLK *lpEvent, long *DB, HWND hWnd, char *event)
{
    /* Allocate the event buffers and initialize the events of interest */
    lpEvent->length = (short)isc_event_block(&(lpEvent->EventBuf),
                                                &(lpEvent->ResultBuf),
                                             1, event);

    /* Store any necessary parameters in the event block */
    lpEvent->lpAstProc = (isc_callback) MakeProcInstance((FARPROC) AstRoutine, 
                                                         hInstance);
    lpEvent->DB = DB;
    lpEvent->hWnd = hWnd;
    lpEvent->EventName = event;

    /* Request the server to notify when any of our events occur */
    isc_que_events(status, &(lpEvent->DB), 
                   &(lpEvent->EventId), lpEvent->length, lpEvent->EventBuf,
                   lpEvent->lpAstProc, lpEvent);

    if (CHK_ERR(status))
        return FALSE;

    return TRUE;
}

/*****************************************************************
 *
 * ReleaseEvents
 *
 *****************************************************************
 *
 * Functional description:
 *      Releases the event buffers allocated by isc_event_block
 *      and releases the main event block.
 * Returns:
 *      none
 *
 *****************************************************************/
void ReleaseEvents (EVENTBLK *lpEvent)
{
    if (lpEvent == NULL)
        return;

    isc_free(lpEvent->EventBuf);
    isc_free(lpEvent->ResultBuf);

    (VOID)GlobalFreePtr(lpEvent);
}

/*****************************************************************
 *
 * WndProc
 *
 *****************************************************************
 *
 * Functional description:
 *      Main window processing function.  
 * Returns:
 *      0 if the message was handled, 
 *      results from DefWindowProc otherwise
 *
 *****************************************************************/
LRESULT CALLBACK _export WndProc (HWND hWnd, UINT message, WPARAM wParam,
                                 LPARAM lParam)
{
    EVENTBLK   		*lpEvent;
    ISC_STATUS_ARRAY	Vector;
    char       		msgbuf[200];

    switch (message)
    {
        case WM_COMMAND:
            switch (wParam)
            {
                case IDM_EXIT:
                    SendMessage(hWnd, WM_CLOSE, 0, 0L);
                    return 0;
            }
            break;

        case WM_DB_EVENT:
            /* The event block is passed in lParam by the ast */
            lpEvent = (EVENTBLK *)lParam;

            /*
            ** isc_event_counts will update Vector with the number
            ** of times each event occurred.  It will then copy
            ** ResultBuf into EventBuf to prepare for the next que_events.
            */
            isc_event_counts(Vector, lpEvent->length, lpEvent->EventBuf,
                             lpEvent->ResultBuf);

            sprintf(msgbuf, "Event %s triggered with count %ld\n...Resetting"
                    " count and calling gds_que_events again",
                    lpEvent->EventName, Vector[0]);
            WinPrint(msgbuf);

            /* Re-queue the event, so we can do this all over again */
            isc_que_events(status, &(lpEvent->DB), &(lpEvent->EventId),
                           lpEvent->length, lpEvent->EventBuf,
                             lpEvent->lpAstProc, lpEvent);
            CHK_ERR(status);

            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}
    
/*****************************************************************
 *
 * AstRoutine
 *
 *****************************************************************
 *
 * Functional description:
 *      This is the callback routine which is called by the
 *      gds library when an event occurs in the database.
 *      The event block which was passed as an argument to
 *      gds_que_events is received as the first argument,
 *      along with the updated event buffer and its length.
 * Returns:
 *      none
 *
 *****************************************************************/
void AstRoutine (EVENTBLK *lpEvent, short length, char *updated)
{
    char *ResultBuf = lpEvent->ResultBuf;

    /* Update the result buffer with the new values */
    while (length--)
        *ResultBuf++ = *updated++;

    /* Let the parent window know the event triggered */
    PostMessage(lpEvent->hWnd, WM_DB_EVENT, 0, (LPARAM) lpEvent);
}

/*****************************************************************
 *
 * WinPrint
 *
 *****************************************************************
 *
 * Functional description:
 *      Print a message in a message box.
 * Returns:
 *      none
 *
 *****************************************************************/
void WinPrint (char *line)
{
    MessageBox(NULL, line, szAppName, MB_ICONINFORMATION | MB_OK);
}

/*****************************************************************
 *
 * CHK_ERR
 *
 *****************************************************************
 *
 * Functional description:
 *      If an error was returned in the status vector, print it
 *      and the post a quit message to terminate the app.
 * Returns:
 *      TRUE if there was error, FALSE otherwise
 *
 *****************************************************************/
int CHK_ERR (long *status)       
{
    if (status[1])
    {
        isc_print_status(status);
        PostQuitMessage(1);
        return TRUE;
    }

    return FALSE;
}

