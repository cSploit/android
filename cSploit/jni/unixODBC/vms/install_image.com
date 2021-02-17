$
$ if p1 .eqs. "" then $goto ERR_NOPARAMS
$
$ install :== $install/command
$
$ if (f$file ("''p1'", "KNOWN"))
$ then install replace/open/header/shared 'p1'
$ else install create/open/header/shared 'p1'
$ endif
$done:
$ exit
$
$ERR_NOPARAMS:
$ write sys$output " "
$ write sys$output "The correct calling sequence is: "
$ write sys$output " "
$ write sys$output "$ @install_server p1
$ write sys$output " "
$ write sys$output "Where: "
$ write sys$output " "
$ write sys$output "    p1 = Image to be installed"
$ write sys$output " "
$ exit 44
$
