@echo off
IF %1~==~ GOTO none
call vsvars32.bat
set PREFIX=D:\cvsbin\RELEAS~1\%1\libxml\win32\Release
set XINCLUDE="d:\cvsdeps\openssl\include;d:\cvsdeps\iconv\include;D:\cvsbin\RELEAS~1\%1\zlib"
set XLIB="d:\cvsdeps\openssl\lib;d:\cvsdeps\iconv\lib;D:\cvsbin\RELEAS~1\%1\zlib\win32\release"
echo "Set working directory"
cd D:\cvsbin\release builder\
cd %1
cd libxml\win32
echo "Working directory is set - now configure"
cscript configure.js prefix=%PREFIX% include=%XINCLUDE% lib=%X	LIB%
echo "configure completed, will now try make"
pause
nmake /f Makefile.msvc
pause

exit /b
:none
echo You must specify a directory name (eg: cvsnt-2504-feb-08)
echo Usage: libxml2_configure.bat cvsnt-2504-feb-08
pause
exit /b
