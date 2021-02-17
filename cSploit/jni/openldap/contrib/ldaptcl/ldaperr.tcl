#
# ldaperr.tcl: scan ldap.h for error return codes for initializing
# errorCode table.
#

proc genstrings {path} {
    set fp [open $path]
    while {[gets $fp line] != -1 &&
	![string match "#define LDAP_SUCCESS*" $line]} { }
    puts "/* This file automatically generated, hand edit at your own risk! */"
    puts -nonewline "char *ldaptclerrorcode\[\] = {
	NULL"
    while {[gets $fp line] != -1} {
	if {[clength $line] == 0 || [ctype space $line]} continue
	if {[string match *typedef* $line]} break
	if {![string match #define* $line]} continue
	if {![string match "#define LDAP_*" $line]} continue
	if {[string match "*LDAP_RANGE*" $line]} continue
	if {[string match "*LDAP_API_RESULT*" $line]} continue
	if {[string match {*\\} $line]} {
	    append line [gets $fp]
	}
	lassign $line define macro value
	set ldap_errcode($macro) $value
    }
    #parray ldap_errcode
    foreach i [array names ldap_errcode] {
	set value $ldap_errcode($i)
	#puts stderr "checking $value"
	if [regexp {^[A-Z_]} $value] {
	    if [info exists ldap_errcode($value)] {
		set value $ldap_errcode($value)
		set ldap_errcode($i) $value
	    }
	}
	set ldap_errname($value) $i
    }
    set lasterr 0
    foreach value [lsort -integer [array names ldap_errname]] {
	incr lasterr
	while {$lasterr < $value} {
	    puts -nonewline ",\n\tNULL"
	    incr lasterr
	}
	puts -nonewline ",\n\t\"$ldap_errname($value)\""
    }
    puts "\n};"
    puts "#define LDAPTCL_MAXERR\t$value"
}

#cmdtrace on
if !$tcl_interactive {
    genstrings [lindex $argv 0]
}
