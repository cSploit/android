/**************************************************************************
** File: listuser.c
**
** Desc:
**       Demonstrates the use of NWDSSearch to list specified attributes
**       of all users in a tree.
**
** Usage:
**       load listuser
**
** Synopsis:
**       - Logs into NDS
**       - allocates the necessary buffers
**       - searches the entire tree for all users
**       - lists all user objects as well as their Given and Full Names
**
** Modules auto-loaded:
**       CLIB
**       DSAPI
**
*/

#define  MAX_VALUE_LEN	255	// max length of attribute values (arbitrary)
#define  PWD_CR		0x0D	// Newline character
#define	 PWD_BS		0x08	// Backspace character
#define  TRUE		1
#define  FALSE		0
#ifndef UNICODE
	#define UNICODE
#endif
#include <stdlib.h>
#include <stdio.h>
#include <ncp/nwnet.h>

/*************************************************************************
** main
*/
int main (int argc, char** argv) {
	NWDSContextHandle	context;
	NWDSCCODE ccode;
	nint32	iterationHandle;
	nint32	countObjectsSearched;
	nuint32	objCntr,attrCntr,valCntr;
	nuint32	objCount;			
	nuint32	attrCount;			
	char 	admin[MAX_DN_CHARS+1];
	char	password[MAX_VALUE_LEN+1];
	char	objectName[MAX_DN_CHARS+1];
	char	attrName[MAX_SCHEMA_NAME_CHARS+1];
	nuint32	attrValCount;
	nuint32	syntaxID;
	nuint32	attrValSize;
	char*	attrVal;
	nint32	screenCntr=0;
	nuint32 contextFlags;
	
	// buffers
	pBuf_T	searchFilter=NULL;	// search filter
	pBuf_T	retBuf=NULL;		// result buffer for NWDSSearch
	pBuf_T	attrNames=NULL;		// specify which attribute values to return
	Filter_Cursor_T* cur=NULL;	// search expression tree temporary buffer
	Object_Info_T	objectInfo;
		
	/*******************************************************************
	create context and set current context to Root
	 *******************************************************************/
	 
	ccode = NWDSCreateContextHandle(&context);
	
	if((long)context == ERR_CONTEXT_CREATION) {
		 printf("Error creating context.\n");
		 goto Exit1;
	}

	ccode=NWDSSetContext(context, DCK_NAME_CONTEXT, "[Root]");
	if(ccode){
		 printf("Error NWDSSetContext(): %d\n",ccode);
		 goto Exit2;
	}


	ccode= NWDSGetContext(context, DCK_FLAGS, &contextFlags);
	if( ccode != 0) 
	{
		printf("NWDSGetContext (DCK_FLAGS) failed, returns %d\n", ccode);
	}

	contextFlags|= DCV_TYPELESS_NAMES;

/*****************************************************
 In order to see the problem, first compile, and run with the line below
 commented out. The program will display a list of users on the NDS.
 Now, un-comment the line belog, compile, and re-run the NLM, and the 
 NWDSSearch function fails with error code -601
 *****************************************************/
 
/*	contextFlags &= ~DCV_XLATE_STRINGS;  */
	
	ccode= NWDSSetContext( context, DCK_FLAGS, &contextFlags);
	if( ccode != 0) 
	{
		printf("NWDSSetContext (DCK_FLAGS DCV_TYPELESS_NAMES) failed, returns %d\n", ccode);
	}


	/*******************************************************************
	Login to NDS
	*/
	//ccode = NWDSLoginAsServer( context );
	{
		NWCONN_HANDLE conn;

//		ccode = ncp_open_mount("/root/fdnet", &conn);
		ccode = NWCCOpenConnByName(NULL, "FDNET", NWCC_NAME_FORMAT_BIND, 0, 0, &conn);

		if (!ccode)
			NWDSAddConnection(context, conn);
	}
	
	if(ccode){
		switch (ccode) {
			case -601:
				// user object not found
				printf("User object not found: %s\n",admin);
				break;
			case -669:
				// auth failed
				printf("Failed authentication.  Check Password\n");
				break;
			default:
				 printf("Error: NWDSLogin failed (%s) for %s\n",strnwerror(ccode),admin);
		}
		goto Exit2;
	}


	/*******************************************************************
	In order to search, we need:
		A Filter Cursor (to build the search expression)
		A Filter Buffer (to store the expression; used by NWDSSearch)
		A Buffer to store which attributes we need information on
		A Result Buffer (to store the search results)
	*/

	/*******************************************************************
	** Allocate Filter buffer and Cursor and populate
	*/
	ccode = NWDSAllocBuf(DEFAULT_MESSAGE_LEN,&searchFilter);
	if (ccode < 0) {
		printf("NWDSAllocBuf returned: %d\n", ccode);
		goto Exit3;
	}

	// Initialize the searchFilter buffer
	ccode = NWDSInitBuf(context,DSV_SEARCH_FILTER,searchFilter);
	if (ccode < 0) {
		printf("NWDSInitBuf returned: %d\n", ccode);
		goto Exit4;
	}

	// Allocate a filter cursor to put the search expression 
	ccode = NWDSAllocFilter(&cur);
	if (ccode < 0) {
		printf("NWDSAllocFilter returned: %d\n", ccode);
		goto Exit4;
	}

	// Build the expression tree in cur, then place into searchFilter 
	// Object Class = User
	ccode = NWDSAddFilterToken(cur,FTOK_ANAME,"Object Class",SYN_CLASS_NAME);
	if (ccode < 0) {
			printf("NWDSAddFilterToken returned: %d\n", ccode);
			goto Exit4;
	}

	ccode = NWDSAddFilterToken(cur,FTOK_EQ,NULL,0);
	if (ccode < 0) {
			printf("NWDSAddFilterToken returned: %d\n", ccode);
			goto Exit4;
	}

	ccode = NWDSAddFilterToken(cur,FTOK_AVAL,"User",SYN_CLASS_NAME);
	if (ccode < 0) {
			printf("NWDSAddFilterToken returned: %d\n", ccode);
			goto Exit4;
	}

	// now place the cursor into the searchFilter buffer 
	// NWDSPutFilter frees the expression tree filter (cur)
	//	so if it succeeds, set cur to NULL so it won't be
	//	freed below 
	ccode = NWDSPutFilter(context,searchFilter,cur,NULL);
	if (ccode < 0) {
			printf("NWDSPutFilter returned: %d\n", ccode);
			goto Exit4;
	}
	else
		cur=NULL;

	/*******************************************************************
	** Specify which attributes we want information on
	*/
	// allocate and initialize the attrNames buffer 
	ccode = NWDSAllocBuf(DEFAULT_MESSAGE_LEN,&attrNames);
	if (ccode < 0) {
			printf("NWDSAllocBuf returned: %d\n", ccode);
			goto Exit4;
	}
	ccode = NWDSInitBuf(context,DSV_SEARCH,attrNames);
	if (ccode < 0) {
			printf("NWDSInitBuf returned: %d\n", ccode);
			goto Exit4;
	}

	// specify the attributes to retrieve for the objects
	// In this case: Given Name and Full Name
	ccode = NWDSPutAttrName(context,attrNames,"Given Name");
	if (ccode < 0) {
			printf("NWDSPutAttrName returned: %d\n", ccode);
			goto Exit4;
	}
	ccode = NWDSPutAttrName(context,attrNames,"Full Name");
	if (ccode < 0) {
			printf("NWDSPutAttrName returned: %d\n", ccode);
			goto Exit4;
	}
	ccode = NWDSPutAttrName(context,attrNames,"Surname");
	if (ccode < 0) {
			printf("NWDSPutAttrName returned: %d\n", ccode);
			goto Exit4;
	}
	ccode = NWDSPutAttrName(context,attrNames,"Title");
	if (ccode < 0) {
			printf("NWDSPutAttrName returned: %d\n", ccode);
			goto Exit4;
	}

	/*******************************************************************
	** Allocate a result buffer
	*/
	ccode = NWDSAllocBuf(65500,&retBuf);
	if (ccode < 0) {
			printf("NWDSAllocBuf returned: %d\n", ccode);
			goto Exit4;
	}

	/*******************************************************************
	** We are now ready to do the search
	*/
	iterationHandle = NO_MORE_ITERATIONS;
	// while NWDSSearch still can get some objects...
	do {
		ccode = NWDSSearch(context,
				"[Root]",
				DS_SEARCH_SUBTREE,
				FALSE,		// don't dereference aliases
				searchFilter,
				TRUE,		// we want attributes and values
				FALSE,		// only want information in attrNames
				attrNames,
				&iterationHandle,
				0,		// reserved
				&countObjectsSearched,
				retBuf);
		if (ccode) {
			printf("NWDSSearch returned: %s\n", strnwerror(ccode));
			goto Exit4;
		}

		// count the object returned in the buffer
		ccode = NWDSGetObjectCount(context,retBuf,&objCount);
		if (ccode) {
			printf("NWDSGetObjectCount returned: %d\n", ccode);
			goto Exit4;
		}

		printf("Objects found: %d\n",objCount);
		printf("-----------------------------------------\n");
		screenCntr+=2;

		// for the number of objects returned...
		for (objCntr=0;objCntr<objCount;objCntr++) {
			// get an object name
			ccode = NWDSGetObjectName(context,retBuf,objectName,&attrCount,&objectInfo);
			if (ccode) {
				printf("NWDSGetObjectName returned: %d\n", ccode);
				goto Exit4;
			}
			printf("%s\n",objectName);
			screenCntr++;

			// for the number of attributes returned with the object...
			for (attrCntr=0;attrCntr<attrCount;attrCntr++) {
				// get the attribute name
				ccode = NWDSGetAttrName(context,retBuf,attrName,&attrValCount,&syntaxID);
				if (ccode) {
					printf("NWDSGetAttrName returned: %d\n", ccode);
					goto Exit4;
				}
				// compute and allocate space for the value
				ccode = NWDSComputeAttrValSize(context,retBuf,syntaxID,&attrValSize);
				if (ccode) {
					printf("NWDSComputeAttrValSize returned: %d\n", ccode);
					goto Exit4;
				}
				attrVal = malloc(attrValSize);
				// for the number of values returned with the attribute...
				for (valCntr=0;valCntr<attrValCount;valCntr++) {
					// get the value
					ccode = NWDSGetAttrVal(context,retBuf,syntaxID,attrVal);
					if (ccode) {
						printf("NWDSGetAttrVal returned: %d\n", ccode);
						goto Exit4;
					}
					printf("  %s =\t%s\n",attrName,attrVal);
					screenCntr++;
				} // valCntr
				free(attrVal);
			} // attrCntr
			printf("\n");
		} // objCntr
	} while ((nuint32)iterationHandle != NO_MORE_ITERATIONS);


Exit4:
	if (retBuf)
		NWDSFreeBuf(retBuf);
	if (cur)
		NWDSFreeFilter(cur, NULL);
	if (searchFilter)
		NWDSFreeBuf(searchFilter);
	if (attrNames)
		NWDSFreeBuf(attrNames);
Exit3:
//	NWDSLogout(context);
Exit2:
	NWDSFreeContext(context);
Exit1:
	exit(0);
}
