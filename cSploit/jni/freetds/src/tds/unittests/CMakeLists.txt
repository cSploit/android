include_directories(..)

add_library(t_common STATIC common.c common.h utf8.c)

if(NOT WIN32)
	set(unix_TESTS challenge)
endif(NOT WIN32)

foreach(target t0001 t0002 t0003 t0004 t0005 t0006 t0007 t0008 dynamic1
    convert dataread utf8_1 utf8_2 utf8_3 numeric iconv_fread toodynamic
    readconf ${unix_TESTS})
	add_executable(t_${target} ${target}.c)
	target_link_libraries(t_${target} t_common tds replacements ${lib_NETWORK} ${lib_BASE})
endforeach(target)
