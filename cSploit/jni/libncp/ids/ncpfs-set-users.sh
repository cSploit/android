#!/bin/sh

nwbols -a | tr '[A-Z]' '[a-z]' | while read nwid type name ; do
	echo $nwid `id 2>/dev/null -u $name || id -u nobody`
done | ./ncpfs-nwuid-test

