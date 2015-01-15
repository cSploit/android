#!/system/bin/sh

exec 3<> ./start_daemon.log

script_dir="$(readlink /proc/$$/cwd )"

test -f ./issues || { ./known-issues > ./issues 2>&3 ; chmod 777 ./issues ;}

while read issue; do

  echo "issue #$issue found" >&3

  case issue in
    1)
       echo "issue #1 found, cSploit will not be started" >&2
       exit 1
       ;;
     
   esac
done < ./issues

exec 3>&-

exec ./cSploitd
