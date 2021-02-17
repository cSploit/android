find_program(PERL perl)

add_subdirectory(unittests)

# TODO checks
add_definitions(-DUNIXODBC -DHAVE_SQLGETPRIVATEPROFILESTRING -DDLL_EXPORT)

#if(PERL)
#	add_custom_command(OUTPUT odbc_export.h
#		COMMAND ${PERL} odbc_export.pl odbc.c > odbc_export.h
#		MAIN_DEPENDENCY odbc_export.pl
#		DEPENDS odbc.c
#		WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})
#	add_custom_command(OUTPUT error_export.h
#		COMMAND ${PERL} odbc_export.pl error.c > error_export.h
#		MAIN_DEPENDENCY odbc_export.pl
#		DEPENDS error.c
#		WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})
#endif(PERL)

if(WIN32)
	set(win_SRCS ../../win32/winlogin.c ../../win32/winsetup.c ../../win32/initnet.c ../../win32/FreeTDS_w.def ../../win32/setup.rc)
	set(libs odbccp32)
else()
	set(libs)
endif()

add_library(tdsodbc SHARED
	odbc.c connectparams.c convert_tds2sql.c
	descriptor.c prepare_query.c odbc_util.c
	native.c sql2tds.c error.c odbc_checks.c sqlwchar.c sqlwparams.h
	odbc_export.h error_export.h ${win_SRCS}
)
# TODO libiconv, odbcinstlib and other dynamics
target_link_libraries(tdsodbc tds replacements ${libs} ${lib_NETWORK} ${lib_BASE})

if(MINGW OR CYGWIN)
	set_target_properties(tdsodbc PROPERTIES LINK_FLAGS "-Wl,--kill-at -Wl,--enable-stdcall-fixup")
endif(MINGW OR CYGWIN)
