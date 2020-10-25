// hardware_device.hpp ----------------------------------------------//
// -----------------------------------------------------------------------------

// Copyright 2013-2014 Albert Wei

// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt

// -----------------------------------------------------------------------------

// Revision History
// 04-29-2014 - Initial Release

// -----------------------------------------------------------------------------

#pragma once

#ifndef __MACHO_WINDOWS_NETWORK_ADAPTER__
#define __MACHO_WINDOWS_NETWORK_ADAPTER__

#include "boost\shared_ptr.hpp"
#include "common\exception_base.hpp"
#include <vector>
#include "windows\wmi.hpp"

namespace macho{
namespace windows{
class network{
public:
    class adapter_config{ // Win32_NetworkAdapterConfiguration
    public:
        typedef boost::shared_ptr<adapter_config> ptr;
        typedef std::vector<ptr> vtr;
        adapter_config(wmi_object &obj) : _obj(obj){}
        virtual ~adapter_config(){}

        virtual bool            arp_always_source_route() { return _obj[L"ArpAlwaysSourceRoute"]; }
        virtual bool            arp_use_ether_snap() { return _obj[L"ArpUseEtherSNAP"]; }
        virtual std::wstring    caption() { return _obj[L"Caption"]; }
        virtual std::wstring    database_path() { return _obj[L"DatabasePath"]; }
        virtual bool            dead_gw_detect_enabled() { return _obj[L"DeadGWDetectEnabled"]; }
        virtual string_array_w  default_ip_gateway() { return _obj[L"DefaultIPGateway"]; }
        virtual uint8_t         default_tos() { return _obj[L"DefaultTOS"]; } 
        virtual uint8_t         default_ttl() { return _obj[L"DefaultTTL"]; } 
        virtual std::wstring    description() { return _obj[L"Description"]; }
        virtual bool            dhcp_enabled() { return _obj[L"DHCPEnabled"]; }
       // virtual datetime      dhcp_lease_expires() { return _obj[L"DHCPLeaseExpires"]; }
       // virtual datetime      dhcp_lase_obtained() { return _obj[L"DHCPLeaseObtained"]; }
        virtual std::wstring    dhcp_server() { return _obj[L"DHCPServer"]; }
        virtual std::wstring    dns_domain() { return _obj[L"DNSDomain"]; }
        virtual string_array_w  dns_domain_suffix_search_order() { return _obj[L"DNSDomainSuffixSearchOrder"]; }
        virtual bool            dns_enabled_for_wins_resolution() { return _obj[L"DNSEnabledForWINSResolution"]; }
        virtual std::wstring    dns_host_name() { return _obj[L"DNSHostName"]; }
        virtual string_array_w  dns_server_search_order() { return _obj[L"DNSServerSearchOrder"]; }
        virtual bool            domain_dns_registration_enabled() { return _obj[L"DomainDNSRegistrationEnabled"]; }
        virtual uint32_t        forward_buffer_memory() { return _obj[L"ForwardBufferMemory"]; }
        virtual bool            full_dns_registration_enabled() { return _obj[L"FullDNSRegistrationEnabled"]; }
        virtual uint16_table    gateway_cost_metric() { return _obj[L"GatewayCostMetric"]; }
        virtual uint16_t        igmp_level() { return _obj[L"IGMPLevel"]; } /*uint8_t*/
        virtual uint32_t        index() { return _obj[L"Index"]; }
        virtual uint32_t        interface_index() { return _obj[L"InterfaceIndex"]; }
        virtual string_array_w  ip_address() { return _obj[L"IPAddress"]; }
        virtual uint32_t        ip_connection_metric() { return _obj[L"IPConnectionMetric"]; }
        virtual bool            ip_enabled() { return _obj[L"IPEnabled"]; }
        virtual bool            ip_filter_security_enabled() { return _obj[L"IPFilterSecurityEnabled"]; }
        virtual bool            ip_port_security_enabled() { return _obj[L"IPPortSecurityEnabled"]; }
        virtual string_array_w  ipsec_permit_ip_protocols() { return _obj[L"IPSecPermitIPProtocols"]; }
        virtual string_array_w  ipsec_permit_tcp_ports() { return _obj[L"IPSecPermitTCPPorts"]; }
        virtual string_array_w  ipsec_permit_udp_ports() { return _obj[L"IPSecPermitUDPPorts"]; }
        virtual string_array_w  ip_subnet() { return _obj[L"IPSubnet"]; }
        virtual bool            ip_use_zero_broadcast() { return _obj[L"IPUseZeroBroadcast"]; }
        virtual std::wstring    ipx_address() { return _obj[L"IPXAddress"]; }
        virtual bool            ipx_enabled() { return _obj[L"IPXEnabled"]; }
        virtual uint32_table    ipx_frame_type() { return _obj[L"IPXFrameType"]; }
        virtual uint32_t        ipx_media_type() { return _obj[L"IPXMediaType"]; }
        virtual string_array_w  ipx_network_number() { return _obj[L"IPXNetworkNumber"]; }
        virtual std::wstring    ipx_virtual_net_number() { return _obj[L"IPXVirtualNetNumber"]; }
        virtual uint32_t        keep_alive_interval() { return _obj[L"KeepAliveInterval"]; }
        virtual uint32_t        keep_alive_time() { return _obj[L"KeepAliveTime"]; }
        virtual std::wstring    mac_address() { return _obj[L"MACAddress"]; }
        virtual uint32_t        mtu() { return _obj[L"MTU"]; }
        virtual uint32_t        num_forward_packets() { return _obj[L"NumForwardPackets"]; }
        virtual bool            pmtu_bh_detect_enabled() { return _obj[L"PMTUBHDetectEnabled"]; }
        virtual bool            pmtu_discovery_enabled() { return _obj[L"PMTUDiscoveryEnabled"]; }
        virtual std::wstring    service_name() { return _obj[L"ServiceName"]; }
        virtual std::wstring    setting_id() { return _obj[L"SettingID"]; }
        virtual uint32_t        tcpip_netbios_options() { return _obj[L"TcpipNetbiosOptions"]; }
        virtual uint32_t        tcp_max_connect_retransmissions() { return _obj[L"TcpMaxConnectRetransmissions"]; }
        virtual uint32_t        tcp_max_data_retransmissions() { return _obj[L"TcpMaxDataRetransmissions"]; }
        virtual uint32_t        tcp_num_connections() { return _obj[L"TcpNumConnections"]; }
        virtual bool            tcp_use_rfc1122_urgent_pointer() { return _obj[L"TcpUseRFC1122UrgentPointer"]; }
        virtual uint16_t        tcp_window_size() { return _obj[L"TcpWindowSize"]; } 
        virtual bool            wins_enable_lmhosts_lookup() { return _obj[L"WINSEnableLMHostsLookup"]; }
        virtual std::wstring    wins_host_lookup_file() { return _obj[L"WINSHostLookupFile"]; }
        virtual std::wstring    wins_primary_server() { return _obj[L"WINSPrimaryServer"]; }
        virtual std::wstring    wins_scope_id() { return _obj[L"WINSScopeID"]; }
        virtual std::wstring    wins_secondary_server() { return _obj[L"WINSSecondaryServer"]; }
        virtual bool            enable_dhcp();
        virtual bool            set_dns_server_search_order(string_array_w dns_server_search_order);
        virtual bool            enable_static(string_array_w address, string_array_w subnet_mask);
        virtual bool            set_gateways(string_array_w default_ip_gateway, std::vector<uint16_t> gateway_cost_metric = std::vector<uint16_t>());
    private:
        std::wstring            get_error_message(DWORD code);
        wmi_object _obj;
    };

    class adapter{ //Win32_NetworkAdapter 
    public:
        typedef boost::shared_ptr<adapter> ptr;
        typedef std::vector<ptr> vtr;
        adapter(wmi_object &obj) : _obj(obj){}
        virtual ~adapter(){}
        virtual std::wstring    adapter_type() { return _obj[L"AdapterType"]; }
        virtual uint16_t        adapter_type_id() { return _obj[L"AdapterTypeID"]; }
        virtual bool            auto_sense() { return _obj[L"AutoSense"]; }
        virtual uint16_t        availability() { return _obj[L"Availability"]; }
        virtual std::wstring    caption() { return _obj[L"Caption"]; }
        virtual uint32_t        config_manager_error_code() { return _obj[L"ConfigManagerErrorCode"]; }
        virtual bool            config_manager_user_config() { return _obj[L"ConfigManagerUserConfig"]; }
        virtual std::wstring    creation_class_name() { return _obj[L"CreationClassName"]; }
        virtual std::wstring    description() { return _obj[L"Description"]; }
        virtual std::wstring    device_id() { return _obj[L"DeviceID"]; }
        virtual bool            error_cleared() { return _obj[L"ErrorCleared"]; }
        virtual std::wstring    error_description() { return _obj[L"ErrorDescription"]; }
        virtual std::wstring    guid() { return _obj[L"GUID"]; }
        virtual uint32_t        index() { return _obj[L"Index"]; }
        //virtual datetime      InstallDate() { return _obj[L"InstallDate"]; }
        virtual bool            installed() { return _obj[L"Installed"]; }
        virtual uint32_t        interface_index() { return _obj[L"InterfaceIndex"]; }
        virtual uint32_t        last_error_code() { return _obj[L"LastErrorCode"]; }
        virtual std::wstring    mac_address() { return _obj[L"MACAddress"]; }
        virtual std::wstring    manufacturer() { return _obj[L"Manufacturer"]; }
        virtual uint32_t        max_number_controlled() { return _obj[L"MaxNumberControlled"]; }
        virtual uint64_t        max_speed() { return _obj[L"MaxSpeed"]; }
        virtual std::wstring    name() { return _obj[L"Name"]; }
        virtual std::wstring    net_connection_id() { return _obj[L"NetConnectionID"]; }
        virtual uint16_t        net_connection_status() { return _obj[L"NetConnectionStatus"]; }
        virtual bool            net_enabled() { return _obj[L"NetEnabled"]; }
        //virtual string_array_w  network_addresses() { return _obj[L"NetworkAddresses"]; } // This property has not been implemented yet. It returns a NULL value by default.
        //virtual std::wstring    permanent_address() { return _obj[L"PermanentAddress"]; } // This property has not been implemented yet. It returns a NULL value by default.
        virtual bool            physical_adapter() { return _obj[L"PhysicalAdapter"]; }
        virtual std::wstring    pnp_device_id() { return _obj[L"PNPDeviceID"]; }
        virtual uint16_table    power_management_capabilities() { return _obj[L"PowerManagementCapabilities"]; }
        virtual bool            power_management_supported() { return _obj[L"PowerManagementSupported"]; }
        virtual std::wstring    product_name() { return _obj[L"ProductName"]; }
        virtual std::wstring    service_name() { return _obj[L"ServiceName"]; }
        virtual uint64_t        speed() { return _obj[L"Speed"]; }
        virtual std::wstring    status() { return _obj[L"Status"]; }
        virtual uint16_t        status_info() { return _obj[L"StatusInfo"]; }
        virtual std::wstring    system_creation_class_name() { return _obj[L"SystemCreationClassName"]; }
        virtual std::wstring    system_name() { return _obj[L"SystemName"]; }
        //virtual datetime      TimeOfLastReset() { return _obj[L"TimeOfLastReset"]; }
        adapter_config::ptr     get_setting(){
            try{
                wmi_object obj = _obj.get_related(L"Win32_NetworkAdapterConfiguration", L"Win32_NetworkAdapterSetting");
                return  adapter_config::ptr(new adapter_config(obj));
            }
            catch (macho::exception_base &ex){
                LOG(LOG_LEVEL_ERROR, L"%s", macho::get_diagnostic_information(ex).c_str());
            }
            catch (...){
                LOG(LOG_LEVEL_ERROR, L"Unknown Exception.");
            }
            return adapter_config::ptr();
        }
    private:
        wmi_object _obj;
    };
    static adapter::vtr get_network_adapters(std::wstring host = L"", std::wstring user = L"", std::wstring password = L"");
    static adapter::vtr get_network_adapters(std::wstring host, std::wstring domain, std::wstring user, std::wstring password);
    static adapter::vtr get_network_adapters(macho::windows::wmi_services &wmi, bool is_wmi_root = true);
};

#ifndef MACHO_HEADER_ONLY

network::adapter::vtr network::get_network_adapters(std::wstring host, std::wstring user, std::wstring password){
    std::wstring domain, account;
    string_array arr = stringutils::tokenize2(user, L"\\", 2, false);
    if (arr.size()){
        if (arr.size() == 1)
            account = arr[0];
        else{
            domain = arr[0];
            account = arr[1];
        }
    }
    return get_network_adapters(host, domain, account, password);
}

network::adapter::vtr network::get_network_adapters(std::wstring host, std::wstring domain, std::wstring user, std::wstring password){
    HRESULT hr = S_OK;
    FUN_TRACE_HRESULT(hr);
    macho::windows::wmi_services root;
    if (host.length())
        hr = root.connect(L"", host, domain, user, password);
    else
        hr = root.connect(L"");
    return get_network_adapters(root);
}

network::adapter::vtr network::get_network_adapters(macho::windows::wmi_services &wmi, bool is_wmi_root){
    HRESULT hr = S_OK;
    FUN_TRACE_HRESULT(hr);
    adapter::vtr adapters;
    macho::windows::wmi_services cimv2;
    if (is_wmi_root)
        hr = wmi.open_namespace(L"CIMV2", cimv2);
    else
        cimv2 = wmi;
    wmi_object_table objs = cimv2.query_wmi_objects(L"Win32_NetworkAdapter");
    for (wmi_object_table::iterator obj = objs.begin(); obj != objs.end(); obj++){
        adapters.push_back(adapter::ptr(new adapter(*obj)));
    }
    return adapters;
}

bool network::adapter_config::enable_dhcp(){
    bool result = false;
    wmi_object inparameters, outparameters;
    DWORD reture_value = 0;
    inparameters = _obj.get_input_parameters(L"EnableDHCP");
    HRESULT hr = _obj.exec_method(L"EnableDHCP", inparameters, outparameters, reture_value);
    if (SUCCEEDED(hr)){
        if (!(result = (0 == reture_value || 1 == reture_value))){
            LOG(LOG_LEVEL_ERROR, get_error_message(reture_value).c_str());
        }
        else if (1 == reture_value){
            LOG(LOG_LEVEL_WARNING, get_error_message(reture_value).c_str());
        }
        else{
            LOG(LOG_LEVEL_INFO, get_error_message(reture_value).c_str());
        }
    }
    return result;
}

bool network::adapter_config::set_dns_server_search_order(string_array_w dns_server_search_order){
    bool result = false;
    wmi_object inparameters, outparameters;
    DWORD reture_value = 0;
    if ( dns_server_search_order.size()){
        inparameters = _obj.get_input_parameters(L"SetDNSServerSearchOrder");     
        inparameters[L"DNSServerSearchOrder"] = dns_server_search_order;
        HRESULT hr = _obj.exec_method(L"SetDNSServerSearchOrder", inparameters, outparameters, reture_value);
        if (SUCCEEDED(hr)){
            if (!(result = (0 == reture_value || 1 == reture_value))){
                LOG(LOG_LEVEL_ERROR, get_error_message(reture_value).c_str());
            }
            else if (1 == reture_value){
                LOG(LOG_LEVEL_WARNING, get_error_message(reture_value).c_str());
            }
            else{
                LOG(LOG_LEVEL_INFO, get_error_message(reture_value).c_str());
            }
        }
    }
    return result;
}

bool network::adapter_config::enable_static(string_array_w address, string_array_w subnet_mask){
    bool result = false;
    wmi_object inparameters, outparameters;
    DWORD reture_value = 0;
    if (address.size() && subnet_mask.size() && address.size() == subnet_mask.size()){
        inparameters = _obj.get_input_parameters(L"EnableStatic");
        inparameters[L"IPAddress"] = address;
        inparameters[L"SubnetMask"] = subnet_mask;
        HRESULT hr = _obj.exec_method(L"EnableStatic", inparameters, outparameters, reture_value);
        if (SUCCEEDED(hr)){
            if (!(result = (0 == reture_value || 1 == reture_value))){
                LOG(LOG_LEVEL_ERROR, get_error_message(reture_value).c_str());
            }
            else if (1 == reture_value){
                LOG(LOG_LEVEL_WARNING, get_error_message(reture_value).c_str());
            }
            else{
                LOG(LOG_LEVEL_INFO, get_error_message(reture_value).c_str());
            }
        }
    }
    return result;
}

bool network::adapter_config::set_gateways(string_array_w default_ip_gateway, std::vector<uint16_t> gateway_cost_metric){
    bool result = false;
    wmi_object inparameters, outparameters;
    DWORD reture_value = 0;
    if (default_ip_gateway.size()){
        inparameters = _obj.get_input_parameters(L"SetGateways");
        inparameters[L"DefaultIPGateway"] = default_ip_gateway;
        if (gateway_cost_metric.size())
            inparameters[L"GatewayCostMetric"] = gateway_cost_metric;
        HRESULT hr = _obj.exec_method(L"SetGateways", inparameters, outparameters, reture_value);
        if (SUCCEEDED(hr)){
            if (!(result = (0 == reture_value || 1 == reture_value))){
                LOG(LOG_LEVEL_ERROR, get_error_message(reture_value).c_str());
            }
            else if (1 == reture_value){
                LOG(LOG_LEVEL_WARNING, get_error_message(reture_value).c_str());
            }
            else{
                LOG(LOG_LEVEL_INFO, get_error_message(reture_value).c_str());
            }
        }
    }
    return result;
}

std::wstring network::adapter_config::get_error_message(DWORD code){
    std::wstring message;
    switch (code){
    case 0:
        message = L"Successful completion, no reboot required";
        break;
    case 1:
        message = L"Successful completion, reboot required";
        break;
    case 64:
        message = L"Method not supported on this platform";
        break;
    case 65:
        message = L"Unknown failure";
        break;
    case 66:
        message = L"Invalid subnet mask";
        break;
    case 67:
        message = L"An error occurred while processing an Instance that was returned";
        break;
    case 68:
        message = L"Invalid input parameter";
        break;
    case 69:
        message = L"More than 5 gateways specified";
        break;
    case 70:
        message = L"Invalid IP address";
        break;
    case 71:
        message = L"Invalid gateway IP address";
        break;
    case 72:
        message = L"An error occurred while accessing the Registry for the requested information";
        break;
    case 73:
        message = L"Invalid domain name";
        break;
    case 74:
        message = L"Invalid host name";
        break;
    case 75:
        message = L"No primary or secondary WINS server defined";
        break;
    case 76:
        message = L"Invalid file";
        break;
    case 77:
        message = L"Invalid system path";
        break;
    case 78:
        message = L"File copy failed";
        break;
    case 79:
        message = L"Invalid security parameter";
        break;
    case 80:
        message = L"Unable to configure TCP/IP service";
        break;
    case 81:
        message = L"Unable to configure DHCP service";
        break;
    case 82:
        message = L"Unable to renew DHCP lease";
        break;
    case 83:
        message = L"Unable to release DHCP lease";
        break;
    case 84:
        message = L"IP not enabled on adapter";
        break;
    case 85:
        message = L"IPX not enabled on adapter";
        break;
    case 86:
        message = L"Frame/network number bounds error";
        break;
    case 87:
        message = L"Invalid frame type";
        break;
    case 88:
        message = L"Invalid network number";
        break;
    case 89:
        message = L"Duplicate network number";
        break;
    case 90:
        message = L"Parameter out of bounds";
        break;
    case 91:
        message = L"Access denied";
        break;
    case 92:
        message = L"Out of memory";
        break;
    case 93:
        message = L"Already exists";
        break;
    case 94:
        message = L"Path, file or object not found";
        break;
    case 95:
        message = L"Unable to notify service";
        break;
    case 96:
        message = L"Unable to notify DNS service";
        break;
    case 97:
        message = L"Interface not configurable";
        break;
    case 98:
        message = L"Not all DHCP leases could be released/renewed";
        break;
    case 100:
        message = L"DHCP not enabled on adapter";
        break;
    default:
        message = L"Other";
    }
    return message;
}

#endif

};
};
#endif