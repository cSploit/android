/* intl-compat.c - Stub functions to call gettext functions from GNU gettext
   Library.
   Copyright (C) 1995, 2000-2002 Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU Library General Public License as published
   by the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
   USA.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#define _INTL_REDIRECT_MACROS
#include "libgnuintl.h"
#include "gettextP.h"

/* @@ end of prolog @@ */

/* This file redirects the gettext functions (without prefix) to those
   defined in the included GNU libintl library (with "libintl_" prefix).
   It is compiled into libintl in order to make the AM_GNU_GETTEXT test
   of gettext <= 0.11.2 work with the libintl library >= 0.11.3 which
   has the redirections primarily in the <libintl.h> include file.  */


#undef gettext
#undef dgettext
#undef dcgettext
#undef ngettext
#undef dngettext
#undef dcngettext
#undef textdomain
#undef bindtextdomain
#undef bind_textdomain_codeset


char *
gettext (msgid)
     const char *msgid;
{
  return libintl_gettext (msgid);
}


char *
dgettext (domainname, msgid)
     const char *domainname;
     const char *msgid;
{
  return libintl_dgettext (domainname, msgid);
}


char *
dcgettext (domainname, msgid, category)
     const char *domainname;
     const char *msgid;
     int category;
{
  return libintl_dcgettext (domainname, msgid, category);
}


char *
ngettext (msgid1, msgid2, n)
     const char *msgid1;
     const char *msgid2;
     unsigned long int n;
{
  return libintl_ngettext (msgid1, msgid2, n);
}


char *
dngettext (domainname, msgid1, msgid2, n)
     const char *domainname;
     const char *msgid1;
     const char *msgid2;
     unsigned long int n;
{
  return libintl_dngettext (domainname, msgid1, msgid2, n);
}


char *
dcngettext (domainname, msgid1, msgid2, n, category)
     const char *domainname;
     const char *msgid1;
     const char *msgid2;
     unsigned long int n;
     int category;
{
  return libintl_dcngettext (domainname, msgid1, msgid2, n, category);
}


char *
textdomain (domainname)
     const char *domainname;
{
  return libintl_textdomain (domainname);
}


char *
bindtextdomain (domainname, dirname)
     const char *domainname;
     const char *dirname;
{
  return libintl_bindtextdomain (domainname, dirname);
}


char *
bind_textdomain_codeset (domainname, codeset)
     const char *domainname;
     const char *codeset;
{
  return libintl_bind_textdomain_codeset (domainname, codeset);
}
