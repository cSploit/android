#!/usr/bin/tixwish
#############################################################################
# Visual Tcl v1.22 Project
#
#
# ndsmount.tcl - mount Netware volumes from tree(s) where current user is
#     authenticated.
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
#        1.2   May 2002                          Patrick Pollet
#		 added -l parameter to all ncplogin/ncpmap calls
#		 to force local mounts in the case of NFS mounted homes
#		 if mount_locally is set in /etc/ndsclient.conf




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
                    set env(NDSCLIENT_HOME) [file join [pwd] [file dirname  [file join [lrange [file split $home] 1 end]]]]
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
}

init $argc $argv

proc upOneLevel {} {
global context
          set dot [string first "." $context]
        if { $dot == "-1" } {
                readTrees
        } else {
                set context [string range $context [ expr $dot +1] [expr [string length $context]-1]]
                readContexts
                readVolumes
        }
}


proc {chgContext} { nctx} {
global context
        if {$nctx != ".."} {
                if {$context !=""} {
                        set context $nctx.$context
                } else {
                        set context $nctx
                }
               readContexts
               readVolumes
        } else {
                upOneLevel
        }
}

proc readTrees {} {
global context
global lastctx
        set trees [NDS:listtrees "authen"]
         .top28.mount.volctx delete 0 end
        set trees [lsort $trees]
        foreach tree $trees {
                .top28.mount.volctx insert end $tree
         }
        .top28.mount.volctx selection set 0
        set context ""
        set lastctx "9999"
        majBtnMount
        majWhoami
}


proc readContexts {} {
global context
global lastctx

        set dot [string last "." $context]
        if { $dot == "-1" } {
                catch {exec ncplist -T $context -v 4 -Q -l "Org*" } ctxs
        } else {
                set nctx [string range $context 0 [ expr $dot -1]]
                set tree [string range $context [expr $dot +1] [expr [string length $context]-1]]
                catch {exec ncplist -T $tree  -v 4 -Q -A -o $nctx -c $nctx -l "Org*" } ctxs
        }
        set lastctx [llength $ctxs]
        .top28.mount.volctx delete 0 end
        .top28.mount.volctx insert end ".."
        foreach ctx $ctxs {
                .top28.mount.volctx insert end $ctx
        }
}


proc readVolumes {} {
global context
        set dot [string last "." $context]
        set nctx [string range $context 0 [ expr $dot -1]]
        set tree [string range $context [expr $dot +1] [expr [string length $context]-1]]
        if { $nctx !="" } {
                catch {exec ncplist -T $tree  -v 4 -Q -A -o $nctx -c $nctx -l "Vol*" } vlist2
                set vlist2 [lsort $vlist2]
                foreach vol $vlist2 {
                        .top28.mount.volctx insert end $vol
                }
        }
}


proc doSelect {} {
global lastctx
global context

        catch {.top28.mount.volctx curselection} num
        if { $num != "-1" } {
                catch {.top28.mount.volctx get $num} sel
                if { $sel == ".." } {
                        upOneLevel
                } else {
                        if { $num <= $lastctx } {
                                chgContext $sel
                        } else {
                                #puts $sel
                                doMount $sel
                        }
                }
        }
}


proc doMount {volume} {
global context
global mount_locally



   set dot [string last "." $context]
   set nctx [string range $context 0 [ expr $dot -1]]
   set tree [string range $context [expr $dot +1] [expr [string length $context]-1]]
   puts "mounting $volume in context $nctx of tree $tree"
   if ($mount_locally) {
      catch { exec ncpmap -T $tree -X $nctx -V $volume -a -E -l} result
   } else {
      catch { exec ncpmap -T $tree -X $nctx -V $volume -a -E} result
   }


   #puts $result
   if [regexp "(.*)mounted(.*):(.*)$" $result aux1 aux2 aux3 path] {
        #puts $path
        refreshMounted
        #KDE 2
        exec kfmclient openURL $path
        return 1
      }
   NDS:dialog $result
   return 0
}


proc doUmount {} {
global STR
 catch {.top28.f2.alreadyLbx curselection } {tmp}
 if {$tmp !=""} {
     catch {.top28.f2.alreadyLbx get $tmp} tmp1
     set tmp2 [split  $tmp1 :]
#     puts $tmp1
     set item [lindex $tmp2 0]
     catch {exec ncpumount $item} status
     if {$status !=""} {
       NDS:dialog [join $status {-}] OK
     }
     refreshMounted
  } else {
    NDS:dialog $STR(nothing)
  }
}

proc majBtnMount {} {
global lastctx
        catch {.top28.mount.volctx curselection} num
        if { $num != "-1"  & $num > $lastctx } {
                .top28.mount.but30 configure -state normal
        } else {
                .top28.mount.but30 configure -state disabled
        }
}


proc majWhoami {} {
global context
#only if at toplevel
        if { $context == "" } {
	        catch {.top28.mount.volctx curselection} num
		if { $num != "-1" } {
		        catch { .top28.mount.volctx get $num} tree
                        catch {exec  ncpwhoami -fTU -s : -T $tree -1} mes
                        #puts $mes
	        	if {![ string match "*no ncp*" $mes]} {
                                set tmpl  [split $mes \n]
                                set tmpl [split [lindex $tmpl 0] :]
                                #puts $tmpl
                                .top28.lab35 configure -text [lindex $tmpl 1]
                        }
		}
        }

}



proc refreshMounted {} {

	catch {exec  ncpwhoami -fMSV -s :} slist
        .top28.f2.alreadyLbx  delete 0 end
        set cnt 0
        if {! [ string match "*no ncp*" $slist]} {
                foreach mnt $slist {
                        .top28.f2.alreadyLbx  insert end $mnt
                        set cnt [expr $cnt +1 ]
                }
        }
        if {$cnt !="0" } {
                .top28.f2.dismount configure -state normal
        } else {
                .top28.f2.dismount configure -state disabled
        }
}




proc {main} {argc argv} {
global STR NDS
global mount_locally; # from /etc/ndsclient.conf

        .top28.lab35 configure -text $STR(not_logged_in)
        NDS:init
        if {$NDS(username) !=""} {
                readTrees
                refreshMounted
        } else {
                .top28.lab35 configure -text $STR(not_logged_in)
                .top28.mount.but30 configure -state disabled
                .top28.f2.dismount configure -state disabled
   }
   # get global preferences
         source /etc/ndsclient.conf
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

proc vTclWindow. {base {container 0}} {
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
    wm geometry $base 450x400+148+95
    wm maxsize $base 785 570
    wm minsize $base 1 1
    wm overrideredirect $base 0
    wm resizable $base 0 0
    wm deiconify $base
    wm title $base  $STR(ndsmount)
    }
    label $base.lab31 \
        -background #dcdcdc -borderwidth 1 -font {Helvetica 12 normal} \
        -foreground #000000 -highlightbackground #dcdcdc \
        -highlightcolor #000000 -justify left -text $STR(loginname)
    label $base.lab35 \
        -background #dcdcdc -borderwidth 1 -foreground #000000 \
        -highlightbackground #dcdcdc -highlightcolor #000000 -justify right \
        -relief sunken -text {}
    label $base.lab36 \
        -background #dcdcdc -foreground #000000 -highlightbackground #dcdcdc \
        -highlightcolor #000000 -image logo -relief sunken -justify left -text label
    button $base.but28 \
        -activebackground #dcdcdc -activeforeground #0000f7 \
        -background #dcdcdc -command exit \
        -font -Adobe-Helvetica-*-R-Normal--*-120-*-*-*-*-*-* \
        -foreground #000000 -highlightbackground #dcdcdc \
        -highlightcolor #000000 -padx 9 -pady 3 -text $STR(close)
    frame $base.mount \
        -background #dcdcdc -borderwidth 2 -height 75 \
        -highlightbackground #dcdcdc -highlightcolor #000000 -relief groove \
        -width 125
    listbox $base.mount.volctx \
        -background #dcdcdc -font {Helvetica -12 normal} -foreground #000000 \
        -highlightbackground #ffffff -highlightcolor #000000 \
        -selectbackground #547098 -selectforeground #ffffff \
        -yscrollcommand "$base.mount.serversscroll set"

    bind $base.mount.volctx <Double-Button-1> {
        doSelect
    }
    bind $base.mount.volctx <ButtonRelease-1> {
        majBtnMount
	majWhoami

    }
    scrollbar $base.mount.serversscroll \
        -activebackground #dcdcdc -background #dcdcdc \
        -command "$base.mount.volctx yview" -cursor left_ptr \
        -highlightbackground #dcdcdc -highlightcolor #000000 -orient vert \
        -troughcolor #dcdcdc -width 10
    button $base.mount.but30 \
        -activebackground #dcdcdc -activeforeground #0000fe \
        -background #dcdcdc -command doSelect \
        -font -Adobe-Helvetica-*-R-Normal--*-120-*-*-*-*-*-* \
        -foreground #000000 -highlightbackground #dcdcdc \
        -highlightcolor #000000 -padx 9 -pady 3 -text $STR(mount)
    label $base.mount.lab17 \
        -background #dcdcdc -borderwidth 1 \
        -foreground #000000 -font {Helvetica -12 normal}\
        -highlightbackground #dcdcdc -highlightcolor #000000 \
        -text $STR(nds_volumes)
    frame $base.f2 \
        -background #dcdcdc -borderwidth 2 -height 75 \
        -highlightbackground #dcdcdc -highlightcolor #000000 -relief groove \
        -width 125
    listbox $base.f2.alreadyLbx  -font {Helvetica 12 normal}\
        -background #dcdcdc -foreground #000000 -height 5 \
        -highlightbackground #ffffff -highlightcolor #000000 \
        -selectbackground #547098 -selectforeground #ffffff \
        -yscrollcommand "$base.f2.sb2 set"
    button $base.f2.dismount \
        -activebackground #dcdcdc -activeforeground #000000 \
        -background #dcdcdc -command doUmount -foreground #000000 \
        -highlightbackground #dcdcdc -highlightcolor #000000 -padx 9 -pady 3 \
        -text $STR(umount)
    scrollbar $base.f2.sb2 \
        -activebackground #dcdcdc -background #dcdcdc \
        -command "$base.f2.alreadyLbx yview" -cursor left_ptr \
        -highlightbackground #dcdcdc -highlightcolor #000000 -orient vert \
        -troughcolor #dcdcdc -width 10
    label $base.f2.lb3 \
        -background #dcdcdc -borderwidth 1 -font {Helvetica -12 normal} \
        -foreground #000000 -highlightbackground #dcdcdc \
        -highlightcolor #000000 -justify left -text $STR(already_mounted)
    ###################
    # SETTING GEOMETRY
    ###################
    place $base.lab31 \
        -x 55 -y 105 -anchor se -bordermode ignore
    place $base.lab35 \
        -x 80 -y 85 -width 291 -height 18 -bordermode ignore
    place $base.lab36 \
        -x 0 -y 0 -width 446 -height 78 -anchor nw -bordermode ignore
    place $base.but28 \
        -x 175 -y 370 -width 98 -height 26 -anchor nw -bordermode ignore
    place $base.mount \
        -x 5 -y 115 -width 440 -height 130 -anchor nw -bordermode ignore
    place $base.mount.volctx \
        -x 5 -y 20 -width 258 -height 101 -anchor nw -bordermode ignore
    place $base.mount.serversscroll \
        -x 265 -y 20 -width 21 -height 97 -anchor nw -bordermode ignore
    place $base.mount.but30 \
        -x 335 -y 50 -width 85 -height 26 -anchor nw -bordermode ignore
    place $base.mount.lab17 \
        -x 7 -y 2 -width 150 -height 18 -anchor nw -bordermode ignore
    place $base.f2 \
        -x 5 -y 255 -width 440 -height 105 -anchor nw -bordermode ignore
    place $base.f2.alreadyLbx \
        -x 5 -y 20 -width 263 -height 76 -anchor nw -bordermode ignore
    place $base.f2.dismount \
        -x 340 -y 35 -width 82 -height 26 -anchor nw -bordermode ignore
    place $base.f2.sb2 \
        -x 270 -y 20 -width 21 -height 77 -anchor nw -bordermode ignore
    place $base.f2.lb3 \
        -x 7 -y 2 -width 126 -height 18 -anchor nw -bordermode ignore
}

Window show .
Window show .top28

main $argc $argv
