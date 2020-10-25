#include <iostream>
#include <fstream>
#include <memory>
#include <stdio.h>
#include "system_tools.h"

#include "log.h"

#include <sys/ioctl.h>
#include <net/if.h> 
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace std;

bool system_tools::version::operator >(const system_tools::version & rhs) const
{
    vector<uint64_t> rhs_uint64Vec = rhs.get_uint64Vec();
    if (uint64Vec.size() != rhs_uint64Vec.size())
    {
        throw system_tools::version::version_exception();
    }
    bool result = false;
    for (int i=0;i<uint64Vec.size();++i)
    {
        if (uint64Vec[i] > rhs_uint64Vec[i])
            return true;
        else if(uint64Vec[i] < rhs_uint64Vec[i])
            return false;
    }
    return false;
}

bool system_tools::version::operator <(const system_tools::version & rhs) const
{
    vector<uint64_t> rhs_uint64Vec = rhs.get_uint64Vec();
    if (uint64Vec.size() != rhs_uint64Vec.size())
    {
        throw system_tools::version::version_exception();
    }
    bool result = false;
    for (int i = 0; i<uint64Vec.size(); ++i)
    {
        if (uint64Vec[i] < rhs_uint64Vec[i])
            return true;
        else if (uint64Vec[i] > rhs_uint64Vec[i])
            return false;
    }
    return false;
}

bool system_tools::version::operator ==(const system_tools::version & rhs) const
{
    vector<uint64_t> rhs_uint64Vec = rhs.get_uint64Vec();
    if (uint64Vec.size() != rhs_uint64Vec.size())
    {
        throw system_tools::version::version_exception();
    }
    bool result = false;
    for (int i = 0; i<uint64Vec.size(); ++i)
    {
        if (uint64Vec[i] != rhs_uint64Vec[i])
            return false;
    }
    return true;
}


string system_tools::analysis_ip_address(string input)
{
    vector<string> strVec, strVec2;
    split(strVec, input, is_any_of("."));
    string ip_address;
    bool is_ip_address = true;
    if (strVec.size() == 4)
    {
        for (auto & sv : strVec)
        {
            char * endptr;
            long int num = strtol(sv.c_str(), &endptr, 10);
            /*any string is no number*/
            if ((endptr != sv.c_str() + sv.size()) ||
                (num > 256 || num < 0))
            {
                is_ip_address = false;
                break;
            }
        }
    }
    else
        is_ip_address = false;
    if (is_ip_address)
        return input;
    else
    {
        string cmd = "nslookup " + input + " | grep Address:";
        string result = system_tools::execute_command(cmd.c_str());
        LOG_TRACE("result = %s", result.c_str());
        split(strVec, result, is_any_of("\n"));
        for (auto & sv1 : strVec)
        {
            if (sv1.size() == 0)
                break;
            LOG_TRACE("sv1.c_str() =  %s", sv1.c_str());
            int address_offset = sv1.find("Address:");
            int Address_size = string("Address:").size();
            ip_address = string_op::remove_trailing_whitespace(string_op::remove_begining_whitespace(sv1.substr(address_offset + Address_size, sv1.size())));
            split(strVec2, ip_address, is_any_of("."));
            is_ip_address = true;
            if (strVec2.size() == 4)
            {
                for (auto & sv : strVec2)
                {
                    char * endptr;
                    long int num = strtol(sv.c_str(), &endptr, 10);
                    /*any string is no number*/
                    if ((endptr != sv.c_str() + sv.size()) ||
                        (num > 256 || num < 0))
                    {
                        is_ip_address = false;
                        ip_address.clear();
                        break;
                    }
                }
            }
            if (is_ip_address == true)
                break;
        }
    }
    return ip_address;
}

string system_tools::get_execution_file_path_linux()
{
    char buf[1024];
    GetExecutionFilePath_Linux(buf);
    //printf("get_execution_file_path_linux:buf = %s\r\n", buf);
    return string(buf);
}
string system_tools::get_execution_command_line()
{
    string out;
    ifstream is(PROC_COMMAND_LINE, ifstream::in);
    if (is)
    {
        getline(is, out);
        is.close();
    }
    return out;
}

string system_tools::execute_command(const char* cmd)
{
    char buffer[128];
    std::string result = "";
    LOG_TRACE("cmd = %s\r\n", cmd);
    FILE* pipe = popen(cmd, "r");
    if (!pipe) throw std::runtime_error("popen() failed!");
    try {
        while (!feof(pipe)) {
            if (fgets(buffer, 128, pipe) != NULL)
                result += buffer;
        }
    }
    catch (...) {
        pclose(pipe);
        throw;
    }
    pclose(pipe);
    //LOG_TRACE("result = %s\r\n", result.c_str());
    return result;
}

string system_tools::path_remove_last(string input)
{
    filesystem::path path(input);
    return path.parent_path().string() + "/";
}
int system_tools::get_cpu_info(cpu_info & ci)
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

int system_tools::get_gateway(std::vector<std::string> & gateway)
{
    char line[1024];
    char token[] = "nameserver ";
    fstream fs;
    boost::filesystem::path p("/proc/net/route");
    if (boost::filesystem::exists(p))
    {
        fs.open(p.string().c_str(), ios::in);
        if (fs)
        {
            while (fs.getline(line, sizeof(line), '\n'))
            {
                LOG_TRACE("%s", line);
                string sl(line);
                int location = sl.find("eth0");
                if (location != string::npos)
                {
                    sl = string_op::remove_begining_whitespace(sl.substr(location+sizeof("eth0")-1));
                    LOG_TRACE("%s", sl.c_str());
                    if (sl.compare(0, 8, "00000000") == 0)//
                    {
                        sl = string_op::remove_begining_whitespace(sl.substr(8));
                        LOG_TRACE("2%s", sl.c_str());
                        int gate[4];
                        sscanf(sl.c_str(), "%2x%2x%2x%2x", &gate[3], &gate[2], &gate[1], &gate[0]);
                        LOG_TRACE("%d.%d.%d.%d", gate[0], gate[1], gate[2], gate[3]);
                        gateway.push_back(string_op::strprintf("%d.%d.%d.%d", gate[0], gate[1], gate[2], gate[3]));
                    }
                }
            }
            fs.close();
        }
    }
}

int system_tools::get_dns_servers(std::vector<std::string> & dns_servers)
{
    char line[1024];
    char token[] = "nameserver ";
    fstream fs;
    boost::filesystem::path p("/etc/resolv.conf");
    if (boost::filesystem::exists(p))
    {
        fs.open(p.string().c_str(), ios::in);
        if (fs)
        {
            while (fs.getline(line, sizeof(line), '\n'))
            {
                string sl(line);
                int location = sl.find(token);
                if (location != std::string::npos)
                {
                    dns_servers.push_back(sl.substr(location + sizeof(token)-1));
                }
            }
            fs.close();
        }
    }
}


vector<string> system_tools::get_ip_address()
{
    vector<string> return_value;
    string cmd = "command -v ip";
    string result = system_tools::execute_command(cmd.c_str());
    if (!result.empty())
    {
        cmd = string("ip addr show | grep inet");
        result = system_tools::execute_command(cmd.c_str());
        vector<string> strVec;
        split(strVec, result, is_any_of("/ \n"));
        bool next_is_ip = false;
        for (auto & s : strVec)
        {
            if (s == "inet")
                next_is_ip = true;
            if (next_is_ip)
            {
                vector<string> strVec3;
                split(strVec3, s, is_any_of("."));
                if (strVec3.size() == 4)
                {
                    if (s != "0.0.0.0" && s != "127.0.0.1")
                        return_value.push_back(s);
                    next_is_ip = false;
                }
            }
        }
    }
    else {
        cmd = "command -v ifconfig";
        result = system_tools::execute_command(cmd.c_str());
        if (!result.empty())
        {
            cmd = string("ifconfig | grep inet");
            result = system_tools::execute_command(cmd.c_str());
            vector<string> strVec;
            split(strVec, result, is_any_of(":/ \n"));
            bool next_is_ip = false;
            for (auto & s : strVec)
            {
                if (s == "inet")
                    next_is_ip = true;
                if (next_is_ip)
                {
                    vector<string> strVec3;
                    split(strVec3, s, is_any_of("."));
                    if (strVec3.size() == 4)
                    {
                        if (s != "0.0.0.0" && s != "127.0.0.1")
                            return_value.push_back(s);
                        next_is_ip = false;
                    }
                }
            }
        }
    }
    for (auto & a : return_value)
        LOG_TRACE("%s",a.c_str());
    return return_value;
}

int system_tools::get_mac_address(unsigned char mac_address[6])
{
    FUNC_TRACER;
    struct ifreq ifr;
    struct ifconf ifc;
    char buf[1024];
    int success = 0;

    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock == -1) { return -1; };

    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = buf;
    if (ioctl(sock, SIOCGIFCONF, &ifc) == -1) { return -1; }

    struct ifreq* it = ifc.ifc_req;
    const struct ifreq* const end = it + (ifc.ifc_len / sizeof(struct ifreq));

    for (; it != end; ++it) {
        strcpy(ifr.ifr_name, it->ifr_name);
        if (ioctl(sock, SIOCGIFFLAGS, &ifr) == 0) {
            if (!(ifr.ifr_flags & IFF_LOOPBACK)) { // don't count loopback
                if (ioctl(sock, SIOCGIFHWADDR, &ifr) == 0) {
                    success = 1;
                    break;
                }
            }
        }
        else { return -1; }
    }
    if (success) memcpy(mac_address, ifr.ifr_hwaddr.sa_data, 6);
    return 0;
}

int system_tools::get_mac_address_by_command(unsigned char mac_address[6])
{
    FUNC_TRACER;
    string result = REMOVE_WITESPACE(system_tools::execute_command("ip link show | grep link/ether"));
    std::vector<string> strVec;
    std::vector<string> strVec2;
    boost::split(strVec, result, boost::is_any_of(" "));
    LOG_TRACE("strVec.size() = %d", strVec.size())
    if (strVec.size() <= 2)
    {
        return -1;
    }
    boost::split(strVec2, strVec[1], boost::is_any_of(":"));
    LOG_TRACE("strVec2.size() = %d", strVec2.size())
    if(strVec2.size()!=6)
    {
        return -1;
    }
    for (int i = 0; i < 6; ++i)
    {
        mac_address[i] = stoi(strVec2[i], 0, 16);
    }
    return 0;
}


int system_tools::get_machine_id(string& suuid) //use MAC address
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
int system_tools::get_total_memory(int& total_memory)
{
    /*string test = system_tools::execute_command("echo aaaaaaaaaaaaaaaaaaaaaaa");
    LOG_TRACE("test = %s", test.c_str());*/
    string cmd_result;
    try
    {
        cmd_result = system_tools::execute_command("sudo dmidecode -t memory -q | grep 'Size: ' | egrep -v 'Installed|Enabled|Maximum' | sed 's/.*[:]//'|sed 's/[(].*//' | sed 's/ //g'");
    }
    catch (std::runtime_error ex)
    {
        LOG_TRACE("execute_command failed");
    }
    //printf("cmd_result = %s", cmd_result.c_str());
    fstream fs;
    LOG_TRACE("cmd_result = %s", cmd_result.c_str());
    total_memory = 0;
    /*open .memory*/
    int from_memory_file = 0;
    int from_meminfo = 0;
    if (cmd_result.empty())
    {
        fs.open(".memory", ios::in);
        if (fs)
        {
            LOG_TRACE(".memory open finish failed");
            char buffer[1024];
            while (fs.getline(buffer, sizeof(buffer), '\n'))
            {
                int memorycount;
                if (sscanf(buffer,"%dMB",&memorycount) ==1)
                    from_memory_file += memorycount;
            }
            fs.close();
            LOG_TRACE("from_memory_file = %d", from_memory_file);
        }     
        char line[1024];
        /*gen the uuid and save it to the specific file*/
        /*open the file first*/
        boost::filesystem::path p("/proc/meminfo");
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
                        sscanf(strVec[1].c_str(), "%d", &from_meminfo);
                        from_meminfo /= 1024;
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
        LOG_TRACE("from_meminfo = %d", from_meminfo);
        if ((from_memory_file - from_meminfo > 300) || from_memory_file==0)/*different over 300MB*/
            total_memory = (((from_meminfo-1)>>9)+1)<<9;
        else
            total_memory = from_memory_file;
    } 
    else
    {
        vector<string> strVec;
        int mem = 0;        
        boost::split(strVec, cmd_result, is_any_of("\n"));
        for (auto &a : strVec)
        {
            if (sscanf(a.c_str(), "%dMB", &mem) == 1)
            {
                total_memory += mem;
            }
        }
    }
    return 0;
error:
    if (fs.is_open())
        fs.close();
    return -1;
}

string system_tools::get_files_mount_point_by_command(string filename)
{
    LOG_TRACE("string & filename = %s", filename.c_str());
    string commands = "df \"" + filename + "\" | tail -1 | awk \'{ print $6 }\'";
    string result = system_tools::execute_command(commands.c_str());
    LOG_TRACE("result = %s", result.c_str());
    int change_line_location = result.find("\n");
    if (change_line_location != std::string::npos)
    {
        result = result.substr(0, change_line_location);
    }
    if (result[result.size() - 1] != '/')
    {
        LOG_TRACE("")
        result += '/';
    }
    return result;
}


string system_tools::get_files_mount_point(string filename, std::set<string> mounted_points)
{
    int size = 0;
    string result;
    for (auto & a : mounted_points)
    {
        LOG_TRACE("mounted_points = %s", a.c_str());
        int index = filename.find(a);
        LOG_TRACE("index = %d", index);
        if (index != std::string::npos)
        {
            LOG_TRACE("a.size()=%d, size = %d", a.size(), size);
            if (a.size() > size && index == 0)
            {
                size = a.size();
                result = a;
                LOG_TRACE("result = %s", result.c_str());
            }
        }
    }
    return result;
}

//static int do_mkdir(char * path, int mode)
//{
//    struct stat buf;
//    int status;
//    if (stat(path, &buf) != 0)
//    {
//        if (mkdir(path, mode) != 0 && errno != EEXIST)
//            return -1;
//    }
//    else if (!S_ISDIR(path))
//    {
//        errno = ENOTDIR;
//        status = -1;
//    }
//    return status;
//}



int system_tools::get_os_pretty_name_and_info(string& pretty_name, string& arch, int & major, int & minor)
{
    std::vector<std::string> os_name_files_array;
    std::string dis_name, version;
    os_name_files_array.push_back("/etc/centos-release");
    os_name_files_array.push_back("/etc/os-release");
    //os_name_files_array.push_back("/etc/SuSE-release");
    int found = false;
    char line[1024];
    /*gen the uuid and save it to the specific file*/
    /*open the file first*/
    fstream fs;
    arch = system_tools::execute_command("uname -m");
    for (auto & file : os_name_files_array)
    {
        LOG_TRACE("file = %s", file.c_str());
        boost::filesystem::path p(file);
        if (!boost::filesystem::exists(p))
            continue;
        else
        {
            fs.open(p.string().c_str(), ios::in);
            if (!fs)
                continue;
            else
            {
                if (file == os_name_files_array[0])/*/etc/centos-release*/
                {
                    dis_name = "CentOS";
                    while (fs.getline(line, sizeof(line), '\n'))
                    {
                        pretty_name = string(line);
                        vector<string> strVec;
                        boost::split(strVec, pretty_name, boost::is_any_of(" "));
                        for (auto & a : strVec)
                        {
                            vector<string> strVec2;
                            boost::split(strVec2, a, boost::is_any_of("."));
                            if (strVec2.size() == 2)
                            {
                                version = a;
                                major = std::stoi(strVec2[0]);
                                minor = std::stoi(strVec2[1]);
                                pretty_name = dis_name + " " + version + " " + arch;
                                if (!pretty_name.empty())
                                {
                                    found = true;
                                    fs.close();
                                    return !found;
                                }
                            }
                        }
                    }
                }
                else if (file == os_name_files_array[1])/*"/etc/os-release"*/
                {
                    while (fs.getline(line, sizeof(line), '\n'))
                    {
                        string sline(line);
                        int found_index = sline.find("NAME=\"");
                        if (found_index != std::string::npos && found_index == 0)
                        {
                            if (sline.find("Red") != std::string::npos)
                                dis_name = "RedHat";
                            else if (sline.find("SLES") != std::string::npos)
                                dis_name = "SuSE";
                            else if (sline.find("Ubuntu") != std::string::npos)
                                dis_name = "Ubuntu";
                            continue;
                        }
                        found_index = sline.find("VERSION_ID=\"");
                        if (found_index != std::string::npos && found_index == 0)
                        {
                            vector<string> strVec;
                            boost::split(strVec, sline, boost::is_any_of("\""));
                            if (!strVec[1].empty())
                            {
                                vector<string> strVec2;
                                boost::split(strVec2, strVec[1], boost::is_any_of("."));
                                if (strVec2.size() >= 2)
                                {
                                    major = stoi(strVec2[0]);
                                    minor = stoi(strVec2[1]);
                                    version = strVec2[0] + "." + strVec2[1];
                                    pretty_name = dis_name + " " + version + " " + arch;
                                    if (!pretty_name.empty())
                                    {
                                        found = true;
                                        fs.close();
                                        return !found;
                                    }
                                }
                            }
                        }
                    }
                }
                fs.close();
            }
        }
    }
    return !found;
}



std::string system_tools::get_fd_name(int fd)
{
    char buf[COMMON_BUFFER_SIZE], buf_link[COMMON_BUFFER_SIZE];
    memset(buf_link, 0, sizeof(buf_link));
    sprintf(buf, PROC_SELF_FD"/%d", fd);
    readlink(buf, buf_link, sizeof(buf_link));
    return std::string(buf_link);
}

int system_tools::get_disk_sector_size_sd(int fd)
{
    int sector_size = -1;
    if (ioctl(fd, BLKSSZGET, &sector_size) != 0)
    {
        return -1;
    }
    return sector_size;
}

int system_tools::get_disk_sector_size_hd(int fd)
{

    struct hd_driveid hd;
    if (!ioctl(fd, HDIO_GET_IDENTITY, &hd))
    {
        LOG_ERROR("HDIO_GET_IDENTITY %s\r\n", hd.serial_no);
        return -1;
    }
    else if (errno == -ENOMSG) {
        LOG_ERROR("No serial number available\n");
        return -1;
    }
    else {
        LOG_ERROR("ERROR: HDIO_GET_IDENTITY");
        return -1;
    }
    return hd.sector_bytes;
}

int system_tools::get_disk_sector_size(int fd)
{
    FUNC_TRACER;
    /*int sector_size = -1;
    std::string fd_name = system_tools::get_fd_name(fd);
    sector_size = system_tools::get_disk_sector_size_sd(fd);
    LOG_TRACE("sector_size = %llu\r\n", sector_size);
    if (sector_size != -1)
        return sector_size;
    sector_size = system_tools::get_disk_sector_size_hd(fd);
    if (sector_size != -1)
        return sector_size;*/
    return 512;
}

std::string system_tools::gen_random_uuid()
{
    std::string out;
    boost::uuids::random_generator gen;
    boost::uuids::uuid snuuid = gen();
    out = boost::uuids::to_string(snuuid);
    return out;
}

bool system_tools::copy_file(const char * src,const char * dst)
{
    FILE * src_fd = fopen(src,"r+");
    if (!src_fd)
    {
        LOG_ERROR("can't open %s", src);
        return false;
    }
    FILE * dst_fd = fopen(dst, "w+");
    if (!dst_fd)
    {
        LOG_ERROR("can't open %s", dst);
        fclose(src_fd);
        return false;
    }
    char buf[16384];
    while (!feof(src_fd))
    {
        int n = fread(buf, 1, 16384, src_fd);
        fwrite(buf, 1, n, dst_fd);
    }
    fclose(src_fd);
    fclose(dst_fd);
    return true;
}
/*this function would interseciton the regions, the src and _rw would base on the first parameter*/
/*template<class T>
bool system_tools::region_intersection(T & a, T & b, T & c)
{
    uint64_t a_end = a.start + a.length;
    int64_t a_src_diff = a.start - a.src_start;
    uint64_t b_end = b.start + b.length;
    uint64_t return_start = max(a.start, b.start);
    uint64_t return_end = min(a_end, b_end);
    if (return_start < return_end)
    {
        c.start = return_start;
        c.length = return_end + 1 - return_start;
        c.src_start = return_start + a_src_diff;
        c._rw = a._rw;
        return true;
    }
    else
        return false;
}*/

/*template<class T,class T_vtr, class T_vtr_itr>
bool system_tools::regions_vectors_intersection(T_vtr & a, T_vtr & b, T_vtr & c)
{
    for (T_vtr_itr ai = a.begin(), bi = b.begin(); ai != a.end() && bi != b.end();)
    {
        T rc;
        uint64_t a_end = ai->start + ai->length, b_end = bi->start + bi->length;
        if (ai->start < b_end)
            ++ai;
        else
            ++bi;
        if (system_tools::region_intersection<T>(*ai, *bi, rc))
            c.push_back(rc);
    }
}*/

/*template<class T, class T_vtr, class T_vtr_itr, class T_map>
bool system_tools::regions_vectors_maps_intersection(T_map & a, T_map & b, T_map & c)
{
    for (auto & ai : a)
    {
        for (auto & bi : b)
        {
            if (ai.first == bi.first)
            {
                c[ai.first] = new T_vtr();
                system_tools::regions_vectors_intersection<T, T_vtr, T_vtr_itr>(a.second, b.second, c[ai.first]);
            }
        }
    }
}*/

bool service_control_helper::start(){
    string command;
    if(service)
        command = service_comtrol_command +" "+ service_name + " start";
    else
        command = service_comtrol_command + " start " + service_name;
    string result = system_tools::execute_command(command.c_str());
}
bool service_control_helper::stop(){
    string command;
    if (service)
        command = service_comtrol_command + " " + service_name + " stop";
    else
        command = service_comtrol_command + " stop " + service_name;
    string result = system_tools::execute_command(command.c_str());
}

namespace json_tools
{
    const mValue& find_value(const json_spirit::mObject& obj, const std::string& name) {
        json_spirit::mObject::const_iterator i = obj.find(name);
        assert(i != obj.end());
        assert(i->first == name);
        return i->second;
    }

    const bool find_value_bool(const mObject& obj, const std::string& name, bool default_value) {
        mObject::const_iterator i = obj.find(name);
        if (i != obj.end()) {
            return i->second.get_bool();
        }
        return default_value;
    }

    const std::string find_value_string(const mObject& obj, const std::string& name) {
        mObject::const_iterator i = obj.find(name);
        if (i != obj.end()) {
            return i->second.get_str();
        }
        return "";
    }

    const int find_value_int32(const json_spirit::mObject& obj, const std::string& name, int default_value) {
        mObject::const_iterator i = obj.find(name);
        if (i != obj.end()) {
            return i->second.get_int();
        }
        return default_value;
    }
    static mArray NULLmArray = mArray();
    const mArray& find_value_array(const mObject& obj, const std::string& name) {
        mObject::const_iterator i = obj.find(name);
        if (i != obj.end())
            return i->second.get_array();
        return NULLmArray;  //warning, maybe some issue.
    }
    ptree boost_json_get_child_helper(ptree tree, const char * input, ptree default_value)
    {
        ptree return_value;
        try {
            return_value = tree.get_child(input);
        }
        catch (...)
        {
            return_value = default_value;
        }
        return return_value;
    }
}