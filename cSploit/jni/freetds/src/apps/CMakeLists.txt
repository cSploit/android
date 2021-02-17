set(libs ${lib_NETWORK} ${lib_BASE})

foreach(target freebcp bsqldb defncopy datacopy)
	add_executable(${target} ${target}.c)
	target_link_libraries(${target} sybdb replacements ${libs})
endforeach(target)


add_executable(tsql tsql.c)
target_link_libraries(tsql tds replacements ${libs})

if(WIN32)
	set(libs odbc32 ${lib_NETWORK} ${lib_BASE})
endif(WIN32)

add_executable(bsqlodbc bsqlodbc.c)
target_link_libraries(bsqlodbc tdsodbc replacements ${libs})
