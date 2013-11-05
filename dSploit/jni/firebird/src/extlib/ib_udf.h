/*
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

#if defined(__cplusplus) && defined(DARWIN)
#include "../jrd/common.h"
#define EXPORT API_ROUTINE
#else
#define EXPORT
#endif 
#define MATHERR	matherr

#include <stdarg.h>

#define IB_E             2.7182818284590452354
#define IB_PI            3.14159265358979323846
#define IB_PI_2          1.57079632679489661923
#define IB_PI_4          0.78539816339744830962
#define IB_1_PI          0.31830988618379067154
#define IB_2_PI          0.63661977236758134308
#define IB_2_SQRTPI      1.12837916709551257390
#define IB_SQRT2         1.41421356237309504880
#define IB_SQRT1_2       0.70710678118654752440
