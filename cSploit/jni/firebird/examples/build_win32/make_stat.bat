
call setenvvar.bat

:: CLEANING
@call clean_stat.bat

@echo.
@echo preprocessing stat*.e
%ROOT_PATH%\bin\gpre -c -m -n -z %ROOT_PATH%\examples\stat\stat1.e %ROOT_PATH%\examples\stat\stat1.c -d localhost:%ROOT_PATH%\examples\empbuild\employee.fdb
%ROOT_PATH%\bin\gpre -c -m -n -z %ROOT_PATH%\examples\stat\stat2.e %ROOT_PATH%\examples\stat\stat2.c -d localhost:%ROOT_PATH%\examples\empbuild\employee.fdb
%ROOT_PATH%\bin\gpre -c -m -n -z %ROOT_PATH%\examples\stat\stat3.e %ROOT_PATH%\examples\stat\stat3.c -d localhost:%ROOT_PATH%\examples\empbuild\employee.fdb
%ROOT_PATH%\bin\gpre -c -m -n -z %ROOT_PATH%\examples\stat\stat4.e %ROOT_PATH%\examples\stat\stat4.c -d localhost:%ROOT_PATH%\examples\empbuild\employee.fdb
%ROOT_PATH%\bin\gpre -c -m -n -z %ROOT_PATH%\examples\stat\stat5.e %ROOT_PATH%\examples\stat\stat5.c -d localhost:%ROOT_PATH%\examples\empbuild\employee.fdb
:: TODO
:: %ROOT_PATH%\bin\gpre -c -m -n -z %ROOT_PATH%\examples\stat\stat6.e %ROOT_PATH%\examples\stat\stat6.c -d localhost:%ROOT_PATH%\examples\empbuild\intlemp.fdb
%ROOT_PATH%\bin\gpre -c -m -n -z %ROOT_PATH%\examples\stat\stat7.e %ROOT_PATH%\examples\stat\stat7.c -d localhost:%ROOT_PATH%\examples\empbuild\employee.fdb
%ROOT_PATH%\bin\gpre -c -m -n -z %ROOT_PATH%\examples\stat\stat8.e %ROOT_PATH%\examples\stat\stat8.c -d localhost:%ROOT_PATH%\examples\empbuild\employee.fdb
%ROOT_PATH%\bin\gpre -c -m -n -z %ROOT_PATH%\examples\stat\stat9.e %ROOT_PATH%\examples\stat\stat9.c -d localhost:%ROOT_PATH%\examples\empbuild\employee.fdb
%ROOT_PATH%\bin\gpre -c -m -n -z %ROOT_PATH%\examples\stat\stat10.e %ROOT_PATH%\examples\stat\stat10.c -b localhost:%DB_PATH%/examples/empbuild/
%ROOT_PATH%\bin\gpre -c -m -n -z %ROOT_PATH%\examples\stat\stat11.e %ROOT_PATH%\examples\stat\stat11.c -b localhost:%DB_PATH%/examples/empbuild/
%ROOT_PATH%\bin\gpre -c -m -n -z %ROOT_PATH%\examples\stat\stat12.e %ROOT_PATH%\examples\stat\stat12.c -b localhost:%DB_PATH%/examples/empbuild/
%ROOT_PATH%\bin\gpre -c -m -n -z %ROOT_PATH%\examples\stat\stat12t.e %ROOT_PATH%\examples\stat\stat12t.c -b localhost:%DB_PATH%/examples/empbuild/

cd %ROOT_PATH%\examples\stat\
set CLFLAGS=-I %ROOT_PATH%\include -I %ROOT_PATH%\examples\include 
cl %CLFLAGS% %ROOT_PATH%\lib\gds32_ms.lib %ROOT_PATH%\examples\stat\stat1.c 
cl %CLFLAGS% %ROOT_PATH%\lib\gds32_ms.lib %ROOT_PATH%\examples\stat\stat2.c 
cl %CLFLAGS% %ROOT_PATH%\lib\gds32_ms.lib %ROOT_PATH%\examples\stat\stat3.c 
cl %CLFLAGS% %ROOT_PATH%\lib\gds32_ms.lib %ROOT_PATH%\examples\stat\stat4.c 
cl %CLFLAGS% %ROOT_PATH%\lib\gds32_ms.lib %ROOT_PATH%\examples\stat\stat5.c 
:: TODO
:: cl %CLFLAGS% %ROOT_PATH%\lib\gds32_ms.lib %ROOT_PATH%\examples\stat\stat6.c 
cl %CLFLAGS% %ROOT_PATH%\lib\gds32_ms.lib %ROOT_PATH%\examples\stat\stat7.c 
cl %CLFLAGS% %ROOT_PATH%\lib\gds32_ms.lib %ROOT_PATH%\examples\stat\stat8.c 
cl %CLFLAGS% %ROOT_PATH%\lib\gds32_ms.lib %ROOT_PATH%\examples\stat\stat9.c 
cl %CLFLAGS% %ROOT_PATH%\lib\gds32_ms.lib %ROOT_PATH%\examples\stat\stat10.c 
cl %CLFLAGS% %ROOT_PATH%\lib\gds32_ms.lib %ROOT_PATH%\examples\stat\stat11.c 
cl %CLFLAGS% %ROOT_PATH%\lib\gds32_ms.lib %ROOT_PATH%\examples\stat\stat12.c 
cl %CLFLAGS% %ROOT_PATH%\lib\gds32_ms.lib %ROOT_PATH%\examples\stat\stat12t.c 

cd %ROOT_PATH%\examples\build_win32
