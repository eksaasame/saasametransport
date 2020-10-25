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
#include "json_spirit/json_spirit.h"
#include "log.hpp"

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


#define GetExecutionFilePath_Linux(buf) do{readlink("/proc/self/exe", buf, sizeof(buf));}while(0)

#define INQUIRY_REPLY_LEN  0x60
#define SERIALINQUIRY { INQUIRY, 0x01, 0x80, 0x00, INQUIRY_REPLY_LEN, 0x00}
#define NORMALINQUIRY { INQUIRY, 0x00, 0x00, 0x00, INQUIRY_REPLY_LEN, 0x00}
#define PROC_PARTITIONS "/proc/partitions"
#define PROC_MOUNTSTATS "/proc/self/mountstats"
#define COMMON_BUFFER_SIZE 1024
#define PROC_SELF_FD "/proc/self/fd"

using namespace boost;
using namespace boost::property_tree;
using namespace std;
using namespace json_spirit;

//class disk_general_info
//{
//public:
//	/*disk_general_info(const disk_general_info &input) :major(input.major), minor(input.minor), disk_index(input.disk_index), blocks(input.blocks), 
//		filename(input.filename), ab_path(input.ab_path), sysfs_path(input.sysfs_path){}*/
//	disk_general_info() {}
//	int major;
//	int minor;
//	int disk_index;	//this is not system info
//	int blocks;
//	std::string filename;
//    std::string ab_path;
//    std::string sysfs_path;
//};
//
//struct partition_collection
//{
//	disk_general_info general_info;
//	int partition_start_offset;
//    std::string mounted_path;
//    std::string filesystem_type;
//};
//
//struct disk_collection
//{
//	disk_general_info general_info;
//    std::string serial_number;
//	ptree uri;
//    std::string string_uri;
//    std::string bus_type;
//	vector<partition_collection> partition_collection_vector;
//};

class system_tools
{
public:
    struct cpu_info
    {
        int logical_core_number;
        unordered_map<int, int> physical_core;

    };

	system_tools() {
		uname(&un);
	}

    static string get_execution_file_path_linux()
    {
        char buf[1024];
        GetExecutionFilePath_Linux(buf);
        return string(buf);
    }
    static string path_remove_last(string input)
    {
        filesystem::path path(input);
        return path.parent_path().string() + "/";
    }
    static int get_cpu_info(cpu_info & ci)
    {
        char line[1024];
        /*gen the uuid and save it to the specific file*/
        /*open the file first*/
        boost::filesystem::path p("/proc/cpuinfo");
        fstream fs;
        ci.logical_core_number = 0;
        if (boost::filesystem::exists(p))
        {
            fs.open(p.string().c_str(), ios::in);
            if (fs)
            {
                while (fs.getline(line, sizeof(line), '\n'))
                {
                    string sl(line);
                    vector<string> strVec;
                    split(strVec, sl, is_any_of(":"));
                    if (strVec[0].find("processor") != string::npos)
                        ++ci.logical_core_number;
                    else if (strVec[0].find("physical id") != string::npos)
                    {
                        int physical_id_number;
                        sscanf(strVec[1].c_str(), "%d", &physical_id_number);
                        ++ci.physical_core[physical_id_number];
                    }
                }
                fs.close();
            }
            else
                goto error;
        }
        else
            goto error;
        if (ci.logical_core_number)
            return 0;
        else
            goto error;
    error:
        if (fs.is_open())
            fs.close();
        return -1;
    }
    static int get_machine_id(string& suuid)
    {
        char line[1024];
        /*gen the uuid and save it to the specific file*/
        /*open the file first*/
        boost::filesystem::path p("/etc/machine-id");
        fstream fs;
        if (boost::filesystem::exists(p))
        {
            fs.open(p.string().c_str(), ios::in);
            if (fs)
            {
                fs.getline(line, sizeof(line), '\n');
                suuid = string(line);
                if (!suuid.size())
                    goto error;
                fs.close();
            }
            else
                goto error;
        }
        else
            goto error;
        return 0;
    error:
        if (fs.is_open())
            fs.close();
        return -1;
    }
    static int get_total_memory(int& total_memory)
    {
        char line[1024];
        /*gen the uuid and save it to the specific file*/
        /*open the file first*/
        boost::filesystem::path p("/proc/meminfo");
        fstream fs;
        if (boost::filesystem::exists(p))
        {
            fs.open(p.string().c_str(), ios::in);
            if (fs)
            {
                while (fs.getline(line, sizeof(line), '\n'))
                {
                    string sl(line);
                    vector<string> strVec;
                    split(strVec, sl, is_any_of(":"));
                    if (strVec[0].find("MemTotal") != string::npos)
                    {
                        sscanf(strVec[1].c_str(), "%d", &total_memory);
                        total_memory /= 1024;
                        break;
                    }
                }
                fs.close();
            }
            else
                goto error;
        }
        else
            goto error;
        return 0;
    error:
        if (fs.is_open())
            fs.close();
        return -1;
    }



    static std::string get_fd_name(int fd)
    {
        char buf[COMMON_BUFFER_SIZE], buf_link[COMMON_BUFFER_SIZE];
        memset(buf_link, 0, sizeof(buf_link));
        sprintf(buf, PROC_SELF_FD"/%d", fd);
        readlink(buf, buf_link, sizeof(buf_link));
        return std::string(buf_link);
    }

    static int get_disk_sector_size_sd(int fd)
    {
        int sector_size;
        if (ioctl(fd, BLKSSZGET, &sector_size) != 0)
        {
            return -1;
        }
        return sector_size;
    }

    static int get_disk_sector_size_hd(int fd)
    {

        struct hd_driveid hd;
        if (!ioctl(fd, HDIO_GET_IDENTITY, &hd))
        {
            LOG_ERROR("HDIO_GET_IDENTITY %s\r\n", hd.serial_no);
        }
        else if (errno == -ENOMSG) {
            LOG_ERROR("No serial number available\n");
        }
        else {
            perror("ERROR: HDIO_GET_IDENTITY");
        }
        return hd.sector_bytes;
    }

    static int get_disk_sector_size(int fd)
    {
        std::string fd_name = system_tools::get_fd_name(fd);
        printf("fd_name = %s\r\n", fd_name.c_str());
        if (fd_name.find("hd", 0) != string::npos)
        {
            printf("hd");
            return system_tools::get_disk_sector_size_hd(fd);

        }
        else if (fd_name.find("sd", 0) != string::npos)
        {
            printf("sd");
            return system_tools::get_disk_sector_size_sd(fd);
        }
        return -1;
    }

    static std::string gen_random_uuid()
    {
        std::string out;        
        boost::uuids::random_generator gen;
        boost::uuids::uuid snuuid = gen();
        out = boost::uuids::to_string(snuuid);
        return out;
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
    /*const json_spirit::mValue& find_value(const json_spirit::mObject& obj, const std::string& name);
    const std::string find_value_string(const json_spirit::mObject& obj, const std::string& name);
    const bool find_value_bool(const json_spirit::mObject& obj, const std::string& name, bool default_value = false);
    const int find_value_int32(const json_spirit::mObject& obj, const std::string& name, int default_value = 0);
    const json_spirit::mArray& find_value_array(const json_spirit::mObject& obj, const std::string& name);*/

    inline static const mValue& find_value(const json_spirit::mObject& obj, const std::string& name) {
        json_spirit::mObject::const_iterator i = obj.find(name);
        assert(i != obj.end());
        assert(i->first == name);
        return i->second;
    }

    inline const bool find_value_bool(const mObject& obj, const std::string& name, bool default_value = false) {
        mObject::const_iterator i = obj.find(name);
        if (i != obj.end()) {
            return i->second.get_bool();
        }
        return default_value;
    }

    inline const std::string find_value_string(const mObject& obj, const std::string& name) {
        mObject::const_iterator i = obj.find(name);
        if (i != obj.end()) {
            return i->second.get_str();
        }
        return "";
    }

    inline const int find_value_int32(const json_spirit::mObject& obj, const std::string& name, int default_value = 0) {
        mObject::const_iterator i = obj.find(name);
        if (i != obj.end()) {
            return i->second.get_int();
        }
        return default_value;
    }

    inline const mArray& find_value_array(const mObject& obj, const std::string& name) {
        mObject::const_iterator i = obj.find(name);
        if (i != obj.end())
            return i->second.get_array();
        return mArray();  //warning, maybe some issue.
    }
}

#endif