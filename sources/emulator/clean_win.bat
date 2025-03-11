@echo off
nmake 1>NUL 2>NUL
if ERRORLEVEL 1 goto mingw
nmake -f makefile.win clean
goto common_end
:mingw
nmake 1>NUL 2>NUL
if ERRORLEVEL 1 goto no_make
mingw32-make -f makefile.mgw clean
:no_make
echo ERROR: cannot find make utility, please check the paths
:common_end

