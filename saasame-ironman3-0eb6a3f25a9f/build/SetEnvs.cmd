Set PROJECTPATH="%~dp0.."
Set SetEvnCmd="%PROJECTPATH%\macho\build\SetEnv.exe" 

call %SetEvnCmd% THRIFT_EXE %PROJECTPATH%\thrift-0.10.0.exe
call %SetEvnCmd% BOOST_ROOT %PROJECTPATH%\macho\includes\boost-1_63
call %SetEvnCmd% LIBEVENT_ROOT %PROJECTPATH%\opensources\libevent-2.0.21-stable
call %SetEvnCmd% OPENSSL_ROOT_DIR %PROJECTPATH%\opensources\openssl-1.0.2j
call %SetEvnCmd% THRIFT_CPP %PROJECTPATH%\opensources\thrift-0.10.0\lib\cpp\src
