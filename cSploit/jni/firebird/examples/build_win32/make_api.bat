
call setenvvar.bat

:: CLEANING
call clean_api.bat

@echo.
@echo preprocessing api14.e
%ROOT_PATH%\bin\gpre -r -m -n -z %ROOT_PATH%\examples\api\api14.e %ROOT_PATH%\examples\api\api14.c -b localhost:%ROOT_PATH%\examples\empbuild\

cd %ROOT_PATH%\examples\api\
:: OLD FLAGS (to analyze)
:: /c /AL /Ge /Zi /Mq /Od /G2 /Zp1 /W3
set CLFLAGS=-I %ROOT_PATH%\include -I %ROOT_PATH%\examples\include 

cl %CLFLAGS% %ROOT_PATH%\lib\gds32_ms.lib %ROOT_PATH%\examples\api\api1.c
cl %CLFLAGS% %ROOT_PATH%\lib\gds32_ms.lib %ROOT_PATH%\examples\api\api2.c
cl %CLFLAGS% %ROOT_PATH%\lib\gds32_ms.lib %ROOT_PATH%\examples\api\api3.c
cl %CLFLAGS% %ROOT_PATH%\lib\gds32_ms.lib %ROOT_PATH%\examples\api\api4.c
cl %CLFLAGS% %ROOT_PATH%\lib\gds32_ms.lib %ROOT_PATH%\examples\api\api5.c
cl %CLFLAGS% %ROOT_PATH%\lib\gds32_ms.lib %ROOT_PATH%\examples\api\api6.c
cl %CLFLAGS% %ROOT_PATH%\lib\gds32_ms.lib %ROOT_PATH%\examples\api\api7.c
cl %CLFLAGS% %ROOT_PATH%\lib\gds32_ms.lib %ROOT_PATH%\examples\api\api8.c
cl %CLFLAGS% %ROOT_PATH%\lib\gds32_ms.lib %ROOT_PATH%\examples\api\api9.c
:: TODO : api9f
cl %CLFLAGS% %ROOT_PATH%\lib\gds32_ms.lib %ROOT_PATH%\examples\api\api10.c
cl %CLFLAGS% %ROOT_PATH%\lib\gds32_ms.lib %ROOT_PATH%\examples\api\api11.c
cl %CLFLAGS% %ROOT_PATH%\lib\gds32_ms.lib %ROOT_PATH%\examples\api\api12.c
cl %CLFLAGS% %ROOT_PATH%\lib\gds32_ms.lib %ROOT_PATH%\examples\api\api13.c
cl %CLFLAGS% %ROOT_PATH%\lib\gds32_ms.lib %ROOT_PATH%\examples\api\api14.c
cl %CLFLAGS% %ROOT_PATH%\lib\gds32_ms.lib %ROOT_PATH%\examples\api\api15.c
cl %CLFLAGS% %ROOT_PATH%\lib\gds32_ms.lib %ROOT_PATH%\examples\api\api16.c
:: TODO : winevent
cl %CLFLAGS% %ROOT_PATH%\lib\gds32_ms.lib %ROOT_PATH%\examples\api\api16t.c
cl %CLFLAGS% %ROOT_PATH%\lib\gds32_ms.lib %ROOT_PATH%\examples\api\apifull.c

cd %ROOT_PATH%\examples\build_win32\
