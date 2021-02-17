#!/usr/bin/tixwish
#############################################################################
# Visual Tcl v1.20 Project
#
# ndsspace.tcl
#  Graphical display of Netware disk quota on a ncpmounted
# permanent connection.
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
#         1.2   March        2001           Patrick Pollet
#	        added server/volume selection logic
#         1.3   Feb 2002                     Patrick Pollet
#                    in some cases ncpvrest chokes not finding current userid
#                    so we now force the username with the -N parameter
#################################
# GLOBAL VARIABLES
#


#################################
# USER DEFINED PROCEDURES
#
proc {initpath} {} {
#set up path to others scripts and images
global env SCRIPT_DIR
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
global SCRIPT_DIR
  initpath
  uplevel #0 [list source [file join $SCRIPT_DIR ndsutils.tcl]]
  uplevel #0 [list source [file join $SCRIPT_DIR ndsstrings.tcl]]
  NDS:init
}

init $argc $argv

proc changeVolume {} {
global NDS
        catch {.top28.treelbx curselection} num
	if {$num !="" } {
                catch {.top28.treelbx get $num} srv_vol
	        set tmpl [split $srv_vol '/']
		set NDS(server) [lindex $tmpl 0]
		set NDS(volume) [lindex $tmpl 1]
		refresh
	}

}


proc {getquota} {} {
# order quota used left
global NDS;
set tmp [exec ncpvrest -S $NDS(server) -V $NDS(volume) -N $NDS(username) ]

set tmpl [split $tmp :]
set limit [lindex $tmpl 2]
set used  [lindex $tmpl 3]
set left  [expr $limit -$used]

set tmpl  [list $used $limit $left]
#NDS:dialog $tmpl
return $tmpl
}


proc {refresh} {} {
  set GREEN   #008e00
  set ORANGE  #feaa00
  set RED     #ee0000


  set quotas [getquota]
  set left [lindex $quotas 2]
  set color $RED
  if {$left >= "1000" } {
      set color $GREEN
  } else {
      if {$left > "500"} {
           set color $ORANGE
       } else {
           set color $RED
       }
   }
  .top28 configure -background $color
  .top28.labut    configure -background $color
  .top28.lablim   configure -background $color
  .top28.labrest  configure -background $color
  .top28.labutR    configure -background $color
  .top28.lablimR   configure -background $color
  .top28.labrestR  configure -background $color
  .top28.labutR   configure -text [format "%9d Kb" [lindex $quotas 0]]
  .top28.lablimR  configure -text [format "%9d Kb" [lindex $quotas 1]]
  .top28.labrestR configure -text [format "%9d Kb" [lindex $quotas 2]]
  after 5000 changeVolume
}


proc getVolumes {} {
  catch {exec ncpwhoami -fSV -s / } result
#  puts $result
  set lresult [split $result \n]
  .top28.treelbx delete 0 end
  foreach item $lresult {
    .top28.treelbx insert end $item
  }
  .top28.treelbx selection set 0
  changeVolume

}

proc {main} {argc argv} {
global NDS STR
    set user $NDS(username)
    puts $user
       # .top28.lab35 configure -text $user
    if {$user !=""} {
        getVolumes
     } else {
          set tmpl [split $STR(not_logged_in) ]
	 .top28.labutR configure -text  [lindex $tmpl 0]
         .top28.lablimR configure -text  [lindex $tmpl 1]
         .top28.labrestR configure -text [lindex $tmpl 2]
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
    wm maxsize $base 785 570
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
global STR
    if {$base == ""} {
        set base .top28
    }
    if {[winfo exists $base]} {
        wm deiconify $base; return
    }
    ###################
    # CREATING WIDGETS
    ###################
    toplevel $base -class Toplevel \
        -background #00ae00
    wm focusmodel $base passive
    wm geometry $base 180x160+292+271
    wm maxsize $base 785 570
    wm minsize $base 1 1
    wm overrideredirect $base 0
    wm resizable $base 0 0
    wm deiconify $base
    wm title $base $STR(ndsspace)
    wm protocol $base WM_DELETE_WINDOW {exit}

        listbox $base.treelbx \
        -background #dcdcdc -font {Helvetica -12 normal} -foreground #000000 \
        -highlightbackground #ffffff -highlightcolor #000000 \
        -selectbackground #547098 -selectforeground #ffffff \
        -selectmode browse -height 4\
        -yscrollcommand {.top28.scr18 set}
    scrollbar $base.scr18 -command {.top28.treelbx yview}\
        -activebackground #dcdcdc -background #dcdcdc -cursor left_ptr \
        -highlightbackground #dcdcdc -highlightcolor #000000 -orient vert \
        -troughcolor #dcdcdc -width 10


    label $base.labut \
        -borderwidth 1 -font {Helvetica -10 normal} -justify right \
        -text $STR(used)
    label $base.lablim \
        -borderwidth 1 -font {Helvetica -10 normal} -justify right \
        -text $STR(quota)
    label $base.labrest \
        -borderwidth 1 -font {Helvetica -10 normal} -justify right \
        -text $STR(left)
    label $base.labutR \
        -borderwidth 1 -font {Helvetica -12 normal} -justify left \
        -relief raised
    label $base.lablimR \
        -borderwidth 1 -font {Helvetica -12 normal} -justify left \
        -relief raised
    label $base.labrestR \
        -borderwidth 1 -font {Helvetica -12 normal} -justify left \
        -relief raised
    bind $base.treelbx <ButtonRelease-1> {
        changeVolume
    }
    bind $base.treelbx <Select> {
        changeVolume
    }

    ###################
    # SETTING GEOMETRY
    ###################
    place $base.labut \
        -x 35 -y 28  -anchor se -bordermode ignore
    place $base.lablim \
        -x 35 -y 48  -anchor se -bordermode ignore
    place $base.labrest \
        -x 35 -y 68  -anchor se -bordermode ignore
    place $base.labutR \
        -x 40 -y 10 -width 100 -height 18 -anchor nw -bordermode ignore
    place $base.lablimR \
        -x 40 -y 30 -width 100 -height 18 -anchor nw -bordermode ignore
    place $base.labrestR \
        -x 40 -y 50 -width 100 -height 18 -anchor nw -bordermode ignore
    place $base.treelbx \
        -x 20 -y 80 -width 130  -anchor nw -bordermode ignore
    place $base.scr18 \
        -x 150 -y 80 -width 16 -height 65 -anchor nw -bordermode ignore

}

Window show .
Window show .top28

main $argc $argv
