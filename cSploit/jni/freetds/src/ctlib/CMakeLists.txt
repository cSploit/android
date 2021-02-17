add_subdirectory(unittests)

if(WIN32)
	set(win_SRCS ../../win32/initnet.c ctlib.def)
endif()

add_library(ct SHARED
	ct.c cs.c blk.c ctutil.c
	${win_SRCS}
)

target_link_libraries(ct tds replacements ${lib_NETWORK} ${lib_BASE})

if(NOT WIN32)
	set_target_properties(ct PROPERTIES SOVERSION "4.0.0")
endif()
