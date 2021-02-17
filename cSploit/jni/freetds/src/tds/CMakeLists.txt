
add_subdirectory(unittests)

find_program(PERL perl)

#if(PERL)
#	add_custom_command(OUTPUT tds_willconvert.h
#		COMMAND ${PERL} tds_willconvert.pl > tds_willconvert.h
#		MAIN_DEPENDENCY tds_willconvert.pl
#		WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})
#	add_custom_command(OUTPUT encodings.h
#		COMMAND ${PERL} encodings.pl . > encodings.h 2> /dev/null
#		MAIN_DEPENDENCY encodings.pl
#		DEPENDS alternative_character_sets.h
#		WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})
#	add_custom_command(OUTPUT num_limits.h
#		COMMAND ${PERL} num_limits.pl > num_limits.h
#		MAIN_DEPENDENCY num_limits.pl
#		WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})
#	add_custom_command(OUTPUT tds_types.h
#		COMMAND ${PERL} types.pl < ../../misc/types.csv > tds_types.h
#		MAIN_DEPENDENCY types.pl
#		DEPENDS ../../misc/types.csv
#		WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})
#endif(PERL)

if(WIN32)
	set(add_SRCS sspi.c)
else(WIN32)
	set(add_SRCS challenge.c md4.c md5.c des.c gssapi.c hmac_md5.c)
endif(WIN32)

add_library(tds STATIC
	mem.c token.c util.c login.c read.c
        write.c convert.c numeric.c config.c query.c iconv.c
        locale.c threadsafe.c vstrbuild.c
        tdsstring.c getmac.c data.c net.c
        tds_checks.c tds_checks.h log.c
        bulk.c packet.c stream.c
	tds_willconvert.h encodings.h num_limits.h tds_types.h
	${add_SRCS}
)
if (NOT WIN32)
	set_target_properties(tds PROPERTIES COMPILE_FLAGS -fPIC)
endif()
