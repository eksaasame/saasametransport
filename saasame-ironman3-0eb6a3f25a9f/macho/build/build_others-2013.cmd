set BUILDOTHERCD=%cd%
cd /d %~dp0

call "setenvmsvc2013.cmd"

call ..\others\makebuild_2013.cmd release win32 
call ..\others\makebuild_2013.cmd release-static win32 
call ..\others\makebuild_2013.cmd release-static-2k3 win32 
call ..\others\makebuild_2013.cmd debug win32 
call ..\others\makebuild_2013.cmd debug-static win32 

call ..\others\makebuild_2013.cmd release x64 
call ..\others\makebuild_2013.cmd release-static x64 
call ..\others\makebuild_2013.cmd release-static-2k3 x64 
call ..\others\makebuild_2013.cmd debug x64 
call ..\others\makebuild_2013.cmd debug-static x64

cd %BUILDOTHERCD%