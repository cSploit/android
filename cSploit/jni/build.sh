#!/bin/bash

oldpwd=$(pwd)

UPDATE_SERVER="http://update.csploit.org/"
RUBY_VERSION=1.0.0
CORE_VERSION=1.0.6
DEBUG=false

die() {
	echo "FAILED"
	echo "--------------------------------------------------"
	echo "an error occurred while creating the $pkg package."
	echo "see build.log for more info"
	echo "--------------------------------------------------"
	
	cd "${oldpwd}"
	exit 1
}

test -d "${oldpwd}" || die

exec 3> build.log

pkg="init"
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
directories="/enc/trans/
/enc/
/io/
/digest/
/dl/
/json/ext/
/mathn/
/racc/
/"
apis="19"
abis="armeabi
armeabi-v7a"

check_ndk() {
  ndk_build=$(which ndk-build) || \
  (echo "android NDK not found, please ensure that it's directory is in your PATH"; die)

  ndk_dir=$(dirname "${ndk_build}")
  ndk_dir="${ndk_dir}/build/core/"

  sudo=""

  test -w "${ndk_dir}" || sudo="sudo"

  for s in $ndk_empty_scripts; do
    if [ ! -f "${ndk_dir}${s}" ]; then
      { $sudo touch "${ndk_dir}${s}" 2>&1 | tee >(cat - >&3) ;} || die
    fi
  done
}

delete_core_packages() {
  find ../dist/ -name "core*" -type f -exec rm "{}" \; 2>&3
}

delete_ruby_packages() {
  find ../dist/ -name "ruby*" -type f -exec rm "{}" \; 2>&3
}

build() {
  app_root=$(readlink -f ../)
  
  ndk_args=""
  
  test "x$DEBUG" == "xtrue" && ndk_args+="APP_OPTIM=debug NDK_DEBUG=1 " # debug
  ndk_args+="APP_PLATFORM=android-${api} " # force android api level 
  ndk_args+="NDK_OUT=${app_root}/obj/android-${api} " # objects directory

  echo
  echo -n "[$pkg] compiling for android-${api}..."
  
  cpus=$(grep -E "^processor" /proc/cpuinfo | wc -l)
  
  echo "running: ndk-build $ndk_args -j${cpus} $@" >&3
  
  ndk-build $ndk_args -j${cpus} $@ >&3 2>&1 || die
  
  echo "ok"
}

select_lower_api() {
  api="0"
  
  for _api in $apis; do
    test ${_api} -lt ${api} -o ${api} -eq 0 && api="${_api}"
  done
}

build_jni_libs() {
  app_root=$(readlink -f ../)
  
  strip=$( ndk-which strip ) 2>&3 || die
  
  build cSploitCommon cSploitClient
  
  echo -ne "[jni] installing into libs..."
  
  for abi in $abis; do 
    objs="${app_root}/obj/android-${api}/local/${abi}"
    bins=$(readlink -fm ../libs/${abi})
    
    mkdir -p "${bins}"
    
    for lib in cSploitCommon cSploitClient; do
      install -p "${objs}/lib${lib}.so" "${bins}/lib${lib}.so" >&3 2>&1 || die
      ${strip} --strip-unneeded "${bins}/lib${lib}.so" >&3 2>&1 || die
    done
  done
  
  echo "ok"
}

copy_jni_libs() {
  
  select_lower_api
  
  echo -n "[jni] copying libs to java project..."
  
  for abi in $abis; do
    javaLibs=$(readlink -fm ../src/org/csploit/android/jniLibs/${abi}/)
    bins=$(readlink -fm ../libs/${abi})
    
    { test -d "${javaLibs}" || mkdir -p "${javaLibs}" ;} >&3 2>&1 || die
    cp "${bins}/libcSploitClient.so" "${bins}/libcSploitCommon.so" "${javaLibs}" >&3 2>&1 || die
  
  done
  
  echo "ok"
}

pack_core() {
  echo
  echo "[core] building android${api}.${abi} package"
  
  jni_root=$(readlink -f ./)
  out="/tmp/cSploitCore"
  bins=$(readlink -f ../libs/${abi})
  
  rm -rf "${out}" >&3 2>&1 || die
  mkdir -p "${out}" >&3 2>&1 || die
  
  echo -n "[core] -  copying programs..."
  cp "${bins}/cSploitd" "${out}/"

  for tool in arpspoof tcpdump ettercap hydra nmap fusemounts network-radar; do
    mkdir -p "${out}/tools/$tool" >&3 2>&1
    cp "${bins}/$tool" "${out}/tools/$tool/$tool" >&3 2>&1 || die
  done
  echo -ne "ok\n[core] -  copying libraries..."
  
  mkdir -p "${out}/handlers" >&3 2>&1 || die
  
  for lib in ${bins}/libhandlers_*.so; do
    lib=$(basename "$lib")
    cp "${bins}/${lib}" "${out}/handlers/${lib:12}" >&3 2>&1 || die
  done
  
  for ec_lib in ${bins}/libec_*.so; do
    ec_lib=$(basename "$ec_lib")
    cp "${bins}/$ec_lib" "${out}/tools/ettercap/${ec_lib:3}" >&3 2>&1 || die
  done
  echo -ne "ok\n[core] -  copying scripts..."
  
  rsync -aq --include "*/" --include "*.lua" --exclude "*" "${jni_root}/nmap/" "${out}/tools/nmap/" >&3 2>&1 || die
  rsync -aq "${jni_root}/nmap/scripts/" "${out}/tools/nmap/scripts/" >&3 2>&1 || die

  cp "${jni_root}/known-issues/start_daemon.sh" "${out}/"
  chmod 700 "${out}/start_daemon.sh"

  echo -ne "ok\n[core] -  copying configuration/database files..."
  for f in $nmap_data; do
    cp "${jni_root}/nmap/$f" "${out}/tools/nmap/" >&3 2>&1 || die
  done

  mkdir -p "${out}/tools/ettercap/share" || die

  for f in $ettercap_share; do
    cp ${jni_root}/ettercap*/share/$f "${out}/tools/ettercap/share/" >&3 2>&1 || die
  done

  echo "android:DEADBEEF" > "${out}/users" 2>&3 || die

  echo -ne "ok\n[core] -  creating archive..."
  
  (cd ${out} && zip -qr ../core.zip ./) >&3 2>&1 || die
  rm -rf "${out}" >&3 2>&1 || die
  if [ ! -d ../assets ]; then
    mkdir ../assets >&3 2>&1 || die
  fi
  mv /tmp/core.zip ../assets/ >&3 2>&1 || die
    
  echo "ok"
}

build_cores() {
  check_ndk
  delete_core_packages
  
  for api in $apis; do
    build
    for abi in $abis; do
      pack_core
    done
  done
}

build_ruby() {
  check_ndk
  delete_ruby_packages
  
  for api in $apis; do
    build
    for abi in $abis; do
      pack_ruby
    done
  done
}

build_jni() {
  check_ndk
  select_lower_api
  
  build_jni_libs
}

pack_ruby() {
  system_ruby=""
  
  echo
  echo "[ruby] building android${api}.${abi} package"
  
  for bin in ruby ruby19; do 
    bin=$(which "$bin")
    
    test -n "$bin" && { $bin -v | grep -q "1.9" ;} && system_ruby="${bin}"
  done

  test -n "${system_ruby}" || { echo "ruby 1.9 not found" >&2; echo "ruby 1.9 not found" >&3; die ;}

  echo -n "[ruby]   - creating root..."

  rm -rf rubyroot >&3 2>&1 || die
  rubyroot=$(readlink -f rubyroot) || die
  echo "rubyroot='${rubyroot}'" >&3
  test -n "${rubyroot}" || die
  
  for d in $directories; do
    echo "making \`${rubyroot}/lib/ruby/1.9.1/arm-linux-androideabi$d'" >&3
    mkdir -p "${rubyroot}/lib/ruby/1.9.1/arm-linux-androideabi$d" >&3 2>&1 || die
  done
  
  mkdir -p "${rubyroot}/lib/ruby/site_ruby/1.9.1/arm-linux-androideabi" || die
  mkdir -p "${rubyroot}/lib/ruby/vendor_ruby/1.9.1/arm-linux-androideabi" || die
  mkdir -p "${rubyroot}/bin" >&3 2>&1 || die
  mkdir -p "${rubyroot}/etc" >&3 2>&1 || die
  mkdir -p "${rubyroot}/home/ruby" >&3 2>&1 || die
  
  echo -ne "ok\n[ruby]   - disabling documentation when installing gems..."
  
  echo -e 'install: --no-document\nupdate: --no-document' > "${rubyroot}/etc/gemrc" 2>&3 || die
  cp "${rubyroot}/etc/gemrc" "${rubyroot}/home/ruby/.gemrc" || die
  
  echo -ne "ok\n[ruby]   - copying binaries..."
  
  rsync -va ruby/bin/ rubyroot/bin/ >&3 2>&1 || die
  for prog in clear infocmp tabs tic toe tput; do
    cp "../libs/${abi}/${prog}" "${rubyroot}/bin/" >&3 2>&1 || die
  done
  cp "../libs/${abi}/ruby" "${rubyroot}/bin/" >&3 2>&1 || die
  
  echo -ne "ok\n[ruby]   - copying libraries..."
  
  for d in $directories; do
    base="libRUBY${d//\//_}"
    search_name="${base}*.so"
    dest="${oldpwd}/rubyroot/lib/ruby/1.9.1/arm-linux-androideabi$d"
    find "${oldpwd}/../libs/${abi}/" -name "${search_name}" | while read file; do
      newname=$(basename "$file")
      newname=${newname/$base/}
      echo "copying \`${file}' to \`${dest}${newname}'" >&3
      cp "$file" "${dest}${newname}" >&3 2>&1|| die
    done
  done
  
  echo -ne "ok\n[ruby]   - copying script libraries..." 
  
  cd ruby/lib >&3 2>&1 || die
  ( find . -name "*.rb" -print0 | rsync -va --files-from=- --from0 ./ ../../rubyroot/lib/ruby/1.9.1/ ) >&3 2>&1  || die
  cd ../.ext/common >&3 2>&1 || die
  ( find . -name "*.rb" -print0 | rsync -va --files-from=- --from0 ./ ../../../rubyroot/lib/ruby/1.9.1/ ) >&3 2>&1 || die
  cp "${oldpwd}/ruby/rbconfig.rb" "${rubyroot}/lib/ruby/1.9.1/arm-linux-androideabi/" >&3 2>&1 || die
  
  echo -ne "ok\n[ruby]   - updating rubygems..."
  
  echo -e "require 'arm-linux-androideabi/rbconfig'" > "${rubyroot}/lib/ruby/1.9.1/rbconfig.rb" 2>&3 || die
  unset RUBYOPT
  export RUBYLIB="${rubyroot}/lib/ruby/site_ruby/1.9.1:${rubyroot}/lib/ruby/site_ruby:${rubyroot}/lib/ruby/vendor_ruby/1.9.1:${rubyroot}/lib/ruby/vendor_ruby:${rubyroot}/lib/ruby/1.9.1"
  export HOME="${rubyroot}/home/ruby"
  export GEM_HOME="${rubyroot}/lib/ruby/gems/1.9.1"
  echo "RUBYLIB=${RUBYLIB}" >&3
  mv "${rubyroot}/bin/ruby" "${rubyroot}/bin/ruby.arm" >&3 2>&1 || die
  ln -s "${system_ruby}" "${rubyroot}/bin/ruby" >&3 2>&1 || die
  $system_ruby "${rubyroot}/bin/gem" "update" "--system" "--no-ri" "--no-rdoc" "-V" >&3 2>&1 || die
  rm "${rubyroot}/bin/ruby" >&3 2>&1 || die
  rm "${rubyroot}/lib/ruby/1.9.1/rbconfig.rb" >&3 2>&1 || die
  rm -rf "${rubyroot}/home/ruby/.gem" >&3 2>&1 || die
  mv "${rubyroot}/bin/ruby.arm" "${rubyroot}/bin/ruby" >&3 2>&1 || die
  cd "${rubyroot}"
  sed -i "1s,^#!${rubyroot}/bin/ruby,#!/usr/bin/env ruby," $(find . -type f) >&3 2>&1 || die
  
  echo -ne "ok\n[ruby]   - creating archive..."
  
  cd "${oldpwd}"
  mkdir -p ../dist >&3 2>&1 || die
  cd rubyroot
  tar -cJf "../../dist/ruby-${RUBY_VERSION}+android${api}.${abi}.tar.xz" . >&3 2>&1 || die
  cd "${oldpwd}"
  rm -rf rubyroot
  
  echo "ok"
}

pkg="jni"

test "$#" -ne 1 || pkg=$1

case $pkg in
ruby) build_ruby
  ;;
core|cores) build_cores
            copy_jni_libs
  ;;
jni) build_jni
     copy_jni_libs
  ;;
*)
  scriptname=$(basename "$0")
  echo -e "Usage: $scriptname <task>\n\ntask must be one of:\n  - ruby : build the ruby archive\n  - cores: build native tools used by dSploit (default)" >&2
  ;;
esac

exit 0
