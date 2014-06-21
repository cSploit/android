#!/bin/sh

oldpwd=$(pwd)

UPDATE_SERVER="http://update.dsploit.net/"
RUBY_VERSION=1

die() {
	echo "FAILED"
	echo "--------------------------------------------------"
	echo "an error occurred while creating the $pkg package."
	echo "see build.log for more info"
	echo "--------------------------------------------------"
	
	cd "${oldpwd}"
	exit 1
}

create_archive_metadata() {
  echo -n "creating metadata file..."
  md5=$(md5sum "$1" 2>&3 | grep -oE "[0-9a-f]{32}")
  test -n "${md5}" || die
  sha1=$(sha1sum "$1" 2>&3 | grep -oE "[0-9a-f]{40}")
  test -n "${sha1}" || die
  filename=$(basename "$1")
  cat > "$2" 2>&3 <<EOF
{
  "url" : "${UPDATE_SERVER}${filename}",
  "name" : "${filename}",
  "version" : $3,
  "archiver" : "$4",
  "compression" : "$5",
  "md5" : "${md5}",
  "sha1" : "${sha1}"
}
EOF
  echo -ne "ok\n"
}

test -d "${oldpwd}" || die

exec 3> build.log

pkg="tools"
nmap_data="nmap-mac-prefixes
nmap-payloads
nmap-rpc
nmap-os-db
nmap-protocols
nmap-services
nmap-service-probes"
ettercap_share="etter.mime
etter.filter.kill
etter.services
etterfilter.cnt
etter.fields
etter.conf
etter.ssl.crt
etter.filter.ssh
etter.filter
etter.dns
etterfilter.tbl
etter.finger.os
etterlog.dtd
etter.filter.examples
etter.filter.pcre
etter.finger.mac"
ndk_empty_scripts="build-host-executable.mk
build-host-shared-library.mk
build-host-static-library.mk"

echo "*** creating tools package ***"

ndk_build=$(which ndk-build) || \
(echo "android NDK not found, please ensure that it's directory is in your PATH"; die)

ndk_dir=$(dirname "${ndk_build}")
ndk_dir+="/build/core/"

sudo=""

test -w "${ndk_dir}" || sudo="sudo"

for s in $ndk_empty_scripts; do
  if [ ! -f "${ndk_dir}${s}" ]; then
    $sudo touch "${ndk_dir}${s}" >&3 2>&1 || die
  fi
done

echo -n "building native executables..."
ndk-build -j$(grep -E "^processor" /proc/cpuinfo | wc -l) >&3 2>&1 || die
echo -ne "ok\ncopying programs..."
for tool in arpspoof tcpdump ettercap hydra nmap fusemounts; do
	mkdir -p ./tools/$tool >&3 2>&1
	cp ../libs/armeabi/$tool ./tools/$tool/$tool >&3 2>&1 || die
done
echo -ne "ok\ncopying libraries..."
for ec_lib in ../libs/armeabi/libec_*.so; do
	ec_lib=$(basename "$ec_lib")
	cp ../libs/armeabi/$ec_lib ./tools/ettercap/${ec_lib:3} >&3 2>&1 || die
done
echo -ne "ok\ncopying scripts..."
find ./nmap -name "*.lua" -print0 | rsync -aq --files-from=- --from0 ./ ./tools/ >&3 2>&1 || die
rsync -aq ./nmap/scripts/ ./tools/nmap/scripts/ >&3 2>&1 || die

echo -ne "ok\ncopying configuration/database files..."
for f in $nmap_data; do
	cp ./nmap/$f ./tools/nmap/ >&3 2>&1 || die
done

mkdir -p ./tools/ettercap/share || die

for f in $ettercap_share; do
	cp ./ettercap*/share/$f ./tools/ettercap/share/ >&3 2>&1 || die
done

echo -ne "ok\ncreating archive..."
zip -qr tools.zip tools >&3 2>&1 || die
echo "ok"
rm -rf tools >&3 2>&1 || die
if [ ! -d ../assets ]; then
	mkdir ../assets >&3 2>&1 || die
fi
mv tools.zip ../assets/ >&3 2>&1 || die

directories="/enc/trans/
/enc/
/io/
/digest/
/dl/
/json/ext/
/mathn/
/racc/
/"
pkg="ruby"

echo -e "\n*** creating ruby package ***"

echo -n "creating rubyroot..."
rm -rf rubyroot >&3 2>&1 || die
rubyroot=$(realpath rubyroot)
for d in $directories; do
	echo "making \`${rubyroot}/lib/ruby/1.9.1/arm-linux-androideabi$d'" >&3
	mkdir -p "${rubyroot}/lib/ruby/1.9.1/arm-linux-androideabi$d" >&3 2>&1 || die
done
mkdir -p "${rubyroot}/lib/ruby/site_ruby/1.9.1/arm-linux-androideabi" || die
mkdir -p "${rubyroot}/lib/ruby/vendor_ruby/1.9.1/arm-linux-androideabi" || die
mkdir -p "${rubyroot}/bin" >&3 2>&1 || die
mkdir -p "${rubyroot}/etc" >&3 2>&1 || die
mkdir -p "${rubyroot}/home/ruby" >&3 2>&1 || die
echo -ne "ok\ndisabling documentation install for gems..."
echo -e 'install: --no-document\nupdate: --no-document' > "${rubyroot}/etc/gemrc" 2>&3 || die
cp "${rubyroot}/etc/gemrc" "${rubyroot}/home/ruby/.gemrc" || die
echo -ne "ok\ncopying binaries..."
rsync -va ruby/bin/ rubyroot/bin/ >&3 2>&1 || die
for prog in clear infocmp tabs tic toe tput; do
	cp "../libs/armeabi/${prog}" "${rubyroot}/bin/" >&3 2>&1 || die
done
cp ../libs/armeabi/ruby "${rubyroot}/bin/" >&3 2>&1 || die
echo -ne "ok\ncopying libraries..."
for d in $directories; do
	base="libRUBY${d//\//_}"
	search_name="${base}*.so"
	dest="${oldpwd}/rubyroot/lib/ruby/1.9.1/arm-linux-androideabi$d"
	find "${oldpwd}/../libs/armeabi/" -name "${search_name}" | while read file; do
		newname=$(basename "$file")
		newname=${newname/$base/}
		echo "copying \`${file}' to \`${dest}${newname}'" >&3
		cp "$file" "${dest}${newname}" >&3 2>&1|| die
	done
done
echo -ne "ok\ncopying script libraries..." 
cd ruby/lib >&3 2>&1 || die
( find . -name "*.rb" -print0 | rsync -va --files-from=- --from0 ./ ../../rubyroot/lib/ruby/1.9.1/ ) >&3 2>&1  || die
cd ../.ext/common >&3 2>&1 || die
( find . -name "*.rb" -print0 | rsync -va --files-from=- --from0 ./ ../../../rubyroot/lib/ruby/1.9.1/ ) >&3 2>&1 || die
cp "${oldpwd}/ruby/rbconfig.rb" "${rubyroot}/lib/ruby/1.9.1/arm-linux-androideabi/" >&3 2>&1 || die
echo -ne "ok\ncreating archive..."

cd "${oldpwd}"

mkdir -p ../dist >&3 2>&1 || die

cd rubyroot
tar -cJf ../../dist/ruby.tar.xz . >&3 2>&1 || die
cd "${oldpwd}"
rm -rf rubyroot

echo -ne "ok\n"

create_archive_metadata ../dist/ruby.tar.xz ../dist/ruby.json $RUBY_VERSION tar xz
