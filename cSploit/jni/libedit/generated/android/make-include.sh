#!/bin/bash

echo  -n "Making include directory..."

if test -f "${1}/include/editline/readline.h"; then
	echo "not needed"
	exit 0
fi

mkdir -p "${1}/include/editline"
cp "${1}/src/editline/readline.h" "${1}/include/editline/readline.h"
echo "ok"