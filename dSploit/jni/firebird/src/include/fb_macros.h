#ifndef INCLUDE_FB_MACROS_H
#define INCLUDE_FB_MACROS_H

#define FB_COMPILER_MESSAGE_STR(x) #x
#define FB_COMPILER_MESSAGE_STR2(x)   FB_COMPILER_MESSAGE_STR(x)
#define FB_COMPILER_MESSAGE(desc) message(__FILE__ "("	\
									FB_COMPILER_MESSAGE_STR2(__LINE__) "):" desc)

#endif
