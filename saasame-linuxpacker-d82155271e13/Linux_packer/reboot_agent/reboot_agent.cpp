#include <stdio.h>
#include "../tools/snapshot.h"
#include "../tools/system_tools.h"
#include <syslog.h>
#include "../tools/log.h"

int main()
{
    b_go_to_dmesg_i = true;
    /*read the snapshot config*/
    system_tools::execute_command("echo 683T mumi > /dev/kmsg");
    snapshot_manager sm(false);
    for (auto & si : sm.snapshot_map)
    {
        //LOG_TRACE("hehe");
        //int ret = si.second->get_info();
        //LOG_TRACE("ret = %d\r\n", ret);
        //if (ret == ENOENT) //not exist
        {
            //LOG_TRACE("hehe2\r\n");
            si.second->reload();
        }
    }
    /**/
    return 0;
}