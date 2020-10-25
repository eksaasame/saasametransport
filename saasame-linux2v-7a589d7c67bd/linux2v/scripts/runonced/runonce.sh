#!/bin/bash

CALLBACK_PATH="/etc/runonce.d/run/_callback.sh"
if [ -f "$CALLBACK_PATH" ]; then
    OUTPUT=`"$CALLBACK_PATH"`
    echo -e "$OUTPUT" > "/etc/runonce.d/callback.log" 
    mv "$CALLBACK_PATH" "/etc/runonce.d/ran/_callback.sh.$(date +%Y%m%dT%H%M%S)"
    initctl reload-configuration
    systemctl daemon-reload
fi

`ntpdate -s 0.cn.pool.ntp.org`
`ntpdate -s time.google.com`
`ntpdate -s time.windows.com`
`hwclock --systohc`

RUN_PATH="/etc/runonce.d/run/run.sh"
POST_SCRIPT_PATH="/etc/runonce.d/run/post_script.sh"
if [ -f "$RUN_PATH" ]; then
    echo -e "Has $RUN_PATH" >> "/etc/runonce.d/callback.log"
    exec $POST_SCRIPT_PATH & #this doesn't blocks!
fi