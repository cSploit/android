# Copyright (c) 2010, 2013, Oracle and/or its affiliates. All rights reserved.
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 of the License.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA

IF(ESSENTIALS)
 SET(CPACK_COMPONENTS_USED "Development")
 SET(CPACK_WIX_UI "WixUI_InstallDir")
 IF(CMAKE_SIZEOF_VOID_P MATCHES 8)
  SET(CPACK_PACKAGE_FILE_NAME  "mysql-essential-${VERSION}-winx64")
 ELSE()
  SET(CPACK_PACKAGE_FILE_NAME  "mysql-essential-${VERSION}-win32")
 ENDIF()
ELSE()
  SET(CPACK_COMPONENTS_USED 
    "Development;SharedLibraries;Documentation;Readme;DebugBinaries")
ENDIF()


# Some components like Embedded are optional
# We will build MSI without embedded if it was not selected for build
#(need to modify CPACK_COMPONENTS_ALL for that)
SET(CPACK_ALL)
FOREACH(comp1 ${CPACK_COMPONENTS_USED})
 SET(found)
 FOREACH(comp2 ${CPACK_COMPONENTS_ALL})
  IF(comp1 STREQUAL comp2)
    SET(found 1)
    BREAK()
  ENDIF()
 ENDFOREACH()
 IF(found)
   SET(CPACK_ALL ${CPACK_ALL} ${comp1})
 ENDIF()
ENDFOREACH()
SET(CPACK_COMPONENTS_ALL ${CPACK_ALL})

# Always install (hidden), includes Readme files
SET(CPACK_COMPONENT_GROUP_ALWAYSINSTALL_HIDDEN 1)
SET(CPACK_COMPONENT_README_GROUP "AlwaysInstall")

 
#Feature "Devel"
SET(CPACK_COMPONENT_GROUP_DEVEL_DISPLAY_NAME "MySQL Connector/C")
SET(CPACK_COMPONENT_GROUP_DEVEL_DESCRIPTION "Installs C/C++ header files and libraries")

 #Subfeature "Development"
 SET(CPACK_COMPONENT_DEVELOPMENT_GROUP "Devel")
 SET(CPACK_COMPONENT_DEVELOPMENT_HIDDEN 1)

 #Subfeature "Shared libraries"
 SET(CPACK_COMPONENT_SHAREDLIBRARIES_GROUP "Devel")
 SET(CPACK_COMPONENT_SHAREDLIBRARIES_DISPLAY_NAME "Client C API library (shared)")
 SET(CPACK_COMPONENT_SHAREDLIBRARIES_DESCRIPTION "Installs shared client library")
 
 # Subfeature "Debug binaries" 
 SET(CPACK_COMPONENT_DEBUGBINARIES_GROUP "Devel")
 SET(CPACK_COMPONENT_DEBUGBINARIES_DISPLAY_NAME "Debug binaries")
 SET(CPACK_COMPONENT_DEBUGBINARIES_DESCRIPTION 
   "Debug/trace versions of executables and libraries" )
 #SET(CPACK_COMPONENT_DEBUGBINARIES_WIX_LEVEL 2)

#Feature Documentation
SET(CPACK_COMPONENT_DOCUMENTATION_DISPLAY_NAME "Documentation")
SET(CPACK_COMPONENT_DOCUMENTATION_DESCRIPTION "Installs documentation")
SET(CPACK_COMPONENT_DOCUMENTATION_WIX_LEVEL 2)


#Feature Misc (hidden, installs only if everything is installed)
SET(CPACK_COMPONENT_GROUP_MISC_HIDDEN 1)
SET(CPACK_COMPONENT_GROUP_MISC_WIX_LEVEL 100)
  SET(CPACK_COMPONENT_INIFILES_GROUP "Misc")
  SET(CPACK_COMPONENT_SERVER_SCRIPTS_GROUP "Misc")
