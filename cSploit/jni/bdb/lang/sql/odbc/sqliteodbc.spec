%define name sqliteodbc
%define version 0.992
%define release 1

Name: %{name}
Summary: ODBC driver for SQLite
Version: %{version}
Release: %{release}
Source: http://www.ch-werner.de/sqliteodbc/%{name}-%{version}.tar.gz
Group: Applications/Databases
URL: http://www.ch-werner.de/sqliteodbc
License: BSD
BuildRoot: %{_tmppath}/%{name}-%{version}-root-%(id -u)

%description
ODBC driver for SQLite interfacing SQLite 2.x and/or 3.x using the
unixODBC or iODBC driver managers. For more information refer to
http://www.sqlite.org    -  SQLite engine
http://www.unixodbc.org  -  unixODBC Driver Manager
http://www.iodbc.org     -  iODBC Driver Manager

%prep
%setup -q
mv configure.in DONT-configure.in # RPM 3.x, don't run libtoolize

%build
%configure
make

%install
mkdir -p $RPM_BUILD_ROOT%{_libdir}
make install DESTDIR=$RPM_BUILD_ROOT
rm -f $RPM_BUILD_ROOT%{_libdir}/libsqliteodbc*.{a,la}
rm -f $RPM_BUILD_ROOT%{_libdir}/libsqlite3odbc*.{a,la}
rm -f $RPM_BUILD_ROOT%{_libdir}/libsqlite3_mod_*.{a,la}

%clean
rm -rf $RPM_BUILD_ROOT

%post
if [ -x /usr/bin/odbcinst ] ; then
   INST=/tmp/sqliteinst$$
   if [ -r %{_libdir}/libsqliteodbc.so ] ; then
      cat > $INST << 'EOD'
[SQLITE]
Description=SQLite ODBC 2.X
Driver=%{_libdir}/libsqliteodbc.so
Setup=%{_libdir}/libsqliteodbc.so
Threading=2
FileUsage=1
EOD
      /usr/bin/odbcinst -q -d -n SQLITE | grep '^\[SQLITE\]' >/dev/null || {
	 /usr/bin/odbcinst -i -d -n SQLITE -f $INST || true
      }
      cat > $INST << 'EOD'
[SQLite Datasource]
Driver=SQLITE
EOD
      /usr/bin/odbcinst -q -s -n "SQLite Datasource" | \
	 grep '^\[SQLite Datasource\]' >/dev/null || {
	 /usr/bin/odbcinst -i -l -s -n "SQLite Datasource" -f $INST || true
      }
   fi
   if [ -r %{_libdir}/libsqlite3odbc.so ] ; then
      cat > $INST << 'EOD'
[SQLITE3]
Description=SQLite ODBC 3.X
Driver=%{_libdir}/libsqlite3odbc.so
Setup=%{_libdir}/libsqlite3odbc.so
Threading=2
FileUsage=1
EOD
      /usr/bin/odbcinst -q -d -n SQLITE3 | grep '^\[SQLITE3\]' >/dev/null || {
	 /usr/bin/odbcinst -i -d -n SQLITE3 -f $INST || true
      }
      cat > $INST << 'EOD'
[SQLite3 Datasource]
Driver=SQLITE3
EOD
      /usr/bin/odbcinst -q -s -n "SQLite3 Datasource" | \
	 grep '^\[SQLite3 Datasource\]' >/dev/null || {
	 /usr/bin/odbcinst -i -l -s -n "SQLite3 Datasource" -f $INST || true
      }
   fi
   rm -f $INST || true
fi

%preun
if [ "$1" = "0" ] ; then
    test -x /usr/bin/odbcinst && {
	/usr/bin/odbcinst -u -d -n SQLITE || true
	/usr/bin/odbcinst -u -l -s -n "SQLite Datasource" || true
	/usr/bin/odbcinst -u -d -n SQLITE3 || true
	/usr/bin/odbcinst -u -l -s -n "SQLite3 Datasource" || true
    }
    true
fi

%files
%defattr(-, root, root)
%doc README license.terms ChangeLog
%{_libdir}/*.so*

%changelog
* Mon Apr 01 2013 ...
- automatically recreated by configure ...
