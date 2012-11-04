call "vcvars32.bat"
call "c:\Program Files\Microsoft Platform SDK for Windows Server 2003 R2\SetEnv.Cmd" /XP64 /RETAIL
set CL=/DHACK_64BIT
start "" "devenv.exe" /useenv 
