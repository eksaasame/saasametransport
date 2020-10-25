set BUILDOTHERCD=%cd%
cd /d %~dp0

call "setenvmsvc2010.cmd"

call ..\others\makebuild.cmd release win32 
call ..\others\makebuild.cmd release-static win32 
call ..\others\makebuild.cmd debug win32 
call ..\others\makebuild.cmd debug-static win32 

call ..\others\makebuild.cmd release x64 
call ..\others\makebuild.cmd release-static x64 
call ..\others\makebuild.cmd debug x64 
call ..\others\makebuild.cmd debug-static x64

cd %BUILDOTHERCD%