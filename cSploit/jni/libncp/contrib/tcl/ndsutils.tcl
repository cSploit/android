# NDSutils library
#  common to all nds*.tcl applications
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

#        1.00  January      1999           Patrick Pollet
#                Initial version using Caldera NDS client
#        1.1   February     2001           Patrick Pollet
#                adapted for ncpfs NDS client
#        1.2   May          2002	   Patrick Pollet
#		 added support for the new -l option of ncplogin/ncpmap
#		 if option mount_locally is 1 in /etc/ndsclient.conf



proc NDS:init {{tree ""}} {
global env NDS

set NDS(username) {}
set NDS(tree) {}
set NDS(context) {}
set NDS(usernameDN) {}
set NDS(homepath) {}
set NDS(default_tree) {}
set NDS(default_context) {}
set NDS(logged) "false"
set NDS(homevolume) {}
set NDS(mnt_point) {}

#these three are possible env variables for the Caldera & ncpfs client
   if {$tree =="" } {
        if {[info exists env(NWCLIENT_PREFERRED_TREE)] } {
                set NDS(default_tree)    $env(NWCLIENT_PREFERRED_TREE)
        }
        if {[info exists env(NWCLIENT_DEFAULT_NAME_CONTEXT)]} {
                set NDS(default_context) $env(NWCLIENT_DEFAULT_NAME_CONTEXT)
        }
        if {[info exists env(NWCLIENT_DEFAULT_USER)]} {
                set NDS(username)       $env(NWCLIENT_DEFAULT_USER)
        }
        #this one is an extra by PP for a possible autologon
        if {[info exists env(NWCLIENT_DEFAULT_PASSWORD)]} {
                set NDS(password)    $env(NWCLIENT_DEFAULT_PASSWORD)
        }
    } else {
         set NDS(default_tree) $tree
    }
    NDS:whoami $tree
}


proc NDS:login {tree user password context} {
global  NDS
global mount_locally; # from /etc/ndsclient.conf
    puts "login on $tree as $user.$context"
    if {$context!="" } {
	if ($mount_locally) {
	    catch [ exec ncplogin -T $tree -U $user.$context -P $password -l]  status
	} else {
	    catch [ exec ncplogin -T $tree -U $user.$context -P $password]  status
	}
    } else {
	if ($mount_locally) {
	    catch [ exec ncplogin -T $tree -U $user  -P $password -l] status
	} else {
	    catch [ exec ncplogin -T $tree -U $user  -P $password] status
	}
    }
     puts $status
     if {[llength $status] ==0 } {
        NDS:whoami $tree
       return 1
     } else {
        NDS:dialog [join $status " " ] OK
        return 0
     }
}

proc NDS:login_nopwd {tree user {context ""} } {
global  NDS
global mount_locally; # from /etc/ndsclient.conf

# pb avec catch blocage si mot de passe requis et aucun fourni
 puts "login on $tree as $user.$context"
  if {$context !="" } {
  	if ($mount_locally) {
          	catch [ exec ncplogin -T $tree -U $user.$context -n -l] status
	} else {
          catch [ exec ncplogin -T $tree -U $user.$context -n ] status
	}
  } else {
	if ($mount_locally) {
	          catch [ exec ncplogin -T $tree -U $user -n -l] status
  } else {
          catch [ exec ncplogin -T $tree -U $user -n ] status
  }
  }
  puts $status
  if {[llength $status] ==0 } {
      NDS:whoami $tree
      return 1
  } else {
      NDS:dialog [join $status " " ] OK
      return 0
  }
}

proc NDS:logout { {tree ""} } {
global  NDS
    if { $NDS(logged)=="true"} {
	if {$tree==""} {
           catch [exec ncplogout -a]
        } else {
           catch [exec ncplogout -T $tree]
        }
        NDS:init
        set NDS(logged) "false"
       return 1
  } else {
       return 0
    }
}

proc NDS:whoami { {tree ""} } {
global NDS

    if {$tree == "" } {
       set tmp [exec ncpwhoami -fTcUMVS -s:]
    } else {
       set tmp [exec ncpwhoami -T $tree -fTcUMVS -s:]
    }
    #puts $tmp
    if {! [ string match "*no ncp*" $tmp]} {
        set tmpl1  [split $tmp \n]
        set tmpl [split [lindex $tmpl1 0] :]
        set NDS(tree) [lindex $tmpl 0]
        set NDS(context) [lindex $tmpl 1]
        set NDS(username) [lindex $tmpl 2]
        if {[string match  {*\.*} $NDS(username)]} {
                set NDS(usernameDN) $NDS(username)
        } else {
                set NDS(usernameDN) $NDS(username).$NDS(context)
        }
        set NDS(mnt_point) [lindex $tmpl 3]
        set NDS(volume)  [lindex $tmpl 4]
        set NDS(server)  [lindex $tmpl 5]
        set NDS(logged) "true"
#        NDS:showinfos
        return $tmpl
      } else {
         set NDS(username) {}
         set NDS(tree) {}
         set NDS(context) {}
         set NDS(usernameDN) {}
         set NDS(homepath) {}
         set NDS(logged) "false"
         set NDS(homevolume) {}
         set NDS(mnt_point) {}
         return {}
     }
}



proc NDS:showinfos {} {
global NDS
 foreach item [array names NDS *] {
   puts "set NDS($item) $NDS($item)"
 }
}


proc NDS:openhome { {tree} } {
global NDS
global mount_locally; # from /etc/ndsclient.conf

  if { $NDS(logged) == "true"} {
    set result [NDS:gethome $tree ]
    if { $result == "1" } {
    	if ($mount_locally) {
      		#catch {exec ncpmap -T $tree -V $NDS(homevol) -R $NDS(homepath) -X "\[Root\]" -a -E -l} result
		catch {exec ncpmap -T $tree -V $NDS(homevol) -R $NDS(homepath)  -a -E -l} result
	} else {
		#catch {exec ncpmap -T $tree -V $NDS(homevol) -R $NDS(homepath) -X "\[Root\]" -a -E} result
		catch {exec ncpmap -T $tree -V $NDS(homevol) -R $NDS(homepath) -a -E} result
	}
      puts $result
      if [regexp "(.*)mounted(.*):(.*)$" $result aux1 aux2 aux3 path] {
        puts $path
        #KDE 2
        exec kfmclient openURL $path
        return 1
      }
    }
  }

  return 0
}

proc NDS:gethome {{tree}} {
global NDS
 catch {exec ncpwhoami -T $tree -fh -1} tmp
 if { $tmp !=""  & ![string match "*failed*" $tmp] } {
   set tmpl [split $tmp ,]
   puts $tmp
   set NDS(homevol) [lindex $tmpl 0]
   set homepath [lindex $tmpl 3]
#dos antislash to unisx slashes
   set NDS(homepath) [join [split $homepath \\] /]
   return 1
 } else {
    puts "error: $tmp"
    return 0
 }

}

proc NDS:listtrees {{ authent ""}} {
  global NDS
  if {$authent ==""} {
   return [exec ncplisttrees]
  } else {
#due bug in NWDSScanConnsForTrees that add a non authenticated connexions
#    return [exec ncplisttrees -a]
     return [exec ncpwhoami -fT | uniq ]
  }
}

proc NDS:listcontexts { tree {context ""}} {
global NDS
    puts "List contexts on $tree :contexte is $context"
    if { $context !="" } {
      catch  {exec  ncplist -Q -A -T $tree -v 4 -o $context -c $context -l "Org*"} result
    } else {
       catch  {exec  ncplist -Q -A -T $tree -v 4 -Q "Org*"} result
    }
    puts $result
    return $result
}


proc NDS:dialog {mesg {options Ok}} {
    set x_mesg ""
    set base .ndsmsg
    set sw [winfo screenwidth .]
    set sh [winfo screenheight .]
    if {![winfo exists $base]} {
        toplevel $base -class Toplevel
        wm title $base "NDS client Message"
        wm transient $base .
        frame $base.f -bd 2 -relief groove
        label $base.f.t -bd 0 -relief flat -text $mesg -justify left
        frame $base.o -bd 1 -relief sunken
        foreach i $options {
            set n [string tolower $i]
            button $base.o.$n -text $i -width 5 \
            -command "
                set x_mesg $i
                destroy $base
            "
            pack $base.o.$n -side left -expand 1 -fill x
        }
        pack $base.f.t -side top -expand 1 -fill both -ipadx 5 -ipady 5
        pack $base.f -side top -expand 1 -fill both -padx 2 -pady 2
        pack $base.o -side top -fill x -padx 2 -pady 2
    }
    wm withdraw $base
    update idletasks
    set w [winfo reqwidth $base]
    set h [winfo reqheight $base]
    set x [expr ($sw - $w)/2]
    set y [expr ($sh - $h)/2]
    wm geometry $base +$x+$y
    wm deiconify $base
    grab $base
    tkwait window $base
    grab release $base
    return x_mesg
}


#NDS:dialog "hello" {Ok  Ko}










