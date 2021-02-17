#!/usr/bin/tixwish
#############################################################################
# Visual Tcl v1.20 Project
#
# ndslogin.tcl
#  Graphical Login to a NDS tree.
#  Will authenticate, open a ncp permanent connection to the SYS volume
#     of the authenticating server and optionnally mount the user's home
#     directory and open a KDE window on it.
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

#        1.00  January      1999           Patrick Pollet
#                Initial version using Caldera NDS client
#        1.1   February     2001           Patrick Pollet
#                adapted for ncpfs NDS client


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
catch { image delete organiz
        image delete logo
        image delete tree32 }
global SCRIPT_DIR
initpath
image create photo organiz -file [file join $SCRIPT_DIR img/organiz.gif]
image create photo logo -file [file join $SCRIPT_DIR img/nw.gif]
image create photo tree32 -file [file join $SCRIPT_DIR img/tree32.gif]
uplevel #0  [list source [file join $SCRIPT_DIR ndsutils.tcl]]
uplevel #0  [list source [file join $SCRIPT_DIR ndsstrings.tcl]]

#consider argument as a tree name
if {$argc } {
      puts "$argv"
      NDS:init $argv
    } else {
      NDS:init
    }
}

init $argc $argv

proc {login} {} {
global password tree username context
global openhome_onlogin mount_locally

  if {$password !=""} {
      set status [NDS:login $tree $username $password $context]
  } else {
      set status [NDS:login_nopwd $tree $username $context]
  }
  if { $status == 1 } {
      if {$openhome_onlogin } {
          if { [NDS:openhome $tree ] ==0 }  {}
      }
    exit
  }
}

proc {toggledisplay} {} {
global STR;
global y;
  if {$y=="1"} {
        place configure .top28.tix29 -y 140
        place configure .top28.fra29 -y 250
        set y 0
        wm geometry .top28 440x280+145+100
        wm withdraw .top28
        wm deiconify .top28
        .top28.fra29.but35 configure -text $STR(minus)
   } else {
        place configure .top28.tix29 -y 500
        place configure .top28.fra29 -y 140
        set y 1
        wm geometry .top28 440x170+145+100
        wm withdraw .top28
        wm deiconify .top28
       .top28.fra29.but35 configure -text $STR(plus)
  }
}

proc {main} {argc argv} {
global tree;
global context;
global username;
global password;
global NDS;
global y;
global openhome_onlogin mount_locally; # from /etc/ndsclient.conf

set numinfo 0
#these three are possible env variables for the Caldera nwclient
catch {
  if {$NDS(default_tree) !="" } {
        incr numinfo
   }
   focus .top28.fra38.ent41
   if {$NDS(default_context)!=""} {
     incr numinfo
   }
   if {$NDS(username)!=""} {
     set username  $NDS(username)
     incr numinfo
     focus .top28.fra38.ent42
   }
#this one is an extra by PP for a possible autologon
  if {$NDS(password)!=""} {
    set password  $NDS(password)
    incr numinfo
  }
}
# get global preferences
         source /etc/ndsclient.conf
# if allowed by root
        if {$home_override != "0"} {source ~/.ndsclient.conf}
        set y $start_advanced
        set tree $NDS(default_tree)
        set context $NDS(default_context)
        if {$browse_tree !="0"} {
           .top28.fra29.but35 configure -state normal
        } else {
                # Plus/moins button out of the window (hide it !)
                place configure .top28.fra29.but35  -y 999
                #and Ok a bit to the right
                place configure .top28.fra29.but32  -x 230
                set y 0
        }
        toggledisplay

 if {$numinfo =="4" && $autologon !=0} {
         login
 }
}



proc {Window} {args} {
global vTcl
    set cmd [lindex $args 0]
    set name [lindex $args 1]
    set newname [lindex $args 2]
    set rest [lrange $args 3 end]
    if {$name == "" || $cmd == ""} {return}
    if {$newname == ""} {
        set newname $name
    }
    set exists [winfo exists $newname]
    switch $cmd {
        show {
            if {$exists == "1" && $name != "."} {wm deiconify $name; return}
            if {[info procs vTclWindow(pre)$name] != ""} {
                eval "vTclWindow(pre)$name $newname $rest"
            }
            if {[info procs vTclWindow$name] != ""} {
                eval "vTclWindow$name $newname $rest"
            }
            if {[info procs vTclWindow(post)$name] != ""} {
                eval "vTclWindow(post)$name $newname $rest"
            }
        }
        hide    { if $exists {wm withdraw $newname; return} }
        iconify { if $exists {wm iconify $newname; return} }
        destroy { if $exists {destroy $newname; return} }
    }
}

#################################
# VTCL GENERATED GUI PROCEDURES
#

proc vTclWindow. {base} {
    if {$base == ""} {
        set base .
    }
    ###################
    # CREATING WIDGETS
    ###################
    wm focusmodel $base passive
    wm geometry $base 1x1+0+0
    wm maxsize $base 1137 834
    wm minsize $base 1 1
    wm overrideredirect $base 0
    wm resizable $base 1 1
    wm withdraw $base
    wm title $base "vt.tcl"
    ###################
    # SETTING GEOMETRY
    ###################
}

proc vTclWindow.top28 {base} {
global STR;
    if {$base == ""} {
        set base .top28
    }
    if {[winfo exists $base]} {
        wm deiconify $base; return
    }
    ###################
    # CREATING WIDGETS
    ###################
    toplevel $base -class Toplevel
    wm focusmodel $base passive
    wm geometry $base 460x280+149+124
    wm maxsize $base 1265 994
    wm minsize $base 1 1
    wm overrideredirect $base 0
    wm resizable $base 0 0
    wm deiconify $base
    wm title $base $STR(ndslogin)

    tixNoteBook $base.tix29
    $base.tix29 add page1 \
        -anchor center -label  $STR(login)
    entry $base.tix29.nbframe.page1.ent47 \
        -background #fefefe -textvariable tree
    place $base.tix29.nbframe.page1.ent47 \
        -x 107 -y 0 -width 261 -height 24 -anchor nw -bordermode ignore
    label $base.tix29.nbframe.page1.lab48 \
        -anchor w -borderwidth 1 -font {Helvetica -12 normal} -justify right \
        -text $STR(tree)
    place $base.tix29.nbframe.page1.lab48 \
        -x 105 -y 20  -anchor se -bordermode ignore
    label $base.tix29.nbframe.page1.lab55 \
        -anchor se -borderwidth 1 -font {Helvetica -12 normal} -justify right \
        -text $STR(context)
    place $base.tix29.nbframe.page1.lab55 \
        -x 105 -y 50 -anchor se -bordermode ignore
    entry $base.tix29.nbframe.page1.ent56 \
        -background #fefefe -textvariable context
    place $base.tix29.nbframe.page1.ent56 \
        -x 107 -y 30 -width 260 -height 24 -anchor nw -bordermode ignore
    button $base.tix29.nbframe.page1.but30 \
        -borderwidth 1 -command {global SCRIPT_DIR;source [file join $SCRIPT_DIR contexts.tcl]} -image organiz -padx 9 \
        -pady 3
    place $base.tix29.nbframe.page1.but30 \
        -x 375 -y 30 -width 37 -height 29 -anchor nw -bordermode ignore
    button $base.tix29.nbframe.page1.but60 \
        -borderwidth 1 -command {global SCRIPT_DIR; source [file join $SCRIPT_DIR trees.tcl]} -image tree32 -padx 9 \
        -pady 3
    place $base.tix29.nbframe.page1.but60 \
        -x 375 -y 0 -width 37 -height 29 -anchor nw -bordermode ignore
    frame $base.fra38 \
        -borderwidth 2 -height 75 -width 125

    label $base.fra38.lab39 \
        -anchor e -borderwidth 1 -font {Helvetica -12 normal} -justify right \
        -text $STR(loginname)
    label $base.fra38.lab40 \
        -anchor e -borderwidth 1 -font {Helvetica -12 normal} -justify right \
        -text $STR(password)
    entry $base.fra38.ent41 \
        -background #fefefe -exportselection 0 -textvariable username
    bind $base.fra38.ent41 <Key-Return> {
        .top28.fra38.ent42 selection from 0
.top28.fra38.ent42 selection to end
focus .top28.fra38.ent42
    }
    bind $base.fra38.ent41 <Key-Tab> {
        .top28.fra38.ent42 selection from 0
.top28.fra38.ent42 selection to end
focus .top28.fra38.ent42
    }
    entry $base.fra38.ent42 \
        -background #fefefe -exportselection 0 -show * -textvariable password
    bind $base.fra38.ent42 <Key-Return> {
        login
    }
    label $base.lab28 \
        -image logo -relief sunken -text label
    frame $base.fra29 \
        -height 75 -relief groove -width 125
    button $base.fra29.but31 \
        -activeforeground #0000fe -command exit -highlightthickness 0 -padx 9 \
        -pady 3 -text $STR(cancel)
    button $base.fra29.but32 \
        -activeforeground #0000fe -command login -highlightthickness 0 \
        -padx 9 -pady 3 -text $STR(ok)
    button $base.fra29.but34 \
        -activeforeground #0000fe -command {global SCRIPT_DIR;source [file join $SCRIPT_DIR  about.tcl]} -padx 9 \
        -pady 3 -text $STR(about)
    button $base.fra29.but35 \
        -activeforeground blue -command toggledisplay -highlightcolor black \
        -highlightthickness 0 -padx 9 -pady 3 -text $STR(minus)
    ###################
    # SETTING GEOMETRY
    ###################
    place $base.tix29 \
        -x 3 -y 140 -width 455 -height 103 -anchor nw -bordermode ignore
    place $base.fra38 \
        -x 5 -y 80 -width 455 -height 55 -anchor nw -bordermode ignore
    place $base.fra38.lab39 \
        -x 90 -y 25  -anchor se -bordermode ignore
    place $base.fra38.lab40 \
        -x 90 -y 50  -anchor se -bordermode ignore
    place $base.fra38.ent41 \
        -x 95 -y 5 -width 335 -height 24 -anchor nw -bordermode ignore
    place $base.fra38.ent42 \
        -x 95 -y 30 -width 335 -height 24 -anchor nw -bordermode ignore
    place $base.lab28 \
        -x 0 -y 0 -width 455 -height 73 -anchor nw -bordermode ignore
    place $base.fra29 \
        -x 10 -y 250 -width 455 -height 30 -anchor nw -bordermode ignore
    place $base.fra29.but31 \
        -x 30 -y 0 -width 90 -height 23 -anchor nw -bordermode ignore
    place $base.fra29.but32 \
        -x 130 -y 0 -width 90 -height 23 -anchor nw -bordermode ignore
    place $base.fra29.but35 \
        -x 230 -y 0 -width 90 -height 23 -anchor nw -bordermode ignore
    place $base.fra29.but34 \
        -x 330 -y 0 -width 90 -height 23 -anchor nw -bordermode ignore

}

Window show .
Window show .top28

main $argc $argv
