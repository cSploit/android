include_directories(.)

include(CheckFunctionExists)

set(FUNCS asprintf vasprintf atoll strtok_r readpassphrase
strlcpy strlcat basename getopt strsep)

set(add_SRCS)

foreach(func ${FUNCS})
	string(TOUPPER "HAVE_${func}" var)
	check_function_exists(${func} ${var})
	if(NOT ${var})
		set(add_SRCS ${add_SRCS} ${func}.c)
	endif()
endforeach(func)

add_library(replacements STATIC
	iconv.c gettimeofday.c fakepoll.c
	getpassarg.c socketpair.c win_mutex.c
	win_cond.c getaddrinfo.c
	${add_SRCS}
)
if (NOT WIN32)
	set_target_properties(replacements PROPERTIES COMPILE_FLAGS -fPIC)
endif()

add_subdirectory(unittests)
