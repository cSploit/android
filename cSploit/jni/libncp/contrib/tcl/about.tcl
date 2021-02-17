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
catch {image delete logo }
image create photo logo -file [file join $SCRIPT_DIR img/nw.gif ]
}

init $argc $argv


proc {main} {argc argv} {
global SCRIPT_DIR
.about.fra41.tex56 delete 1.1 end
 catch {exec cat [file join $SCRIPT_DIR ABOUT] } tmp
 .about.fra41.tex56 insert end $tmp
# après l'avoir rempli. si on le met disabled dans WinProc
#ne le rempli pas !!!
 .about.fra41.tex56 configure -state disabled
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

proc vTclWindow.about {base} {
global STR
    if {$base == ""} {
        set base .about
    }
    if {[winfo exists $base]} {
        wm deiconify $base; return
    }
    ###################
    # CREATING WIDGETS
    ###################
    toplevel $base -class Toplevel
    wm focusmodel $base passive
    wm geometry $base 443x395+55+110
    wm maxsize $base 785 570
    wm minsize $base 1 1
    wm overrideredirect $base 0
    wm resizable $base 0 0
    wm deiconify $base
    wm title $base $STR(ndsabout)
    label $base.lab40 \
        -borderwidth 0 -image logo -relief raised
    frame $base.fra41 \
        -borderwidth 3 -height 324 -relief sunken -width 443
    label $base.fra41.lab44 \
        -anchor e -borderwidth 0 -font {Helvetica -12 normal} -text $STR(name)
    label $base.fra41.lab47 \
        -anchor e -borderwidth 0 -font {Helvetica -12 normal} -text $STR(author)
    label $base.fra41.lab48 \
        -anchor e -borderwidth 0 -font {Helvetica -12 normal} -text $STR(date)
    label $base.fra41.lab49 \
        -anchor e -borderwidth 0 -font {Helvetica -12 normal} -text $STR(license)
    label $base.fra41.lab51 \
        -font {Helvetica -12 normal} \
        -anchor w -borderwidth 0 -text $STR(version)
    label $base.fra41.lab52 \
        -anchor w -borderwidth 0 \
        -font {Helvetica -12 normal} \
        -text {Patrick Pollet <patrick.pollet@insa-lyon.fr>}
    label $base.fra41.lab53 \
          -font {Helvetica -12 normal} \
          -anchor w -borderwidth 0 -text $STR(date_version)
    label $base.fra41.lab54 \
         -font {Helvetica -12 normal} \
        -anchor w -borderwidth 0 -text $STR(gpl)
    text $base.fra41.tex56 \
	    -font {Helvetica -10 normal} \
        -highlightthickness 0 \
        -yscrollcommand {.about.fra41.scr59 set}
    button $base.fra41.but58 \
        -activeforeground blue -command { destroy .about } \
        -font  {Helvetica -12 normal} \
        -highlightthickness 0 -padx 9 -pady 3 -text $STR(ok)
    scrollbar $base.fra41.scr59 \
        -command {.about.fra41.tex56 yview} -highlightthickness 0 \
        -orient vert
    button $base.fra41.gpl \
          -font {Helvetica -12 normal} \
         -command {global SCRIPT_DIR; source [file join $SCRIPT_DIR license.tcl]} -padx 9 -pady 3 \
        -text $STR(see_gpl)
    ###################
    # SETTING GEOMETRY
    ###################
    pack $base.lab40 \
        -in .about -anchor center -expand 0 -fill none -side top
    pack $base.fra41 \
        -in .about -anchor center -expand 0 -fill both -side top
    place $base.fra41.lab44 \
        -x 10 -y 10 -width 77 -height 16 -anchor nw -bordermode ignore
    place $base.fra41.lab47 \
        -x 10 -y 30 -width 77 -height 16 -anchor nw -bordermode ignore
    place $base.fra41.lab48 \
        -x 10 -y 50 -width 77 -height 16 -anchor nw -bordermode ignore
    place $base.fra41.lab49 \
        -x 10 -y 70 -width 77 -height 16 -anchor nw -bordermode ignore
    place $base.fra41.lab51 \
        -x 95 -y 10 -width 337 -height 16 -anchor nw -bordermode ignore
    place $base.fra41.lab52 \
        -x 95 -y 30 -width 337 -height 16 -anchor nw -bordermode ignore
    place $base.fra41.lab53 \
        -x 95 -y 50 -width 337 -height 16 -anchor nw -bordermode ignore 
    place $base.fra41.lab54 \
        -x 95 -y 70 -width 112 -height 16 -anchor nw -bordermode ignore 
    place $base.fra41.tex56 \
        -x 15 -y 105 -width 395 -height 182 -anchor nw -bordermode ignore 
    place $base.fra41.but58 \
        -x 205 -y 290 -anchor nw -bordermode ignore 
    place $base.fra41.scr59 \
        -x 415 -y 104 -width 13 -height 184 -anchor nw -bordermode ignore 
    place $base.fra41.gpl \
        -x 290 -y 65 -anchor nw -bordermode ignore 
}

Window show .
Window show .about

main $argc $argv
