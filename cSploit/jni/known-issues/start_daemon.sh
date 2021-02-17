#!/system/bin/sh

exec 3<> ./start_daemon.log

script_dir="$(readlink /proc/$$/cwd )"

die1() {
  cd "$script_dir"
  grep -q "/cSploit" /proc/mounts && umount /cSploit
  test -d /cSploit && { mount -o remount,rw /; rmdir /cSploit; mount -o remount,ro / ;}
  echo "$1" >&3
  exit 1
}

test -f ./issues || { ./known-issues > ./issues 2>&3 ; chmod 777 ./issues ;}

while read issue; do

  echo "issue #$issue found" >&3

  case $issue in
    1)
       if [ ! -f "/cSploit/cSploitd" ]; then
          mount -o remount,rw / 2>&3 || die1 "remount rw failed"
          test -d "/cSploit" || mkdir "/cSploit" 2>&3 || die1 "mkdir failed"
          mount -o bind "$script_dir" "/cSploit" 2>&3 || die1 "bind failed"
          mount -o remount,ro / 2>&3 || echo "remount ro failed" >&3
        fi
        cd /cSploit 2>&3 || die1 "chdir failed"
        ;;
     
   esac
done < ./issues

exec 3>&-

exec ./cSploitd
