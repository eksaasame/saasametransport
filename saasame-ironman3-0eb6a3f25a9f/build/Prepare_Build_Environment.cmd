@echo ON
Set PROJECT_PATH=%~dp0..

call "%PROJECT_PATH%\macho\macho-config-2013.cmd"

if errorlevel == 1 goto error

call "%PROJECT_PATH%\Build\Compile-Thrift.cmd"

if errorlevel == 1 goto error

goto cp

:error
echo "Skip compile"

goto end

:cp
REM call "%PROJECT_PATH%\macho\libs\copy_thrift_libs.cmd"

:end