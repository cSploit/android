add_subdirectory(unittests)

if(WIN32)
	set(win_SRCS ../../win32/initnet.c ../../win32/dblib.def dbopen.c)
endif()

# TODO add dbopen.c if necessary
add_library(sybdb SHARED
	dblib.c dbutil.c rpc.c bcp.c xact.c dbpivot.c buffering.h
	${win_SRCS}
)
target_link_libraries(sybdb tds replacements ${lib_NETWORK} ${lib_BASE})

if(NOT WIN32)
	set_target_properties(sybdb PROPERTIES SOVERSION "5.0.0")
endif()
