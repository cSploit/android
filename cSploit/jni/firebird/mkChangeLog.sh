#!/bin/sh

PositiveOffset=15462	# will be correct for all branches after FB3
			# Other values are: 2.5:13820 2.1:9165 2.0:6144 1.5:1748

TempLog=temp.log
TmpFile=temp.build.num
WriteBuildNumFile="src/misc/writeBuildNum.sh"
HeaderFile="src/jrd/build_no.h"

cd /home/fbadmin/changelogs/trunk
svn up || exit
svn log -v >$TempLog || exit
smallog <$TempLog >ChangeLog || exit

VersionCount=`egrep -c '[0-9]{4}-[0-9]{2}-[0-9]{2} [0-9]{2}:[0-9]{2}' ChangeLog`
BuildNo=$((${VersionCount}+${PositiveOffset}))
Starting="BuildNum="
NewLine="BuildNum=$BuildNo"
AwkProgram="(/^$Starting.*/ || \$1 == \"$Starting\") {\$0=\"$NewLine\"} {print \$0}"
awk "$AwkProgram" <$WriteBuildNumFile >$TmpFile && mv $TmpFile $WriteBuildNumFile
chmod +x $WriteBuildNumFile

$WriteBuildNumFile rebuildHeader $HeaderFile $TmpFile
                        
svn commit -m "nightly update" ChangeLog $WriteBuildNumFile $HeaderFile
rm -f $TempLog $TmpFile
