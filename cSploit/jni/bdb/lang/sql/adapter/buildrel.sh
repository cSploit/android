#!/bin/bash

OLD_DB_VERSION=db-4.8.25
NEW_DB_VERSION=4.8.908
PACKAGE_NAME=db-$NEW_DB_VERSION

# We should be running in a db_sqlite repository, where do we get DB sources?
DB_MERCURIAL_REPO=~/work/db.clean

die() {
	echo "$@"
	exit 1
}

START_DIR=`/bin/pwd`
PATCH_DIR=$START_DIR/db-patches
[ -f "$START_DIR/buildrel.sh" ] || die "$0 should be run from the sql/adapter directory"
RELEASE_DIR=`cd "$START_DIR/.." && /bin/pwd`/releases/$PACKAGE_NAME
rm -rf $RELEASE_DIR
mkdir -p $RELEASE_DIR

# Grab the original source trees.
#hg clone $DB_MERCURIAL_REPO db
#hg clone $DB_SQLITE_MERCURIAL_REPO db_sqlite

# Make archive copies of the trees, don't use the packaged Berkeley DB, since
# we want to run bumprel, and that is not clean there.
(cd $DB_MERCURIAL_REPO && hg archive -r $OLD_DB_VERSION $RELEASE_DIR)

# Remove unneccessary files from the DB tree.
(cd $RELEASE_DIR && rm -rf ./docs ./docs_src)

# Patch the Berkeley DB tree
for p in $PATCH_DIR/*.patch ; do
	(cd $RELEASE_DIR && patch -p1) < $p
done

# Some changes require configure to be regenerated.
(cd $RELEASE_DIR && find . -name '*.rej' -o -name '*.orig' | xargs rm -f)
(cd $RELEASE_DIR/dist && sh s_config)

# Bump the release version of Berkeley DB. Needs to happen before s_perm
(cd $RELEASE_DIR/dist && ./bumprel $NEW_DB_VERSION > /dev/null)

# Taken from the db-4.8.24/dist/buildpkg script (ideally we would add a mode
# to that script to just do this work):
# Remove source directories we don't distribute.
R=$RELEASE_DIR
cd $R && rm -rf build_brew_x build_s60_x
cd $R && rm -rf java/src/com/sleepycat/xa
cd $R && rm -rf rpc_* dbinc/db_server_int.h dbinc_auto/rpc*.h
cd $R && rm -rf test/TODO test/upgrade test/scr036 test_erlang
cd $R && rm -rf test_perf test_purify test_repmgr
cd $R && rm -rf test_server test_stl/ms_examples test_stl/stlport
cd $R && rm -rf test_vxworks
cd $R && find . -name '.hg*' | xargs rm -f
cd $R && find . -name 'tags' | xargs rm -f
# Fix permissions
#cd $R && find . -type d | xargs chmod 775
#cd $R && find . -type f | xargs chmod 444
#cd $R && chmod 664 build_windows/*.dsp build_windows/*vcproj
#cd $R && chmod 664 csharp/doc/libdb_dotnet*.XML
#cd $R/dist && sh s_perm
# End taken from db-4.8.24/dist/buildpkg
cd $START_DIR

rm -f $RELEASE_DIR/README
hg tag $PACKAGE_NAME
hg archive $RELEASE_DIR

# Move the Berkeley DB code into the sqlite tree.
(cd $RELEASE_DIR/sql/adapter && sh ./install.sh)

# Remove unneccessary files from the DB SQL tree. It would be nice if we did
# not need to do this, but some invalid links are created in the directory.
rm -rf $RELEASE_DIR/sqlite/src/orig

# Generate a build script:
cat > $RELEASE_DIR/build.sh <<EOF
#!/bin/bash

case "\$1" in
fast)
	DB_CONF_ARGS="--disable-mutexsupport \$DB_CONF_ARGS"
	DBSQL_CFLAGS="-DBDBSQL_FAST_TRANSIENTS -DBDBSQL_OMIT_SHARING -DBDBSQL_OMIT_LEAKCHECK -DBDBSQL_PRELOAD_HANDLES -DBDBSQL_SINGLE_THREAD \$DBSQL_CFLAGS"
	DBSQL_CONF_ARGS="--disable-debug --disable-tcl \$DBSQL_CONF_ARGS"
	;;
extensible)
	DBSQL_CONF_ARGS="--enable-load-extension \$DBSQL_CONF_ARGS"
	export LIBS="\$LIBS -ldl"
	;;
esac

cd build_unix || exit \$?
env CFLAGS="-O3 \$DB_CFLAGS" ../dist/configure --enable-smallbuild --enable-statistics \$DB_CONF_ARGS && make amalgamate.lo

mkdir dbsql
cd dbsql
env CFLAGS="-O3 -I.. \$DBSQL_CFLAGS" ../../sqlite/configure \$DBSQL_CONF_ARGS && make LTLINK_EXTRAS="../amalgamate.lo" libsqlite3.la && make

echo 'Build finished. The SQL command line (sqlite3) and test runner (testfixture) can be found in build_unix/dbsql'
EOF

chmod 775 $RELEASE_DIR/build.sh $RELEASE_DIR/sql/adapter/*.sh

# Build the package into an archive.
cd $RELEASE_DIR/..
tar zcfh $PACKAGE_NAME.tar.gz $PACKAGE_NAME

echo "WARNING: Should push the new tag in db_sqlite to central repository."
