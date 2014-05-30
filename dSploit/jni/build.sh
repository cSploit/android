#!/bin/sh
ndk-build -j$(grep -E "^processor" /proc/cpuinfo | wc -l) && \
unzip -q ../assets/tools.zip && \
for tool in arpspoof tcpdump ettercap hydra nmap; do \
	mv ../libs/armeabi/$tool ./tools/$tool/$tool; \
done && \
find ./nmap -name "*.lua" -print0 | rsync -aq --files-from=- --from0 ./ ./tools/ && \
rsync -aq ./nmap/scripts/ ./tools/nmap/scripts/ &&
zip -qr tools.zip tools && \
rm -rf tools && \
mv tools.zip ../assets/

directories="/enc/trans/
/enc/
/io/
/digest/
/dl/
/json/ext/
/mathn/
/racc/
/"

oldpwd=$(pwd)

function die {
	echo "FAILED"
	echo "--------------------------------------------------"
	echo "an error occurred while creating the ruby package."
	echo "see build.log for more info"
	echo "--------------------------------------------------"
	
	cd "${oldpwd}"
	exit 1
}

test -d "${oldpwd}" || die

exec 3> build.log

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
cd rubyroot
tar -cJf ../../assets/ruby.tar.xz . || die
cd "${oldpwd}"
rm -rf rubyroot

echo -ne "ok\n"

echo "ruby package successfuly created"

