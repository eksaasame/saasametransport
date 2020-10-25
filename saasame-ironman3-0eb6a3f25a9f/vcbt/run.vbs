Set objShell = WScript.CreateObject("WScript.Shell")

objShell.run "cmd.exe /C "&objShell.CurrentDirectory&"\prepare_for_driver_test.cmd"
objShell.run "cmd.exe /C "&objShell.CurrentDirectory&"\auto_trace_start.cmd"