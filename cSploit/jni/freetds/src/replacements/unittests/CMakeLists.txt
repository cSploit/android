foreach(target mutex1 condition)
	add_executable(${target} ${target}.c)
	target_link_libraries(${target} replacements ${lib_BASE})
endforeach(target)
