/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2002-2014 The OpenLDAP Foundation.
 * Portions Copyright 1997,2002-2003 IBM Corporation.
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
/* ACKNOWLEDGEMENTS:
 * This work was initially developed by IBM Corporation for use in
 * IBM products and subsequently ported to OpenLDAP Software by
 * Steve Omrani.  Additional significant contributors include:
 *    Luke Howard
 */

#include "portable.h"
#include "ldap_pvt_thread.h"
#include "slap.h"
#include "config.h"
#include "slapi.h"
#include "lutil.h"

/*
 * Note: if ltdl.h is not available, slapi should not be compiled
 */
#include <ltdl.h>

static int slapi_int_load_plugin( Slapi_PBlock *, const char *, const char *, int, 
	SLAPI_FUNC *, lt_dlhandle * );

/* pointer to link list of extended objects */
static ExtendedOp *pGExtendedOps = NULL;

/*********************************************************************
 * Function Name:      plugin_pblock_new
 *
 * Description:        This routine creates a new Slapi_PBlock structure,
 *                     loads in the plugin module and executes the init
 *                     function provided by the module.
 *
 * Input:              type - type of the plugin, such as SASL, database, etc.
 *                     path - the loadpath to load the module in
 *                     initfunc - name of the plugin function to execute first
 *                     argc - number of arguements
 *                     argv[] - an array of char pointers point to
 *                              the arguments passed in via
 *                              the configuration file.
 *
 * Output:             
 *
 * Return Values:      a pointer to a newly created Slapi_PBlock structrue or
 *                     NULL - function failed 
 *
 * Messages:           None
 *********************************************************************/

static Slapi_PBlock *
plugin_pblock_new(
	int type, 
	int argc, 
	char *argv[] ) 
{
	Slapi_PBlock	*pPlugin = NULL; 
	Slapi_PluginDesc *pPluginDesc = NULL;
	lt_dlhandle	hdLoadHandle;
	int		rc;
	char		**av2 = NULL, **ppPluginArgv;
	char		*path = argv[2];
	char		*initfunc = argv[3];

	pPlugin = slapi_pblock_new();
	if ( pPlugin == NULL ) {
		rc = LDAP_NO_MEMORY;
		goto done;
	}

	slapi_pblock_set( pPlugin, SLAPI_PLUGIN_TYPE, (void *)&type );
	slapi_pblock_set( pPlugin, SLAPI_PLUGIN_ARGC, (void *)&argc );

	av2 = ldap_charray_dup( argv );
	if ( av2 == NULL ) {
		rc = LDAP_NO_MEMORY;
		goto done;
	}

	if ( argc > 0 ) {
		ppPluginArgv = &av2[4];
	} else {
		ppPluginArgv = NULL;
	}

	slapi_pblock_set( pPlugin, SLAPI_PLUGIN_ARGV, (void *)ppPluginArgv );
	slapi_pblock_set( pPlugin, SLAPI_X_CONFIG_ARGV, (void *)av2 );

	rc = slapi_int_load_plugin( pPlugin, path, initfunc, 1, NULL, &hdLoadHandle );
	if ( rc != 0 ) {
		goto done;
	}

	if ( slapi_pblock_get( pPlugin, SLAPI_PLUGIN_DESCRIPTION, (void **)&pPluginDesc ) == 0 &&
	     pPluginDesc != NULL ) {
		slapi_log_error(SLAPI_LOG_TRACE, "plugin_pblock_new",
				"Registered plugin %s %s [%s] (%s)\n",
				pPluginDesc->spd_id,
				pPluginDesc->spd_version,
				pPluginDesc->spd_vendor,
				pPluginDesc->spd_description);
	}

done:
	if ( rc != 0 && pPlugin != NULL ) {
		slapi_pblock_destroy( pPlugin );
		pPlugin = NULL;
		if ( av2 != NULL ) {
			ldap_charray_free( av2 );
		}
	}

	return pPlugin;
} 

/*********************************************************************
 * Function Name:      slapi_int_register_plugin
 *
 * Description:        insert the slapi_pblock structure to the end of the plugin
 *                     list 
 *
 * Input:              a pointer to a plugin slapi_pblock structure to be added to 
 *                     the list
 *
 * Output:             none
 *
 * Return Values:      LDAP_SUCCESS - successfully inserted.
 *                     LDAP_LOCAL_ERROR.
 *
 * Messages:           None
 *********************************************************************/
int 
slapi_int_register_plugin(
	Backend *be, 
	Slapi_PBlock *pPB )
{ 
	Slapi_PBlock	*pTmpPB;
	Slapi_PBlock	*pSavePB;
	int   		 rc = LDAP_SUCCESS;

	assert( be != NULL );

	pTmpPB = SLAPI_BACKEND_PBLOCK( be );
	if ( pTmpPB == NULL ) {
		SLAPI_BACKEND_PBLOCK( be ) = pPB;
	} else {
		while ( pTmpPB != NULL && rc == LDAP_SUCCESS ) {
			pSavePB = pTmpPB;
			rc = slapi_pblock_get( pTmpPB, SLAPI_IBM_PBLOCK, &pTmpPB );
		}

		if ( rc == LDAP_SUCCESS ) { 
			rc = slapi_pblock_set( pSavePB, SLAPI_IBM_PBLOCK, (void *)pPB ); 
		}
	}
     
	return ( rc != LDAP_SUCCESS ) ? LDAP_OTHER : LDAP_SUCCESS;
}
       
/*********************************************************************
 * Function Name:      slapi_int_get_plugins
 *
 * Description:        get the desired type of function pointers defined 
 *                     in all the plugins 
 *
 * Input:              the type of the functions to get, such as pre-operation,etc.
 *
 * Output:             none
 *
 * Return Values:      this routine returns a pointer to an array of function
 *                     pointers containing backend-specific plugin functions
 *                     followed by global plugin functions
 *
 * Messages:           None
 *********************************************************************/
int 
slapi_int_get_plugins(
	Backend *be, 		
	int functype, 
	SLAPI_FUNC **ppFuncPtrs )
{
 
	Slapi_PBlock	*pCurrentPB; 
	SLAPI_FUNC	FuncPtr;
	SLAPI_FUNC	*pTmpFuncPtr;
	int		numPB = 0;
	int		rc = LDAP_SUCCESS;

	assert( ppFuncPtrs != NULL );

	if ( be == NULL ) {
		goto done;
	}

	pCurrentPB = SLAPI_BACKEND_PBLOCK( be );

	while ( pCurrentPB != NULL && rc == LDAP_SUCCESS ) {
		rc = slapi_pblock_get( pCurrentPB, functype, &FuncPtr );
		if ( rc == LDAP_SUCCESS ) {
			if ( FuncPtr != NULL )  {
				numPB++;
			}
			rc = slapi_pblock_get( pCurrentPB,
				SLAPI_IBM_PBLOCK, &pCurrentPB );
		}
	}

	if ( numPB == 0 ) {
		*ppFuncPtrs = NULL;
		rc = LDAP_SUCCESS;
		goto done;
	}

	/*
	 * Now, build the function pointer array of backend-specific
	 * plugins followed by global plugins.
	 */
	*ppFuncPtrs = pTmpFuncPtr = 
		(SLAPI_FUNC *)ch_malloc( ( numPB + 1 ) * sizeof(SLAPI_FUNC) ); 
	if ( ppFuncPtrs == NULL ) {
		rc = LDAP_NO_MEMORY;
		goto done;
	}

	pCurrentPB = SLAPI_BACKEND_PBLOCK( be );

	while ( pCurrentPB != NULL && rc == LDAP_SUCCESS )  {
		rc = slapi_pblock_get( pCurrentPB, functype, &FuncPtr );
		if ( rc == LDAP_SUCCESS ) {
			if ( FuncPtr != NULL )  {
				*pTmpFuncPtr = FuncPtr;
				pTmpFuncPtr++;
			} 
			rc = slapi_pblock_get( pCurrentPB,
					SLAPI_IBM_PBLOCK, &pCurrentPB );
		}
	}

	*pTmpFuncPtr = NULL;


done:
	if ( rc != LDAP_SUCCESS && *ppFuncPtrs != NULL ) {
		ch_free( *ppFuncPtrs );
		*ppFuncPtrs = NULL;
	}

	return rc;
}

/*********************************************************************
 * Function Name:      createExtendedOp
 *
 * Description: Creates an extended operation structure and
 *              initializes the fields
 *
 * Return value: A newly allocated structure or NULL
 ********************************************************************/
ExtendedOp *
createExtendedOp()
{
	ExtendedOp *ret;

	ret = (ExtendedOp *)slapi_ch_malloc(sizeof(ExtendedOp));
	ret->ext_oid.bv_val = NULL;
	ret->ext_oid.bv_len = 0;
	ret->ext_func = NULL;
	ret->ext_be = NULL;
	ret->ext_next = NULL;

	return ret;
}


/*********************************************************************
 * Function Name:      slapi_int_unregister_extop
 *
 * Description:        This routine removes the ExtendedOp structures 
 *					   asscoiated with a particular extended operation 
 *					   plugin.
 *
 * Input:              pBE - pointer to a backend structure
 *                     opList - pointer to a linked list of extended
 *                              operation structures
 *                     pPB - pointer to a slapi parameter block
 *
 * Output:
 *
 * Return Value:       none
 *
 * Messages:           None
 *********************************************************************/
void
slapi_int_unregister_extop(
	Backend *pBE, 
	ExtendedOp **opList, 
	Slapi_PBlock *pPB )
{
	ExtendedOp	*pTmpExtOp, *backExtOp;
	char		**pTmpOIDs;
	int		i;

#if 0
	assert( pBE != NULL); /* unused */
#endif /* 0 */
	assert( opList != NULL );
	assert( pPB != NULL );

	if ( *opList == NULL ) {
		return;
	}

	slapi_pblock_get( pPB, SLAPI_PLUGIN_EXT_OP_OIDLIST, &pTmpOIDs );
	if ( pTmpOIDs == NULL ) {
		return;
	}

	for ( i = 0; pTmpOIDs[i] != NULL; i++ ) {
		backExtOp = NULL;
		pTmpExtOp = *opList;
		for ( ; pTmpExtOp != NULL; pTmpExtOp = pTmpExtOp->ext_next) {
			int	rc;
			rc = strcasecmp( pTmpExtOp->ext_oid.bv_val,
					pTmpOIDs[ i ] );
			if ( rc == 0 ) {
				if ( backExtOp == NULL ) {
					*opList = pTmpExtOp->ext_next;
				} else {
					backExtOp->ext_next
						= pTmpExtOp->ext_next;
				}

				ch_free( pTmpExtOp );
				break;
			}
			backExtOp = pTmpExtOp;
		}
	}
}


/*********************************************************************
 * Function Name:      slapi_int_register_extop
 *
 * Description:        This routine creates a new ExtendedOp structure, loads
 *                     in the extended op module and put the extended op function address
 *                     in the structure. The function will not be executed in
 *                     this routine.
 *
 * Input:              pBE - pointer to a backend structure
 *                     opList - pointer to a linked list of extended
 *                              operation structures
 *                     pPB - pointer to a slapi parameter block
 *
 * Output:
 *
 * Return Value:       an LDAP return code
 *
 * Messages:           None
 *********************************************************************/
int 
slapi_int_register_extop(
	Backend *pBE, 	
	ExtendedOp **opList, 
	Slapi_PBlock *pPB )
{
	ExtendedOp	*pTmpExtOp = NULL;
	SLAPI_FUNC	tmpFunc;
	char		**pTmpOIDs;
	int		rc = LDAP_OTHER;
	int		i;

	if ( (*opList) == NULL ) { 
		*opList = createExtendedOp();
		if ( (*opList) == NULL ) {
			rc = LDAP_NO_MEMORY;
			goto error_return;
		}
		pTmpExtOp = *opList;
		
	} else {                        /* Find the end of the list */
		for ( pTmpExtOp = *opList; pTmpExtOp->ext_next != NULL;
				pTmpExtOp = pTmpExtOp->ext_next )
			; /* EMPTY */
		pTmpExtOp->ext_next = createExtendedOp();
		if ( pTmpExtOp->ext_next == NULL ) {
			rc = LDAP_NO_MEMORY;
			goto error_return;
		}
		pTmpExtOp = pTmpExtOp->ext_next;
	}

	rc = slapi_pblock_get( pPB,SLAPI_PLUGIN_EXT_OP_OIDLIST, &pTmpOIDs );
	if ( rc != 0 ) {
		rc = LDAP_OTHER;
		goto error_return;
	}

	rc = slapi_pblock_get(pPB,SLAPI_PLUGIN_EXT_OP_FN, &tmpFunc);
	if ( rc != 0 ) {
		rc = LDAP_OTHER;
		goto error_return;
	}

	if ( (pTmpOIDs == NULL) || (tmpFunc == NULL) ) {
		rc = LDAP_OTHER;
		goto error_return;
	}

	for ( i = 0; pTmpOIDs[i] != NULL; i++ ) {
		pTmpExtOp->ext_oid.bv_val = pTmpOIDs[i];
		pTmpExtOp->ext_oid.bv_len = strlen( pTmpOIDs[i] );
		pTmpExtOp->ext_func = tmpFunc;
		pTmpExtOp->ext_be = pBE;
		if ( pTmpOIDs[i + 1] != NULL ) {
			pTmpExtOp->ext_next = createExtendedOp();
			if ( pTmpExtOp->ext_next == NULL ) {
				rc = LDAP_NO_MEMORY;
				break;
			}
			pTmpExtOp = pTmpExtOp->ext_next;
		}
	}

error_return:
	return rc;
}

/*********************************************************************
 * Function Name:      slapi_int_get_extop_plugin
 *
 * Description:        This routine gets the function address for a given function
 *                     name.
 *
 * Input:
 *                     funcName - name of the extended op function, ie. an OID.
 *
 * Output:             pFuncAddr - the function address of the requested function name.
 *
 * Return Values:      a pointer to a newly created ExtendOp structrue or
 *                     NULL - function failed
 *
 * Messages:           None
 *********************************************************************/
int 
slapi_int_get_extop_plugin(
	struct berval *reqoid, 		
	SLAPI_FUNC *pFuncAddr ) 
{
	ExtendedOp	*pTmpExtOp;

	assert( reqoid != NULL );
	assert( pFuncAddr != NULL );

	*pFuncAddr = NULL;

	if ( pGExtendedOps == NULL ) {
		return LDAP_OTHER;
	}

	pTmpExtOp = pGExtendedOps;
	while ( pTmpExtOp != NULL ) {
		int	rc;
		
		rc = strcasecmp( reqoid->bv_val, pTmpExtOp->ext_oid.bv_val );
		if ( rc == 0 ) {
			*pFuncAddr = pTmpExtOp->ext_func;
			break;
		}
		pTmpExtOp = pTmpExtOp->ext_next;
	}

	return ( *pFuncAddr == NULL ? 1 : 0 );
}

/***************************************************************************
 * This function is similar to slapi_int_get_extop_plugin above. except it returns one OID
 * per call. It is called from root_dse_info (root_dse.c).
 * The function is a modified version of get_supported_extop (file extended.c).
 ***************************************************************************/
struct berval *
slapi_int_get_supported_extop( int index )
{
        ExtendedOp	*ext;

        for ( ext = pGExtendedOps ; ext != NULL && --index >= 0;
			ext = ext->ext_next) {
                ; /* empty */
        }

        if ( ext == NULL ) {
		return NULL;
	}

        return &ext->ext_oid ;
}

/*********************************************************************
 * Function Name:      slapi_int_load_plugin
 *
 * Description:        This routine loads the specified DLL, gets and executes the init function
 *                     if requested.
 *
 * Input:
 *                     pPlugin - a pointer to a Slapi_PBlock struct which will be passed to
 *                               the DLL init function.
 *                     path - path name of the DLL to be load.
 *                     initfunc - either the DLL initialization function or an OID of the
 *                                loaded extended operation.
 *                     doInit - if it is TRUE, execute the init function, otherwise, save the
 *                              function address but not execute it.
 *
 * Output:             pInitFunc - the function address of the loaded function. This param
 *                                 should be not be null if doInit is FALSE.
 *                     pLdHandle - handle returned by lt_dlopen()
 *
 * Return Values:      LDAP_SUCCESS, LDAP_LOCAL_ERROR
 *
 * Messages:           None
 *********************************************************************/

static int 
slapi_int_load_plugin(
	Slapi_PBlock	*pPlugin,
	const char	*path,
	const char	*initfunc, 
	int		doInit,
	SLAPI_FUNC	*pInitFunc,
	lt_dlhandle	*pLdHandle ) 
{
	int		rc = LDAP_SUCCESS;
	SLAPI_FUNC	fpInitFunc = NULL;

	assert( pLdHandle != NULL );

	if ( lt_dlinit() ) {
		return LDAP_LOCAL_ERROR;
	}

	/* load in the module */
	*pLdHandle = lt_dlopen( path );
	if ( *pLdHandle == NULL ) {
		fprintf( stderr, "failed to load plugin %s: %s\n",
			 path, lt_dlerror() );
		return LDAP_LOCAL_ERROR;
	}

	fpInitFunc = (SLAPI_FUNC)lt_dlsym( *pLdHandle, initfunc );
	if ( fpInitFunc == NULL ) {
		fprintf( stderr, "failed to find symbol %s in plugin %s: %s\n",
			 initfunc, path, lt_dlerror() );
		lt_dlclose( *pLdHandle );
		return LDAP_LOCAL_ERROR;
	}

	if ( doInit ) {
		rc = ( *fpInitFunc )( pPlugin );
		if ( rc != LDAP_SUCCESS ) {
			lt_dlclose( *pLdHandle );
		}

	} else {
		*pInitFunc = fpInitFunc;
	}

	return rc;
}

/*
 * Special support for computed attribute plugins
 */
int 
slapi_int_call_plugins(
	Backend		*be, 	
	int		funcType, 
	Slapi_PBlock	*pPB )
{

	int rc = 0;
	SLAPI_FUNC *pGetPlugin = NULL, *tmpPlugin = NULL; 

	if ( pPB == NULL ) {
		return 1;
	}

	rc = slapi_int_get_plugins( be, funcType, &tmpPlugin );
	if ( rc != LDAP_SUCCESS || tmpPlugin == NULL ) {
		/* Nothing to do, front-end should ignore. */
		return rc;
	}

	for ( pGetPlugin = tmpPlugin ; *pGetPlugin != NULL; pGetPlugin++ ) {
		rc = (*pGetPlugin)(pPB);

		/*
		 * Only non-postoperation plugins abort processing on
		 * failure (confirmed with SLAPI specification).
		 */
		if ( !SLAPI_PLUGIN_IS_POST_FN( funcType ) && rc != 0 ) {
			/*
			 * Plugins generally return negative error codes
			 * to indicate failure, although in the case of
			 * bind plugins they may return SLAPI_BIND_xxx
			 */
			break;
		}
	}

	slapi_ch_free( (void **)&tmpPlugin );

	return rc;
}

int
slapi_int_read_config(
	Backend		*be, 		
	const char	*fname, 
	int		lineno, 
	int		argc, 
	char		**argv )
{
	int		iType = -1;
	int		numPluginArgc = 0;

	if ( argc < 4 ) {
		fprintf( stderr,
			"%s: line %d: missing arguments "
			"in \"plugin <plugin_type> <lib_path> "
			"<init_function> [<arguments>]\" line\n",
			fname, lineno );
		return 1;
	}

	/* automatically instantiate overlay if necessary */
	if ( !slapi_over_is_inst( be ) ) {
		ConfigReply cr = { 0 };
		if ( slapi_over_config( be, &cr ) != 0 ) {
			fprintf( stderr, "Failed to instantiate SLAPI overlay: "
				"err=%d msg=\"%s\"\n", cr.err, cr.msg );
			return -1;
		}
	}
	
	if ( strcasecmp( argv[1], "preoperation" ) == 0 ) {
		iType = SLAPI_PLUGIN_PREOPERATION;
	} else if ( strcasecmp( argv[1], "postoperation" ) == 0 ) {
		iType = SLAPI_PLUGIN_POSTOPERATION;
	} else if ( strcasecmp( argv[1], "extendedop" ) == 0 ) {
		iType = SLAPI_PLUGIN_EXTENDEDOP;
	} else if ( strcasecmp( argv[1], "object" ) == 0 ) {
		iType = SLAPI_PLUGIN_OBJECT;
	} else {
		fprintf( stderr, "%s: line %d: invalid plugin type \"%s\".\n",
				fname, lineno, argv[1] );
		return 1;
	}
	
	numPluginArgc = argc - 4;

	if ( iType == SLAPI_PLUGIN_PREOPERATION ||
		  	iType == SLAPI_PLUGIN_EXTENDEDOP ||
			iType == SLAPI_PLUGIN_POSTOPERATION ||
			iType == SLAPI_PLUGIN_OBJECT ) {
		int rc;
		Slapi_PBlock *pPlugin;

		pPlugin = plugin_pblock_new( iType, numPluginArgc, argv );
		if (pPlugin == NULL) {
			return 1;
		}

		if (iType == SLAPI_PLUGIN_EXTENDEDOP) {
			rc = slapi_int_register_extop(be, &pGExtendedOps, pPlugin);
			if ( rc != LDAP_SUCCESS ) {
				slapi_pblock_destroy( pPlugin );
				return 1;
			}
		}

		rc = slapi_int_register_plugin( be, pPlugin );
		if ( rc != LDAP_SUCCESS ) {
			if ( iType == SLAPI_PLUGIN_EXTENDEDOP ) {
				slapi_int_unregister_extop( be, &pGExtendedOps, pPlugin );
			}
			slapi_pblock_destroy( pPlugin );
			return 1;
		}
	}

	return 0;
}

void
slapi_int_plugin_unparse(
	Backend *be,
	BerVarray *out
)
{
	Slapi_PBlock *pp;
	int i, j;
	char **argv, ibuf[32], *ptr;
	struct berval idx, bv;

	*out = NULL;
	idx.bv_val = ibuf;
	i = 0;

	for ( pp = SLAPI_BACKEND_PBLOCK( be );
	      pp != NULL;
	      slapi_pblock_get( pp, SLAPI_IBM_PBLOCK, &pp ) )
	{
		slapi_pblock_get( pp, SLAPI_X_CONFIG_ARGV, &argv );
		if ( argv == NULL ) /* could be dynamic plugin */
			continue;
		idx.bv_len = snprintf( idx.bv_val, sizeof( ibuf ), "{%d}", i );
		if ( idx.bv_len >= sizeof( ibuf ) ) {
			/* FIXME: just truncating by now */
			idx.bv_len = sizeof( ibuf ) - 1;
		}
		bv.bv_len = idx.bv_len;
		for (j=1; argv[j]; j++) {
			bv.bv_len += strlen(argv[j]);
			if ( j ) bv.bv_len++;
		}
		bv.bv_val = ch_malloc( bv.bv_len + 1 );
		ptr = lutil_strcopy( bv.bv_val, ibuf );
		for (j=1; argv[j]; j++) {
			if ( j ) *ptr++ = ' ';
			ptr = lutil_strcopy( ptr, argv[j] );
		}
		ber_bvarray_add( out, &bv );
	}
}

