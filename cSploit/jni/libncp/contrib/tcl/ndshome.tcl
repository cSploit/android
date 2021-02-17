#!/usr/bin/tixwish
#############################################################################
# Visual Tcl v1.20 Project
#
# ndshome.tcl - open a KDE window on current user Netware home directory
#    User must be authenticated to that tree, otherwise ndslogin.tcl is
#     executed.
#    Copyright (C) 2001 by Patrick Pollet
#
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.

#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.

#    You should have received a copy of the GNU General Public License
#    along with this program; if not, write to the Free Software
#    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


#    Revision history:

#        1.00  2001      February 2001           Patrick Pollet
#                Initial release.
#	1.2	May 	2002	Patrick Pollet
#		added support for the new -l option of ncplogin/ncpmap
#		if option mount_locally is 1 in /etc/ndsclient.conf

#################################
# GLOBAL VARIABLES
#

#################################
# USER DEFINED PROCEDURES
#

proc {initpath} {} {
#set up path to others scripts and images
global SCRIPT_DIR env

 if {![info exists env(NDSCLIENT_HOME)]} {
            set home [file dirname [info script]]
            switch [file pathtype $home] {
                absolute {set env(NDSCLIENT_HOME) $home}
                relative {set env(NDSCLIENT_HOME) [file join [pwd] $home]}
                volumerelative {
                    set curdir [pwd]
                    cd $home
                    set env(NDSCLIENT_HOME) [file join [pwd] [file dirname \
                        [file join [lrange [file split $home] 1 end]]]]
                    cd $curdir
                }
            }
    }
set   SCRIPT_DIR $env(NDSCLIENT_HOME)
#puts $SCRIPT_DIR
}

proc init {argc argv} {
global SCRIPT_DIR NDS
global mount_locally; # from /etc/ndsclient.conf
initpath
uplevel #0  [list source [file join $SCRIPT_DIR ndsutils.tcl]]

set tree ""
if { $argc } {
      puts "$argv"
      set tree $argv
 }
#use defaults or command line argument as the tree name
 NDS:init $tree

 set user $NDS(username)
 if {$user !=""} {
 	source /etc/ndsclient.conf
        puts "searching for home of $user on tree $NDS(tree)"
         set res [NDS:openhome $NDS(tree) ]
         if {$res =="1"} {
	        exit
	 }
 }
 source [file join $SCRIPT_DIR ndslogin.tcl]

}

init $argc $argv

