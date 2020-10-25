#pragma once
#ifndef system_tools_H
#define system_tools_H
#include <iostream>
#include <unistd.h>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>

#include <unordered_map>
#include <string>
#include <set>
#include "json_spirit/json_spirit.h"
#include "log.h"

//system include
#include <sys/utsname.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/hdreg.h>
#include <scsi/sg.h>
#include <scsi/scsi.h>
#include <dirent.h>
#include <linux/fs.h>
#include <exception>


#define GetExecutionFilePath_Linux(buf) do{int count = readlink("/proc/self/exe", buf, sizeof(buf)); if(count!=-1){buf[count] = '\0';} }while(0)
#define INQUIRY_REPLY_LEN  0x60
#define SERIALINQUIRY { INQUIRY, 0x01, 0x80, 0x00, INQUIRY_REPLY_LEN, 0x00}
#define NORMALINQUIRY { INQUIRY, 0x00, 0x00, 0x00, INQUIRY_REPLY_LEN, 0x00}
#define PROC_PARTITIONS "/proc/partitions"
#define PROC_MOUNTSTATS "/proc/self/mountstats"
#define PROC_COMMAND_LINE "/proc/self/cmdline"
#define COMMON_BUFFER_SIZE 1024
#define PROC_SELF_FD "/proc/self/fd"

using namespace boost;
using namespace boost::property_tree;
using namespace std;
using namespace json_spirit;


class system_tools
{
#define system_tools_declared
public:
    struct cpu_info
    {
        int logical_core_number;
        unordered_map<int, int> physical_core;
    };

    class version
    {
    public:
        class version_exception : public std::exception
        {
            const char * what() const throw ()
            {
                return "version_compare_exception";
            }
        };
        version(const string & input) {
            boost::split(strVec, input, boost::is_any_of("."));
            for (auto & s : strVec)
            {
                LOG_TRACE("s.c_str() = %s", s.c_str());
                try
                {
                    uint64Vec.push_back((uint64_t)stoi(s));
                }catch(...)
                {}
            }
        }
        bool operator < (const version &) const;
        bool operator > (const version &) const;
        bool operator == (const version &) const;
        vector<string> get_strVec() { return strVec; }
        vector<uint64_t> get_uint64Vec() { return uint64Vec; }
    private:
        vector<string> strVec;
        vector<uint64_t> uint64Vec;
    };

	system_tools() {
		uname(&un);
	}
    static string get_execution_file_path_linux();
    static string get_execution_command_line();
    static string path_remove_last(string input);
    static int get_cpu_info(cpu_info & ci);
    static int get_mac_address(unsigned char mac_address[6]);
    static int system_tools::get_mac_address_by_command(unsigned char mac_address[6]);
    static int get_dns_servers(std::vector<std::string> & dns_servers);
    static int get_gateway(std::vector<std::string> & gateway);
    static int get_machine_id(string& suuid);
    static int get_total_memory(int& total_memory);
    static int get_os_pretty_name_and_info(string& pretty_name, string& arch, int & major, int & minor);
    static string execute_command(const char* cmd);
    static vector<string> get_ip_address();
    static bool system_tools::copy_file(const char * src,const char * dst);
    static string get_files_mount_point(string filename, set<string> mounted_points);
    static string get_files_mount_point_by_command(string filename);
    static string analysis_ip_address(string input);
    static std::string get_fd_name(int fd);
    static int get_disk_sector_size_sd(int fd);
    static int get_disk_sector_size_hd(int fd);
    static int get_disk_sector_size(int fd);
    static uint64_t get_version_number(string & version);
    //static int do_mkdir(char * path, int mode);
    static std::string gen_random_uuid();
    template<class T>
    static bool region_intersection(T & a, T & b, T & c)
    {
        uint64_t a_end = a.start + a.length-1;
        int64_t a_src_diff = a.start - a.src_start;
        uint64_t b_end = b.start + b.length-1;
        uint64_t return_start = max(a.start, b.start);
        uint64_t return_end = min(a_end, b_end);
        if (return_start < return_end)
        {
            c.start = return_start;
            c.length = return_end - return_start + 1;
            c.src_start = return_start - a_src_diff;
            c._rw = a._rw;
            return true;
        }
        else
            return false;
    }
    template<class T, class T_vtr, class T_vtr_itr>
    static bool regions_vectors_intersection(T_vtr & a, T_vtr & b, T_vtr & c)
    {
        for (T_vtr_itr ai = a.begin(), bi = b.begin(); ai != a.end() && bi != b.end();)
        {
            T rc(0,0);
            LOG_TRACE("@@========================================================================\r\n");
            LOG_TRACE("@@ai.start = %llu , ai.src_start = %llu, ai.length = %llu, ai.rw.path = %s\r\n", ai->start, ai->src_start, ai->length, ai->_rw->path().c_str());
            LOG_TRACE("@@bi.start = %llu , bi.src_start = %llu, bi.length = %llu, bi.rw.path = %s\r\n", bi->start, bi->src_start, bi->length, bi->_rw->path().c_str());
            if (system_tools::region_intersection<T>(*ai, *bi, rc))
            {
                LOG_TRACE("@@rc.start = %llu , rc.src_start = %llu, rc.length = %llu, rc.rw.path = %s\r\n", rc.start, rc.src_start, rc.length, rc._rw->path().c_str());
                c.push_back(rc);
            }
            uint64_t a_end = ai->start + ai->length - 1, b_end = bi->start + bi->length - 1;
            if (a_end < b_end)
                ++ai;
            else
                ++bi;
        }
    }

    template<class T>
    static bool region_union(T & a, T & b, T & c)
    {
        uint64_t a_end = a.start + a.length - 1;
        int64_t a_src_diff = a.start - a.src_start;
        uint64_t b_end = b.start + b.length - 1;
        uint64_t return_start = min(a.start, b.start);
        uint64_t return_end = max(a_end, b_end);
        if ((b.start <= a_end && a.start <= b.start) ||
            (a.start <= b_end && b.start <= a.start))
        {
            c.start = return_start;
            c.length = return_end - return_start + 1;
            c.src_start = return_start - a_src_diff;
            c._rw = a._rw;
            return true;
        }
        else
            return false;
    }

    template<class T, class T_vtr, class T_vtr_itr>
    static bool regions_vectors_union(T_vtr & a, T_vtr & b, T_vtr & c)
    {
        T_vtr_itr ai, bi;
        for (ai = a.begin(), bi = b.begin(); ai != a.end() && bi != b.end();)
        {
            T rc(0, 0);
            LOG_TRACE("@@========================================================================\r\n");
            LOG_TRACE("@@ai.start = %llu , ai.src_start = %llu, ai.length = %llu, ai.rw.path = %s\r\n", ai->start, ai->src_start, ai->length, ai->_rw->path().c_str());
            LOG_TRACE("@@bi.start = %llu , bi.src_start = %llu, bi.length = %llu, bi.rw.path = %s\r\n", bi->start, bi->src_start, bi->length, bi->_rw->path().c_str());
            if (system_tools::region_union<T>(*ai, *bi, rc))
            {
                LOG_TRACE("@@rc.start = %llu , rc.src_start = %llu, rc.length = %llu, rc.rw.path = %s\r\n", rc.start, rc.src_start, rc.length, rc._rw->path().c_str());
                c.push_back(rc);
                ++ai;
                ++bi;
            }
            else
            {
                uint64_t a_end = ai->start + ai->length - 1, b_end = bi->start + bi->length - 1;
                if (a_end < b_end)
                {
                    LOG_TRACE("@@ai->start = %llu , ai->src_start = %llu, ai->length = %llu, ai->rw.path = %s\r\n", ai->start, ai->src_start, ai->length, ai->_rw->path().c_str());
                    c.push_back(*ai);
                    ++ai;
                }
                else
                {
                    LOG_TRACE("@@bi->start = %llu , bi->src_start = %llu, bi->length = %llu, bi->rw.path = %s\r\n", bi->start, bi->src_start, bi->length, bi->_rw->path().c_str());
                    c.push_back(*bi);
                    ++bi;
                }
            }
        }
        for (;ai != a.end();++ai)
        {
            LOG_TRACE("@@ai->start = %llu , ai->src_start = %llu, ai->length = %llu, ai->rw.path = %s\r\n", ai->start, ai->src_start, ai->length, ai->_rw->path().c_str());
            c.push_back(*ai);
        }
        for (;bi != b.end(); ++bi)
        {
            LOG_TRACE("@@bi->start = %llu , bi->src_start = %llu, bi->length = %llu, bi->rw.path = %s\r\n", bi->start, bi->src_start, bi->length, bi->_rw->path().c_str());
            c.push_back(*bi);
        }
    }


    template<class T>
    static int region_complement(T & a, T & b, T & c, T & d) //a is base, b is sub
    {
        uint64_t a_end = a.start + a.length -1;
        int64_t a_src_diff = a.start - a.src_start;
        uint64_t b_end = b.start + b.length -1;
        uint64_t return_start = 0;
        uint64_t return_end = -1;
        uint64_t remain_start = 0;
        uint64_t remain_end = -1;
        int inter = 1;
        if (a.start <= b.start && b.start <= a_end)  // the refion has interection and a < b
        {
            return_start = a.start;
            return_end = b.start - 1;
            if (b_end < a_end)
            {
                remain_start = b_end + 1;
                remain_end = a_end;
            }
        }
        else if (b.start <= a.start && a.start <= b_end) // the refion has interection
        {
            if (b_end < a_end)
            {
                remain_start = b_end+1;
                remain_end = a_end;
            }
        }
        else // the refion has no interection
        {
            if(a_end < b_end)
                inter = 0;
            else
                inter = -1;
            return_start = a.start;
            return_end = a_end;
        }
        c.start = return_start;
        c.length = return_end - return_start + 1;
        c.src_start = return_start - a_src_diff;
        c._rw = a._rw;
        d.start = remain_start;
        d.length = remain_end - remain_start + 1;
        d.src_start = remain_start - a_src_diff;
        d._rw = a._rw;
        return inter;
    }

    template<class T, class T_vtr, class T_vtr_itr>
    static bool regions_vectors_complement(T_vtr & a, T_vtr & b, T_vtr & c)
    {
        bool new_base = true;
        T_vtr_itr ai, bi;
        for (ai = a.begin(), bi = b.begin(); ai != a.end();)
        {
            if (bi != b.end())
            {
                T rc(0, 0), rd(0, 0);
                LOG_TRACE("@@========================================================================\r\n");
                LOG_TRACE("@@ai.start = %llu , ai.src_start = %llu, ai.length = %llu, ai.rw.path = %s\r\n", ai->start, ai->src_start, ai->length, ai->_rw->path().c_str());
                LOG_TRACE("@@bi.start = %llu , bi.src_start = %llu, bi.length = %llu, bi.rw.path = %s\r\n", bi->start, bi->src_start, bi->length, bi->_rw->path().c_str());
                T ta(ai->start, ai->length, ai->src_start, ai->_rw);
                do {
                    LOG_TRACE("@@ta.start = %llu , ta.src_start = %llu, ta.length = %llu, ta.rw.path = %s\r\n", ta.start, ta.src_start, ta.length, ta._rw->path().c_str());
                    int result = system_tools::region_complement<T>(ta, *bi, rc, rd); //return 1 has inter, 0 no inter and a is small, -1 no inter and b is small
                    if (1 == result)
                    {
                        if (rc.length != 0)
                        {
                            LOG_TRACE("rc.start = %llu , rc.src_start = %llu, rc.length = %llu, rc.rw.path = %s\r\n", rc.start, rc.src_start, rc.length, rc._rw->path().c_str());
                            c.push_back(rc);
                        }
                    }
                    else if (0 == result)
                    {
                        if (new_base)
                        {
                            new_base = false;
                            LOG_TRACE("tapush.start = %llu , tapush.src_start = %llu, tapush.length = %llu, tapush.rw.path = %s\r\n", ta.start, ta.src_start, ta.length, ta._rw->path().c_str());
                            c.push_back(ta);
                        }
                    }

                    ta.start = rd.start;
                    ta.length = rd.length;
                    ta.src_start = rd.src_start;
                    ta._rw = rd._rw;
                    if (rd.length != 0)
                    {
                        LOG_TRACE("rd.start = %llu , rd.src_start = %llu, rd.length = %llu, rd.rw.path = %s\r\n", rd.start, rd.src_start, rd.length, rd._rw->path().c_str());
                        if (bi != b.end())
                        {
                            LOG_TRACE("b++1\r\n");
                            ++bi;
                            if (bi != b.end())
                            {
                                LOG_TRACE("@@bi.start = %llu , bi.src_start = %llu, bi.length = %llu, bi.rw.path = %s\r\n", bi->start, bi->src_start, bi->length, bi->_rw->path().c_str());
                            }
                        }
                        if (bi == b.end())
                        {
                            LOG_TRACE("rd.start = %llu , rd.src_start = %llu, rd.length = %llu, rd.rw.path = %s\r\n", rd.start, rd.src_start, rd.length, rd._rw->path().c_str());
                            c.push_back(rd);
                            break;
                        }
                    }
                } while (rd.length != 0);
                uint64_t a_end = ai->start + ai->length - 1, b_end = bi->start + bi->length - 1;
                if (a_end <= b_end)
                {
                    if (ai != a.end())
                    {
                        LOG_TRACE("a++2\r\n");
                        ++ai;
                        new_base = true;
                    }
                }
                else
                {
                    if (bi != b.end())
                    {
                        LOG_TRACE("b++2\r\n");
                        ++bi;
                    }
                }
            }
            else
            {
                c.insert(c.end(),ai, a.end());
                break;
            }
        }
    }

    template<class T, class T_vtr, class T_vtr_itr, class T_map>
    static bool regions_vectors_maps_complement(T_map & a, T_map & b, T_map & c)
    {
        for (auto & ai : a)
        {
            for (auto & bi : b)
            {
                if (ai.first == bi.first)
                {
                    c[ai.first] = T_vtr();
                    system_tools::regions_vectors_complement<T, T_vtr, T_vtr_itr>(ai.second, bi.second, c[ai.first]);
                }
            }
        }
    }

    template<class T, class T_vtr, class T_vtr_itr, class T_map>
    static bool regions_vectors_maps_intersection(T_map & a, T_map & b, T_map & c)
    {
        for (auto & ai : a)
        {
            for (auto & bi : b)
            {
                if (ai.first == bi.first)
                {
                    c[ai.first] = T_vtr();
                    system_tools::regions_vectors_intersection<T, T_vtr, T_vtr_itr>(ai.second, bi.second, c[ai.first]);
                }
            }
        }
    }
	char * get_sysname() { return un.sysname; }
	char * get_nodename() { return un.nodename; }
	char * get_release() { return un.release; }
	char * get_version() { return un.version; }
	char * get_machine() { return un.machine; }
	char * get_domainname() { return un.domainname; }
private:
	struct utsname un;
};

namespace json_tools
{
    const mValue& find_value(const json_spirit::mObject& obj, const std::string& name);
    const bool find_value_bool(const mObject& obj, const std::string& name, bool default_value = false);
    const std::string find_value_string(const mObject& obj, const std::string& name);
    const int find_value_int32(const json_spirit::mObject& obj, const std::string& name, int default_value = 0);
    const mArray& find_value_array(const mObject& obj, const std::string& name);
    template<typename typeA>
    typeA boost_json_get_value_helper(ptree tree, const char * input , typeA default_value)
    {
        typeA return_value;
        try {
            return_value = tree.get<typeA>(input);
        }
        catch (...)
        {
            return_value = default_value;
        }
        return return_value;
    }
    ptree boost_json_get_child_helper(ptree tree, const char * input, ptree default_value);
}

class service_control_helper
{
private:
    string service_comtrol_command;
    string service_name;
    bool service;
public:
    service_control_helper(string _service_name) :service(false){
        service_name = _service_name;
        string result = system_tools::execute_command("command -v initctl");
        if (!result.empty())
            service_comtrol_command = "initctl";
        else
        {
            result = system_tools::execute_command("command -v systemctl");
            if (!result.empty())
            {
                service_comtrol_command = "systemctl";
            }
            else
            {
                result = system_tools::execute_command("command -v service");
                if (!result.empty())
                {
                    service = true;
                    service_comtrol_command = "service";
                }
            }
        }
    }
    bool start();
    bool stop();
};
#endif