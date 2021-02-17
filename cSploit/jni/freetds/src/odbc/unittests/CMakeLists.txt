include_directories(..)

set(tests
	t0001 t0002 t0003
	t0004 connect print
	date norowset funccall
	lang_error tables
	binary_test moreandcount
	earlybind putdata params
	raiserror getdata
	transaction type genparams
	preperror prepare_results
	testodbc data error
	rebindpar rpc convert_error
	typeinfo const_params
	insert_speed compute
	timeout array array_out
	cursor1 scroll cursor2
	describecol copydesc
	prepclose warning
	paramcore timeout2 timeout3
	connect2 timeout4 freeclose
	cursor3 cursor4 cursor5
	attributes hidden blob1
	cancel wchar rowset transaction2
	cursor6 cursor7 utf8 utf8_2
	stats descrec peter test64
	prepare_warn long_error mars1
	array_error
)

if(WIN32)
	set(libs odbc32 odbccp32)
else()
	set(libs tdsodbc odbcinst)
endif()

add_library(o_common STATIC common.c common.h c2string.c)

foreach(target ${tests})
	add_executable(o_${target} ${target}.c)
	target_link_libraries(o_${target} o_common replacements ${libs} ${lib_NETWORK} ${lib_BASE})
	add_test(NAME o_${target} WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} COMMAND ${target})
endforeach(target)
