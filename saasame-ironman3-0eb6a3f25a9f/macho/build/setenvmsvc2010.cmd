@echo off
if not exist "%VS100COMNTOOLS%vsvars32.bat" goto error
call "%VS100COMNTOOLS%vsvars32.bat"
goto done

:error
echo *************************
echo * SETTINGS ERROR vcvars32.bat MSVC2010
echo *
echo *************************
echo Press Ctrl C to stop the build.
pause

:done


