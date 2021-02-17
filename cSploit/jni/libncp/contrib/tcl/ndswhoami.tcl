#!/usr/bin/wish
#############################################################################
# Visual Tcl v1.22 Project
#
# ndswhoami.tcl
#  Graphical information about connections to NDS trees.
#    Copyright (C) 1999,2001 by Patrick Pollet
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
#	        added tree selection logic
#################################
# GLOBAL VARIABLES
#
global NDS;
global SCRIPT_DIR;
global STR;
global widget;

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
                    set env(NDSCLIENT_HOME) [file join [pwd] [file dirname  [file join [lrange [file split $home] 1 end]]]]
                    cd $curdir
                }
            }
    }
set   SCRIPT_DIR $env(NDSCLIENT_HOME)
#puts $SCRIPT_DIR
}

proc maj_whoami {} {
global STR

    catch {.top28.treelbx curselection} num
    catch {.top28.treelbx get $num} tree
    catch {exec  ncpwhoami -fTcUM -s : -T $tree} mes
    #puts $mes
    set tmpl  [split $mes \n]
    set tmpl [split [lindex $tmpl 0] :]
    #puts $tmpl
    if {![ string match "*no ncp*" $tmpl]} {
        .top28.lab35 configure -text [lindex $tmpl 2]
        .top28.lab34 configure -text [lindex $tmpl 1]
        .top28.lab37 configure -text [lindex $tmpl 3]
    } else {
        .top28.lab35 configure -text $STR(not_logged_in)
        .top28.lab34 configure -text ""
        .top28.lab37 configure -text ""
    }
}


proc {readtrees} {} {
global NDS
 set trees [NDS:listtrees "authen"]
 .top28.treelbx delete 0 end
 set trees [lsort $trees]
 foreach tree $trees {
    .top28.treelbx insert end $tree
 }
  .top28.treelbx selection set 0
  maj_whoami
}


proc {main} {argc argv} {
global NDS STR
     .top28.lab34 configure -text { }
     .top28.lab35 configure -text $STR(not_logged_in)
     .top28.lab34 configure -text $NDS(context)
     .top28.lab37 configure -text $NDS(mnt_point)
     readtrees
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

proc init {argc argv} {
global SCRIPT_DIR
initpath
catch { image delete logo}
    image create photo logo -file [file join $SCRIPT_DIR img/nw.gif]
    uplevel #0 [list source [file join $SCRIPT_DIR ndsutils.tcl]]
    uplevel #0 [list source [file join $SCRIPT_DIR ndsstrings.tcl]]
    if {$argc } {
      puts "$argv"
      NDS:init $argv
    } else {
      NDS:init
    }
}

init $argc $argv

#################################
# VTCL GENERATED GUI PROCEDURES
#

proc vTclWindow. {base {container 0}} {
global STR
    if {$base == ""} {
        set base .
    }
    ###################
    # CREATING WIDGETS
    ###################
    if {!$container} {
    wm focusmodel $base passive
    wm geometry $base 1x1+0+0
    wm maxsize $base 785 570
    wm minsize $base 1 1
    wm overrideredirect $base 0
    wm resizable $base 1 1
    wm withdraw $base
    wm title $base "vt.tcl"
    }
    ###################
    # SETTING GEOMETRY
    ###################
}

proc vTclWindow.top28 {base {container 0}} {
global STR
    if {$base == ""} {
        set base .top28
    }
    if {[winfo exists $base] && (!$container)} {
        wm deiconify $base; return
    }
    ###################
    # CREATING WIDGETS
    ###################
    if {!$container} {
    toplevel $base -class Toplevel \
        -background #dcdcdc -highlightbackground #dcdcdc \
        -highlightcolor #000000
    wm focusmodel $base passive
    wm geometry $base 445x260+180+100
    wm maxsize $base 785 570
    wm minsize $base 1 1
    wm overrideredirect $base 0
    wm resizable $base 0 0
    wm deiconify $base
    wm title $base  $STR(ndswhoami)
    bind $base <Key-Return> {
        exit
    }
    }
    label $base.lab29 \
        -background #dcdcdc -borderwidth 1 -font {Helvetica -12 normal} \
        -foreground #000000 -highlightbackground #dcdcdc \
        -highlightcolor #000000 -justify right -text $STR(context)
    label $base.lab30 \
        -background #dcdcdc -borderwidth 1 -font {Helvetica -12 normal} \
        -foreground #000000 -highlightbackground #dcdcdc \
        -highlightcolor #000000 -justify right -text $STR(trees)
    label $base.lab31 \
        -background #dcdcdc -borderwidth 1 -font {Helvetica -12 normal} \
        -foreground #000000 -highlightbackground #dcdcdc \
        -highlightcolor #000000 -justify right -text $STR(user)
    label $base.lab32 \
        -background #dcdcdc -borderwidth 1 -font {Helvetica -12 normal} \
        -foreground #000000 -highlightbackground #dcdcdc \
        -highlightcolor #000000 -justify right -text  $STR(mnt_point)
    button $base.but32 \
        -activebackground #dcdcdc -activeforeground #0000f7 \
        -background #dcdcdc -command exit -foreground #000000 \
        -highlightbackground #dcdcdc -highlightcolor #000000 -padx 9 -pady 3 \
        -text OK
    label $base.lab34 \
        -background #dcdcdc -borderwidth 1 -font {Helvetica -12 normal} -justify left\
        -foreground #000000 -highlightbackground #dcdcdc \
        -highlightcolor #000000 -justify left -relief raised -text ""
    label $base.lab35 \
        -background #dcdcdc -borderwidth 1 -font {Helvetica -12 normal} -justify left\
        -foreground #000000 -highlightbackground #dcdcdc \
        -highlightcolor #000000 -justify left -relief raised -text ""
    label $base.lab37 \
        -background #dcdcdc -borderwidth 1 -font {Helvetica -12 normal} -justify left \
        -foreground #000000 -highlightbackground #dcdcdc \
        -highlightcolor #000000  -relief raised \
        -text ""
    label $base.lab36 \
        -background #dcdcdc -foreground #000000 -highlightbackground #dcdcdc \
        -highlightcolor #000000 -image logo -relief sunken -text label
    listbox $base.treelbx \
        -background #dcdcdc -font {Helvetica -12 normal} -foreground #000000 \
        -highlightbackground #ffffff -highlightcolor #000000 \
        -selectbackground #547098 -selectforeground #ffffff \
        -selectmode single -height 2\
        -yscrollcommand {.top28.scr18 set}
    scrollbar $base.scr18 -command {.top28.treelbx yview}\
        -activebackground #dcdcdc -background #dcdcdc -cursor left_ptr \
        -highlightbackground #dcdcdc -highlightcolor #000000 -orient vert \
        -troughcolor #dcdcdc -width 10
   bind $base.treelbx <ButtonRelease-1> {
        maj_whoami
    }
    ###################
    # SETTING GEOMETRY
    ###################
    place $base.lab29 \
        -x 115 -y 155 -anchor se -bordermode ignore
    place $base.lab30 \
        -x 115 -y 105 -anchor se -bordermode ignore
    place $base.lab31 \
        -x 115 -y 190 -anchor se -bordermode ignore
    place $base.lab32 \
        -x 115 -y 220 -anchor se -bordermode ignore
    place $base.but32 \
        -x 200 -y 230 -width 68 -height 26 -anchor nw -bordermode ignore
    place $base.lab34 \
        -x 135 -y 140 -width 216 -height 18 -anchor nw -bordermode ignore
    place $base.lab35 \
        -x 135 -y 170 -width 216 -height 18 -anchor nw -bordermode ignore
    place $base.lab37 \
        -x 135 -y 200 -width 216 -height 18 -anchor nw -bordermode ignore
    place $base.lab36 \
        -x 0 -y 0 -width 446 -height 78 -anchor nw -bordermode ignore
    place $base.treelbx \
        -x 135 -y 85 -width 213  -anchor nw -bordermode ignore
#        -x 135 -y 85 -width 213 -height 46 -anchor nw -bordermode ignore
    place $base.scr18 \
        -x 350 -y 85 -width 16 -height 42 -anchor nw -bordermode ignore
}

Window show .
Window show .top28

main $argc $argv
