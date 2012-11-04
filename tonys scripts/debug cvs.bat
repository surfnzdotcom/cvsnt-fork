@echo off
set TYPE=windebug

set BASE=/cygdrive/d/cvssrc/cvsnt
set BASE_WIN32=%1
set TARGET=/cygdrive/d/cvsbin
set TARGET_WIN32=d:/cvsbin

echo BASE_WIN32 = %BASE_WIN32%
if %BASE_WIN32%=="" goto one
for /f "usebackq delims=" %%a in (`cygpath %BASE_WIN32%`) do set BASE=%%a
:one
bash "%BASE%/tonys scripts/copy_common.sh"

