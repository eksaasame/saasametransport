@echo off
echo logman stop "Trace_vcbt" -ets
logman stop "Trace_vcbt" -ets
echo logman delete "autosession\Trace_vcbt" -ets
logman delete "autosession\Trace_vcbt" -ets
