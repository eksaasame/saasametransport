@echo ON
Set PROJECT_PATH=%~dp0..

call "%PROJECT_PATH%\macho\boost-config-2013.cmd"

if errorlevel == 1 goto error

goto end

:error
echo "Skip compile"

:end