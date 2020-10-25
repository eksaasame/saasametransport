#pragma once

#ifndef db_op_H
#define db_op_H

#include "macho.h"
#include <sstream>
#include <map>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

// under soci
#include "soci.h"
#include "soci-mysql.h"

#pragma comment(lib, "mariadbclient.lib")
#pragma comment(lib, "libsoci_core_3_2.lib")
#pragma comment(lib, "libsoci_mysql_3_2.lib")

using namespace macho;
using namespace macho::windows;

#define timegm _mkgmtime

struct host{
    typedef std::vector<host> vtr;
    host(soci::row &h){
        name = h.get<std::string>("_HOST_NAME", std::string());
        addr = h.get<std::string>("_HOST_ADDR", std::string());
        type = h.get<std::string>("_SERV_TYPE", std::string());
        std::string info = h.get<std::string>("_HOST_INFO", std::string());
        using namespace boost::property_tree;
        if (info.empty())
            return;
        ptree _info;
        std::istringstream is(info);
        read_json(is, _info);
        if (type == "Physical" || type == "Offline"){
            uuid = _info.get<std::string>("machine_id");
        }
        else if (type == "Virtual"){
            uuid = _info.get<std::string>("uuid");
        }
    }
    std::string name;
    std::string addr;
    std::string uuid;
    std::string type;  
};

struct replica{
    typedef std::vector<replica> vtr;
    replica(){}
    std::string repl_uuid;
    std::string packer;
    std::string status;
    std::time_t create_time;
    std::time_t delete_time;
    std::time_t update_time;
    host::vtr hosts;
};

class mysql_op
{
public:
    typedef boost::shared_ptr<mysql_op> ptr;
    mysql_op(){}
    virtual ~mysql_op(){
        try{
            _session.close();
        }
        catch (std::exception const & e){
            LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(e.what()).c_str());
        }
    }

    static mysql_op::ptr open(const std::string connect_string = "dbname=management user=root password=saasameFTW" ){
        try{
            mysql_op::ptr p = mysql_op::ptr(new mysql_op());
            p->_session.open(soci::mysql, connect_string);
            return p;
        }
        catch (std::exception const & e){
            LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(e.what()).c_str());
        }
        return NULL;
    }

    std::string query_replica_machine_id(const std::string& job_id, bool is_recovery){
        replica repl;
        try{
            std::string repl_uuid = job_id;
            std::string service_sql = "SELECT _REPL_UUID FROM _SERVICE WHERE _SERV_UUID = :serv_uuid";
            std::string replica_sql = "SELECT * FROM  _REPLICA WHERE _REPL_UUID = :repl_uuid";
            std::string server_host_sql = "SELECT * FROM _SERVER_HOST WHERE _HOST_UUID = :host_uuid AND _STATUS = 'Y'";
            if (is_recovery){
                soci::rowset<soci::row> service_sql_results = (_session.prepare << service_sql, soci::use(job_id));;
                foreach(soci::row& r, service_sql_results){
                    repl_uuid = r.get<std::string>("_REPL_UUID", std::string());
                    break;
                }
            }
            soci::rowset<soci::row> replica_sql_results = (_session.prepare << replica_sql, soci::use(repl_uuid));;
            foreach(soci::row& r, replica_sql_results){
                repl.packer = r.get<std::string>("_PACK_UUID", std::string());
                if (!repl.packer.empty()){
                    try{
                        soci::rowset<soci::row> server_host_sql_results = (_session.prepare << server_host_sql, soci::use(repl.packer));
                        foreach(soci::row &h, server_host_sql_results){
                            repl.hosts.push_back(host(h));
                        }
                    }
                    catch (std::exception const & e){
                        LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(e.what()).c_str());
                    }
                }
                break;
            }
        }
        catch (std::exception const & e){
            LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(e.what()).c_str());
        }
        if (repl.hosts.size())
            return repl.hosts[0].uuid;
        return "";
    }

    replica::vtr query_replicas(){
        replica::vtr repls;
        try{
            std::string replica_sql = "SELECT * FROM _REPLICA";
            std::string snapshot_count_sql = "SELECT _SNAP_TIME FROM _REPLICA_SNAP WHERE _REPL_UUID = :repl_uuid";
            std::string server_host_sql = "SELECT * FROM _SERVER_HOST WHERE _HOST_UUID = :host_uuid AND _STATUS = 'Y'";
            soci::rowset<soci::row> replica_sql_results = _session.prepare << replica_sql;
            foreach(soci::row& r, replica_sql_results){
                replica repl;
                repl.repl_uuid = r.get<std::string>("_REPL_UUID", std::string());
                soci::rowset<soci::row> snapshot_count_sql_results = (_session.prepare << snapshot_count_sql, soci::use(repl.repl_uuid));
                std::vector<boost::posix_time::ptime> snapshot_times;
                foreach(soci::row &h, snapshot_count_sql_results){
                    std::string snap_time = h.get<std::string>("_SNAP_TIME", std::string());
                    if (!snap_time.empty()){
                        try{
                            snapshot_times.push_back(boost::posix_time::time_from_string(snap_time));
                        }
                        catch (std::exception const & e){
                            LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(e.what()).c_str());
                        }
                    }
                }
                if (snapshot_times.size() > 0){
                    std::sort(snapshot_times.begin(), snapshot_times.end());
                    repl.packer = r.get<std::string>("_PACK_UUID", std::string());
                    repl.status = r.get<std::string>("_STATUS", std::string());
                    repl.create_time = timegm(&r.get<std::tm>("_REPL_CREATE_TIME", std::tm()));
                    repl.delete_time = timegm(&r.get<std::tm>("_REPL_DELETE_TIME", std::tm()));
                    repl.update_time = boost::posix_time::to_time_t(snapshot_times[snapshot_times.size() - 1]);
                    if (!repl.packer.empty()){
                        try{
                            soci::rowset<soci::row> server_host_sql_results = (_session.prepare << server_host_sql, soci::use(repl.packer));
                            foreach(soci::row &h, server_host_sql_results){
                                repl.hosts.push_back(host(h));
                            }
                        }
                        catch (std::exception const & e){
                            LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(e.what()).c_str());
                        }
                    }
                    if (repl.hosts.size())
                        repls.push_back(repl);
                }
            }
        }
        catch (std::exception const & e){
            LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(e.what()).c_str());
        }
        return repls;
    }

protected:

    bool begin(){
        try{
            _session.begin();
        }
        catch (std::exception const & e){
            LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(e.what()).c_str());
        }
    }
    
    bool commit(){
        try{
            _session.commit();
        }
        catch (std::exception const & e){
            LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(e.what()).c_str());
        }
    }

    bool rollback(){
        try{
            _session.rollback();
        }
        catch (std::exception const & e){
            LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(e.what()).c_str());
        }
    }

private:
    soci::session _session;
};

#endif