add_library(d_common STATIC common.c common.h)

foreach(target t0001 t0002 t0003 t0004 t0005 t0006 t0007 t0008 t0009
	t0011 t0012 t0013 t0014 t0015 t0016 t0017 t0018 t0019 t0020
	t0021 t0022 t0023 rpc dbmorecmds bcp thread text_buffer
	done_handling timeout hang null null2 setnull numeric)
	add_executable(d_${target} ${target}.c)
	target_link_libraries(d_${target} d_common sybdb replacements ${lib_NETWORK} ${lib_BASE})
endforeach(target)
