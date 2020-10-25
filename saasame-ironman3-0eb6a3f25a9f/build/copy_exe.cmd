@echo ON

set BLD_NO=0

if "%1"=="" (
echo.
echo.
echo Enter Build Number
echo.
echo.
set /p BLD_NO=Enter Build Number:

) else (
if not "%1"=="" set BLD_NO=%1
)

set BLD_PLATFORM=x64
set BLD_CONFIG=release

copy /y ..\%BLD_CONFIG%\*.dll "..\output\%BLD_NO%\%BLD_PLATFORM%_%BLD_CONFIG%\*.*"
copy /y ..\%BLD_CONFIG%\*.pdb "..\output\%BLD_NO%\Symbols\%BLD_PLATFORM%_%BLD_CONFIG%\*.*"
copy /y ..\%BLD_CONFIG%\*.exe "..\output\%BLD_NO%\%BLD_PLATFORM%_%BLD_CONFIG%\*.*"

copy /y ..\%BLD_PLATFORM%\%BLD_CONFIG%\*.pdb "..\output\%BLD_NO%\Symbols\%BLD_PLATFORM%_%BLD_CONFIG%\*.*"
copy /y ..\%BLD_PLATFORM%\%BLD_CONFIG%\*.dll "..\output\%BLD_NO%\%BLD_PLATFORM%_%BLD_CONFIG%\*.*"
copy /y ..\%BLD_PLATFORM%\%BLD_CONFIG%\*.exe "..\output\%BLD_NO%\%BLD_PLATFORM%_%BLD_CONFIG%\*.*"

set BLD_CONFIG=release-static

copy /y ..\%BLD_CONFIG%\*.dll "..\output\%BLD_NO%\%BLD_PLATFORM%_%BLD_CONFIG%\*.*"
copy /y ..\%BLD_CONFIG%\*.pdb "..\output\%BLD_NO%\Symbols\%BLD_PLATFORM%_%BLD_CONFIG%\*.*"
copy /y ..\%BLD_CONFIG%\*.exe "..\output\%BLD_NO%\%BLD_PLATFORM%_%BLD_CONFIG%\*.*"

copy /y ..\%BLD_PLATFORM%\%BLD_CONFIG%\*.pdb "..\output\%BLD_NO%\Symbols\%BLD_PLATFORM%_%BLD_CONFIG%\*.*"
copy /y ..\%BLD_PLATFORM%\%BLD_CONFIG%\*.dll "..\output\%BLD_NO%\%BLD_PLATFORM%_%BLD_CONFIG%\*.*"
copy /y ..\%BLD_PLATFORM%\%BLD_CONFIG%\*.exe "..\output\%BLD_NO%\%BLD_PLATFORM%_%BLD_CONFIG%\*.*"

set BLD_CONFIG=release-static-2k3

copy /y ..\%BLD_CONFIG%\*.dll "..\output\%BLD_NO%\%BLD_PLATFORM%_%BLD_CONFIG%\*.*"
copy /y ..\%BLD_CONFIG%\*.pdb "..\output\%BLD_NO%\Symbols\%BLD_PLATFORM%_%BLD_CONFIG%\*.*"
copy /y ..\%BLD_CONFIG%\*.exe "..\output\%BLD_NO%\%BLD_PLATFORM%_%BLD_CONFIG%\*.*"

copy /y ..\%BLD_PLATFORM%\%BLD_CONFIG%\*.pdb "..\output\%BLD_NO%\Symbols\%BLD_PLATFORM%_%BLD_CONFIG%\*.*"
copy /y ..\%BLD_PLATFORM%\%BLD_CONFIG%\*.dll "..\output\%BLD_NO%\%BLD_PLATFORM%_%BLD_CONFIG%\*.*"
copy /y ..\%BLD_PLATFORM%\%BLD_CONFIG%\*.exe "..\output\%BLD_NO%\%BLD_PLATFORM%_%BLD_CONFIG%\*.*"
