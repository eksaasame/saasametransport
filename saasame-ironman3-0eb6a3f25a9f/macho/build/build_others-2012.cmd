set BUILDOTHERCD=%cd%
cd /d %~dp0

call "setenvmsvc2012.cmd"

call ..\others\makebuild_2012.cmd release win32 
call ..\others\makebuild_2012.cmd release-static win32 
call ..\others\makebuild_2012.cmd debug win32 
call ..\others\makebuild_2012.cmd debug-static win32 

call ..\others\makebuild_2012.cmd release x64 
call ..\others\makebuild_2012.cmd release-static x64 
call ..\others\makebuild_2012.cmd debug x64 
call ..\others\makebuild_2012.cmd debug-static x64

cd %BUILDOTHERCD%