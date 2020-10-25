#pragma once
#ifndef winpe_util_H
#define winpe_util_H

#include "macho.h"

struct winpe_network_setting{
    std::wstring              mac_addr;
    std::vector<std::wstring> tbIPAddress;
    std::vector<std::wstring> tbSubnetMask;
    std::vector<std::wstring> tbDNSServers;
    std::vector<std::wstring> tbGateways;
};

struct winpe_settings{
    winpe_settings() : is_keep(false){}
    bool                                           is_keep;
    std::wstring                                   host_name;
    std::wstring                                   session_id;
    std::wstring                                   mgmt_addr;
    std::wstring                                   machine_id;
    std::wstring                                   client_id;
    std::wstring                                   digital_id;
    std::map<std::wstring, winpe_network_setting > network_settings;
    void save();
    bool load();
    void apply();
    static void set_ip_settings_by_netsh_command(std::wstring settings_id, std::wstring connection, std::vector<std::wstring> tbIPAddress, std::vector<std::wstring> tbSubnetMask, std::vector<std::wstring> tbDNSServers, std::vector<std::wstring> tbGateways, std::wstring gatewaymetric, std::vector<std::wstring> tbWINSs, bool is_enable_dhcp);
    static void sync_time();
};


#endif
