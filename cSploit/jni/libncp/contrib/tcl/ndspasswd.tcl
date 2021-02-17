#!/usr/bin/tixwish
#############################################################################
# Visual Tcl v1.20 Project
#
# ndspassword.tcl
#  Graphical password changer for NDS tress.
#  User must be authenticated.
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
#        1.2   March        2001           Patrick Pollet
#	        added tree selection logic and "logout from all"
#        1.3  May 21 2002 Patrick Pollet
#               bug in reading password expiration date
#               fixed context problem 
#################################
# GLOBAL VARIABLES
#
global newpwd;
global newpwd2;
global oldpwd;
global pass_expiration;
global pass_minilength;
global pass_required;
global username
global context
global NDS;

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

proc {readtrees} {} {
global NDS
global tree
 set trees [NDS:listtrees "authen"]
 .top28.treelbx delete 0 end
 set trees [lsort $trees]
 foreach tree $trees {
    .top28.treelbx insert end $tree
 }
  .top28.treelbx selection set 0
  maj_whoami
}


proc maj_whoami {} {
global STR tree
global username
global context
    catch {.top28.treelbx curselection} num
    catch {.top28.treelbx get $num} tree
    catch {exec  ncpwhoami -fcU -s : -T $tree -1 } mes
    puts $mes
    set tmpl  [split $mes \n]
    set tmpl [split [lindex $tmpl 0] :]
    #puts $tmpl
    if {![ string match "*no ncp*" $tmpl]} {
        set username [lindex $tmpl 1]
        if {![string match {*\.*}  $username ]} {
                set context [lindex $tmpl 0]
        } else {
           set tmpl [split $username .]
           set username [lindex $tmpl 0]
           set context [join [lrange $tmpl 1 end] ':']
        }


    } else {
        .top28.lab35 configure -text $STR(not_logged_in)
        .top28.lab34 configure -text ""

    }
    getuserrestrictions $tree
}


proc {canchange} {} {
global newpwd;
global newpwd2;
global pass_minilength;
  if { $newpwd == $newpwd2 && [string length $newpwd] >= $pass_minilength } {
       .top28.but32 configure -state normal
  } else {
      .top28.but32 configure -state disabled
  }
}

proc {dochangepwd} {} {
global oldpwd;
global newpwd;
global newpwd2;
global pass_minilength;
global tree
global username
global context
global NDS STR

#currently chgpwd dose not use the -c context
#so we must send a DN user name
#not anymore in v 1.03 of chgpwd.c
# puts "exec chgpwd -T $tree -o $username -c $context -P $oldpwd -n $newpwd"
  if { $newpwd == $newpwd2 && [string length $newpwd]  >= $pass_minilength } {
      catch {exec chgpwd -T $tree -o $username -c$context -P $oldpwd -n $newpwd } status
      if {$status ==""} {
        NDS:dialog $STR(success_chg_pwd) Ok
        exit
      } else {
         NDS:dialog [join $status { }] Ok
      }
 }
}

proc getuserrestrictions { tree } {
global STR  NDS
global pass_minilength
global pass_required
global pass_expiration
global oldpwd
    set pass_minilength 0
    set pass_required $STR(no)
    set pass_expiration $STR(none)
    catch {exec ncpwhoami -fpT -1 -T $tree} res
    puts $res
    set tmpl [split $res ',']
    if {[lindex $tmpl 2]!="true"} {
       .top28.oldpwd  configure -show
       set oldpwd $STR(no_chg_pwd)
       .top28.oldpwd  configure -state disabled
       .top28.newpwd  configure -state disabled
       .top28.newpwd2  configure -state disabled
       return
    }
#change it to regular password entry line
  .top28.oldpwd  configure -show *
    if {[lindex $tmpl 0]=="true"} {
       set pass_required $STR(yes)
       set pass_minilength 4
    }
    if {[lindex $tmpl 3]!="***"} {
       set pass_minilength [lindex $tmpl 3]
    }
    if {[lindex $tmpl 4]!="***"} {
       set pass_expiration [lindex $tmpl 4]
    }
}

proc {main} {argc argv} {
global STR  NDS
global tree
global username
      readtrees
     if {$username !=""} {
        focus .top28.oldpwd
     } else {
       .top28.lab35 configure -text $STR(not_logged_in)
       .top28.but32 configure -state disabled
       .top28.oldpwd  configure -state disabled
       .top28.newpwd  configure -state disabled
       .top28.newpwd2  configure -state disabled
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
    toplevel $base -class Toplevel
    wm focusmodel $base passive
    wm geometry $base 445x311+141+173
    wm maxsize $base 785 570
    wm minsize $base 1 1
    wm overrideredirect $base 0
    wm resizable $base 0 0
    wm deiconify $base
    wm title $base $STR(ndspasswd)
    label $base.lab29 \
        -borderwidth 1 -font {Helvetica 12 normal} -justify right -text $STR(context)
    label $base.lab30 \
        -borderwidth 1 -font {Helvetica 12 normal} -justify right -text $STR(tree)
    label $base.lab31 \
        -borderwidth 1 -font {Helvetica 12 normal} -justify right -text $STR(user)
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
    button $base.but32 \
        -activeforeground #0000f7 -command { dochangepwd } \
        -font -Adobe-Helvetica-*-R-Normal--*-120-*-*-*-*-*-* -padx 9 -pady 3 \
        -state disabled -text $STR(change)
    label $base.lab34 \
	    -borderwidth 1 -justify left -relief sunken -text {}  -textvariable context
    label $base.lab35 \
	    -borderwidth 1 -justify left -relief sunken -text {}   -textvariable username
    label $base.lab36 \
        -image logo -relief sunken -text label
    button $base.but28 \
        -activeforeground #0000f7 -command exit \
        -font -Adobe-Helvetica-*-R-Normal--*-120-*-*-*-*-*-* -padx 9 -pady 3 \
        -text $STR(cancel)
    label $base.l1 \
        -borderwidth 1 -font {Helvetica 12 bold} -justify right \
        -text $STR(old_pwd)
    label $base.lab28 \
        -borderwidth 1 -font {Helvetica 12 bold} -justify right \
        -text $STR(new_pwd)
    label $base.lab32 \
        -borderwidth 1 -font {Helvetica 12 bold} -justify right \
        -text $STR(new_pwd2)
    entry $base.oldpwd \
        -background #f8fefe -textvariable oldpwd
    entry $base.newpwd \
        -background #fefefe -show * -textvariable newpwd
    bind $base.newpwd <KeyRelease> {
        canchange
    }
    entry $base.newpwd2 \
        -background #fefefe -show * -textvariable newpwd2
    bind $base.newpwd2 <KeyRelease> {
        canchange
    }
    frame $base.infos \
        -borderwidth 2 -height 75 -relief groove -width 125
    label $base.infos.requislb \
        -borderwidth 1 -justify left -relief sunken -text $STR(none) \
        -textvariable pass_expiration
    label $base.infos.expirelbl \
        -borderwidth 1 -justify left -relief sunken -text $STR(no) \
        -textvariable pass_required
    label $base.infos.lenlbl \
        -borderwidth 1 -justify left -relief sunken -text 0 \
        -textvariable pass_minilength
    label $base.infos.lb1 \
        -borderwidth 1 -font {Helvetica -12 normal} -justify right\
        -text $STR(pwd_exp_date)
    label $base.infos.lb2 \
        -borderwidth 1 -font {Helvetica -12 normal}  -justify right\
        -text $STR(pwd_min_len)
    label $base.infos.lb3 \
        -borderwidth 1 -font {Helvetica -12 normal} -justify right -text $STR(pwd_required)

     bind $base.treelbx <ButtonRelease-1> {
        maj_whoami
    }
    ###################
    # SETTING GEOMETRY
    ###################
    place $base.lab29 \
        -x 260 -y 105 -anchor se -bordermode ignore
    place $base.lab30 \
        -x 40 -y 105 -anchor se -bordermode ignore
    place $base.lab31 \
        -x 260 -y 125  -anchor se -bordermode ignore
    place $base.treelbx \
        -x 50 -y 85 -width 140  -anchor nw -bordermode ignore
    place $base.scr18 \
        -x 190 -y 85 -width 16 -height 38 -anchor nw -bordermode ignore

    place $base.but32 \
        -x 100 -y 275 -width 98 -height 26 -anchor nw -bordermode ignore
    place $base.lab34 \
        -x 270 -y 85 -width 151 -height 18 -anchor nw -bordermode ignore
    place $base.lab35 \
        -x 270 -y 108 -width 151 -height 18 -anchor nw -bordermode ignore
    place $base.lab36 \
        -x 0 -y 0 -width 446 -height 78 -anchor nw -bordermode ignore
    place $base.but28 \
        -x 245 -y 275 -width 98 -height 26 -anchor nw -bordermode ignore
    place $base.l1 \
        -x 155 -y 200 -anchor se -bordermode ignore
    place $base.lab28 \
        -x 155 -y 230  -anchor se -bordermode ignore
    place $base.lab32 \
        -x 155 -y 255 -anchor se -bordermode ignore
    place $base.oldpwd \
        -x 160 -y 182 -width 268 -height 22 -anchor nw -bordermode ignore
    place $base.newpwd \
        -x 160 -y 209 -width 268 -height 22 -anchor nw -bordermode ignore
    place $base.newpwd2 \
        -x 160 -y 238 -width 268 -height 22 -anchor nw -bordermode ignore
    place $base.infos \
        -x 0 -y 135 -width 445 -height 40 -anchor nw -bordermode ignore
    place $base.infos.requislb \
        -x 71 -y 10 -width 161 -height 18 -anchor nw -bordermode ignore
    place $base.infos.expirelbl \
        -x 400 -y 10 -width 31 -height 18 -anchor nw -bordermode ignore
    place $base.infos.lenlbl \
        -x 320 -y 10 -width 26 -height 18 -anchor nw -bordermode ignore
    place $base.infos.lb1 \
        -x 70 -y 28 -anchor se -bordermode ignore
    place $base.infos.lb2 \
        -x 325 -y 28 -anchor se -bordermode ignore
    place $base.infos.lb3 \
        -x 400 -y 28 -anchor se -bordermode ignore
}

Window show .
Window show .top28

main $argc $argv
