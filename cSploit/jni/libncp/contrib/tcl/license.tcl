#!/usr/bin/tixwish
#############################################################################
# Visual Tcl v1.20 Project
#

#################################
# GLOBAL VARIABLES
#
global widget; 

#################################
# USER DEFINED PROCEDURES
#
proc init {argc argv} {
global SCRIPT_DIR
catch {image delete logo}
image create photo logo -file [file join $SCRIPT_DIR  img/nw.gif]
}

init $argc $argv


proc {main} {argc argv} {
global SCRIPT_DIR
.license.tex30 delete 1.1 end
 catch {exec cat [file join $SCRIPT_DIR COPYING] } about
.license.tex30 insert end $about
# après l'avoir rempli. si on le met disabled dans WinProc
#ne le rempli pas !!!
 .license.tex30 configure -state disabled
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
    wm maxsize $base 1265 1024
    wm minsize $base 1 1
    wm overrideredirect $base 0
    wm resizable $base 1 1
    wm withdraw $base
    wm title $base "vt.tcl"
    ###################
    # SETTING GEOMETRY
    ###################
}

proc vTclWindow.license {base} {
global STR
    if {$base == ""} {
        set base .license
    }
    if {[winfo exists $base]} {
        wm deiconify $base; return
    }
    ###################
    # CREATING WIDGETS
    ###################
    toplevel $base -class Toplevel
    wm focusmodel $base passive
    wm geometry $base 455x424+149+50
    wm maxsize $base 1265 994
    wm minsize $base 1 1
    wm overrideredirect $base 0
    wm resizable $base 1 1
    wm deiconify $base
    wm title $base $STR(ndslicense)
    label $base.01 \
        -anchor s -background white -borderwidth 3 -image logo -relief sunken \
        -text label
    button $base.but40 \
        -activeforeground blue -command {destroy .license}\
        -font -Adobe-Helvetica-*-R-Normal--*-120-*-*-*-*-*-*\
        -padx 9 -pady 3 -text $STR(close)
    text $base.tex30 \
	    -font {Helvetica -10 normal} -wrap none \
        -xscrollcommand {.license.scr32 set} \
        -yscrollcommand {.license.scr31 set}
    scrollbar $base.scr31 \
        -command {.license.tex30 yview} -orient vert
    scrollbar $base.scr32 \
        -command {.license.tex30 xview} -orient horiz
    ###################
    # SETTING GEOMETRY
    ###################
    place $base.01 \
        -x 4 -y 5 -width 448 -height 87 -anchor nw -bordermode ignore
    place $base.but40 \
        -x 185 -y 385 -width 77 -height 28 -anchor nw -bordermode ignore
    place $base.tex30 \
        -x 5 -y 95 -width 430 -height 262 -anchor nw -bordermode ignore
    place $base.scr31 \
        -x 435 -y 95 -width 18 -height 262 -anchor nw -bordermode ignore
    place $base.scr32 \
        -x 5 -y 360 -width 429 -height 18 -anchor nw -bordermode ignore
}

Window show .
Window show .license

main $argc $argv
