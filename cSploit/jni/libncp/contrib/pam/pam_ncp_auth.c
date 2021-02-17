/* $Header: /usr/src/redhat/BUILD/Linux-PAM-0.59/modules/pam_ncp/RCS/pam_ncp_auth.c,v 1.3 1998/03/04 02:52:07 dwmw2 Exp $ */

/*
 * Copyright David Woodhouse, 1998.  All rights reserved.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * $Log: pam_ncp_auth.c,v $
 *
 * Revision 1.28 2002/10/02 PP
 *    	1) default zenflag is now 0 and not anymore ZF_CREATE_NWCLIENT | ZF_AUTOMOUNT_NWHOME | ZF_CREATE_NWINFOS
 *	   that set trouble with some users not familiar with C coding. they turned these off in NDS
 *	   and they were still applied.
 *
 * Revision 1.27 2002/10/02 PP
 *   Experimental code:
 *	1) We experienced some stange behavior with contexless login. If user miss his password and retry to 
 *         login nw_search_ctx is called again and freeze...
 *	2) trying to find why console login do not work in RH73 ... this module says OK but session is closed 
 *         immediatly (?)
 *
 *    New Code:
 *      1) Some users complained that this module consumes 2 connections to NDS (one for the authentication
 *         and one for the autmounting of the Netware home). Actually only one should stay permanent, to be
 *         later reused by mounting of extra Netware ressource via ncpmap without having to provide again 
 *         login/password.  Since the authenticating connection is not in /etc/mtab, so it cannot be "reused" 
 *         by ncpmap anyway, so we must get rid of it at end of session opening part.
 *      2) Paul Berger (bergerp@breedtech.com) signaled me that in some environnement, home directories paths 
 *         are coded in NwAdmin as dir1/dir2/%LOGIN_NAME or dir1/dir2/%CN (with or without a terminating %) and 
 *         the mapping of the home is done within the context login script, with automatic substitution 
 *         of %LOGIN_NAME by the user's cn truncated to 8 or of %CN by the full user's CN. Some provisions are 
 *         added to do the substitution here when automounting home and writing data in the ~/.nwinfos file. 
 *         This is a crude code that chop off whatever starts with a % and append the user's name. We do not 
 *         support dir1/dir2/%LOGIN_NAME/dir3 !!!! See nds_pp_home_directory function.
 *
 * Revision 1.26 2002/05/24 PP
 *    Extended -Z option to allow some zenflags to be turned OFF on that workstation.
 *    With -Z all zenflags are off, but with -ZABCD, only flags ABCD are off.
 *
 *    Some support is given for the case of the same CN in different contexts. In that case, if the
 *	password is good but some other login errors is reported ( account disabled, too many connexions...)
 *	we report error and abort.
 *    But if the password is bad, then we try to find another user with the same CN in another context ( if any).
 *  TODO: since the original code, expired password error is ignored and user is allowed to login...(?)
 *  Added the service su to the table of allowed service.
 *    still problems with impersonnating ( operation not permitted)
 *    authentication and access is granted but pam still fails ( check /etc/pam.d/su )
 *
 * Revision 1.25 2002/05/10 PP
 *   The modifications required for operations in a pure IP network have been added.
 *   These modifications were proposed by Jean-François Burdet <jean-francois.burdet@adm.unige.ch>
 *     and are described at http://lnxjfb.unige.ch/grpnov/article.php3?id_article=81
 *   They include:
 *     adding a -A that trigger adding -A server option to ncpmount
 *     adding logic for context searches in case no list has been provided on the command line
 *
 *    Merging of group zenflags is now done before automatic group creation, so if creation fails
 *	(due to a name too long for example) zenflags are still unherited
 * Revision 1.24 2002/05/09 PP
 * EXPERIMENTAL CODE
 * NEW CODE is marked with #ifdef INPERSONATE
 *     Provisions have been added for home directories on shared NFS servers
 *	In that case, new user home directory already exists and will be mounted via autofs
 *	so
 *	1)  this module should stop complaining that user's home already exists !!!
 *	2)  the useradd command should be called with -M option ( DO NOT create HOME) !
 *	    instead of -m (DO create home)
 *	3) actually the module should complains if the home DO NOT EXIST (not created on NFS server
 *		or NFS server down).
 *	This is the exact reverse behaviour of the previous versions !!!!
 *	In order not to break things a flag (-n) has been added to the command line
 *	When present the module do not attempt to autocreate a local home; it expects the home to be present
 *		either locally or more likely as a NFS mounted ressource.
 *
 * This leads to an extra security problem. The PAM module is ran with root privileges and requires
 *   - read privileges to check existence of nwhome in current user home
 *   - write privileges to the NFS mounted user's home to create a directory (nwhome) and some files
 *  (.forward, .nwclient and .nwinfos).
 *   Thus the NFS directories must be exported with the no_root_squash option which is quite unsafe.
 *
 * This means that if somebody get root access on a local workstation, he can access any home directory on the NFS server
 * with root privileges !!!
 *  And any student in the campus that is root on his own machine have full rights on the NFS directories !
 *
 *  PARTIAL FIX: We do not set the no_root_squash option on NFS servers; As soon as this module has determined
 *  the current user's Unix UID ( either server required or calculated), and
 *   eventually autocreated it on the local machine, it impersonnates to that UID at every access to his home
 *
 *  MORE SERIOUS: if NFS directory is mounted with no_root_squash, internally called ncpmount fails with "mount point not found"
 *    since this suid program has no rights in NFS remote directories.
 *    Same problems with ncplogin, ncpmap that have no rights to "autocreate" the mounting point ~/ncp/SERVER/VOLUME !
 *
 * so when the -n parameter is present we mount Netware home in a local /mnt/ncp/$USER directory chmoded to 0700
 * and pass this directory to ncpmount...(this is the new behaviour of ncplogin/ncpmap with the -l option)
 *
 * Revision 1.23  2002/02/21 pp
 *	Fixed possible buffer overflow in mount_nwhome in verbose mode
 *	Fixed remote access granting table for samba >2.07 ( pam_tty=samba and pam_rhost=hostname)
 *		was null, null in samba 2.06
 *
 * Revision 1.22  2002/02/03 vana
 *	Fix couple of possible and real buffer overflows in the code.
 *	It still needs review, but buffer overflows exploitable by malicious
 *		NDS administrators should be fixed now. It is true that if you
 *		trust them enough to manage logins for your box, they do not have
 *		need for setting user's home directories to 4KB strings, but
 *		if they do, it should not crash PAM authentication module.
 *	I believe that no buffer overflow was exploitable by mere user.
 *	Reformatted some functions. We use tab size 8, not 4, not 6, not 7.5...
 *
 * Revision 1.21  2002/01/20 PP
 *	kscreensaver (from KDE) calls again PAM when tring to unlock the screen.
 *	currently only the authenticate service is called (not session)  But in that case
 *		current user environnment is used, so it is quite likely that a default context will be red from there.
 *		ncp_auth_tree has been modified to take this in account.
 *	furthermore, user's info and zen processing should not be done again . So another flag has been added
 *		this skip these parts if the PAM service name contains "saver". I hope it does include other
 *		screensavers 
 *
 * Revision 1.20  2002/01/14 PP
 * as suggested by vana, changed code in exechelper() to simplify use of su. rather than calling su -l username ....
 *
 *
 * Revision 1.19 2002/01/10 PP
 *	automatic ncpmounting is now run by this module as the current user and not as root
 *	this has two major consequenes:
 *	1) nwhome directory now get group permissions for the group users the group root. This is a cosmetic detail
 *	   since permissions are set to 700
 *	2) more important. If ncpmount is done by root, user cannot read back later its private key data for this connection.
 *	   ncp_get_private_data grant access to the private data but returns a length of 0 for the private data (???).
 *	   Note that if root requests these private data (using contrib/testing/pp/nwgetconnlist -a), he get a EACCESS (13) 
 *         error as expected. Private IS private!
 *      so this initial connection was not flagged as authenticated for this user and cannot be used later by ncpmap to mount
 *      additionnal netware volumes of the same tree with the same credentials.
 *
 *	changed mount_nwhome to call /bin/su -l unixname -c "/usr/sbin/ncpmount  -S sss -V vvvv -U uuu ... /home/user/nwhome"
 *	changed process zenscripts to do the same : for security reasons zenscripts are now run with current user's permissions
 *		and not anymore as root.
 *
 *	added a new account flag NDS_IS_NEW_USER in .nwinfos to ease zenscript processing
 *		is account is new, we can further personnalize config files such as kmailrc, liprefs.js ...
 *
 * Revision 1.18 2002/01/09 PP
 *	With redHat 7.2 the RHOST value passed to PAM with X remote access is emty end not NULL
 *		the kde entry in the zen table has been duplicated
 *	added -o "symlinks" to options of ncpmount.
 *
 * Revision 1.17 2001/12/12 PP
 *	With redHat 7.2 the return value of pam_set-cred and pam_get_cred that was PAM_IGNORE
 *		is really used, so authentication fails !
 *	Changed to PAM_SUCCESS
 *
 * Revision 1.16  2001/11/01 PP
 *	added the new entries needed by nwnet.c v >1.17 in ~/.nwclient
 *	must be after the usual server/user pawwd line since many script use 'read server passwd <~/.nwclient'
 *
 * Revision 1.15 2001/10/31  pp
 *	added support for Server AND contextless authentication upon request of Steve Flynn <sflyn@tpg.com.au>
 *		--> the server command line now accept a list of context to search ndsserver=SERVER_NAME:Ctx1,Ctx2,...Ctxn/group
 *	idem for password changing
 *
 * Revision 1.15  2001/11/01 PP
 *	added the new entries needed by nwnet.c v >1.17 in ~/.nwclient
 *	must be after the usual server/user pawwd line since many script use 'read server passwd <~/.nwclient'
 *
 * Revision 1.14 2001/10/27 PP
 *     Made the -m "empty" feature more reliable:
 *      1) if empty the Netware home directory is mounted as /home/user but we
 *       still have problem with chown and kde on a mounted netware
 *      2) parsemntpoint() did not scan the argument to the end, so with option
 *             -mnwhome, it was called twice ; once for "nwhome" and once for "me" !
 *        result is that the mounting point was ~/e !!! Corrected in pam_sm_athenticate.
 *     Due to this feature in -m option:
 *         we moved the creation of the .forward file during the authentication section.
 *         so:  if user is not currently logged in, system will still find the .forward file in local /home/user ( it cannot mount user's netware
 *           home to find it)...
 *         We do it twice (before and after automounting) so if user IS logged in system will
 *           find the .forward file... This second time could be done in a zenscript ...
 *
 * Revision 1.13 2001/10/24  pp
 *   added support for Tree & contextless authentication
 *                 --> new command line tree=TREE_NAME:Ctx1,Ctx2,...Ctxn/group
 *                 --> reorganized authentication code for the group verification part to be common
 *                         to auth_server and auth_tree
 *                 --> modified nw_auto_mount_home to pass a FQDN to ncpmount
 *  idem for password changing
 *
 * Revision 1.12 2001/10/24  pp
 *   added support for ssh remote access ( under the same ZEN flag as Telnet access)
 *   due to current implementation of OpenSSH, user must have an account & home on the local machine
 *   for authentication by Netware to work. sshd peeks first in user's home to find data files...
 * so automagic creation does not work with ssh
 *
 * Revision 1.11 2001/02/24  pp
 * Added some support for reading bindery properties
 *    IDENTIFICATION --> Gecos field
 *    HOME_DIRECTORY --> nwhome ( must be on the same server as the "autenticator"
 *
 * Revision 1.10 2001/02/21 12:34:56  vana
 * Cleaned warnings, fixed bindery login.
 *
 * Revision 1.9  2001/01/26 16:50:00  pp
 * Added Zenux Flags concept for tuning user's account on Linux
 * for more documentation see http://cipcinsa.insa-lyon.fr/ppollet/pamncp/
 *
 * Revision 1.8  2001/01/15 19:30:00  pp
 * Added optional automounting of Netware Home directory
 * in ~/nwhome, and dismounting upon session ending
 *
 * Revision 1.7  2001/01/06 10:50:00  pp
 * Added support for pré DS8 network using either Location
 * attribute or Postal Adress
 *
 * Revision 1.6  2000/07/06 14:40:00  vana
 * Added DCV_DEREF_ALIASES where needed
 *
 * Revision 1.5  2000/06/02 17:40:00  vana
 * Tons of new code - passwd, changed auth code
 *
 * Revision 1.4  1999/10/31 15:44:44  vana
 * Couple of new options, new check for group membership
 *
 * Revision 1.3  1998/03/04 02:52:07  dwmw2
 * Oops - licensing has to be GPL as it's using the ncpfs libraries and headers.
 *
 * Revision 1.2  1998/03/04 02:39:12  dwmw2
 * Tidied up (a bit) for initial release.
 *
 * Revision 1.1  1998/03/04 02:21:07  dwmw2
 * Initial revision
 *
 *
 */

#define _GNU_SOURCE
#define _BSD_SOURCE
#define inline __inline__
#define NCP_OBSOLETE
#include <ncp/nwcalls.h>
#include <ncp/nwnet.h>
#include <ncp/nwclient.h>

#include <features.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>

#include "support.h"

static const char rcsid[] = "$Id: pam_ncp_auth.c,v 1.9 2001/21/26 16:52:07 PP Exp $ pam_ncp authentication functions. Dave@imladris.demon.co.uk,vandrove@vc.cvut.cz,patrick.pollet@insa-lyon.fr ";

/* Define function prototypes */

static void 
nw_cleanup_conn(UNUSED(pam_handle_t *pamh),
		void *data,
		UNUSED(int error_status))
{
	NWCCODE err;

	err = ncp_close((struct ncp_conn*)data);
	syslog(LOG_NOTICE, "pam closing authenticating connection: %s", strnwerror(err));
}

#define QFC_FIRST_UNUSED	0
#define QFC_NEXT_UNUSED		1
#define QFC_PREFFER_SERVER	2
#define QFC_REQUIRE_SERVER	4

// caldera open linux does not likes the -G parameter in useradd !!!PP
// so we will have a specific flag for these and use a second call to usermod -G
#define NO_PROCESS_GROUPS	1

#define QFMU_GID	0x0001
#define QFMU_GECOS	0x0002
#define QFMU_DIR	0x0004
#define QFMU_SHELL	0x0008

/* reactivate if you prefer NDS property Postal Address
rather than Location for pre-DS 8 systems*/
#if 0
#define USE_POSTAL_ADDRESS
#endif

// from the snapin C code
// names of NDS attributes used

// USE_DUMMY_ATTRibutes must be set in the Makefile

#ifndef USING_DUMMY_ATTRIBUTES
// the real ones
#define ATTR_UID	"UNIX:UID"
#define ATTR_PGNAME	"UNIX:Primary GroupName"
#define ATTR_PGID	"UNIX:Primary GroupID"
#define ATTR_GID	"UNIX:GID"
#define ATTR_SHELL	"UNIX:Login Shell"
#define ATTR_COM	"UNIX:Comments"
#define ATTR_HOME	"UNIX:Home Directory"
#else
// dummy attributes for testing
// created with Schemax with the same syntax
// and associated to user class and group class
#define ATTR_UID	              "LINUX:UID"
#define ATTR_PGNAME	"LINUX:Primary GroupName"
#define ATTR_PGID	"LINUX:Primary GroupID"
#define ATTR_GID	"LINUX:GID"
#define ATTR_SHELL	"LINUX:Login Shell"
#define ATTR_COM	"LINUX:Comments"
#define ATTR_HOME	"LINUX:Home Directory"
#endif

// the attribute used to test presence of NDS8
// either real or dummy (not used yet)
#define ATTR_NDS8	ATTR_UID

// other attributes used
// absent NDS8 attributes are searched in L attribute
// also new properties ( Zenux Flags, Other group...)
#define ATTR_LOCATION	"L"

#define ATTR_LOGIN_SCRIPT	"Login Script"
#define ATTR_GRP_MBS		"Group Membership"
#define ATTR_HOST_SERVER	"Host Server"
#define ATTR_HOST_RN		"Host Resource Name"
#define ATTR_SMTP_EMAIL		"Email Address"
#define ATTR_LDAP_EMAIL		"Internet Email Address"
#define ATTR_FULL_NAME		"Full Name"
#define ATTR_SURNAME		"Surname"
#define ATTR_GIVEN_NAME		"Given Name"
#define ATTR_POSTAL_ADDRESS	"Postal Address"
#define ATTR_HOME_NW		"Home Directory"
#define ATTR_MESSAGE_SERVER	"Message Server"

// the proper naming attribute may be customized here (must be a CI_STRING )
#define ATTR_GECOS		ATTR_FULL_NAME

// syntaxes of the used attributes
#define SYN_LOCATION		SYN_CI_STRING
#define SYN_LOGIN_SCRIPT	SYN_STREAM
#define SYN_UID			SYN_INTEGER
#define SYN_PGNAME		SYN_DIST_NAME
#define SYN_PGID		SYN_INTEGER
#define SYN_GID			SYN_INTEGER
#define SYN_SHELL		SYN_CE_STRING
#define SYN_COM			SYN_CI_STRING
#define SYN_HOME		SYN_CE_STRING
#define SYN_GRP_MBS		SYN_DIST_NAME

/* user account tuning flags red from the NDS  (ZenFlag) */
// zenFlag processing can be turned off by a command line -Z
// it is on by default

#define NDS_ZF_AUTOMOUNT_NWHOME		'A'
#define NDS_ZF_BROADCAST_ALL		'B'
#define NDS_ZF_BROADCAST_CONSOLE	'C'
#define NDS_ZF_D			'D'
#define NDS_ZF_E			'E'
#define NDS_ZF_ALLOW_FTP_ACCESS		'F'
#define NDS_ZF_G			'G'
#define NDS_ZF_ALLOW_RSH_ACCESS		'H'
#define NDS_ZF_CREATE_NWINFOS		'I'
#define NDS_ZF_J			'J'
#define NDS_ZF_K			'K'
#define NDS_ZF_L			'L'
#define NDS_ZF_FORWARD_MAIL		'M'
#define NDS_ZF_CREATE_NWCLIENT		'N'
#define NDS_ZF_OVERWRITE_NWCLIENT	'O'
#define NDS_ZF_PASSWD_IN_NWCLIENT	'P'
#define NDS_ZF_Q			'Q'
#define NDS_ZF_ALLOW_RLOGIN_ACCESS	'R'
#define NDS_ZF_ALLOW_SAMBA_ACCESS	'S'
#define NDS_ZF_ALLOW_TELNET_ACCESS	'T'
#define NDS_ZF_U			'U'
#define NDS_ZF_VOLATILE_ACCOUNT		'V'
// I reserve this letter, but I still don't know
// how auth_nds_mod of Apache works ( TODO !)
#define NDS_ZF_ALLOW_WEB_ACCESS		'W'
#define NDS_ZF_ALLOW_X_ACCESS		'X'
#define NDS_ZF_Y			'Y'
#define NDS_ZF_Z			'Z'
// three admin defined scripts that can be launched at session opening
// these scripts WILL source the file ~/.nwinfos to get user's data
// if more than one will be executed in this order
#define NDS_ZF_0			'0'
#define NDS_ZF_1			'1'
#define NDS_ZF_2			'2'
// three admin defined scripts that can be launched at session closing
// these scripts WILL source the file ~/.nwinfos to get user's data
// if more than one will be executed in this order
#define NDS_ZF_3			'3'
#define NDS_ZF_4			'4'
#define NDS_ZF_5			'5'

/*corresponding binary flag*/

#define ZF_UNSUPPORTED			0x00000000

#define ZF_AUTOMOUNT_NWHOME		0x00000001
#define ZF_BROADCAST_ALL		0x00000002
#define ZF_BROADCAST_CONSOLE		0x00000004
#define ZF_D				0x00000008
#define ZF_E				0x00000010
#define ZF_ALLOW_FTP_ACCESS		0x00000020
#define ZF_G				0x00000040
#define ZF_ALLOW_RSH_ACCESS		0x00000080
#define ZF_CREATE_NWINFOS		0x00000100
#define ZF_J				0x00000200
#define ZF_K				0x00000400
#define ZF_L				0x00000800
#define ZF_FORWARD_MAIL			0x00001000
#define ZF_CREATE_NWCLIENT		0x00002000
#define ZF_OVERWRITE_NWCLIENT		0x00004000
#define ZF_PASSWD_IN_NWCLIENT		0x00008000
#define ZF_Q				0x00010000
#define ZF_ALLOW_RLOGIN_ACCESS		0x00020000
#define ZF_ALLOW_SAMBA_ACCESS		0x00040000
#define ZF_ALLOW_TELNET_ACCESS		0x00080000
#define ZF_U				0x00100000
#define ZF_V				0x00200000
#define ZF_ALLOW_WEB_ACCESS		0x00400000
#define ZF_ALLOW_X_ACCESS		0x00800000
#define ZF_Y				0x01000000
#define ZF_Z				0x02000000
#define ZF_0				0x04000000
#define ZF_1				0x08000000
#define ZF_2				0x10000000
#define ZF_3				0x20000000
#define ZF_4				0x40000000
#define ZF_5				0x80000000

#define ZF_OPENING_SCRIPTS		(ZF_0 | ZF_1 | ZF_2)
#define ZF_CLOSING_SCRIPTS		(ZF_3 | ZF_4 | ZF_5)


//#define ZF_DEFAULTS ZF_CREATE_NWCLIENT | ZF_AUTOMOUNT_NWHOME | ZF_CREATE_NWINFOS
#define ZF_DEFAULTS 0

// where must be the scripts
// TODO: let PAM finds them somewhere in its path
// (if it has one ?)
#define ZEN_SCRIPT_0	"/usr/local/bin/zenscript0"
#define ZEN_SCRIPT_1	"/usr/local/bin/zenscript1"
#define ZEN_SCRIPT_2	"/usr/local/bin/zenscript2"
#define ZEN_SCRIPT_3	"/usr/local/bin/zenscript3"
#define ZEN_SCRIPT_4	"/usr/local/bin/zenscript4"
#define ZEN_SCRIPT_5	"/usr/local/bin/zenscript5"

#define NWINFOS_FILE	".nwinfos"
#define NWCLIENT_FILE	".nwclient"
#define DEF_MNT_PNT	"nwhome"

struct zenElement {
	char letter;
	long value;
};
/* only the flag processed in this version are !=ZF_UNSUPPORTED */
static struct zenElement zenTable[]={
{ NDS_ZF_AUTOMOUNT_NWHOME,	ZF_AUTOMOUNT_NWHOME },
{ NDS_ZF_BROADCAST_ALL,		ZF_BROADCAST_ALL },
{ NDS_ZF_BROADCAST_CONSOLE,	ZF_BROADCAST_CONSOLE },
{ NDS_ZF_D,			ZF_UNSUPPORTED },
{ NDS_ZF_E,			ZF_UNSUPPORTED },
{ NDS_ZF_ALLOW_FTP_ACCESS,	ZF_ALLOW_FTP_ACCESS },
{ NDS_ZF_G,			ZF_UNSUPPORTED },
{ NDS_ZF_ALLOW_RSH_ACCESS,	ZF_ALLOW_RSH_ACCESS },
{ NDS_ZF_CREATE_NWINFOS,	ZF_CREATE_NWINFOS },
{ NDS_ZF_J,			ZF_UNSUPPORTED },
{ NDS_ZF_K,			ZF_UNSUPPORTED },
{ NDS_ZF_L,			ZF_UNSUPPORTED },
{ NDS_ZF_FORWARD_MAIL,		ZF_FORWARD_MAIL },
{ NDS_ZF_CREATE_NWCLIENT,	ZF_CREATE_NWCLIENT },
{ NDS_ZF_OVERWRITE_NWCLIENT,	ZF_OVERWRITE_NWCLIENT },
{ NDS_ZF_PASSWD_IN_NWCLIENT,	ZF_PASSWD_IN_NWCLIENT },
{ NDS_ZF_Q,			ZF_UNSUPPORTED },
{ NDS_ZF_ALLOW_RLOGIN_ACCESS,	ZF_ALLOW_RLOGIN_ACCESS },
{ NDS_ZF_ALLOW_SAMBA_ACCESS,	ZF_ALLOW_SAMBA_ACCESS },
{ NDS_ZF_ALLOW_TELNET_ACCESS,	ZF_ALLOW_TELNET_ACCESS },
{ NDS_ZF_U,			ZF_UNSUPPORTED },
{ NDS_ZF_VOLATILE_ACCOUNT,	ZF_UNSUPPORTED },
{ NDS_ZF_ALLOW_WEB_ACCESS,	ZF_UNSUPPORTED },
{ NDS_ZF_ALLOW_X_ACCESS,	ZF_ALLOW_X_ACCESS },
{ NDS_ZF_Y,			ZF_UNSUPPORTED },
{ NDS_ZF_Z,			ZF_UNSUPPORTED },
// zen scripts should source ~/.nwinfos file, so make sure
// it is created
{ NDS_ZF_0,			ZF_0 | ZF_CREATE_NWINFOS },
{ NDS_ZF_1,			ZF_1 | ZF_CREATE_NWINFOS },
{ NDS_ZF_2,			ZF_2 | ZF_CREATE_NWINFOS },
{ NDS_ZF_3,			ZF_3 | ZF_CREATE_NWINFOS },
{ NDS_ZF_4,			ZF_4 | ZF_CREATE_NWINFOS },
{ NDS_ZF_5,			ZF_5 | ZF_CREATE_NWINFOS },
{ 0,				0 }
};

static long
decodeZenFlag(const char **optp)
{
	long r = 0;
	char c;
	const char *ZF = *optp;

	while ((c = *ZF++) != 0) {
		struct zenElement *z;

		c = toupper(c);
		for (z = zenTable; z->letter; z++) {
			if (c == z->letter) {
				r |= z->value;
				break;
			}
		}
	}
	*optp = ZF;
	return r;
}

struct pam_ncp_state {
	struct {
		uid_t	min;
		uid_t	max;
		int	flags;
		int	modflags;
	} uid;
	struct {
		gid_t	min;
		gid_t	max;
		int	flags;
	} gid;

};

// concat2a (give a local string that is destroyed at the end of the function)
#define concat2a(one,two)	({		\
	size_t lone = strlen(one);		\
	size_t ltwo = strlen(two);		\
	char* buf = alloca(lone + ltwo + 1);	\
	memcpy(buf, one, lone);			\
	memcpy(buf + lone, two, ltwo + 1);	\
	buf; })

// concat2m (give a "global" string that is NOT destroyed at the end of the function)
#define concat2m(one,two)	({		\
	size_t lone = strlen(one);		\
	size_t ltwo = strlen(two);		\
	char* buf = malloc(lone + ltwo + 1);	\
	memcpy(buf, one, lone);			\
	memcpy(buf + lone, two, ltwo + 1);	\
	buf; })


#define concat3a(one,med,two)	({		\
	size_t lone = strlen(one);		\
	size_t ltwo = strlen(two);		\
	char* buf = alloca(lone + 1+ ltwo + 1);	\
	memcpy(buf, one, lone);			\
	buf[lone] = med;			\
	memcpy(buf + lone + 1, two, ltwo + 1);	\
	buf; })

static int getnumber(int* val, const char** str) {
	const char *p = *str;
	char *z;

	if (!*p)
		return 1;
	if (*p == ',') {
		*str = p + 1;
		return 1;
	}
	*val = strtoul(p, &z, 0);
	if (p == z)
		return -1;
	if (*z == ',')
		z++;
	*str = z;
	return 0;
}

// flip Dos antislash to Unix
// converts to uppercase for ncpmount to work with ROOT options ON
static void
unixifyPathUC(char *dosPath)
{
	size_t i;

	for (i = 0; i < strlen(dosPath); i++) {
		if (dosPath[i] == '\\')
			dosPath[i] = '/';
	}
	str_upper(dosPath);
}

// remove all spaces from a NDS name
static void
trim(char *string)
{
	char *aux = string;
	char c;

	while ((c = *string++) != 0) {
		if (c != ' ')
			*aux++ = c;
	}
	*aux = 0;
}

#define QF_VERBOSE		0x0001
#define QF_DEBUG		0x0002
#define QF_NOSU			0x0004
#define QF_NOSUEQ		0x0008
#define QF_AUTOCREATE		0x0010
#define QF_AUTOMODIFY		0x0020
#define QF_BINDERY		0x0040
#define QF_NO_PEER_CHECKS	0x0080
#define QF_USE_NETWARE_IP	0x1000
// version 1.24. automatically create home directory on local machine for new users
// it is on by default
// should be turned off by -n option if homes are on a remote NFS server (automounted)
#define QF_MOUNTLOCALLY		0x2000
#define QF_CREATEHOME		0x4000
// test v 1.25 impersonnating to current user when accessing his NFS mounted home
#define IMPERSONNATE 1

static int
is_member_of_bindery_group(NWCONN_HANDLE conn, const char *user, const char *group)
{
	int err;

	err = NWIsObjectInSet(conn, user, NCP_BINDERY_USER, "GROUPS_I'M_IN", group, NCP_BINDERY_UGROUP);
	if (!err) {
		err = NWIsObjectInSet(conn, group, NCP_BINDERY_UGROUP, "GROUP_MEMBERS", user, NCP_BINDERY_USER);
		if (!err) {
			return 0;
		} else {
			syslog(LOG_WARNING, "inconsistent bindery database for user %s and group %s: %s\n", user, group, strnwerror(err));
		}
	} else {
		syslog(LOG_WARNING, "user %s is not member of %s: %s\n", user, group, strnwerror(err));
	}
	return -1;
}

static int
is_member_of_nds_group(NWDSContextHandle ctx, NWCONN_HANDLE conn, NWObjectID oid, const char *group)
{
	int eval = 0;
	Buf_T *buf = NULL;
	NWDSCCODE dserr;
	nbool8 match;
	dserr = NWDSAllocBuf(DEFAULT_MESSAGE_LEN, &buf);
	if (dserr) {
		syslog(LOG_WARNING, "NWDSAllocBuf() failed with %s\n", strnwerror(dserr));
		eval = 120;
		goto bailout;
	}
	dserr = NWDSInitBuf(ctx, DSV_COMPARE, buf);
	if (dserr) {
		syslog(LOG_WARNING, "NWDSInitBuf() failed with %s\n", strnwerror(dserr));
		eval = 121;
		goto bailout;
	}
	dserr = NWDSPutAttrName(ctx, buf, "Group Membership");
	if (dserr) {
		syslog(LOG_WARNING, "NWDSPutAttrName() failed with %s\n", strnwerror(dserr));
		eval = 122;
		goto bailout;
	}
	dserr = NWDSPutAttrVal(ctx, buf, SYN_DIST_NAME, group);
	if (dserr) {
		syslog(LOG_WARNING, "NWDSPutAttrVal() failed with %s\n", strnwerror(dserr));
		eval = 123;
		goto bailout;
	}
	dserr = __NWDSCompare(ctx, conn, oid, buf, &match);
	if (dserr) {
		syslog(LOG_WARNING, "__NWDSCompare() failed with %s(oid=%x)\n", strnwerror(dserr), oid);
		eval = 124;
		goto bailout;
	}
	if (!match) {
		eval = 125;
	}
bailout:;
	if (buf)
		NWDSFreeBuf(buf);
	return eval;
}

static NWCCODE
nw_create_conn_to_server(NWCONN_HANDLE * conn, const char *server, const char *user, const char *pwd, int qflag)
{
	struct ncp_bindery_object uinfo;
	unsigned char ncp_key[8];
	NWCONN_HANDLE cn;
	char *pwd2;
	long err;

	if (qflag & QF_DEBUG)
		syslog(LOG_DEBUG, "Trying to contact %s/%s\n", server, user);

	pwd2 = strdup(pwd);
	if (!pwd2) {
		syslog(LOG_WARNING, "Not enough memory when uppercasing password\n");
		err = PAM_TRY_AGAIN;
		goto fail2;
	}
	str_upper(pwd2);
	err = NWCCOpenConnByName(NULL, server, NWCC_NAME_FORMAT_BIND, 0, NWCC_RESERVED, &cn);
	if (err) {
		syslog(LOG_WARNING, "%s when trying to open connection\n", strnwerror(err));
		err = PAM_TRY_AGAIN;
		goto fail2;
	}
	if (!(qflag & QF_BINDERY) && NWIsDSServer(cn, NULL)) {
		err = nds_login_auth(cn, user, pwd2);
	} else {
		err = ncp_get_encryption_key(cn, ncp_key);
		if (err) {
			syslog(LOG_WARNING, "%s when trying to get encryption key. Doing unencrypted\n", strnwerror(err));
			err = ncp_login_unencrypted(cn, NCP_BINDERY_USER, user, pwd2);
		} else {
			err = ncp_get_bindery_object_id(cn, NCP_BINDERY_USER, user, &uinfo);
			if (err) {
				syslog(LOG_WARNING, "%s when trying to get object ID\n", strnwerror(err));
				err = PAM_USER_UNKNOWN;
				goto fail;
			}
			err = ncp_login_encrypted(cn, &uinfo, ncp_key, pwd2);
		}
	}
	if (err && err != NWE_PASSWORD_EXPIRED) {
		syslog(LOG_WARNING, "%s when trying to login\n", strnwerror(err));
		switch (err) {
		case ERR_NO_SUCH_ENTRY:
		case NWE_SERVER_UNKNOWN:
			err = PAM_USER_UNKNOWN;
			break;
		case NWE_LOGIN_MAX_EXCEEDED:
		case NWE_LOGIN_UNAUTHORIZED_TIME:
		case NWE_LOGIN_UNAUTHORIZED_STATION:
		case NWE_ACCT_DISABLED:
			err = PAM_AUTH_ERR;
			break;
		default:
			err = PAM_AUTH_ERR;
			break;
		}
		goto fail;
	}
// PP: I don't understand why we do not report this error ???
#if 0
	if (err)
		err = PAM_NEW_AUTHTOK_REQD;
#else
	err = 0;
#endif
	if (qflag & QF_DEBUG)
		syslog(LOG_DEBUG, "User %s/%s was successfully authorized\n", server, user);
	*conn = cn;
	return err;
fail:;
	NWCCCloseConn(cn);
fail2:;
	*conn = NULL;
	return err;
}

static int
nw_get_nwid(NWCONN_HANDLE conn, NWObjectID * id, UNUSED(int qflag))
{
	NWCCODE err;

	err = NWCCGetConnInfo(conn, NWCC_INFO_USER_ID, sizeof (*id), id);
	if (err) {
		syslog(LOG_WARNING, "%s when retrieving object ID\n", strnwerror(err));
		return PAM_SYSTEM_ERR;
	}
	return PAM_SUCCESS;
}

/* old server centric code called by server=XXX/group  cmd line parameter */
static int
nw_create_verify_conn_to_server(NWCONN_HANDLE * conn, NWObjectID * id, 
				const char *server, const char *user, const char *pwd, 
				int qflag, const char *group)
{
	NWCONN_HANDLE cn;
	NWCCODE err;
	NWObjectID oid;

	err = nw_create_conn_to_server(&cn, server, user, pwd, qflag);
	if (err && err != PAM_NEW_AUTHTOK_REQD)
		return err;
	err = nw_get_nwid(cn, &oid, qflag);
	if (err) {
		syslog(LOG_WARNING, "Error %s retrieving user ID for %s\n", strnwerror(err), user);
		goto bailout;
	}

	if (qflag & QF_NOSU) {
		if (oid == 0x00000001) {
			err = PAM_AUTH_ERR;
			syslog(LOG_WARNING, "Access denied for %s/%s because of it is supervisor\n", server, user);
			goto bailout;
		}
		if (qflag & QF_DEBUG)
			syslog(LOG_DEBUG, "User %s/%s passed supervisor check\n", server, user);
	}
	if (qflag & QF_NOSUEQ) {
		nuint8 level;

		err = NWGetBinderyAccessLevel(cn, &level, NULL);
		if (err) {
			syslog(LOG_WARNING, "Access denied for %s/%s because of I/O error during object rights verification\n", server, user);
			err = PAM_AUTH_ERR;
			goto bailout;
		}
		if ((level >= 0x30) || ((level & 0xF) >= 3)) {
			syslog(LOG_WARNING, "Access denied for %s/%s because of it is supervisor equivalent\n", server, user);
			err = PAM_AUTH_ERR;
			goto bailout;
		}
	}
	if (group) {
		if (!(qflag & QF_BINDERY)) {
			NWDSContextHandle ctx;
			nuint32 c;

			err = NWDSCreateContextHandle(&ctx);
			if (err) {
				syslog(LOG_WARNING, "NWDSCreateContextHandle() failed with %s\n", strnwerror(err));
				err = PAM_SYSTEM_ERR;
				goto bailout;
			}
			c = DCV_XLATE_STRINGS | DCV_TYPELESS_NAMES | DCV_DEREF_ALIASES;
			err = NWDSSetContext(ctx, DCK_FLAGS, &c);
			if (err) {
				syslog(LOG_WARNING, "NWDSSetContext() failed with %s\n", strnwerror(err));
				err = PAM_SYSTEM_ERR;
				NWDSFreeContext(ctx);
				goto bailout;
			}
			err = is_member_of_nds_group(ctx, cn, oid, group);
			NWDSFreeContext(ctx);
			if (err) {
				syslog(LOG_WARNING, "%s is not member of NDS %s\n", user, group);
				err = PAM_AUTH_ERR;
				goto bailout;
			}
		} else {	/* bindery testing */
			if (is_member_of_bindery_group(cn, user, group)) {
				syslog(LOG_WARNING, "%s is not member of BINDERY %s\n", user, group);
				err = PAM_AUTH_ERR;
				goto bailout;
			}
		}
	}
	if (id)
		*id = oid;
	if (conn)
		*conn = cn;
	else
		NWCCCloseConn(cn);
	return PAM_SUCCESS;
bailout:;
	/* Close the connection. */
	NWCCCloseConn(cn);
	return err;
}

// this code was contributed by Jean Francois Burdet <jean.francois.burdet@adm.unige.ch>
// to implement a contextless login if a list of context to search is not provided in the command line
// The parameter ndserver was replaced by a conn  since it is now called from nw_create_verify_conn_to_tree
// with an open connection to a tree or a ndsserver
// extra errors checkings and buffer overflow tested
//static void nw_ctx_search(const char * user_cn, const char * nds_server, char * contexts) {
static NWDSCCODE
nw_ctx_search(const char *user_cn, NWCONN_HANDLE conn, char *contexts, size_t maxsize)
{
	NWDSContextHandle context;
	NWDSCCODE ccode;
	nuint32 iterationHandle;
	nint32 countObjectsSearched;
	nuint32 objCntr;
	nuint32 objCount;
	char objectName[MAX_DN_CHARS + 1];
	size_t ctxp;
	nuint32 contextFlags;

	// buffers
	pBuf_T searchFilter;	// search filter
	pBuf_T retBuf;		// result buffer for NWDSSearch
	Filter_Cursor_T *cur;	// search expression tree temporary buffer

	if (!contexts || maxsize < MAX_DN_CHARS + 1)
		return EINVAL;

	ccode = NWDSCreateContextHandle(&context);
	if (ccode) {
		syslog(LOG_NOTICE, "nw_ctx_search:NWDSCreateContextHandle (DCK_FLAGS) failed, returns %d\n", ccode);
		goto Exit1;
	}
	ccode = NWDSSetContext(context, DCK_NAME_CONTEXT, "[Root]");
	if (ccode) {
		syslog(LOG_NOTICE, "nw_ctx_search:NWDSGetContext (DCK_FLAGS) failed, returns %d\n", ccode);
		goto Exit2;
	}

	ccode = NWDSGetContext(context, DCK_FLAGS, &contextFlags);
	if (ccode) {
		syslog(LOG_NOTICE, "nw_ctx_search:NWDSGetContext (DCK_FLAGS) failed, returns %d\n", ccode);
		goto Exit2;
	}

	contextFlags |= DCV_TYPELESS_NAMES;

	ccode = NWDSSetContext(context, DCK_FLAGS, &contextFlags);
	if (ccode) {
		syslog(LOG_NOTICE, "nw_ctx_search:NWDSSetContext (DCK_FLAGS DCV_TYPELESS_NAMES) failed, returns %d\n", ccode);
		goto Exit2;
	}

	ccode = NWDSAddConnection(context, conn);
	if (ccode) {
		syslog(LOG_NOTICE, "nw_ctx_search:NWDSAddConnection failed, returns %d\n", ccode);
		goto Exit2;
	}

	/**********************************************************************
	 * In order to search, we need:                                       *
	 *	A Filter Cursor (to build the search expression)              *
	 *	A Filter Buffer (to store the expression; used by NWDSSearch) *
	 *	A Buffer to store which attributes we need information on     *
	 *	A Result Buffer (to store the search results)                 *
	 **********************************************************************/

	/* Allocate Filter buffer and Cursor and populate                     */
	ccode = NWDSAllocBuf(DEFAULT_MESSAGE_LEN, &searchFilter);
	if (ccode) {
		syslog(LOG_NOTICE, "nw_ctx_search:NWDSAllocBuf returned: %d\n", ccode);
		goto Exit3;
	}
	// Initialize the searchFilter buffer
	ccode = NWDSInitBuf(context, DSV_SEARCH_FILTER, searchFilter);
	if (ccode) {
		syslog(LOG_NOTICE, "nw_ctx_search:NWDSInitBuf returned: %d\n", ccode);
		goto Exit6;
	}
	// Allocate a filter cursor to put the search expression
	ccode = NWDSAllocFilter(&cur);
	if (ccode) {
		syslog(LOG_NOTICE, "nw_ctx_search:NWDSAllocFilter returned: %d\n", ccode);
		goto Exit6;
	}
	// Build the expression tree in cur, then place into searchFilter
	// Object Class = User AND CN =  user_cn
	ccode = NWDSAddFilterToken(cur, FTOK_ANAME, "Object Class", SYN_CLASS_NAME);
	ccode = NWDSAddFilterToken(cur, FTOK_EQ, NULL, 0);
	ccode = NWDSAddFilterToken(cur, FTOK_AVAL, "User", SYN_CLASS_NAME);
	ccode = NWDSAddFilterToken(cur, FTOK_AND, NULL, 0);
	ccode = NWDSAddFilterToken(cur, FTOK_ANAME, "CN", SYN_CI_STRING);
	ccode = NWDSAddFilterToken(cur, FTOK_EQ, NULL, 0);
	ccode = NWDSAddFilterToken(cur, FTOK_AVAL, user_cn, SYN_CI_STRING);
	ccode = NWDSAddFilterToken(cur, FTOK_END, NULL, 0);

	ccode = NWDSPutFilter(context, searchFilter, cur, NULL);
	if (ccode) {
		syslog(LOG_NOTICE, "nw_ctx_search:NWDSPutFilter returned: %d\n", ccode);
		goto Exit5;
	} else
		cur = NULL;

	ccode = NWDSAllocBuf(DEFAULT_MESSAGE_LEN, &retBuf);
	if (ccode) {
		syslog(LOG_NOTICE, "nw_ctx_search:NWDSAllocBuf returned: %d\n", ccode);
		goto Exit5;
	}

	ctxp = 0;
	iterationHandle = NO_MORE_ITERATIONS;
	// while NWDSSearch still can get some objects...
	do {
		ccode = NWDSSearch(context, "[Root]", DS_SEARCH_SUBTREE, 0,	// don't dereference aliases
				   searchFilter, 0,	// we want attributes and values
				   0,	// only want information in attrNames
				   NULL, &iterationHandle, 0,	// reserved
				   &countObjectsSearched, retBuf);
		if (ccode) {
			syslog(LOG_NOTICE, "nw_ctx_search:NWDSSearch returned: %s\n", strnwerror(ccode));
			goto Exit4;
		}
		// count the object returned in the buffer
		ccode = NWDSGetObjectCount(context, retBuf, &objCount);
		if (ccode) {
			syslog(LOG_NOTICE, "nw_ctx_search:NWDSGetObjectCount returned: %d\n", ccode);
			goto Exit4;
		}
		// for the number of objects returned...

		for (objCntr = 0; objCntr < objCount; objCntr++) {
			char *p;
			size_t ln;

			// get an object name
			ccode = NWDSGetObjectName(context, retBuf, objectName, NULL, NULL);
			if (ccode) {
				syslog(LOG_NOTICE, "nw_ctx_search:NWDSGetObjectName returned: %d\n", ccode);
				goto Exit4;
			}
			// now, we get the user context wich starts at the first . occurence in the string
			p = strchr(objectName, '.');
			if (!p) {
				break;
			}
			ln = strlen(p + 1);
			if (ctxp + ln >= maxsize) {
				break;
			}
			if (ctxp) {
				*contexts++ = ',';
				maxsize--;
			}
			memcpy(contexts, p + 1, ln);
			contexts += ln;
			maxsize -= ln;
			ctxp = 1;

		}		// objCntr
	} while (iterationHandle != NO_MORE_ITERATIONS);
Exit4:;
	if (iterationHandle != NO_MORE_ITERATIONS) {
		// let's keep the final dserr as the 'out of memory error' from ptr->getval()
		NWDSCloseIteration(context, DSV_SEARCH_FILTER, iterationHandle);
	}
	//lets remove trailing ':'
	if (ctxp)
		*contexts = 0;
	else
		ccode = PAM_USER_UNKNOWN;	// not found...

	if (retBuf)
		NWDSFreeBuf(retBuf);
Exit5:;
	if (cur)
		NWDSFreeFilter(cur, NULL);
Exit6:;
	if (searchFilter)
		NWDSFreeBuf(searchFilter);
Exit3:;
Exit2:;
	NWDSFreeContext(context);
Exit1:;
	return ccode;
}

/* new code called by tree=XXX:ctx1,ctx2...,ctxn/group  cmd line parameter  */
/* or by ndsserver =XXX:ctx1.../Group */
static int
nw_create_verify_conn_to_tree(NWCONN_HANDLE * conn, NWObjectID * id,
			      const char *tree, const char *user, const char *contexts, const char *pwd,
			      int qflag, const char *group, nuint nameFormat)
{
	NWCONN_HANDLE cn = NULL;
	NWCCODE err;
	NWObjectID oid;
	NWDSContextHandle ctx = NULL;
	nuint32 c;
	char fqdn[MAX_DN_CHARS + 1];
	const char *ctxStart;
	const char *ctxEnd;
	char ctxbuf[4096];	// buffer to hold possible matches in a contexless login

	err = NWCCOpenConnByName(NULL, tree, nameFormat, NWCC_OPEN_NEW_CONN, NWCC_RESERVED, &cn);
	if (err) {
		if (nameFormat == NWCC_NAME_FORMAT_NDS_TREE)
			syslog(LOG_WARNING, "%s when trying to open connection to tree %s \n", strnwerror(err), tree);
		else
			syslog(LOG_WARNING, "%s when trying to open connection to NDS server !%s! \n", strnwerror(err), tree);
		return PAM_TRY_AGAIN;
	}

	err = PAM_SYSTEM_ERR;
	if (NWDSCreateContextHandle(&ctx)) {
		syslog(LOG_WARNING, "NWDSCreateContextHandle() failed with %s\n", strnwerror(err));
		goto bailout;
	}
	if (NWDSGetContext(ctx, DCK_FLAGS, &c)) {
		syslog(LOG_WARNING, "NWDSGetContext(DCK_FLAGS) failed with %s\n", strnwerror(err));
		goto bailout;
	}
	c |= DCV_XLATE_STRINGS | DCV_TYPELESS_NAMES | DCV_DEREF_ALIASES;
	c &= ~DCV_CANONICALIZE_NAMES;

	if (NWDSSetContext(ctx, DCK_FLAGS, &c)) {
		syslog(LOG_WARNING, "NWDSSetContext(DCK_FLAGS) failed with %s\n", strnwerror(err));
		goto bailout;
	}

	if (NWDSAddConnection(ctx, cn)) {
		syslog(LOG_WARNING, "NWDSAddConnection failed with %s\n", strnwerror(err));
		goto bailout;
	}
	/* test 1.21* WITH PAM first login context is [Root]  (pam has no $HOME)
	   but with xscreensaver, context is red by ncpfs from a nwclient file in user's home
	   and resolution fails !
	   same problem with nds_group checking OK with PAM since both value are realtive to [root]
	   by with xscreensaver , the ctx is set to default, so comparing is done to the wrong group name
	   if (NWDSGetContext(ctx,DCK_NAME_CONTEXT,fqdn))
	   syslog(LOG_WARNING, "NWDSGetContext (DCK_NAME_CONTEXT) failed with %s\n", strnwerror(err));
	   else
	   syslog(LOG_WARNING, "Default context is %s :\n", fqdn);

	 */

// rev 1.25 , implement contextless login if no contexts are provided in the contexts list

	if (!contexts) {
		if (qflag & QF_DEBUG)
			syslog(LOG_DEBUG, "Trying contexless login for %s on %s\n", tree, user);
		if (nw_ctx_search(user, cn, ctxbuf, sizeof (ctxbuf))) {
			err = PAM_AUTH_ERR;
			goto bailout;
		}
		contexts = ctxbuf;
		if (qflag & QF_DEBUG)
			syslog(LOG_DEBUG, "Found  %s for %s\n", ctxbuf, user);
	}

	/* scan the search contexts list */
	ctxStart = contexts;
	do {
		size_t ln;

		err = PAM_AUTH_ERR;
		ln = strlen(user);
		if (ln >= sizeof (fqdn)) {
			syslog(LOG_ERR, "Buffer overflow attack attempted at %s:%u (%s)\n", __FILE__, __LINE__, __FUNCTION__);
			goto bailout;
		}
		memcpy(fqdn, user, ln + 1);
		if (ctxStart) {
			size_t appln;

			ctxEnd = strchr(ctxStart, ',');
			if (ctxEnd) {
				appln = ctxEnd - ctxStart;
				ctxEnd++;
			} else {
				appln = strlen(ctxStart);
			}
			if (ln + 1 + appln >= sizeof (fqdn)) {
				syslog(LOG_ERR, "Buffer overflow attack attempted at %s:%u (%s)\n", __FILE__, __LINE__, __FUNCTION__);
				goto bailout;
			}
			fqdn[ln] = '.';
			memcpy(fqdn + ln + 1, ctxStart, appln);
			fqdn[ln + 1 + appln] = 0;

			ctxStart = ctxEnd;
		}
		if (qflag & QF_DEBUG)
			syslog(LOG_NOTICE, "nw_create_verify_conn_to_tree: trying to resolve  %s\n", fqdn);
		/* rev 1.26 : some support for the same CN in different contexts
		   the password makes the difference */
		err = NWDSMapNameToID(ctx, cn, fqdn, &oid);
		if (!err) {
			if (qflag & QF_DEBUG)
				syslog(LOG_WARNING, "trying to login as %s\n", fqdn);
			err = nds_login_auth(cn, fqdn, pwd);
			if (err && err != NWE_PASSWORD_EXPIRED) {
				syslog(LOG_WARNING, "%s when trying to login\n", strnwerror(err));
				switch (err) {
					/* case 1:good password but some other problem, report failure */
				case NWE_LOGIN_MAX_EXCEEDED:
				case NWE_LOGIN_UNAUTHORIZED_TIME:
				case NWE_LOGIN_UNAUTHORIZED_STATION:
				case NWE_ACCT_DISABLED:
					err = PAM_AUTH_ERR;
					goto bailout;
					/* case 2: one day we should really do something about this ? */
				case NWE_PASSWORD_EXPIRED:
					err = PAM_NEW_AUTHTOK_REQD;
					goto bailout;
					/*case 3: wrong password or other errors. let's try in another context */
				case ERR_NO_SUCH_ENTRY:
				case NWE_SERVER_UNKNOWN:
					err = PAM_USER_UNKNOWN;
					break;
				default:
					err = PAM_AUTH_ERR;
					break;
				}
				//goto bailout; not anymore
			} else
				break;	/* everything is Ok, except maybe password expired */
		} else {
			syslog(LOG_WARNING, "NWDSMapNameToID for %s failed with %s\n", fqdn, strnwerror(err));
		}

	} while (ctxStart);

	if (err) {
		err = PAM_AUTH_ERR;
		goto bailout;
	}
	/* PP:: not very sure this code is STILL relevant against NDS tree */
	err = nw_get_nwid(cn, &oid, qflag);	// needed again ?
	if (err) {
		syslog(LOG_WARNING, "Error %s retrieving user ID for %s\n", strnwerror(err), user);
		goto bailout;
	}
	if (qflag & QF_NOSU) {
		if (oid == 0x00000001) {
			err = PAM_AUTH_ERR;
			syslog(LOG_WARNING, "Access denied for %s/%s because of it is supervisor\n", tree, user);
			goto bailout;
		}
		if (qflag & QF_DEBUG)
			syslog(LOG_DEBUG, "User %s/%s passed supervisor check\n", tree, user);
	}
	if (qflag & QF_NOSUEQ) {
		nuint8 level;

		err = NWGetBinderyAccessLevel(cn, &level, NULL);
		if (err) {
			syslog(LOG_WARNING, "Access denied for %s/%s because of I/O error during object rights verification\n", tree, user);
			err = PAM_AUTH_ERR;
			goto bailout;
		}
		if ((level >= 0x30) || ((level & 0xF) >= 3)) {
			syslog(LOG_WARNING, "Access denied for %s/%s because of it is supervisor equivalent\n", tree, user);
			err = PAM_AUTH_ERR;
			goto bailout;
		}
	}
	if (qflag & QF_DEBUG) {
		char aux[512];
		aux[0] = 0;
		if (nameFormat == NWCC_NAME_FORMAT_NDS_TREE) {
			NWCCGetConnInfo(cn, NWCC_INFO_SERVER_NAME, sizeof (aux), aux);
			syslog(LOG_DEBUG, "User %s was successfully authorized on tree %s by server %s \n", fqdn, tree, aux);
		} else {
			NWCCGetConnInfo(cn, NWCC_INFO_TREE_NAME, sizeof (aux), aux);
			syslog(LOG_DEBUG, "User %s was successfully authorized by NDS server %s on tree %s\n", fqdn, tree, aux);
		}
	}

	if (group) {
		/* v 1.21 */
		err = is_member_of_nds_group(ctx, cn, oid, group);
		if (err) {
			syslog(LOG_WARNING, "%s is not member of NDS %s\n", user, group);
			err = PAM_AUTH_ERR;
			goto bailout;
		}
	}
	if (id)
		*id = oid;
	if (conn)
		*conn = cn;
	else
		NWCCCloseConn(cn);
	NWDSFreeContext(ctx);
	return PAM_SUCCESS;
bailout:;
	/* code to be executed upon any error */
	if (ctx)
		NWDSFreeContext(ctx);
	if (cn)
		NWCCCloseConn(cn);
	return err;
}

static int
nw_attempt_auth_server(pam_handle_t * pamh, const char *server, const char *user, const char *pwd, int qflag, const char *group)
{
	int err;
	NWCONN_HANDLE conn;

	err = nw_create_verify_conn_to_server(&conn, NULL, server, user, pwd, qflag, group);
	if (err)
		return err;
	pam_set_data(pamh, "pam.ncpfs.passwd.conn", conn, nw_cleanup_conn);
	return PAM_SUCCESS;
}

/* trial code server is a Tree or a NDS server... and contexts is a comma separated list of CTX to search */
/* rev 1.25: if contexts is NULL, contextless login is attempted */
static int
nw_attempt_auth_tree(pam_handle_t * pamh,
		     const char *tree, const char *user, const char *contexts, const char *pwd,
		     int qflag, const char *group, nuint nameFormat)	// either  NWCC_NAME_FORMAT_NDS_TREE,  NWCC_NAME_FORMAT_BIND
{
	int err;
	NWCONN_HANDLE conn;

	err = nw_create_verify_conn_to_tree(&conn, NULL, tree, user, contexts, pwd, qflag, group, nameFormat);
	if (err)
		return err;
	pam_set_data(pamh, "pam.ncpfs.passwd.conn", conn, nw_cleanup_conn);
	return PAM_SUCCESS;
}

/********************************************************* part II NDS properties reading *******************/
struct nw_group_info {
	struct nw_group_info *next;
	char *name;
	gid_t gid;
	long zenFlag;		//PP
};

// thes infos will be "dumped" in a ~/nwinfos file that can be "sourced" by the 6 "Zen scripts"
// or any future scripts / applications. (see the pam_session_opening part for the used format
// and script variables names
struct nw_user_info {
	char *name;
	char *gecos;
	char *shell;
	char *dir;
	uid_t uid;
	gid_t gid;
	struct nw_group_info *groups;
	pam_handle_t *pamh;
	struct pam_ncp_state state;
	int qflag;
	char *fqdn;		/* User's FQDN from NDS */
	char *nwhomeServer;	//PP   CN of the server extracted from NDS prop Home Directory
	char *nwhomeVolume;	//PP   real name of the volume (SYS...)extracted from NDS prop Home Directory
	char *nwhomePath;	//PP   "unixified" and "uppercased" extracted from NDS prop Home Directory
	char *nwhomeMntPnt;	//PP   defaut =nwhome , can be changed by -m option
	char *emailSMTP;	//PP   NDS property (obsolete but still used)
	char *emailLDAP;	//PP   NDS property
	char *messageServer;	//PP   CN of the NDS property maybe != authenticating server used by PAM
	char *defaultTree;	//PP   the tree of the server that authenticated him
	char *defaultNameCtx;	//PP   from his canonical name
	long zenFlag;		//PP   see above
	long zenFlagOFF;	//PP   rev 1.26 turn off some zenflags on that workstation
	int isNewUser;		// PP rev 1.19
	int isScreenSaverRelogin;	// PP rev 1.21
};

static void
init_nw_user_info(struct nw_user_info *ui)
{
	ui->pamh = NULL;
	ui->qflag = 0;
	ui->zenFlag = ZF_DEFAULTS;	//PP
	ui->zenFlagOFF = 0;
	ui->name = NULL;
	ui->gecos = NULL;
	ui->shell = NULL;
	ui->dir = NULL;
	ui->uid = (uid_t) -1;
	ui->gid = (gid_t) -1;
	ui->groups = NULL;
	ui->fqdn = NULL;
	ui->nwhomeServer = NULL;	//PP
	ui->nwhomeVolume = NULL;	//PP
	ui->nwhomePath = NULL;	//PP
	ui->nwhomeMntPnt = NULL;	//PP
	ui->emailSMTP = NULL;	//PP
	ui->emailLDAP = NULL;	//PP
	ui->messageServer = NULL;	//PP
	ui->defaultTree = NULL;	//PP
	ui->defaultNameCtx = NULL;	//PP
	ui->isNewUser = 0;	//PP
}

static void
free_nw_user_info(struct nw_user_info *ui)
{
	struct nw_group_info *gi;
	struct nw_group_info *bkgi;

#define FREEFIELD(x) do if (ui->x) { free(ui->x); ui->x = NULL; } while (0)
	FREEFIELD(name);
	FREEFIELD(gecos);
	FREEFIELD(shell);
	FREEFIELD(dir);
	FREEFIELD(fqdn);
	FREEFIELD(nwhomeServer);
	FREEFIELD(nwhomeVolume);
	FREEFIELD(nwhomePath);
	FREEFIELD(nwhomeMntPnt);
	FREEFIELD(emailSMTP);
	FREEFIELD(emailLDAP);
	FREEFIELD(messageServer);
	FREEFIELD(defaultTree);
	FREEFIELD(defaultNameCtx);

#undef FREEFIELD
	for (gi = ui->groups; gi; gi = bkgi) {
		bkgi = gi->next;
		free(gi->name);
		free(gi);
	}
	ui->groups = NULL;
}

// PP:called by PAM at pam_end
// There is a possible problem storing the user_data in PAM during all the session
// If an admin changes something to that user, the data is out on sync
// so we should use this data ONLY in a "PAM_open_session" call back
// that happens just after the successful login
static void
cleanup_user_info(UNUSED(pam_handle_t * pamh), void *data, UNUSED(int error_status))
{
	free_nw_user_info((struct nw_user_info *) data);
}

static int
nw_retrieve_bindery_user_info(struct nw_user_info *ui, NWCONN_HANDLE conn, UNUSED(NWObjectID oid))
{

/* we have to give some support here since:
  1) writing the .nwclient file in user's home (flags NOP) requires that
     either ui.messageServer or ui.nwHomeServer to be not NULL
  2) we need user's home from bindery to mount it (flag H)
      warning: the home MUST be on the authenticating server else
       the bindery property is likely to be empty
 */
	char serverName[128];
	char userName[128];
	char *v;
	NWCCODE err;
	struct nw_property p;
	const char me[] = "nw_retrieve_bindery_user_info";

	err = NWCCGetConnInfo(conn, NWCC_INFO_USER_NAME, sizeof (userName), userName);
	if (err) {
		syslog(LOG_WARNING, "%s:unable to get back user name from connection.\n", me);
		return err;
	}

	err = NWCCGetConnInfo(conn, NWCC_INFO_SERVER_NAME, sizeof (serverName), serverName);
	if (err) {
		syslog(LOG_WARNING, "%s:unable to get back server name from connection.\n", me);
		return err;
	}
	v = strdup(serverName);
	if (!v) {
		syslog(LOG_WARNING, "%s:Not enough memory for strdup()\n", me);
		return ENOMEM;
	}
	ui->messageServer = v;

	memset(&p, 0, sizeof (p));
	err = ncp_read_property_value(conn, NCP_BINDERY_USER, userName, 1, "IDENTIFICATION", &p);
	if (!err && p.value[0]) {
		if (ui->qflag & QF_DEBUG)
			syslog(LOG_NOTICE, "%s:got a full name %s for %s\n ", me, p.value, userName);

		v = strdup(p.value);
		if (!v) {
			syslog(LOG_WARNING, "%s:Not enough memory for strdup()\n", me);
			return ENOMEM;
		}
		ui->gecos = v;
	}
	/*do not rely too much on this with NW4 servers:
	   in my place they do not answer correctly to this call even if
	   the user DO have his home on that server.
	   in contrast, one NW5 server, were the user DO NOT has his home on it
	   DID reply correctly ???
	 */

	memset(&p, 0, sizeof (p));
	err = ncp_read_property_value(conn, NCP_BINDERY_USER, userName, 1, "HOME_DIRECTORY", &p);
	if (!err && p.value[0]) {
		char *v1;
		char *mark = strchr(p.value, ':');
		if (mark && *(mark + 1)) {
			if (ui->qflag & QF_DEBUG)
				syslog(LOG_NOTICE, "%s:got a home directory %s for %s\n ", me, p.value, userName);

			*mark = 0;
			v = strdup(p.value);
			if (!v) {
				syslog(LOG_WARNING, "%s:Not enough memory for strdup()\n", me);
				return ENOMEM;
			}
			v1 = strdup(mark + 1);
			if (!v1) {
				syslog(LOG_WARNING, "%s:Not enough memory for strdup()\n", me);
				return ENOMEM;
			}

			ui->nwhomeServer = strdup(ui->messageServer);
			ui->messageServer = NULL;
			ui->nwhomeVolume = v;
			unixifyPathUC(v1);
			ui->nwhomePath = v1;

		}
	}
	return 0;
}

/******************************************** NDS_Reading ********************/
struct attrop {
	const NWDSChar *attrname;
	NWDSCCODE (*getval)(NWDSContextHandle, const void *val, void *arg);
	enum SYNTAX synt;
};

static NWDSCCODE
nds_read_attrs(NWDSContextHandle ctx, const NWDSChar * objname, void *arg, const struct attrop *atlist)
{
	Buf_T *attrlist;
	Buf_T *info;
	NWDSCCODE dserr;
	const struct attrop *ptr;
	nuint32 iterHandle;

	dserr = NWDSAllocBuf(DEFAULT_MESSAGE_LEN, &attrlist);
	if (dserr) {
		syslog(LOG_WARNING, "NWDSAllocBuf() failed with %s\n", strnwerror(dserr));
		goto bailout;
	}
	dserr = NWDSInitBuf(ctx, DSV_READ, attrlist);
	if (dserr) {
		syslog(LOG_WARNING, "NWDSInitBuf() failed with %s\n", strnwerror(dserr));
		goto bailoutbuf1;
	}
	for (ptr = atlist; ptr->attrname; ptr++) {
		dserr = NWDSPutAttrName(ctx, attrlist, ptr->attrname);
		if (dserr) {
			syslog(LOG_WARNING, "NWDSPutAttrName(%s) failed with %s\n", ptr->attrname, strnwerror(dserr));
			goto bailoutbuf1;
		}
	}
	dserr = NWDSAllocBuf(DEFAULT_MESSAGE_LEN, &info);
	if (dserr) {
		syslog(LOG_WARNING, "NWDSAllocBuf() failed with %s\n", strnwerror(dserr));
		goto bailoutbuf1;
	}
	iterHandle = NO_MORE_ITERATIONS;
	do {
		NWObjectCount attrs;

		dserr = NWDSRead(ctx, objname, DS_ATTRIBUTE_VALUES, 0, attrlist, &iterHandle, info);
		if (dserr) {
			if (dserr == ERR_NO_SUCH_ATTRIBUTE)
				dserr = 0;
			else
				syslog(LOG_WARNING, "NWDSRead() failed with %s\n", strnwerror(dserr));
			goto bailoutbuf2;
		}
		dserr = NWDSGetAttrCount(ctx, info, &attrs);
		if (dserr) {
			syslog(LOG_WARNING, "NWDSGetAttrCount() failed with %s\n", strnwerror(dserr));
			goto bailoutcloit;
		}
		while (attrs--) {
			NWDSChar attrname[MAX_SCHEMA_NAME_BYTES];
			enum SYNTAX synt;
			NWObjectCount vals;

			dserr = NWDSGetAttrName(ctx, info, attrname, &vals, &synt);
			if (dserr) {
				syslog(LOG_WARNING, "NWDSGetAttrName() failed with %s\n", strnwerror(dserr));
				goto bailoutcloit;
			}
			while (vals--) {
				size_t sz;
				void *val;

				dserr = NWDSComputeAttrValSize(ctx, info, synt, &sz);
				if (dserr) {
					syslog(LOG_WARNING, "NWDSComputeAttrValSize() failed with %s\n", strnwerror(dserr));
					goto bailoutcloit;
				}
				val = malloc(sz);
				if (!val) {
					syslog(LOG_WARNING, "malloc() failed with %s\n", strnwerror(ENOMEM));
					goto bailoutcloit;
				}
				dserr = NWDSGetAttrVal(ctx, info, synt, val);
				if (dserr) {
					free(val);
					syslog(LOG_WARNING, "NWDSGetAttrVal() failed with %s\n", strnwerror(dserr));
					goto bailoutcloit;
				}
				for (ptr = atlist; ptr->attrname; ptr++) {
					if (!strcasecmp(ptr->attrname, attrname))
						break;
				}
				if (ptr->getval) {
					if (ptr->synt != synt) {
						syslog(LOG_WARNING, "Incompatible tree schema, %s has syntax %d instead of %d\n", attrname, synt, ptr->synt);
					} else {
						// ajout PP  dserr= !!! en cas de pb mémoire
						dserr = ptr->getval(ctx, val, arg);
					}
				}
				free(val);
				if (dserr) {
					goto bailoutcloit;
				}
			}
		}
	} while (iterHandle != NO_MORE_ITERATIONS);
bailoutcloit:;
	if (iterHandle != NO_MORE_ITERATIONS) {
		// PP let's keep the final dserr as the 'out of memory error' from ptr->getval()
		NWDSCCODE dserr2 = NWDSCloseIteration(ctx, DSV_READ, iterHandle);
		if (dserr2) {
			syslog(LOG_WARNING, "NWDSCloseIteration() failed with %s\n", strnwerror(dserr2));
		}
	}
bailoutbuf2:;
	NWDSFreeBuf(info);
bailoutbuf1:;
	NWDSFreeBuf(attrlist);
bailout:;
	return dserr;
}

/*PP ************************************************* splitting home directory attribute*/

struct nw_home_info {
	char *hs;
	char *hrn;
};

static NWDSCCODE
nds_pp_host_server(UNUSED(NWDSContextHandle ctx), const void *val, void *arg)
{
	struct nw_home_info *hi = (struct nw_home_info *) arg;
	const char *src = val;
	size_t len;
	char *dot;
	char *v;

	dot = strchr(src, '.');
	if (dot) {
		len = dot - src;
	} else {
		len = strlen(src);
	}
	v = malloc((len + 1) * sizeof (char));
	if (!v) {
		syslog(LOG_WARNING, "Not enough memory for strdup()\n");
		return ENOMEM;
	}
	memcpy(v, src, len);
	v[len] = 0;
	hi->hs = v;
	return 0;
}

static NWDSCCODE
nds_pp_host_resource_name(UNUSED(NWDSContextHandle ctx), const void *val, void *arg)
{
	struct nw_home_info *hi = (struct nw_home_info *) arg;

	char *v = strdup((const char *) val);
	if (!v) {
		syslog(LOG_WARNING, "Not enough memory for strdup()\n");
		return ENOMEM;
	}
	hi->hrn = v;
	return 0;
}

static NWDSCCODE
nds_home_info(NWDSContextHandle ctx, const NWDSChar * objname, struct nw_home_info *hi)
{
	static const struct attrop atlist[] = {
		{ATTR_HOST_SERVER, nds_pp_host_server, SYN_DIST_NAME},
		{ATTR_HOST_RN, nds_pp_host_resource_name, SYN_CI_STRING},
		{NULL, NULL, SYN_UNKNOWN}
	};

	return nds_read_attrs(ctx, objname, hi, atlist);
}

/******************************************************************************/

/** gather Unix groups informations */

static NWDSCCODE
nds_ga_group_unixgid(UNUSED(NWDSContextHandle ctx), const void *val, void *arg)
{
	struct nw_group_info *gi = (struct nw_group_info *) arg;

	if (gi->gid == (gid_t) -1) {
		gi->gid = *(const Integer_T *) val;
	}
	return 0;
}

// PP: id no NDS8 is present, collect the group Unix ID from one of the location
// string with the format G:nnn
// This is relevant ONLY if the flag QFC_REQUIRE_SERVER has been set in the command line
// can also used to specify a name of unix group different of the NDS'one
// eg. everyone --> users
// staff --> root
static NWDSCCODE
nds_pp_group_location(UNUSED(NWDSContextHandle ctx), const void *val, void *arg)
{
	struct nw_group_info *gi = (struct nw_group_info *) arg;
	const char *pt = (const char *) val;
	int n;

	//syslog(LOG_NOTICE, "start of NW group location got %s\n ",pt);
	if (strlen(pt) > 2 && pt[1] == ':') {
		const char *cur_pt = pt + 2;
		switch (*pt) {
		case 'g':
		case 'G':
			if (gi->gid == (gid_t) -1) {
				switch (getnumber(&n, &cur_pt)) {
				case 0:
					gi->gid = n;
					break;
				default:
					syslog(LOG_ERR, "Invalid group GID %s for %s\n", pt, gi->name);
				}
			}
			break;
		case 'n':
		case 'N':	// unix equivalent name
			if (!gi->name) {
				char *v = strdup(cur_pt);
				if (!v) {
					syslog(LOG_WARNING, "Not enough memory for strdup()\n");
					return ENOMEM;
				}
				gi->name = v;
			}
			break;
			/* interesting feature: since we OR them, a group may have
			   serveral Z:xxxx strings in the location property */
		case 'z':
		case 'Z':
			gi->zenFlag |= decodeZenFlag(&cur_pt);
			break;
		}
	}
	return 0;
}

static NWDSCCODE
nds_group_info(NWDSContextHandle ctx, const NWDSChar * objname, struct nw_group_info *gi)
{
	static const struct attrop atlist[] = {
		{ATTR_GID, nds_ga_group_unixgid, SYN_INTEGER},
		{ATTR_LOCATION, nds_pp_group_location, SYN_CI_STRING},
		{NULL, NULL, SYN_UNKNOWN}
	};

	gi->gid = (gid_t) -1;
	gi->zenFlag = 0;
	gi->name = NULL;
	return nds_read_attrs(ctx, objname, gi, atlist);
}

// PP:called only if no "Unix name has been found in NDS for that group
// a string N:nnnnn in L attribute
static NWDSCCODE
nds_group_name(UNUSED(NWDSContextHandle ctx), const NWDSChar * objname, struct nw_group_info *gi)
{
	char *buff;
	const unsigned char *f;
	int c;

	/* Worst case: output is Z, two chars per one in input, and zero terminating byte */
	gi->name = buff = malloc(strlen(objname) * 2 + 2);
	if (!buff) {
		syslog(LOG_WARNING, "Not enough memory for strdup()\n");
		return ENOMEM;
	}
	f = objname;
	for (; (c = *f++) != 0; *buff++ = c) {
		if (c >= 'a' && c <= 'z')
			continue;
		if (c >= 'A' && c <= 'Z') {
			c += 'a' - 'A';
			continue;
		}

		if (gi->name == buff)
			*buff++ = 'Z';
// PP: why ? a group starting by a digit is not legal?
// 24/01/2001 I know why now !!!  grpadd will add user to group 1, 2 ... (bin, daemon !!!)
		if (c >= '0' && c <= '9')
			continue;
		if (c == '_' || c == ' ') {
			c = '_';
			continue;
		}
		if (c == '.') {	//stop at first dot
			if (1)
				break;
			c = 'U';
			continue;
		}
		*buff++ = 'A' + (c >> 4);
		c = 'A' + (c & 0xF);
	}
	*buff = 0;
	return 0;
}

static int nw_update_group_info(struct nw_user_info *ui, struct nw_group_info *gi);

static int
rqgroup(struct nw_user_info *ui, NWDSContextHandle ctx, const NWDSChar * objname, int primary)
{
	struct nw_group_info gi;
	struct group *grp;
	struct nw_group_info *i;
	int err;

	//syslog(LOG_NOTICE, "call of rggroup for %s\n ",objname);
	err = nds_group_info(ctx, objname, &gi);
	if (err)
		return err;
	ui->zenFlag |= gi.zenFlag;	// merge group zenFlag to user's (rev 1.25 now, so it is done even if group cannot be autocreated)
	if (!gi.name)		// no alias sent by NDS
		if (nds_group_name(ctx, objname, &gi))	// make a name
			return -1;
	setgrent();		//PP
	grp = getgrnam(gi.name);
	endgrent();		//PP
	if (grp) {
		gi.gid = grp->gr_gid;

	} else {
		err = nw_update_group_info(ui, &gi);
		if (err)
			return err;
	}
	if (primary) {
		ui->gid = gi.gid;
	}
	// PP no duplicates in group list. may happen with an alias sent by NDS
	for (i = ui->groups; i; i = i->next) {
		if (!strcmp(gi.name, i->name))
			break;
	}
	if (!i) {
		i = malloc(sizeof (*i));
		if (!i)
			return ENOMEM;
		i->name = gi.name;
		i->gid = gi.gid;
		i->next = ui->groups;
		i->zenFlag = gi.zenFlag;
		ui->groups = i;
	}
	return 0;
}

static int
build_groups_list(const struct nw_user_info *ui, char **list)
{
	const struct nw_group_info *gi;
	size_t ln;
	char *p;

	*list = NULL;
	ln = 1;
	for (gi = ui->groups; gi; gi = gi->next)
		ln += strlen(gi->name) + 1;
	if (ln == 1)
		return 0;

	p = (char *) malloc(ln);
	if (!p)
		return ENOMEM;
	*list = p;
	for (gi = ui->groups; gi; gi = gi->next) {
		ln = strlen(gi->name);
		memcpy(p, gi->name, ln);
		p += ln;
		*p++ = ',';
	}
	*--p = 0;
	//syslog(LOG_NOTICE, "end of build group list got %s\n ",*list);
	return 0;
}

/************************************ helper functions to extract some user's properties **/
static NWDSCCODE
nds_ga_unixuid(UNUSED(NWDSContextHandle ctx), const void *val, void *arg)
{
	struct nw_user_info *ui = (struct nw_user_info *) arg;
	if (ui->uid == (uid_t) -1) {
		ui->uid = *(const Integer_T *) val;
		// talk a bit (real NDS8 attribute or dummy ?)
		if (ui->qflag & QF_DEBUG)
			syslog(LOG_NOTICE, "got a Unix ID %d from %s\n ", ui->uid, ATTR_UID);
	}
	return 0;
}

static NWDSCCODE
nds_ga_unixpgid(UNUSED(NWDSContextHandle ctx), const void *val, void *arg)
{
	struct nw_user_info *ui = (struct nw_user_info *) arg;

	if (ui->gid == (gid_t) -1) {
		ui->gid = *(const Integer_T *) val;
		// talk a bit (real NDS8 attribute or dummy ?)
		if (ui->qflag & QF_DEBUG)
			syslog(LOG_NOTICE, "got a Unix PGID %d from %s\n ", ui->gid, ATTR_PGID);
	}
	return 0;
}

// PP this is the same founction as below ???
// does Netware has two synonyms for the same property (UNIX:GID"
// and UNIX:Primary GroupID???
static NWDSCCODE
nds_ga_unixgid(UNUSED(NWDSContextHandle ctx), const void *val, void *arg)
{
	struct nw_user_info *ui = (struct nw_user_info *) arg;

	if (ui->gid == (gid_t) -1) {
		ui->gid = *(const Integer_T *) val;
		// talk a bit (real NDS8 attribute or dummy ?)
		if (ui->qflag & QF_DEBUG)
			syslog(LOG_NOTICE, "got a Unix GID %d from %s\n ", ui->gid, ATTR_GID);
	}
	return 0;
}

static NWDSCCODE
nds_ga_unixhome(UNUSED(NWDSContextHandle ctx), const void *val, void *arg)
{
	struct nw_user_info *ui = (struct nw_user_info *) arg;

	if (!ui->dir) {
		char *v = strdup((const char *) val);
		if (!v) {
			syslog(LOG_WARNING, "Not enough memory for strdup()\n");
			return ENOMEM;
		}
		ui->dir = v;
		// talk a bit (real NDS8 attribute or dummy ?)
		if (ui->qflag & QF_DEBUG)
			syslog(LOG_NOTICE, "got a Unix Home %s from %s\n ", ui->dir, ATTR_HOME);
	}
	return 0;
}

static NWDSCCODE
nds_ga_unixshell(UNUSED(NWDSContextHandle ctx), const void *val, void *arg)
{
	struct nw_user_info *ui = (struct nw_user_info *) arg;

	if (!ui->shell) {
		char *v = strdup((const char *) val);
		if (!v) {
			syslog(LOG_WARNING, "Not enough memory for strdup()\n");
			return ENOMEM;
		}
		ui->shell = v;
		// talk a bit (real NDS8 attribute or dummy ?)
		if (ui->qflag & QF_DEBUG)
			syslog(LOG_NOTICE, "got a Unix shell %s from %s\n ", ui->shell, ATTR_SHELL);
	}
	return 0;
}

static NWDSCCODE
nds_update_gecos(struct nw_user_info *ui, const char *str)
{
	char *v;
	size_t sadd = strlen(str) + 1;

	if (ui->gecos) {	// already got the name
		size_t sold = strlen(ui->gecos);

		v = realloc(ui->gecos, sold + 1 + sadd);
		if (!v) {
			syslog(LOG_WARNING, "Not enough memory for strdup()\n");
			return ENOMEM;
		}
		v[sold] = ',';
		memcpy(v + 1, str, sadd);
	} else {
		v = malloc(sadd);
		if (!v) {
			syslog(LOG_WARNING, "Not enough memory for strdup()\n");
			return ENOMEM;
		}
		memcpy(v, str, sadd);
	}
	ui->gecos = v;
	return 0;
}

// PP we append the Comment after the full name, separated by a comma
static NWDSCCODE
nds_ga_unixcomment(UNUSED(NWDSContextHandle ctx), const void *val, void *arg)
{
	struct nw_user_info *ui = (struct nw_user_info *) arg;

	// talk a bit (real NDS8 attribute or dummy ?)
	if (ui->qflag & QF_DEBUG)
		syslog(LOG_NOTICE, "got a Unix Comment %s from %s\n ", (const char *) val, ATTR_COM);
	return nds_update_gecos(ui, (const char *) val);
}

// PP can be any naming attribute returning a SYN_CI_STRING see define before nds_user_info()
// PP we add the name before any comment that can be there
static NWDSCCODE
nds_ga_gecos(UNUSED(NWDSContextHandle ctx), const void *val, void *arg)
{
	struct nw_user_info *ui = (struct nw_user_info *) arg;
	NWDSCCODE err;

	if (ui->qflag & QF_DEBUG) {
		syslog(LOG_NOTICE, "before full name gecos is %s\n ", ui->gecos ? : "(null)");
	}
	err = nds_update_gecos(ui, (const char *) val);
	if (err) {
		return err;
	}
	if (ui->qflag & QF_DEBUG) {
		syslog(LOG_NOTICE, "after full name gecos is %s\n ", ui->gecos);
	}
	return 0;
}

static NWDSCCODE
nds_ga_unixpgname(NWDSContextHandle ctx, const void *val, void *arg)
{
	struct nw_user_info *ui = (struct nw_user_info *) arg;

	// talk a bit (real NDS8 attribute or dummy ?)
	if (ui->qflag & QF_DEBUG)
		syslog(LOG_NOTICE, "got a Unix PGroup Name %s from %s\n ", (const char *) val, ATTR_PGNAME);
	if (ui->gid == (gid_t) -1) {
		rqgroup(ui, ctx, val, 1);
	}
	return 0;
}

static NWDSCCODE
nds_ga_group_membership(NWDSContextHandle ctx, const void *val, void *arg)
{
	struct nw_user_info *ui = (struct nw_user_info *) arg;

	rqgroup(ui, ctx, val, 0);
	return 0;
}

#ifdef USE_POSTAL_ADDRESS
// PP: id no NDS8 is present, collect the user's basic  Unix informations from
// 6 fields of the Postal Address attribute
// This is relevant ONLY if the flag QFC_REQUIRE_SERVER has been set in the command line
// and if zenFlag processing is activated.
// some people may prefer the postal address instead of the Location property
// used here by default.
// advantages: can be directty edited.No need to prepend X:
// disadvantages: limited to 30 chars and 6 strings
//    The location attribute is more convenient
//    ( no limit of number of strings and 128 chars rather than 30).

static NWDSCCODE
nds_pp_postal_address(NWDSContextHandle ctx, const void *val, void *arg)
{
	struct nw_user_info *ui = (struct nw_user_info *) arg;
	Postal_Address_T *pa = (Postal_Address_T *) val;
	int i, n;
	const char *pt;
	//rqgroup(ui, ctx, val, 0);
	// adress 0 = unix UID        leading spaces not significant
	// adress 1= unix primary GID leading spaces not significant
	// adress 2= unix home dir    spaces significant
	// adress 3= unix shell       spaces  significant
	// adress 4= other group name spaces  turned to _
	//       ONLY ONE: do not use a comma separated list group1,group2,.....
	// adress 5= Zen flags        spaces ignored
/* note that this attribute is not mandatory
so if it does not exists that "call back" is not called */

	if (ui->qflag & QF_DEBUG)
		syslog(LOG_NOTICE, "start of PO address\n");
	for (i = 0; i < 6; i++) {
		pt = (const char *) (*pa)[i];
		if (ui->qflag & QF_DEBUG)
			syslog(LOG_NOTICE, "reading po_ad %d %s ", i, pt);
		// caution it may be there but empty (pointing to à 0) !!!
		if (pt && strlen(pt))
			switch (i) {
			case 0:
				if (ui->uid == (uid_t) -1) {	// do not overwrite a DS 8 answer
					const char *cur_pt = pt;
					switch (getnumber(&n, &cur_pt)) {
					case 0:
						ui->uid = n;
						break;
					default:
						syslog(LOG_ERR, "Invalid user ID %s for users %s\n", *pt, ui->name);
					}
				}
				break;
			case 1:
				if (ui->gid == (gid_t) -1) {	// do not overwrite a DS 8 answer
					const char *cur_pt = pt;
					switch (getnumber(&n, &cur_pt)) {
					case 0:
						ui->gid = n;
						break;
					default:
						syslog(LOG_ERR, "Invalid primary user GID %s for user %s\n", *pt, ui->name);
					}
				}
				break;
			case 2:
				if (!ui->dir) {	// do not overwrite a DS 8 answer
					char *v = strdup(pt);
					if (!v) {
						syslog(LOG_WARNING, "Not enough memory for strdup()\n");
						return ENOMEM;
					}

					ui->dir = v;
				}
				break;
			case 3:
				if (!ui->shell) {	// do not overwrite a DS 8 answer
					char *v = strdup(pt);
					if (!v) {
						syslog(LOG_WARNING, "Not enough memory for strdup()\n");
						return ENOMEM;
					}
					ui->shell = v;
				}
				break;
			case 4:	// other group name: only one may be specified, unless we
				// separe them by comma and parse. but limited to 30 chars
				//(one of the reasons why we dropped Postal Address as a substitue for NDS8)
				rqgroup(ui, ctx, val, 0);
				break;

			case 5:	// ZenFlag per user ABCDEFGHIJKLMNOPQRSTUVWXYZ  (30 max!!!)
				ui->zenFlag |= decodeZenFlag(&pt);
				break;
			}
	}
	if (ui->qflag & QF_DEBUG)
		syslog(LOG_NOTICE, "end of PO address\n");
	return 0;
}
#else
// PP: id no NDS8 is present, collect the user's basic  Unix informations from  the location
// strings with the format X:nnnnnnnn  , X = [U,G,H,S,P,O,C,Z] upper of lower case
// This is relevant ONLY if the flag QFC_REQUIRE_SERVER has been set in the command line
// Of course, even if NDS8 IS present, we still look at these, just in case the migration
// is not complete and to look for the user's ZENFLAG

// feature I: since zenflags are ORED, a user can have SEVERAL z:xxxx strings in his location strings
// better than deleting and recreating it !!!
// feature II: several O:groups strings are possible ( secondary groups defintion)
// DO NOT use o:group1,group2.....
static NWDSCCODE
nds_pp_location(NWDSContextHandle ctx, const void *val, void *arg)
{
	struct nw_user_info *ui = (struct nw_user_info *) arg;
	const char *pt = (const char *) val;
	char *v;
	int n;
	int err;

	if (ui->qflag & QF_DEBUG)
		syslog(LOG_NOTICE, "start of NW location got %s\n ", pt);

	if (strlen(pt) > 2 && pt[1] == ':') {
		const char *cur_pt = pt + 2;
		switch (*pt) {
		case 'u':	//user ID leading spaces not significant
		case 'U':
			if (ui->uid == (uid_t) -1) {	// do not overwrite a DS 8 answer
				switch (getnumber(&n, &cur_pt)) {
				case 0:
					ui->uid = n;
					break;
				default:
					syslog(LOG_ERR, "Invalid user ID %s\n", pt);
				}
			}
			break;
		case 'g':	// primary group number GID leading spaces not significant
		case 'G':
			if (ui->gid == (gid_t) -1) {	// do not overwrite a DS 8 answer
				switch (getnumber(&n, &cur_pt)) {
				case 0:
					ui->gid = n;
					break;
				default:
					syslog(LOG_ERR, "Invalid primary user GID %s\n", pt);
				}
			}
			break;
		case 'p':	// primary group name, illegal chars converted by nds_group_name
		case 'P':
			if (ui->gid == (gid_t) -1)	// do not overwrite a DS 8 answer
				rqgroup(ui, ctx, cur_pt, 1);
			break;
		case 'h':	// home Unix all spaces significant (must have none ?)
		case 'H':
			if (!ui->dir) {	// do not overwrite a DS 8 answer
				v = strdup(cur_pt);
				if (!v) {
					syslog(LOG_WARNING, "Not enough memory for strdup()\n");
					return ENOMEM;
				}

				ui->dir = v;
			}
			break;
		case 's':	//shell Unix all spaces significant (must have none ?)
		case 'S':
			if (!ui->shell) {	// do not overwrite a DS 8 answer
				v = strdup(cur_pt);
				if (!v) {
					syslog(LOG_WARNING, "Not enough memory for strdup()\n");
					return ENOMEM;
				}
				ui->shell = v;
			}
			break;
		case 'c':	// comment all spaces significant. Will be appended to the gecos naming
		case 'C':	// attribute with a comma and set by calling chfn -f xxxx -o xxxx
			// if comma are present in the string chfn will fails
			if (ui->qflag & QF_DEBUG) {
				syslog(LOG_NOTICE, "before comment gecos is %s\n ", ui->gecos);
			}
			err = nds_update_gecos(ui, cur_pt);
			if (err) {
				return err;
			}
			if (ui->qflag & QF_DEBUG) {
				syslog(LOG_NOTICE, "gecos %s\n ", ui->gecos);
			}
			break;
		case 'o':	// other group names
		case 'O':	// wze can have several entries o:group (but not o:group1,group2...)
			rqgroup(ui, ctx, cur_pt, 0);
			break;

		case 'z':	// ZenFlag per user ABCDEFGHIJKLMNOPQRSTUVWXYZ012345 (32 max)
		case 'Z':
			if (ui->qflag & QF_DEBUG)
				syslog(LOG_NOTICE, "before decode ZF is %s\n ", cur_pt);
			ui->zenFlag |= decodeZenFlag(&cur_pt);
			if (ui->qflag & QF_DEBUG)
				syslog(LOG_NOTICE, "after decode ZF is %lx\n ", ui->zenFlag);
			break;
		}
	}
	return 0;
}
#endif

static NWDSCCODE nds_pp_home_directory(NWDSContextHandle ctx,
					 const void* val,
					 void* arg) {
	struct nw_user_info *ui = (struct nw_user_info *) arg;
	const Path_T *pa = (const Path_T *) val;
	NWDSCCODE dserr;
	struct nw_home_info hi = { NULL, NULL };
	char *v;
	char * p;


	if (ui->qflag & QF_DEBUG) syslog(LOG_NOTICE, "start of NW home dir got %s %s \n",pa->volumeName,pa->path);
	dserr = nds_home_info(ctx, (char *) pa->volumeName, &hi);
	if (dserr)
		return dserr;
	if (ui->qflag & QF_DEBUG) syslog(LOG_NOTICE, "got %s %s ",hi.hs,hi.hrn);

	ui->nwhomeServer = hi.hs;
	ui->nwhomeVolume = hi.hrn;

	// revision 1.27. Do something smart ;-) if %LOGIN_ or %CN in the path to home.
	if ((p = strstr(pa->path,"%CN")) != NULL) {
		*p=0; // chop off %CN and replace by user's name
		v = concat2m(pa->path,ui->name);
	} else if ((p = strstr(pa->path, "%LOG")) != NULL) { // chop off %LOGIN_NAME or  and replace by user's name up to 8 chars
		char aux[9];
		*p=0;

		strncpy(aux, ui->name, 8);
		aux[8] = 0;
		v = concat2m(pa->path, aux);
	} else {
		v = strdup(pa->path);
	}
	if (!v) {
		syslog(LOG_WARNING, "Not enough memory for strdup()\n");
		return ENOMEM;
	}
	unixifyPathUC(v);
	ui->nwhomePath = v;
	if (ui->qflag & QF_DEBUG)
		 syslog(LOG_NOTICE, "end of NW home dir: final path UNIX %s \n",ui->nwhomePath);
	return 0;
}

static NWDSCCODE nds_pp_smtp_email_address(UNUSED(NWDSContextHandle ctx),
					   const void* val,
					   void* arg) {
	struct nw_user_info *ui = (struct nw_user_info *) arg;
	const EMail_Address_T *em = (const EMail_Address_T *) val;
	char *v;

	if (ui->qflag & QF_DEBUG) syslog(LOG_NOTICE, "start of NW smtp email got %u %s\n",em->type,em->address);
	// pick up only the first with type=0 (SMTP) !
	if ((!ui->emailSMTP && em->type == 0) && (!strncmp(em->address, "SMTP:", 5))) {
		// skip header SMTP: must be if type is 0 ?
		v = strdup(em->address + 5);
		if (!v) {
			syslog(LOG_WARNING, "Not enough memory for strdup()\n");
			return ENOMEM;
		}
		ui->emailSMTP = v;

	}
	if (ui->qflag & QF_DEBUG)
		syslog(LOG_NOTICE, "end of NW smtp email GOT %s\n", ui->emailSMTP);
	return 0;
}

static NWDSCCODE
nds_pp_ldap_email_address(UNUSED(NWDSContextHandle ctx), const void *val, void *arg)
{
	struct nw_user_info *ui = (struct nw_user_info *) arg;

	if (ui->qflag & QF_DEBUG)
		syslog(LOG_NOTICE, "start of NW ldap email got %s\n", (const char *) val);

	// pick up only the first of a multi-valued attribute !
	if (!ui->emailLDAP) {
		char *v = strdup((const char *) val);
		if (!v) {
			syslog(LOG_WARNING, "Not enough memory for strdup()\n");
			return ENOMEM;
		}
		trim(v);	// remove the leading space added by other PP utilities
		ui->emailLDAP = v;
	}
	if (ui->qflag & QF_DEBUG)
		syslog(LOG_NOTICE, "end of NW ldap email GOT [%s]\n", ui->emailLDAP);
	return 0;
}

static NWDSCCODE
nds_pp_message_server(UNUSED(NWDSContextHandle ctx), const void *val, void *arg)
{
	struct nw_user_info *ui = (struct nw_user_info *) arg;

	if (ui->qflag & QF_DEBUG)
		syslog(LOG_NOTICE, "start of NW message server got %s\n", (const char *) val);

	if (!ui->messageServer) {
		char *v;
		const char *dot;
		const char *str = val;
		size_t ln;

		dot = strchr(str, '.');
		if (dot) {
			ln = dot - str;
		} else {
			ln = strlen(str);
		}
		v = malloc(ln + 1);
		if (!v) {
			syslog(LOG_WARNING, "Not enough memory for strdup()\n");
			return ENOMEM;
		}
		memcpy(v, str, ln);
		v[ln] = 0;
		ui->messageServer = v;
	}
	if (ui->qflag & QF_DEBUG)
		syslog(LOG_NOTICE, "end of NW message server GOT [%s]\n", ui->messageServer);
	return 0;
}

/*****************************************************GET ALL USER INFO FROM NDS *************/

static int
nds_user_info(NWDSContextHandle ctx, const NWDSChar * objname, struct nw_user_info *ui)
{
	static const struct attrop atlist[] = {
		{ATTR_UID, nds_ga_unixuid, SYN_INTEGER},
		{ATTR_PGNAME, nds_ga_unixpgname, SYN_DIST_NAME},
		{ATTR_PGID, nds_ga_unixpgid, SYN_INTEGER},
		{ATTR_GID, nds_ga_unixgid, SYN_INTEGER},
		{ATTR_HOME, nds_ga_unixhome, SYN_CE_STRING},
		{ATTR_SHELL, nds_ga_unixshell, SYN_CE_STRING},
		{ATTR_COM, nds_ga_unixcomment, SYN_CI_STRING},
		{ATTR_GECOS, nds_ga_gecos, SYN_CI_STRING},
		{ATTR_GRP_MBS, nds_ga_group_membership, SYN_DIST_NAME},
		 /*PP*/ {ATTR_HOME_NW, nds_pp_home_directory, SYN_PATH},
		 /*PP*/ {ATTR_SMTP_EMAIL, nds_pp_smtp_email_address, SYN_EMAIL_ADDRESS},
		 /*PP*/ {ATTR_LDAP_EMAIL, nds_pp_ldap_email_address, SYN_CI_STRING},
		 /*PP*/ {ATTR_MESSAGE_SERVER, nds_pp_message_server, SYN_DIST_NAME},
		{NULL, NULL, SYN_UNKNOWN}
	};

	static const struct attrop atlist2[] = {
#ifdef USE_POSTAL_ADDRESS
		 /*PP*/ {ATTR_POSTAL_ADDRESS, nds_pp_postal_address, SYN_PO_ADDRESS},
#else
		 /*PP*/ {ATTR_LOCATION, nds_pp_location, SYN_CI_STRING},
#endif
		{NULL, NULL, SYN_UNKNOWN}
	};

	int err;

// we must do TWO NDS queries since NDS does not return attributes in this order
// studies of /var/log/secure showed that L attribute usually come out before the NDS8 ones !
	err = nds_read_attrs(ctx, objname, ui, atlist);
	if (err)
		return err;
	if (ui->qflag & QF_DEBUG) {
#ifdef USE_POSTAL_ADDRESS
		syslog(LOG_NOTICE, "using postal address attribute\n");
#else
		syslog(LOG_NOTICE, "using location attribute\n");
#endif
	}
	return nds_read_attrs(ctx, objname, ui, atlist2);
}

/****************** main NDS reading function ********************************************/
static int
nw_retrieve_nds_user_info(struct nw_user_info *ui, NWCONN_HANDLE conn, NWObjectID oid)
{
	NWDSContextHandle ctx;
	nuint32 c;
	NWDSChar username[MAX_DN_BYTES];
	int err;

	err = NWDSCreateContextHandle(&ctx);
	if (err) {
		syslog(LOG_WARNING, "NWDSCreateContextHandle() failed with %s\n", strnwerror(err));
		err = PAM_SYSTEM_ERR;
		goto bailout;
	}
	c = DCV_XLATE_STRINGS | DCV_TYPELESS_NAMES | DCV_DEREF_ALIASES;
	err = NWDSSetContext(ctx, DCK_FLAGS, &c);
	if (err) {
		syslog(LOG_WARNING, "NWDSSetContext() failed with %s\n", strnwerror(err));
		err = PAM_SYSTEM_ERR;
		goto bailoutctx;
	}
	NWDSAddConnection(ctx, conn);
	err = NWDSMapIDToName(ctx, conn, oid, username);
	if (err) {
		syslog(LOG_WARNING, "NWDSMapIDToName() failed with %s\n", strnwerror(err));
		err = PAM_USER_UNKNOWN;
		goto bailoutctx;
	}
	if (ui->qflag & QF_DEBUG)
		syslog(LOG_DEBUG, "User has DN %s\n", username);
	err = nds_user_info(ctx, username, ui);
	if (err) {
		syslog(LOG_NOTICE, "Could not retrieve nds user info: %s\n", strnwerror(err));
		err = PAM_AUTHTOK_ERR;
		goto bailoutctx;
	}
	{
		ui->fqdn = strdup(username);
		if (!ui->fqdn) {
			syslog(LOG_WARNING, "Not enough memory for strdup()\n");
			err = PAM_SYSTEM_ERR;
			goto bailoutctx;
		}
	}
//PP a good spot to retrieve user's defaultNameCtx and defaultTree
	{
		char *p = username;
		char tn[MAX_TREE_NAME_CHARS + 1];

		while (*p && (*p != '.'))
			p++;	//luckily we are in XLATE_STRINGS[TYPELESS_NAMES !!!
		if (*p && *(p + 1)) {	// skip also the first dot
			ui->defaultNameCtx = strdup(p + 1);
			if (!ui->defaultNameCtx) {
				syslog(LOG_WARNING, "Not enough memory for strdup()\n");
				err = ENOMEM;
			}
		}
		if (!(err = NWCCGetConnInfo(conn, NWCC_INFO_TREE_NAME, sizeof (tn), tn))) {
			ui->defaultTree = strdup(tn);
			if (ui->qflag & QF_DEBUG)
				syslog(LOG_NOTICE, "NWCCGetConnInfo(NWCC_INFO_TREE_NAME) returned %s\n", tn);
			if (!ui->defaultTree) {
				syslog(LOG_WARNING, "Not enough memory for strdup()\n");
				err = ENOMEM;
			}
		} else {
			if (ui->qflag & QF_DEBUG)
				syslog(LOG_WARNING, "NWCCGetConnInfo(NWCC_INFO_TREE_NAME) returned %x\n", err);
			err = 0;	// not lethal !
		}
	}
//end PP
	if (ui->qflag & QF_DEBUG)
		syslog(LOG_NOTICE, "end of retrieve nds user info code: %s\n", strnwerror(err));
bailoutctx:;
	NWDSFreeContext(ctx);
bailout:;
	if (!err & (ui->qflag & QF_DEBUG))
		syslog(LOG_NOTICE, "%u %u %s %s %s\n", ui->uid, ui->gid, ui->dir, ui->gecos, ui->shell);
	return err;
}

/******************************************************************************************/

static int
nw_retrieve_user_info(struct nw_user_info *ui)
{
	struct ncp_conn *conn;
	long err;
	NWObjectID oid;

	err = my_pam_get_data(ui->pamh, "pam.ncpfs.passwd.conn", &conn);
	if (err)
		return err;

	err = nw_get_nwid(conn, &oid, ui->qflag);
	if (err)
		return err;

	if (ui->qflag & QF_BINDERY) {
		return nw_retrieve_bindery_user_info(ui, conn, oid);
	} else {
		return nw_retrieve_nds_user_info(ui, conn, oid);
	}
}

static int
exechelper(const char *program, const char *argv[], const char *username)
{
	int i;
	int err;

	i = fork();
	if (i < 0) {
		err = errno;
		syslog(LOG_ERR, "Cannot fork: %s\n", strerror(err));
		return err;
	}
	if (i) {
		int status;

		switch (waitpid(i, &status, 0)) {
		case (pid_t)-1:
			syslog(LOG_ERR, "waitpid unexpectedly terminated: %s\n", strerror(errno));
			return -1;
		case 0:
			syslog(LOG_ERR, "waitpid: Fatal: No child processes\n");
			return -1;
		default:
			if (!WIFEXITED(status)) {
				syslog(LOG_ERR, "%s killed by signal\n", program);
				return -1;
			}
			if (WEXITSTATUS(status)) {
				syslog(LOG_ERR, "%s finished with error %d\n", program, WEXITSTATUS(status));
				return -1;
			}
		}
		return 0;
	} else {
		int fd;

		fd = open("/dev/null", O_RDWR);
		if (fd == -1) {
			err = errno;
			syslog(LOG_ERR, "Cannot open /dev/null: %s\n", strerror(err));
			exit(126);
		}
		dup2(fd, 0);
		dup2(fd, 1);
		dup2(fd, 2);
		argv[0] = program;

		if (username) {	// run child process as username
			struct passwd *pwd = getpwnam(username);
			//syslog(LOG_ERR, " as user %s ..",username);
			if (!pwd) {
				syslog(LOG_ERR, "Oops, something wicked happened, user %s does not exist...", username);
				exit(111);
			}
			if (initgroups(username, pwd->pw_gid)) {
				/* initgroups() fails on shutdown as we 
				   do not run as 'root' at shutdown time...
				   Just ignore error from initgroups,
				   but treat setgid/setuid problems still
				   as fatal. */
				if (errno != EPERM) {
					syslog(LOG_ERR, "Oops, initgroups failed for user %s: %s\n", username, strerror(errno));
					exit(112);
				}
			}
			if (setgid(pwd->pw_gid)) {
				syslog(LOG_ERR, "Oops, setgid failed for user %s: %s\n", username, strerror(errno));
				exit(113);
			}
			if (setuid(pwd->pw_uid)) {
				syslog(LOG_ERR, "Oops, setuid failed for user %s: %s\n", username, strerror(errno));
				exit(114);
			}
		}
		execv(program, (char *const *) (unsigned long) argv);
		err = errno;
		syslog(LOG_ERR, "Cannot execute %s: %s\n", program, strerror(err));
		exit(127);
	}
}

static int
groupadd(const char *name, gid_t gid, int verbose)
{
	char gidstr[30];
	const char *argv[10];
	int err;

	sprintf(gidstr, "%u", gid);

	argv[1] = "-g";
	argv[2] = gidstr;
	argv[3] = name;
	argv[4] = NULL;

//TODO: it would be nice to have the name of the adding group utility
// in some config file (/etc/pam_ncp.conf)
// so we could give more flexibility to the group creation ( a good shell script)
	err = exechelper("/usr/sbin/groupadd", argv, NULL);
	if (err)
		return err;
	if (verbose)
		syslog(LOG_NOTICE, "Group %s(%u) created\n", name, gid);
	return 0;
}

static int
do_chfn(const char *uname, const char *gecos, int verbose)
{
	char *gecosbuf;
	char *p;
	const char *argv[10];
	int err;
	int i = 1;

	gecosbuf = strdup(gecos);
	if (!gecosbuf) {
		syslog(LOG_WARNING, "Not enough memory for gecos buffer\n");
		return ENOMEM;
	}
	p = strchr(gecosbuf, ',');
	if (p) {
		*p++ = '\0';
	} else {
		p = (char*)"";
	}

	argv[i++] = "-f";
	argv[i++] = gecosbuf;

	argv[i++] = "-o";
	argv[i++] = p;

	argv[i++] = uname;
	argv[i++] = NULL;

	/******/
	if (verbose) {
		int j;
		char s[8192];
		char *pos = s;
		size_t space = sizeof (s) - 1;

		for (j = 1; j < i; j++) {
			size_t ln = strlen(argv[j]);

			if (ln > space) {
				ln = space;
			}
			memcpy(pos, argv[j], ln);
			pos += ln;
			space -= ln;
			if (space) {
				*pos++ = ' ';
				space--;
			}
		}
		*pos = 0;
		syslog(LOG_WARNING, "%s", s);
	}
//TODO: it would be nice to have the name of the chfn utility
// in some config file (/etc/pam_ncp.conf)
	err = exechelper("/usr/bin/chfn", argv, NULL);
	if (verbose)
		syslog(LOG_NOTICE, "chfn (%s) for user %s ended with error code %d\n", gecos, uname, err);

	return err;
}

static int
usermod(const char *uname, gid_t gid, const char *gecos, const char *dir, const char *shell,
/*PP added */ const char *grplist, int verbose)
{

	char gidstr[30];
	const char *argv[30];
	int err;
	int i, j;
	int usechfn = 0;

	i = 1;
	if (gid != (gid_t) -1) {
		sprintf(gidstr, "%u", gid);
		argv[i++] = "-g";
		argv[i++] = gidstr;
	}
	if (gecos) {		// donne un erreur 3 !!!
		if (!strchr(gecos, ',')) {
			argv[i++] = "-c";
			argv[i++] = gecos;
		} else {
			usechfn = 1;
		}
	}
	if (dir) {
		argv[i++] = "-d";
		argv[i++] = dir;
	}
	if (shell) {
		argv[i++] = "-s";
		argv[i++] = shell;
	}
/******************** OK with COL 24 usermod !!! ****/
	if (grplist) {
		argv[i++] = "-G";
		argv[i++] = grplist;
	}

	argv[i++] = uname;
	argv[i] = NULL;
	// temp debug
	if (verbose) {
		for (j = 1; j < i; j++)
			syslog(LOG_NOTICE, "usermod %u %s", j, argv[j]);
	}
//TODO: it would be nice to have the name of the modif user utility
// in some config file (/etc/pam_ncp.conf)
// so we could give more flexibility to the user modification ( a good shell script)
	err = exechelper("/usr/sbin/usermod", argv, NULL);
	if (err)
		return err;

	if (usechfn) {
		if (verbose)
			syslog(LOG_NOTICE, "User %s has a comma in his gecos %s\n", uname, gecos);
								/*err= */ do_chfn(uname, gecos, verbose);
								//don't return err, it is not lethal
	}
	if (verbose)
		syslog(LOG_NOTICE, "User %s modified\n", uname);

	return 0;
}

#define concat2a(one,two)	({		\
	size_t lone = strlen(one);		\
	size_t ltwo = strlen(two);		\
	char* buf = alloca(lone + ltwo + 1);	\
	memcpy(buf, one, lone);			\
	memcpy(buf + lone, two, ltwo + 1);	\
	buf; })

#define concat3a(one,med,two)	({		\
	size_t lone = strlen(one);		\
	size_t ltwo = strlen(two);		\
	char* buf = alloca(lone + 1+ ltwo + 1);	\
	memcpy(buf, one, lone);			\
	buf[lone] = med;			\
	memcpy(buf + lone + 1, two, ltwo + 1);	\
	buf; })

static int
useradd(const char *uname, uid_t uid, gid_t gid, const char *gecos, const char *dir, const char *shell, const char *grplist, int process_groups_later, int create_local_home, int verbose)
{
/* rev PP
  -Caldera Open Linux does not like the -G parameter in useradd, but it is OK in usermod ?
  - so we call usermod at the end if needed...
  -add some defaut if nothing has been retrieved from NDS and we are in SERVER_REQUIRED MODE
    Il dir is empty change it to /home/username
    if shell is empty change it to /bin/bash
    if guid is empty change it to 100
    so only UNIX_UID is REALLY SERVER_REQUIRED ( others get default values)

*/
	char gidstr[30];
	char uidstr[30];
	const char *argv[30];
	int err;
	int i;
	int usechfn = 0;

	i = 1;
	if (gid == (gid_t) -1) {
		gid = 100;
	}
	sprintf(gidstr, "%u", gid);
	argv[i++] = "-g";
	argv[i++] = gidstr;

	//PP
	/******************** Not with COL 24 useradd****/
	if (!process_groups_later && grplist) {
		argv[i++] = "-G";
		argv[i++] = grplist;
	}
	//PP

	if (!gecos) {
		gecos = uname;
	}
	// may have a comma in it ( full name,comment...)
	if (!strchr(gecos, ',')) {
		argv[i++] = "-c";
		argv[i++] = gecos;
	} else {
		usechfn = 1;
	}

	// PP modified for a  default value and still testing for no existence
	if (!dir) {
		dir = concat2a("/home/", uname);
	}
	{
		struct stat statbuf;
		argv[i++] = "-d";
		argv[i++] = dir;
		if (create_local_home && !lstat(dir, &statbuf)) {	// rev 1.24
			syslog(LOG_ERR, "Will not create %s because of home directory %s already exist\n", uname, dir);
			return -1;
		}
	}
/*
Currently if the home is remote and the NFS server is down, the home WILL not be created locally
So console login will not find the home and fall to / directory, and graphical login will fails
*/

	if (!shell) {
		shell = "/bin/bash";
	}
	argv[i++] = "-s";
	argv[i++] = shell;
	// end PP
	sprintf(uidstr, "%u", uid);
	argv[i++] = "-u";
	argv[i++] = uidstr;
	argv[i++] = create_local_home ? "-m" : "-M";	// rev 1.24
	argv[i++] = uname;
	argv[i] = NULL;
	if (verbose) {
		syslog(LOG_NOTICE, "useradd %s %s %s %s %s %s", argv[1], argv[2], argv[3], argv[4], argv[5], argv[6]);
		syslog(LOG_NOTICE, "useradd %s %s %s %s %s %s", argv[7], argv[8], argv[9], argv[10], argv[11], argv[12]);
	}
//TODO: it would be nice to have the name of the adding user utility
// in some config file (/etc/pam_ncp.conf)
// so we could give more flexibility to the user autocreation ( a good shell script)

	err = exechelper("/usr/sbin/useradd", argv, NULL);
	if (err)
		return err;
	if (verbose)
		syslog(LOG_NOTICE, "User %s(%u) added\n", uname, uid);

	if (usechfn) {
		if (verbose)
			syslog(LOG_NOTICE, "User %s has a comma in his gecos %s\n", uname, gecos);
								/*err= */ do_chfn(uname, gecos, verbose);
								// not lethal if it fails
	}
	// PP: use a call to usermod to process the other groups lists. Needed at least with Caldera OpenLinux
	if (process_groups_later && grplist) {
		return usermod(uname, (gid_t) -1, NULL, NULL, NULL, grplist, verbose);
	}
	return 0;
}

static int
uidcmp(const void *p1, const void *p2)
{
	if (*(const uid_t *) p1 < *(const uid_t *) p2)
		return -1;
	if (*(const uid_t *) p1 > *(const uid_t *) p2)
		return 1;
	return 0;
}

static int
gidcmp(const void *p1, const void *p2)
{
	if (*(const gid_t *) p1 < *(const gid_t *) p2)
		return -1;
	if (*(const gid_t *) p1 > *(const gid_t *) p2)
		return 1;
	return 0;
}

static int
allocategid(const struct pam_ncp_state *state, gid_t *gid)
{
	struct group *grp;
	gid_t now;

	now = state->gid.min;
	setgrent();
	if (state->gid.flags & QFC_NEXT_UNUSED) {
		while ((grp = getgrent()) != NULL) {
			if (grp->gr_gid < state->gid.min)
				continue;
			if (grp->gr_gid >= state->gid.max)
				continue;
			if (now <= grp->gr_gid)
				now = grp->gr_gid + 1;
		}
	} else {
		struct {
			gid_t *array;
			size_t used;
			size_t alloc;
		} gids;
		gid_t *p;

		gids.array = NULL;
		gids.alloc = 0;
		gids.used = 0;
		while ((grp = getgrent()) != NULL) {
			if (grp->gr_gid < state->gid.min)
				continue;
			if (grp->gr_gid >= state->gid.max)
				continue;
			if (gids.used >= gids.alloc) {
				gid_t *np;
				size_t ns;

				if (gids.array) {
					ns = gids.alloc * 2;
					np = (gid_t *) realloc(gids.array, ns * sizeof (gid_t));
				} else {
					ns = 16;
					np = (gid_t *) malloc(ns * sizeof (gid_t));
				}
				if (!np) {
					syslog(LOG_WARNING, "Not enough memory\n");
					if (gids.array)
						free(gids.array);
					return -1;
				}
				gids.array = np;
				gids.alloc = ns;
			}
			gids.array[gids.used++] = grp->gr_gid;
		}
		qsort(gids.array, gids.used, sizeof (*gids.array), gidcmp);
		p = gids.array;
		while (gids.used-- && (now == *p)) {
			now++;
			p++;
		}
		free(gids.array);
	}
	if (now >= state->gid.max) {
		return -1;
	}
	endgrent();
	*gid = now;
	return 0;
}

static int
allocateuid(const struct pam_ncp_state *state, uid_t *uid)
{
	struct passwd *pwd;
	uid_t now;

	now = state->uid.min;
	setpwent();
	if (state->uid.flags & QFC_NEXT_UNUSED) {
		while ((pwd = getpwent()) != NULL) {
			if (pwd->pw_uid < state->uid.min)
				continue;
			if (pwd->pw_uid >= state->uid.max)
				continue;
			if (now <= pwd->pw_uid)
				now = pwd->pw_uid + 1;
		}
	} else {
		struct {
			uid_t *array;
			size_t used;
			size_t alloc;
		} uids;
		uid_t *p;

		uids.array = NULL;
		uids.alloc = 0;
		uids.used = 0;
		while ((pwd = getpwent()) != NULL) {
			if (pwd->pw_uid < state->uid.min)
				continue;
			if (pwd->pw_uid >= state->uid.max)
				continue;
			if (uids.used >= uids.alloc) {
				uid_t *np;
				size_t ns;

				if (uids.array) {
					ns = uids.alloc * 2;
					np = (uid_t *) realloc(uids.array, ns * sizeof (uid_t));
				} else {
					ns = 16;
					np = (uid_t *) malloc(ns * sizeof (uid_t));
				}
				if (!np) {
					syslog(LOG_WARNING, "Not enough memory\n");
					if (uids.array)
						free(uids.array);
					return -1;
				}
				uids.array = np;
				uids.alloc = ns;
			}
			uids.array[uids.used++] = pwd->pw_uid;
		}
		qsort(uids.array, uids.used, sizeof (*uids.array), uidcmp);
		p = uids.array;
		while (uids.used-- && (now == *p)) {
			now++;
			p++;
		}
		free(uids.array);
	}
	if (now >= state->uid.max) {
		return -1;
	}
	endpwent();
	*uid = now;
	return 0;
}

/*
   -10     : 100% match with server required, but no UNIX:GID or
             gid or name already in use
   -11     : no more free gids
   -12     : group creation failed
 */
static int
nw_update_group_info(struct nw_user_info *ui, struct nw_group_info *gi)
{
	struct group *grp;

// PP: added endgrent()
// no more return but goto bailout.
	int err = PAM_SUCCESS;

	setgrent();
	// check again for grp name, so if an alias has been sent by NDS,
	// Location string of a NDS group with N:UnixAlias, we will find it here.
	// i.e.  everyone -> users or staff -> root
	grp = getgrnam(gi->name);
	if (!grp) {
		while (ui->state.gid.flags & (QFC_PREFFER_SERVER | QFC_REQUIRE_SERVER)) {
			if (gi->gid == (gid_t) -1) {
				if ((ui->qflag & QF_DEBUG) && (ui->state.gid.flags & QFC_REQUIRE_SERVER))
					syslog(LOG_DEBUG, "Will not create group %s: no UNIX:GID present\n", gi->name);
				break;
			}
			grp = getgrgid(gi->gid);
			if (grp) {
				if ((ui->qflag & QF_DEBUG) && (ui->state.gid.flags & QFC_REQUIRE_SERVER))
					syslog(LOG_DEBUG, "Will not create group %s: gid %u is already used by %s\n", gi->name, gi->gid, grp->gr_name);
				break;
			}
			if (groupadd(gi->name, gi->gid, ui->qflag & QF_DEBUG))
				break;	// trouble
			goto bailout;	// OK
		}
		if (ui->state.gid.flags & QFC_REQUIRE_SERVER) {
			err = -10;
			goto bailout;
			//return -10;
		}

		if (allocategid(&ui->state, &gi->gid)) {
			syslog(LOG_WARNING, "Cannot allocate gid for group %s\n", gi->name);
			err = -11;
			goto bailout;
			//return -11;
		}
		if (groupadd(gi->name, gi->gid, ui->qflag & QF_DEBUG)) {
			syslog(LOG_WARNING, "Could not create group %s\n", gi->name);
			err = -12;
			goto bailout;
			//return -12;
		}
	} else {
		gi->gid = grp->gr_gid;
	}
bailout:;
	endgrent();		//PP
	return err;
}

static int
nw_update_user_info(struct nw_user_info *ui)
{

	struct passwd *pwd;
	char *glist;
	const char *gecos;
// PP: added endpwent() and freeing of malloced glist
// no more return but goto bailout.
	int err = PAM_SUCCESS;

	if (!ui->name) {
		if (ui->qflag & QF_DEBUG)
			syslog(LOG_NOTICE, "NW_UPDATE_USER:failed user has no name %u\n", ui->uid);
		return -1;
	}

	if (ui->qflag & QF_DEBUG)
		syslog(LOG_NOTICE, "NW_UPDATE_USER: %u %u %s %s %s\n", ui->uid, ui->gid, ui->dir, ui->gecos, ui->shell);

	if (build_groups_list(ui, &glist)) {
		if (ui->qflag & QF_DEBUG)
			syslog(LOG_NOTICE, "NW_UPDATE_USER:failed to build group list for %s\n", ui->name);
		return -1;
	}

	setpwent();
	pwd = getpwnam(ui->name);

	//PP
	gecos = ui->gecos ? : "";
	// end PP
	if (!pwd) {
		if (ui->qflag & QF_DEBUG)
			syslog(LOG_NOTICE, "NW_UPDATE_USER:creating %u %u %s %s %s\n", ui->uid, ui->gid, ui->dir, ui->gecos, ui->shell);
		while (ui->state.uid.flags & (QFC_PREFFER_SERVER | QFC_REQUIRE_SERVER)) {
			if (ui->uid == (uid_t) -1) {
				if ((ui->qflag & QF_DEBUG) && (ui->state.uid.flags & QFC_REQUIRE_SERVER))
					syslog(LOG_DEBUG, "Will not create user %s: no UNIX:UID present\n", ui->name);
				break;
			}
			pwd = getpwuid(ui->uid);
			if (pwd) {
				if ((ui->qflag & QF_DEBUG) && (ui->state.uid.flags & QFC_REQUIRE_SERVER))
					syslog(LOG_DEBUG, "Will not create user %s: uid %u is already used by %s\n", ui->name, ui->uid, pwd->pw_name);
				break;
			}

			err = useradd(ui->name, ui->uid, ui->gid, gecos, ui->dir, ui->shell, glist, NO_PROCESS_GROUPS, ui->qflag & QF_CREATEHOME, ui->qflag & QF_DEBUG);
			if (err)
				break;
			ui->isNewUser = 1;	// rev 1.19
			err = PAM_SUCCESS;
			goto bailout;
		}
		if (ui->state.uid.flags & QFC_REQUIRE_SERVER) {
			syslog(LOG_WARNING, "Cannot create uid %u required by server for user %s\n", ui->uid, ui->name);
			err = -10;
			goto bailout;
			//return -10;
		}
		if (allocateuid(&ui->state, &ui->uid)) {
			syslog(LOG_WARNING, "Cannot allocate uid for user %s\n", ui->name);
			err = -11;
			goto bailout;
			//return -11;
		}

		if (useradd(ui->name, ui->uid, ui->gid, gecos, ui->dir, ui->shell, glist, NO_PROCESS_GROUPS, ui->qflag & QF_CREATEHOME, ui->qflag & QF_DEBUG)) {
			syslog(LOG_WARNING, "Cannot create user %s\n", ui->name);
			err = -12;
			goto bailout;
			//return -12;
		}
	} else {

		int diff;
		ui->uid = pwd->pw_uid;
		if (ui->qflag & QF_DEBUG)
			syslog(LOG_NOTICE, "NW_UPDATE_USER:updating %u %u %s %s %s\n", ui->uid, ui->gid, ui->dir, ui->gecos, ui->shell);

		diff = 0;
		if ((ui->gid != (gid_t) -1) && ui->gid != pwd->pw_gid)
			diff |= QFMU_GID;
		if (*gecos && (!pwd->pw_gecos || strcmp(gecos, pwd->pw_gecos)))
			diff |= QFMU_GECOS;
		if (ui->dir && (!pwd->pw_dir || strcmp(ui->dir, pwd->pw_dir)))
			diff |= QFMU_DIR;
		if (ui->shell && (!pwd->pw_shell || strcmp(ui->shell, pwd->pw_shell)))
			diff |= QFMU_SHELL;
		diff &= ui->state.uid.modflags;
		// FIXME: glist is not processed if it is the only change ! 26/01/2001
		if (diff) {
			err = usermod(ui->name, (diff & QFMU_GID) ? ui->gid : (gid_t) -1, (diff & QFMU_GECOS) ? gecos : NULL,	//PP
				      (diff & QFMU_DIR) ? ui->dir : NULL, (diff & QFMU_SHELL) ? ui->shell : NULL, glist,	//  process groups          //PP
				      ui->qflag & QF_DEBUG);
		}
	}

bailout:;
	endpwent();
	if (glist)
		free(glist);
	if (ui->qflag & QF_DEBUG)
		syslog(LOG_DEBUG, "return value of update_user_info  %d for %s\n", err, ui->name);

	return err;
}

static int
getcflag(int *val, const char **str)
{
	const char *p = *str;
	int valm;
	int c;

	*val = 0;
	valm = 1;
	if (!*p)
		return 1;
	while ((c = *p++) != 0) {
		if (c == ',')
			break;
		switch (c) {
		case 'f':
		case 'N':
			*val &= ~QFC_NEXT_UNUSED;
			break;
		case 'n':
		case 'F':
			*val |= QFC_NEXT_UNUSED;
			break;
		case 'P':
			*val &= ~QFC_PREFFER_SERVER;
			break;
		case 'p':
			*val |= QFC_PREFFER_SERVER;
			break;
		case 'R':
			*val &= ~QFC_REQUIRE_SERVER;
			break;
		case 'r':
			*val |= QFC_REQUIRE_SERVER;
			break;
		default:
			return -1;
		}
		valm = 0;
	}
	if (!c)
		p--;
	*str = p;
	return valm;
}

static int
getmodflag(int *val, const char **str)
{
	const char *p = *str;
	int valm;
	int c;

	*val = 0;
	valm = 1;
	if (!*p)
		return 1;
	while ((c = *p++) != 0) {
		if (c == ',')
			break;
		switch (c) {
		case 'G':
			*val &= ~QFMU_GID;
			break;
		case 'g':
			*val |= QFMU_GID;
			break;
		case 'C':
			*val &= ~QFMU_GECOS;
			break;
		case 'c':
			*val |= QFMU_GECOS;
			break;
		case 'D':
			*val &= ~QFMU_DIR;
			break;
		case 'd':
			*val |= QFMU_DIR;
			break;
		case 'S':
			*val &= ~QFMU_SHELL;
			break;
		case 's':
			*val |= QFMU_SHELL;
			break;

		default:
			return -1;
		}
		valm = 0;
	}
	if (!c)
		p--;
	*str = p;
	return valm;
}

static int
parseuid(struct nw_user_info *ui, const char **str)
{
	int val;

	switch (getnumber(&val, str)) {
	case 0:
		ui->state.uid.min = val;
		break;
	case 1:
		break;
	default:
		syslog(LOG_ERR, "Unknown min uid value %s\n", *str);
		return -1;
	}
	switch (getnumber(&val, str)) {
	case 0:
		ui->state.uid.max = val;
		break;
	case 1:
		break;
	default:
		syslog(LOG_ERR, "Unknown max uid value %s\n", *str);
		return -1;
	}
	switch (getcflag(&val, str)) {
	case 0:
		ui->state.uid.flags = val;
		ui->qflag |= QF_AUTOCREATE;
		break;
	case 1:
		break;
	default:
		syslog(LOG_ERR, "Unknown uid flags value %s\n", *str);
		return -1;
	}
	switch (getmodflag(&val, str)) {
	case 0:
		ui->state.uid.modflags = val;
		ui->qflag |= QF_AUTOMODIFY;
		break;
	case 1:
		break;
	default:
		syslog(LOG_ERR, "Unknown uid modflags value %s\n", *str);
		return -1;
	}
	return 0;
}

static int
parsegid(struct nw_user_info *ui, const char **str)
{
	int val;

	switch (getnumber(&val, str)) {
	case 0:
		ui->state.gid.min = val;
		break;
	case 1:
		break;
	default:
		syslog(LOG_ERR, "Unknown min gid value %s\n", *str);
		return -1;
	}
	switch (getnumber(&val, str)) {
	case 0:
		ui->state.gid.max = val;
		break;
	case 1:
		break;
	default:
		syslog(LOG_ERR, "Unknown max gid value %s\n", *str);
		return -1;
	}
	switch (getcflag(&val, str)) {
	case 0:
		ui->state.gid.flags = val;
		break;
	case 1:
		break;
	default:
		syslog(LOG_ERR, "Unknown gid flags value %s\n", *str);
		return -1;
	}
	return 0;
}

// ajout PP 2000/12/19 session_on et off
// will mount Netware home directory in ~/nwhome

static int
parsemntpnt(struct nw_user_info *ui, const char **str)
{
// found another value that default nwhome in the command line (-mPATH (no space!)
// and cannot have space in it ;-)
// v 1.14: if the argument of -m is empty user's Netware home will be mounted as Unix home
	ui->nwhomeMntPnt = strdup(*str);
	if (ui->qflag & QF_DEBUG)
		syslog(LOG_DEBUG, "new default mounting point is \"%s\"", ui->nwhomeMntPnt);
	return 0;
}

// found a default zenFlag value on this machine
// apply it to user instead of the ZF_DEFAULTS
static int
parsezenflag_on(struct nw_user_info *ui, const char **str)
{
	ui->zenFlag = decodeZenFlag(str);
	return 0;
}

// found a zenFlags not allowed on this machine
// apply it to user
static int
parsezenflag_off(struct nw_user_info *ui, const char **str)
{
	if (**str)
		ui->zenFlagOFF = decodeZenFlag(str);
	else
		ui->zenFlagOFF = ~0UL;	// all are off

	return 0;
}

static void
mergeZenFlags(struct nw_user_info *ui)
{
	ui->zenFlag &= ~ui->zenFlagOFF;
	// make sure .nwinfos file is created if any zenscript is required
	if (ui->zenFlag & (ZF_OPENING_SCRIPTS | ZF_CLOSING_SCRIPTS))
		ui->zenFlag |= ZF_CREATE_NWINFOS;
}

// rev 1.19 we now  run the following command
// su -l USER -c "ncpmount ..... "
static int
mount_nwhome(const char *unixname, const char *uname,	// nw FQDN name
	     const char *pwd, const char *server, const char *volume, 
	     const char *path, const char *mntpoint, int uid, UNUSED(int zenFlag), int useIP, int verbose)
{

	const char *argv[30];
	int err;
	int i = 1, j = 0;
	char uidstr[30];

/* -B flag not yet supported by ncpmount */
#ifdef SET_BCAST
	int bMode = 0;
	char bModeStr[128];

	switch (zenFlag & (ZF_BROADCAST_ALL | ZF_BROADCAST_CONSOLE)) {
	case ZF_BROADCAST_ALL:
		bMode = NWCC_BCAST_PERMIT_ALL;
		break;
	case ZF_BROADCAST_CONSOLE:
		bMode = NWCC_BCAST_PERMIT_SYSTEM;
		break;
	default:
		bMode = NWCC_BCAST_PERMIT_NONE;
	}
	sprintf(bModeStr, "%u", bMode);
	argv[i++] = "-B";
	argv[i++] = bModeStr;
#endif

	if (uname) {
		argv[i++] = "-U";
		argv[i++] = uname;
	}
	if (pwd) {
		argv[i++] = "-P";
		argv[i++] = pwd;
	}
	if (server) {
		argv[i++] = "-S";
		argv[i++] = server;
		// added for JFB
		if (useIP) {
			argv[i++] = "-A";
			argv[i++] = server;
		}
	}
	if (volume) {
#ifdef NCP_IOC_GETROOT
		if (path) {	// already in Unix & uppercase
			char *fullPathToHome;

			fullPathToHome = concat3a(volume, ':', path);
			argv[i++] = "-V";
			argv[i++] = fullPathToHome;
		} else
#endif
		{
			argv[i++] = "-V";
			argv[i++] = volume;
		}
	}
	argv[i++] = "-o";
	argv[i++] = "symlinks,exec";

	sprintf(uidstr, "%u", uid);	// current user is owner

	argv[i++] = "-u";
	argv[i++] = uidstr;

	argv[i++] = "-c";
	argv[i++] = uidstr;

	argv[i++] = "-d";
	argv[i++] = "0700";	// current user is owner and only him

	if (mntpoint)		// better be ;-)
		argv[i++] = mntpoint;

	argv[i] = NULL;

	if (verbose) {
		char s[4096];	/* buf... buf... buffer overflow... */

		// do not log a clear password in /var/log/secure !
		sprintf(s, "running as %s \"%s", unixname, NCPMOUNT_PATH);
		for (j = 1; j < i; j++) {
			if (strlen(s) + strlen(argv[j]) >= sizeof (s) - 2)
				break;	/* fix the possible buffer overflow */
			if (strcmp(argv[j], "-P")) {
				strcat(s, " ");
				strcat(s, argv[j]);
			} else {
				j++;
			}
		}
		strcat(s, "\"");
		syslog(LOG_WARNING, "%s", s);
	}

	err = exechelper(NCPMOUNT_PATH, argv, unixname);
	if (verbose) {
		if (err)
			syslog(LOG_DEBUG, "user %s had trouble mounting %s/%s on %s", uname, server, volume, mntpoint);
		else
			syslog(LOG_NOTICE, "User %s has mounted %s/%s as %s\n", uname, server, volume, mntpoint);
	}

	return err;
}

// called by pam_end_session
static int
umount_nwhome(const char *unixname, const char *mntpoint, int verbose)
{
	const char *argv[5];
	int err;
	int i;

	i = 1;
	if (mntpoint) {		// better be ;-)
		argv[i++] = mntpoint;
	}
	argv[i] = NULL;
	err = exechelper(NCPUMOUNT_PATH, argv, unixname);	// umount as user
	if (verbose) {
		if (err)
			syslog(LOG_DEBUG, "user %s had trouble unmounting %s", unixname, mntpoint);
		else
			syslog(LOG_NOTICE, "User %s has unmounted  %s\n", unixname, mntpoint);
	}

	return err;
}

static int
nw_automount_home(const char *uname, struct nw_user_info *ui, const struct passwd *pwd, const char *pass)
{
	long err;
	struct stat stt;
	char *mountpnt;
	const char *fqdn;
	int perm_err;

	if (ui->qflag & QF_DEBUG)
		syslog(LOG_DEBUG, "start of nw_auto_mount_home \n");

	// few sanity checks !!!
	if (!ui->nwhomeServer || !ui->nwhomeVolume)
		return -1;
	// no value red from the command line, use default ~/nwhome
	if (!ui->nwhomeMntPnt)
		ui->nwhomeMntPnt = strdup(DEF_MNT_PNT);

	if (pwd == NULL) {
		syslog(LOG_DEBUG, "/etc/passwd/%s not found !\n", uname);
		return PAM_USER_UNKNOWN;
	}

	if (ui->qflag & QF_MOUNTLOCALLY) {	// rev 1.24
#		define TOP_OF_ALL_MOUNTS "/mnt/ncp"
		if (stat(TOP_OF_ALL_MOUNTS, &stt))
			if (mkdir(TOP_OF_ALL_MOUNTS, 0711)) {
				syslog(LOG_DEBUG, "Unable to create common mounting point %s \n", TOP_OF_ALL_MOUNTS);
				return PAM_USER_UNKNOWN;
			}
		mountpnt = concat3a(TOP_OF_ALL_MOUNTS, '/', pwd->pw_name);
		if (stat(mountpnt, &stt)) {
			perm_err = mkdir(mountpnt, 0700) || chown(mountpnt, pwd->pw_uid, pwd->pw_gid);
			if (perm_err) {
				syslog(LOG_DEBUG, "Unable to create local mounting point %s \n", mountpnt);
				return PAM_USER_UNKNOWN;
			}
		}
		mountpnt = concat3a(mountpnt, '/', ui->nwhomeMntPnt);

	} else {
		if (stat(pwd->pw_dir, &stt) != 0) {
			syslog(LOG_DEBUG, "Unix home %s not found !\n", pwd->pw_dir);
			return PAM_USER_UNKNOWN;
		}
		mountpnt = concat3a(pwd->pw_dir, '/', ui->nwhomeMntPnt);
	}
	if (stat(mountpnt, &stt) != 0) {
		if (ui->qflag & QF_DEBUG)
			syslog(LOG_DEBUG, "Netware home mounting point %s not found. Will create it \n", mountpnt);
		perm_err = mkdir(mountpnt, 0700) || chown(mountpnt, pwd->pw_uid, pwd->pw_gid);
		if (perm_err) {
			syslog(LOG_DEBUG, "error creating Netware home mounting point %s :%d (%s)\n", mountpnt, perm_err, strerror(errno));
			return PAM_USER_UNKNOWN;
		}
	}
//rev 1.24 :remember full path to Netware home instead of simply 'nwhome'
// will be written in $HOME/.nwinfos file and red again at PAM end session
	free(ui->nwhomeMntPnt);
	ui->nwhomeMntPnt = strdup(mountpnt);

	/* v 1.13 if we allow contextless login, we must pass to ncpmount a FQDN and not a CN ! */

	if (ui->fqdn) {
		/* Add leading dot to denote absolute DN... */
		fqdn = concat2a(".", ui->fqdn);
	} else {
		/* Bindery... */
		fqdn = uname;
	}

	err = mount_nwhome(uname,	// unix name for su
			   fqdn,	// NW name for ncpmount
			   pass, ui->nwhomeServer, ui->nwhomeVolume, ui->nwhomePath, mountpnt, pwd->pw_uid, ui->zenFlag, ui->qflag & QF_USE_NETWARE_IP, ui->qflag & QF_DEBUG);
	if (!err)
		err = PAM_SUCCESS;

	return err;
}

/* create the 600 .nwclient file in user's home
   this file is used by others ncp utilities
   it contains the line SERVER/USER password or SERVER/USER
   depending of ZF_PASSWD_IN_NWCLIENT */
static int
nw_create_nwclient(const char *uname, const struct nw_user_info *ui, const struct passwd *pwd, const char *pass)
{
	char *nwclient;
	const char *server;
	FILE *f;
#ifdef IMPERSONNATE
	uid_t currentUID = getuid();
#endif
	int perm_err;

	if (pwd == NULL) {
		syslog(LOG_DEBUG, "/etc/passwd/%s not found !\n", uname);
		return PAM_USER_UNKNOWN;
	}
// I have a problem here. They may be different . If the Netware home is automounted
// the connection to the homeServer (not to the messageServer) will be marked as permanent.
	server = (ui->messageServer ? ui->messageServer : ui->nwhomeServer);
	if (!server)
		return PAM_SUCCESS;

	nwclient = concat3a(pwd->pw_dir, '/', NWCLIENT_FILE);

#ifdef IMPERSONNATE
	// v 1.24 NFS mounted homes are mounted with no_root_squash option , so we must change UID to current user
	if (seteuid(pwd->pw_uid)) {
		syslog(LOG_DEBUG, "Cannot impersonnate to %s: %s\n", pwd->pw_name, strerror(errno));
		return -1;
	}
#endif
	if (ui->zenFlag & ZF_OVERWRITE_NWCLIENT)
		f = fopen(nwclient, "w");	// erase it every time
	else
		f = fopen(nwclient, "a");	// I should append ?

	if (f) {
		// must have a CR/LF
		if (ui->zenFlag & ZF_PASSWD_IN_NWCLIENT)
			fprintf(f, "%s/%s %s\n", server, uname, pass);
		else
			fprintf(f, "%s/%s \n", server, uname);

		/* rev 1.15 */

		if (ui->defaultTree) {
			fprintf(f, "\n[Requester]\n");
			fprintf(f, "Default Tree Name=%s\n", ui->defaultTree);
			if (ui->defaultNameCtx)
				fprintf(f, "Default Name Context=%s\n", ui->defaultNameCtx);
		}

		fclose(f);
		// must be 600 and owner by current user, else ncpfs apps disregard it
		//perm_err=chmod(nwclient,0600) || chown(nwclient,pwd->pw_uid,pwd->pw_gid);
		perm_err = chmod(nwclient, 0600);
#ifdef IMPERSONNATE
		seteuid(currentUID);
#endif
		if (perm_err)
			syslog(LOG_DEBUG, "problem %d (%s)changing permissions of %s (%d %d)\n", perm_err, strerror(errno), nwclient, pwd->pw_uid, pwd->pw_gid);
		else {
			if (ui->qflag & QF_DEBUG)
				syslog(LOG_DEBUG, "DONE writing to %s\n", nwclient);
			return PAM_SUCCESS;	// OK OK
		}
	} else {
#ifdef IMPERSONNATE
		seteuid(currentUID);
#endif
		syslog(LOG_DEBUG, "problem writing to %s\n", nwclient);
	}
	return -1;		// trouble somewhere (should not be lethal)
}

static int
nw_process_forward_file(UNUSED(const char *uname), const struct nw_user_info *ui, const struct passwd *pwd)
{
// what we do here it to setup a forward to any email address found in NDS
// we do it during the authentication part  so if the Netware home directory is mounted
// as /home/user ( -m with no argument) the .forward file is on the local machine if user is not currently logged in ...
//We do it again in the pam_session part ( if the argument of -m is empty) so if user IS logged in,
// the local /home/user is "masqued off"  and system will still find this file in the mounted Netware home ...
	FILE *f;
	char *p;
	char *forward;
#ifdef IMPERSONNATE
	uid_t currentUID = getuid();
#endif
	int perm_err;

	if (ui->emailLDAP) {
		p = ui->emailLDAP;	// priority to the "new format"
	} else if (ui->emailSMTP) {
		p = ui->emailSMTP;
	} else {
		return 0;	// too bad
	}

	forward = concat2a(pwd->pw_dir, "/.forward");

#ifdef IMPERSONNATE
	// v 1.25 NFS mounted homes are mounted with no_root_squash option , so we must change UID to current user
	if (seteuid(pwd->pw_uid)) {
		syslog(LOG_DEBUG, "Cannot inpersonnate to %s: %s\n", pwd->pw_name, strerror(errno));
		return -1;
	}
#endif
	f = fopen(forward, "w");	// erase it every time
	if (f) {
		// must have a CR/LF ??
		fprintf(f, "%s\n", p);
		fclose(f);
		// makes no harm to restrict reading to user ?
		//perm_err= chmod(forward,0600) || chown(forward,pwd->pw_uid,pwd->pw_gid);
		perm_err = chmod(forward, 0600);
#ifdef IMPERSONNATE
		seteuid(currentUID);
#endif
		if (perm_err)
			syslog(LOG_DEBUG, "problem %d (%s)changing permissions of %s\n", perm_err, strerror(errno), forward);
	} else {
#ifdef IMPERSONNATE
		seteuid(currentUID);
#endif
		syslog(LOG_DEBUG, "Cannot open %s: %s\n", forward, strerror(errno));
	}
	return 0;
}

/******************************* check for allowed remote access by zen ***/

#if 0
static void
report(int err, const char *what, const char *info)
{

	if (err != PAM_SUCCESS) {
		syslog(LOG_NOTICE, "error reading pam item %s \n", what);
	} else {
		if (!info)
			syslog(LOG_NOTICE, "pam item %s is missing \n", what);
		else if (*info == '\0')
			syslog(LOG_NOTICE, "pam item %s is empty \n", what);
		else
			syslog(LOG_NOTICE, "pam item %s value is %s\n", what, info);
	}
}

static void __attribute__((unused))
test_pam_items(const char *me, pam_handle_t * pamh)
{
	const char *info;
	int err;
	syslog(LOG_NOTICE, "%s testing for PAM items \n", me);
	err = my_pam_get_item(pamh, PAM_RUSER, &info);
	report(err, "PAM_RUSER", info);
	err = my_pam_get_item(pamh, PAM_SERVICE, &info);
	report(err, "PAM_SERVICE", info);
	err = my_pam_get_item(pamh, PAM_RHOST, &info);
	report(err, "PAM_RHOST", info);
	err = my_pam_get_item(pamh, PAM_TTY, &info);
	report(err, "PAM_TTY", info);
}
#endif

struct pam_auth_element {
	const char *service;
	const char *rhost;
	const char *tty;
	const char *ruser;
	int zenFlag;
	int result;
	int cutZen;		// turn off incompatible flags
};

// this table was built on Caldera openLinux 2.4 and  Redhat 7.1
// using the call to test_pam_items()  to determine the correct values received by PAM
// for each service...
// you may have to adapt it to your distribution
// use the very same trick to add other remote access
// STRANGE: with samba pam_rhost is missing ( but pam_tty) also
// fixed with samba >=2.07 pam_rhost=hostname pam_tty=samba
// FIXME: with ftp : my ftpd does not call pam session, so mounted nwhome stays mounted
// I am sure I have the line in /etc/pam.d/ftp "session    required     pam_ncp_auth.so -d"
// SAME with samba
// so my current solution is to turn off automounting !!!
// ssh  RHOST=IP TTY=ssh SERVICE=sshd RUSER=missing
// X access  RHOST=missing TTY=hostname only SERVICE=kde  is tested, others pass through ?
// TESTME: rlogin, rsh  (I never used these utilities ;-))
// TODO LATER: Web authorization

static struct pam_auth_element pam_auth_table[] = {
	{ "login",  "*",  "*",     NULL, ZF_ALLOW_TELNET_ACCESS, PAM_SUCCESS, 0},
	{ "ftp",    "*",  NULL,    NULL, ZF_ALLOW_FTP_ACCESS,	 PAM_SUCCESS, ZF_AUTOMOUNT_NWHOME},
	{ "samba",  "*",  "samba", NULL, ZF_ALLOW_SAMBA_ACCESS,	 PAM_SUCCESS, ZF_AUTOMOUNT_NWHOME},	//samba >2.07
	{ "samba",  NULL, NULL,    NULL, ZF_ALLOW_SAMBA_ACCESS,	 PAM_SUCCESS, ZF_AUTOMOUNT_NWHOME},
	{ "kde",    NULL, "*",     NULL, ZF_ALLOW_X_ACCESS,	 PAM_SUCCESS, 0},	// RedHat 7.1 send NULL RHOST
	{ "kde",    "",   "*",     NULL, ZF_ALLOW_X_ACCESS,	 PAM_SUCCESS, 0},	// RedHat 7.2 send empty RHOST
	{ "rlogin", "*",  "*",     "*",  ZF_ALLOW_RLOGIN_ACCESS, PAM_SUCCESS, 0},
	{ "rsh",    "*",  "*",     "*",  ZF_ALLOW_RSH_ACCESS,	 PAM_SUCCESS, 0},
	{ "sshd",   "*",  "ssh",   NULL, ZF_ALLOW_TELNET_ACCESS, PAM_SUCCESS, 0},
	{ "su",     NULL, NULL,    NULL, ZF_ALLOW_TELNET_ACCESS, PAM_SUCCESS, 0},
	{ NULL,     NULL, NULL,    NULL, 0,			 PAM_AUTH_ERR, 0}
};

static int
process_zenflag_remote(pam_handle_t * pamh, UNUSED(const char *user), struct nw_user_info *ui)
{
	int err;
	char *service;
	char *rhost;
	char *ruser;
	char *tty;

// zenflags restricting remote access are LETHAL
//reactivate to test values of ruser, tty, rhost and service for a new service
// see output in /var/log/secure
#if 0
	test_pam_items(user, pamh);
#endif
	if (ui->qflag & QF_NO_PEER_CHECKS) {
		if (ui->qflag & QF_DEBUG)
			syslog(LOG_NOTICE, "Remote host and tty port are not checked\n");
		return PAM_SUCCESS;
	}
	err = my_pam_get_item(pamh, PAM_TTY, &tty);
	if (err != PAM_SUCCESS)
		return PAM_SYSTEM_ERR;
// local access
	if (ui->qflag & QF_DEBUG)
		syslog(LOG_NOTICE, "testing remote acces PAM_TTY is %s", tty);

	// PP not really sure this is good enough
	// JFB added tty="vc/" for Mandrake 8.2
	if (tty) {
		if (!memcmp(tty, "/dev/", 5)) {
			tty += 5;
		}
		if (!memcmp(tty, "tty", 3) || !memcmp(tty, ":0", 2) || !memcmp(tty, "vc/", 2)) {
			if (ui->qflag & QF_DEBUG)
				syslog(LOG_NOTICE, "local acces OK to %s", tty);
			return PAM_SUCCESS;
		}
	}

	err = my_pam_get_item(pamh, PAM_RHOST, &rhost);
	if (err != PAM_SUCCESS) {
		syslog(LOG_WARNING, "error getting  PAM_RHOST");
		return PAM_SYSTEM_ERR;
	}

	err = my_pam_get_item(pamh, PAM_RUSER, &ruser);
	if (err != PAM_SUCCESS) {
		syslog(LOG_WARNING, "error getting  PAM_RUSER");
		return PAM_SYSTEM_ERR;
	}

	err = my_pam_get_item(pamh, PAM_SERVICE, &service);
	if (err != PAM_SUCCESS) {
		syslog(LOG_WARNING, "error getting  PAM_SERVICE");
		return PAM_SYSTEM_ERR;
	}

	{
		struct pam_auth_element *z;

		err = PAM_AUTH_ERR;

		for (z = pam_auth_table; z->service; z++) {
			if (!strcmp(z->service, service))
				if ((rhost && z->rhost) || (!rhost && !z->rhost))
					if ((tty && z->tty) || (!tty && !z->tty))
						if ((ruser && z->ruser) || (!ruser && !z->ruser)) {
							if (!(z->zenFlag & ui->zenFlag)) {
								if (ui->qflag & QF_DEBUG)
									syslog(LOG_NOTICE, "remote acces for service %s rejected by Zen for %s", service, ui->name);
								return PAM_AUTH_ERR;

							} else {
								// turn OFF incompatible FLAGS ( automounting ?)
								ui->zenFlag &= ~z->cutZen;
								if (ui->qflag & QF_DEBUG)
									syslog(LOG_NOTICE, "remote acces for service %s granted by Zen for %s", service, ui->name);
								err = PAM_SUCCESS;
								break;
							}
						}
#if 0
						else
							syslog(LOG_NOTICE, "Mismatching ruser found: %s expecting:%s", ruser, z->ruser);
					else
						syslog(LOG_NOTICE, "Mismatching tty found: %s expecting:%s", tty, z->tty);
				else
					syslog(LOG_NOTICE, "Mismatching rhost found: %s expecting:%s", rhost, z->rhost);
#endif
		}

	}
#if 0
	syslog(LOG_NOTICE, "final return on remote check %d", err);
#endif
	return err;		//PAM_SUCCESS;
}

/***************************************************************************************************/

// must be done at this stage since they requires the user's current passwd
static int
process_zenflag_auth(UNUSED(pam_handle_t * pamh), const char *user, struct nw_user_info *ui, const struct passwd *pwd, const char *userpass)
{

	int err;		// ignore errors for now

	/* must be done BEFORE mounting Netware home since local /home/user
	   can become unavailable if the Netware home directory is mounted as /home/user
	   using the -m option with NO ARGUMENT */
	if (ui->zenFlag & ZF_FORWARD_MAIL)
		err = nw_process_forward_file(user, ui, pwd);
	if (ui->zenFlag & ZF_AUTOMOUNT_NWHOME) {
		err = nw_automount_home(user, ui, pwd, userpass);
		/* do it AGAIN after mounting Netware home, so the .forward file is still available
		   when user is logged in */
		if (!err && ui->zenFlag & ZF_FORWARD_MAIL)
			if (ui->nwhomeMntPnt && ui->nwhomeMntPnt[0] == 0)
				err = nw_process_forward_file(user, ui, pwd);
	}
	if (ui->zenFlag & (ZF_CREATE_NWCLIENT | ZF_OVERWRITE_NWCLIENT))
		nw_create_nwclient(user, ui, pwd, userpass);	// ignore errors for now
	return PAM_SUCCESS;
}

/* The code - what there is of it :) */

PAM_EXTERN int
pam_sm_authenticate(pam_handle_t * pamh, int flags, int argc, const char **argv)
{
	int retval;
	const char *name;
	char *p;
	int c;
	struct nw_user_info inf;
	struct passwd *pwd_entry;

	openlog("pam_ncp_auth", LOG_PID, LOG_AUTHPRIV);

	init_nw_user_info(&inf);
	inf.pamh = pamh;
	inf.qflag = QF_VERBOSE | QF_CREATEHOME;

	inf.state.uid.min = 1000;
	inf.state.uid.max = 60000;
	inf.state.uid.flags = 0;
	inf.state.uid.modflags = 0;
	inf.state.gid.min = 1000;
	inf.state.gid.max = 60000;
	inf.state.gid.flags = QFC_NEXT_UNUSED | QFC_PREFFER_SERVER;

	/* Get options */
	for (c = 0; c < argc; c++) {
		const char *chrp = argv[c];

		if (*chrp++ == '-') {
			int chr;

			while ((chr = *chrp++) != 0) {
				switch (chr) {
				case 'v':
					inf.qflag |= QF_VERBOSE;
					break;	/* verbose */
				case 'q':
					inf.qflag &= ~QF_VERBOSE;
					break;	/* quiet */
				case 'd':
					inf.qflag |= QF_DEBUG;
					break;	/* debug */
				case 's':
					inf.qflag |= QF_NOSU;
					break;	/* no supervisor */
				case 'S':
					inf.qflag |= QF_NOSUEQ;
					break;	/* no supervisor equivalent */
				case 'a':
					inf.qflag |= QF_AUTOCREATE;
					break;	/* create account automagically */
				case 'b':
					inf.qflag |= QF_BINDERY;
					break;	/* use bindery access */
				case 'u':
					parseuid(&inf, &chrp);
					break;
				case 'g':
					parsegid(&inf, &chrp);
					break;
				case 'm':
					parsemntpnt(&inf, &chrp);
					while ((chr = *chrp++) != 0) ;	// bug corrected v 1.14
					break;	//PP
				case 'z':
					parsezenflag_on(&inf, &chrp);
					break;	//PP
				case 'Z':
					parsezenflag_off(&inf, &chrp);
					break;	//PP
				case 'n':
					inf.qflag &= ~QF_CREATEHOME;
					break;	// PP v 1.24 (DO NOT create home for new users!)
				case 'l':
					inf.qflag |= QF_MOUNTLOCALLY;
					break;	// PP v 1.24 (homes are NFS mounted !)
				case 'A':
					inf.qflag |= QF_USE_NETWARE_IP;	// PP v 1.24
					break;
				case 'L':
					inf.qflag |= QF_NO_PEER_CHECKS;
					break;
				default:;	/* just silently ignore unknown option... */
				}
			}
		}
	}

	//if (inf.qflag &QF_DEBUG)
	//   test_pam_items ("authenticate",pamh);
	/* Get username */
	if ((retval = pam_get_user(pamh, &name, "login: ")) != PAM_SUCCESS)
		goto quit;

	/* Get password */
	my_pam_get_item(pamh, PAM_AUTHTOK, &p);

	if (!p) {
		retval = _set_auth_tok(pamh, flags);
		if (retval != PAM_SUCCESS)
			goto quit;
	}

	my_pam_get_item(pamh, PAM_AUTHTOK, &p);

	/* v 1.22 check the service is not a screensaver */
	{
		const char *service;
		if ((retval = my_pam_get_item(pamh, PAM_SERVICE, &service)) != PAM_SUCCESS)
			goto quit;
		inf.isScreenSaverRelogin = strstr(service, "saver") != NULL;
		// report (retval, "PAM_SERVICE",service);
		if (inf.isScreenSaverRelogin && inf.qflag & QF_DEBUG)
			syslog(LOG_NOTICE, "relogin of %s from the screen saver %s ", name, service);

	}
	/* Find the server name in the configuration. */

	for (c = 0; c < argc; c++) {
		if (!strncmp("server=", argv[c], 7)) {
			const char *server;
			const char *group;
			char sbuf[256];

			server = argv[c] + 7;
			group = strchr(server, '/');
			if (group) {
				if ((size_t) (group - server) < sizeof (sbuf) - 1) {
					memcpy(sbuf, server, group - server);
					sbuf[group - server] = 0;
					server = sbuf;
					group = group + 1;
				} else {
					syslog(LOG_ALERT, "Error in configuration file: server name too long!\n");
					continue;
				}
			}
			retval = nw_attempt_auth_server(pamh, server, name, p, inf.qflag, group);
			if (retval == PAM_SUCCESS)
				goto success;

		}
		if (!strncmp("tree=", argv[c], 5)) {
			const char *tree;
			const char *contexts;
			const char *group;
			char tbuf[512];

			tree = argv[c] + 5;
			if (strlen(tree) >= sizeof (tbuf)) {
				syslog(LOG_ALERT, "Error in configuration file: tree argument too long!\n");
				continue;
			}
			strcpy(tbuf, tree);
			tree = tbuf;
			group = strchr(tree, '/');
			if (group) {
				tbuf[group - tree] = 0;
				group++;
			}
			contexts = strchr(tree, ':');
			if (contexts) {
				tbuf[contexts - tree] = 0;
				contexts++;
			}
			if (inf.qflag & QF_DEBUG)
				syslog(LOG_NOTICE, "using tree %s ctxs %s group %s", tree, contexts, group);

			retval = nw_attempt_auth_tree(pamh, tree, name, contexts, p, inf.qflag, group, NWCC_NAME_FORMAT_NDS_TREE);
			if (retval == PAM_SUCCESS)
				goto success;

		}
		if (!strncmp("ndsserver=", argv[c], 10)) {
			const char *server;
			const char *contexts;
			const char *group;
			char tbuf[512];

			server = argv[c] + 10;
			if (strlen(server) >= sizeof (tbuf)) {
				syslog(LOG_ALERT, "Error in configuration file: ndsserver argument too long!\n");
				continue;
			}
			strcpy(tbuf, server);
			server = tbuf;
			group = strchr(server, '/');
			if (group) {
				tbuf[group - server] = 0;
				group++;
			}
			contexts = strchr(server, ':');
			if (contexts) {
				tbuf[contexts - server] = 0;
				contexts++;
			}
			if (inf.qflag & QF_DEBUG)
				syslog(LOG_NOTICE, "using server %s ctxs %s group %s", server, contexts, group);

			retval = nw_attempt_auth_tree(pamh, server, name, contexts, p, inf.qflag, group, NWCC_NAME_FORMAT_BIND);
			if (retval == PAM_SUCCESS)
				goto success;
		}

	}

	retval = PAM_AUTH_ERR;
	goto quit;
      success:;
	if (inf.qflag & QF_DEBUG)
		syslog(LOG_DEBUG, "Auth OK\n");
	setpwent();
	pwd_entry = getpwnam(name);
	endpwent();
	retval = PAM_SUCCESS;	//moved here
	if (!inf.isScreenSaverRelogin && inf.qflag & (QF_AUTOCREATE | QF_AUTOMODIFY)) {	// v 1.22
		if ((!pwd_entry && (inf.qflag & QF_AUTOCREATE)) || (pwd_entry && (inf.qflag & QF_AUTOMODIFY))) {
			int err;

			inf.name = strdup(name);
			if (!inf.name) {
				syslog(LOG_WARNING, "Not enough memory for strdup()\n");
				err = -10;	// added PP
			} else {
				// NDS error means NO LOGIN
				err = nw_retrieve_user_info(&inf);
				// remote access denied = NO LOGIN (and no update/creation!)
				if (inf.qflag & QF_DEBUG)
					syslog(LOG_DEBUG, "FOUND ZF %lx in NDS \n", inf.zenFlag);
				if (inf.qflag & QF_DEBUG)
					syslog(LOG_DEBUG, "APPLYING ZF %lx OFF \n", inf.zenFlagOFF);
				mergeZenFlags(&inf);	// rev 1.26  turn off prohibited flags
				if (inf.qflag & QF_DEBUG)
					syslog(LOG_DEBUG, "USING ZF %lx \n", inf.zenFlag);
				if (!err)
					err = process_zenflag_remote(pamh, name, &inf);
				// in this version creation error is lethal, not update
				if (!err)
					err = nw_update_user_info(&inf);
#if 0
				err = 0;	// testing
#endif
			}
			retval = err ? PAM_AUTH_ERR : PAM_SUCCESS;	// added here PP
		}
// feature: if you do not allow autocreation, automodification, NDS is not RED
// so no Zen, no remote access restrictions...
	}

      quit:;
	//PP keep the nw_user_info into PAM for a later use in sm_open_session
	// be Zen, no password saved yet ;-)

	if (retval == PAM_SUCCESS && !inf.isScreenSaverRelogin) {
		void *sav;

		//read again pwd_entry ( was NULL if it is a new account !)
		setpwent();
		pwd_entry = getpwnam(name);
		endpwent();
		mergeZenFlags(&inf);	// rev 1.26  turn off prohibited flags
		retval = process_zenflag_auth(pamh, name, &inf, pwd_entry, p);
		if (retval == PAM_SUCCESS) {	// some ZeN_FLAGS may stop the authentification process !
			sav = malloc(sizeof (inf));
			if (sav) {
				if (inf.qflag & QF_DEBUG)
					syslog(LOG_NOTICE, "saving user_info\n");
				memcpy(sav, &inf, sizeof (inf));
				pam_set_data(pamh, "pam.ncpfs.user_info", sav, cleanup_user_info);
			} else {
				if (inf.qflag & QF_DEBUG)
					syslog(LOG_NOTICE, "Out of memory. NOT saving user_info\n");
				free_nw_user_info(&inf);	// forget about it
				// I feel bad about the pam-session parts, but they should just quit gracefully
			}
		}
	}
	if (inf.qflag & QF_DEBUG)
		syslog(LOG_NOTICE, "final PAM retval %u\n", retval);

	//PP: may be a good spot to close and get rid on the still opened connection ???
	closelog();
	return retval;
}

/*
 * Does nothing. for now
 */

PAM_EXTERN int
pam_sm_setcred(UNUSED(pam_handle_t * pamh),
	       UNUSED(int flags),
	       UNUSED(int argc),
	       UNUSED(const char **argv))
{
	//return PAM_IGNORE;
	return PAM_SUCCESS;
}

/*
 * Does nothing. for now
 */

PAM_EXTERN int
pam_sm_acct_mgmt(UNUSED(pam_handle_t * pamh),
		 UNUSED(int flags), 
		 UNUSED(int argc), 
		 UNUSED(const char **argv))
{
	//return PAM_IGNORE;
	return PAM_SUCCESS;
}

/* old code used with server=XXX cmd line argument */
static int
nw_attempt_passwd_prelim_server(pam_handle_t * pamh, 
				const char *server, const char *user, const char *oldpwd,
				int qflag, const char *group, UNUSED(int flags))
{
	struct ncp_conn *conn;
	int err;

	err = nw_create_verify_conn_to_server(&conn, NULL, server, user, oldpwd, qflag, group);
	if (err)
		return err;
	pam_set_data(pamh, "pam.ncpfs.passwd.conn", conn, nw_cleanup_conn);
	return 0;
}

/*new code for tree=argument */
static int
nw_attempt_passwd_prelim_tree(pam_handle_t * pamh,
			      const char *tree, const char *user, const char *contexts, const char *oldpwd,
			      int qflag, const char *group, UNUSED(int flags), nuint nameFormat)
{
	struct ncp_conn *conn;
	int err;

	err = nw_create_verify_conn_to_tree(&conn, NULL, tree, user, contexts, oldpwd, qflag, group, nameFormat);
	if (err)
		return err;
	pam_set_data(pamh, "pam.ncpfs.passwd.conn", conn, nw_cleanup_conn);
	return 0;
}

static int
nw_attempt_passwd_post(pam_handle_t * pamh, const char *pwd, const char *oldpwd, int qflag, int flags)
{
	struct ncp_conn *conn;
	long err;
	NWObjectID oid;
	char *oldpwdup;
	char *pwdup;

	err = PAM_AUTHTOK_ERR;
	if (!(flags & PAM_UPDATE_AUTHTOK))
		goto bailout_nf;

	oldpwdup = strdup(oldpwd);
	if (!oldpwd) {
		err = ENOMEM;
		goto bailout_nf;
	}
	str_upper(oldpwdup);

	pwdup = strdup(pwd);
	if (!pwd) {
		err = ENOMEM;
		goto bailout_nf2;
	}
	str_upper(pwdup);

	err = my_pam_get_data(pamh, "pam.ncpfs.passwd.conn", &conn);
	if (err)
		goto bailout;
	err = nw_get_nwid(conn, &oid, qflag);
	if (err)
		goto bailout;



	if (qflag & QF_BINDERY) {
		struct ncp_bindery_object uinfo;
		struct ncp_bindery_object u0;
		unsigned char ncp_key[8];

		err = ncp_get_bindery_object_name(conn, oid, &u0);
		if (err) {
			syslog(LOG_WARNING, "%s when trying to get object name\n", strnwerror(err));
			err = PAM_USER_UNKNOWN;
			goto bailout;
		}
		err = ncp_get_encryption_key(conn, ncp_key);
		if (err) {
			syslog(LOG_WARNING, "%s when trying to get encryption key\n", strnwerror(err));
			err = PAM_AUTHTOK_ERR;
			goto bailout;
		} else {
			err = ncp_get_bindery_object_id(conn, u0.object_type, u0.object_name, &uinfo);
			if (err) {
				syslog(LOG_WARNING, "%s when trying to get object ID\n", strnwerror(err));
				err = PAM_USER_UNKNOWN;
				goto bailout;
			}
			err = ncp_change_login_passwd(conn, &uinfo, ncp_key, oldpwdup, pwdup);
			if (err) {
				syslog(LOG_WARNING, "%s when trying to change password\n", strnwerror(err));
				err = PAM_AUTHTOK_ERR;
				goto bailout;
			}
		}
	} else {
		NWDSContextHandle ctx;
		nuint32 c;
		char username[MAX_DN_BYTES];

		err = NWDSCreateContextHandle(&ctx);
		if (err) {
			syslog(LOG_WARNING, "NWDSCreateContextHandle() failed with %s\n", strnwerror(err));
			err = PAM_SYSTEM_ERR;
			goto bailout;
		}
		c = DCV_XLATE_STRINGS | DCV_TYPELESS_NAMES | DCV_DEREF_ALIASES;
		err = NWDSSetContext(ctx, DCK_FLAGS, &c);
		if (err) {
			syslog(LOG_WARNING, "NWDSSetContext() failed with %s\n", strnwerror(err));
			err = PAM_SYSTEM_ERR;
			goto bailoutctx;
		}
		NWDSAddConnection(ctx, conn);
		err = NWDSMapIDToName(ctx, conn, oid, username);
		if (err) {
			syslog(LOG_WARNING, "NWDSMapIDToName() failed with %s\n", strnwerror(err));
			err = PAM_USER_UNKNOWN;
			goto bailoutctx;
		}
		if (qflag & QF_DEBUG)
			syslog(LOG_DEBUG, "User has DN %s\n", username);
		err = NWDSChangeObjectPassword(ctx, NDS_PASSWORD, username, oldpwdup, pwdup);
		if (err) {
			syslog(LOG_NOTICE, "NWDSChangeObjectPassword() failed with %s\n", strnwerror(err));
			err = PAM_AUTHTOK_ERR;
			goto bailoutctx;
		}
		if (qflag & QF_DEBUG)
			syslog(LOG_DEBUG, "User %s has succesfully changed its NW pasword\n", username);
	      bailoutctx:;
		NWDSFreeContext(ctx);

	}
bailout:;
	// set it to null so nw_clean_up_conn will do no harm
	err = pam_set_data(pamh, "pam.ncpfs.passwd.conn", NULL, NULL);
	if (qflag & QF_DEBUG)
		syslog (LOG_NOTICE,"pam end of PWD:setting conn to NULL %lx",err);
	free(pwdup);
bailout_nf2:;
	free(oldpwdup);
bailout_nf:;
	return err;
}

//PP: untested code ..
// one question.
//
PAM_EXTERN int
pam_sm_chauthtok(pam_handle_t * pamh, int flags, int argc, const char **argv)
{
	int retval;
	const char *name;
	char *p;
	char *oldpasswd;
	int c;

	int qflag = QF_VERBOSE;

	openlog("pam_ncp_auth", LOG_PID, LOG_AUTHPRIV);
	/* Get options */
	for (c = 0; c < argc; c++) {
		if (argv[c][0] == '-') {
			int i;

			for (i = 1; argv[c][i]; i++) {
				switch (argv[c][i]) {
				case 'v':
					qflag |= QF_VERBOSE;
					break;	/* verbose */
				case 'q':
					qflag &= ~QF_VERBOSE;
					break;	/* quiet */
				case 'd':
					qflag |= QF_DEBUG;
					break;	/* debug */
				case 's':
					qflag |= QF_NOSU;
					break;	/* no supervisor */
				case 'S':
					qflag |= QF_NOSUEQ;
					break;	/* no supervisor equivalent */
				case 'b':
					qflag |= QF_BINDERY;
					break;	/* use bindery access */
				default:;	/* just silently ignore unknown option... */
				}
			}
		}
	}
	//if (qflag &QF_DEBUG)
	//   test_pam_items ("chauthtok",pamh);

	/* Get username */
	if ((retval = pam_get_user(pamh, &name, "passwd: ")) != PAM_SUCCESS)
		goto quit;

	pam_get_item(pamh, PAM_OLDAUTHTOK, (void *) &oldpasswd);
	if (!oldpasswd) {
		retval = _set_oldauth_tok(pamh, flags);
		if (retval != PAM_SUCCESS)
			goto quit;
		pam_get_item(pamh, PAM_OLDAUTHTOK, (void *) &oldpasswd);
	}

	if (flags & PAM_PRELIM_CHECK) {
		/* Find the server name in the configuration. */

		for (c = 0; c < argc; c++) {
			if (!strncmp("server=", argv[c], 7)) {
				const char *server;
				const char *group;
				char sbuf[256];

				server = argv[c] + 7;
				group = strchr(server, '/');
				if (group) {
					if ((size_t) (group - server) < sizeof (sbuf) - 1) {
						memcpy(sbuf, server, group - server);
						sbuf[group - server] = 0;
						server = sbuf;
						group = group + 1;
					} else {
						syslog(LOG_ALERT, "Error in configuration file: server name too long!\n");
						continue;
					}
				}
				retval = nw_attempt_passwd_prelim_server(pamh, server, name, oldpasswd, qflag, group, flags);
				if (retval == PAM_SUCCESS)
					goto quit;
			}
			if (!strncmp("tree=", argv[c], 5)) {
				const char *tree;
				const char *contexts;
				const char *group;
				char tbuf[512];

				tree = argv[c] + 5;
				if (strlen(tree) >= sizeof (tbuf)) {
					syslog(LOG_ALERT, "Error in configuration file: tree argument too long!\n");
					continue;
				}
				strcpy(tbuf, tree);
				tree = tbuf;
				group = strchr(tree, '/');
				if (group) {
					tbuf[group - tree] = 0;
					group++;
				}
				contexts = strchr(tree, ':');
				if (contexts) {
					tbuf[contexts - tree] = 0;
					contexts++;
				}
				if (qflag & QF_DEBUG)
					syslog(LOG_NOTICE, "using tree %s ctxs %s group %s", tree, contexts, group);

				retval = nw_attempt_passwd_prelim_tree(pamh, tree, name, contexts, oldpasswd, qflag, group, flags, NWCC_NAME_FORMAT_NDS_TREE);
				if (retval == PAM_SUCCESS)
					goto quit;

			}
			if (!strncmp("ndsserver=", argv[c], 10)) {
				const char *tree;
				const char *contexts;
				const char *group;
				char tbuf[512];

				tree = argv[c] + 10;
				if (strlen(tree) >= sizeof (tbuf)) {
					syslog(LOG_ALERT, "Error in configuration file: NDS server argument too long!\n");
					continue;
				}
				strcpy(tbuf, tree);
				tree = tbuf;
				group = strchr(tree, '/');
				if (group) {
					tbuf[group - tree] = 0;
					group++;
				}
				contexts = strchr(tree, ':');
				if (contexts) {
					tbuf[contexts - tree] = 0;
					contexts++;
				}
				if (qflag & QF_DEBUG)
					syslog(LOG_NOTICE, "using NDS server %s ctxs %s group %s", tree, contexts, group);

				retval = nw_attempt_passwd_prelim_tree(pamh, tree, name, contexts, oldpasswd, qflag, group, flags, NWCC_NAME_FORMAT_BIND);
				if (retval == PAM_SUCCESS)
					goto quit;

			}
		}
		retval = PAM_AUTHTOK_ERR;
		goto quit;
	}

	if (flags & PAM_UPDATE_AUTHTOK) {
		/* Get password */
		pam_get_item(pamh, PAM_AUTHTOK, (void *) &p);

		if (!p) {
			retval = _read_new_pwd(pamh, flags);
			if (retval != PAM_SUCCESS)
				return retval;
			pam_get_item(pamh, PAM_AUTHTOK, (void *) &p);
		}

		retval = nw_attempt_passwd_post(pamh, p, oldpasswd, qflag, flags);
	} else {
		retval = PAM_SYSTEM_ERR;
	}
quit:;
	closelog();
	return retval;
}

// ajout PP 2000/12/19
// will process session on and session end zenFlag

static int
nw_process_nwinfos_file(UNUSED(const char *uname), const struct nw_user_info *ui, const struct passwd *pwd)
{
// create a ~/.nwinfos file that can be "sourced" in future scripts
	FILE *f;
	char *nwinfos;
#ifdef IMPERSONNATE
	uid_t currentUID = getuid();
#endif
	int perm_err;

	nwinfos = concat3a(pwd->pw_dir, '/', NWINFOS_FILE);
#ifdef IMPERSONNATE
	// v 1.24 NFS mounted homes are mounted with no_root_squash option , so we must change UID to current user
	if (seteuid(pwd->pw_uid)) {
		syslog(LOG_DEBUG, "Cannot inpersonnate to %s: %s\n", pwd->pw_name, strerror(errno));
		return 1;
	}
#endif

	f = fopen(nwinfos, "w");
	if (f) {
		if (ui->qflag & QF_DEBUG)
			syslog(LOG_DEBUG, "writing a new %s file\n", nwinfos);

		// either NDS or local values
		fprintf(f, "NDS_USER=%s\n", ui->name ? ui->name : pwd->pw_name);
		fprintf(f, "NDS_GECOS=\"%s\"\n", ui->gecos ? ui->gecos : pwd->pw_gecos);	//may have space in it
		fprintf(f, "NDS_SHELL=%s\n", ui->shell ? ui->shell : pwd->pw_shell);
		fprintf(f, "NDS_HOME=%s\n", ui->dir ? ui->dir : pwd->pw_dir);
		fprintf(f, "NDS_UID=%u\n", (ui->uid != (uid_t) -1) ? ui->uid : pwd->pw_uid);
		fprintf(f, "NDS_GID=%u\n", (ui->gid != (gid_t) -1) ? ui->gid : pwd->pw_gid);
		// no local alernatives
		fprintf(f, "NDS_QFLAG=%x\n", ui->qflag);
		if (ui->nwhomeServer)
			fprintf(f, "NDS_HOME_SERVER=%s\n", ui->nwhomeServer);
		if (ui->nwhomeVolume)
			fprintf(f, "NDS_HOME_VOLUME=%s\n", ui->nwhomeVolume);
		if (ui->nwhomePath)
			fprintf(f, "NDS_HOME_PATH=%s\n", ui->nwhomePath);
		if (ui->nwhomeMntPnt)
			fprintf(f, "NDS_HOME_MNT_PNT=%s\n", ui->nwhomeMntPnt);
		if (ui->emailSMTP)
			fprintf(f, "NDS_EMAIL=%s\n", ui->emailSMTP);
		if (ui->emailLDAP)
			fprintf(f, "NDS_EMAIL=%s\n", ui->emailLDAP);	// overwrite it
		if (ui->messageServer)
			fprintf(f, "NDS_PREFERRED_SERVER=%s\n", ui->messageServer);
		if (ui->defaultTree)
			fprintf(f, "NDS_PREFERRED_TREE=%s\n", ui->defaultTree);
		if (ui->defaultNameCtx)
			fprintf(f, "NDS_PREFERRED_NAME_CTX=%s\n", ui->defaultNameCtx);

		// v 1.19 added a new account flag to ease zenscript processing
		// is account is new, we can further personnalize config files such as kmailrc, liprefs.js ...
		fprintf(f, "NDS_IS_NEW_USER=%s\n", (ui->isNewUser) ? "1" : "0");

		fprintf(f, "NDS_ZEN_FLAG=0x%lx\n", ui->zenFlag);
		switch (ui->zenFlag && (ZF_BROADCAST_ALL + ZF_BROADCAST_CONSOLE)) {
		case ZF_BROADCAST_ALL:
			fprintf(f, "NDS_BCAST=2\n");
			break;
		case ZF_BROADCAST_CONSOLE:
			fprintf(f, "NDS_BCAST=1\n");
			break;
		default:
			fprintf(f, "NDS_BCAST=0\n");
			break;
		}
		fclose(f);
		//perm_err=chmod(nwinfos,0600) || chown(nwinfos,pwd->pw_uid,pwd->pw_gid);
		perm_err = chmod(nwinfos, 0600);
		seteuid(currentUID);
		if (perm_err)
			syslog(LOG_DEBUG, "problem %d (%s)changing permissions to %s\n", perm_err, strerror(errno), nwinfos);
		if (ui->qflag & QF_DEBUG)
			syslog(LOG_DEBUG, "done writing %s \n", nwinfos);

	} else {
		seteuid(currentUID);
		syslog(LOG_DEBUG, "Cannot open %s: %s\n", nwinfos, strerror(errno));
	}

	return 0;
}

// starting at rev. 1.19 zencripts are launched via su -l user for security reasons !

static int
exechelper2(const char *program,	// script name just to test if the command exist
	    const char *argv[],		// extras arguments to -c program
	    const char *unixname,	// user to su to
	    int verbose)
{
	struct stat stt;

	if (verbose) {
		if (unixname) {
			syslog(LOG_NOTICE, "launching %s %s %s as %s\n", program, argv[1], argv[2], unixname);
		} else {
			syslog(LOG_NOTICE, "launching %s %s %s as root\n", program, argv[1], argv[2]);
		}
	}
	if (stat(program, &stt) != 0) {
		if (verbose)
			syslog(LOG_NOTICE, "%s not found\n", program);
		return -1;
	}
	return exechelper(program, argv, unixname);
}

static int
process_zenflag_session_opening(const char *user, const struct nw_user_info *ui, const struct passwd *pwd)
{

	int err;
	if (ui->qflag & QF_DEBUG)
		syslog(LOG_DEBUG, "APPLYING ZF %lx\n", ui->zenFlag);

	if (ui->zenFlag & ZF_CREATE_NWINFOS)
		err = nw_process_nwinfos_file(user, ui, pwd);
	if (ui->zenFlag & ZF_OPENING_SCRIPTS) {
		const char *argv[5];

		argv[1] = pwd->pw_dir;	// argument 1 = location of nwinfos file to source in
		argv[2] = NWINFOS_FILE;	// actual file name
		argv[3] = NULL;
		// argument 1 = location of nwinfos file to source in (=user's home dir)
		//argument 2 = actual file name (default=.nwinfos)

		if (ui->qflag & QF_DEBUG)
			syslog(LOG_NOTICE, "running opening scripts.\n");
		if (ui->zenFlag & ZF_0)
			err = exechelper2(ZEN_SCRIPT_0, argv, user, ui->qflag & QF_DEBUG);
		if (ui->zenFlag & ZF_1)
			err = exechelper2(ZEN_SCRIPT_1, argv, user, ui->qflag & QF_DEBUG);
		if (ui->zenFlag & ZF_2)
			err = exechelper2(ZEN_SCRIPT_2, argv, user, ui->qflag & QF_DEBUG);
	}
#if 1
	err = 0;		// ignore errors for now
#endif
	return err;
}

PAM_EXTERN int
pam_sm_open_session(pam_handle_t * pamh, UNUSED(int flags), int argc, const char **argv)
{
	/* NO WAY to mount netware home here
	   passord is lost, we are in the pam_session now.
	   I could have stored it in nw-user_info struct saved in PAM data
	   but I am not sure this is really safe !!!
	   we only process here some ZENFLAGS red from NDS
	   and stored in the PAM item "pam.ncpfs.user_info"
	 */
	struct ncp_conn *conn;
	struct nw_user_info *ui;
	const struct passwd *pwd;
	struct stat stt;
	const char *user;
	int err, c;

	int qflag = QF_VERBOSE;

	openlog("pam_ncp_auth", LOG_PID, LOG_AUTHPRIV);
	/* Get options */
	for (c = 0; c < argc; c++) {
		if (argv[c][0] == '-') {
			int i;

			for (i = 1; argv[c][i]; i++) {
				switch (argv[c][i]) {
				case 'v':
					qflag |= QF_VERBOSE;
					break;	/* verbose */
				case 'q':
					qflag &= ~QF_VERBOSE;
					break;	/* quiet */
				case 'd':
					qflag |= QF_DEBUG;
					break;	/* debug */
				default:;	/* just silently ignore unknown option... */
				}
			}
		}
	}

	/*****************************************************
        if (qflag & QF_DEBUG) {
           test_pam_items ("session closing",pamh);
        }
        *****************************************************/
	if (qflag & QF_DEBUG) {
		syslog(LOG_NOTICE, "start of session \n");
	}
	// just in case user has been removed ???
	err = my_pam_get_item(pamh, PAM_USER, &user);
	if (err != PAM_SUCCESS || user == NULL || *user == '\0')
		goto doneok;

	setpwent();
	pwd = getpwnam(user);
	endpwent();
	if (pwd == NULL) {
		syslog(LOG_DEBUG, "%s not found\n", user);
		goto doneok;
	}
	if (stat(pwd->pw_dir, &stt) != 0) {
		syslog(LOG_DEBUG, "Unix home of %s not found !\n", user);
		goto doneok;
	}
	// get back user's info stored in PAM by the authentification section
	// no password there !
	err = my_pam_get_data(pamh, "pam.ncpfs.user_info", &ui);
	if (!err) {
		if (qflag & QF_DEBUG)
			syslog(LOG_NOTICE, "got user info back %u", ui->uid);
		process_zenflag_session_opening(user, ui, pwd);
	} else {
	        if (qflag & QF_DEBUG)
			syslog(LOG_DEBUG, "failure reading back pam.ncpfs.user_info %u\n", err);
	}
	// no problem, he is authenticated but "zen failed"
doneok:
	// revision  1.27 we get rid of the authenticating connection .
	// it is not in /etc/mtab, so it caoonot be "reused" by ncpmap anyway
	err = my_pam_get_data(pamh, "pam.ncpfs.passwd.conn", &conn);
	if (!err && conn) {
		err = pam_set_data(pamh, "pam.ncpfs.passwd.conn", NULL, NULL);
		if (qflag & QF_DEBUG)
			syslog (LOG_NOTICE,"pam start of session :setting internal conn to NULL %x",err);
	}
	return PAM_SUCCESS;
}

// starting at rev. 1.19 zencripts are lauched via su -l user for security reasons !
static int
process_zenflag_session_closing(const char *user, const struct nw_user_info *ui, const struct passwd *pwd)
{
	int err = 0;

	// first zenScripts in case they opeate on the mounted nwhome !
	if (ui->zenFlag & ZF_CLOSING_SCRIPTS) {
		const char *argv[5];

		argv[1] = pwd->pw_dir;	// argument 1 = location of nwinfos file to source in
		argv[2] = NWINFOS_FILE;	// actual file name
		argv[3] = NULL;
		// argument 1 = location of nwinfos file to source in (=user's home dir)
		// argument 2 = actual file name (default=.nwinfos)

		if (ui->qflag & QF_DEBUG)
			syslog(LOG_NOTICE, "running closing scripts.\n");
		if (ui->zenFlag & ZF_3)
			err = exechelper2(ZEN_SCRIPT_3, argv, user, ui->qflag & QF_DEBUG);
		if (ui->zenFlag & ZF_4)
			err = exechelper2(ZEN_SCRIPT_4, argv, user, ui->qflag & QF_DEBUG);
		if (ui->zenFlag & ZF_5)
			err = exechelper2(ZEN_SCRIPT_5, argv, user, ui->qflag & QF_DEBUG);
	}

	if (ui->zenFlag & ZF_AUTOMOUNT_NWHOME) {
		err = umount_nwhome(user, ui->nwhomeMntPnt, ui->qflag & QF_DEBUG);
	}
#if 1
	err = 0;		// ignore error
#endif
	return err;
}

static int
pam_do_close_session(const char *user, const struct nw_user_info *ui, const struct passwd *pwd)
{

	return process_zenflag_session_closing(user, ui, pwd);
}

PAM_EXTERN int
pam_sm_close_session(pam_handle_t * pamh, UNUSED(int flags), int argc, const char **argv)
// logout fom Netware at session end
// no complains
{
	struct nw_user_info *ui;
	long err;
	const char *user;
	const struct passwd *pwd;
	struct stat stt;

	int qflag = QF_VERBOSE;
	int c;
	openlog("pam_ncp_auth", LOG_PID, LOG_AUTHPRIV);
	/* Get options */
	for (c = 0; c < argc; c++) {
		if (argv[c][0] == '-') {
			int i;

			for (i = 1; argv[c][i]; i++) {
				switch (argv[c][i]) {
				case 'v':
					qflag |= QF_VERBOSE;
					break;	/* verbose */
				case 'q':
					qflag &= ~QF_VERBOSE;
					break;	/* quiet */
				case 'd':
					qflag |= QF_DEBUG;
					break;	/* debug */
				default:;	/* just silently ignore unknown option... */
				}
			}
		}
	}

	/*****************************************************
        if (qflag & QF_DEBUG) {
           test_pam_items ("session closing",pamh);
        }
        *****************************************************/
	if (qflag & QF_DEBUG) {
		syslog(LOG_NOTICE, "end of session\n");
	}
// just in case user has been removed ???
	err = my_pam_get_item(pamh, PAM_USER, &user);
	if (err != PAM_SUCCESS || user == NULL || *user == '\0')
		goto doneok;

	setpwent();
	pwd = getpwnam(user);
	endpwent();

	if (pwd == NULL) {
		syslog(LOG_NOTICE, "%s not found\n", user);
		goto doneok;
	}

	if (stat(pwd->pw_dir, &stt) != 0) {
		syslog(LOG_NOTICE, "Unix home of %s not found !\n", user);
		goto doneok;
	}
	// read back all user data
	err = my_pam_get_data(pamh, "pam.ncpfs.user_info", &ui);
	if (!err) {
		if (qflag & QF_DEBUG)
			syslog(LOG_NOTICE, "got it back %u", ui->uid);
	} else {
		if (qflag & QF_DEBUG)
			syslog(LOG_DEBUG, "failed reading pam.ncpfs.user_info %lu\n", err);
		goto doneok;
	}
	pam_do_close_session(user, ui, pwd);
	free_nw_user_info(ui);	// forget about it
doneok:
	closelog();
	return PAM_SUCCESS;
}

/* static module data */
#ifdef PAM_STATIC
struct pam_module _pam_ncp_auth_modstruct = {
	"pam_ncp_auth",
	pam_sm_authenticate,
	pam_sm_setcred,
	pam_sm_acct_mgmt,
	pam_sm_open_session,	// session IN  was NULL PP
	pam_sm_close_session,	// session OUT was NULL PP
	pam_sm_chauthtok,
};
#endif
