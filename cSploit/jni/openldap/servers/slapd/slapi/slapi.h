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
 *   Luke Howard
 */

#ifdef LDAP_SLAPI /* SLAPI is OPTIONAL */

#ifndef _SLAPI_H
#define _SLAPI_H

LDAP_BEGIN_DECL

/*
 * Quick 'n' dirty to make struct slapi_* in slapi-plugin.h opaque
 */
#define slapi_entry	Entry
#define slapi_attr	Attribute
#define slapi_value	berval
#define slapi_valueset	berval *
#define slapi_filter	Filter

LDAP_END_DECL

#include <slapi-plugin.h>

LDAP_BEGIN_DECL

#define SLAPI_OVERLAY_NAME			"slapi"

#define SLAPI_OPERATION_PBLOCK(_op)		((_op)->o_callback->sc_private)
#define SLAPI_BACKEND_PBLOCK(_be)		((_be)->be_pb)

#define SLAPI_OPERATION_EXTENSIONS(_op)		((_op)->o_hdr->oh_extensions)
#define SLAPI_CONNECTION_EXTENSIONS(_conn)	((_conn)->c_extensions)

#define SLAPI_CONTROL_MANAGEDSAIT_OID		LDAP_CONTROL_MANAGEDSAIT
#define SLAPI_CONTROL_SORTEDSEARCH_OID		LDAP_CONTROL_SORTREQUEST
#define SLAPI_CONTROL_PAGED_RESULTS_OID		LDAP_CONTROL_PAGEDRESULTS

typedef int (*SLAPI_FUNC)( Slapi_PBlock *pb );

typedef struct _slapi_control {
        int			s_ctrl_num;
        char			**s_ctrl_oids;
        unsigned long		*s_ctrl_ops;
} Slapi_Control;

typedef struct _ExtendedOp {
	struct berval		ext_oid;
        SLAPI_FUNC		ext_func;
        Backend			*ext_be;
        struct _ExtendedOp	*ext_next;
} ExtendedOp;

/* Computed attribute support */
struct _computed_attr_context {
	Slapi_PBlock	*cac_pb;
	Operation	*cac_op;
	void		*cac_private;
};

/* for slapi_attr_type_cmp() */
#define SLAPI_TYPE_CMP_EXACT	0
#define SLAPI_TYPE_CMP_BASE	1
#define SLAPI_TYPE_CMP_SUBTYPE	2

typedef enum slapi_extension_e {
	SLAPI_X_EXT_CONNECTION = 0,
	SLAPI_X_EXT_OPERATION = 1,
	SLAPI_X_EXT_MAX = 2
} slapi_extension_t;

struct slapi_dn {
	unsigned char flag;
	struct berval dn;
	struct berval ndn;
};

struct slapi_rdn {
	unsigned char flag;
	struct berval bv;
	LDAPRDN rdn;
};

/*
 * Was: slapi_pblock.h
 */

#ifndef NO_PBLOCK_CLASS		/* where's this test from? */

typedef enum slapi_pblock_class_e {
	PBLOCK_CLASS_INVALID = 0,
	PBLOCK_CLASS_INTEGER,
	PBLOCK_CLASS_LONG_INTEGER,
	PBLOCK_CLASS_POINTER,
	PBLOCK_CLASS_FUNCTION_POINTER
} slapi_pblock_class_t;

#define PBLOCK_SUCCESS			(0)
#define PBLOCK_ERROR			(-1)
#define PBLOCK_MAX_PARAMS		100

union slapi_pblock_value {
	int pv_integer;
	long pv_long_integer;
	void *pv_pointer;
	int (*pv_function_pointer)();
};

struct slapi_pblock {
	ldap_pvt_thread_mutex_t	pb_mutex;
	int			pb_nParams;
	int			pb_params[PBLOCK_MAX_PARAMS];
	union slapi_pblock_value pb_values[PBLOCK_MAX_PARAMS];
	/* native types */
	Connection		*pb_conn;
	Operation		*pb_op;
	SlapReply		*pb_rs;
	int			pb_intop;
	char			pb_textbuf[ SLAP_TEXT_BUFLEN ];
};

#endif /* !NO_PBLOCK_CLASS */

/*
 * Was: plugin.h
 */

#define SLAPI_PLUGIN_IS_POST_FN(x) ((x) >= SLAPI_PLUGIN_POST_BIND_FN && (x) <= SLAPI_PLUGIN_BE_POST_DELETE_FN)

#define SLAPI_IBM_PBLOCK			-3

#define	SLAPI_ENTRY_PRE_OP			52
#define	SLAPI_ENTRY_POST_OP			53

/* This is the spelling in the SunOne 5.2 docs */
#define	SLAPI_RES_CONTROLS	SLAPI_RESCONTROLS

#define SLAPI_ABANDON_MSGID			120

#define SLAPI_OPERATION_PARAMETERS		138

#define SLAPI_SEQ_TYPE				150
#define SLAPI_SEQ_ATTRNAME			151
#define SLAPI_SEQ_VAL				152

#define SLAPI_MR_FILTER_ENTRY			170	
#define SLAPI_MR_FILTER_TYPE			171
#define SLAPI_MR_FILTER_VALUE			172
#define SLAPI_MR_FILTER_OID			173
#define SLAPI_MR_FILTER_DNATTRS			174

#define SLAPI_LDIF2DB_FILE			180
#define SLAPI_LDIF2DB_REMOVEDUPVALS		185

#define SLAPI_DB2LDIF_PRINTKEY			183

#define	SLAPI_CHANGENUMBER			197
#define	SLAPI_LOG_OPERATION			198

#define SLAPI_DBSIZE				199

#define	SLAPI_PLUGIN_DB_TEST_FN			227
#define SLAPI_PLUGIN_DB_NO_ACL        		250

/* OpenLDAP private parametrs */
#define SLAPI_PLUGIN_COMPUTE_EVALUATOR_FN	1200
#define SLAPI_PLUGIN_COMPUTE_SEARCH_REWRITER_FN	1201

#define SLAPI_X_CONFIG_ARGV			1400
#define SLAPI_X_INTOP_FLAGS			1401
#define SLAPI_X_INTOP_RESULT_CALLBACK		1402
#define SLAPI_X_INTOP_SEARCH_ENTRY_CALLBACK	1403
#define SLAPI_X_INTOP_REFERRAL_ENTRY_CALLBACK	1404
#define SLAPI_X_INTOP_CALLBACK_DATA		1405
#define SLAPI_X_OLD_RESCONTROLS			1406

LDAP_SLAPI_V (ldap_pvt_thread_mutex_t)	slapi_hn_mutex;
LDAP_SLAPI_V (ldap_pvt_thread_mutex_t)	slapi_time_mutex;
LDAP_SLAPI_V (ldap_pvt_thread_mutex_t)	slapi_printmessage_mutex; 
LDAP_SLAPI_V (char *)			slapi_log_file;
LDAP_SLAPI_V (int)			slapi_log_level;

#include "proto-slapi.h"

#endif /* _SLAPI_H */
#endif /* LDAP_SLAPI */
