%global __provides_exclude_from ^%{_libdir}/%{name}/.*\.so$

Name:           javasqlite
Version:        20120209
Release:        1%{?dist}
Summary:        SQLite Java Wrapper/JDBC Driver

Group:          Development/Libraries
License:        BSD
URL:            http://www.ch-werner.de/javasqlite/
Source0:        http://www.ch-werner.de/javasqlite/%{name}-%{version}.tar.gz
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

# >= 3.4 for zeroblob stuff in %%check's test3
BuildRequires:  sqlite-devel >= 3.4
BuildRequires:  java-1.6.0-openjdk-devel
BuildRequires:  java-1.6.0-openjdk-javadoc
Requires:       java-1.6.0-openjdk

%description
javasqlite is a Java wrapper including a basic JDBC driver for the
SQLite database engine. It is designed using JNI to interface to the
SQLite API.

%package        javadoc
Summary:        API documentation for %{name}
Group:          Documentation
BuildArch:      noarch
Requires:       java-javadoc

%description    javadoc
API documentation for %{name}.

%prep
%setup -q
sed -e 's|@JNIPATH@|%{_libdir}/%{name}|' fc-jnipath.patch | patch -p1 --fuzz=0
sed -i -e 's/\r//g' doc/ajhowto.txt
f=ChangeLog ; iconv -f iso-8859-1 -t utf-8 $f > $f.utf8 ; mv $f.utf8 $f

%build
origpath="$PATH"
# Note that --enable-load-extension has security concerns, see configure --help
common_flags="
    --with-jardir=%{_libdir}/%{name}
    --libdir=%{_libdir}/%{name}
"

export PATH="%{_jvmdir}/java-1.6.0/bin:$origpath" # bug 460761 (to be sure)
%configure $common_flags --with-jdk=%{_jvmdir}/java-1.6.0
make # Java build not parallel clean
make javadoc JAVADOCLINK=%{_javadocdir}/java

%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT
rm -f $RPM_BUILD_ROOT%{_libdir}/%{name}/libsqlite_jni.la
install -dm 755 $RPM_BUILD_ROOT%{_javadocdir}/%{name}
cp -pR doc/* $RPM_BUILD_ROOT%{_javadocdir}/%{name}

%clean
rm -rf $RPM_BUILD_ROOT

%check
origpath="$PATH"
for javaver in 1.6.0 ; do
    jdir=%{_jvmdir}/java-$javaver/bin
    export PATH="$jdir:$origpath" # bug 460761
    # test2 is for SQLite 2.x, which we don't support
    make JAVA_RUN="$jdir/java" JAVAC="$jdir/javac" test test3
done

%files
%defattr(-,root,root,-)
%doc ChangeLog license.terms
%dir %{_libdir}/%{name}/
%{_libdir}/%{name}/sqlite.jar
%{_libdir}/%{name}/libsqlite_jni.so

%files javadoc
%defattr(-,root,root,-)
%doc license.terms
%{_javadocdir}/%{name}

%changelog
* Sun May  1 2011 Ville Skyttä <ville.skytta@iki.fi> - 20120209-1
- Use rpmbuild >= 4.9's built in Provides filtering.
