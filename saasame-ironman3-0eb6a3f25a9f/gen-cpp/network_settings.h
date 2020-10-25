#pragma once

#ifndef network_settings_H
#define network_settings_H

#include "saasame_constants.h"


#include "macho.h"
#include "json_spirit.h"
#include "..\buildenv.h"

#pragma comment(lib, "json_spirit_lib.lib")

using namespace  ::saasame::transport;

class network_settings{
public :
    static std::string to_string(const std::set<network_info> &infos, std::string default_mtu){
    using namespace json_spirit;
        mObject value;
        mArray network_infos;
        foreach(saasame::transport::network_info n, infos){
            mObject net;
            net["adapter_name"] = n.adapter_name;
            net["description"] = n.description;
            net["is_dhcp_v4"] = n.is_dhcp_v4;
            net["is_dhcp_v6"] = n.is_dhcp_v6;
            net["mac_address"] = n.mac_address;
            mArray  ip_addresses(n.ip_addresses.begin(), n.ip_addresses.end());
            net["ip_addresses"] = ip_addresses;
            mArray  subnet_masks(n.subnet_masks.begin(), n.subnet_masks.end());
            net["subnet_masks"] = subnet_masks;
            mArray  gateways(n.gateways.begin(), n.gateways.end());
            net["gateways"] = gateways;
            mArray  dnss(n.dnss.begin(), n.dnss.end());
            net["dnss"] = dnss;
            network_infos.push_back(net);
        }
        value["network_infos"] = network_infos;
        value["default_mtu"] = default_mtu;
        return write(value, json_spirit::raw_utf8);
    }

    static std::set<network_info> load(const std::string &config, std::string &default_mtu){
        if (config.length()){
            try{
                using namespace json_spirit;
                mValue value;
                read(config, value);
                return _load(value, default_mtu);
            }
            catch (...){

            }
        }
        return std::set<network_info>();
    }

    static std::set<network_info> load(boost::filesystem::path _path, std::string &default_mtu){
        if (boost::filesystem::exists(_path)){
            try{
                using namespace json_spirit;
                mValue value;
                std::ifstream is(_path.wstring());
                read(is, value);
                return _load(value, default_mtu);
            }
            catch (...){

            }
        }
        return std::set<network_info>();
    }
private:
    static std::set<network_info> _load(json_spirit::mValue &value, std::string &default_mtu){
        using namespace json_spirit;  
        std::set<network_info> networks;
        mArray network_infos = find_value(value.get_obj(), "network_infos").get_array();
        foreach(mValue n, network_infos){
            saasame::transport::network_info net;
            net.adapter_name = find_value_string(n.get_obj(), "adapter_name");
            net.description = find_value_string(n.get_obj(), "description");
            net.mac_address = find_value_string(n.get_obj(), "mac_address");
            net.is_dhcp_v4 = find_value(n.get_obj(), "is_dhcp_v4").get_bool();
            net.is_dhcp_v6 = find_value(n.get_obj(), "is_dhcp_v6").get_bool();
            mArray dnss = find_value(n.get_obj(), "dnss").get_array();
            foreach(mValue d, dnss){
                net.dnss.push_back(d.get_str());
            }
            mArray gateways = find_value(n.get_obj(), "gateways").get_array();
            foreach(mValue g, gateways){
                net.gateways.push_back(g.get_str());
            }
            mArray ip_addresses = find_value(n.get_obj(), "ip_addresses").get_array();
            foreach(mValue i, ip_addresses){
                net.ip_addresses.push_back(i.get_str());
            }
            mArray subnet_masks = find_value(n.get_obj(), "subnet_masks").get_array();
            foreach(mValue s, subnet_masks){
                net.subnet_masks.push_back(s.get_str());
            }
            networks.insert(net);
        }
        default_mtu = find_value_string(value.get_obj(), "default_mtu");
        return networks;
    }

    static const json_spirit::mValue& find_value(const json_spirit::mObject& obj, const std::string& name){
        json_spirit::mObject::const_iterator i = obj.find(name);
        assert(i != obj.end());
        assert(i->first == name);
        return i->second;
    }

    static const std::string find_value_string(const json_spirit::mObject& obj, const std::string& name){
        json_spirit::mObject::const_iterator i = obj.find(name);
        if (i != obj.end()){
            return i->second.get_str();
        }
        return "";
    }
};

#endif