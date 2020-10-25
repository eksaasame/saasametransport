#pragma once

#ifndef service_status_H
#define service_status_H

#include "string_operation.h"
#include "sslHelper.h"
#include "TCurlClient.h"
#include "system_tools.h"
#include "string_operation.h"
#include "log.h"
#define SERVICE_CONFIG_PATH "/etc/saasame/service_config"
#define SERVICE_CONFIG_FOLDER_PATH "/etc/saasame"

class service_status
{
private:
    string SessionId;
    string MgmtAddr;
    int AllowMultiple;
    void read_service_config_file();
public:
    typedef boost::shared_ptr<service_status> ptr;
    static service_status::ptr create();
    void setSessionId(string input) { SessionId = input; }
    void setMgmtAddr(string input) { MgmtAddr = input; }
    void setAllowMultiple(int input) { AllowMultiple = input; }
    static void unregister() { remove(SERVICE_CONFIG_PATH); }
    string getSessionId() { return SessionId; }
    string getMgmtAddr() { return MgmtAddr; }
    int getAllowMultiple() { return AllowMultiple; }
    service_status() { read_service_config_file(); }
    void write_service_config_file();
    void print();
};

#endif