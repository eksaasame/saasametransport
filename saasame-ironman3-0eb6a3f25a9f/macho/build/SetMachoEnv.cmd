Set PROJECTPATH="%~dp0.."

call %PROJECTPATH%\build\SetEnv.exe MACHOINCS  %PROJECTPATH%\INCLUDES
call %PROJECTPATH%\build\SetEnv.exe MACHOLIBS %PROJECTPATH%\LIBS