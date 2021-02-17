# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.

CONFIG=debug
#CONFIG=release
# will be appended to 'svn --version --quiet' output; set to zero to suppress
BUILDDESCR=0

MSBUILD=msbuild subversion_vcnet.sln /nologo /v:q /p:Configuration=$(CONFIG)
CP=cp
TEEPATH=C:\Path\To\Parent\Of\tee\dot\exe\and\other\utils\as\needed\see\below.80

SOURCESDIR=C:\Path\To\Dependency\Sources
SVNDIR=C:\Path\To\Subversion\SourceTree
TARGETDIR=$(SVNDIR)\dist
INSTALLDIR=E:\svn

EXPATVER=2.0.0
HTTPDVER=2.2.13
NEONVER=0.28.2
OPENSSLVER=0.9.8k
SQLITEVER=3.6.3
ZLIBVER=1.2.3
#ENABLE_ML=--enable-ml

PATCHESDIR=$(HOME)\mydiffs\svn
OPENSSLDIR=$(SOURCESDIR)\openssl-$(OPENSSLVER)
EXPATDIR=$(SOURCESDIR)\expat-$(EXPATVER)
NEONDIR=$(SOURCESDIR)\neon-$(NEONVER)
HTTPDDIR=$(SOURCESDIR)\httpd-$(HTTPDVER)
#APRDIR=$(SOURCESDIR)\apr
#APRUTILDIR=$(SOURCESDIR)\apr-util
#APRICONVDIR=$(SOURCESDIR)\apr-iconv
APRDIR=$(HTTPDDIR)\srclib\apr
APRUTILDIR=$(HTTPDDIR)\srclib\apr-util
APRICONVDIR=$(HTTPDDIR)\srclib\apr-iconv
SQLITEDIR=$(SOURCESDIR)\sqlite-amalgamation
ZLIBDIR=$(SOURCESDIR)\zlib-$(ZLIBVER)
SERFDIR=$(SOURCESDIR)\serf

all:
	@echo Available targets: newfiles versionstamp
	@echo Available targets: config
	@echo Available targets: progname testname
	@echo Available targets: all1 all2
	@echo Available targets: buildlog package
	@echo Available targets: check checklog
TARGETDIRset: SVNDIRset
	if X$(TARGETDIR) == X exit 1
SVNDIRset:
	if X$(SVNDIR) == X exit 1

removealllocalmods:
	svn revert -R .
	svn status --no-ignore | xargs rm -rf --
	svn status --no-ignore
	# last, in case of wc format bump
	rmdir /s /q dist local Release Debug

newfiles: SVNDIRset
	xcopy /s /y $(PATCHESDIR)\newfiles $(SVNDIR)
versionstamp:
	perl tools\dev\windows-build\document-version.pl subversion\include\svn_version.h $(TARGETDIR) $(SVNDIR) $(BUILDDESCR)
	svn diff subversion\include\svn_version.h

cleanup1: TARGETDIR
	del log.all-tests log.gen-make.py log.devenv log.win-tests
	rmdir /s /q $(TARGETDIR)\bin

clean:
	@echo "Sorry, '$@' target not yet implemented" >&2
# TODO also copy sqlite3.dll if it's used
install: TARGETDIRset
	test ! -d $(INSTALLDIR)
	mkdir $(INSTALLDIR)\bin
	pushd $(TARGETDIR)\bin                      &&\
	$(CP) *.exe        $(INSTALLDIR)/bin        &&\
	$(CP) libapr*.dll  $(INSTALLDIR)/bin        &&\
	$(CP) libeay32.dll $(INSTALLDIR)/bin        &&\
	$(CP) ssleay32.dll $(INSTALLDIR)/bin        &&\
	$(CP) libsvn*.dll  $(INSTALLDIR)/bin        &&\
	$(CP) ..\*.diff    $(INSTALLDIR)            &&\
	popd

targetdir: TARGETDIRset
	test -d $(TARGETDIR)\bin || mkdir $(TARGETDIR)\bin

# TODO: pass --with-apr-* if you don't have httpd; make --with-* args optional
config: targetdir
	python gen-make.py --$(CONFIG) --with-httpd=$(HTTPDDIR) --with-neon=$(NEONDIR) --with-serf=$(SERFDIR) --with-openssl=$(OPENSSLDIR) --with-sqlite=$(SQLITEDIR) --with-zlib=$(ZLIBDIR) $(ENABLE_ML) --vsnet-version=2008 -t vcproj 2>&1 | tee log.gen-make

# Visual Studio 2008
libsvn_auth_gnome_keyring libsvn_auth_kwallet libsvn_client libsvn_delta libsvn_diff libsvn_fs libsvn_fs_base libsvn_fs_fs libsvn_fs_util libsvn_ra libsvn_ra_local libsvn_ra_neon libsvn_ra_serf libsvn_ra_svn libsvn_repos libsvn_subr libsvn_wc: targetdir
	$(MSBUILD) /t:Libraries\$@
	$(MAKE) package
svn svnadmin svndumpfilter svnlook svnmucc svnserve svnsync svnversion entries-dump: targetdir
	$(MSBUILD) /t:Programs\$@
	$(MAKE) package
auth-test cache-test changes-test checksum-test client-test compat-test config-test db-test diff-diff3-test dir-delta-editor dirent_uri-test error-test fs-base-test fs-pack-test fs-test hashdump-test key-test locks-test mergeinfo-test opt-test path-test ra-local-test random-test repos-test revision-test skel-test stream-test string-test strings-reps-test svn_test_fs svn_test_main svndiff-test target-test time-test translate-test tree-conflict-data-test utf-test vdelta-test window-test: targetdir
	$(MSBUILD) /t:Tests\$@
	$(MAKE) package

__ALL__ __ALL_TESTS__: targetdir
	$(MSBUILD) /t:$@
	$(MAKE) package
all1: targetdir
	$(MSBUILD) /t:__ALL__
	$(MAKE) package
	@echo TODO entries-test
all2: targetdir
	$(MSBUILD) /t:__ALL_TESTS__
	$(MAKE) package

package:
	test -d $(SVNDIR)\$(CONFIG)\Subversion\tests\cmdline || mkdir $(SVNDIR)\$(CONFIG)\Subversion\tests\cmdline
	test -d $(TARGETDIR)\bin || mkdir $(TARGETDIR)\bin
	for %%i in (svn svnadmin svndumpfilter svnlook svnserve svnsync svnversion) do @$(CP) $(CONFIG)\subversion\%%i\%%i.exe $(TARGETDIR)\bin
	for %%i in (diff diff3 diff4) do @if exist $(CONFIG)\tools\diff\%%i.exe $(CP) $(CONFIG)\tools\diff\%%i.exe $(TARGETDIR)\bin
	$(CP) $(APRDIR)\$(CONFIG)/*.dll $(TARGETDIR)\bin
	$(CP) $(APRUTILDIR)\$(CONFIG)/*.dll $(TARGETDIR)\bin
	$(CP) $(APRICONVDIR)\$(CONFIG)/*.dll $(TARGETDIR)\bin
	$(CP) $(OPENSSLDIR)\out32dll/*.dll $(TARGETDIR)\bin
	for %%i in (client delta diff fs ra repos subr wc) do @$(CP) $(CONFIG)\subversion\libsvn_%%i\*.dll $(TARGETDIR)\bin

buildlog:
	gvim -c "set autoread nowrap" -c "/\(\<0 \)\@<!error" log.devenv
# 'make check'
# TODO: also support svncheck, etc
check:
	echo %date% %time% :: Starting fsfs file >> log.all-tests
	python win-tests.py --verbose --cleanup --bin=$(TARGETDIR)\bin --$(CONFIG) -f fsfs 2>&1 | %TEEPATH%\tee log.win-tests
	echo %date% %time% :: Finished fsfs file >> log.all-tests


# check errors
checklog:
	gvim -c "set autoread" -p log.win-tests *\*.log "+silent! /X\@<!FAIL\|XPASS"

tags: .
	REM vim +Ctags +quit
	ctags -R .
	$(CP) tags ..\svntags
