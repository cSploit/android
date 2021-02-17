#! /bin/sh
# $OpenLDAP$
## This work is part of OpenLDAP Software <http://www.openldap.org/>.
##
## Copyright 1998-2014 The OpenLDAP Foundation.
## All rights reserved.
##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted only as authorized by the OpenLDAP
## Public License.
##
## A copy of this license is available in the file LICENSE in the
## top-level directory of the distribution or, alternatively, at
## <http://www.OpenLDAP.org/license.html>.

umask 077

TESTWD=`pwd`

# backends
MONITORDB=${AC_monitor-no}
BACKLDAP=${AC_ldap-ldapno}
BACKMETA=${AC_meta-metano}
BACKRELAY=${AC_relay-relayno}
BACKSQL=${AC_sql-sqlno}
	RDBMS=${SLAPD_USE_SQL-rdbmsno}
	RDBMSWRITE=${SLAPD_USE_SQLWRITE-no}

# overlays
ACCESSLOG=${AC_accesslog-accesslogno}
CONSTRAINT=${AC_constraint-constraintno}
DDS=${AC_dds-ddsno}
DYNLIST=${AC_dynlist-dynlistno}
MEMBEROF=${AC_memberof-memberofno}
PROXYCACHE=${AC_pcache-pcacheno}
PPOLICY=${AC_ppolicy-ppolicyno}
REFINT=${AC_refint-refintno}
RETCODE=${AC_retcode-retcodeno}
RWM=${AC_rwm-rwmno}
SYNCPROV=${AC_syncprov-syncprovno}
TRANSLUCENT=${AC_translucent-translucentno}
UNIQUE=${AC_unique-uniqueno}
VALSORT=${AC_valsort-valsortno}

# misc
WITH_SASL=${AC_WITH_SASL-no}
USE_SASL=${SLAPD_USE_SASL-no}
ACI=${AC_ACI_ENABLED-acino}
THREADS=${AC_THREADS-threadsno}
SLEEP0=${SLEEP0-1}
SLEEP1=${SLEEP1-7}
SLEEP2=${SLEEP2-15}

# dirs
PROGDIR=./progs
DATADIR=${USER_DATADIR-./testdata}
TESTDIR=${USER_TESTDIR-$TESTWD/testrun}
SCHEMADIR=${USER_SCHEMADIR-./schema}
case "$SCHEMADIR" in
.*)	ABS_SCHEMADIR="$TESTWD/$SCHEMADIR" ;;
*)  ABS_SCHEMADIR="$SCHEMADIR" ;;
esac

DBDIR1A=$TESTDIR/db.1.a
DBDIR1B=$TESTDIR/db.1.b
DBDIR1C=$TESTDIR/db.1.c
DBDIR1=$DBDIR1A
DBDIR2A=$TESTDIR/db.2.a
DBDIR2B=$TESTDIR/db.2.b
DBDIR2C=$TESTDIR/db.2.c
DBDIR2=$DBDIR2A
DBDIR3=$TESTDIR/db.3.a
DBDIR4=$TESTDIR/db.4.a
DBDIR5=$TESTDIR/db.5.a
DBDIR6=$TESTDIR/db.6.a
SQLCONCURRENCYDIR=$DATADIR/sql-concurrency

CLIENTDIR=../clients/tools
#CLIENTDIR=/usr/local/bin

# conf
CONF=$DATADIR/slapd.conf
CONFTWO=$DATADIR/slapd2.conf
CONF2DB=$DATADIR/slapd-2db.conf
MCONF=$DATADIR/slapd-master.conf
COMPCONF=$DATADIR/slapd-component.conf
PWCONF=$DATADIR/slapd-pw.conf
WHOAMICONF=$DATADIR/slapd-whoami.conf
ACLCONF=$DATADIR/slapd-acl.conf
RCONF=$DATADIR/slapd-referrals.conf
SRMASTERCONF=$DATADIR/slapd-syncrepl-master.conf
DSRMASTERCONF=$DATADIR/slapd-deltasync-master.conf
DSRSLAVECONF=$DATADIR/slapd-deltasync-slave.conf
PPOLICYCONF=$DATADIR/slapd-ppolicy.conf
PROXYCACHECONF=$DATADIR/slapd-proxycache.conf
CACHEMASTERCONF=$DATADIR/slapd-cache-master.conf
R1SRSLAVECONF=$DATADIR/slapd-syncrepl-slave-refresh1.conf
R2SRSLAVECONF=$DATADIR/slapd-syncrepl-slave-refresh2.conf
P1SRSLAVECONF=$DATADIR/slapd-syncrepl-slave-persist1.conf
P2SRSLAVECONF=$DATADIR/slapd-syncrepl-slave-persist2.conf
P3SRSLAVECONF=$DATADIR/slapd-syncrepl-slave-persist3.conf
REFSLAVECONF=$DATADIR/slapd-ref-slave.conf
SCHEMACONF=$DATADIR/slapd-schema.conf
GLUECONF=$DATADIR/slapd-glue.conf
REFINTCONF=$DATADIR/slapd-refint.conf
RETCODECONF=$DATADIR/slapd-retcode.conf
UNIQUECONF=$DATADIR/slapd-unique.conf
LIMITSCONF=$DATADIR/slapd-limits.conf
DNCONF=$DATADIR/slapd-dn.conf
EMPTYDNCONF=$DATADIR/slapd-emptydn.conf
IDASSERTCONF=$DATADIR/slapd-idassert.conf
LDAPGLUECONF1=$DATADIR/slapd-ldapglue.conf
LDAPGLUECONF2=$DATADIR/slapd-ldapgluepeople.conf
LDAPGLUECONF3=$DATADIR/slapd-ldapgluegroups.conf
RELAYCONF=$DATADIR/slapd-relay.conf
CHAINCONF1=$DATADIR/slapd-chain1.conf
CHAINCONF2=$DATADIR/slapd-chain2.conf
GLUESYNCCONF1=$DATADIR/slapd-glue-syncrepl1.conf
GLUESYNCCONF2=$DATADIR/slapd-glue-syncrepl2.conf
SQLCONF=$DATADIR/slapd-sql.conf
SQLSRMASTERCONF=$DATADIR/slapd-sql-syncrepl-master.conf
TRANSLUCENTLOCALCONF=$DATADIR/slapd-translucent-local.conf
TRANSLUCENTREMOTECONF=$DATADIR/slapd-translucent-remote.conf
METACONF=$DATADIR/slapd-meta.conf
METACONF1=$DATADIR/slapd-meta-target1.conf
METACONF2=$DATADIR/slapd-meta-target2.conf
GLUELDAPCONF=$DATADIR/slapd-glue-ldap.conf
ACICONF=$DATADIR/slapd-aci.conf
VALSORTCONF=$DATADIR/slapd-valsort.conf
DYNLISTCONF=$DATADIR/slapd-dynlist.conf
RSLAVECONF=$DATADIR/slapd-repl-slave-remote.conf
PLSRSLAVECONF=$DATADIR/slapd-syncrepl-slave-persist-ldap.conf
PLSRMASTERCONF=$DATADIR/slapd-syncrepl-multiproxy.conf
DDSCONF=$DATADIR/slapd-dds.conf
PASSWDCONF=$DATADIR/slapd-passwd.conf
UNDOCONF=$DATADIR/slapd-config-undo.conf
NAKEDCONF=$DATADIR/slapd-config-naked.conf
VALREGEXCONF=$DATADIR/slapd-valregex.conf

DYNAMICCONF=$DATADIR/slapd-dynamic.ldif

# generated files
CONF1=$TESTDIR/slapd.1.conf
CONF2=$TESTDIR/slapd.2.conf
CONF3=$TESTDIR/slapd.3.conf
CONF4=$TESTDIR/slapd.4.conf
CONF5=$TESTDIR/slapd.5.conf
CONF6=$TESTDIR/slapd.6.conf
ADDCONF=$TESTDIR/slapadd.conf
CONFLDIF=$TESTDIR/slapd-dynamic.ldif

LOG1=$TESTDIR/slapd.1.log
LOG2=$TESTDIR/slapd.2.log
LOG3=$TESTDIR/slapd.3.log
LOG4=$TESTDIR/slapd.4.log
LOG5=$TESTDIR/slapd.5.log
LOG6=$TESTDIR/slapd.6.log
SLAPADDLOG1=$TESTDIR/slapadd.1.log
SLURPLOG=$TESTDIR/slurp.log

CONFIGPWF=$TESTDIR/configpw

# args
TOOLARGS="-x $LDAP_TOOLARGS"
TOOLPROTO="-P 3"

# cmds
CONFFILTER=$SRCDIR/scripts/conf.sh

MONITORDATA=$SRCDIR/scripts/monitor_data.sh

SLAPADD="$TESTWD/../servers/slapd/slapd -Ta -d 0 $LDAP_VERBOSE"
SLAPCAT="$TESTWD/../servers/slapd/slapd -Tc -d 0 $LDAP_VERBOSE"
SLAPINDEX="$TESTWD/../servers/slapd/slapd -Ti -d 0 $LDAP_VERBOSE"
SLAPPASSWD="$TESTWD/../servers/slapd/slapd -Tpasswd"

unset DIFF_OPTIONS
# NOTE: -u/-c is not that portable...
DIFF="diff -i"
CMP="diff -i"
BCMP="diff -iB"
CMPOUT=/dev/null
SLAPD="$TESTWD/../servers/slapd/slapd -s0"
LDAPPASSWD="$CLIENTDIR/ldappasswd $TOOLARGS"
LDAPSASLSEARCH="$CLIENTDIR/ldapsearch $TOOLPROTO $LDAP_TOOLARGS -LLL"
LDAPSEARCH="$CLIENTDIR/ldapsearch $TOOLPROTO $TOOLARGS -LLL"
LDAPRSEARCH="$CLIENTDIR/ldapsearch $TOOLPROTO $TOOLARGS"
LDAPDELETE="$CLIENTDIR/ldapdelete $TOOLPROTO $TOOLARGS"
LDAPMODIFY="$CLIENTDIR/ldapmodify $TOOLPROTO $TOOLARGS"
LDAPADD="$CLIENTDIR/ldapmodify -a $TOOLPROTO $TOOLARGS"
LDAPMODRDN="$CLIENTDIR/ldapmodrdn $TOOLPROTO $TOOLARGS"
LDAPWHOAMI="$CLIENTDIR/ldapwhoami $TOOLARGS"
LDAPCOMPARE="$CLIENTDIR/ldapcompare $TOOLARGS"
LDAPEXOP="$CLIENTDIR/ldapexop $TOOLARGS"
SLAPDTESTER=$PROGDIR/slapd-tester
LDIFFILTER=$PROGDIR/ldif-filter
SLAPDMTREAD=$PROGDIR/slapd-mtread
LVL=${SLAPD_DEBUG-0x4105}
LOCALHOST=localhost
BASEPORT=${SLAPD_BASEPORT-9010}
PORT1=`expr $BASEPORT + 1`
PORT2=`expr $BASEPORT + 2`
PORT3=`expr $BASEPORT + 3`
PORT4=`expr $BASEPORT + 4`
PORT5=`expr $BASEPORT + 5`
PORT6=`expr $BASEPORT + 6`
URI1="ldap://${LOCALHOST}:$PORT1/"
URI2="ldap://${LOCALHOST}:$PORT2/"
URI3="ldap://${LOCALHOST}:$PORT3/"
URI4="ldap://${LOCALHOST}:$PORT4/"
URI5="ldap://${LOCALHOST}:$PORT5/"
URI6="ldap://${LOCALHOST}:$PORT6/"

# LDIF
LDIF=$DATADIR/test.ldif
LDIFADD1=$DATADIR/do_add.1
LDIFGLUED=$DATADIR/test-glued.ldif
LDIFORDERED=$DATADIR/test-ordered.ldif
LDIFORDEREDCP=$DATADIR/test-ordered-cp.ldif
LDIFORDEREDNOCP=$DATADIR/test-ordered-nocp.ldif
LDIFBASE=$DATADIR/test-base.ldif
LDIFPASSWD=$DATADIR/passwd.ldif
LDIFWHOAMI=$DATADIR/test-whoami.ldif
LDIFPASSWDOUT=$DATADIR/passwd-out.ldif
LDIFPPOLICY=$DATADIR/ppolicy.ldif
LDIFLANG=$DATADIR/test-lang.ldif
LDIFLANGOUT=$DATADIR/lang-out.ldif
LDIFREF=$DATADIR/referrals.ldif
LDIFREFINT=$DATADIR/test-refint.ldif
LDIFUNIQUE=$DATADIR/test-unique.ldif
LDIFLIMITS=$DATADIR/test-limits.ldif
LDIFDN=$DATADIR/test-dn.ldif
LDIFEMPTYDN1=$DATADIR/test-emptydn1.ldif
LDIFEMPTYDN2=$DATADIR/test-emptydn2.ldif
LDIFIDASSERT1=$DATADIR/test-idassert1.ldif
LDIFIDASSERT2=$DATADIR/test-idassert2.ldif
LDIFLDAPGLUE1=$DATADIR/test-ldapglue.ldif
LDIFLDAPGLUE2=$DATADIR/test-ldapgluepeople.ldif
LDIFLDAPGLUE3=$DATADIR/test-ldapgluegroups.ldif
LDIFCOMPMATCH=$DATADIR/test-compmatch.ldif
LDIFCHAIN1=$DATADIR/test-chain1.ldif
LDIFCHAIN2=$DATADIR/test-chain2.ldif
LDIFTRANSLUCENTDATA=$DATADIR/test-translucent-data.ldif
LDIFTRANSLUCENTCONFIG=$DATADIR/test-translucent-config.ldif
LDIFTRANSLUCENTADD=$DATADIR/test-translucent-add.ldif
LDIFTRANSLUCENTMERGED=$DATADIR/test-translucent-merged.ldif
LDIFMETA=$DATADIR/test-meta.ldif
LDIFVALSORT=$DATADIR/test-valsort.ldif
SQLADD=$DATADIR/sql-add.ldif
LDIFUNORDERED=$DATADIR/test-unordered.ldif
LDIFREORDERED=$DATADIR/test-reordered.ldif

# strings
MONITOR=""
REFDN="c=US"
BASEDN="dc=example,dc=com"
MANAGERDN="cn=Manager,$BASEDN"
UPDATEDN="cn=Replica,$BASEDN"
PASSWD=secret
BABSDN="cn=Barbara Jensen,ou=Information Technology DivisioN,ou=People,$BASEDN"
BJORNSDN="cn=Bjorn Jensen,ou=Information Technology DivisioN,ou=People,$BASEDN"
JAJDN="cn=James A Jones 1,ou=Alumni Association,ou=People,$BASEDN"
JOHNDDN="cn=John Doe,ou=Information Technology Division,ou=People,$BASEDN"
MELLIOTDN="cn=Mark Elliot,ou=Alumni Association,ou=People,$BASEDN"
REFINTDN="cn=Manager,o=refint"
RETCODEDN="ou=RetCodes,$BASEDN"
UNIQUEDN="cn=Manager,o=unique"
EMPTYDNDN="cn=Manager,c=US"
TRANSLUCENTROOT="o=translucent"
TRANSLUCENTUSER="ou=users,o=translucent"
TRANSLUCENTDN="uid=binder,o=translucent"
TRANSLUCENTPASSWD="bindtest"
METABASEDN="ou=Meta,$BASEDN"
METAMANAGERDN="cn=Manager,$METABASEDN"
VALSORTDN="cn=Manager,o=valsort"
VALSORTBASEDN="o=valsort"
MONITORDN="cn=Monitor"
OPERATIONSMONITORDN="cn=Operations,$MONITORDN"
CONNECTIONSMONITORDN="cn=Connections,$MONITORDN"
DATABASESMONITORDN="cn=Databases,$MONITORDN"
STATISTICSMONITORDN="cn=Statistics,$MONITORDN"

# generated outputs
SEARCHOUT=$TESTDIR/ldapsearch.out
SEARCHOUT2=$TESTDIR/ldapsearch2.out
SEARCHFLT=$TESTDIR/ldapsearch.flt
SEARCHFLT2=$TESTDIR/ldapsearch2.flt
LDIFFLT=$TESTDIR/ldif.flt
TESTOUT=$TESTDIR/test.out
INITOUT=$TESTDIR/init.out
VALSORTOUT1=$DATADIR/valsort1.out
VALSORTOUT2=$DATADIR/valsort2.out
VALSORTOUT3=$DATADIR/valsort3.out
MONITOROUT1=$DATADIR/monitor1.out
MONITOROUT2=$DATADIR/monitor2.out
MONITOROUT3=$DATADIR/monitor3.out
MONITOROUT4=$DATADIR/monitor4.out

SERVER1OUT=$TESTDIR/server1.out
SERVER1FLT=$TESTDIR/server1.flt
SERVER2OUT=$TESTDIR/server2.out
SERVER2FLT=$TESTDIR/server2.flt
SERVER3OUT=$TESTDIR/server3.out
SERVER3FLT=$TESTDIR/server3.flt
SERVER4OUT=$TESTDIR/server4.out
SERVER4FLT=$TESTDIR/server4.flt
SERVER5OUT=$TESTDIR/server5.out
SERVER5FLT=$TESTDIR/server5.flt
SERVER6OUT=$TESTDIR/server6.out
SERVER6FLT=$TESTDIR/server6.flt

MASTEROUT=$SERVER1OUT
MASTERFLT=$SERVER1FLT
SLAVEOUT=$SERVER2OUT
SLAVE2OUT=$SERVER3OUT
SLAVEFLT=$SERVER2FLT
SLAVE2FLT=$SERVER3FLT

MTREADOUT=$TESTDIR/mtread.out

# original outputs for cmp
PROXYCACHEOUT=$DATADIR/proxycache.out
REFERRALOUT=$DATADIR/referrals.out
SEARCHOUTMASTER=$DATADIR/search.out.master
SEARCHOUTX=$DATADIR/search.out.xsearch
COMPSEARCHOUT=$DATADIR/compsearch.out
MODIFYOUTMASTER=$DATADIR/modify.out.master
ADDDELOUTMASTER=$DATADIR/adddel.out.master
MODRDNOUTMASTER0=$DATADIR/modrdn.out.master.0
MODRDNOUTMASTER1=$DATADIR/modrdn.out.master.1
MODRDNOUTMASTER2=$DATADIR/modrdn.out.master.2
MODRDNOUTMASTER3=$DATADIR/modrdn.out.master.3
ACLOUTMASTER=$DATADIR/acl.out.master
REPLOUTMASTER=$DATADIR/repl.out.master
MODSRCHFILTERS=$DATADIR/modify.search.filters
CERTIFICATETLS=$DATADIR/certificate.tls
CERTIFICATEOUT=$DATADIR/certificate.out
DNOUT=$DATADIR/dn.out
EMPTYDNOUT1=$DATADIR/emptydn.out.slapadd
EMPTYDNOUT2=$DATADIR/emptydn.out
IDASSERTOUT=$DATADIR/idassert.out
LDAPGLUEOUT=$DATADIR/ldapglue.out
LDAPGLUEANONYMOUSOUT=$DATADIR/ldapglueanonymous.out
RELAYOUT=$DATADIR/relay.out
CHAINOUT=$DATADIR/chain.out
CHAINREFOUT=$DATADIR/chainref.out
CHAINMODOUT=$DATADIR/chainmod.out
GLUESYNCOUT=$DATADIR/gluesync.out
SQLREAD=$DATADIR/sql-read.out
SQLWRITE=$DATADIR/sql-write.out
TRANSLUCENTOUT=$DATADIR/translucent.search.out
METAOUT=$DATADIR/meta.out
METACONCURRENCYOUT=$DATADIR/metaconcurrency.out
MANAGEOUT=$DATADIR/manage.out
SUBTREERENAMEOUT=$DATADIR/subtree-rename.out
ACIOUT=$DATADIR/aci.out
DYNLISTOUT=$DATADIR/dynlist.out
DDSOUT=$DATADIR/dds.out
MEMBEROFOUT=$DATADIR/memberof.out
MEMBEROFREFINTOUT=$DATADIR/memberof-refint.out
SHTOOL="$SRCDIR/../build/shtool"

