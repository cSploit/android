// $OpenLDAP$
/*
 * Copyright 2000-2014 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */

#ifndef DEBUG_H
#define DEBUG_H
#include <iostream>
#include "config.h"
#define LDAP_DEBUG_NONE         0x0000
#define LDAP_DEBUG_TRACE        0x0001
#define LDAP_DEBUG_CONSTRUCT    0x0002
#define LDAP_DEBUG_DESTROY      0x0004
#define LDAP_DEBUG_PARAMETER    0x0008
#define LDAP_DEBUG_ANY          0xffff 

#define DEBUGLEVEL LDAP_DEBUG_ANY

#define PRINT_FILE	\
	std::cerr << "file: " __FILE__  << " line: " << __LINE__ 

#ifdef WITH_DEBUG
#define DEBUG(level, arg)       \
    if((level) & DEBUGLEVEL){     \
        std::cerr  << arg ;          \
    } 
#else
#undef DEBUG
#define DEBUG(level,arg)
#endif //WITH_DEBUG

#endif // DEBUG_H
