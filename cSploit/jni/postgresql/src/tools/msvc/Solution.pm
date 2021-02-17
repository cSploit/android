package Solution;

#
# Package that encapsulates a Visual C++ solution file generation
#
# src/tools/msvc/Solution.pm
#
use Carp;
use strict;
use warnings;

sub new
{
    my $junk = shift;
    my $options = shift;
    my $self = {
        projects => {},
        options  => $options,
        numver   => '',
        strver   => '',
        vcver    => undef,
        platform => undef,
    };
    bless $self;

    # integer_datetimes is now the default
    $options->{integer_datetimes} = 1
      unless exists $options->{integer_datetimes};
    $options->{float4byval} = 1
      unless exists $options->{float4byval};
    if ($options->{xml})
    {
        if (!($options->{xslt} && $options->{iconv}))
        {
            die "XML requires both XSLT and ICONV\n";
        }
    }
    $options->{blocksize} = 8
      unless $options->{blocksize}; # undef or 0 means default
    die "Bad blocksize $options->{blocksize}"
      unless grep {$_ == $options->{blocksize}} (1,2,4,8,16,32);
    $options->{segsize} = 1
      unless $options->{segsize}; # undef or 0 means default
    # only allow segsize 1 for now, as we can't do large files yet in windows
    die "Bad segsize $options->{segsize}"
      unless $options->{segsize} == 1;
    $options->{wal_blocksize} = 8
      unless $options->{wal_blocksize}; # undef or 0 means default
    die "Bad wal_blocksize $options->{wal_blocksize}"
      unless grep {$_ == $options->{wal_blocksize}} (1,2,4,8,16,32,64);
    $options->{wal_segsize} = 16
      unless $options->{wal_segsize}; # undef or 0 means default
    die "Bad wal_segsize $options->{wal_segsize}"
      unless grep {$_ == $options->{wal_segsize}} (1,2,4,8,16,32,64);

    $self->DetermineToolVersions();

    return $self;
}

sub DetermineToolVersions
{
    my $self = shift;

    # Determine version of vcbuild command, to set proper verison of visual studio
    open(P,"vcbuild /? |") || die "vcbuild command not found";
    my $line = <P>;
    close(P);
    if ($line !~ /^Microsoft\s*\(R\) Visual C\+\+ [^-]+ - \D+(\d+)\.00\.\d+/)
    {
        die "Unable to determine vcbuild version from first line of output!";
    }
    if ($1 == 8) { $self->{vcver} = '8.00' }
    elsif ($1 == 9) { $self->{vcver} = '9.00' }
    else { die "Unsupported version of Visual Studio: $1" }
    print "Detected Visual Studio version $self->{vcver}\n";

    # Determine if we are in 32 or 64-bit mode. Do this by seeing if CL has
    # 64-bit only parameters.
    $self->{platform} = 'Win32';
    open(P,"cl /? 2>NUL|") || die "cl command not found";
    while (<P>)
    {
        if (/^\/favor:</)
        {
            $self->{platform} = 'x64';
            last;
        }
    }
    close(P);
    print "Detected hardware platform: $self->{platform}\n";
}

# Return 1 if $oldfile is newer than $newfile, or if $newfile doesn't exist.
# Special case - if config.pl has changed, always return 1
sub IsNewer
{
    my ($newfile, $oldfile) = @_;
    if ($oldfile ne 'src\tools\msvc\config.pl' && $oldfile ne 'src\tools\msvc\config_default.pl')
    {
        return 1
          if (-f 'src\tools\msvc\config.pl') && IsNewer($newfile, 'src\tools\msvc\config.pl');
        return 1
          if (-f 'src\tools\msvc\config_default.pl')
          && IsNewer($newfile, 'src\tools\msvc\config_default.pl');
    }
    return 1 if (!(-e $newfile));
    my @nstat = stat($newfile);
    my @ostat = stat($oldfile);
    return 1 if ($nstat[9] < $ostat[9]);
    return 0;
}

# Copy a file, *not* preserving date. Only works for text files.
sub copyFile
{
    my ($src, $dest) = @_;
    open(I,$src) || croak "Could not open $src";
    open(O,">$dest") || croak "Could not open $dest";
    while (<I>)
    {
        print O;
    }
    close(I);
    close(O);
}

sub GenerateFiles
{
    my $self = shift;
    my $bits = $self->{platform} eq 'Win32' ? 32 : 64;

    # Parse configure.in to get version numbers
    open(C,"configure.in") || confess("Could not open configure.in for reading\n");
    while (<C>)
    {
        if (/^AC_INIT\(\[PostgreSQL\], \[([^\]]+)\]/)
        {
            $self->{strver} = $1;
            if ($self->{strver} !~ /^(\d+)\.(\d+)(?:\.(\d+))?/)
            {
                confess "Bad format of version: $self->{strver}\n";
            }
            $self->{numver} = sprintf("%d%02d%02d", $1, $2, $3?$3:0);
            $self->{majorver} = sprintf("%d.%d", $1, $2);
        }
    }
    close(C);
    confess "Unable to parse configure.in for all variables!"
      if ($self->{strver} eq '' || $self->{numver} eq '');

    if (IsNewer("src\\include\\pg_config_os.h","src\\include\\port\\win32.h"))
    {
        print "Copying pg_config_os.h...\n";
        copyFile("src\\include\\port\\win32.h","src\\include\\pg_config_os.h");
    }

    if (IsNewer("src\\include\\pg_config.h","src\\include\\pg_config.h.win32"))
    {
        print "Generating pg_config.h...\n";
        open(I,"src\\include\\pg_config.h.win32") || confess "Could not open pg_config.h.win32\n";
        open(O,">src\\include\\pg_config.h") || confess "Could not write to pg_config.h\n";
        while (<I>)
        {
            s{PG_VERSION "[^"]+"}{PG_VERSION "$self->{strver}"};
            s{PG_VERSION_NUM \d+}{PG_VERSION_NUM $self->{numver}};
s{PG_VERSION_STR "[^"]+"}{__STRINGIFY(x) #x\n#define __STRINGIFY2(z) __STRINGIFY(z)\n#define PG_VERSION_STR "PostgreSQL $self->{strver}, compiled by Visual C++ build " __STRINGIFY2(_MSC_VER) ", $bits-bit"};
            print O;
        }
        print O "#define PG_MAJORVERSION \"$self->{majorver}\"\n";
        print O "#define LOCALEDIR \"/share/locale\"\n" if ($self->{options}->{nls});
        print O "/* defines added by config steps */\n";
        print O "#ifndef IGNORE_CONFIGURED_SETTINGS\n";
        print O "#define USE_ASSERT_CHECKING 1\n" if ($self->{options}->{asserts});
        print O "#define USE_INTEGER_DATETIMES 1\n" if ($self->{options}->{integer_datetimes});
        print O "#define USE_LDAP 1\n" if ($self->{options}->{ldap});
        print O "#define HAVE_LIBZ 1\n" if ($self->{options}->{zlib});
        print O "#define USE_SSL 1\n" if ($self->{options}->{openssl});
        print O "#define ENABLE_NLS 1\n" if ($self->{options}->{nls});

        print O "#define BLCKSZ ",1024 * $self->{options}->{blocksize},"\n";
        print O "#define RELSEG_SIZE ",
          (1024 / $self->{options}->{blocksize}) *$self->{options}->{segsize} * 1024, "\n";
        print O "#define XLOG_BLCKSZ ",1024 * $self->{options}->{wal_blocksize},"\n";
        print O "#define XLOG_SEG_SIZE (",$self->{options}->{wal_segsize}," * 1024 * 1024)\n";

        if ($self->{options}->{float4byval})
        {
            print O "#define USE_FLOAT4_BYVAL 1\n";
            print O "#define FLOAT4PASSBYVAL true\n";
        }
        else
        {
            print O "#define FLOAT4PASSBYVAL false\n";
        }
        if ($self->{options}->{float8byval})
        {
            print O "#define USE_FLOAT8_BYVAL 1\n";
            print O "#define FLOAT8PASSBYVAL true\n";
        }
        else
        {
            print O "#define FLOAT8PASSBYVAL false\n";
        }

        if ($self->{options}->{uuid})
        {
            print O "#define HAVE_UUID_H\n";
        }
        if ($self->{options}->{xml})
        {
            print O "#define HAVE_LIBXML2\n";
            print O "#define USE_LIBXML\n";
        }
        if ($self->{options}->{xslt})
        {
            print O "#define HAVE_LIBXSLT\n";
            print O "#define USE_LIBXSLT\n";
        }
        if ($self->{options}->{krb5})
        {
            print O "#define KRB5 1\n";
            print O "#define HAVE_KRB5_ERROR_TEXT_DATA 1\n";
            print O "#define HAVE_KRB5_TICKET_ENC_PART2 1\n";
            print O "#define HAVE_KRB5_FREE_UNPARSED_NAME 1\n";
            print O "#define ENABLE_GSS 1\n";
        }
        if (my $port = $self->{options}->{"--with-pgport"})
        {
            print O "#undef DEF_PGPORT\n";
            print O "#undef DEF_PGPORT_STR\n";
            print O "#define DEF_PGPORT $port\n";
            print O "#define DEF_PGPORT_STR \"$port\"\n";
        }
        print O "#define VAL_CONFIGURE \"" . $self->GetFakeConfigure() . "\"\n";
        print O "#endif /* IGNORE_CONFIGURED_SETTINGS */\n";
        close(O);
        close(I);
    }

    $self->GenerateDefFile("src\\interfaces\\libpq\\libpqdll.def",
        "src\\interfaces\\libpq\\exports.txt","LIBPQ");
    $self->GenerateDefFile(
        "src\\interfaces\\ecpg\\ecpglib\\ecpglib.def",
        "src\\interfaces\\ecpg\\ecpglib\\exports.txt",
        "LIBECPG"
    );
    $self->GenerateDefFile(
        "src\\interfaces\\ecpg\\compatlib\\compatlib.def",
        "src\\interfaces\\ecpg\\compatlib\\exports.txt",
        "LIBECPG_COMPAT"
    );
    $self->GenerateDefFile(
        "src\\interfaces\\ecpg\\pgtypeslib\\pgtypeslib.def",
        "src\\interfaces\\ecpg\\pgtypeslib\\exports.txt",
        "LIBPGTYPES"
    );

    if (IsNewer('src\backend\utils\fmgrtab.c','src\include\catalog\pg_proc.h'))
    {
        print "Generating fmgrtab.c and fmgroids.h...\n";
        chdir('src\backend\utils');
        system("perl -I ../catalog Gen_fmgrtab.pl ../../../src/include/catalog/pg_proc.h");
        chdir('..\..\..');
        copyFile('src\backend\utils\fmgroids.h','src\include\utils\fmgroids.h');
    }

    if (IsNewer('src\include\utils\probes.h','src\backend\utils\probes.d'))
    {
        print "Generating probes.h...\n";
        system(
'psed -f src\backend\utils\Gen_dummy_probes.sed src\backend\utils\probes.d > src\include\utils\probes.h'
        );
    }

    if ($self->{options}->{python} && IsNewer('src\pl\plpython\spiexceptions.h','src\include\backend\errcodes.txt'))
    {
        print "Generating spiexceptions.h...\n";
        system('perl src\pl\plpython\generate-spiexceptions.pl src\backend\utils\errcodes.txt > src\pl\plpython\spiexceptions.h');
    }

    if (IsNewer('src\include\utils\errcodes.h','src\backend\utils\errcodes.txt'))
    {
        print "Generating errcodes.h...\n";
        system('perl src\backend\utils\generate-errcodes.pl src\backend\utils\errcodes.txt > src\backend\utils\errcodes.h');
        copyFile('src\backend\utils\errcodes.h','src\include\utils\errcodes.h');
    }

    if (IsNewer('src\pl\plpgsql\src\plerrcodes.h','src\backend\utils\errcodes.txt'))
    {
        print "Generating plerrcodes.h...\n";
        system('perl src\pl\plpgsql\src\generate-plerrcodes.pl src\backend\utils\errcodes.txt > src\pl\plpgsql\src\plerrcodes.h');
    }

    if (IsNewer('src\interfaces\libpq\libpq.rc','src\interfaces\libpq\libpq.rc.in'))
    {
        print "Generating libpq.rc...\n";
        my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = localtime(time);
        my $d = ($year - 100) . "$yday";
        open(I,'<', 'src\interfaces\libpq\libpq.rc.in') || confess "Could not open libpq.rc.in";
        open(O,'>', 'src\interfaces\libpq\libpq.rc') || confess "Could not open libpq.rc";
        while (<I>)
        {
            s/(VERSION.*),0/$1,$d/;
            print O;
        }
        close(I);
        close(O);
    }

    if (IsNewer('src\bin\psql\sql_help.h','src\bin\psql\create_help.pl'))
    {
        print "Generating sql_help.h...\n";
        chdir('src\bin\psql');
        system("perl create_help.pl ../../../doc/src/sgml/ref sql_help");
        chdir('..\..\..');
    }

    if (IsNewer('src\interfaces\ecpg\preproc\preproc.y','src\backend\parser\gram.y'))
    {
        print "Generating preproc.y...\n";
        chdir('src\interfaces\ecpg\preproc');
        system('perl parse.pl < ..\..\..\backend\parser\gram.y > preproc.y');
        chdir('..\..\..\..');
    }

    if (
        IsNewer(
            'src\interfaces\ecpg\include\ecpg_config.h',
            'src\interfaces\ecpg\include\ecpg_config.h.in'
        )
      )
    {
        print "Generating ecpg_config.h...\n";
        open(O,'>','src\interfaces\ecpg\include\ecpg_config.h')
          || confess "Could not open ecpg_config.h";
        print O <<EOF;
#if (_MSC_VER > 1200)
#define HAVE_LONG_LONG_INT_64
#define ENABLE_THREAD_SAFETY 1
EOF
        print O "#define USE_INTEGER_DATETIMES 1\n" if ($self->{options}->{integer_datetimes});
        print O "#endif\n";
        close(O);
    }

    unless (-f "src\\port\\pg_config_paths.h")
    {
        print "Generating pg_config_paths.h...\n";
        open(O,'>', 'src\port\pg_config_paths.h') || confess "Could not open pg_config_paths.h";
        print O  <<EOF;
#define PGBINDIR "/bin"
#define PGSHAREDIR "/share"
#define SYSCONFDIR "/etc"
#define INCLUDEDIR "/include"
#define PKGINCLUDEDIR "/include"
#define INCLUDEDIRSERVER "/include/server"
#define LIBDIR "/lib"
#define PKGLIBDIR "/lib"
#define LOCALEDIR "/share/locale"
#define DOCDIR "/doc"
#define HTMLDIR "/doc"
#define MANDIR "/man"
EOF
        close(O);
    }

    my $mf = Project::read_file('src\backend\catalog\Makefile');
    $mf =~ s{\\s*[\r\n]+}{}mg;
    $mf =~ /^POSTGRES_BKI_SRCS\s*:?=[^,]+,(.*)\)$/gm
      || croak "Could not find POSTGRES_BKI_SRCS in Makefile\n";
    my @allbki = split /\s+/, $1;
    foreach my $bki (@allbki)
    {
        next if $bki eq "";
        if (IsNewer('src/backend/catalog/postgres.bki', "src/include/catalog/$bki"))
        {
            print "Generating postgres.bki and schemapg.h...\n";
            chdir('src\backend\catalog');
            my $bki_srcs = join(' ../../../src/include/catalog/', @allbki);
            system(
"perl genbki.pl -I../../../src/include/catalog --set-version=$self->{majorver} $bki_srcs"
            );
            chdir('..\..\..');
            copyFile('src\backend\catalog\schemapg.h', 'src\include\catalog\schemapg.h');
            last;
        }
    }

    open(O, ">doc/src/sgml/version.sgml") || croak "Could not write to version.sgml\n";
    print O <<EOF;
<!ENTITY version "$self->{strver}">
<!ENTITY majorversion "$self->{majorver}">
EOF
    close(O);
}

sub GenerateDefFile
{
    my ($self, $deffile, $txtfile, $libname)  = @_;

    if (IsNewer($deffile,$txtfile))
    {
        print "Generating $deffile...\n";
        open(I,$txtfile) || confess("Could not open $txtfile\n");
        open(O,">$deffile") || confess("Could not open $deffile\n");
        print O "LIBRARY $libname\nEXPORTS\n";
        while (<I>)
        {
            next if (/^#/);
            next if (/^\s*$/);
            my ($f, $o) = split;
            print O " $f @ $o\n";
        }
        close(O);
        close(I);
    }
}

sub AddProject
{
    my ($self, $name, $type, $folder, $initialdir) = @_;

    my $proj = new Project($name, $type, $self);
    push @{$self->{projects}->{$folder}}, $proj;
    $proj->AddDir($initialdir) if ($initialdir);
    if ($self->{options}->{zlib})
    {
        $proj->AddIncludeDir($self->{options}->{zlib} . '\include');
        $proj->AddLibrary($self->{options}->{zlib} . '\lib\zdll.lib');
    }
    if ($self->{options}->{openssl})
    {
        $proj->AddIncludeDir($self->{options}->{openssl} . '\include');
        $proj->AddLibrary($self->{options}->{openssl} . '\lib\VC\ssleay32.lib', 1);
        $proj->AddLibrary($self->{options}->{openssl} . '\lib\VC\libeay32.lib', 1);
    }
    if ($self->{options}->{nls})
    {
        $proj->AddIncludeDir($self->{options}->{nls} . '\include');
        $proj->AddLibrary($self->{options}->{nls} . '\lib\libintl.lib');
    }
    if ($self->{options}->{krb5})
    {
        $proj->AddIncludeDir($self->{options}->{krb5} . '\inc\krb5');
        $proj->AddLibrary($self->{options}->{krb5} . '\lib\i386\krb5_32.lib');
        $proj->AddLibrary($self->{options}->{krb5} . '\lib\i386\comerr32.lib');
        $proj->AddLibrary($self->{options}->{krb5} . '\lib\i386\gssapi32.lib');
    }
    if ($self->{options}->{iconv})
    {
        $proj->AddIncludeDir($self->{options}->{iconv} . '\include');
        $proj->AddLibrary($self->{options}->{iconv} . '\lib\iconv.lib');
    }
    if ($self->{options}->{xml})
    {
        $proj->AddIncludeDir($self->{options}->{xml} . '\include');
        $proj->AddLibrary($self->{options}->{xml} . '\lib\libxml2.lib');
    }
    if ($self->{options}->{xslt})
    {
        $proj->AddIncludeDir($self->{options}->{xslt} . '\include');
        $proj->AddLibrary($self->{options}->{xslt} . '\lib\libxslt.lib');
    }
    return $proj;
}

sub Save
{
    my ($self) = @_;
    my %flduid;

    $self->GenerateFiles();
    foreach my $fld (keys %{$self->{projects}})
    {
        foreach my $proj (@{$self->{projects}->{$fld}})
        {
            $proj->Save();
        }
    }

    open(SLN,">pgsql.sln") || croak "Could not write to pgsql.sln\n";
    print SLN <<EOF;
Microsoft Visual Studio Solution File, Format Version 9.00
# Visual Studio 2005
EOF

    foreach my $fld (keys %{$self->{projects}})
    {
        foreach my $proj (@{$self->{projects}->{$fld}})
        {
            print SLN <<EOF;
Project("{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}") = "$proj->{name}", "$proj->{name}.vcproj", "$proj->{guid}"
EndProject
EOF
        }
        if ($fld ne "")
        {
            $flduid{$fld} = Win32::GuidGen();
            print SLN <<EOF;
Project("{2150E333-8FDC-42A3-9474-1A3956D46DE8}") = "$fld", "$fld", "$flduid{$fld}"
EndProject
EOF
        }
    }

    print SLN <<EOF;
Global
	GlobalSection(SolutionConfigurationPlatforms) = preSolution
		Debug|$self->{platform}= Debug|$self->{platform}
		Release|$self->{platform} = Release|$self->{platform}
	EndGlobalSection
	GlobalSection(ProjectConfigurationPlatforms) = postSolution
EOF

    foreach my $fld (keys %{$self->{projects}})
    {
        foreach my $proj (@{$self->{projects}->{$fld}})
        {
            print SLN <<EOF;
		$proj->{guid}.Debug|$self->{platform}.ActiveCfg = Debug|$self->{platform}
		$proj->{guid}.Debug|$self->{platform}.Build.0  = Debug|$self->{platform}
		$proj->{guid}.Release|$self->{platform}.ActiveCfg = Release|$self->{platform}
		$proj->{guid}.Release|$self->{platform}.Build.0 = Release|$self->{platform}
EOF
        }
    }

    print SLN <<EOF;
	EndGlobalSection
	GlobalSection(SolutionProperties) = preSolution
		HideSolutionNode = FALSE
	EndGlobalSection
	GlobalSection(NestedProjects) = preSolution
EOF

    foreach my $fld (keys %{$self->{projects}})
    {
        next if ($fld eq "");
        foreach my $proj (@{$self->{projects}->{$fld}})
        {
            print SLN "\t\t$proj->{guid} = $flduid{$fld}\n";
        }
    }

    print SLN <<EOF;
	EndGlobalSection
EndGlobal
EOF
    close(SLN);
}

sub GetFakeConfigure
{
    my $self = shift;

    my $cfg = '--enable-thread-safety';
    $cfg .= ' --enable-cassert' if ($self->{options}->{asserts});
    $cfg .= ' --enable-integer-datetimes' if ($self->{options}->{integer_datetimes});
    $cfg .= ' --enable-nls' if ($self->{options}->{nls});
    $cfg .= ' --with-ldap' if ($self->{options}->{ldap});
    $cfg .= ' --without-zlib' unless ($self->{options}->{zlib});
    $cfg .= ' --with-openssl' if ($self->{options}->{ssl});
    $cfg .= ' --with-ossp-uuid' if ($self->{options}->{uuid});
    $cfg .= ' --with-libxml' if ($self->{options}->{xml});
    $cfg .= ' --with-libxslt' if ($self->{options}->{xslt});
    $cfg .= ' --with-krb5' if ($self->{options}->{krb5});
    $cfg .= ' --with-tcl' if ($self->{options}->{tcl});
    $cfg .= ' --with-perl' if ($self->{options}->{perl});
    $cfg .= ' --with-python' if ($self->{options}->{python});

    return $cfg;
}

1;
