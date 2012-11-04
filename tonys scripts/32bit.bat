call "vcvars32.bat"
set INCLUDE=%INCLUDE%;d:\cvsdeps\bonjour\include;d:\cvsdeps\howl\include;d:\cvsdeps\iconv\include;d:\cvsdeps\infozip;d:\cvsdeps\infozip\c-headers;d:\cvsdeps\libxml2\include;d:\cvsdeps\openssl\include;d:\cvsdeps\postgres\include;d:\cvsdeps\sqlite\include;d:\cvsdeps\u3 sdk\DAPI;c:\xeclient\oci\include
set LIB=%LIB%;d:\cvsdeps\bonjour\lib;d:\cvsdeps\howl\lib;d:\cvsdeps\iconv\lib;d:\cvsdeps\infozip;d:\cvsdeps\infozip\c-headers;d:\cvsdeps\libxml2\lib;d:\cvsdeps\openssl\lib;d:\cvsdeps\postgres\lib;d:\cvsdeps\sqlite\lib;d:\cvsdeps\u3 sdk\DAPI;c:\xeclient\oci\lib\msvc
call "c:\Program Files\Microsoft Platform SDK for Windows Server 2003 R2\SetEnv.Cmd" /XP32 /RETAIL
start "" "devenv.exe" /useenv
