/*
 *
 *     The contents of this file are subject to the Initial
 *     Developer's Public License Version 1.0 (the "License");
 *     you may not use this file except in compliance with the
 *     License. You may obtain a copy of the License at
 *     http://www.ibphoenix.com/idpl.html.
 *
 *     Software distributed under the License is distributed on
 *     an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either
 *     express or implied.  See the License for the specific
 *     language governing rights and limitations under the License.
 *
 *
 *  The Original Code was created by Claudio Valderrama C. for IBPhoenix.
 *  The development of the Original Code was sponsored by Craig Leonardi.
 *
 *  Copyright (c) 2001 IBPhoenix
 *  All Rights Reserved.
 */


// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__F60BDFDF_7A2D_11D5_8EEB_4854E8274D24__INCLUDED_)
#define AFX_STDAFX_H__F60BDFDF_7A2D_11D5_8EEB_4854E8274D24__INCLUDED_


// TODO: reference additional headers your program requires here


#include "firebird.h"

#ifdef HAVE_MATH_H
#include <math.h>
#endif

//#include <stdlib.h>

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#ifdef HAVE_SYS_TIMEB_H
# include <sys/timeb.h>
#endif

#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif

//#include "ib_util.h"
//#include "ib_udf.h"


#ifdef HAVE_PTHREAD_H
#include "fb_pthread.h"
#endif

#include "../../jrd/ibase.h"

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__F60BDFDF_7A2D_11D5_8EEB_4854E8274D24__INCLUDED_)
