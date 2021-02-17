use ExtUtils::MakeMaker;

sub MY::postamble {
	return <<'EOF'
ncpfs.int: ncpfs.i
	set -e; sed -e 's/\\$$/PlAcElFhErE \\/' < ncpfs.i | cpp -E -C -P | sed -e "`echo -e 's/PlAcElFhErE /\\\\\\n/g'`" > ncpfs.int

ncpfs_wrap.c: ncpfs.int
	swig -perl5 -v -xslike -shadow -nocleanup -I../../../include ncpfs.int

EOF
}

WriteMakefile(
	NAME => 'ncpfs',
	LIBS => ['-lncp'],
	INC => '-I../../../include',
	OBJECT => 'ncpfs_wrap.o',
	DEFINE => '-W -Wall -Wno-unused -D_GNU_SOURCE',
	clean => { FILES => 'ncpfs.int ncpfs.pm ncpfs_wrap.c' },
	dynamic_lib => { OTHERLDFLAGS => '-L../../../lib' },
);

