#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <descrip.h>
#include <starlet.h>
#include <ssdef.h>
#include <stsdef.h>
#include <lnmdef.h>
#include <libdef.h>
#include <libfisdef.h>
#include <lib$routines>
#include <psldef.h>
#include <rmsdef.h>
#include <fabdef.h>
#include <namdef.h>
#include <unixlib.h>

static int	error_status = SS$_NORMAL;
static char	error_buffer[256];
static char	getenv_buffer[256];

typedef struct 
{
    struct dsc$descriptor_s	fnmdes;
    struct dsc$descriptor_s	imgdes;
    struct dsc$descriptor_s	symdes;

    char			filename[NAM$C_MAXRSS];
} vms_dl;


int vms_dlsym	    (vms_dl	*, void	**, int);
void * lt_dlsym	    (void *, const char *);

int lt_dlinit (void)
{
    return 0;
}

char translate_buffer[NAM$C_MAXRSS+1];

int to_vms_callback(char *name, int type)
{
	strcpy(translate_buffer, name);
	return 1;
}

void * lt_dlopen (const char *filename)
{

   /* 
    * Locates and validates a shareable image.  If the caller supplies a 
    * path as part of the image name, that's where we look.  Note that this
    * includes cases where the image name is a logical name pointing to a full
    * file spec including path.  If there is no path,  we look at wherever the
    * logical name LTDL_LIBRARY_PATH points (if it exists); it'd normally be a
    * search list logical defined in lt_dladdsearchdir, so it could point to a
    * number of places.   As a last resort we look in SYS$SHARE. 
    */

    vms_dl	*dh;
    int		status;  
    struct FAB	imgfab;  
    struct NAM  imgnam;
    char defimg[NAM$C_MAXRSS+1];
    char *defpath;
	char local_fspec[NAM$C_MAXRSS+1];

    if (filename == NULL) 
    {
	error_status = SS$_UNSUPPORTED;
	return NULL;
    }

	strcpy(local_fspec, filename);

    /* 
     * The driver manager will handle a hard-coded path from a .ini file, but
     * only if it's an absolute path in unix syntax.  To make those acceptable
     * to lib$find_image_symbol, we have to convert to VMS syntax.  N.B. Because
     * of the static buffer, this is not thread-safe, but copying immediately to
     * local storage should limit the damage.
     */
	
    if (filename[0] == '/') {
        int num_translated = decc$to_vms(local_fspec, to_vms_callback, 0, 1);
        if (num_translated == 1) strcpy(local_fspec, translate_buffer);
    }

    dh = (vms_dl *)malloc (sizeof (vms_dl));  
    memset( dh, 0, sizeof(vms_dl) );
    if (dh == NULL) 
    {
	error_status = SS$_INSFMEM;
	return NULL;
    }

    imgfab = cc$rms_fab;
	imgfab.fab$l_fna = local_fspec;
	imgfab.fab$b_fns = (int) strlen (local_fspec);
    imgfab.fab$w_ifi = 0;  

   /* If the logical name LTDL_LIBRARY_PATH does not exist, we'll depend
    * on the image name being a logical name or on the image residing in
    * SYS$SHARE.
    */
    if ( getvmsenv("LTDL_LIBRARY_PATH") == NULL )
    {
        strcpy( defimg, ".EXE" );
    }
    else 
    {
        strcpy( defimg, "LTDL_LIBRARY_PATH:.EXE" );
    }
    imgfab.fab$l_dna = defimg;
    imgfab.fab$b_dns = strlen(defimg);
    imgfab.fab$l_fop = FAB$M_NAM;
    imgfab.fab$l_nam = &imgnam;  
    imgnam = cc$rms_nam;
    imgnam.nam$l_esa = dh->filename;  
    imgnam.nam$b_ess = NAM$C_MAXRSS;
  
    status = sys$parse (&imgfab);  
    if (!($VMS_STATUS_SUCCESS(status)))
    {
        /* No luck with LTDL_LIBRARY_PATH; try SYS$SHARE */
        strcpy( defimg, "SYS$SHARE:.EXE" );
        imgfab.fab$b_dns = strlen(defimg);

        status = sys$parse (&imgfab);
        if (!($VMS_STATUS_SUCCESS(status)))
        {
	error_status = status;
	return NULL;
    }
    }

    dh->fnmdes.dsc$b_dtype = DSC$K_DTYPE_T;
    dh->fnmdes.dsc$b_class = DSC$K_CLASS_S;
    dh->fnmdes.dsc$a_pointer = imgnam.nam$l_name;
    dh->fnmdes.dsc$w_length = imgnam.nam$b_name;
    dh->imgdes.dsc$b_dtype = DSC$K_DTYPE_T;
    dh->imgdes.dsc$b_class = DSC$K_CLASS_S;
    dh->imgdes.dsc$a_pointer = dh->filename;
    dh->imgdes.dsc$w_length = imgnam.nam$b_esl;  

    /*
    ** Attempt to load a symbol at this stage to
    ** validate that the shared file can be loaded
    */
    lt_dlsym (dh, "Fake_Symbol_Name");
    if (!((error_status ^ LIB$_KEYNOTFOU) & ~7)) error_status = SS$_NORMAL;

    if (!($VMS_STATUS_SUCCESS(error_status)))
    {
	free (dh);
	return NULL;
    }
 
    return dh;
}

int lt_dlclose (void *handle)
{
    free (handle);
    return 0;
}

void * lt_dlsym (void *handle, const char *name)
{
    vms_dl			*dh;
    void			*ptr;
    int				status, flags;

    dh = (vms_dl *)handle;
    if (!dh) return NULL;

    dh->symdes.dsc$b_dtype = DSC$K_DTYPE_T;  
    dh->symdes.dsc$b_class = DSC$K_CLASS_S;
    dh->symdes.dsc$a_pointer = (char *) name;
    dh->symdes.dsc$w_length = strlen (name);

    /* firstly attempt with flags set to 0 case insensitive */
    flags = 0;
    status = vms_dlsym (dh, &ptr, flags);
    if (!($VMS_STATUS_SUCCESS(status)))
    {
	/*
	** Try again with mixed case flag set 
	*/
        flags = LIB$M_FIS_MIXEDCASE;

	status = vms_dlsym (dh, &ptr, flags);
	if (!($VMS_STATUS_SUCCESS(status)))
	{
	    error_status = status;
	    return NULL;
	}
    }

    return ptr;
}

int vms_dlsym (
    vms_dl	*dh,
    void	**ptr,
    int		flags)
{
    LIB$ESTABLISH (LIB$SIG_TO_RET);
    return LIB$FIND_IMAGE_SYMBOL (&dh->fnmdes, &dh->symdes, ptr, &dh->imgdes, flags);
}


const char *lt_dlerror (void)
{
    struct dsc$descriptor   desc;
    short		    outlen;
    int			    status;

    if (($VMS_STATUS_SUCCESS(error_status))) return NULL;
    
    desc.dsc$b_dtype = DSC$K_DTYPE_T;
    desc.dsc$b_class = DSC$K_CLASS_S;  
    desc.dsc$a_pointer = error_buffer;
    desc.dsc$w_length = sizeof (error_buffer);

    status = sys$getmsg (error_status, &outlen, &desc, 15, 0);  
    if ($VMS_STATUS_SUCCESS(status)) error_buffer[outlen] = '\0';    
    else sprintf (error_buffer, "OpenVMS exit status %8X", error_status);

    error_status = SS$_NORMAL;  

    return (error_buffer);
}

struct itemlist3 {
    unsigned short	buflen;
    unsigned short	item;
    void		*buf;
    unsigned short	*retlen;
};

int lt_dladdsearchdir (const char *search_dir)
{
    /*
     * Adds a path to the list of paths where lt_dlopen will look for shareable images.
     * We do this via a user-mode search list logical, adding one more item to the end of
     * the list whenever called.
     */

    $DESCRIPTOR(lib_path_d, "LTDL_LIBRARY_PATH");
    $DESCRIPTOR( proc_table_d, "LNM$PROCESS_TABLE" );
    unsigned int status = 0;
    unsigned char accmode = 0, lnm_exists = 1;
    int index = 0, max_index = 0;
    struct itemlist3 trnlnmlst[4] = {{sizeof(accmode), LNM$_ACMODE, &accmode, 0},
                                     {sizeof(max_index), LNM$_MAX_INDEX, &max_index, 0},
                                     {0, 0, 0, 0},
                                     {0, 0, 0, 0}};
    struct itemlist3 *crelnmlst;
    struct eqvarray {
      char eqvname[256];
    };
    struct eqvarray *eqvlist;

    /* First just check to see whether the logical name exists and how many equivalence
     * names there are.
     */
    status = SYS$TRNLNM ( NULL,
                          &proc_table_d,
                          &lib_path_d,
                          NULL,
                          trnlnmlst);

    /* If the logical name doesn't exist or exists at a privilege mode or table I
     * can't even look at, then I'll want to proceed with creating my user-mode logical.
     */
    if ( status == SS$_NOLOGNAM || status == SS$_NOPRIV ) {
       status = SS$_NORMAL;
       lnm_exists = 0;        /* skip further translation attempts */
       max_index = 0;
    }
    if (!$VMS_STATUS_SUCCESS(status)) {
       error_status = status;
       return -1;
    }

    /* Also skip further translation if the first translation exists in any mode other
     * than user mode; we want to stick user mode logicals in front of these.
     */
    if ( accmode != PSL$C_USER ) {
       lnm_exists = 0;
       max_index = 0;
    }

    /* Allocate an item list for logical name creation and an array of equivalence
     * name buffers.
     */
    crelnmlst = (struct itemlist3 *) malloc( sizeof(struct itemlist3) * (max_index + 3) );
    if (crelnmlst == NULL) { error_status = SS$_INSFMEM; return -1; }

    eqvlist = (struct eqvarray *) malloc( sizeof(struct eqvarray) * (max_index + 2) );
    if (eqvlist == NULL) { error_status = SS$_INSFMEM; return -1; }

    trnlnmlst[1].buflen = sizeof(index);
    trnlnmlst[1].item = LNM$_INDEX;
    trnlnmlst[1].buf = &index;
    trnlnmlst[1].retlen = NULL;

    trnlnmlst[2].buflen = sizeof(eqvlist[0].eqvname);
    trnlnmlst[2].item = LNM$_STRING;

    /* The loops iterates over the search list index, getting the translation
     * for each index and storing it in the item list for re-creation.
     */

    for (index = 0; index <= max_index && lnm_exists; index++ ) {

        /* Wire the input buffer for translation to the output buffer for creation */
        trnlnmlst[2].buf = &eqvlist[index].eqvname;
        crelnmlst[index].buf = &eqvlist[index].eqvname;
        trnlnmlst[2].retlen = &crelnmlst[index].buflen;

        status = SYS$TRNLNM ( NULL,
                              &proc_table_d,
                              &lib_path_d,
                              NULL,
                              trnlnmlst);

        if (!$VMS_STATUS_SUCCESS(status)) {
           error_status = status;
           free(crelnmlst);
           free(eqvlist);
           return -1;
        }

        /* If we come across a non-user-mode translation, back up and get out
         * because we don't want to recreate those in user mode.
         */
        if ( accmode != PSL$C_USER ) {
            index--;
            break;
        }

        crelnmlst[index].item = LNM$_STRING;
        crelnmlst[index].retlen = NULL;
    }

    /* At this point we have captured all the existing translations (if
     * any) and stored them in the item list for re-creation of the logical
     * name.  All that's left is to add the new item to the end of the list
     * and terminate the list.
     */

    crelnmlst[index].buflen = strlen(search_dir);
    crelnmlst[index].item = LNM$_STRING;
    crelnmlst[index].buf = (char *) search_dir;
    crelnmlst[index].retlen = NULL;

    crelnmlst[index+1].buflen = 0;
    crelnmlst[index+1].item = 0;
    crelnmlst[index+1].buf = NULL;
    crelnmlst[index+1].retlen = NULL;

    accmode = PSL$C_USER;

    status = SYS$CRELNM( NULL,
                         &proc_table_d,
                         &lib_path_d,
                         &accmode,
                         crelnmlst );

    if (!$VMS_STATUS_SUCCESS(status)) {
       error_status = status;
       free(crelnmlst);
       free(eqvlist);
       return -1;
    }

    free(crelnmlst);
    free(eqvlist);
    error_status = SS$_NORMAL;
    return 0;
}


char * getvmsenv (char *symbol)
{
    int			    status;
    unsigned short	    cbvalue;
    $DESCRIPTOR		    (logicalnametable, "LNM$FILE_DEV");
    struct dsc$descriptor_s logicalname;
    struct itemlist3	    itemlist[2];

    logicalname.dsc$w_length = strlen (symbol);
    logicalname.dsc$b_dtype = DSC$K_DTYPE_T;
    logicalname.dsc$b_class = DSC$K_CLASS_S;
    logicalname.dsc$a_pointer = symbol;

    itemlist[0].buflen = sizeof (getenv_buffer) -1;
    itemlist[0].item = LNM$_STRING;
    itemlist[0].buf = getenv_buffer;
    itemlist[0].retlen = &cbvalue;

    itemlist[1].buflen = 0;
    itemlist[1].item = 0;
    itemlist[1].buf = 0;
    itemlist[1].retlen = 0;

    status = SYS$TRNLNM (0, &logicalnametable, &logicalname, 0, itemlist);
    if (!($VMS_STATUS_SUCCESS(status))) return NULL;

    if (cbvalue > 0)
    {
	getenv_buffer[cbvalue] = '\0';
	return getenv_buffer;
    }

    return NULL;
}

