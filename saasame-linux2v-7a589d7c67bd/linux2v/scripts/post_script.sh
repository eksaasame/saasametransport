#!/bin/sh

POST_SCRIPT_PATH="/etc/runonce.d/run/run.sh"
if [ -f "$POST_SCRIPT_PATH" ]; then
    sleep 10s
    OUTPUT=`"$POST_SCRIPT_PATH"`
    echo -e "$OUTPUT" >> "/etc/runonce.d/callback.log" 
    mv "$POST_SCRIPT_PATH" "/etc/runonce.d/ran/run.sh.$(date +%Y%m%dT%H%M%S)"
fi