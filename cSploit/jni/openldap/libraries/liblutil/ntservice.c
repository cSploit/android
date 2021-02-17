/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1998-2014 The OpenLDAP Foundation.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in the file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */

/*
 * NT Service manager utilities for OpenLDAP services
 */

#include "portable.h"

#ifdef HAVE_NT_SERVICE_MANAGER

#include <ac/stdlib.h>
#include <ac/string.h>

#include <stdio.h>

#include <windows.h>
#include <winsvc.h>

#include <ldap.h>

#include "ldap_pvt_thread.h"

#include "ldap_defaults.h"

#include "slapdmsg.h"

#define SCM_NOTIFICATION_INTERVAL	5000
#define THIRTY_SECONDS				(30 * 1000)

int	  is_NT_Service;	/* is this is an NT service? */

SERVICE_STATUS			lutil_ServiceStatus;
SERVICE_STATUS_HANDLE	hlutil_ServiceStatus;

ldap_pvt_thread_cond_t	started_event,		stopped_event;
ldap_pvt_thread_t		start_status_tid,	stop_status_tid;

void (*stopfunc)(int);

static char *GetLastErrorString( void );

int lutil_srv_install(LPCTSTR lpszServiceName, LPCTSTR lpszDisplayName,
		LPCTSTR lpszBinaryPathName, int auto_start)
{
	HKEY		hKey;
	DWORD		dwValue, dwDisposition;
	SC_HANDLE	schSCManager, schService;
	char *sp = strchr( lpszBinaryPathName, ' ');

	if ( sp ) *sp = '\0';
	fprintf( stderr, "The install path is %s.\n", lpszBinaryPathName );
	if ( sp ) *sp = ' ';
	if ((schSCManager = OpenSCManager( NULL, NULL, SC_MANAGER_CONNECT|SC_MANAGER_CREATE_SERVICE ) ) != NULL )
	{
	 	if ((schService = CreateService( 
							schSCManager, 
							lpszServiceName, 
							lpszDisplayName, 
							SERVICE_ALL_ACCESS, 
							SERVICE_WIN32_OWN_PROCESS, 
							auto_start ? SERVICE_AUTO_START : SERVICE_DEMAND_START, 
							SERVICE_ERROR_NORMAL, 
							lpszBinaryPathName, 
							NULL, NULL, NULL, NULL, NULL)) != NULL)
		{
			char regpath[132];
			CloseServiceHandle(schService);
			CloseServiceHandle(schSCManager);

			snprintf( regpath, sizeof regpath,
				"SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\%s",
				lpszServiceName );
			/* Create the registry key for event logging to the Windows NT event log. */
			if ( RegCreateKeyEx(HKEY_LOCAL_MACHINE, 
				regpath, 0, 
				"REG_SZ", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, 
				&dwDisposition) != ERROR_SUCCESS)
			{
				fprintf( stderr, "RegCreateKeyEx() failed. GetLastError=%lu (%s)\n", GetLastError(), GetLastErrorString() );
				RegCloseKey(hKey);
				return(0);
			}
			if ( sp ) *sp = '\0';
			if ( RegSetValueEx(hKey, "EventMessageFile", 0, REG_EXPAND_SZ, lpszBinaryPathName, strlen(lpszBinaryPathName) + 1) != ERROR_SUCCESS)
			{
				fprintf( stderr, "RegSetValueEx(EventMessageFile) failed. GetLastError=%lu (%s)\n", GetLastError(), GetLastErrorString() );
				RegCloseKey(hKey);
				return(0);
			}

			dwValue = EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE | EVENTLOG_INFORMATION_TYPE;
			if ( RegSetValueEx(hKey, "TypesSupported", 0, REG_DWORD, (LPBYTE) &dwValue, sizeof(DWORD)) != ERROR_SUCCESS) 
			{
				fprintf( stderr, "RegCreateKeyEx(TypesSupported) failed. GetLastError=%lu (%s)\n", GetLastError(), GetLastErrorString() );
				RegCloseKey(hKey);
				return(0);
			}
			RegCloseKey(hKey);
			return(1);
		}
		else
		{
			fprintf( stderr, "CreateService() failed. GetLastError=%lu (%s)\n", GetLastError(), GetLastErrorString() );
			CloseServiceHandle(schSCManager);
			return(0);
		}
	}
	else
		fprintf( stderr, "OpenSCManager() failed. GetLastError=%lu (%s)\n", GetLastError(), GetLastErrorString() );
	return(0);
}


int lutil_srv_remove(LPCTSTR lpszServiceName, LPCTSTR lpszBinaryPathName)
{
	SC_HANDLE schSCManager, schService;

	fprintf( stderr, "The installed path is %s.\n", lpszBinaryPathName );
	if ((schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT|SC_MANAGER_CREATE_SERVICE)) != NULL ) 
	{
	 	if ((schService = OpenService(schSCManager, lpszServiceName, DELETE)) != NULL) 
		{
			if ( DeleteService(schService) == TRUE) 
			{
				CloseServiceHandle(schService);
				CloseServiceHandle(schSCManager);
				return(1);
			} else {
				fprintf( stderr, "DeleteService() failed. GetLastError=%lu (%s)\n", GetLastError(), GetLastErrorString() );
				fprintf( stderr, "The %s service has not been removed.\n", lpszBinaryPathName);
				CloseServiceHandle(schService);
				CloseServiceHandle(schSCManager);
				return(0);
			}
		} else {
			fprintf( stderr, "OpenService() failed. GetLastError=%lu (%s)\n", GetLastError(), GetLastErrorString() );
			CloseServiceHandle(schSCManager);
			return(0);
		}
	}
	else
		fprintf( stderr, "OpenSCManager() failed. GetLastError=%lu (%s)\n", GetLastError(), GetLastErrorString() );
	return(0);
}


#if 0 /* unused */
DWORD
svc_installed (LPTSTR lpszServiceName, LPTSTR lpszBinaryPathName)
{
	char buf[256];
	HKEY key;
	DWORD rc;
	DWORD type;
	long len;

	strcpy(buf, TEXT("SYSTEM\\CurrentControlSet\\Services\\"));
	strcat(buf, lpszServiceName);
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, buf, 0, KEY_QUERY_VALUE, &key) != ERROR_SUCCESS)
		return(-1);

	rc = 0;
	if (lpszBinaryPathName) {
		len = sizeof(buf);
		if (RegQueryValueEx(key, "ImagePath", NULL, &type, buf, &len) == ERROR_SUCCESS) {
			if (strcmp(lpszBinaryPathName, buf))
				rc = -1;
		}
	}
	RegCloseKey(key);
	return(rc);
}


DWORD
svc_running (LPTSTR lpszServiceName)
{
	SC_HANDLE service;
	SC_HANDLE scm;
	DWORD rc;
	SERVICE_STATUS ss;

	if (!(scm = OpenSCManager(NULL, NULL, GENERIC_READ)))
		return(GetLastError());

	rc = 1;
	service = OpenService(scm, lpszServiceName, SERVICE_QUERY_STATUS);
	if (service) {
		if (!QueryServiceStatus(service, &ss))
			rc = GetLastError();
		else if (ss.dwCurrentState != SERVICE_STOPPED)
			rc = 0;
		CloseServiceHandle(service);
	}
	CloseServiceHandle(scm);
	return(rc);
}
#endif

static void *start_status_routine( void *ptr )
{
	DWORD	wait_result;
	int		done = 0;

	while ( !done )
	{
		wait_result = WaitForSingleObject( started_event, SCM_NOTIFICATION_INTERVAL );
		switch ( wait_result )
		{
			case WAIT_ABANDONED:
			case WAIT_OBJECT_0:
				/* the object that we were waiting for has been destroyed (ABANDONED) or
				 * signalled (TIMEOUT_0). We can assume that the startup process is
				 * complete and tell the Service Control Manager that we are now runnng */
				lutil_ServiceStatus.dwCurrentState	= SERVICE_RUNNING;
				lutil_ServiceStatus.dwWin32ExitCode	= NO_ERROR;
				lutil_ServiceStatus.dwCheckPoint++;
				lutil_ServiceStatus.dwWaitHint		= 1000;
				SetServiceStatus(hlutil_ServiceStatus, &lutil_ServiceStatus);
				done = 1;
				break;
			case WAIT_TIMEOUT:
				/* We've waited for the required time, so send an update to the Service Control 
				 * Manager saying to wait again. */
				lutil_ServiceStatus.dwCheckPoint++;
				lutil_ServiceStatus.dwWaitHint = SCM_NOTIFICATION_INTERVAL * 2;
				SetServiceStatus(hlutil_ServiceStatus, &lutil_ServiceStatus);
				break;
			case WAIT_FAILED:
				/* theres been some problem with WaitForSingleObject so tell the Service
				 * Control Manager to wait 30 seconds before deploying its assasin and 
				 * then leave the thread. */
				lutil_ServiceStatus.dwCheckPoint++;
				lutil_ServiceStatus.dwWaitHint = THIRTY_SECONDS;
				SetServiceStatus(hlutil_ServiceStatus, &lutil_ServiceStatus);
				done = 1;
				break;
		}
	}
	ldap_pvt_thread_exit(NULL);
	return NULL;
}



static void *stop_status_routine( void *ptr )
{
	DWORD	wait_result;
	int		done = 0;

	while ( !done )
	{
		wait_result = WaitForSingleObject( stopped_event, SCM_NOTIFICATION_INTERVAL );
		switch ( wait_result )
		{
			case WAIT_ABANDONED:
			case WAIT_OBJECT_0:
				/* the object that we were waiting for has been destroyed (ABANDONED) or
				 * signalled (TIMEOUT_0). The shutting down process is therefore complete 
				 * and the final SERVICE_STOPPED message will be sent to the service control
				 * manager prior to the process terminating. */
				done = 1;
				break;
			case WAIT_TIMEOUT:
				/* We've waited for the required time, so send an update to the Service Control 
				 * Manager saying to wait again. */
				lutil_ServiceStatus.dwCheckPoint++;
				lutil_ServiceStatus.dwWaitHint = SCM_NOTIFICATION_INTERVAL * 2;
				SetServiceStatus(hlutil_ServiceStatus, &lutil_ServiceStatus);
				break;
			case WAIT_FAILED:
				/* theres been some problem with WaitForSingleObject so tell the Service
				 * Control Manager to wait 30 seconds before deploying its assasin and 
				 * then leave the thread. */
				lutil_ServiceStatus.dwCheckPoint++;
				lutil_ServiceStatus.dwWaitHint = THIRTY_SECONDS;
				SetServiceStatus(hlutil_ServiceStatus, &lutil_ServiceStatus);
				done = 1;
				break;
		}
	}
	ldap_pvt_thread_exit(NULL);
	return NULL;
}



static void WINAPI lutil_ServiceCtrlHandler( IN DWORD Opcode)
{
	switch (Opcode)
	{
	case SERVICE_CONTROL_STOP:
	case SERVICE_CONTROL_SHUTDOWN:

		lutil_ServiceStatus.dwCurrentState	= SERVICE_STOP_PENDING;
		lutil_ServiceStatus.dwCheckPoint++;
		lutil_ServiceStatus.dwWaitHint		= SCM_NOTIFICATION_INTERVAL * 2;
		SetServiceStatus(hlutil_ServiceStatus, &lutil_ServiceStatus);

		ldap_pvt_thread_cond_init( &stopped_event );
		if ( stopped_event == NULL )
		{
			/* the event was not created. We will ask the service control manager for 30
			 * seconds to shutdown */
			lutil_ServiceStatus.dwCheckPoint++;
			lutil_ServiceStatus.dwWaitHint		= THIRTY_SECONDS;
			SetServiceStatus(hlutil_ServiceStatus, &lutil_ServiceStatus);
		}
		else
		{
			/* start a thread to report the progress to the service control manager 
			 * until the stopped_event is fired. */
			if ( ldap_pvt_thread_create( &stop_status_tid, 0, stop_status_routine, NULL ) == 0 )
			{
				
			}
			else {
				/* failed to create the thread that tells the Service Control Manager that the
				 * service stopping is proceeding. 
				 * tell the Service Control Manager to wait another 30 seconds before deploying its
				 * assasin.  */
				lutil_ServiceStatus.dwCheckPoint++;
				lutil_ServiceStatus.dwWaitHint = THIRTY_SECONDS;
				SetServiceStatus(hlutil_ServiceStatus, &lutil_ServiceStatus);
			}
		}
		stopfunc( -1 );
		break;

	case SERVICE_CONTROL_INTERROGATE:
		SetServiceStatus(hlutil_ServiceStatus, &lutil_ServiceStatus);
		break;
	}
	return;
}

void *lutil_getRegParam( char *svc, char *value )
{
	HKEY hkey;
	char path[255];
	DWORD vType;
	static char vValue[1024];
	DWORD valLen = sizeof( vValue );

	if ( svc != NULL )
		snprintf ( path, sizeof path, "SOFTWARE\\%s", svc );
	else
		snprintf ( path, sizeof path, "SOFTWARE\\OpenLDAP\\Parameters" );
	
	if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE, path, 0, KEY_READ, &hkey ) != ERROR_SUCCESS )
	{
		return NULL;
	}

	if ( RegQueryValueEx( hkey, value, NULL, &vType, vValue, &valLen ) != ERROR_SUCCESS )
	{
		RegCloseKey( hkey );
		return NULL;
	}
	RegCloseKey( hkey );
	
	switch ( vType )
	{
	case REG_BINARY:
	case REG_DWORD:
		return (void*)&vValue;
	case REG_SZ:
		return (void*)&vValue;
	}
	return (void*)NULL;
}

void lutil_LogStartedEvent( char *svc, int slap_debug, char *configfile, char *urls )
{
	char *Inserts[5];
	WORD i = 0, j;
	HANDLE hEventLog;
	
	hEventLog = RegisterEventSource( NULL, svc );

	Inserts[i] = (char *)malloc( 20 );
	itoa( slap_debug, Inserts[i++], 10 );
	Inserts[i++] = strdup( configfile );
	Inserts[i++] = strdup( urls ? urls : "ldap:///" );

	ReportEvent( hEventLog, EVENTLOG_INFORMATION_TYPE, 0,
		MSG_SVC_STARTED, NULL, i, 0, (LPCSTR *) Inserts, NULL );

	for ( j = 0; j < i; j++ )
		ldap_memfree( Inserts[j] );
	DeregisterEventSource( hEventLog );
}



void lutil_LogStoppedEvent( char *svc )
{
	HANDLE hEventLog;
	
	hEventLog = RegisterEventSource( NULL, svc );
	ReportEvent( hEventLog, EVENTLOG_INFORMATION_TYPE, 0,
		MSG_SVC_STOPPED, NULL, 0, 0, NULL, NULL );
	DeregisterEventSource( hEventLog );
}


void lutil_CommenceStartupProcessing( char *lpszServiceName,
							   void (*stopper)(int) )
{
	hlutil_ServiceStatus = RegisterServiceCtrlHandler( lpszServiceName, (LPHANDLER_FUNCTION)lutil_ServiceCtrlHandler);

	stopfunc = stopper;

	/* initialize the Service Status structure */
	lutil_ServiceStatus.dwServiceType				= SERVICE_WIN32_OWN_PROCESS;
	lutil_ServiceStatus.dwCurrentState				= SERVICE_START_PENDING;
	lutil_ServiceStatus.dwControlsAccepted			= SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
	lutil_ServiceStatus.dwWin32ExitCode				= NO_ERROR;
	lutil_ServiceStatus.dwServiceSpecificExitCode	= 0;
	lutil_ServiceStatus.dwCheckPoint					= 1;
	lutil_ServiceStatus.dwWaitHint					= SCM_NOTIFICATION_INTERVAL * 2;

	SetServiceStatus(hlutil_ServiceStatus, &lutil_ServiceStatus);

	/* start up a thread to keep sending SERVICE_START_PENDING to the Service Control Manager
	 * until the slapd listener is completed and listening. Only then should we send 
	 * SERVICE_RUNNING to the Service Control Manager. */
	ldap_pvt_thread_cond_init( &started_event );
	if ( started_event == NULL)
	{
		/* failed to create the event to determine when the startup process is complete so
		 * tell the Service Control Manager to wait another 30 seconds before deploying its
		 * assasin  */
		lutil_ServiceStatus.dwCheckPoint++;
		lutil_ServiceStatus.dwWaitHint = THIRTY_SECONDS;
		SetServiceStatus(hlutil_ServiceStatus, &lutil_ServiceStatus);
	}
	else
	{
		/* start a thread to report the progress to the service control manager 
		 * until the started_event is fired.  */
		if ( ldap_pvt_thread_create( &start_status_tid, 0, start_status_routine, NULL ) == 0 )
		{
			
		}
		else {
			/* failed to create the thread that tells the Service Control Manager that the
			 * service startup is proceeding. 
			 * tell the Service Control Manager to wait another 30 seconds before deploying its
			 * assasin.  */
			lutil_ServiceStatus.dwCheckPoint++;
			lutil_ServiceStatus.dwWaitHint = THIRTY_SECONDS;
			SetServiceStatus(hlutil_ServiceStatus, &lutil_ServiceStatus);
		}
	}
}

void lutil_ReportShutdownComplete(  )
{
	if ( is_NT_Service )
	{
		/* stop sending SERVICE_STOP_PENDING messages to the Service Control Manager */
		ldap_pvt_thread_cond_signal( &stopped_event );
		ldap_pvt_thread_cond_destroy( &stopped_event );

		/* wait for the thread sending the SERVICE_STOP_PENDING messages to the Service Control Manager to die.
		 * if the wait fails then put ourselves to sleep for half the Service Control Manager update interval */
		if (ldap_pvt_thread_join( stop_status_tid, (void *) NULL ) == -1)
			ldap_pvt_thread_sleep( SCM_NOTIFICATION_INTERVAL / 2 );

		lutil_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
		lutil_ServiceStatus.dwCheckPoint++;
		lutil_ServiceStatus.dwWaitHint = SCM_NOTIFICATION_INTERVAL;
		SetServiceStatus(hlutil_ServiceStatus, &lutil_ServiceStatus);
	}
}

static char *GetErrorString( int err )
{
	static char msgBuf[1024];

	FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		msgBuf, 1024, NULL );

	return msgBuf;
}

static char *GetLastErrorString( void )
{
	return GetErrorString( GetLastError() );
}
#endif
