#! /bin/sh
#
# $Id: unixodbc.install.sh,v 1.1 2002-10-09 17:05:44 castellano Exp $
#
# FreeTDS - Library of routines accessing Sybase and Microsoft databases
# Copyright (C) 1998-1999  Brian Bruns
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Library General Public
# License as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Library General Public License for more details.
#
# You should have received a copy of the GNU Library General Public
# License along with this library; if not, write to the
# Free Software Foundation, Inc., 59 Temple Place - Suite 330,
# Boston, MA 02111-1307, USA.

#
# Run this script as root to install FreeTDS unixODBC drivers and datasources.
#
# You will most likely want to edit unixodbc.jdbc.datasource.template
# to add your own dataserver instead of the public Sybase dataserver.
#
# For more information about these template files, please see
# http://www.unixodbc.org/doc/FreeTDS.html
#

# Install driver.
odbcinst -i -d -f unixodbc.freetds.driver.template
# Install system datasource.  
odbcinst -i -s -l -f unixodbc.jdbc.datasource.template
