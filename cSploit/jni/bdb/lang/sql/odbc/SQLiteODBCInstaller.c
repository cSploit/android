/*
SQLiteODBCInstaller:
--------------------
Installs SQLiteODBC Driver.

Contact: 
Fjord-e-design GmbH, Kanzleistr. 91-93, D-24943 Flensburg
Telephone: +49 (0)461/480897-80, (Germany)
Fax: +49 (0)461/480897-81 (Germany)
EMail: hinrichsen@fjord-e-design.com
WWW: http://www.fjord-e-design.com
-----------------------------------------------------------------------

- History --------------------------------------------------------------
Version 1.0: 2006-08-11 by fjord-e-design GmbH
- Initial release.

Version 1.1: 2006-08-14 by fjord-e-design GmbH
- Change license from GNU to BSD.
- Replaced rundll32 to direct DLL access.
-----------------------------------------------------------------------

- LICENSE -------------------------------------------------------------
Copyright (c) 2006, fjord-e-design GmbH
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, 
	this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, 
	this list of conditions and the following disclaimer in the documentation 
	and/or other materials provided with the distribution.
    * Neither the name of the fjord-e-design nor the names of its contributors 
	may be used to endorse or promote products derived from this software without 
	specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED 
TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR 
PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE 
GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT 
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY 
OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF 
SUCH DAMAGE.
-----------------------------------------------------------------------
*/

#include "stdio.h"
#include "direct.h"
#include "string.h"
#include "windows.h"

// Version of Installer
#define SQLINST_VERSION				"1.2"

// Known Argument Types
#define SQLINST_ARGID_COUNT			9
#define SQLINST_ARGID_UNKNOWN		(-1)
#define SQLINST_ARGID_HELP			(0)
#define SQLINST_ARGID_VERSION		(1)
#define SQLINST_ARGID_INSTALL		(2)
#define SQLINST_ARGID_UNINSTALL		(3)
#define SQLINST_ARGID_ALL			(4)
#define SQLINST_ARGID_DRIVER		(5)
#define SQLINST_ARGID_QUIET			(6)
#define SQLINST_ARGID_SHOW_MSG_BOX	(7)
#define SQLINST_ARGID_DLL			(8)

// Some internel values....
#define SQLINST_MAX_BUFFER			(1024)
#define SQLINST_MAX_ARG_VALUE		(5)
#define SQLINST_MAX_ARGS			(10)

// Known driver
#define SQLINST_DRIVER_COUNT		(4)
#define SQLINST_DRIVER_SQL2			(0x01)
#define SQLINST_DRIVER_SQL2_UTF		(0x02)
#define SQLINST_DRIVER_SQL3			(0x04)
#define SQLINST_DRIVER_DLL			(0x08)

// Actions: What have I todo?
#define SQLINST_ACTION_NOT_KNOWN	(0)
#define SQLINST_ACTION_PRINT_HELP	(1)
#define SQLINST_ACTION_INSTALL		(2)
#define SQLINST_ACTION_UNINSTALL	(3)

// DLL-Function
typedef UINT (CALLBACK* InstallFunc)(HWND hwnd, HINSTANCE hinst, LPSTR lpszCmdLine, int nCmdShow);
typedef UINT (CALLBACK* UnInstallFunc)(HWND hwnd, HINSTANCE hinst, LPSTR lpszCmdLine, int nCmdShow);

struct SQLiteDriverData_Struct
{
	// Name for Argument
	const char		*pArgOptionName;
	// SQLINST_DRIVER_*
	const unsigned char	ArgFlag;
	
	// DriverName
	const char		*pDriverName;
	// DSN-Name
	const char		*pDSNName;
	// DLL-Name
	const char		*pDLLName;
};

// Define availably Driver (SQLINST_DRIVER_DLL = use given DLL)
static const struct SQLiteDriverData_Struct	g_SQLiteDriverData[SQLINST_DRIVER_COUNT] = \
{
	{	"sql2"	, SQLINST_DRIVER_SQL2		, "SQLite ODBC Driver"			, "SQLite Datasource"		, "sqliteodbc.dll"	},	
	{	"sql2u"	, SQLINST_DRIVER_SQL2_UTF	, "SQLite ODBC (UTF-8) Driver"		, "SQLite UTF-8 Datasource"	, "sqliteodbcu.dll"	},
	{	"sql3"	, SQLINST_DRIVER_SQL3		, "SQLite3 ODBC Driver"			, "SQLite3 Datasource"		, "sqlite3odbc.dll"	},
	{	""	, SQLINST_DRIVER_DLL		, ""					, ""				, ""			}
};

struct ArgData_Struct
{
	// Name of the Arg
	const char	*pArgName;
	// The id
	const char  ArgID;
	// Commet
	const char *pArgComment;
};

static const struct ArgData_Struct g_ArgData[SQLINST_ARGID_COUNT] =\
{
	{	"h", SQLINST_ARGID_HELP			, "-h = print help"	},
	{	"v", SQLINST_ARGID_VERSION		, "-v = print version"	},
	{	"i", SQLINST_ARGID_INSTALL		, "-i = install"	},
	{	"u", SQLINST_ARGID_UNINSTALL		, "-u = uninstall"	},
	{	"a", SQLINST_ARGID_ALL			, "-a = (un)install all driver (default)"	},
	{	"d", SQLINST_ARGID_DRIVER		, "-d = (un)install only given driver"		},
	{	"l", SQLINST_ARGID_DLL			, "-l = (un)install given dll. (full path and dll name)"	},
	{	"q", SQLINST_ARGID_QUIET		, "-q = quiet"		},
	{	"m", SQLINST_ARGID_SHOW_MSG_BOX	, "-m = show errors in windows messageboxes!"		}
};

// Pointer to versionstring
static const char g_Version[] = { SQLINST_VERSION };

struct ParstArgsData_Struct
{
	// Name of the Arg
	char ArgName[SQLINST_MAX_BUFFER];
	// If Arg known than the id, or -1 if unknown.
	char	ArgID;

	// Arg values
	unsigned char	ArgValueCount;	// Number
	char			ArgValue[SQLINST_MAX_ARG_VALUE][SQLINST_MAX_BUFFER];
};

// What have the (Un)Installer todo?
struct RunningOptionData_Struct
{
	// What have I todo? (SQLINST_ACTION_*)
	unsigned char DoFollow;

	// Which Drivers? (SQLINST_DRIVER_*)
	unsigned char FollowDriver;

	// Install follow dll (Used when: FollowDriver & SQLINST_DRIVER_DLL)
	char DirectDll[SQLINST_MAX_BUFFER];

	// Shut up...
	BOOL Quiet;
};

// Parse the Arg, return number when some found
unsigned char ParseArg(int argc, char* argv[], struct ParstArgsData_Struct *pDestArg, unsigned char MaxDestArg );
// Install or Uninstall (return 0 for success)
int RunUnInstaller( const struct RunningOptionData_Struct *pRunData );
// Seperate Arg and values from type "-d=sql2,sql2u", return starting point from values
char *SeperateArgAndValue( char *pArgName );
// Give back the Arg ID (SQLINST_ARGID_*)
char GetArgID( const char *pArgName );
// Save valuelist in struct
void SaveValues( struct ParstArgsData_Struct *pDestArg, const char *pValues );
// Put Messsage to user
void MessageToUser( const char *pMessage );
// Remove path
void MakeTempFile( char *pDestBuffer, int MaxDestBuffer, const char *pSourceBuffer );

// Error Message Buffer
#define SQLINST_MAX_ERROR_BUFFER	4096
char g_ErrorMessage[SQLINST_MAX_ERROR_BUFFER];
BOOL g_ShowErrorInWindowsMessageBoxes = FALSE;


int main(int argc, char* argv[])
{
	struct ParstArgsData_Struct	Arguments[SQLINST_MAX_ARGS];	// Parst Argument data
	unsigned char ArgCount;	// Number of Arguments
	unsigned char Count, Driver, Value;	// Counter
	struct RunningOptionData_Struct	RunData;	// Run Program
	BOOL Found;	// Found Value (or not)

	// Check if any arguments need to be checked
	ArgCount = ParseArg(argc, argv, Arguments, SQLINST_MAX_ARGS);
	if ( ArgCount )
	{
		// Init RunData
		memset( &RunData, 0, sizeof(struct RunningOptionData_Struct) );
		RunData.DoFollow = SQLINST_ACTION_NOT_KNOWN;

		// Check all Args
		for ( Count = 0; Count < ArgCount; Count++ )
		{
			// Procced ArgID
			switch ( Arguments[Count].ArgID )
			{
				default:
				case SQLINST_ARGID_UNKNOWN:
					_snprintf( g_ErrorMessage, SQLINST_MAX_ERROR_BUFFER, "Unknown argument: %s\n", Arguments[Count].ArgName );
					g_ErrorMessage[SQLINST_MAX_ERROR_BUFFER-1] = 0;
					MessageToUser( g_ErrorMessage );
					// Break with error
					return -1;
				break;

				case SQLINST_ARGID_HELP:
					 // Set print help
					RunData.DoFollow = SQLINST_ACTION_PRINT_HELP;

					// Break: for ( Count = 0; Count < ArgCount; Count++ )
					Count = ArgCount;
				break;

				case SQLINST_ARGID_VERSION:
					// Print Version
					_snprintf( g_ErrorMessage, SQLINST_MAX_ERROR_BUFFER, "SQLiteODBCInstaller %s\n", g_Version );
					g_ErrorMessage[SQLINST_MAX_ERROR_BUFFER-1] = 0;
					MessageToUser( g_ErrorMessage );
					// Return OK
					return 0;
				break;

				case SQLINST_ARGID_INSTALL:
					// Check if RunData.DoFollow already set
					if ( RunData.DoFollow )
					{
						_snprintf( g_ErrorMessage, SQLINST_MAX_ERROR_BUFFER, "Please use only -i OR -u one time and alone!\n" );
						g_ErrorMessage[SQLINST_MAX_ERROR_BUFFER-1] = 0;
						MessageToUser( g_ErrorMessage );
						// Break with error
						return -1;
					} // if ( InstallOption )

					// Set InstallOption to "INSTALL"
					RunData.DoFollow = SQLINST_ACTION_INSTALL;
				break;

				case SQLINST_ARGID_UNINSTALL:
					// Check if RunData.DoFollow already set
					if ( RunData.DoFollow )
					{
						_snprintf( g_ErrorMessage, SQLINST_MAX_ERROR_BUFFER, "Please use only -i OR -u one time and 	alone!\n" );
						g_ErrorMessage[SQLINST_MAX_ERROR_BUFFER-1] = 0;
						MessageToUser( g_ErrorMessage );
						// Break with error
						return -1;
					} // if ( InstallOption )

					// Set InstallOption to "UNINSTALL"
					RunData.DoFollow = SQLINST_ACTION_UNINSTALL;
				break;

				case SQLINST_ARGID_ALL:
					// (Un)Install all Drivers
					for ( Driver = 0; Driver < SQLINST_DRIVER_COUNT; Driver++ )
					{
						// If not special entry
						if ( strlen(g_SQLiteDriverData[Driver].pDLLName) )
						{
							RunData.FollowDriver |= g_SQLiteDriverData[Driver].ArgFlag;
						}
					} // for ( Driver = 0; Driver < SQLINST_DRIVER_COUNT; Driver++ )
				break;

				case SQLINST_ARGID_DRIVER:
					// Any Driver give?
					if ( 0 == Arguments[Count].ArgValueCount )
					{
						_snprintf( g_ErrorMessage, SQLINST_MAX_ERROR_BUFFER, "Argument: \"%s\" need options! Use -h for help!\n", Arguments[Count].ArgName );
						g_ErrorMessage[SQLINST_MAX_ERROR_BUFFER-1] = 0;
						MessageToUser( g_ErrorMessage );
						// Break with error
						return -1;
					} // if ( 0 == Arguments[Count].ArgValueCount )

					// Check if given driver in Database
					for ( Value = 0; Value < Arguments[Count].ArgValueCount; Value++ )
					{
						// Driver not found in Database
						Found =  FALSE;
						for ( Driver = 0; Driver < SQLINST_DRIVER_COUNT; Driver++ )
						{
							// Ignore special entry.
							if ( !strlen(g_SQLiteDriverData[Driver].pDLLName) )
							{
								continue;
							}

							// If Identical set Flag
							if ( 0 == strcmp( g_SQLiteDriverData[Driver].pArgOptionName, Arguments[Count].ArgValue[Value]) )
							{
								RunData.FollowDriver |= g_SQLiteDriverData[Driver].ArgFlag;
								Found = TRUE;
								// Break:  for ( Driver = 0; Driver < SQLINST_DRIVER_COUNT; Driver++ )
								break;
							} // if ( NULL == strcmp( g_SQLiteDriverData[Driver].pArgOptionName, Arguments[Count].ArgValue[Value]) )
						} // for ( Driver = 0; Driver < SQLINST_DRIVER_COUNT; Driver++ )

						// Return with error if unknown drivername given
						if ( !Found )
						{
							_snprintf( g_ErrorMessage, SQLINST_MAX_ERROR_BUFFER, "Argument: %s unknown drivername: %s! Use -h for help!\n", Arguments[Count].ArgName, Arguments[Count].ArgValue[Value] );
							g_ErrorMessage[SQLINST_MAX_ERROR_BUFFER-1] = 0;
							MessageToUser( g_ErrorMessage );
							// Break with error
							return -1;
						} // if ( !Found )
					} // for ( Value = 0; Value < Arguments[Count].ArgValueCount; Value++ )
				break;

				case SQLINST_ARGID_DLL:
					// Need one given DLL
					if ( 1 == Arguments[Count].ArgValueCount )
					{
						// Copy DLL to run struct
						RunData.FollowDriver |= SQLINST_DRIVER_DLL;
						strncpy( RunData.DirectDll, Arguments[Count].ArgValue[0], SQLINST_MAX_BUFFER );
						RunData.DirectDll[SQLINST_MAX_BUFFER-1] = 0;
					}
					else
					{
						_snprintf( g_ErrorMessage, SQLINST_MAX_ERROR_BUFFER, "Argument: %s need dllname! Use -h for help!\n", Arguments[Count].ArgName );
						g_ErrorMessage[SQLINST_MAX_ERROR_BUFFER-1] = 0;
						MessageToUser( g_ErrorMessage );
					}
				break;

				case SQLINST_ARGID_QUIET:
					RunData.Quiet = TRUE;
				break;
				
				case SQLINST_ARGID_SHOW_MSG_BOX:
					g_ShowErrorInWindowsMessageBoxes = TRUE;
				break;
			} // switch ( Arguments[Count].ArgID )
		} // for ( Count = 0; Count < ArgCount; Count++ )

		// Check if any driver given, if not use all (Make argument "-a" default!)
		if ( !RunData.FollowDriver )
		{
			// Install all Drivers
			for ( Driver = 0; Driver < SQLINST_DRIVER_COUNT; Driver++ )
			{
				// If not special entry
				if ( strlen(g_SQLiteDriverData[Driver].pDLLName) )
				{
					RunData.FollowDriver |= g_SQLiteDriverData[Driver].ArgFlag;
				}
			} // for ( Driver = 0; Driver < SQLINST_DRIVER_COUNT; Driver++ )
		} // if ( !RunData.FollowDriver )

		switch ( RunData.DoFollow )
		{
			case SQLINST_ACTION_NOT_KNOWN: // No Argument for Install or Uninstall given
				_snprintf( g_ErrorMessage, SQLINST_MAX_ERROR_BUFFER, "Please give install(-i) or uninstall option(-u)!\n" );
				g_ErrorMessage[SQLINST_MAX_ERROR_BUFFER-1] = 0;
				MessageToUser( g_ErrorMessage );
				// Break with error
				return -1;
			break;

			case SQLINST_ACTION_INSTALL: // Install driver
			case SQLINST_ACTION_UNINSTALL: // Uninstall driver
				// Ok now (un)install
				return RunUnInstaller( &RunData );
			break;

			default:
			case SQLINST_ACTION_PRINT_HELP: // Print Help
				// Do nothing
			break;
		}
	} // if ( ArgCount )

	// Print help
	printf( "SQLiteODBCInstaller [-i or -u] [-a, -d=driverlist or -l=fullpath\\dllname]\n" );
	printf( "Version: %s\n", g_Version );
	printf( "\n");

	// Print all Options
	for ( Count = 0; Count < SQLINST_ARGID_COUNT; Count++ )
	{
		// Print comment
		printf( "%s\n", g_ArgData[Count].pArgComment );

		// If ArgID driver print availble driver
		if ( SQLINST_ARGID_DRIVER == g_ArgData[Count].ArgID )
		{
			// Print all known driver
			printf( "     {" );
			for ( Driver = 0; Driver < SQLINST_DRIVER_COUNT; Driver++ )
			{
				// Ignore special entry.
				if ( !strlen(g_SQLiteDriverData[Driver].pDLLName) )
				{
					continue;
				}

				// Seperator ";" if needed
				if ( Driver ) { printf( "; "); }
				// Data
				printf( "%s=%s", g_SQLiteDriverData[Driver].pArgOptionName, g_SQLiteDriverData[Driver].pDriverName );
			} // for ( Count = 0; Count < SQLINST_DRIVER_COUNT; Count++ )
			printf( "}\n" );
		}
	} // for ( Count = 0; Count < SQLINST_ARGID_COUNT; Count++ )

	// Print some examples
	printf( "\n");
	printf( "Example 1: Install all driver\n");
	printf( "SQLiteODBCInstaller -i -a\n");
	printf( "\n");
	printf( "Example 2: Uninstall all driver\n");
	printf( "SQLiteODBCInstaller -u -a\n");
	printf( "\n");
	printf( "Example 3: Install only SQLite2 and SQLite2 UTF8 driver\n");
	printf( "SQLiteODBCInstaller -i -d=sql2,sql2u\n");
	printf( "\n");
	printf( "Example 4: Install dll directly\n");
	printf( "SQLiteODBCInstaller -i -l=\"c:\\download\\sqlite3odbc.dll\"\n");
	printf( "\n");

	return 0;
}



// Parse the Arg, return TRUE when some found
unsigned char ParseArg(int argc, char* argv[], struct ParstArgsData_Struct *pDestArg, unsigned char MaxDestArg )
{
	unsigned char ArgCount;	// Counters
	int DestArg;	// Count of parst arguments
	char *pValues;	// Pointer to values

	// No args parst yet
	DestArg	= -1;

	// Check all arguments but not me self (ArgCount=0)
	for ( ArgCount = 1; ArgCount < argc; ArgCount++ )
	{
		// Need "-" or more windows like "/" at start of argument
		if ( '-' == argv[ArgCount][0] || '/' == argv[ArgCount][0] )
		{
			// Stop at max arguments
			if ( DestArg+1 > MaxDestArg )
			{
				return DestArg;
			}
			// Use Next argument
			DestArg++;

			// Init
			memset( &pDestArg[DestArg], 0, sizeof( struct ParstArgsData_Struct) );

			// Copy Argument
			strncpy( pDestArg[DestArg].ArgName, &argv[ArgCount][1], SQLINST_MAX_BUFFER );
			pDestArg[DestArg].ArgName[SQLINST_MAX_BUFFER-1] = 0;

			// If from type "-d=sql2,sql2u" break after space or "="
			pValues = SeperateArgAndValue( pDestArg[DestArg].ArgName );

			// Find ArgID
			pDestArg[DestArg].ArgID = GetArgID( pDestArg[DestArg].ArgName );

			// Save values if needed
			if ( pValues )
			{
				// Save valuelist in struct
				SaveValues( &pDestArg[DestArg], pValues );
			}
		} // if ( argv[ArgCount][0] = '-' || argv[ArgCount][0] = '/' )
		else
		{
			// No argument so it should be an value
			// if -d from type "-d = sql2,sql2u" ignore "="
			if ( '=' == argv[ArgCount][0] )
			{
				continue;
			} // if ( '=' == argv[ArgCount][0] )

			// Save valuelist in struct
			if ( -1 < DestArg )
			{
				SaveValues( &pDestArg[DestArg], argv[ArgCount] );
			}
		} // if ( argv[ArgCount][0] = '-' || argv[ArgCount][0] = '/' )

	} // for ( ArgCount = 0; ArgCount < argc; ArgCount++ )

	// Be sure to give only unsigned value back (DestArg starts with -1)
	DestArg++;

	return DestArg;
}

// Give back the Arg ID (SQLINST_ARGID_*)
char GetArgID( const char *pArgName )
{
	unsigned char IdCount;	// Counter

	// Get ArgID
	for ( IdCount = 0; IdCount < SQLINST_ARGID_COUNT; IdCount++ )
	{
		// Check if arg found in database
		if ( 0 == strcmp(g_ArgData[IdCount].pArgName, pArgName) )
		{
			// Save ID
			return g_ArgData[IdCount].ArgID;
		} // if ( NULL == strcmp( g_ArgData[IdCount].pArgName, &argv[ArgCount][1] )
	} // for ( IdCount = 0; IdCount < SQLINST_ARGID_COUNT; IdCount++ )

	// Nothing found...
	return SQLINST_ARGID_UNKNOWN;
}

// Seperate Arg and values from type "-d=sql2,sql2u", return starting point from values
char *SeperateArgAndValue( char *pArgName )
{
	unsigned int StringPos;
	BOOL		 CheckValaue;

	// Parse String
	StringPos	= 0;	// First pos in string
	CheckValaue	= FALSE; // Arg not ended
	while ( pArgName[StringPos] )
	{
		// Check some end chars 
		switch ( pArgName[StringPos] )
		{
			case '\t':
			case '=':
				// Arg-Type?
				if ( !CheckValaue )
				{
					// Arg ends
					pArgName[StringPos] = 0;
					CheckValaue = TRUE;
				} // if ( !CheckValaue )
			break;

			default:
				// Value starts
				if ( CheckValaue )
				{
					return &pArgName[StringPos];
				}
			break;
		}

		StringPos++;
	} // while ( pArgName[StringPos] )

	// No value found
	return NULL;
}

// Save valuelist in struct
void SaveValues( struct ParstArgsData_Struct *pDestArg, const char *pValues )
{
	unsigned int ValueLength;	// Length of string
	unsigned int StringPos;	// Position in string
	BOOL BreakMe; // Break while
	const char *pValueStart;
	unsigned char Quote;

	// Parse from start
	StringPos = 0;

	// do not write above dest buffer stop when string ends
	while ( (pDestArg->ArgValueCount < SQLINST_MAX_ARG_VALUE) && pValues[StringPos] )
	{
		// no value startet 
		ValueLength = 0;
		BreakMe = FALSE;
		pValueStart	= &pValues[StringPos]; 
		Quote = 0;

		// Parse string
		while ( pValues[StringPos] && !BreakMe )
		{
			// If some End chars
			switch ( pValues[StringPos] )
			{
				case ' ':
				case '\n':
				case ',':
					if ( !Quote )
					{
						BreakMe = TRUE;
					}
				break;

				case '"':
					if ( Quote )
					{
						BreakMe = TRUE;
					}
					else
					{
						Quote = 1;
					}
				break;

				default:
					ValueLength++;
				break;
			} // switch ( pValues[StringPos] )

			StringPos++;
		} // while ( pValues[StringPos] )

		// When somethings to save
		if ( ValueLength )
		{
			if ( ValueLength > SQLINST_MAX_BUFFER )
			{
				ValueLength = SQLINST_MAX_BUFFER;
			}

			// Copy value
			strncpy( pDestArg->ArgValue[pDestArg->ArgValueCount], pValueStart, ValueLength );

			// Saved...
			pDestArg->ArgValueCount++;
		} // if ( ValueLength )
	} // while ( pDestArg->ArgValueCount < SQLINST_MAX_ARG_VALUE )
}

// Install or Uninstall
int RunUnInstaller( const struct RunningOptionData_Struct *pRunData )
{
	unsigned int Driver;	// Counter
	char SystemPath[1024];	// System path (C:\Windows\System32)
	char WorkPath[1024];	// My own path
	char TempPath[1024];	// Temp path for working
	char TempFile[1024];	// TempFile
	char SourceFile[2048];	// SourceFile
	char DestFile[2048];	// DestFile
	HMODULE	hDll;			// DLL-Handle
	InstallFunc	Install;	// DLL-Function
	UnInstallFunc UnInstall;	// DLL-Function

	// Get needed Pathes:
	// Systempath
	GetSystemDirectory( SystemPath, 1024 );
	SystemPath[1024-1] = 0;
	// Workpath
	_getcwd( WorkPath, 1024 );
	WorkPath[1024-1] = 0;
	// Temppath
	GetTempPath( 1024, TempPath );
	TempPath[1024-1] = 0;

	// Install uninstall all drivers
	for ( Driver = 0; Driver < SQLINST_DRIVER_COUNT; Driver++ )
	{
		// (Un)Install follow driver
		if ( !(pRunData->FollowDriver & g_SQLiteDriverData[Driver].ArgFlag) )
		{
			// No...
			continue;
		} // if ( (pRunData->FollowDriver & g_SQLiteDriverData[Driver].ArgFlag) )

		// What have I todo?
		switch ( pRunData->DoFollow )
		{
			case SQLINST_ACTION_INSTALL:
				if ( g_SQLiteDriverData[Driver].ArgFlag == SQLINST_DRIVER_DLL )
				{
					_snprintf( SourceFile, 2048, "%s", pRunData->DirectDll );
					SourceFile[2048-1] = 0;
				}
				else
				{
					_snprintf( SourceFile, 2048, "%s\\%s", WorkPath, g_SQLiteDriverData[Driver].pDLLName );
					SourceFile[2048-1] = 0;
				}

				// Try to load the DLL
				hDll = LoadLibrary( SourceFile );
				if ( !hDll )
				{
					_snprintf( g_ErrorMessage, SQLINST_MAX_ERROR_BUFFER, "Could not open DLL: %s\n", SourceFile );
					g_ErrorMessage[SQLINST_MAX_ERROR_BUFFER-1] = 0;
					MessageToUser( g_ErrorMessage );
					continue;
				} // if ( !hDll )

				// Get Address from the Install Function;
				Install = (InstallFunc)GetProcAddress(hDll, "install");
				if ( !Install )
				{
					FreeLibrary(hDll);
					_snprintf( g_ErrorMessage, SQLINST_MAX_ERROR_BUFFER, "Dll not provid install function: %s\n", SourceFile );
					g_ErrorMessage[SQLINST_MAX_ERROR_BUFFER-1] = 0;
					MessageToUser( g_ErrorMessage );
					continue;
				}

				// Use the install function...
				if ( pRunData->Quiet )
				{
					Install( NULL, NULL, "quiet", 0 );
				}
				else
				{
					Install( NULL, NULL, "", 0 );
				}

				// Cleaning...
				FreeLibrary(hDll);
			break;

			case SQLINST_ACTION_UNINSTALL:
				if ( g_SQLiteDriverData[Driver].ArgFlag == SQLINST_DRIVER_DLL )
				{
					MakeTempFile( TempFile, 1024, pRunData->DirectDll );
					_snprintf( SourceFile, 2048, "%s", pRunData->DirectDll );
					_snprintf( DestFile, 2048, "%s\\%s", TempPath, TempFile );
				}
				else
				{
					_snprintf( SourceFile, 2048, "%s\\%s", SystemPath, g_SQLiteDriverData[Driver].pDLLName );
					_snprintf( DestFile, 2048, "%s\\%s", TempPath, g_SQLiteDriverData[Driver].pDLLName );
				}

				SourceFile[2048-1] = 0;
				DestFile[2048-1] = 0;

				// Delete maybe old file before copy
				DeleteFile( DestFile );
				if ( !CopyFile( SourceFile, DestFile, FALSE ) )
				{
					// Could not copy File
					_snprintf( g_ErrorMessage, SQLINST_MAX_ERROR_BUFFER, "Error: Could not copy %s to temppath!\n", SourceFile );
					g_ErrorMessage[SQLINST_MAX_ERROR_BUFFER-1] = 0;
					MessageToUser( g_ErrorMessage );
					continue;
				}

				// Try to load the DLL
				hDll = LoadLibrary( DestFile );
				if ( !hDll )
				{
					// Use the SourceFile path (user should know what source dll is incompatibel)
					_snprintf( g_ErrorMessage, SQLINST_MAX_ERROR_BUFFER, "Could not open DLL: %s\n", SourceFile );
					g_ErrorMessage[SQLINST_MAX_ERROR_BUFFER-1] = 0;
					MessageToUser( g_ErrorMessage );
					continue;
				} // if ( !hDll )

				// Get Address from the Install Function;
				UnInstall = (InstallFunc)GetProcAddress(hDll, "uninstall");
				if ( !UnInstall )
				{
					FreeLibrary(hDll);
					// Use the SourceFile path (user should know what source dll is incompatibel)
					_snprintf( g_ErrorMessage, SQLINST_MAX_ERROR_BUFFER, "Dll not provid install function: %s\n", SourceFile );
					g_ErrorMessage[SQLINST_MAX_ERROR_BUFFER-1] = 0;
					MessageToUser( g_ErrorMessage );
					continue;
				}

				// Use the uninstall function...
				if ( pRunData->Quiet )
				{
					UnInstall( NULL, NULL, "quiet", 0 );
				}
				else
				{
					UnInstall( NULL, NULL, "", 0 );
				}

				// Cleaning...
				FreeLibrary(hDll);
				DeleteFile( DestFile );
			break;

			default:
				_snprintf( g_ErrorMessage, SQLINST_MAX_ERROR_BUFFER, "Internel error: %s: Can not run: %d\n", "RunUnInstaller", pRunData->DoFollow );
				g_ErrorMessage[SQLINST_MAX_ERROR_BUFFER-1] = 0;
				MessageToUser( g_ErrorMessage );
				// Break with error
				return -1;
			break;
		} // switch ( pRunData->DoFollow )

	} // for ( Driver = 0; Driver < SQLINST_DRIVER_COUNT; Driver++ )

	// return with all ok
	return 0;
}

// Put Messsage to user
void MessageToUser( const char *pMessage )
{
	if ( g_ShowErrorInWindowsMessageBoxes )
	{
	    MessageBox( NULL, pMessage, "Message", MB_ICONSTOP|MB_OK|MB_TASKMODAL|MB_SETFOREGROUND );
	}
	else
	{
		printf ( "%s", pMessage );
	} // if ( g_ShowErrorInWindowsMessageBoxes )
}

// Remove path
void MakeTempFile( char *pDestBuffer, int MaxDestBuffer, const char *pSourceBuffer )
{
	int StartPosition;	// Position in String

	// in case of doubt return nothing
	pDestBuffer[0] = 0;

	// Start with last char
	StartPosition	= (int)strlen(pSourceBuffer);
	if ( !StartPosition )
	{
		return;
	}
	StartPosition--; // 0-Index

	// Find filename beginning:
	while ( StartPosition >= 0 )
	{
		if	(	'\\'	== pSourceBuffer[StartPosition]	||
				'/'		== pSourceBuffer[StartPosition]	||
				':'		== pSourceBuffer[StartPosition]
			)
		{
			// Found
			break;
		}
	
		StartPosition--;
	} // while ( SrcPosition >= 0 )
	// if -1 or skip "\" or "/" or ":"
	StartPosition++;

	// Copy Filename (Or nothing: "C:\path\")
	strncpy( pDestBuffer, &pSourceBuffer[StartPosition], MaxDestBuffer );

	// return in evry situation a string
	pDestBuffer[MaxDestBuffer-1] = 0;
}
