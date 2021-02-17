
call setenvvar.bat

@echo.
@echo cleaning api
:: CLEANING
del %ROOT_PATH%\examples\api\*.obj 2>nul
del %ROOT_PATH%\examples\api\*.exp 2>nul
del %ROOT_PATH%\examples\api\*.lib 2>nul
del %ROOT_PATH%\examples\api\*.exe 2>nul
del %ROOT_PATH%\examples\api\*.pdb 2>nul
del %ROOT_PATH%\examples\api\api14.c 2>nul
