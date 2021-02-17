ACLOCAL_AMFLAGS=-I libltdl/m4

SUBDIRS = \
	extras \
	log \
	lst \
	ini \
    libltdl \
	odbcinst \
	DriverManager \
	exe \
	cur \
	DRVConfig \
	Drivers \
	include \
	doc \
    man \
	samples 

EXTRA_DIST = \
	README.OSX \
	README.VMS \
	README.QNX \
	README.CYGWIN \
	README.AIX \
	README.SOLARIS \
	README.INTERIX \
	vms/install_image.com \
	vms/odbc2_axp.opt \
	vms/odbc_axp.opt \
	vms/odbcinst_axp.opt \
	vms/odbc_setup.com \
	vms/drivermanager_axp.opt \
	vmsbuild.com \
	Interix/configure \
	Interix/config.guess \
	Interix/libtool \
    m4 \
    configure.in \
    Makefile.svn

install-data-am:
	-touch $(DESTDIR)${sysconfdir}/odbcinst.ini
	-touch $(DESTDIR)${sysconfdir}/odbc.ini
	-mkdir -p $(DESTDIR)${sysconfdir}/ODBCDataSources
	cp unixodbc_conf.h $(DESTDIR)${includedir}/unixodbc_conf.h
