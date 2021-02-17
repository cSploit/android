#!/usr/bin/tixwish
#############################################################################
# Visual Tcl v1.20 Project
#
# ncpmount.tcl - mount Netware volumes from a list of servers.
#   user must have at least one ncpfs permanent connection.
#   mounting will be done in ~/ncp/SERVER/VOL directory (autocreated)
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
#                  (only root could use it !)
#        1.1   February     2001           Patrick Pollet
#                adapted for ncpfs NDS client
#        1.2   May          2002           Patrick Pollet
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
NDS:init
}

init $argc $argv

proc {dobrowse} {} {
#global mntpoint

}

proc {domount} {} {
#global mntpoint
global server
global volume
global username
global mount_locally

  puts "mounting $volume on server $server"
  if ($mount_locally) {
    catch {exec ncpmap -S $server -V $volume  -a -E -l} result
  } else {
    catch {exec ncpmap -S $server -V $volume  -a -E } result
  }
  puts $result
  if [regexp "(.*)mounted(.*):(.*)$" $result aux1 aux2 aux3 path] {
        #puts $path
        refresh_mounted
        #KDE 2
        exec kfmclient openURL $path
        return 1
      }
   NDS:dialog $result
   return 0
}


proc {doumount} {} {
global STR
 catch {.top28.f2.already curselection } {tmp}
 if {$tmp !=""} {
     catch {.top28.f2.already get $tmp} tmp1
     set tmp2 [split  $tmp1 :]
#     puts $tmp1
     set item [lindex $tmp2 0]
     catch [exec ncpumount $item] status
     if {$status !=""} {
       NDS:dialog [join $status {-}] OK
     }
     refresh_mounted
  } else {
    NDS:dialog $STR(nothing)
  }
}


proc {refreshservers} {} {
global server;

 catch {exec  slist2 } slist
 .top28.mount.serverslbx delete 0 end
 set slist [lsort $slist]
 foreach serv $slist {
    .top28.mount.serverslbx insert end $serv
 }
  .top28.mount.serverslbx selection set 0
  catch {.top28.mount.serverslbx get 0} {server}
  if {$server !=""} {
   .top28.mount.vlist configure -state active
    refreshvolumes
 }
}

proc {refreshvolumes} {} {
global server
global volume

 catch {exec vlist $server } vlist1
 set vlist1 [lsort $vlist1]
.top28.mount.volumeslbx delete 0 end
 foreach vol $vlist1 {
    .top28.mount.volumeslbx insert end $vol
  }
  .top28.mount.volumeslbx selection set 0
  catch {.top28.mount.volumeslbx get 0} volume
  majbtn_mount
}

proc {refresh_mounted} {} {
catch {exec  ncpwhoami -fMSV -s :} slist

 .top28.f2.already  delete 0 end
 set cnt 0
 if {! [ string match "*no ncp*" $slist]} {
   foreach mnt $slist {
      .top28.f2.already  insert end $mnt
      set cnt [expr $cnt +1 ]
   }
 }
 majbtn_umount {$cnt}
}

proc {selectserver} {} {
global server;
    catch {.top28.mount.serverslbx curselection} num
    if {$num !=""} {
        catch {.top28.mount.serverslbx get $num} server
        if {$server !=""} {
           .top28.mount.vlist  configure -state active
           refreshvolumes
        }
    }
}

proc {selectvolume} {} {
global volume;
    catch {.top28.mount.volumeslbx curselection} num
    if {$num !=""} {
        catch {.top28.mount.volumeslbx get $num} volume
        if {$volume !=""} {
           .top28.mount.vlist  configure -state active
        }
        majbtn_mount

    }
}

proc {majbtn_mount} {} {
#global mntpoint
global volume

    if {$volume !="" } {
        .top28.mount.but30 configure -state normal
   } else {
        .top28.mount.but30 configure -state disabled
   }
}

proc {majbtn_umount} { onoff } {
#global mntpoint
global volume

    if {$onoff !="0" } {
        .top28.f2.dismount configure -state normal
   } else {
        .top28.f2.dismount configure -state disabled
   }
}

proc {main} {argc argv} {
global server
global volume
global username
#global mntpoint
global mount_locally; # from /etc/ndsclient.conf
global STR NDS

  .top28.lab33 configure -text { }
  .top28.lab34 configure -text { }
  .top28.lab35 configure -text $STR(not_logged_in)
  .top28.lab33 configure -text $NDS(tree)
  .top28.lab34 configure -text $NDS(context)
  set username $NDS(username)
  set server ""
  set volume ""
  if {$username !=""} {
        .top28.lab35 configure -text $username
        refreshservers
        refresh_mounted
  }
  if {$server==""} {
       .top28.lab35 configure -text $STR(not_logged_in)
       .top28.mount.but30 configure -state disabled
        .top28.f2.dismount configure -state disabled
#       .top28.mount.mntedit  configure -state disabled
#       .top28.mount.browsebtn  configure -state disabled
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
    wm geometry $base 450x456+144+75
    wm maxsize $base 785 570
    wm minsize $base 1 1
    wm overrideredirect $base 0
    wm resizable $base 0 0
    wm deiconify $base
    wm title $base $STR(ndsmount)

    label $base.lab29 \
        -borderwidth 1 -font {Helvetica 12 normal} -justify right -text $STR(context)
    label $base.lab30 \
        -borderwidth 1 -font {Helvetica 12 normal} -justify right -text $STR(tree)
    label $base.lab31 \
        -borderwidth 1 -font {Helvetica 12 normal} -justify right -text $STR(user)
    label $base.lab33 \
        -borderwidth 1 -justify left -relief sunken -text { }
    label $base.lab34 \
        -borderwidth 1 -justify left -relief sunken -text { }
    label $base.lab35 \
        -borderwidth 1 -justify left -relief sunken \
        -text $STR(not_logged_in)
    label $base.lab36 \
        -image logo -relief sunken -text label
    button $base.but28 \
	    -activeforeground #0000f7 -command {exit} \
        -font -Adobe-Helvetica-*-R-Normal--*-120-*-*-*-*-*-* -padx 9 -pady 3 \
        -text $STR(close)
    frame $base.mount \
        -borderwidth 2 -height 75 -relief groove -width 125
    listbox $base.mount.serverslbx \
        -font {Helvetica -12 normal} \
        -yscrollcommand {.top28.mount.serversscroll set}

    bind $base.mount.serverslbx <Double-Button-1> {
        selectserver
    }
    listbox $base.mount.volumeslbx \
        -font {Helvetica -12 normal} \
        -yscrollcommand {.top28.mount.volumesscroll set}
    bind $base.mount.volumeslbx <ButtonRelease-1> {
        majbtn_mount
    }
     bind $base.mount.volumeslbx <Double-Button-1> {
        selectvolume
        domount
    }
    button $base.mount.vlist \
        -command {selectserver } -padx 9 -pady 3 -text >>

    scrollbar $base.mount.serversscroll \
        -command {.top28.mount.serverslbx yview} -orient vert
    scrollbar $base.mount.volumesscroll \
        -command {.top28.mount.volumeslbx yview} -orient vert
    button $base.mount.but30 \
        -activeforeground #0000fe \
          -command {domount} \
        -font -Adobe-Helvetica-*-R-Normal--*-120-*-*-*-*-*-* -padx 9 -pady 3 \
        -text $STR(mount)
    frame $base.f2 \
        -borderwidth 2 -height 75 -relief groove -width 125
    listbox $base.f2.already -font {Helvetica -12 normal}\
        -height 5   \
        -yscrollcommand {.top28.f2.sb2 set}
    button $base.f2.dismount \
           -command {doumount} \
        -padx 9 -pady 3 -text $STR(umount)  
    scrollbar $base.f2.sb2 \
        -command {.top28.f2.already yview} -orient vert
    label $base.f2.lb3 \
        -borderwidth 1 -font {Helvetica -12 normal} -justify left \
        -text $STR(already_mounted)
    ###################
    # SETTING GEOMETRY
    ###################
    place $base.lab29 \
        -x 280 -y 102 -anchor se -bordermode ignore
    place $base.lab30 \
        -x 100 -y 102 -anchor se -bordermode ignore
    place $base.lab31 \
        -x 100 -y 132 -anchor se -bordermode ignore
    place $base.lab33 \
        -x 105 -y 85 -width 106 -height 18 -anchor nw -bordermode ignore
    place $base.lab34 \
        -x 285 -y 85 -width 151 -height 18 -anchor nw -bordermode ignore
    place $base.lab35 \
        -x 105 -y 115 -width 331 -height 18 -anchor nw -bordermode ignore
    place $base.lab36 \
        -x 0 -y 0 -width 446 -height 78 -anchor nw -bordermode ignore
    place $base.but28 \
        -x 175 -y 425 -width 98 -height 26 -anchor nw -bordermode ignore
    place $base.mount \
        -x 5 -y 145 -width 440 -height 155 -anchor nw -bordermode ignore
    place $base.mount.serverslbx \
        -x 5 -y 5 -width 163 -height 106 -anchor nw -bordermode ignore
    place $base.mount.volumeslbx \
        -x 245 -y 5 -width 163 -height 106 -anchor nw -bordermode ignore
    place $base.mount.vlist \
        -x 200 -y 35 -width 37 -height 56 -anchor nw -bordermode ignore
#    place $base.mount.lb \
#        -x 10 -y 120 -anchor nw -bordermode ignore
#    place $base.mount.mntedit \
#        -x 120 -y 120 -width 128 -height 22 -anchor nw -bordermode ignore
#    place $base.mount.browsebtn \
#        -x 250 -y 120 -width 75 -height 26 -anchor nw -bordermode ignore
    place $base.mount.serversscroll \
        -x 170 -y 5 -width 21 -height 107 -anchor nw -bordermode ignore
    place $base.mount.volumesscroll \
        -x 410 -y 5 -width 21 -height 107 -anchor nw -bordermode ignore
#    place $base.mount.but28 \
#        -x 95 -y 330 -width 98 -height 26 -anchor nw -bordermode ignore
#    place $base.mount.but29 \
#        -x 95 -y 330 -width 98 -height 26 -anchor nw -bordermode ignore
    place $base.mount.but30 \
        -x 170 -y 120 -width 85 -height 26 -anchor nw -bordermode ignore
    place $base.f2 \
        -x 5 -y 310 -width 440 -height 105 -anchor nw -bordermode ignore
    place $base.f2.already \
        -x 5 -y 20 -width 298 -height 76 -anchor nw -bordermode ignore
    place $base.f2.dismount \
        -x 350 -y 35 -width 82 -height 26 -anchor nw -bordermode ignore
    place $base.f2.sb2 \
        -x 305 -y 20 -width 21 -height 77 -anchor nw -bordermode ignore
    place $base.f2.lb3 \
        -x 7 -y 2 -width 126 -height 18 -anchor nw -bordermode ignore
}

Window show .
Window show .top28

main $argc $argv
