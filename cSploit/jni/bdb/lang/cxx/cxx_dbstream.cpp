/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2013, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"

#include "db_cxx.h"
#include "dbinc/cxx_int.h"

#include "dbinc/db_page.h"
#include "dbinc/db_am.h"
#include "dbinc/blob.h"
#include "dbinc/fop.h"
#include "dbinc/mp.h"

// Helper macro for simple methods that pass through to the
// underlying C method. It may return an error or raise an exception.
// Note this macro expects that input _argspec is an argument
// list element (e.g., "char *arg") and that _arglist is the arguments
// that should be passed through to the C method (e.g., "(db, arg)")
//
#define	DBSTREAM_METHOD(_name, _argspec, _arglist)			\
int DbStream::_name _argspec						\
{									\
	int ret;							\
	DB_STREAM *dbs = this;						\
									\
	ret = dbs->_name _arglist;					\
	if (!DB_RETOK_STD(ret))						\
		DB_ERROR(DbEnv::get_DbEnv(dbs->dbc->dbenv),		\
		    "Dbstream::" # _name, ret, ON_ERROR_UNKNOWN);	\
	return (ret);							\
}

// It's private, and should never be called, but VC4.0 needs it resolved
//
DbStream::~DbStream()
{
}

DBSTREAM_METHOD(close, (u_int32_t flags), (dbs, flags));
DBSTREAM_METHOD(read,
    (Dbt *data, db_off_t offset, u_int32_t size, u_int32_t flags),
    (dbs, data, offset, size, flags));
DBSTREAM_METHOD(size, (db_off_t *size, u_int32_t flags),
    (dbs, size, flags));
DBSTREAM_METHOD(write, (Dbt *data, db_off_t offset, u_int32_t flags),
    (dbs, data, offset, flags));

