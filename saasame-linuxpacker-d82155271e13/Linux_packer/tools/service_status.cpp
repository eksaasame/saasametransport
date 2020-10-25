#include <event.h>
#include <unistd.h>

#include "service_status.h"

void service_status::read_service_config_file()
{
    ptree root;
    struct stat buf;
    int a = stat(SERVICE_CONFIG_PATH, &buf);
    LOG_TRACE("a = %d", a);
    if (!a)
    {
        try {
            read_json(SERVICE_CONFIG_PATH, root);
        }
        catch (const std::exception &e)
        {
            LOG_ERROR("read %s fail\r\n", SERVICE_CONFIG_PATH);
        }
    }
    if (!root.empty())
    {
        ptree status = root.get_child("service_status");
        if (!status.empty())
        {
            SessionId = status.get<string>("SessionId");
            LOG_TRACE("SessionId = %s\r\n", SessionId.c_str());
            MgmtAddr = status.get<string>("MgmtAddr");
            LOG_TRACE("MgmtAddr = %s\r\n", MgmtAddr.c_str());
            AllowMultiple = status.get<int>("AllowMultiple");
            LOG_TRACE("AllowMultiple = %d\r\n", AllowMultiple);
        }
    }
}

void service_status::write_service_config_file()
{
    //FUNC_TRACER;
    ptree root;
    ptree status;
    status.put("SessionId", SessionId);
    status.put("MgmtAddr", MgmtAddr);
    status.put("AllowMultiple", AllowMultiple);
    root.add_child("service_status", status);
    mkdir(SERVICE_CONFIG_FOLDER_PATH, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    fstream fs;
    fs.open(SERVICE_CONFIG_PATH, ios::out | ios::trunc);
    if (fs)
    {
        write_json(fs, root, true);
        fs.close();
    }
    else
    {
        LOG_TRACE("%s create file fail\r\n", SERVICE_CONFIG_PATH);
    }
}
void service_status::print()
{
    LOG_TRACE("SessionId = %s\r\n MgmtAddr = %s\r\n AllowMultiple = %d\r\n", SessionId.c_str(), MgmtAddr.c_str(), AllowMultiple);
}

service_status::ptr service_status::create()
{
    return boost::shared_ptr<service_status>(new service_status());
}