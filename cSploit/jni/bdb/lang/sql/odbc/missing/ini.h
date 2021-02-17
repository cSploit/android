#ifndef _INI_H
#define _INI_H

/*
 * Missing in most unixODBC installations but
 * needed by <odbcinstext.h>
 */

#define INI_MAX_LINE           1000
#define INI_MAX_PROPERTY_NAME  INI_MAX_LINE
#define INI_MAX_PROPERTY_VALUE INI_MAX_LINE

typedef void *HINI;
#endif
