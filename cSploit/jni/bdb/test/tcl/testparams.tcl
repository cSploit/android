# See the file LICENSE for redistribution information.
#
# Copyright (c) 2000, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$

source ./include.tcl
global is_freebsd_test
global tcl_platform
global one_test
global serial_tests
set serial_tests {rep002 rep005 rep016 rep020 rep022 rep026 rep031 rep063 \
    rep078 rep079 rep096 rep097 rep106}
#set serial_tests {}

set subs {auto_repmgr bigfile dead env fail fop lock log memp multi_repmgr \
    mutex other_repmgr plat recd rep rsrc sdb sdbtest sec si test txn}

set test_names(bigfile)	[list bigfile001 bigfile002]
set test_names(compact) [list test111 test112 test113 test114 test115 test117 \
    test130 test148]
set test_names(dead)    [list dead001 dead002 dead003 dead004 dead005 dead006 \
    dead007 dead008 dead009 dead010 dead011]
set test_names(elect)	[list rep002 rep005 rep016 rep020 rep022 rep026 \
    rep063 rep067 rep069 rep076 rep093 rep094]
set test_names(env)	[list env001 env002 env003 env004 env005 env006 \
    env007 env008 env009 env010 env011 env012 env013 env014 env015 env016 \
    env017 env018 env019 env020 env021]
set test_names(fail)	[list fail001]
set test_names(fop)	[list fop001 fop002 fop003 fop004 fop005 fop006 \
    fop007 fop008 fop009 fop010 fop011 fop012]
set test_names(init)	[list rep029 rep030 rep031 rep033 rep037 rep038 rep039\
    rep055 rep060 rep061 rep062 rep070 rep072 rep084 rep085 rep086 rep087 \
    rep089 rep098 rep104]
set test_names(lock)    [list lock001 lock002 lock003 lock004 lock005 lock006]
set test_names(log)     [list log001 log002 log003 log004 log005 log006 \
    log007 log008 log009]
set test_names(memp)	[list memp001 memp002 memp003 memp004 memp005 memp006 \
    memp007]
set test_names(mutex)	[list mut001 mut002]
set test_names(plat)	[list plat001]
set test_names(recd)	[list recd001 recd002 recd003 recd004 recd005 recd006 \
    recd007 recd008 recd009 recd010 recd011 recd012 recd013 recd014 recd015 \
    recd016 recd017 recd018 recd019 recd020 recd022 recd023 recd024 recd025]
set test_names(rep)	[list rep001 rep002 rep003 rep005 rep006 rep007 \
    rep008 rep009 rep010 rep011 rep012 rep013 rep014 rep015 rep016 rep017 \
    rep018 rep019 rep020 rep021 rep022 rep023 rep024 rep025 rep026 rep027 \
    rep028 rep029 rep030 rep031 rep032 rep033 rep034 rep035 rep036 rep037 \
    rep038 rep039 rep040 rep041 rep042 rep043 rep044 rep045 rep046 rep047 \
    rep048 rep049 rep050 rep051 rep052 rep053 rep054 rep055 \
    rep058 rep060 rep061 rep062 rep063 rep064 rep065 rep066 rep067 \
    rep068 rep069 rep070 rep071 rep072 rep073 rep074 rep075 rep076 rep077 \
    rep078 rep079 rep080 rep081 rep082 rep083 rep084 rep085 rep086 rep087 \
    rep088 rep089 rep090 rep091 rep092 rep093 rep094 rep095 rep096 rep097 \
    rep098 rep099 rep100 rep101 rep102 rep103 rep104 rep105 rep106 rep107 \
    rep108 rep109 rep110 rep111 rep112 rep113 rep115]
set test_names(skip_for_env_private) [list rep002 rep003 rep004 rep005 \
    rep014 rep016 rep017 rep018 rep020 rep022 rep026 rep028 rep031 \
    rep033 rep035 rep036 rep038 rep039 rep040 rep041 rep042 rep043 rep044 \
    rep045 rep048 rep054 rep055 rep056 rep057 rep059 rep060 rep061 rep063 \
    rep065 rep066 rep067 rep068 rep069 rep070 rep072 rep076 rep078 \
    rep079 rep081 rep082 rep083 rep088 rep095 rep096 rep098 rep100 \
    rep106 rep110 ]
set test_names(skip_for_inmem_db) [list rep002 rep003 rep004 rep008 rep009 \
    rep011 rep015 rep017 rep018 rep027 rep036 rep042 rep043 rep056 rep057 \
    rep058 rep059 rep065 rep068 rep078 rep079 rep081 rep082 rep083 rep084 \
    rep085 rep086 rep087 rep088 rep090 rep099 rep100 rep103 rep104 rep106 \
    rep108 rep111 rep112 rep113]
set test_names(skip_for_inmem_rep) [list rep089] 
set test_names(auto_repmgr) [list repmgr001 repmgr002 repmgr003 ]
set test_names(basic_repmgr) [list basic_repmgr_test \
    basic_repmgr_election_test basic_repmgr_init_test ]
set test_names(multi_repmgr) [list repmgr100 repmgr101 repmgr102 \
    repmgr105 repmgr106 repmgr107 repmgr108 repmgr109 \
    repmgr110 repmgr111 repmgr112 repmgr113 repmgr150]
set test_names(other_repmgr) [list repmgr004 repmgr007 repmgr009 repmgr010 \
    repmgr011 repmgr012 repmgr013 repmgr017 repmgr018 repmgr023 \
    repmgr025 repmgr026 repmgr027 repmgr028 repmgr029 repmgr030 repmgr031 \
    repmgr032 repmgr033 repmgr034 repmgr035 repmgr036 repmgr037 repmgr038]
set test_names(rsrc)	[list rsrc001 rsrc002 rsrc003 rsrc004]
set test_names(sdb)	[list sdb001 sdb002 sdb003 sdb004 sdb005 sdb006 \
    sdb007 sdb008 sdb009 sdb010 sdb011 sdb012 sdb013 sdb014 sdb015 sdb016 \
    sdb017 sdb018 sdb019 sdb020 ]
set test_names(sdbtest)	[list sdbtest001 sdbtest002]
set test_names(sec)	[list sec001 sec002]
set test_names(si)	[list si001 si002 si003 si004 si005 si006 si007 si008]
set test_names(test)	[list test001 test002 test003 test004 test005 \
    test006 test007 test008 test009 test010 test011 test012 test013 test014 \
    test015 test016 test017 test018 test019 test020 test021 test022 test023 \
    test024 test025 test026 test027 test028 test029 test030 test031 test032 \
    test033 test034 test035 test036 test037 test038 test039 test040 test041 \
    test043 test044 test045 test046 test047 test048 test049 test050 \
    test051 test052 test053 test054 test055 test056 test057 test058 test059 \
    test060 test061 test062 test063 test064 test065 test066 test067 test068 \
    test069 test070 test071 test072 test073 test074 test076 test077 \
    test078 test079 test081 test082 test083 test084 test085 test086 \
    test087 test088 test089 test090 test091 test092 test093 test094 test095 \
    test096 test097 test098 test099 test100 test101 test102 test103 test107 \
    test109 test110 test111 test112 test113 test114 test115 test116 test117 \
    test119 test120 test121 test122 test123 test124 test125 test126 test127 \
    test128 test129 test130 test131 test132 test133 test134 test135 test136 \
    test137 test138 test139 test140 test141 test142 test143 test144 test145 \
    test146 test147 test148 test149]

set test_names(txn)	[list txn001 txn002 txn003 txn004 txn005 txn006 \
    txn007 txn008 txn009 txn010 txn011 txn012 txn013 txn014]

# Set up a list of rep tests to run before committing changes to the
# replication system.  By default, all tests in test_names(rep) are
# included.  To skip a test, add it to the 'skip_for_rep_commit' variable.
set skip_for_rep_commit [list rep005 rep020 rep022 rep026 rep063 rep065 \
    rep069 rep076]
set test_names(rep_commit) $test_names(rep)
foreach test $skip_for_rep_commit {
	set idx [lsearch -exact $test_names(rep_commit) $test]
	if { $idx >= 0 } {
		set test_names(rep_commit) [lreplace \
		    $test_names(rep_commit) $idx $idx]
	}
}

 # Source all the tests, whether we're running one or many.
foreach sub $subs {
	foreach test $test_names($sub) {
		source $test_path/$test.tcl
	}
}

# Reset test_names if we're running only one test.
if { $one_test != "ALL" } {
	foreach sub $subs {
		set test_names($sub) ""
	}
	set type [string trim $one_test 0123456789]
	set test_names($type) [list $one_test]
}

source $test_path/archive.tcl
source $test_path/backup.tcl
source $test_path/byteorder.tcl
source $test_path/dbm.tcl
source $test_path/foputils.tcl
source $test_path/hsearch.tcl
source $test_path/join.tcl
source $test_path/logtrack.tcl
source $test_path/ndbm.tcl
source $test_path/parallel.tcl
source $test_path/portable.tcl
source $test_path/reputils.tcl
source $test_path/reputilsnoenv.tcl
source $test_path/sdbutils.tcl
source $test_path/shelltest.tcl
source $test_path/sijointest.tcl
source $test_path/siutils.tcl
source $test_path/testutils.tcl
source $test_path/upgrade.tcl
source $test_path/../tcl_utils/multi_proc_utils.tcl
source $test_path/../tcl_utils/common_test_utils.tcl

set parms(recd001) 0
set parms(recd002) 0
set parms(recd003) 0
set parms(recd004) 0
set parms(recd005) ""
set parms(recd006) 0
set parms(recd007) ""
set parms(recd008) {4 4}
set parms(recd009) 0
set parms(recd010) 0
set parms(recd011) {200 15 1}
set parms(recd012) {0 49 25 100 5}
set parms(recd013) 100
set parms(recd014) ""
set parms(recd015) ""
set parms(recd016) ""
set parms(recd017) 0
set parms(recd018) 10
set parms(recd019) 50
set parms(recd020) ""
set parms(recd022) ""
set parms(recd023) ""
set parms(recd024) ""
set parms(recd025) ""
set parms(rep001) {1000 "001"}
set parms(rep002) {10 3 "002"}
set parms(rep003) "003"
set parms(rep005) ""
set parms(rep006) {1000 "006"}
set parms(rep007) {10 "007"}
set parms(rep008) {10 "008"}
set parms(rep009) {10 "009"}
set parms(rep010) {100 "010"}
set parms(rep011) "011"
set parms(rep012) {10 "012"}
set parms(rep013) {10 "013"}
set parms(rep014) {10 "014"}
set parms(rep015) {100 "015" 3}
set parms(rep016) ""
set parms(rep017) {10 "017"}
set parms(rep018) {10 "018"}
set parms(rep019) {3 "019"}
set parms(rep020) ""
set parms(rep021) {3 "021"}
set parms(rep022) ""
set parms(rep023) {10 "023"}
set parms(rep024) {1000 "024"}
set parms(rep025) {200 "025"}
set parms(rep026) ""
set parms(rep027) {1000 "027"}
set parms(rep028) {100 "028"}
set parms(rep029) {200 "029"}
set parms(rep030) {500 "030"}
set parms(rep031) {200 "031"}
set parms(rep032) {200 "032"}
set parms(rep033) {200 "033"}
set parms(rep034) {2 "034"}
set parms(rep035) {100 "035"}
set parms(rep036) {200 "036"}
set parms(rep037) {1500 "037"}
set parms(rep038) {200 "038"}
set parms(rep039) {200 "039"}
set parms(rep040) {200 "040"}
set parms(rep041) {500 "041"}
set parms(rep042) {10 "042"}
set parms(rep043) {25 "043"}
set parms(rep044) {"044"}
set parms(rep045) {"045"}
set parms(rep046) {200 "046"}
set parms(rep047) {200 "047"}
set parms(rep048) {3000 "048"}
set parms(rep049) {10 "049"}
set parms(rep050) {10 "050"}
set parms(rep051) {1000 "051"}
set parms(rep052) {200 "052"}
set parms(rep053) {200 "053"}
set parms(rep054) {200 "054"}
set parms(rep055) {200 "055"}
set parms(rep058) "058"
set parms(rep060) {200 "060"}
set parms(rep061) {500 "061"}
set parms(rep062) "062"
set parms(rep063) ""
set parms(rep064) {10 "064"}
set parms(rep065) {3}
set parms(rep066) {10 "066"}
set parms(rep067) ""
set parms(rep068) {"068"}
set parms(rep069) {200 "069"}
set parms(rep070) {200 "070"}
set parms(rep071) { 10 "071"}
set parms(rep072) {200 "072"}
set parms(rep073) {200 "073"}
set parms(rep074) {"074"}
set parms(rep075) {"075"}
set parms(rep076) ""
set parms(rep077) {"077"}
set parms(rep078) {"078"}
set parms(rep079) {"079"}
set parms(rep080) {200 "080"}
set parms(rep081) {200 "081"}
set parms(rep082) {200 "082"}
set parms(rep083) {200 "083"}
set parms(rep084) {200 "084"}
set parms(rep085) {20 "085"}
set parms(rep086) {"086"}
set parms(rep087) {200 "087"}
set parms(rep088) {20 "088"}
set parms(rep089) {200 "089"}
set parms(rep090) {50 "090"}
set parms(rep091) {20 "091"}
set parms(rep092) {20 "092"}
set parms(rep093) {20 "093"}
set parms(rep094) {"094"}
set parms(rep095) {200 "095"}
set parms(rep096) {20 "096"}
set parms(rep097) {"097"}
set parms(rep098) {200 "098"}
set parms(rep099) {200 "099"}
set parms(rep100) {10 "100"}
set parms(rep101) {100 "101"}
set parms(rep102) {100 "102"}
set parms(rep103) {200 "103"}
set parms(rep104) {10 "104"}
set parms(rep105) {10 "105"}
set parms(rep106) {"106"}
set parms(rep107) {"107"}
set parms(rep108) {500 "108"}
set parms(rep109) {"109"}
set parms(rep110) {200 "110"}
set parms(rep111) {100 "111"}
set parms(rep112) {100 "112"}
set parms(rep113) {100 "113"}
set parms(rep115) {20 "115"}
set parms(repmgr007) {100 "007"}
set parms(repmgr009) {10 "009"}
set parms(repmgr010) {100 "010"}
set parms(repmgr011) {100 "011"}
set parms(repmgr012) {100 "012"}
set parms(repmgr013) {100 "013"}
set parms(repmgr017) {1000 "017"}
set parms(repmgr018) {100 "018"}
set parms(repmgr023) {50 "023"}
set parms(repmgr024) {50 "024"}
set parms(repmgr025) {100 "025"}
set parms(repmgr026) {"026"}
set parms(repmgr027) {"027"}
set parms(repmgr028) {"028"}
set parms(repmgr030) {100 "030"}
set parms(repmgr031) {1 "031"}
set parms(repmgr032) {"032"}
set parms(repmgr034) {3 "034"}
set parms(repmgr035) {3 "035"}
set parms(repmgr036) {100 "036"}
set parms(repmgr037) {100 "037"}
set parms(repmgr038) {100 "038"}
set parms(repmgr100) ""
set parms(repmgr101) ""
set parms(repmgr102) ""
set parms(repmgr105) ""
set parms(repmgr106) ""
set parms(repmgr107) ""
set parms(repmgr108) ""
set parms(repmgr109) ""
set parms(repmgr110) ""
set parms(repmgr111) ""
set parms(repmgr112) ""
set parms(repmgr113) ""
set parms(repmgr150) ""
set parms(subdb001) ""
set parms(subdb002) 10000
set parms(subdb003) 1000
set parms(subdb004) ""
set parms(subdb005) 100
set parms(subdb006) 100
set parms(subdb007) ""
set parms(subdb008) ""
set parms(subdb009) ""
set parms(subdb010) ""
set parms(subdb011) {13 10}
set parms(subdb012) ""
set parms(sdb001) ""
set parms(sdb002) 10000
set parms(sdb003) 1000
set parms(sdb004) ""
set parms(sdb005) 100
set parms(sdb006) 100
set parms(sdb007) ""
set parms(sdb008) ""
set parms(sdb009) ""
set parms(sdb010) ""
set parms(sdb011) {13 10}
set parms(sdb012) ""
set parms(sdb013) 10
set parms(sdb014) ""
set parms(sdb015) 1000
set parms(sdb016) 100
set parms(sdb017) ""
set parms(sdb018) 100
set parms(sdb019) 100
set parms(sdb020) 10
set parms(si001) {200 "001"}
set parms(si002) {200 "002"}
set parms(si003) {200 "003"}
set parms(si004) {200 "004"}
set parms(si005) {200 "005"}
set parms(si006) {200 "006"}
set parms(si007) {10 "007"}
set parms(si008) {10 "008"}
set parms(test001) {10000 0 0 "001"}
set parms(test002) 10000
set parms(test003) ""
set parms(test004) {10000 "004" 0}
set parms(test005) 10000
set parms(test006) {10000 0 "006" 5}
set parms(test007) {10000 "007" 5}
set parms(test008) {"008" 0}
set parms(test009) ""
set parms(test010) {10000 5 "010"}
set parms(test011) {10000 5 "011"}
set parms(test012)  ""
set parms(test013) 10000
set parms(test014) 10000
set parms(test015) {7500 0}
set parms(test016) 10000
set parms(test017) {0 19 "017"}
set parms(test018) 10000
set parms(test019) 10000
set parms(test020) 10000
set parms(test021) 10000
set parms(test022) ""
set parms(test023) ""
set parms(test024) 10000
set parms(test025) {10000 0 "025"}
set parms(test026) {2000 5 "026"}
set parms(test027) {100}
set parms(test028) ""
set parms(test029) 10000
set parms(test030) 10000
set parms(test031) {10000 5 "031"}
set parms(test032) {10000 5 "032" 0}
set parms(test033) {10000 5 "033" 0}
set parms(test034) 10000
set parms(test035) 10000
set parms(test036) 10000
set parms(test037) 100
set parms(test038) {10000 5 "038"}
set parms(test039) {10000 5 "039"}
set parms(test040) 10000
set parms(test041) 10000
set parms(test042) 1000
set parms(test043) 10000
set parms(test044) {5 10 0}
set parms(test045) 1000
set parms(test046) ""
set parms(test047) ""
set parms(test048) ""
set parms(test049) ""
set parms(test050) ""
set parms(test051) ""
set parms(test052) ""
set parms(test053) ""
set parms(test054) ""
set parms(test055) ""
set parms(test056) ""
set parms(test057) ""
set parms(test058) ""
set parms(test059) ""
set parms(test060) ""
set parms(test061) ""
set parms(test062) {200 200 "062"}
set parms(test063) ""
set parms(test064) ""
set parms(test065) ""
set parms(test066) ""
set parms(test067) {1000 "067"}
set parms(test068) ""
set parms(test069) {50 "069"}
set parms(test070) {4 2 1000 CONSUME 0 -txn "070"}
set parms(test071) {1 1 10000 CONSUME 0 -txn "071"}
set parms(test072) {512 20 "072"}
set parms(test073) {512 50 "073"}
set parms(test074) {-nextnodup 100 "074"}
set parms(test076) {1000 "076"}
set parms(test077) {1000 "077"}
set parms(test078) {100 512 "078"}
set parms(test079) {10000 512 "079" 20}
set parms(test081) {13 "081"}
set parms(test082) {-prevnodup 100 "082"}
set parms(test083) {512 5000 2}
set parms(test084) {10000 "084" 65536}
set parms(test085) {512 3 10 "085"}
set parms(test086) ""
set parms(test087) {512 50 "087"}
set parms(test088) ""
set parms(test089) 1000
set parms(test090) {10000 "090"}
set parms(test091) {4 2 1000 0 "091"}
set parms(test092) {1000}
set parms(test093) {10000 "093"}
set parms(test094) {10000 10 "094"}
set parms(test095) {"095"}
set parms(test096) {512 1000 19}
set parms(test097) {500 400}
set parms(test098) ""
set parms(test099) 10000
set parms(test100) {10000 "100"}
set parms(test101) {1000 -txn "101"}
set parms(test102) {1000 "102"}
set parms(test103) {100 4294967250 "103"}
set parms(test107) ""
set parms(test109) {"109"}
set parms(test110) {10000 3}
set parms(test111) {10000 "111"}
set parms(test112) {80000 "112"}
set parms(test113) {10000 5 "113"}
set parms(test114) {10000 "114"}
set parms(test115) {10000 "115"}
set parms(test116) {"116"}
set parms(test117) {10000 "117"}
set parms(test119) {"119"}
set parms(test120) {"120"}
set parms(test121) {"121"}
set parms(test122) {"122"}
set parms(test123) ""
set parms(test124) 1000 
set parms(test125) ""
set parms(test126) {10000 "126" 1 0 0 0}
set parms(test127) {10000 5 "127" 0 0}
set parms(test128) {10000 1}
set parms(test129) {10000 5}
set parms(test130) {10000 3 "130"}
set parms(test131) {1000 "131" 5 0 0}
set parms(test132) {1000 5}
set parms(test133) {1000 "133" 0}
set parms(test134) {1000}
set parms(test135) {10 10 0 "135"}
set parms(test136) {10 10}
set parms(test137) {1000 0 0 0 "137" "ds"}
set parms(test138) {1000 0 0}
set parms(test139) {512 1000 "139"}
set parms(test140) {"140"}
set parms(test141) {10000 "141"}
set parms(test142) {"142"}
set parms(test143) {"143"}
set parms(test144) {"144"}
set parms(test145) {"145"}
set parms(test146) {"146"}
set parms(test147) {"147"}
set parms(test148) {10000 "148"}
set parms(test149) {"149"}

# Shell script tests.  Each list entry is a {directory filename rundir} list,
# invoked with "/bin/sh filename".
set shelltest_list {
	{ c		chk.ctests  "" }
	{ cxx		chk.cxxtests .. }
	{ java/junit	chk.bdb junit }
	{ java/compat	chk.bdb compat }
	{ sql_codegen	chk.bdb "" }
	{ xa	        chk.xa "" }
}
