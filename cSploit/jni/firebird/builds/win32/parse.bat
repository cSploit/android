@echo off

@echo.

@call setenvvar.bat
@if errorlevel 1 (goto :END)

@call set_build_target.bat %*

@echo Generating parse.cpp and dsql.tab.h

@sed -n "/%%type .*/p" < %FB_ROOT_PATH%\src\dsql\parse.y > types.y
@sed "s/%%type .*//" < %FB_ROOT_PATH%\src\dsql\parse.y > y.y

%FB_ROOT_PATH%\temp\%FB_OBJ_DIR%\btyacc\btyacc -l -d -S %FB_ROOT_PATH%\src\dsql\btyacc_fb.ske y.y
@if errorlevel 1 (exit /B 1)
@copy y_tab.h %FB_ROOT_PATH%\src\include\gen\parse.h > nul
@copy y_tab.c %FB_ROOT_PATH%\src\dsql\parse.cpp > nul
@del y.y
@del types.y
@del y_tab.h
@del y_tab.c
@del sed*

:END
