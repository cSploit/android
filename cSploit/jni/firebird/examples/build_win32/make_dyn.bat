
call setenvvar.bat

:: CLEANING
@call clean_dyn.bat

@echo.
@echo preprocessing dyn*.e
%ROOT_PATH%\bin\gpre -r -m -n -z %ROOT_PATH%\examples\dyn\dyn1.e %ROOT_PATH%\examples\dyn\dyn1.c -b localhost:%ROOT_PATH%\examples\empbuild\
%ROOT_PATH%\bin\gpre -r -m -n -z %ROOT_PATH%\examples\dyn\dyn2.e %ROOT_PATH%\examples\dyn\dyn2.c -b localhost:%ROOT_PATH%\examples\empbuild\
%ROOT_PATH%\bin\gpre -r -m -n -z %ROOT_PATH%\examples\dyn\dyn3.e %ROOT_PATH%\examples\dyn\dyn3.c -b localhost:%ROOT_PATH%\examples\empbuild\
%ROOT_PATH%\bin\gpre -r -m -n -z %ROOT_PATH%\examples\dyn\dyn4.e %ROOT_PATH%\examples\dyn\dyn4.c -b localhost:%ROOT_PATH%\examples\empbuild\
%ROOT_PATH%\bin\gpre -r -m -n -z %ROOT_PATH%\examples\dyn\dyn5.e %ROOT_PATH%\examples\dyn\dyn5.c -b localhost:%ROOT_PATH%\examples\empbuild\
%ROOT_PATH%\bin\gpre -r -m -n -z %ROOT_PATH%\examples\dyn\dynfull.e %ROOT_PATH%\examples\dyn\dynfull.c -b localhost:%ROOT_PATH%\examples\empbuild\

cd %ROOT_PATH%\examples\dyn\
set CLFLAGS=-I %ROOT_PATH%\include -I %ROOT_PATH%\examples\include 
cl %CLFLAGS% %ROOT_PATH%\lib\gds32_ms.lib %ROOT_PATH%\examples\dyn\dyn1.c 
cl %CLFLAGS% %ROOT_PATH%\lib\gds32_ms.lib %ROOT_PATH%\examples\dyn\dyn2.c 
cl %CLFLAGS% %ROOT_PATH%\lib\gds32_ms.lib %ROOT_PATH%\examples\dyn\dyn3.c 
cl %CLFLAGS% %ROOT_PATH%\lib\gds32_ms.lib %ROOT_PATH%\examples\dyn\dyn4.c 
cl %CLFLAGS% %ROOT_PATH%\lib\gds32_ms.lib %ROOT_PATH%\examples\dyn\dyn5.c 
cl %CLFLAGS% %ROOT_PATH%\lib\gds32_ms.lib %ROOT_PATH%\examples\dyn\dynfull.c 
cd %ROOT_PATH%\examples\build_win32
