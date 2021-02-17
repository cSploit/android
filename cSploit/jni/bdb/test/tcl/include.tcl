# Automatically built by dist/s_test; may require local editing.

set tclsh_path @TCL_TCLSH@
set tcllib .libs/libdb_tcl-@DB_VERSION_MAJOR@.@DB_VERSION_MINOR@@LIBTSO_MODSUFFIX@

set src_root @srcdir@/..
set test_path @srcdir@/../test/tcl
set je_root @srcdir@/../../je
set tcl_utils @srcdir@/../test/tcl_utils

global testdir
set testdir ./TESTDIR

global dict
global util_path

global is_aix_test
global is_freebsd_test
global is_hp_test
global is_linux_test
global is_osx_test
global is_qnx_test
global is_sunos_test
global is_windows_test
global is_windows9x_test

global valid_methods
global checking_valid_methods
global test_recopts

set KILL "@KILL@"
