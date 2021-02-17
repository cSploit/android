/* FreeTDS - Library of routines accessing Sybase and Microsoft databases
 * Copyright (C) 2005  James K. Lowden
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*! <!-- $Id: sybdb.dox.h,v 1.2 2005-06-26 14:25:20 jklowden Exp $ --> */
/*! <!-- There is no code in this file.  It is strictly for documentation. --> */

/** \fn DBCMDROW(x)
 * \ingroup dblib_core
 * \brief Sybase macro mapping to the Microsoft (lower-case) function.  
 * \sa dbcmdrow()
 */
/** \fn DBCOUNT(x)
 * \ingroup dblib_core
 * \brief Sybase macro mapping to the Microsoft (lower-case) function.  
 * \sa dbcount()
 */
/** \fn DBCURCMD(x)
 * \ingroup dblib_core
 * \brief Sybase macro mapping to the Microsoft (lower-case) function.  
 * \sa dbcurcmd()
 */
/** \fn DBCURROW(x)
 * \ingroup dblib_core
 * \brief Sybase macro mapping to the Microsoft (lower-case) function.  
 * \sa dbcurrow()
 */
/** \fn DBDEAD(x)
 * \ingroup dblib_core
 * \brief Sybase macro mapping to the Microsoft (lower-case) function.  
 * \sa dbdead()
 */
/** \fn DBFIRSTROW(x)
 * \ingroup dblib_core
 * \brief Sybase macro mapping to the Microsoft (lower-case) function.  
 * \sa dbfirstrow()
 */
/** \fn DBIORDESC(x)
 * \ingroup dblib_core
 * \brief Sybase macro, maps to the internal (lower-case) function.  
 * \sa dbiordesc()
 */
/** \fn DBIOWDESC(x)
 * \ingroup dblib_core
 * \brief Sybase macro, maps to the internal (lower-case) function.  
 * \sa dbiowdesc()
 */
/** \fn DBISAVAIL(x)
 * \ingroup dblib_core
 * \brief Sybase macro mapping to the Microsoft (lower-case) function.  
 * \sa dbisavail()
 */
/** \fn DBLASTROW(x)
 * \ingroup dblib_core
 * \brief Sybase macro mapping to the Microsoft (lower-case) function.  
 * \sa dblastrow(), DBFIRSTROW()
 */
/** \fn DBMORECMDS(x)
 * \ingroup dblib_core
 * \brief Sybase macro mapping to the Microsoft (lower-case) function.  
 * \sa dbmorecmds()
 */
/** \fn DBROWS(x)
 * \ingroup dblib_core
 * \brief Sybase macro mapping to the Microsoft (lower-case) function.  
 * \sa dbrows()
 */
/** \fn DBROWTYPE(x)
 * \ingroup dblib_core
 * \brief Sybase macro mapping to the Microsoft (lower-case) function.  
 * \sa dbrowtype()
 */
/** \fn DBTDS(a)
 * \ingroup dblib_core
 * \brief Sybase macro, maps to the internal (lower-case) function.  
 * \sa dbtds()
 */
 /*------------------------*/
/** \fn DBSETLHOST(x,y)
 * \ingroup dblib_api
 * \brief Set the (client) host name in the login packet.  
 * \sa dbsetlhost()
 */
/** \fn DBSETLUSER(x,y)
 * \ingroup dblib_api
 * \brief Set the username in the login packet.  
 * \sa dbsetluser()
 */
/** \fn DBSETLPWD(x,y)
 * \ingroup dblib_api
 * \brief Set the password in the login packet.  
 * \sa dbsetlpwd()
 */
/** \fn DBSETLAPP(x,y)
 * \ingroup dblib_api
 * \brief Set the (client) application name in the login packet.  
 * \sa dbsetlapp()
 */
/** \fn BCP_SETL(x,y)
 * \ingroup dblib_api
 * \brief Enable (or prevent) bcp operations for connections made with a login.  
 * \sa bcp_setl()
 */
 /*------------------------*/
/** \fn DBSETLCHARSET(x,y)
 * \ingroup dblib_core
 * \brief Set the client character set in the login packet. 
 * \remark Has no effect on TDS 7.0+ connections.  
 */
/** \fn DBSETLNATLANG(x,y)
 * \ingroup dblib_core
 * \brief Set the language the server should use for messages.  
 * \sa dbsetlnatlang(), dbsetlname()
 */
/** \fn dbsetlnatlang(x,y)
 * \ingroup dblib_core
 * \brief Set the language the server should use for messages.  
 * \sa DBSETLNATLANG(), dbsetlname()
 */
/* \fn DBSETLHID(x,y) (not implemented)
 * \ingroup dblib_core
 * \brief 
 * \sa dbsetlhid()
 */
/* \fn DBSETLNOSHORT(x,y) (not implemented)
 * \ingroup dblib_core
 * \brief 
 * \sa dbsetlnoshort()
 */
/* \fn DBSETLHIER(x,y) (not implemented)
 * \ingroup dblib_core
 * \brief 
 * \sa dbsetlhier()
 */
/** \fn DBSETLPACKET(x,y)
 * \ingroup dblib_core
 * \brief Set the packet size in the login packet for new connections.  
 * \sa dbsetlpacket(), dbsetllong()
 */
/** \fn dbsetlpacket(x,y)
 * \ingroup dblib_core
 * \brief Set the packet size in the login packet for new connections.  
 * \sa DBSETLPACKET(), dbsetllong()
 */
/** \fn DBSETLENCRYPT(x,y)
 * \ingroup dblib_core
 * \brief Enable (or not) network password encryption for Sybase servers version 10.0 or above.
 * \todo Unimplemented.
 * \sa dbsetlencrypt()
 */
/** \fn DBSETLLABELED(x,y)
 * \ingroup dblib_internal
 * \brief Alternative way to set login packet fields.  
 * \sa dbsetllabeled()
 */
/* \fn BCP_SETLABELED(x,y) (not implemented)
 * \ingroup dblib_internal
 * \brief Sybase macro mapping to the Microsoft (lower-case) function.  
 * \sa bcp_setlabeled()
 */
/** \fn DBSETLVERSION(login, version)
 * \ingroup dblib_internal
 * \brief maps to the Microsoft (lower-case) function.  
 * \sa dbsetlversion()
 */

 /*------------------------*/
 
/** \mainpage FreeTDS Reference Manual
 *
 * \section intro_sec Introduction
 *
   Given that the vendors provide their own complete documentation for their implementations, 
   one might ask why embark on this adventure at all?  The answer is both principled and practical.  
   
   First, this is \a our implementation, and to be complete it needs independent documentation.  
   Unlike the vendors' offerings, ours offers some features from each version, while lacking others.  
   When we have been forced to choose between documented behaviors, the FreeTDS user is presumably 
   best served by the FreeTDS reference manual.  
   
   Second, while the vendors today freely offer their documentation on the world wide web, that might
   not -- in fact, surely will not -- always be the case.  Should the vendor decide to drop its offering 
   for whatever reason, it will likely pull its documentation, too, sooner or later.  Free sofware, in short, 
   can lean on the capitalist model for support, but it can never rely on it.  
  
   \section status Status

   This manual is incomplete, and is being completed in fits and starts.  
   The focus to date has been on \a db-lib and the underlying network library, \a libtds. 
   It's the kind of enormous Quixoitic undertaking that you'd reasonably doubt will
   ever be completed.  If you feel inclined to jump in, please don't hesitate.  
   
   The organization of the manual could be better.  We're still learning how to use Doxygen to 
   best effect. 
   
   Probably the most useful link at the moment is "Modules", above.  That gives a categorized, alphabetical
   listing of the documented functions.  
 * 
 */

