
cd /d %~dp0

call .\build\build_others-2012.cmd

call .\build\SetMachoEnv.cmd

cd %cd%