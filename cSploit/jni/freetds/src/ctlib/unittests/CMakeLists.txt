add_library(c_common STATIC common.c common.h)

foreach(target t0001 t0002 t0003 t0004
	t0005 t0006 t0007 t0008
	t0009 connect_fail ct_options
	lang_ct_param array_bind cs_diag
	get_send_data rpc_ct_param rpc_ct_setparam
	ct_diagclient ct_diagserver ct_diagall
	cs_config cancel blk_in
	blk_out ct_cursor ct_cursors
	ct_dynamic blk_in2 datafmt)
	add_executable(c_${target} ${target}.c)
	target_link_libraries(c_${target} c_common ct replacements ${lib_NETWORK} ${lib_BASE})
endforeach(target)
