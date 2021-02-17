/*-------------------------------------------
NWADmin snapin for PAM_NCP_AUTH module

version 1.00a (c) Patrick Pollet
  patrick.pollet@insa-lyon.fr
detailled documentation:
at http://cipcinsa.insa-lyon.fr/ppollet/pamncp/

I am not sure this program cant be distributed
as "Open source" since it use the Netware SDK for C(c)
and the NetWare API for snapins(c)
--------------------------------------------*/
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <nwnet.h>
#include <nwsnapin.h>
#include <nwdsattr.h>
#include <nwlocale.h>
#include <nwfile.h>
#include "pamncp.h"

// if 1 will use dummy NDS8 attributes (LINUX:*)
// created with ShemaX ( see define below for
// the appropriate names)

// #define USING_DUMMY_ATTRIBUTES   1

// UNDEFINE to USE THE REAL ONES UNIX:*
// Either way , the snapin will always try to get *missing* data
// from the L attribute
// Of course NDS8 real or dummy attributes have priority, that is
// if both exists NDS8 attribut will keep their values and the
// L atribute value will be erased when saving the NDS
/* user
	 U:nnn       Unix UID
	 G:nnn       Unix GID
	 S:/.../...  Unix Shell
	 H:/.../.... Unix Home
  Group
	  G:nnn      Unix GID

 Currently the following attributes recognized by the PAM module
 are NOT modifiable by this snapin. You should modify them
 using the "built-in" string editor of NWADmin under the Identification Page
  ( Location attribute):
  User:
	  O: Unix or NDS name of other group ( if that group is a NDS name
			and has a N:nnnn attribute, this value will be used for
			 translation to Unix name, otherwise, a "proper Unix name"
			 will be made from the NDS one ( remove context part,
			 skip all spaces and illegal chars and prepend a Z if its
			 starts by a digit (  this is required by useradd/usermod
			 that misbehave with a group name starting with a digit)
		C: Unix comment, added to the Gecos field after the full name
		(-comma separator added)
		P: Primary group name. So far the snapin works with the numeric
			  Primary Group ID for user's. It may change in the next release.

 Of course any other L (location) strings is left untouched by the snapin
  ( no error message)!
 */

/*------------------------------------------------------------------------*/
/*  Function Prototypes                                                   */
/*------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C"
{
#endif

NWA_SNAPIN_CALLBACK (NWRCODE)
 SnapinPamNcpProcUser (pnstr8 name, nuint16 msg, nparam p1, nparam p2);

NWA_SNAPIN_CALLBACK (NWRCODE)
 SnapinPamNcpProcGroup (pnstr8 name, nuint16 msg, nparam p1, nparam p2);

// DLG proc: the same for users and groups
BOOL FAR PASCAL PamNcpSnapinUserPageDlg (
	HWND hwnd,
	UINT message,
	WPARAM wParam,
	LPARAM lParam);

// DLG proc fror help dialog
BOOL FAR PASCAL PamNcpSnapinHelpDlg (
	HWND hwnd,
	UINT message,
	WPARAM wParam,
	LPARAM lParam);

// DLG proc for trace dialog
BOOL FAR PASCAL PamNcpSnapinTraceDlg (
	HWND hwnd,
	UINT message,
	WPARAM wParam,
	LPARAM lParam);

NWDSContextHandle GetMeAContext(char * caller);
NWDSCCODE ReadObjectData (NWDSContextHandle cx, char *objectName);
NWDSCCODE ModifyObjectDataDS7 ( NWDSContextHandle cx ,char *objectName);
NWDSCCODE ModifyObjectDataDS8 ( NWDSContextHandle cx ,char *objectName,int page);
NWDSCCODE ReadAttributeDef (NWDSContextHandle cx, char* attrname,Attr_Info_T *attrInfo);
NWDSCCODE ReadUnheritedZenFlags ( char *objectName);

void ShowAdminPreference (void);
void ReadLinuxProfileLoginScript (NWDSContextHandle cx);
int  UpdateLinuxProfileLoginScript ( NWDSContextHandle cx,char* objectName,long nextUID,long nextGID);
void setZenCheckBoxes ( HWND hwnd, char * zenString);
void getZenCheckBoxes ( HWND hwnd, char * zenString);
void setZenCheckBoxesBis ( HWND hwnd, char * zenString) ;
void clearZenCheckBoxesBis ( HWND hwnd) ;
void showHideZenCheckBoxesBis ( HWND hwnd, int show) ;
void mergeZen (char * z1, char *z2);
void trim (char * string);
int doPrompt(int page) ;
void Infos (char *message);
void Debug (char *message);
void Trace (char * 	fmt,...);
void getNameAndContext (char* sourceDistName ,char* nameBuffer,char* contextBuffer);
long displayError	(char* prelude,char*	AppRoutineName,char*	SysRoutineName,long errVal);
void displayMessage(	char*	AppRoutineName,char*	Message );

 #ifdef __cplusplus
}
#endif

/*------------------------------------------------------------------------*/
/*  Defines and structures                                                */
/*------------------------------------------------------------------------*/
#define ME "Pam Ncp authentication"
#define VERSION  "version 1.00 (c) Patrick Pollet INSA Lyon 2001"
#define VERSION_NUM "v 1.00a"

/**** translatable strings ******/
// to translate you must also edit the pamncp.rc file
#define SHOW_UNH "Show unherited"
#define HIDE_UNH "Hide unherited"
#define FROM_DS7 " Basic Unix properties (from Location): "
#define FROM_DS8 " Basic Unix properties (from eDirectory): "
#define UID_INVALID  "the Unix user ID must be a number."
#define UID_OUTRANGE "the Unix user ID %d is outside permitted range [%d to %d]."
#define GID_INVALID  "the Unix group ID must be a number."
#define GID_OUTRANGE "the Unix group ID %d is outside permitted range [%d to %d]."
#define NO_DEF_LINUX_PROFILE "no default Linux profile"

// names of NDS attributes used

#ifndef USING_DUMMY_ATTRIBUTES
// the real ones
#define ATTR_UID         "UNIX:UID"
#define ATTR_PGNAME 	    "UNIX:Primary GroupName"
#define ATTR_PGID        "UNIX:Primary GroupID"
#define ATTR_GID         "UNIX:GID"
#define ATTR_SHELL       "UNIX:Login Shell"
#define ATTR_COM         "UNIX:Comments"
#define ATTR_HOME	       "UNIX:Home Directory"
#else
// created with Schemax with the same syntax
// and associated to user class and group class
#define ATTR_UID         "LINUX:UID"
#define ATTR_PGNAME 	    "LINUX:Primary GroupName"
#define ATTR_PGID        "LINUX:Primary GroupID"
#define ATTR_GID         "LINUX:GID"
#define ATTR_SHELL       "LINUX:Login Shell"
#define ATTR_COM         "LINUX:Comments"
#define ATTR_HOME	       "LINUX:Home Directory"
#endif

// the attribute used to test presence of NDS8
// either real or dummy
#define ATTR_NDS8         ATTR_UID

// other attributes used
// absent NDS8 attributes are searched in L attribute
// also new properties ( Zenux Flags, Other group...)
#define ATTR_LOCATION     "L"

#define ATTR_LOGIN_SCRIPT "Login Script"
#define ATTR_GRP_MBS       "Group Membership"


// syntaxes of the used attributes
#define SYN_LOCATION     SYN_CI_STRING
#define SYN_LOGIN_SCRIPT SYN_STREAM
#define SYN_UID          SYN_INTEGER
#define SYN_PGNAME 	    SYN_DIST_NAME
#define SYN_PGID         SYN_INTEGER
#define SYN_GID          SYN_INTEGER
#define SYN_SHELL        SYN_CE_STRING
#define SYN_COM          SYN_CI_STRING
#define SYN_HOME	       SYN_CE_STRING
#define SYN_GRP_MBS      SYN_DIST_NAME


// misc
#define MAX_STRINGS         32
#define EGAL " = "
#define CR_LF    "\r\n"  // was inverted in v <105d
#define YES "Y"
#define NO "N"


// snapin defaults are searched in the login script
// of a Profile object named "Linux Profile"
// search starts in current objet context and , if
// not found, move up contexts up to [ROOT]
// search DOES not go in "brother contexts" at any level.
// the first found get the ticket ( no merging of defaults)
#define LINUX_PROFILE  "Linux Profile"

/*** sample login script (case sensitive for now)
no space at the beginning of any line !
comment with any caracter (# for readability), but
a simple space will do !
[common]
debug=0
prompt=1
[user]
# range of valid Unix UID
minUID=1000
maxUID=60000
# range of valid Unix primary group GID
minGID=100
maxGID=60000
# default value for primary GID
defGID=100
#default shell
shell=/bin/bash
#default home.
#the common name of current user will be appended
#after conversion to lower case
#the ending / is REQUIRED
home=/home/
#default Zenux Flag for user
#F,T: allow remote access by FTP and Telnet
#M: create a ~/.forward file with the NDS property
#Email Address(obsolete) or Internet Email Address
zen=FMT

[group]
# range of valid group ID
minGID=101
maxGID=60000
#default Zenux Flag for members
#A=automount Netware home directory in ~/nwhome
zen=A

[current]
#this section MUST stay at the end
#since it will be overwritten by the snapin
#in case you selected "automatic values"
#it keeps in NDS the next values to be used
#thus ensuring unique IDS in that context and all BELOW.
nextUID=2000
nextGID=550
****************/

#define USER_SECTION "[user]"
#define GRP_SECTION "[group]"
#define CUR_SECTION "[current]"
#define COM_SECTION "[common]"

#define MYHOME     "\nhome"
#define MYSHELL     "\nshell"
#define MYZEN       "\nzen"
#define MYMINUID    "\nminUID"
#define MYMAXUID    "\nmaxUID"
#define MYMINGID    "\nminGID"
#define MYMAXGID    "\nmaxGID"
#define MYDEFGID    "\ndefGID"
#define MYNEXTUID   "\nnextUID"
#define MYNEXTGID   "\nnextGID"
#define MYDEBUG     "\ntrace"
#define MYPROMPT     "\nprompt"

// default vlues if Linux Profile not found or incomplete
#define DEF_MINUID      1000
#define DEF_MAXUID      60000
#define DEF_MINGID_USR  500
#define DEF_MAXGID_USR  30000
#define DEF_GID_USR     100
#define DEF_MINGID_GRP  500
#define DEF_MAXGID_GRP  30000
#define DEF_ZEN_USR  ""
#define DEF_ZEN_GRP  ""
#define DEF_HOME    "/home/"
#define DEF_SHELL   "/bin/bash"



// what has been red from NDS ( what may has changed)
// set when reading but not used (yet)
#define HAD_UID8   	0x0001
#define HAD_GID8   	0x0002
#define HAD_GID18  	0x0004
#define HAD_SHELL8 	0x0008
#define HAD_HOME8  	0x0010
#define HAD_PGNAME8 	0x0020
#define HAD_COM8  	0x0040
#define HAD_PGID8  	0x0080
#define HAD_NAME8  	0x0100
#define HAD_ZEN      0x0200
// important flag. If the user had NO location attribute
// we muste create it before adding strings to it.
#define HAD_LOCATION 0x8000

// global variables needed by Snapin API
#define NUM_PAGES_USER      1
#define NUM_PAGES_GRP       1
#define PAGE_USER 0
#define PAGE_GROUP 1

HINSTANCE hDLL;
NWAPageStruct userPages[NUM_PAGES_USER];   // pages supplementaires
NWAPageStruct groupPages[NUM_PAGES_GRP];   // pages supplementaires


// my global variables.
int currentPage =PAGE_USER;  // we are editing User or Group object
long  DEBUG=0;
nbool bNDS8Around = N_FALSE;     // true if NDS8 installed (with UNIX:* attributes)
char currentObject[MAX_DN_CHARS]; // DN of current user
char linuxProfileObjectUsed[MAX_DN_CHARS]="";

// some day I will use this  struct ... ;-)
struct uInfo {
	char initValues[MAX_DN_CHARS][MAX_STRINGS];  // initial values of Location
	int nInitValues;       // number of loaction red from NDS
	char newValues[MAX_DN_CHARS][MAX_STRINGS];  // final values of Location
	int nNewValues;       // number of location to write back to NDS
	long currentUID;
	long currentGID;
	char currentSHELL[MAX_DN_CHARS];
	char currentHOME[MAX_DN_CHARS];
	char currentPGNAME[MAX_DN_CHARS];
	char currentZEN[MAX_DN_CHARS];
	char currentCOMMENT[MAX_DN_CHARS];
	char unheritedZEN[MAX_DN_CHARS];

	long newUID;
	long newGID;
	char newSHELL[MAX_DN_CHARS];
	char newHOME[MAX_DN_CHARS];
	char newPGNAME[MAX_DN_CHARS];

	int currentReadFLAG; // what has been red
	int bChanged; //  has changed ?
	int bShouldUpdateCurrent;
};

char initValues[MAX_DN_CHARS][MAX_STRINGS];  // initial values of Location
int nInitValues=0;       // number of loaction red from NDS
char newValues[MAX_DN_CHARS][MAX_STRINGS];  // final values of Location
int nNewValues=0;       // number of location to write back to NDS
long currentUID=-1;
long currentGID=-1;
char currentSHELL[MAX_DN_CHARS]="";
char currentHOME[MAX_DN_CHARS]="";
char currentPGNAME[MAX_DN_CHARS]="";
char currentZEN[MAX_DN_CHARS]="";
char currentCOMMENT[MAX_DN_CHARS]="";
char unheritedZEN[MAX_DN_CHARS]="";

long newUID=-1;
long newGID=-1;
char newSHELL[MAX_DN_CHARS]="";
char newHOME[MAX_DN_CHARS]="";
char newPGNAME[MAX_DN_CHARS]="";

int currentReadFLAG =0; // what has been red
int bChanged =0; //  has changed ?
int bShouldUpdateCurrent=0;

// these values are set to contents of Linux Profile
// or reset to default values at every opening
// of a detail page ...
long gMinUID=-1;
long gMaxUID=-1;
long gNextUID=-1;
long gMinGID=-1;
long gMaxGID=-1;
long gNextGID=-1;
long gDefGID=-1;
char gDefHome[MAX_DN_CHARS]="";
char gDefShell[MAX_DN_CHARS]="";
char gDefZen[MAX_DN_CHARS]="";
long gPrompt =0; // ask confirm before "stuffing in" next and default values

NWDSContextHandle MyContext;

// must be static to stay !on screen between users !!!
static nbool bTraceWindowVisible=FALSE;
static HWND TraceWindowhwnd;

// content of the comboBox for possible shells
struct shellElement {
	char * name;
	char * shell;
};

static struct shellElement SHELLS []={
{"Bourne","/bin/bash"},
{"C","/bin/tch"},
{"Korne","/bin/ksh"},
{"Other",""}
};
#define NB_SHELLS 4

/*****************************************************************************/
// must be called at every call with NWA_GET_PAGECOUNT
// that is just before opening the user or group details multipage dialog
// name = FQDN of object being edited
/*****************************************************************************/
int doInitValues (char * name, int page) {

	int retERR=NWA_RET_SUCCESS;

	currentPage=page; // don't forget that !
	currentReadFLAG =0; // what has been red
	bChanged=FALSE;
	bShouldUpdateCurrent=FALSE;
	memset(initValues,0,sizeof(initValues));
	memset(linuxProfileObjectUsed,0,sizeof(linuxProfileObjectUsed));
	nInitValues=0;
	nNewValues=0;
	memset(currentObject,0,sizeof(currentObject));
	currentUID=-1;
	currentGID=-1;
	newUID=-1;
	newGID=-1;
	memset(currentSHELL,0,sizeof(currentSHELL));
	memset(currentHOME,0,sizeof(currentHOME));
	memset(currentPGNAME,0,sizeof(currentPGNAME));
	memset(newSHELL,0,sizeof(newSHELL));
	memset(newHOME,0,sizeof(newHOME));
	memset(newPGNAME,0,sizeof(newPGNAME));

	memset(currentZEN,0,sizeof(currentZEN));
	memset(currentCOMMENT,0,sizeof(currentCOMMENT));
	memset(unheritedZEN,0,sizeof(unheritedZEN));
	if (name) { // should be ??
	  lstrcpy(currentObject,name);
	  MyContext=GetMeAContext("PamNcp Snapin:doInitValues");
	  retERR=NWA_ERR_ERROR;
	  if (MyContext !=(NWDSContextHandle) ERR_CONTEXT_CREATION)
		 {
			// doit first, the DEBUG flag may be modified there
			ReadLinuxProfileLoginScript (MyContext);
			if (DEBUG && !bTraceWindowVisible) {
				 CreateDialog (hDLL,"TRACE_DLG",NULL,PamNcpSnapinTraceDlg);
				 bTraceWindowVisible=TRUE;
			}
			bNDS8Around=ReadAttributeDef (MyContext,ATTR_NDS8,NULL)==0;
			ShowAdminPreference();
			// what to do if error ?
			// if I return NWA_ERR_ERROR it may stop the loading
			// of all the user's detail page !!!
			if (!ReadObjectData(MyContext,currentObject))
				retERR=NWA_RET_SUCCESS; // OK OK
			NWDSFreeContext (MyContext);
		 }
	}
	if (bTraceWindowVisible)
		if (nInitValues)
		  Trace ("%d Unix properties found in L. ",nInitValues);
		 else
			Trace ("No  Unix properties found in L.");
	Trace ("End of process IniSnapin: %s.","doInitValues");
	return retERR;
}


int doSaveValues (void ) {
NWDSCCODE err;
	  Trace ("Begin of process:PamNcp Snapin:doSaveValues:%s"," ");
	  if (bChanged  && currentObject[0]) { // really important !
		  MyContext=GetMeAContext("PamNcp Snapin:doSaveValues:");
		  if (MyContext !=(NWDSContextHandle) ERR_CONTEXT_CREATION)
			 {
				 err=ModifyObjectDataDS7(MyContext,currentObject);
				 if (!err && bNDS8Around)
					err= ModifyObjectDataDS8(MyContext,currentObject,currentPage);
				 if (!err && bShouldUpdateCurrent) {
						 Trace (" should update current to NUID=%d NGID%d ",gNextUID,gNextGID);
						 UpdateLinuxProfileLoginScript (MyContext,linuxProfileObjectUsed,gNextUID,gNextGID);
				 }
				 NWDSFreeContext (MyContext);
			 }
		}
		// MessageBox (0,"Trouble with Context" ,"",MB_OK);
	 Trace ("End of process:PamNcp Snapin:doSaveValues:%s"," ");
	 return 0;
}

int getNumberAndCheck ( char * buffer, long minV, long maxV,
							  long * foundV, char * msgInvalid, char* msgOutOfRange){

  char *v; long n;
  char aux[256];

  trim(buffer); // removes spaces
  if (buffer[0]) {
	  n=strtoul(buffer,&v,10);
	  if (n) {
		 if (n>=minV && n<=maxV) {
			 *foundV=n;
			 return 0;
		 }
		 else {
		  sprintf (aux,msgOutOfRange,n,minV,maxV);
		  displayMessage (ME,aux);
		 }
	  }
	  else displayMessage(ME,msgInvalid);
	  return -1;
  }
 *foundV =-1; // just says it's empty
 return 0; // OK if empty
}


int collectValues (HWND hwnd, int page) {
char aux [MAX_DN_CHARS+1];
// this is the point where I should set bChanged to true or false
  nNewValues=0;
  getZenCheckBoxes (hwnd, aux);
  if (strlen(aux))  // not empty
	 sprintf(newValues[nNewValues++],"Z:%s",aux);

  if (page==PAGE_GROUP) {
	 // GetDlgItem() returns the number of caracters copied or 0 if fails !
	 // this edit line is hidden in user oject page
	 if (GetDlgItemText(hwnd,IDC_EDIT_GID1,aux,sizeof(aux))) {
		if (getNumberAndCheck(aux,gMinGID,gMaxGID,&newGID,GID_INVALID,GID_OUTRANGE))
			return 1;
		if (!bNDS8Around) // goes to a location string
			sprintf(newValues[nNewValues++],"G:%s",aux);
	 }else newGID=-1; // empty

	 // this edit line is hidden in user oject page
	 if (GetDlgItemText(hwnd,IDC_EDIT_GNAME,aux,sizeof(aux)))
			sprintf(newValues[nNewValues++],"N:%s",aux); // not a NDS8 attribute

  }else { // unneeded test since the others are "non visible" and empty

	 if (GetDlgItemText(hwnd,IDC_EDIT_UID,aux,sizeof(aux))) {
			if (getNumberAndCheck(aux,gMinUID,gMaxUID,&newUID,UID_INVALID,UID_OUTRANGE))
				return 1;
		if (!bNDS8Around)
			sprintf(newValues[nNewValues++],"U:%s",aux);
	 }else newUID=-1; //empty

	 if  (GetDlgItemText(hwnd,IDC_EDIT_GID,aux,sizeof(aux))) {
			if (getNumberAndCheck(aux,gMinGID,gMaxGID,&newGID,GID_INVALID,GID_OUTRANGE))
				return 1;
		if (!bNDS8Around)
			sprintf(newValues[nNewValues++],"G:%s",aux);
	 }else newGID=-1; //empty

	 if  (GetDlgItemText(hwnd,IDC_EDIT_SHELL,aux,sizeof(aux)))
		if (!bNDS8Around)
			sprintf(newValues[nNewValues++],"S:%s",aux);
		else
			 sprintf(newSHELL,aux); // for ModifyObjectNDS8
	 // newSHELL stays empty otherwise

	 if (GetDlgItemText(hwnd,IDC_EDIT_HOME,aux,sizeof(aux)))
			if (!bNDS8Around)
				sprintf(newValues[nNewValues++],"H:%s",aux);
			else
			 sprintf(newHOME,aux); // for ModifyObjectNDS8
	 //newHOME stays empty otherwise
	}
 return 0;
}

#define MODEL_MSG_USR "The following values will be used: \n UID= %d \n GID= %d \n Home=%s \n Shell= %s \n Zenux=%s \n"
#define MODEL_MSG_GRP "The following values will be used: \n GID= %d\n Zenux=%s \n"
int doPrompt (int page) {
char message [512];
  if (page==PAGE_USER)
	 sprintf(message,MODEL_MSG_USR,gNextUID,gDefGID,gDefHome,gDefShell,gDefZen);
  else
	 sprintf(message,MODEL_MSG_GRP,gNextGID,gDefZen);

	return (MessageBox (NULL,message,ME, MB_YESNO+MB_ICONQUESTION)==IDYES);

}

/*-------------------------------------------------------------------------*/
/* Function : DllEntryPoint (32-bit)                                       */
/*            LibMain (16-bit)                                             */
/* From Snapin API demos                                                   */
/* Description :                                                           */
/*   Required for DLL startup                                              */
/* don't ask me what is does...                                            */
/*-------------------------------------------------------------------------*/
#ifdef WIN32
#ifdef __BORLANDC__
// the only case I have tested
BOOL WINAPI DllEntryPoint (
  HINSTANCE hinstDLL,
  DWORD fdwReason,
  LPVOID lpvReserved)
#else
BOOL WINAPI DllMain (
  HINSTANCE hinstDLL,
  DWORD fdwReason,
  LPVOID lpvReserved)
#endif
{
 (void) lpvReserved; // warnings out !
 switch (fdwReason)
 {
  case DLL_PROCESS_ATTACH:
  {
	hDLL = hinstDLL;
  }
  /* fall through to thread attach code */
  case DLL_THREAD_ATTACH:
  {
	break;
  }
  case DLL_THREAD_DETACH:
  {
	break;
  }
  case DLL_PROCESS_DETACH:
  {
	break;
  }
  default:
  {
	break;
  }
 }
 return 1;
}

#else
int FAR PASCAL LibMain (
  HINSTANCE hInstance,
  WORD wDataSeg,
  WORD cbHeapSize,
  LPSTR lpCmdLine)
{

 hDLL = hInstance;
 if (cbHeapSize != 0)
 {
  UnlockData (0);
 }
 return 1;
};

#endif

/*-------------------------------------------------------------------------*/
/* Function : InitSnapin                                                   */
/*                                                                         */
/* Description :                                                           */
/*    Every Snapin DLL must provide this function. In this function,       */
/*    snapin menu items (under Tools) are registered. Also, object procs   */
/*    are registered.                                                      */
/*                                                                         */
/*-------------------------------------------------------------------------*/
int FAR PASCAL InitSnapin ()
{
 NWARegisterObjectProc (
	NWA_DS_OBJECT_TYPE,
	C_USER,    // we provide a page from class User
	VERSION,
	hDLL,
	SnapinPamNcpProcUser, // call me back here
	NWA_SNAPIN_VERSION);
  NWARegisterObjectProc (
	NWA_DS_OBJECT_TYPE,
	C_GROUP,    // we provide a page from class Group
	VERSION,
	hDLL,
	SnapinPamNcpProcGroup, // call me back here
	NWA_SNAPIN_VERSION);
 return NWA_RET_SUCCESS;
}


/*-------------------------------------------------------------------------*/
/* Description :                                                           */
/*   Snapin  Class Proc                                                    */
/* must be exported in .DEF					                                 */
/*-------------------------------------------------------------------------*/
N_GLOBAL_CALLBACK (NWRCODE)
SnapinPamNcpProcUser (pnstr8 name, nuint16 msg, nparam p1, nparam p2){

 switch (msg)
 {
  case NWA_MSG_INITSNAPIN:   {
	/*-------------------------------------------------*/
	/* initialize page(s)                              */
	/*-------------------------------------------------*/
	userPages[0].dlgProc = (DLGPROC) PamNcpSnapinUserPageDlg;
	userPages[0].resName = "PAMNCP_UPAGE";
	userPages[0].pageTitle =ME;
	userPages[0].initParam = 0;
	return NWA_RET_SUCCESS;
  }
  case NWA_MSG_GETPAGECOUNT:   {
	 if (!doInitValues(name,PAGE_USER))
			return NUM_PAGES_USER;
	 else return NWA_ERR_ERROR;
  }
  case NWA_MSG_REGISTERPAGE: {
	NWAPageStruct *pageInfo =(NWAPageStruct *)p2;
	pageInfo->dlgProc = userPages[p1].dlgProc;
	pageInfo->resName = userPages[p1].resName;
	pageInfo->pageTitle = userPages[p1].pageTitle;
	pageInfo->hDLL = hDLL;
	pageInfo->initParam = userPages[p1].initParam;
	return NWA_RET_SUCCESS;
  }
  case NWA_MSG_MODIFY: {
	doSaveValues();
	return NWA_RET_SUCCESS;
  }
  case NWA_MSG_CLOSESNAPIN: {
	 if (bTraceWindowVisible) {
		 DestroyWindow (TraceWindowhwnd);
		 bTraceWindowVisible=FALSE;
	 }
	 return NWA_RET_SUCCESS;
  }
  default: {
	  break;
  }
 }
 return NWA_RET_SUCCESS;
}


// about the same for group
// we use the same "front end" and hide or show some interface elements
/*-------------------------------------------------------------------------*/
/* Description :                                                           */
/*   Snapin  Class Proc                                                    */
/* must be exported in .DEF					                                 */
/*-------------------------------------------------------------------------*/
N_GLOBAL_CALLBACK (NWRCODE)
SnapinPamNcpProcGroup (pnstr8 name, nuint16 msg, nparam p1, nparam p2){

 switch (msg)
 {
  case NWA_MSG_INITSNAPIN:   {
	/*-------------------------------------------------*/
	/* initialize pages                                */
	/*-------------------------------------------------*/
	groupPages[0].dlgProc = (DLGPROC) PamNcpSnapinUserPageDlg;
	groupPages[0].resName = "PAMNCP_UPAGE";
	groupPages[0].pageTitle =ME;
	groupPages[0].initParam = 0;
	return NWA_RET_SUCCESS;
  }
  case NWA_MSG_GETPAGECOUNT:   {
	 if (!doInitValues(name,PAGE_GROUP))
			return NUM_PAGES_GRP;
	 else return NWA_ERR_ERROR;
  }
  case NWA_MSG_REGISTERPAGE: {
	NWAPageStruct *pageInfo =(NWAPageStruct *)p2;
	pageInfo->dlgProc = groupPages[p1].dlgProc;
	pageInfo->resName = groupPages[p1].resName;
	pageInfo->pageTitle = groupPages[p1].pageTitle;
	pageInfo->hDLL = hDLL;
	pageInfo->initParam = groupPages[p1].initParam;
	return NWA_RET_SUCCESS;
  }
  case NWA_MSG_MODIFY: {
	doSaveValues();
	return NWA_RET_SUCCESS;
  }
  case NWA_MSG_CLOSESNAPIN: {
	 // evite une GPF en quittant ???
	 if (bTraceWindowVisible) {
		 DestroyWindow (TraceWindowhwnd);
		 bTraceWindowVisible=FALSE;
	 }
	 return NWA_RET_SUCCESS;
  }
  default: {
	  break;
  }
 }
 return NWA_RET_SUCCESS;
}


/*-------------------------------------------------------------------------*/
/* Function : PamNCPSnapinPageDlg                                        */
/*                                                                          */
/* Description :                                                           */
/*   Page dialog proc to enhance User's or Group's details Details.         */
/*  no need to  export it  in .def file                                    */
/*-------------------------------------------------------------------------*/
BOOL FAR PASCAL PamNcpSnapinUserPageDlg (
  HWND hwnd,
  UINT message,
  WPARAM wParam,
  LPARAM lParam)  {

  char auxBuf [33];
  int i,selected;

(void) lParam; // warnings out !
 switch (message)
 {
	case WM_INITDIALOG:
	{// called if admin press my page button
	 // setup control
	Trace("Begin of process WM_INITDIALOG%s."," ");
	SetDlgItemText(hwnd,IDC_VERSION_NUM,VERSION_NUM);
	if (bNDS8Around)
		SetDlgItemText(hwnd,IDC_BASIC,FROM_DS8);
	else
		SetDlgItemText(hwnd,IDC_BASIC,FROM_DS7);
	setZenCheckBoxes (hwnd,currentZEN);

	ShowWindow (GetDlgItem(hwnd,IDC_EDIT_UID), currentPage==PAGE_USER);
	ShowWindow (GetDlgItem(hwnd,IDC_EDIT_GID), currentPage==PAGE_USER);
	ShowWindow (GetDlgItem(hwnd,IDC_COMBO_SHELL), currentPage==PAGE_USER);
	ShowWindow (GetDlgItem(hwnd,IDC_EDIT_SHELL), currentPage==PAGE_USER);
	ShowWindow (GetDlgItem(hwnd,IDC_EDIT_HOME), currentPage==PAGE_USER);
	ShowWindow (GetDlgItem(hwnd,IDC_UNHERITED), currentPage==PAGE_USER);
	ShowWindow (GetDlgItem(hwnd,IDC_UID_LBL), currentPage==PAGE_USER);
	ShowWindow (GetDlgItem(hwnd,IDC_GID_LBL), currentPage==PAGE_USER);
	ShowWindow (GetDlgItem(hwnd,IDC_HOME_LBL), currentPage==PAGE_USER);
	ShowWindow (GetDlgItem(hwnd,IDC_SHELL_LBL), currentPage==PAGE_USER);
	ShowWindow (GetDlgItem(hwnd,IDC_EDIT_GID1), currentPage!=PAGE_USER);
	ShowWindow (GetDlgItem(hwnd,IDC_GID1_LBL), currentPage!=PAGE_USER);
	ShowWindow (GetDlgItem(hwnd,IDC_EDIT_GNAME), currentPage!=PAGE_USER);
	ShowWindow (GetDlgItem(hwnd,IDC_GNAME_LBL), currentPage!=PAGE_USER);
	showHideZenCheckBoxesBis (hwnd,currentPage==PAGE_USER);

	// activate or not the "default button"
	if (linuxProfileObjectUsed[0]) {
	  SetDlgItemText(hwnd,IDC_DEFAULT,linuxProfileObjectUsed);
			  EnableWindow (GetDlgItem(hwnd,IDC_DEFAULT),TRUE);
	}else{
		 SetDlgItemText(hwnd,IDC_DEFAULT,NO_DEF_LINUX_PROFILE);
		 EnableWindow (GetDlgItem(hwnd,IDC_DEFAULT),FALSE);
	}
	if (currentPage==PAGE_USER)  {
		SetDlgItemText(hwnd,IDC_EDIT_HOME,currentHOME);
		SetDlgItemText(hwnd,IDC_EDIT_SHELL,currentSHELL);

		if (currentUID !=-1  && ultoa(currentUID,auxBuf,10))
			SetDlgItemText(hwnd,IDC_EDIT_UID,auxBuf);
		else
		  SetDlgItemText(hwnd,IDC_EDIT_UID,"");

		if (currentGID !=-1  && ultoa(currentGID,auxBuf,10))
			SetDlgItemText(hwnd,IDC_EDIT_GID,auxBuf);
		else
			SetDlgItemText(hwnd,IDC_EDIT_GID,"");

		selected=0;
		for ( i=0; i<NB_SHELLS; i++) {
			SendDlgItemMessage(hwnd,IDC_COMBO_SHELL,CB_ADDSTRING,0,(LPARAM)SHELLS[i].name);
			if (!strcmp (SHELLS[i].shell,currentSHELL))
				selected=i;
		 }
		SendDlgItemMessage(hwnd,IDC_COMBO_SHELL,CB_SETCURSEL,selected,0);
	 } else { // is a group
		if (currentGID !=-1  && ultoa(currentGID,auxBuf,10))
			SetDlgItemText(hwnd,IDC_EDIT_GID1,auxBuf);
		else
			SetDlgItemText(hwnd,IDC_EDIT_GID1,"");
		SetDlgItemText(hwnd,IDC_EDIT_GNAME,currentPGNAME);
	 }
	 // all changes made so far in controls are not significant!
	 bChanged=FALSE; // extremely important line !
	 Trace("End of process WM_INITDIALOG%s."," ");
	return TRUE;
  }
  case NWA_WM_F1HELP:
  {
	  // one day i'll learn how to make a real HLP file ...
	  /*********************
	  DialogBox (hDLL,"HELP_DLG",NULL,PamNcpSnapinHelpDlg);
	  **********************************/
	  return TRUE;
  }
  case NWA_WM_CANCLOSE:
  {
	 Trace("Begin of process NWA_CANCLOSE %s."," ");
	 if (collectValues(hwnd,currentPage)) return FALSE;
	 Trace("End of process NWA_CANCLOSE%s."," ");
	 return TRUE;
  }
  case WM_COMMAND:
	 switch (LOWORD(wParam))
		 {
			case IDC_UNHERITED: { // hidden in group page
				char aux[80];
				GetDlgItemText(hwnd,IDC_UNHERITED,aux,sizeof(aux));
				if (!strcmp(aux,SHOW_UNH)) {
					SetDlgItemText(hwnd,IDC_UNHERITED,HIDE_UNH);
					if (!ReadUnheritedZenFlags (currentObject))
							 setZenCheckBoxesBis(hwnd,unheritedZEN);
				}
				else {
					clearZenCheckBoxesBis(hwnd);
					SetDlgItemText(hwnd,IDC_UNHERITED,SHOW_UNH);
				}
			  }
			  break;
			case IDC_DEFAULT: {   // hidden is group page
			char aux[256];
			  if (gPrompt)
				  if (!doPrompt (currentPage)) break;

				 if (currentPage==PAGE_GROUP) {
					 SetDlgItemText(hwnd,IDC_EDIT_GID1,ltoa(gNextGID++,aux,10));
					 mergeZen(currentZEN,gDefZen);
					 setZenCheckBoxes(hwnd,currentZEN);
					 bShouldUpdateCurrent=TRUE;
				 }else{
				  bShouldUpdateCurrent=TRUE;
				  SetDlgItemText(hwnd,IDC_EDIT_UID,ltoa(gNextUID++,aux,10));
				  SetDlgItemText(hwnd,IDC_EDIT_GID,ltoa(gDefGID,aux,10));
				  SetDlgItemText(hwnd,IDC_EDIT_SHELL,gDefShell);
				  SetDlgItemText(hwnd,IDC_EDIT_HOME,gDefHome);
				  mergeZen(currentZEN,gDefZen);
				  setZenCheckBoxes(hwnd,currentZEN);
				 }
			}
			break;
			case IDC_COMBO_SHELL: // hidden is group page
				if (HIWORD(wParam)==CBN_SELCHANGE) {
					long dwIndex;
					dwIndex = SendDlgItemMessage(hwnd, IDC_COMBO_SHELL, CB_GETCURSEL, 0, 0);
					 if (dwIndex != CB_ERR && dwIndex <NB_SHELLS)
						 SetDlgItemText(hwnd, IDC_EDIT_SHELL,SHELLS[dwIndex].shell);
				}
			  break;
			case IDC_EDIT_UID: ;
			case IDC_EDIT_GID: ;
			case IDC_EDIT_HOME: ;
			case IDC_EDIT_SHELL: ;
			case IDC_EDIT_GID1: ;
			case IDC_EDIT_GNAME:
				  if (HIWORD(wParam)==EN_UPDATE) {
				  // turn on the little black triangle and ask Nwadmin to call be back
				  // for saving data
				  // not true: looks like NWADmin call me back with NWA_MSG_MODIFY
				  // even if I do not sent this message (???)
						SendMessage(hwnd,NWA_WM_SETPAGEMODIFY,(WORD) hwnd,MAKELONG (0, TRUE));
						Trace("WM_COMMAND called for an edit EN_UPDATE ID= %d.",LOWORD(wParam));
						bChanged=TRUE;
				  }
				  break;
		  default:
			if ( (LOWORD(wParam))>=IDC_ZF_A && (LOWORD(wParam))<=IDC_ZF_5) {
				SendMessage(hwnd,NWA_WM_SETPAGEMODIFY,(WORD) hwnd,MAKELONG (0, TRUE));
				Trace("WM_COMMAND called for an CBX ID= %d.",LOWORD(wParam));
				bChanged=TRUE;
			}
		}
	}
 return FALSE;
}

/************************************** DLG PROC for HELP window ***
 must be exported in .DEF
*******************************************************************/
BOOL FAR PASCAL PamNcpSnapinHelpDlg (
	HWND hwnd,
	UINT message,
	WPARAM wParam,
	LPARAM lParam){

(void) lParam; // warnings out !
 switch (message)
 {
	case WM_INITDIALOG:
  {
	  //MessageBox (0,"INIT DIALOG HELP","begin OK",MB_OK);
	 SetDlgItemText(hwnd,IDC_VERSION,VERSION);
	 SetDlgItemText(hwnd,IDC_MEMO,
"Blah Blah "
"\r\n"
);
 return TRUE;
  }
  case WM_COMMAND:
  {  switch (LOWORD(wParam) )
		 {
		 case IDOK:
		 EndDialog(hwnd,TRUE);
		 return TRUE;
		}
	}
 }
 return FALSE;
}

/************************************** DLG PROC for TRACE window ***
 must be exported in .DEF
*******************************************************************/
BOOL FAR PASCAL PamNcpSnapinTraceDlg (
	HWND hwnd,
	UINT message,
	WPARAM wParam,
	LPARAM lParam)

{  int len; char aux[512];
 switch (message)
 {
	case WM_INITDIALOG:
  {
	 SetDlgItemText(hwnd,IDC_VERSION,VERSION);
	 SetDlgItemText(hwnd,IDC_MEMO,"");
	 bTraceWindowVisible=TRUE;
	 TraceWindowhwnd=hwnd;
	 Trace ("Begin of trace mode %s.","");
	  if (currentPage==PAGE_USER)
			  Trace ("Current user:%s",currentObject);
		else
			Trace ("Current group:%s",currentObject);
	 Trace ("Using Linux Profile Object:%s",linuxProfileObjectUsed);
	 return TRUE;
  }
  case WM_CLOSE: // v 1.10  on peut la fermer aussi par la case
						 // de fermeture !!!!
  {
	 bTraceWindowVisible=FALSE;
	 DestroyWindow(hwnd);
	 return FALSE;
  }
  case WM_COMMAND:
  {  switch (LOWORD(wParam) )
		 {
		 case IDOK:
			 bTraceWindowVisible=FALSE;
			 DestroyWindow(hwnd);
		 return TRUE;
		 case IDC_ADD_LINE: // version memo
		 if (lParam)  //add a line to the memo control
		  {
			  strcpy(aux,(char *)lParam);
			  strcat(aux,CR_LF);
			  len=SendDlgItemMessage(hwnd, IDC_MEMO,WM_GETTEXTLENGTH,0,0L);
			  SendDlgItemMessage(hwnd, IDC_MEMO,EM_SETSEL,len,len);
			  SendDlgItemMessage(hwnd,IDC_MEMO,EM_REPLACESEL,0,(LPARAM) aux);
			  SendDlgItemMessage(hwnd,IDC_MEMO,EM_SETSEL,-1,0L);
		 }
		 else //empty memo
		 {    SetDlgItemText(hwnd,IDC_MEMO,"");
				Trace ("Begin of trace mode %s.","");
				if (currentPage==PAGE_USER)
				  Trace ("Current user:%s",currentObject);
				else
					Trace ("Current group:%s",currentObject);
				Trace ("Using Linux Profile Object:%s",linuxProfileObjectUsed);
		 }
		 return TRUE;
		}
	}
 }
 return FALSE;
}

/****************************** NDS CODE *********************************/

NWDSContextHandle GetMeAContext(char * caller)
{ NWDSContextHandle context;
  NWDSCCODE err;
  uint32 flags = 0;
// context = NWDSCreateContext();  now OBSOLETE (linker error)
// if (context == (NWDSContextHandle) ERR_CONTEXT_CREATION)
 err =NWDSCreateContextHandle(&context);
 if (err)
 {
  displayError( NULL, caller,"NWDSContextHandle" , 0 );
  return ERR_CONTEXT_CREATION;
 }
// move up to ROOT (all names are FQDN
 err = NWDSSetContext (context, DCK_NAME_CONTEXT, DS_ROOT_NAME);
 if (err < 0)
 {
  displayError( NULL, caller,"NWDSSetContext" , err );
failed:
	 NWDSFreeContext(context);
	 return (NWDSContextHandle) ERR_CONTEXT_CREATION;
  }
 err = NWDSGetContext (context, DCK_FLAGS,&flags);
 if (err < 0)
 {
  displayError( NULL, caller,"NWDSGetContext" , err );
  goto failed;
 }
 flags |= DCV_TYPELESS_NAMES | DCV_XLATE_STRINGS
			| DCV_DEREF_ALIASES | DCV_DEREF_BASE_CLASS;

 err = NWDSSetContext (context, DCK_FLAGS, &flags);
 if (err < 0)
 {
  displayError( NULL, caller,"NWDSSetContext" , err );
  goto failed;
 }
  return context;
}

/*-------------------------------------------------------------------------*/
/* Function : ReadAttributeDef                                             */
/*                                                                         */
/* Description :                                                           */
/*   Function for reading an attribute definition                          */
/*-------------------------------------------------------------------------*/
NWDSCCODE ReadAttributeDef (NWDSContextHandle cx, char* attrname, Attr_Info_T *attrInfo)

{ Attr_Info_T        tmpattrInfo;
  // copied back if attrInfo if <>NULL
  char me[]="PamNcp Snapin:ReadAttributeDef";
 NWDSCCODE err;
 int i ;
 int32 iterHandle = -1L;
 Buf_T *resBuf = NULL;
 Buf_T *attrBuf = NULL;
 uint32 totAttr = 0;

 Trace("%s: begin of action.",me);
 err = NWDSAllocBuf (DEFAULT_MESSAGE_LEN, &resBuf);
 if (err < 0)
 {
  displayError( NULL,me,"NWDSAllocBuffer ResBuf" , err );
  goto read_cleanup;
 }
 err = NWDSAllocBuf (DEFAULT_MESSAGE_LEN, &attrBuf);
 if (err < 0)
 {
  displayError( NULL,me,"NWDSSetContext AttrBuf" , err );
  goto read_cleanup;
 }
 Trace ("%s: result buffer OK.",me);
 err = NWDSInitBuf (cx, DSV_READ_ATTR_DEF, attrBuf);
 if (err < 0)
 {
  displayError( NULL,me,"NWDSInitBuff" , err );
  goto read_cleanup;
 }
  Trace ("%s: attribute buffer OK.",me);
 err = NWDSPutAttrName (cx, attrBuf, attrname);
 if (err < 0)
 {
  displayError( NULL,me,"NWDSPutAttrName " , err );
  goto read_cleanup;
 }else  Trace ("%s: putting attribute name%s.",me,attrname);
 iterHandle = NO_MORE_ITERATIONS;
 do
 {
  err = NWDSReadAttrDef(cx,
				DS_ATTR_DEFS,    /* infoType, attribute definitions    */
				FALSE,           /* allAttrs = false, passing in one   */
				attrBuf,          /* through this buffer               */
				&iterHandle,
				resBuf);
  if (err < 0)
  {
	if (err == -603)
	 { Trace ("%s:%s is not in the schema. too bad...",me,attrname);
	  goto read_cleanup;
	 }
	displayError( NULL,me,"NWDSReadAttrDef" , err );
	goto read_cleanup;
  }
  err = NWDSGetAttrCount (cx, resBuf, &totAttr);
  if (err < 0)
  {
	displayError( NULL,me,"NWDSGetAttrCount" , err );
	goto read_cleanup;
  }else  Trace ("%s: reading %d attributes .",me,totAttr); // v 1.10

  for (i = 0; i < totAttr; i++)
  {
	err =NWDSGetAttrDef(cx, resBuf, attrname, &tmpattrInfo);

	if (err < 0)
	{
	 displayError( NULL,me,"NWDSGetAttrDef" , err );
	 goto read_cleanup;
	}
  }
 } while (iterHandle != NO_MORE_ITERATIONS);
 Trace ("%s:%s found in the schema. Unix attributes may be used...",me,attrname);

read_cleanup:
 if (resBuf != NULL) NWDSFreeBuf (resBuf);
 if (attrBuf != NULL) NWDSFreeBuf (attrBuf);
 Trace ("%s: end of action. Status :%d",me, err);

 if (!err &&attrInfo) memcpy (attrInfo,&tmpattrInfo,sizeof(attrInfo));
 return err;
}

/*-------------------------------------------------------------------------*/
/* Function : ReadAttributesValues                                         */
/*                                                                         */
/* Description :                                                           */
/*   Function for reading an set of attributes values                      */
/*    same as the one used in pam_ncp_auth.c                               */
/*-------------------------------------------------------------------------*/

// element of the table of attributes to read
struct attrop {
	const pnstr8 attrname;
	NWDSCCODE (*getval)(NWDSContextHandle, const void* val, void* arg);
	enum SYNTAX synt;
};


static NWDSCCODE ReadAttributesValues(NWDSContextHandle ctx,
			 char* objname,
			 void* arg,
			 struct attrop* atlist,
			 int complainsIfObjectNotFound) {
// added to use it to search for an object with some attributes
// and not complaining if not found !
// cf ReadLoginScript

 char me[]="PamNcp Snapin:ReadAttributesValues";
	Buf_T* inBuf=NULL;
	Buf_T* outBuf=NULL;
	NWDSCCODE dserr;
	struct attrop* ptr;
	nint32 iterHandle;

	Trace("%s: begin of action.",me);

	iterHandle = NO_MORE_ITERATIONS; // test at bailout point !!!

	dserr = NWDSAllocBuf(DEFAULT_MESSAGE_LEN, &inBuf);
	if (dserr) {
		displayError( NULL,me,"NWDSAllocBuf failed for input buffer" , dserr );
		goto bailout;
	}
	dserr = NWDSInitBuf(ctx, DSV_READ, inBuf);
	if (dserr) {
		displayError( NULL,me,"NWDSInitBuf failed for input buffer" , dserr );
		goto bailout;
	}
	for (ptr = atlist; ptr->attrname; ptr++) {
		dserr = NWDSPutAttrName(ctx, inBuf, ptr->attrname);
		if (dserr) {
			displayError( NULL,me,"NWDSPutAttrName  failed for input buffer" , dserr );
			goto bailout;
		}
	}
	dserr = NWDSAllocBuf(DEFAULT_MESSAGE_LEN, &outBuf);
	if (dserr) {
		displayError( NULL,me,"NWDSAllocBuf failed for output buffer" , dserr );
		goto bailout;
	}
	do {
		nuint32 attrs;
		dserr = NWDSRead(ctx, objname, DS_ATTRIBUTE_VALUES, 0, inBuf, &iterHandle, outBuf);
		if (dserr) {
			if (dserr == ERR_NO_SUCH_ATTRIBUTE)
				dserr = 0; // OK these attributes are not mandatory
			else {
				if ((dserr != ERR_NO_SUCH_ENTRY) || complainsIfObjectNotFound){
					// keep errCode but NO complains
				displayError( NULL,me,"NWDSRead() failed " , dserr );
				}
			 }
			goto bailout;
		}
		dserr = NWDSGetAttrCount(ctx, outBuf, &attrs);
		if (dserr) {
			displayError( NULL,me,"NWDSGetAttrCount() failed " , dserr );
			goto bailout;
		}
		while (attrs--) {
			char attrname[MAX_SCHEMA_NAME_CHARS+1];
			nuint32 synt;
			nuint32 vals;

			dserr = NWDSGetAttrName(ctx, outBuf, attrname, &vals, &synt);
			if (dserr) {
				displayError( NULL,me,"NWDSGetAttrName() failed " , dserr );
				goto bailout;
			}
			while (vals--) {
				nuint32 sz;
				void* val;

				dserr = NWDSComputeAttrValSize(ctx, outBuf, synt, &sz);
				if (dserr) {
					displayError( NULL,me,"NWDSComputeAttrValSize() failed " , dserr );
					goto bailout;
				}
				val = malloc(sz);
				if (!val) {
					displayError( NULL,me,"malloc failed " , -1 );
					goto bailout;
				}
				dserr = NWDSGetAttrVal(ctx, outBuf, synt, val);
				if (dserr) {
					free(val);
					displayError( NULL,me,"NWDSGetAttrVal() failed " , dserr );
					goto bailout;
				}
				for (ptr = atlist; ptr->attrname; ptr++) {
					if (!strcmp(ptr->attrname, attrname))
						break;
				}
				if (ptr->getval) {
					if (ptr->synt != synt) {
						char aux [256];
						sprintf(aux,"Incompatible tree schema, %s has syntax %d instead of %d\n",attrname, synt, ptr->synt);
						displayError( NULL,me,aux , -1 );
					} else {
						dserr= ptr->getval(ctx, val, arg);
					}
				}
				free(val);
				if (dserr) {
					goto bailout;
				}
			}
		}
	} while (iterHandle != NO_MORE_ITERATIONS);
bailout:;
	if (iterHandle != NO_MORE_ITERATIONS) {
		  // PP let's keep the final dserr as the 'out of memory error' from ptr->getval()
		NWDSCCODE dserr2 = NWDSCloseIteration(ctx, DSV_READ, iterHandle);
		if (dserr2) {
			displayError( NULL,me,"NWDSCloseIteration() failed " , dserr2 );
		}
	}
	if (inBuf) NWDSFreeBuf(inBuf);
	if (outBuf) NWDSFreeBuf(outBuf);
	return dserr;
}


/************************************ helper functions to extract some user's properties **/
static NWDSCCODE nds_user_unixuid(NWDSContextHandle ctx,const void* val, void* arg) {
	(void) ctx; // warnings out !
	(void) arg; // warnings out !
// despite the fact that UNIX:UID is in buffer before L,
// it looks like the L values comes out BEFORE !!!!
//	if (currentUID ==-1)	currentUID = *(const Integer_T*)val;
	currentUID = *(const Integer_T*)val; // DS8 has priority
	Trace ("got %s=%d",ATTR_UID,currentUID);
	currentReadFLAG |=HAD_UID8;
	return 0;
}

static NWDSCCODE nds_user_unixpgid(NWDSContextHandle ctx, const void* val, void* arg) {
	(void) ctx; // warnings out !
	(void) arg; // warnings out !

//	if (currentGID ==-1)	currentGID = *(const Integer_T*)val;
	currentGID = *(const Integer_T*)val; // DS8 has priority
	Trace ("got %s=%d",ATTR_GID,currentGID);
	currentReadFLAG |=HAD_PGID8;
	return 0;
}

static NWDSCCODE nds_user_unixgid(NWDSContextHandle ctx,const void* val, void* arg) {
	(void) ctx; // warnings out !
	(void) arg; // warnings out !
//	if (currentGID ==-1)	currentGID = *(const Integer_T*)val;
	currentGID = *(const Integer_T*)val;// DS8 has priority
	currentReadFLAG |=HAD_GID8;
	Trace ("got %s=%d",ATTR_PGID,currentGID);
	return 0;
}

static NWDSCCODE nds_user_unixhome(NWDSContextHandle ctx,const void* val, void* arg) {
	(void) ctx; // warnings out !
	(void) arg; // warnings out !
	if (!strlen(currentHOME)) strcpy(currentHOME,(char*) val);
	Trace ("got %s=%s",ATTR_HOME,currentHOME);
	currentReadFLAG |=HAD_HOME8;
	return 0;
}

static NWDSCCODE nds_user_unixshell(NWDSContextHandle ctx, const void* val, void* arg) {
	(void) ctx; // warnings out !
	(void) arg; // warnings out !
	//if (!strlen(currentSHELL)) strcpy(currentSHELL,(char*) val);
	strcpy(currentSHELL,(char*) val); // DS8 has priority
	Trace ("got %s=%s",ATTR_SHELL,currentSHELL);
	currentReadFLAG |=HAD_SHELL8;
	return 0;}


static NWDSCCODE nds_user_unixcomment(NWDSContextHandle ctx,const void* val, void* arg) {
	(void) ctx; // warnings out !
	(void) arg; // warnings out !
  //	if (!strlen(currentCOMMENT)) strcpy(currentCOMMENT,(char*) val);
	strcpy(currentCOMMENT,(char*) val);// DS8 has priority
	Trace ("got %s=%s",ATTR_COM,currentCOMMENT); // not editable here
	currentReadFLAG |=HAD_COM8;
	return 0;
}


static NWDSCCODE nds_user_unixpgname(NWDSContextHandle ctx,const void* val, void* arg) {
 (void) ctx; // warnings out !
 (void) arg; // warnings out !
  //if (!strlen(currentPGNAME)) strcpy(currentPGNAME,(char*) val);
	strcpy(currentPGNAME,(char*) val);// DS8 has priority
	Trace ("got %s=%s",ATTR_PGID,currentPGNAME); // not yet editable here
	currentReadFLAG |=HAD_PGNAME8;
	return 0;
}


// PP: id no NDS8 is present, collect the user's basic  Unix informations from  the location
// strings with the format X:nnnnnnnn  , X = [U,G,H,S,P,O,C,Z] upper of lower case
// This is relevant ONLY if the flag QFC_REQUIRE_SERVER has been set in the command line
// Of course, even if NDS8 IS present, we still look at these, just in case the migration
// is not complete and to look for the user's ZENFLAG

// feature I: since zenflags are ORED, a user can have SEVERAL z:xxxx strings in his location strings
// better than deleting and recreating it !!!
// feature II: several O:groups strings are possible ( secondary groups defintion)
// DO NOT use o:group1,group2.....
static NWDSCCODE nds_user_location(NWDSContextHandle ctx, const void* val, void* arg) {
		  char *pt= (char*) val;
		  char *v;
		  int n;

		(void) ctx ; // warnings out !
		(void) arg; // warnings out !
		 currentReadFLAG |=HAD_LOCATION; // no need to create this attribute later
		 if (strlen(pt)>2 && pt[1]==':' &&nInitValues <MAX_STRINGS) {
			 char* cur_pt=pt+2;
			 Trace ("got %s=%s",ATTR_LOCATION,pt);
			 switch (*pt) {
			 case 'u':  //user ID leading spaces not significant
			 case 'U':
				 if (currentUID ==-1) {
					 n = strtoul(cur_pt, &v, 0);
					 if (n) currentUID=n;
				 }
				 break;
			 case 'g': // primary group number GID leading spaces not significant
			 case 'G':
				 if (currentGID ==-1) {
					 n = strtoul(cur_pt, &v, 0);
					 if (n) currentGID=n;
				 }
				 break;
			 case 'p':
			 case 'P':
					 return 0;  // don't touch, don't store, don't delete !
			 case 'h': // home Unix all spaces significant (must have none ?)
			 case 'H':
					  if (!strlen(currentHOME))
						  strcpy(currentHOME,cur_pt);
					 break;
			 case 's': //shell Unix all spaces significant (must have none ?)
			 case 'S':
					  if (!strlen(currentSHELL))
						  strcpy(currentSHELL,cur_pt);
					 break;
			 case 'c':
			 case 'C':
						  return 0;  // don't touch, don't store, don't delete !

			 case 'o':  // other group names
			 case 'O':  // wze can have several entries o:group (but not o:group1,group2...)
				return 0;  // don't touch, don't store, don't delete !

			 case 'z':  // ZenFlag per user ABCDEFGHIJKLMNOPQRSTUVWXYZ012345 (32 max)
			 case 'Z':
				mergeZen (currentZEN,cur_pt);
				break;
			 default: return 0; // just ignore it
			 }
			 strcpy(initValues[nInitValues++],(char *) val);
			 // keep it to delete from NDS before putting changes
		 }
		 return 0;
}

// PP: id no NDS8 is present, collect the group Unix ID from one of the location
// string with the format G:nnn
// This is relevant ONLY if the flag QFC_REQUIRE_SERVER has been set in the command line
// can also used to specify a name of unix group different of the NDS'one
// eg. everyone --> users
// staff --> root
// arg=NULL reading global group
// arg !=NULL reading a user's group for unherited Zen (do not update flags !!)
static NWDSCCODE nds_group_location(NWDSContextHandle ctx, const void* val, void* arg) {
	char *pt= (char*) val;
	char *v;
	int n;

  (void) ctx; // warnings out !
	if (arg)  { // just searching for a Zenux Flag to merge
		if (strlen(pt)>2 && pt[1]==':')  {
			char* cur_pt=pt+2;
			 switch (*pt) {
				 case 'z':
				 case 'Z':mergeZen ((char *) arg, cur_pt);  // searching for zen of groups
				 break;
			 }
		}
		return 0;
	}else {
		currentReadFLAG |=HAD_LOCATION; // no need to create this attribute later
		if (strlen(pt)>2 && pt[1]==':' &&nInitValues <MAX_STRINGS) {
			char* cur_pt=pt+2;
			Trace ("got %s=%s",ATTR_LOCATION,pt);
			switch (*pt) {
				case 'g':
			 case 'G':
				 if (currentGID ==-1) {
					 n = strtoul(cur_pt, &v, 0);
					 if (n) currentGID=n;
				 }
					  break;
			 case 'n':
			 case 'N':
				  if (!strlen(currentPGNAME))
						  strcpy(currentPGNAME,cur_pt);
					  break;
			 /* interesting feature: since we OR them, a group may have
				 several Z:xxxx strings in the location property */
			 case 'z':
			 case 'Z':
			  mergeZen (currentZEN,cur_pt); // global
				break;
			default: return 0;
			 }
			  strcpy(initValues[nInitValues++],(char *) val);
			 // keep it to delete from NDS before putting changes
		 }
	 }
	 return 0;
}


NWDSCCODE ReadObjectData (NWDSContextHandle cx, char *objectName){
// despite the fact that UNIX:UID is in buffer before L,
// it looks like the L values comes out BEFORE !!!!
// so we will overwrite any received values in all but the last helper functions
	static struct attrop atlistuser[] = {
		{ ATTR_UID,			         nds_user_unixuid,		SYN_UID },
		{ ATTR_PGNAME,					nds_user_unixpgname,	SYN_PGNAME },
		{ ATTR_PGID,			 	   nds_user_unixpgid,	SYN_PGID },
		{ ATTR_GID,			         nds_user_unixgid,		SYN_GID },
		{ ATTR_SHELL,				   nds_user_unixshell,	SYN_SHELL },
		{ ATTR_COM,				      nds_user_unixcomment,SYN_COM },
		{ ATTR_HOME,			      nds_user_unixhome,   SYN_HOME },
		{ ATTR_LOCATION,           nds_user_location,   SYN_LOCATION},
		{ NULL,				            NULL,			         SYN_UNKNOWN }};

  static struct attrop atlistgrp[] = {
		{ ATTR_GID,	 	 	         nds_user_unixgid,		SYN_GID },
		{ ATTR_LOCATION,           nds_group_location,   SYN_LOCATION},
		{ NULL,			            NULL,			         SYN_UNKNOWN }};

  if (currentPage==PAGE_USER)
	 return ReadAttributesValues(cx,objectName,NULL,atlistuser,1);
  else
	 return ReadAttributesValues(cx, objectName,NULL, atlistgrp,1);
}


NWDSCCODE nds_group_membership(NWDSContextHandle cx,const void* val,void* arg) {

	static  struct attrop atlistgrp[] = {
		{ ATTR_LOCATION,          nds_group_location,     SYN_CI_STRING},
		{ NULL,				        NULL,			         SYN_UNKNOWN }};

	(void) arg; // warnings out !
	// non null argument, with only get then Zenux flags
	return ReadAttributesValues(cx, (char *)val, unheritedZEN, atlistgrp,1);
}


NWDSCCODE ReadUnheritedZenFlags (char *objectName){

  static  struct attrop atlistgrp[] = {
		{ ATTR_GRP_MBS,         nds_group_membership,	 SYN_GRP_MBS},
		{ NULL,				      NULL,			             SYN_UNKNOWN }};

 if (strlen(unheritedZEN)) return 0; //already got one
 // I don't have anymore a context
 MyContext=GetMeAContext("PamNcp Snapin:ReadUnheritedZenFlags:");
 if (MyContext !=(NWDSContextHandle) ERR_CONTEXT_CREATION)
  {
	 // non null argument, with only get then Zenux flags
	 ReadAttributesValues(MyContext, objectName, unheritedZEN, atlistgrp,1);
	 NWDSFreeContext (MyContext);
  }
 return 0;
}


/*** login script reading & writing ****************************************/

struct loginScriptInfo {
	char * user;
	char * buffer;
	int max;
};


static NWDSCCODE nds_read_loginscript(NWDSContextHandle ctx,const void* val, void* arg) {

static char attrName[MAX_DN_CHARS + 1]=ATTR_LOGIN_SCRIPT;
struct loginScriptInfo* lsi=arg;
NWFILE_HANDLE  FileHandle;
nuint32 bytesRead;
NWDSCCODE err ;

	 (void) val; // warnings out !
	 // 1 = for READING
	 err= NWDSOpenStream(ctx, lsi->user,attrName, 1, &FileHandle);
	 if(err==0){
		 //NWReadFile(FileHandle, lsi->max-1,&bytesRead,(pnuint8)lsi->buffer);
		 //NWReadFile not anymore in calwin32 ???
		 bytesRead=_lread((HFILE)FileHandle,lsi->buffer,lsi->max-1);
		 lsi->buffer[bytesRead]='\0';
		 NWCloseFile(FileHandle);
		 Trace("%s:%d/%d bytes red from login script.", lsi->user, bytesRead,lsi->max);
	  }else
	  displayError( NULL,"","NWDSOpenStream" , err );
	  return err;
}

/*-------------------------------------------------------------------------*/
/* Function : ReadLoginScript                                              */
/*                                                                         */
/* Description :                                                           */
/*   Function for reading object data in DS                                */
/* note the trace instructions are never executed since                    */
/*	we find the Mydebug flag in this process !!!                            */
/*-------------------------------------------------------------------------*/
NWDSCCODE ReadLoginScript ( NWDSContextHandle ctx,char * objectName, char * buffer, int max)
{
  static  struct attrop atlist[] = {
		{ ATTR_LOGIN_SCRIPT,        nds_read_loginscript,	SYN_LOGIN_SCRIPT},
		{ NULL,				          NULL,			         SYN_UNKNOWN }};

  struct loginScriptInfo lsi;

  lsi.user=objectName;
  lsi.buffer=buffer;
  lsi.max=max;
  //0= no complains  if it does not exist
  return ReadAttributesValues(ctx,objectName,&lsi,atlist,0);
}

static NWDSCCODE nds_write_loginscript(NWDSContextHandle ctx,const void* val, void* arg) {

static char attrName[MAX_DN_CHARS + 1]=ATTR_LOGIN_SCRIPT;
struct loginScriptInfo* lsi=arg;
NWFILE_HANDLE  FileHandle;
nuint32 bytesWritten;
NWDSCCODE err ;

	 (void) val; // warnings out !
	 // 1 = for READING 2 for WRITING
	 err= NWDSOpenStream(ctx, lsi->user,attrName, 2, &FileHandle);
	 if(err==0){
		 bytesWritten=_lwrite((HFILE)FileHandle,lsi->buffer,lsi->max);
		 NWCloseFile(FileHandle);
		 Trace("%s:%d/%d bytes written to login script.", lsi->user, bytesWritten,lsi->max);
	  }else
	  displayError( NULL,"","NWDSOpenStream" , err );
	  return err;
}


/*-------------------------------------------------------------------------*/
/* Function : WriteLoginScript                                              */
/*                                                                         */
/* Description :                                                           */
/*-------------------------------------------------------------------------*/
NWDSCCODE WriteLoginScript ( NWDSContextHandle ctx,char * objectName, char * buffer, int max)
{
  static  struct attrop atlist[] = {
		{ ATTR_LOGIN_SCRIPT,        nds_write_loginscript,	SYN_LOGIN_SCRIPT},
		{ NULL,				          NULL,			         SYN_UNKNOWN }};

  struct loginScriptInfo lsi;

  lsi.user=objectName;
  lsi.buffer=buffer;
  lsi.max=max;
  // no complains  if it does not exist
  // the way I call it, it does exist !
  return ReadAttributesValues(ctx,objectName,&lsi,atlist,0);
}


/*-------------------------------------------------------------------------*/
/* Function : ModifyObjectData                                             */
/*                                                                         */
/* Description :                                                           */
/*  Modifys a User:Group NDS Locations infos                              */
/* version that remove all previous strings and reinsert the new ones     */
/*-------------------------------------------------------------------------*/
NWDSCCODE ModifyObjectDataDS7 ( NWDSContextHandle cx ,char *objectName ){
 char me[]="PamNcp Snapin:ModifyObjectDataDS7";
 NWDSCCODE err ;
 int j;
 Buf_T *inpBuf = NULL;

 Trace("%s: begin of process: saving to NDS.",me);
 err = NWDSAllocBuf (DEFAULT_MESSAGE_LEN, &inpBuf);
 if (err < 0){
	 displayError( NULL,me,"NWDSAllocBuffer()" , err );
	 goto modify_cleanup;
 }
 err = NWDSInitBuf (cx, DSV_MODIFY_ENTRY, inpBuf);
 if (err < 0){
	 displayError( NULL,me,"NWDSInitBuf()" , err );
	 goto modify_cleanup;
 }
 if (nInitValues){
	 Trace("%s:removing %d previous values.",me,nInitValues);
	for (j = 0; j < nInitValues; j++){
#if 1
		err = NWDSPutChange (cx, inpBuf, DS_REMOVE_VALUE, ATTR_LOCATION);
		if (err < 0){
			  displayError( NULL,me,"NWDSPutChange(DS_REMOVE_VALUE)" , err );
			  goto modify_cleanup;
		}
		err = NWDSPutAttrVal (cx,inpBuf,SYN_LOCATION,initValues[j]);
#else
// this has no effect and no error ???
		err = NWDSPutChangeAndVal (cx, inpBuf, DS_REMOVE_VALUE, ATTR_LOCATION,
											SYN_LOCATION,initValues[j]);
#endif
		if (err < 0){
			  displayError( NULL,me,"NWDSPutAttrVal" , err );
			  goto modify_cleanup;
		} else
			Trace ("%s: removing %s.",me,initValues[j]);
	 } //for
  } // if
 else {  // may be L  Attribute does not exist yet we must create it
			// is not mandatory
	 if (!(currentReadFLAG & HAD_LOCATION)){
			Trace("%s:No Values found Creating Attribute Location ",me);
			err = NWDSPutChange (cx, inpBuf, DS_ADD_ATTRIBUTE, ATTR_LOCATION);
			if (err < 0){
				displayError( NULL,me,"NWDSPutChange(DS_ADD_ATTRIBUTE)" , err );
				goto modify_cleanup;
			}
	 }
  }
  // at last the real thing
	for (j=0; j< nNewValues; j++){
		Trace("%s:sending %s.",me,newValues[j]);
#if 1
		err = NWDSPutChange (cx, inpBuf, DS_ADD_VALUE, ATTR_LOCATION);
		if (err < 0){
			displayError( NULL,me,"NWDSPutChange(DS_ADD_VALUE)" , err );
			goto modify_cleanup;
		 }
		err = NWDSPutAttrVal (cx,inpBuf,SYN_LOCATION,newValues[j]);
#else
// this has no effect and give no error (strings stays unaffected)
		err = NWDSPutChangeAndVal (cx, inpBuf, DS_ADD_VALUE, ATTR_LOCATION,
											SYN_LOCATION,initValues[j]);

#endif
		if (err < 0){
		  displayError( NULL,me,"NWDSPutAttrVal" , err );
		  goto modify_cleanup;
		}
	}// for j

 err = NWDSModifyObject (cx, objectName, NULL, 0, inpBuf);
 if (err <0) displayError( NULL,me,"NWDSModifyObject" , err );
 else  Trace ("%s:Modifications sent OK.",me );

modify_cleanup:
 if (inpBuf != NULL) NWDSFreeBuf (inpBuf);
 Trace ("%s: end of process. Status: %d.",me,err);
 return err;
}

/*-------------------------------------------------------------------------*/
/* Function : ModifyObjectData                                             */
/*                                                                         */
/* Description :                                                           */
/*  Modifys a User:Group NDS8 specific attributes
 we assume the following from NDSK DOC
 DS_OVERWRITE_VALUE:Modifies an attribute value without needing to remove
the old value first and then add the new value.
-If the attribute is single-valued, deletes the old value,
and adds the new value. If the old and new value are
the same, NDS updates the value’s timestamp but
does not return an error.
-If the attribute is multivalued, adds the new value,
leaving the old values. If the new value already
exists, NDS updates the value’s timestamp but does
not return an error.
This flag can be use to add the first value to an attribute
(single valued or multivalued). <--------- !!!!

DS_CLEAR_VALUE:Clears an attribute value. If the value does not exists,
does not report an error.
----------------------------------------------------------------------------*/



NWDSCCODE ModifyObjectDataDS8 ( NWDSContextHandle cx ,char *objectName , int page){
 char me[]="PamNcp Snapin:ModifyObjectDataDS8";
 NWDSCCODE err ;
 Buf_T *inpBuf = NULL;

#if 0
 Trace ("new values sent to NDS8 %s","");
 if (newUID==-1 && newUID==currentUID)
	 Trace ("UID unchanged %d",currentUID);
 else
	Trace ("UID %d changed to %d",currentUID,newUID);
 if (newGID==-1 && newGID==currentGID)
	 Trace ("GID unchanged %d",currentGID);
 else
	Trace ("GID %d changed to %d",currentGID,newGID);
 if (strcmp(currentSHELL,newSHELL))
	Trace ("shell %s changed to %s",currentSHELL,newSHELL);
 else
	Trace ("shell unchanged %s",currentSHELL);
 if (strcmp(currentHOME,newHOME))
	Trace ("home %s changed to %s",currentHOME,newHOME);
 else
	Trace ("home unchanged %s",currentHOME);
 return 0;
#endif

 Trace("%s: begin of process: saving to NDS8.",me);
 err = NWDSAllocBuf (DEFAULT_MESSAGE_LEN, &inpBuf);
 if (err < 0){
	 displayError( NULL,me,"NWDSAllocBuffer()" , err );
	 goto modify_cleanup;
 }
 err = NWDSInitBuf (cx, DSV_MODIFY_ENTRY, inpBuf);
 if (err < 0){
	 displayError( NULL,me,"NWDSInitBuf()" , err );
	 goto modify_cleanup;
 }

 if (newGID==-1) {
	 err=NWDSPutChange(cx,inpBuf,DS_CLEAR_ATTRIBUTE,ATTR_GID);
	 if (err < 0){
		 displayError( NULL,me,"NWDSPutChange(DS_CLEAR_ATTRIBUTE UNIX:GID)" , err );
		 goto modify_cleanup;
	 }
 }
 else {
	err=NWDSPutChange(cx,inpBuf,DS_OVERWRITE_VALUE,ATTR_GID);
	if (err < 0){
		 displayError( NULL,me,"NWDSPutChange(DS_OVERWRITE_ATTRIBUTE)" , err );
		 goto modify_cleanup;
	}
	err=NWDSPutAttrVal(cx,inpBuf,SYN_GID,&newGID);
	if (err < 0){
		 displayError( NULL,me,"NWDSPutAttrVal(UNIX:GID)" , err );
		 goto modify_cleanup;
	}
 }
 if (page==PAGE_USER) {
	 if (newUID==-1) {
		 err=NWDSPutChange(cx,inpBuf,DS_CLEAR_ATTRIBUTE,ATTR_UID);
		 if (err < 0){
			 displayError( NULL,me,"NWDSPutChange(DS_CLEAR_ATTRIBUTE UNIX:UID)" , err );
			 goto modify_cleanup;
		 }
	  }else {
		 err=NWDSPutChange(cx,inpBuf,DS_OVERWRITE_VALUE,ATTR_UID);
		if (err < 0){
			 displayError( NULL,me,"NWDSPutChange(DS_OVERWRITE_ATTRIBUTE)" , err );
			 goto modify_cleanup;
		}
		err=NWDSPutAttrVal(cx,inpBuf,SYN_UID,&newUID);
		if (err < 0){
			 displayError( NULL,me,"NWDSPutAttrVal(UNIX:UID)" , err );
			 goto modify_cleanup;
		}
	 }
	 if (!newSHELL[0]) {
		 err=NWDSPutChange(cx,inpBuf,DS_CLEAR_ATTRIBUTE,ATTR_SHELL);
		 if (err < 0){
			 displayError( NULL,me,"NWDSPutChange(DS_CLEAR_ATTRIBUTE UNIX:SHELL)" , err );
			 goto modify_cleanup;
		 }
	 }else {
		 err=NWDSPutChange(cx,inpBuf,DS_OVERWRITE_VALUE,ATTR_SHELL);
		 if (err < 0){
			 displayError( NULL,me,"NWDSPutChange(DS_OVERWRITE_ATTRIBUTE)" , err );
			 goto modify_cleanup;
		 }
		 err=NWDSPutAttrVal(cx,inpBuf,SYN_SHELL,&newSHELL);
		 if (err < 0){
			 displayError( NULL,me,"NWDSPutAttrVal(UNIX:SHELL)" , err );
			 goto modify_cleanup;
		 }
	  }

	  if (!newHOME[0]) {
		  err=NWDSPutChange(cx,inpBuf,DS_CLEAR_ATTRIBUTE,ATTR_HOME);
		  if (err < 0){
			  displayError( NULL,me,"NWDSPutChange(DS_CLEAR_ATTRIBUTE UNIX:HOME)" , err );
			  goto modify_cleanup;
		  }
	  } else {
		  err=NWDSPutChange(cx,inpBuf,DS_OVERWRITE_VALUE,ATTR_HOME);
		  if (err < 0){
			  displayError( NULL,me,"NWDSPutChange(DS_OVERWRITE_ATTRIBUTE)" , err );
			  goto modify_cleanup;
		  }
		  err=NWDSPutAttrVal(cx,inpBuf,SYN_HOME,&newHOME);
		  if (err < 0){
			  displayError( NULL,me,"NWDSPutAttrVal(UNIX:HOME)" , err );
			  goto modify_cleanup;
		  }
	  }
  }
 err = NWDSModifyObject (cx, objectName, NULL, 0, inpBuf);
 if (err <0) displayError( NULL,me,"NWDSModifyObject" , err );
 else  Trace ("%s:Modifications sent OK.",me );

modify_cleanup:
 if (inpBuf != NULL) NWDSFreeBuf (inpBuf);
 Trace ("%s: end of process. Status: %d.",me,err);
 return err;
}

struct zenElement {
  int resID;
  char letter;
};

static struct zenElement zenTable[]= {
 {IDC_ZF_A,'A'}, {IDC_ZF_B,'B'}, {IDC_ZF_C,'C'}, {IDC_ZF_D,'D'},
 {IDC_ZF_E,'E'}, {IDC_ZF_F,'F'}, {IDC_ZF_G,'G'}, {IDC_ZF_H,'H'},
 {IDC_ZF_I,'I'}, {IDC_ZF_J,'J'}, {IDC_ZF_K,'K'}, {IDC_ZF_L,'L'},
 {IDC_ZF_M,'M'}, {IDC_ZF_N,'N'}, {IDC_ZF_O,'O'}, {IDC_ZF_P,'P'},
 {IDC_ZF_Q,'Q'}, {IDC_ZF_R,'R'}, {IDC_ZF_S,'S'}, {IDC_ZF_T,'T'},
 {IDC_ZF_U,'U'}, {IDC_ZF_V,'V'}, {IDC_ZF_W,'W'}, {IDC_ZF_X,'X'},
 {IDC_ZF_Y,'Y'}, {IDC_ZF_Z,'Z'}, {IDC_ZF_0,'0'}, {IDC_ZF_1,'1'},
 {IDC_ZF_2,'2'}, {IDC_ZF_3,'3'}, {IDC_ZF_4,'4'}, {IDC_ZF_5,'5'},
 {0,0}
};


static void setZenCheckBoxes ( HWND hwnd, char * zenString) {
 struct zenElement* z;
 CharUpper (zenString);
 for (z=zenTable; z->letter; z++)
	if (IsWindowEnabled (GetDlgItem(hwnd,z->resID)))
		if (strchr(zenString,z->letter)) CheckDlgButton(hwnd,z->resID,TRUE);
		else  CheckDlgButton(hwnd,z->resID,FALSE);
}

static void getZenCheckBoxes ( HWND hwnd, char * zenString) {
 struct zenElement* z;
 int i=0;

 for (z=zenTable; z->letter; z++)
	if (IsWindowEnabled (GetDlgItem(hwnd,z->resID)))
		if (IsDlgButtonChecked(hwnd,z->resID))
		  zenString [i++]=z->letter;
 zenString[i]=0;

}

static struct zenElement zenTableBis[]= {
 {IDC_ZF_AH,'A'}, {IDC_ZF_BH,'B'}, {IDC_ZF_CH,'C'}, {IDC_ZF_DH,'D'},
 {IDC_ZF_EH,'E'}, {IDC_ZF_FH,'F'}, {IDC_ZF_GH,'G'}, {IDC_ZF_HH,'H'},
 {IDC_ZF_IH,'I'}, {IDC_ZF_JH,'J'}, {IDC_ZF_KH,'K'}, {IDC_ZF_LH,'L'},
 {IDC_ZF_MH,'M'}, {IDC_ZF_NH,'N'}, {IDC_ZF_OH,'O'}, {IDC_ZF_PH,'P'},
 {IDC_ZF_QH,'Q'}, {IDC_ZF_RH,'R'}, {IDC_ZF_SH,'S'}, {IDC_ZF_TH,'T'},
 {IDC_ZF_UH,'U'}, {IDC_ZF_VH,'V'}, {IDC_ZF_WH,'W'}, {IDC_ZF_XH,'X'},
 {IDC_ZF_YH,'Y'}, {IDC_ZF_ZH,'Z'}, {IDC_ZF_0H,'0'}, {IDC_ZF_1H,'1'},
 {IDC_ZF_2H,'2'}, {IDC_ZF_3H,'3'}, {IDC_ZF_4H,'4'}, {IDC_ZF_5H,'5'},
 {0,0}
};

static void setZenCheckBoxesBis ( HWND hwnd, char * zenString) {
 struct zenElement* z;
 CharUpper (zenString);
 for (z=zenTableBis; z->letter; z++)
		if (strchr(zenString,z->letter)) CheckDlgButton(hwnd,z->resID,TRUE);
		else  CheckDlgButton(hwnd,z->resID,FALSE);
}

static void clearZenCheckBoxesBis ( HWND hwnd) {
 struct zenElement* z;
 for (z=zenTableBis; z->letter; z++)
		CheckDlgButton(hwnd,z->resID,FALSE);
}

void showHideZenCheckBoxesBis ( HWND hwnd, int show) {
struct zenElement* z;
 for (z=zenTableBis; z->letter; z++)
		ShowWindow(GetDlgItem(hwnd,z->resID),show);

}

void mergeZen (char * z1, char *z2) {
// make sure a flag is not duplicated ( just for the string to stay small!)
  char * p;
  int tail =strlen (z1);
 if (!z2) return ;
 Trace ("merging %s + %s",z1,z2);
 for (p=z2; *p; p++)
	 if (!strchr(z1,*p)) z1[tail++]=*p;
 z1[tail]=0;
}

/******************** cfg file handling routines (case sensitive for now )****/

 char*  findString (char *buffer, char * name, char *value, char * defvalue)
{ char * tmpstr;int i,j;
 j=0;
 tmpstr=strstr(buffer,name);
 if (tmpstr){
	i=strlen(name);
	while ((tmpstr[i]!='\0')&& (tmpstr[i] !='\n')){
		if (tmpstr[i]>' ' && tmpstr[i] !='=')  // skip egal space and non-ascii
				value[j++]=tmpstr[i];
		i++;
	}
	value[j]='\0';
  } else  strcpy( value,defvalue);
 return tmpstr;  // NULL if not Found
}

void findBool (char *buffer, char * name, nbool *value, nbool defvalue){
 char aux[33];
 if (findString(buffer,name,aux,"")) *value=aux[0]=='Y';
 else *value=defvalue;
}

void findInt (char *buffer, char * name, int *value, int defvalue) {
 char aux[33];
 if (findString(buffer,name,aux,""))*value=atoi(aux);
 else *value=defvalue;
}

char* readCFGString (char *buffer, char * section,  char *item,
						  char* value, char*  defvalue) {

	char* sStart;
	char* sEnd;
	char* aux;

	sStart =strstr (buffer,section);
	if (!sStart) {
			 strcpy(value,defvalue);
			 return NULL; // not found
	}
	sStart +=strlen(section);
	sEnd = strchr(sStart,'[');
	if (sEnd) *sEnd='\0';  // stop at beginning of next section
	aux =findString(sStart,item,value,defvalue);
	if (sEnd) *sEnd='[';  // put it back
	return aux;
}


void readCFGInt (char *buffer, char * section,  char *item,
						  long *value, long defvalue) {
 char aux[33];

 if (readCFGString(buffer,section,item,aux,""))*value=atoi(aux);
 else *value=defvalue;
}

/****************************************************************************/
/*																                            */
/* Function : getNameAndContext		                                        */
/* Description :                                                            */
/*   copies name and context from a distinguished name into dest buffers	 */
/****************************************************************************/
void	getNameAndContext(char*	sourceDistName,char* nameBuffer,char* contextBuffer)
{
	int		copyIndex=0;
	char	* srcPtr;

	while( 	sourceDistName[copyIndex] != '.' &&
					sourceDistName[copyIndex] != 0 &&
					copyIndex < MAX_DN_CHARS )
	{
		if( nameBuffer )
			nameBuffer[copyIndex]=sourceDistName[copyIndex];
		copyIndex++;
	}
//	if( copyIndex < MAX_DN_CHARS && sourceDistName[copyIndex] != 0 )
// MODIF PP. le contexte PEUT devenir vide a force
	if( copyIndex < MAX_DN_CHARS)
	{
		if(contextBuffer)
		{
			if( sourceDistName[copyIndex] == '.' )
				srcPtr =  &sourceDistName[copyIndex+1];
			else
				srcPtr =  &sourceDistName[copyIndex];
			lstrcpy( contextBuffer , srcPtr );
		}
	}
	if(nameBuffer)
		nameBuffer[ copyIndex ] = 0;
}



/********************************************/

int findLinuxProfileLoginScript(NWDSContextHandle cx,char * buffer, int max)
{ char me[]= "findLinuxProfileLoginScript";
  char context[MAX_DN_CHARS];int err;
  // start at current object context
  getNameAndContext(currentObject,NULL,context);
  err=-601;
  while(err && context[0])
 {
  Trace("%s: searching Linux Profile %s.",me,context);
  strcpy(linuxProfileObjectUsed,LINUX_PROFILE);
  strcat(linuxProfileObjectUsed,".");
  strcat(linuxProfileObjectUsed,context);
  err=ReadLoginScript(cx,linuxProfileObjectUsed,buffer,max);
  if (err)
	 {
		 // move up one context
		 strcpy(linuxProfileObjectUsed,context);
		 getNameAndContext(linuxProfileObjectUsed,NULL,context);
	 }
 }
 if (err){
	linuxProfileObjectUsed[0]='\0';
	Trace ("%s: no Linux Profile found. using defaults",me);
 }
 else Trace( "%s: using Linux Profile %s",me,linuxProfileObjectUsed);
 return err==0;
}

void ReadLinuxProfileLoginScript ( NWDSContextHandle cx)
{
  char * tmpstr; char buffer[8192];char aux[MAX_DN_CHARS];

  if (!findLinuxProfileLoginScript(cx,buffer,sizeof(buffer)-1))
			buffer[0]='\0'; // empty all default values will be set

  tmpstr=buffer;
  if (currentPage==PAGE_USER) {
		readCFGInt    (tmpstr,USER_SECTION,MYMINUID,&gMinUID,DEF_MINUID);
		readCFGInt    (tmpstr,USER_SECTION,MYMAXUID,&gMaxUID,DEF_MAXUID);
		readCFGInt    (tmpstr,USER_SECTION,MYMINGID,&gMinGID,DEF_MINGID_USR);
		readCFGInt    (tmpstr,USER_SECTION,MYMAXGID,&gMaxGID,DEF_MAXGID_USR);
		readCFGInt    (tmpstr,USER_SECTION,MYDEFGID,&gDefGID,DEF_GID_USR);
		readCFGString (tmpstr,USER_SECTION,MYHOME ,gDefHome,DEF_HOME);
		readCFGString (tmpstr,USER_SECTION,MYSHELL ,gDefShell,DEF_SHELL);
		readCFGString (tmpstr,USER_SECTION,MYZEN ,gDefZen,DEF_ZEN_USR);
		getNameAndContext(currentObject,aux,NULL);
		if (aux)
			strcat (gDefHome,strlwr(aux));
  }
  else {
		readCFGInt    (tmpstr,GRP_SECTION,MYMINGID,&gMinGID,DEF_MINGID_GRP);
		readCFGInt    (tmpstr,GRP_SECTION,MYMAXGID,&gMaxGID,DEF_MAXGID_GRP);
		readCFGString (tmpstr,GRP_SECTION,MYZEN ,gDefZen,DEF_ZEN_GRP);
  }

  readCFGInt (tmpstr,COM_SECTION,MYDEBUG,&DEBUG,0);
  readCFGInt (tmpstr,COM_SECTION,MYPROMPT,&gPrompt,1);  // default =yes
  readCFGInt (tmpstr,CUR_SECTION,MYNEXTUID,&gNextUID,gMinUID);
  readCFGInt (tmpstr,CUR_SECTION,MYNEXTGID,&gNextGID,gMinGID);
}

void TraceBool  (char* keyword, nbool value){
 if (value) Trace("%s=Y",keyword); else Trace("%s=N",keyword);
}

void TraceHex (char * buffer) {
//use with care.. ( not too big buffer else GPF !!!)
char buf [8192];
char aux[4];
int i;
  buf[0]='\0';
  for (i=0;i<strlen(buffer);i++){
	 sprintf (aux,"%x|",buffer[i]);
	 strcat(buf,aux);
  }
  Trace ("%s",buf);
}

// caution the \n is already at the beginning of MYNEXTUID,MYNEXTGID
#define MODEL "%s\r\n#this section MUST be the last one. \r%s=%ld\r%s=%ld\r\n"

int  UpdateLinuxProfileLoginScript ( NWDSContextHandle cx,char * objectName,
												 long nextUID,long nextGID)
{
  char * aux;
  char buffer[8192];
  int err;
  char newSection[512]="hello";

  err=ReadLoginScript(cx,objectName,buffer,sizeof(buffer)-1);
  if (!err) {
	 aux=strstr(buffer,CUR_SECTION);
	 if (aux) *aux='\0';
	 //TraceHex (buffer);
	 sprintf(newSection,MODEL,CUR_SECTION,MYNEXTUID,nextUID,MYNEXTGID,nextGID);
	 Trace ("on ajoute %s",newSection);
	 strcat (buffer,newSection);
	 Trace("%s",buffer);
	 err=WriteLoginScript(cx,objectName,buffer,strlen(buffer));
	 /***
	 err=ReadLoginScript(cx,objectName,buffer,sizeof(buffer)-1);
	 Trace("alors %s","");
	 TraceHex (buffer);
	 ***/
  }
  return err;
}

void ShowAdminPreference(void)
{
  if (!bTraceWindowVisible) return;
  if (currentPage==PAGE_USER) {
	 Trace("%s=%d",MYMINUID,gMinUID);
	 Trace("%s=%d",MYMAXUID,gMaxUID);
	 Trace("%s=%d",MYMINGID,gMinGID);
	 Trace("%s=%d",MYMAXGID,gMaxGID);
	 Trace("%s=%s",MYSHELL,gDefShell);
	 Trace("%s=%s",MYHOME,gDefHome);
  } else {
	Trace("%s=%d",MYMINGID,gMinGID);
	Trace("%s=%d",MYMAXGID,gMaxGID);
  }
 Trace("%s=%s",MYZEN,gDefZen);
 Trace("%s=%d",MYNEXTUID,gNextUID);
 Trace("%s=%d",MYNEXTGID,gNextGID);
}



/************************************************  UTILS ******************/

void trim (char * string) // v 1.05C
{
char aux [MAX_DN_CHARS];int i,j;
 j=0;
  for (i=0;i<strlen(string); i++)
  if (string[i] !=' ') aux[j++]=string[i];
  aux[j]='\0';
  strcpy(string,aux);
}



void Trace (char * 	fmt,...)
{
 if (bTraceWindowVisible)
  {
	static char	tempstr[8192]="";
	va_list param;
	va_start(param,fmt);
	wvsprintf(tempstr,fmt,param);
	va_end(param);
	// postMessage pose un pb car lors du traitement du message
	// la chaine tempstr() peut avoir ete changée ou ne plus exister
	// si elle est locale a une fonction !!!
	SendMessage(TraceWindowhwnd,WM_COMMAND,IDC_ADD_LINE,(LPARAM)tempstr);
  }
}


void Infos (char *message)
{
 MessageBox (NULL, message, ME, MB_OK);
 Trace (message," ");
}


/****************************************************************************/
/* Function : displayError                                         			 */
/* Description :                                                            */
/*   Displays the specified error														 */
/****************************************************************************/
long displayError( 	char * 	prelude,
							char *	AppRoutineName ,
							char *	SysRoutineName ,
							long 		errVal )
{
	char	tempstr[512]="";
	char	numstr[64]="";
	int		len,i;

	if (prelude != NULL)
	{
		lstrcpy(tempstr, prelude);
		lstrcat(tempstr, "\n");
	}
	// v 1.05e
	Trace ("%s: Fatal error :%s . Code : %d/%X",AppRoutineName,
					SysRoutineName,errVal,errVal);

	lstrcat(tempstr, "(In client app routine ");
	lstrcat(tempstr,AppRoutineName);
	lstrcat(tempstr,", got an error from ");
	lstrcat(tempstr,SysRoutineName);
	lstrcat(tempstr," = Dec ");
	itoa   ((int)errVal,numstr, 10   );
	lstrcat(tempstr,numstr);
	lstrcat(tempstr,"/Hex ");
	itoa   (errVal,numstr, 16   );
	len = strlen(numstr);
	for (i=0; i<len; i++)
		numstr[i] = (char)toupper(numstr[i]);
	lstrcat(tempstr,numstr);
	lstrcat(tempstr, ")");
	MessageBox(NULL, tempstr, ME, MB_OK);
	return (errVal);
}


/****************************************************************************/
/* Function : displayMessage                                         		 */
/* Description :                                                            */
/*   Displays the specified message														 */
/****************************************************************************/
void displayMessage( char *AppRoutineName ,
							char *Message )
{
	char	tempstr[512];

	Trace ("%s:%s ",AppRoutineName,Message); // v 1.05e
	lstrcpy(tempstr,AppRoutineName);
	lstrcat(tempstr,"\n");
	lstrcat(tempstr,Message);
	MessageBox(NULL, tempstr, ME, MB_OK);

}







