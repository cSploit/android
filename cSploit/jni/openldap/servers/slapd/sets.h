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

#ifndef SLAP_SETS_H_
#define SLAP_SETS_H_

#include <ldap_cdefs.h>

LDAP_BEGIN_DECL

typedef struct slap_set_cookie {
	Operation *set_op;
} SetCookie;

/* this routine needs to return the bervals instead of
 * plain strings, since syntax is not known.  It should
 * also return the syntax or some "comparison cookie"
 * that is used by set_filter.
 */
typedef BerVarray (SLAP_SET_GATHER)( SetCookie *cookie,
		struct berval *name, AttributeDescription *ad);

LDAP_SLAPD_F (int) slap_set_filter(
	SLAP_SET_GATHER gatherer,
	SetCookie *cookie, struct berval *filter,
	struct berval *user, struct berval *target, BerVarray *results);

LDAP_SLAPD_F (BerVarray) slap_set_join(SetCookie *cp,
	BerVarray lset, unsigned op, BerVarray rset);

#define SLAP_SET_OPMASK		0x00FF

#define SLAP_SET_REFARR		0x0100
#define SLAP_SET_REFVAL		0x0200
#define SLAP_SET_REF		(SLAP_SET_REFARR|SLAP_SET_REFVAL)

/* The unsigned "op" can be ORed with the flags below;
 * - if the rset's values must not be freed, or must be copied if kept,
 *   it is ORed with SLAP_SET_RREFVAL
 * - if the rset array must not be freed, or must be copied if kept,
 *   it is ORed with SLAP_SET_RREFARR
 * - the same applies to the lset with SLAP_SET_LREFVAL and SLAP_SET_LREFARR
 * - it is assumed that SLAP_SET_REFVAL implies SLAP_SET_REFARR,
 *   i.e. the former is checked only if the latter is set.
 */

#define SLAP_SET_RREFARR	SLAP_SET_REFARR
#define SLAP_SET_RREFVAL	SLAP_SET_REFVAL
#define SLAP_SET_RREF		SLAP_SET_REF
#define SLAP_SET_RREFMASK	0x0F00

#define SLAP_SET_RREF2REF(r)	((r) & SLAP_SET_RREFMASK)
	
#define SLAP_SET_LREFARR	0x1000
#define SLAP_SET_LREFVAL	0x2000
#define SLAP_SET_LREF		(SLAP_SET_LREFARR|SLAP_SET_LREFVAL)
#define SLAP_SET_LREFMASK	0xF000
	
#define SLAP_SET_LREF2REF(r)	(((r) & SLAP_SET_LREFMASK) >> 4)
	
LDAP_END_DECL

#endif
