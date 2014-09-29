/* LICENSE
 * 
 */

#ifndef COMMON_EXPORT_H
#define COMMON_EXPORT_H

#ifdef BUILDING_LIBDSPLOITCOMMON
# define LIBCOMMON_EXPORTED __attribute__((__visibility__("default")))
#else
# define LIBCOMMON_EXPORTED
#endif

#endif