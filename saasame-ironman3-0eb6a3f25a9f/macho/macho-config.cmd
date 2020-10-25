
cd /d %~dp0

call .\build\build_others.cmd

call .\build\SetMachoEnv.cmd

cd %cd%