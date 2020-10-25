#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TSSLServerSocket.h>
#include <thrift/transport/TTransportUtils.h>
#include <thrift/async/TEvhttpClientChannel.h>
#include <thrift/async/TAsyncChannel.h>
#include <thrift/stdcxx.h>
#include <thrift/transport/TSSLSocket.h>
#include <utility>
#include "../tools/clone_disk.h"
#include "../tools/archive.h"
#include "../tools/service_status.h"
#include "../tools/thrift_helper.h"
#include "carrier_rw.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <dirent.h>
#include "compile_config.h"

using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
using namespace apache::thrift::async;

extern std::streambuf * backup;
extern std::ofstream out;

#include <boost/random/seed_seq.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/random/variate_generator.hpp>

#include "common_service_handler.h"
#include "../gen-cpp/saasame_constants.h"
#include "../tools/sslHelper.h"
#include <memory>
unordered_map<string, bus_type::type> bus_type_index;
//extern bool get_physicak_location_of_specify_file_by_command(std::vector<std::pair<uint64_t, uint64_t>> & inextents, const string & filename, const string & block_device, bool is_xfs, linuxfs::volume::ptr v);

void printf_disk_base(disk_baseA::ptr diskbase)
{
    LOG_TRACE("bisk_baseA.major = %d\r\n", diskbase->major);
    LOG_TRACE("bisk_baseA.minor = %d\r\n", diskbase->minor);
    LOG_TRACE("bisk_baseA.disk_index = %d\r\n", diskbase->disk_index);
    LOG_TRACE("bisk_baseA.blocks = %llu\r\n", diskbase->blocks);
    LOG_TRACE("bisk_baseA.device_filename = %s\r\n", diskbase->device_filename.c_str());
    LOG_TRACE("bisk_baseA.ab_path = %s\r\n", diskbase->ab_path.c_str());
    LOG_TRACE("bisk_baseA.sysfs_path = %s\r\n", diskbase->sysfs_path.c_str());
    LOG_TRACE("bisk_baseA.system_uuid = %s\r\n", diskbase->get_system_uuid().c_str());
}

void printf_partition(partitionA::ptr p)
{
    LOG_TRACE("===================partition====================\r\n");
    printf_disk_base(p);
    LOG_TRACE("partitions.partition_start_offset = %llu\r\n", p->partition_start_offset);
    LOG_TRACE("partitions.filesystem_type = %s\r\n", p->filesystem_type.c_str());
    LOG_TRACE("partitions.lvname = %s\r\n", p->lvname.c_str());
    LOG_TRACE("partitions.uuid = %s\r\n", p->uuid.c_str());
    if (p->mp != NULL)
    {
        auto &m = p->mp;
        LOG_TRACE("===================mounted_point====================\r\n");
        LOG_TRACE("mounted.get_device = %s\r\n", m->get_device().c_str());
        LOG_TRACE("mounted.get_mounted_on = %s\r\n", m->get_mounted_on().c_str());
        LOG_TRACE("mounted.get_fstype = %s\r\n", m->get_fstype().c_str());
        LOG_TRACE("mounted.get_system_uuid = %s\r\n", m->get_system_uuid().c_str());
    }
    if (!p->mps.empty())
    {
        for (auto & m : p->mps)
        {
            LOG_TRACE("===================mounted_point====================\r\n");
            LOG_TRACE("mounted.get_device = %s\r\n", m->get_device().c_str());
            LOG_TRACE("mounted.get_mounted_on = %s\r\n", m->get_mounted_on().c_str());
            LOG_TRACE("mounted.get_fstype = %s\r\n", m->get_fstype().c_str());
            LOG_TRACE("mounted.get_system_uuid = %s\r\n", m->get_system_uuid().c_str());
        }        
    }

    for (auto &l : p->lvms)
    {
        LOG_TRACE("===================lvms====================\r\n");
        printf_partition(l);
    }
}

void printf_disk(disk::ptr d)
{
    LOG_TRACE("===================disk====================\r\n");
    printf_disk_base(d);
    LOG_TRACE("disk.serial_number = %s\r\n", d->serial_number.c_str());
    LOG_TRACE("disk.string_uri = %s\r\n", d->string_uri.c_str());
    LOG_TRACE("disk.bus_type = %s\r\n", d->bus_type.c_str());
    LOG_TRACE("disk.sector_size = %llu\r\n", d->sector_size);
    LOG_TRACE("disk.filesystem_type = %s\r\n", d->filesystem_type.c_str());
    if (d->mp != NULL)
    {
        auto &m = d->mp;
        LOG_TRACE("===================mounted_point====================\r\n");
        LOG_TRACE("mounted.get_device = %s\r\n", m->get_device().c_str());
        LOG_TRACE("mounted.get_mounted_on = %s\r\n", m->get_mounted_on().c_str());
        LOG_TRACE("mounted.get_fstype = %s\r\n", m->get_fstype().c_str());
        LOG_TRACE("mounted.get_system_uuid = %s\r\n", m->get_system_uuid().c_str());
    }
    if (!d->mps.empty())
    {
        for (auto & m : d->mps)
        {
            LOG_TRACE("===================mounted_point====================\r\n");
            LOG_TRACE("mounted.get_device = %s\r\n", m->get_device().c_str());
            LOG_TRACE("mounted.get_mounted_on = %s\r\n", m->get_mounted_on().c_str());
            LOG_TRACE("mounted.get_fstype = %s\r\n", m->get_fstype().c_str());
            LOG_TRACE("mounted.get_system_uuid = %s\r\n", m->get_system_uuid().c_str());
        }
    }

    for (auto &p : d->partitions)
    {
        printf_partition(p);
    }
}


//bool get_physicak_location_of_specify_file_by_command_test(std::vector<std::pair<uint64_t, uint64_t>> & inextents, const set<string> & excluded_paths, const string & block_device, uint32_t block_size, bool is_xfs, linuxfs::volume::ptr v)
//{
//    //first parsing the filenames to 
//    for (auto & p : excluded_paths)
//    {
//        set<string> filenames;
//        boost::filesystem::directory_iterator end;
//        for (boost::filesystem::directory_iterator pos(p); pos!= end;++pos)
//        {
//            boost::filesystem::path path(*pos);
//            if (boost::filesystem::is_regular(path))
//            {
//                filenames.insert(path.c_str)
//            }
//            else if (boost::filesystem::is_directory(path))
//        }
//    }
//
//    FUNC_TRACER;
//    if (!is_xfs) //ext2
//    {
//        string O_DEBUGFS_PAGER = system_tools::execute_command("echo \"$DEBUGFS_PAGER\"");
//        if (O_DEBUGFS_PAGER.size() != 0)
//            system_tools::execute_command("export DEBUGFS_PAGER=");
//
//        string O_PAGER = system_tools::execute_command("echo \"$PAGER\"");
//        if (O_PAGER.size() != 0)
//            system_tools::execute_command("export PAGER=");
//        //remove the debugfs_pager and pager environment variable to NULL
//
//        string command = "debugfs -R \"stat " + filename + "\" " + block_device;
//        string result = system_tools::execute_command(command.c_str());
//        string target[] = { "EXTENTS:","BLOCKS:" };
//        int target_location = -1;
//        int i;
//        //LOG_TRACE("block_size = %llu", block_size);
//
//        for (i = 0; i < 2; i++)
//        {
//            target_location = result.find(target[i]);
//            if (target_location != -1)
//                break;
//        }
//        bool is_extent = (i == 0);
//        string extents = result.substr(target_location + target[i].size(), result.size());
//        //LOG_TRACE("extents = %s\r\n", extents.c_str());
//        vector<string> strVec;
//        boost::split(strVec, extents, boost::is_any_of(",:"));
//        bool is_index = false;
//        bool is_total = false;
//        bool is_ext0 = false;
//        //LOG_TRACE("block_size = %llu", block_size);
//        for (string & s : strVec)
//        {
//            //LOG_TRACE("s = %s\r\n", s.c_str());
//            if (s.find("IND") != std::string::npos)
//            {
//                is_index = true;
//                continue;
//            }
//            if (s.find("TOTAL") != std::string::npos)
//            {
//                is_total = true;
//                continue;
//            }
//            if (s.find("ETB") != std::string::npos)
//            {
//                is_ext0 = true;
//                continue;
//            }
//            if (s.find("(") == std::string::npos)
//            {
//                if (is_index)
//                {
//                    is_index = false;
//                    continue;
//                }
//                if (is_total)
//                {
//                    is_total = false;
//                    continue;
//                }
//                if (is_ext0)
//                {
//                    is_ext0 = false;
//                    continue;
//                }
//                vector<string> strVec2;
//                boost::split(strVec2, s, boost::is_any_of("-"));
//                if (strVec2.size() == 1)
//                {
//                    /*for (auto & strV2 : strVec2)
//                    {
//                    LOG_TRACE("strV2 = %s\r\n", strV2.c_str());
//                    }*/
//                    //LOG_TRACE("strVec2[0] = %s", strVec2[0].c_str());
//                    //LOG_TRACE("block_size[0] = %u", block_size);
//                    uint64_t start = std::stoll(strVec2[0], 0, 10) * block_size;
//                    //LOG_TRACE("start = %llu,lenght = %u \r\n", start, block_size);
//                    inextents.push_back(std::make_pair(start, block_size));
//                }
//                else
//                {
//                    uint64_t start = std::stoll(strVec2[0], 0, 10);
//                    uint64_t end = std::stoll(strVec2[1], 0, 10);
//                    //LOG_TRACE("start = %llu,lenght = %llu \r\n", start, end - start + 1);
//                    inextents.push_back(std::make_pair(start*block_size, (end - start + 1)*block_size));
//                }
//            }
//            else //if there are the index of  the  blocks
//            {
//                vector<string> strVec2;
//                boost::split(strVec2, s, boost::is_any_of("([]-)"));
//                if (strVec2.size() == 4)
//                {
//                    uint64_t start = std::stoll(strVec2[1], 0, 10);
//                    uint64_t end = std::stoll(strVec2[2], 0, 10);
//                }
//                else
//                    uint64_t end = std::stoll(strVec2[1], 0, 10);
//            }
//        }
//
//        if (O_DEBUGFS_PAGER.size() != 0)
//        {
//            string cmd = "export DEBUGFS_PAGER=" + O_DEBUGFS_PAGER;
//            system_tools::execute_command(cmd.c_str());
//        }
//        if (O_PAGER.size() != 0)
//        {
//            string cmd = "export PAGER=" + O_PAGER;
//            system_tools::execute_command(cmd.c_str());
//        }
//    }
//    else
//    {
//        std::vector<std::pair<uint64_t, uint64_t>> inextents2;
//        //LOG_TRACE("XFS MUMI!!!!!!!!!!!!");
//        struct stat var;
//        if (-1 == stat(filename.c_str(), &var))
//        {
//            LOG_ERROR("get %s status error.", filename.c_str());
//            return false;
//        }
//        string command = "xfs_metadump " + block_device + " /.dattodump";
//        string result = system_tools::execute_command(command.c_str());
//        if (result.size() != 0)
//        {
//            LOG_ERROR("%s", result.c_str());
//            return false;
//        }
//        command = "xfs_mdrestore /.dattodump /.dattofs";
//        result = system_tools::execute_command(command.c_str());
//        if (result.size() != 0)
//        {
//            LOG_ERROR("%s", result.c_str());
//            return false;
//        }
//        remove("/.dattodump");
//        command = "xfs_repair -L /.dattofs";
//        result = system_tools::execute_command(command.c_str());
//        if (result.size() != 0)
//        {
//            LOG_ERROR("%s", result.c_str());
//            return false;
//        }
//        command = "xfs_db -c \"inode " + std::to_string(var.st_ino) + "\" -c \"bmap\" /.dattofs";
//        result = system_tools::execute_command(command.c_str());
//        if (result.size() != 0)
//        {
//            //LOG_TRACE("result = %s", result.c_str());
//            uint64_t data_offset = 0;
//            uint64_t startblock = 0;
//            uint64_t before = 0;
//            uint64_t after = 0;
//            uint64_t count = 0;
//            uint64_t offset = 0;
//            const char * result_start = result.c_str();
//            int flag = 0;
//            std::string line;
//            while (sscanf(result_start, "data offset %llu startblock %llu (%llu/%llu) count %llu flag %d\n%n", &data_offset, &startblock, &before, &after, &count, &flag, &offset) == 6)
//            {
//                uint64_t blocks = before * v->get_sb_agblocks() + after;
//                result_start += offset;
//                inextents.push_back(std::make_pair(blocks * block_size, count *block_size));
//                //LOG_TRACE("push_back, %llu , %llu", blocks * block_size, count *block_size);
//            }
//        }
//        else
//        {
//            command = "xfs_db -c \"blockget -i " + std::to_string(var.st_ino) + "\" /.dattofs | grep / | sed 's/.*block //'";
//            result = system_tools::execute_command(command.c_str());
//            //LOG_TRACE("result = %s", result.c_str());
//            vector<string> strVec;
//            boost::split(strVec, result, boost::is_any_of("\n"));
//            if (v == NULL)
//            {
//                remove("/.dattofs");
//                return false;
//            }
//            uint64_t first_start = 0;
//            uint64_t previous_start = 0;
//            uint64_t lenght = 0;
//            for (string & s : strVec)
//            {
//                vector<string> strVec2;
//                boost::split(strVec2, s, boost::is_any_of("/"));
//                /**/
//
//                if (strVec2.size() == 2)
//                {
//                    uint64_t offset = (std::stoll(strVec2[0], 0, 10) * v->get_sb_agblocks() + std::stoll(strVec2[1], 0, 10))* block_size;
//                    if (first_start == 0)
//                    {
//                        first_start = offset;
//                        previous_start = offset;
//                        lenght = 1;
//                    }
//                    else
//                    {
//                        if ((previous_start + block_size) != offset)
//                        {
//                            inextents.push_back(std::make_pair(first_start, lenght *block_size));
//                            //LOG_TRACE("2push_back, %llu , %llu", first_start, lenght *block_size);
//                            previous_start = first_start = offset;
//                            lenght = 1;
//                        }
//                        else
//                        {
//                            previous_start = offset;
//                            lenght++;
//                        }
//                    }
//                }
//            }
//            if (lenght)
//            {
//                inextents.push_back(std::make_pair(first_start, lenght *block_size));
//                //LOG_TRACE("2push_back, %llu , %llu", first_start, lenght *block_size);
//            }
//        }
//        //if (s.find("extent") == std::string::npos) //
//        //{
//        //    vector<string> strVec2;
//        //    boost::split(strVec2, s, boost::is_any_of(","));
//        //    /*plz add error handle*/
//        //    if (strVec2.size() == 4)
//        //    {
//        //        uint64_t start = std::stoll(strVec2[1], 0, 10) *block_size;
//        //        LOG_TRACE("start = %llu,lenght = %llu\r\n", start, std::stoll(strVec2[2], 0, 10) *block_size);
//        //        inextents.push_back(std::make_pair(start, std::stoll(strVec2[2], 0, 10) *block_size));
//        //    }
//        //}
//        remove("/.dattofs");
//    }
//    return true;
//}


void create_config_file()
{
    FUNC_TRACER;
    boost::filesystem::path p = boost::filesystem::path(system_tools::get_execution_file_path_linux()).parent_path();
    p = p / ".hostConfig";
    LOG_TRACE("%s", p.string().c_str());
    if (!boost::filesystem::exists(p))
    {
        fstream fs;
        fs.open(p.string().c_str(), fstream::out);
        if (fs)
        {
            LOG_TRACE("create succuess");
            /*create uuid*/


            unsigned char mac[6];
            string out;
            boost::uuids::uuid snuuid;
            int macret = system_tools::get_mac_address_by_command(mac);
            if(macret)
                macret = system_tools::get_mac_address(mac);
            if (!macret)
            {
                boost::random::seed_seq seed{ mac[0] ,mac[1], mac[2], mac[3], mac[4], mac[5] };
                boost::mt19937 ran(seed);
                boost::uuids::basic_random_generator<boost::mt19937> gen(&ran);
                snuuid = gen();
            }
            else
            {
                boost::uuids::random_generator gen;
                snuuid = gen();
            }
            out = std::to_string(PRODUCT_MAJOR) + "." + std::to_string(PRODUCT_MINOR) + "." + std::to_string(PRODUCT_BUILD);
            if (out.size())
            {
                out = "version:" + out + "\n";
                fs.write(out.c_str(), out.size());
            }
            out = boost::uuids::to_string(snuuid);
            if (out.size())
            {
                out = "uuid:" + out + "\n";
                fs.write(out.c_str(), out.size());
            }
            /*write the default g_level*/
            out = "verbose:" + to_string(global_verbose);
            fs.write(out.c_str(), out.size());
            LOG_TRACE("out = %s\r\n", out.c_str());
            /*close fs*/
            fs.close();
        }
    }
}

void write_config_file_with_context(std::string * version, std::string * uuid, int * verbose, bool * is_skip_read_error, std::map<std::string, int> * snapshots_cow_space_user_config, uint64_t * merge_size)
{
    boost::filesystem::path p = boost::filesystem::path(system_tools::get_execution_file_path_linux()).parent_path();
    p = p / ".hostConfig";
    
    fstream fs;
    fs.open(p.string().c_str(), fstream::out);
    if (fs)
    {
        LOG_RECORD("create succuess");
        string out;
        out = "version:" + *version + "\n";
        fs.write(out.c_str(), out.size());
        
        
        out = "uuid:" + *uuid + "\n";
        fs.write(out.c_str(), out.size());
        /*write the default g_level*/
        out = "verbose:" + std::to_string(*verbose) + "\n";
        fs.write(out.c_str(), out.size());
        if (*is_skip_read_error)
        {
            out = "skip_read_error:1\n";
            fs.write(out.c_str(), out.size());
        }
        if (*merge_size!=-1)
        {
            out = "mergesize:" + std::to_string(*merge_size) + "\n";
            fs.write(out.c_str(), out.size());
        }
        for (auto & cow_config : *snapshots_cow_space_user_config)
        {
            out = "cow_config:" + cow_config.first +":"+ std::to_string(cow_config.second) +"\n";
            fs.write(out.c_str(), out.size());
        }

        /*close fs*/
        fs.close();
    }
    
}

void read_config_file(std::string * version, std::string * uuid, int * verbose,bool * is_skip_read_error,std::map<std::string,int> * snapshots_cow_space_user_config, uint64_t * merge_size)
{
    FUNC_TRACER;
    char line[1024];
    boost::filesystem::path p = boost::filesystem::path(system_tools::get_execution_file_path_linux()).parent_path();
    p = p / ".hostConfig";
    fstream fs;
    if (boost::filesystem::exists(p))
    {
        fs.open(p.string().c_str(), ios::in | ios::out);
        LOG_TRACE("p=%s", p.string().c_str());
        if (fs)
        {
            LOG_TRACE("open succuess");
            while (fs.getline(line, sizeof(line), '\n'))
            {
                LOG_TRACE("2");
                string sl(line);
                vector<string> strVec;
                split(strVec, sl, is_any_of(":"));
                if (strVec.size() == 1) {}
                else if (strVec.size() == 2)
                {
                    if (!strVec[0].compare("uuid") && uuid!=NULL)
                        *uuid = REMOVE_WITESPACE(strVec[1]);
                    else if (!strVec[0].compare("version") && uuid != NULL)
                        *version = REMOVE_WITESPACE(strVec[1]);
                    else if (!strVec[0].compare("mergesize") && uuid != NULL)
                    {
                        *merge_size = stoi(REMOVE_WITESPACE(strVec[1]));
                        if (*merge_size != 0)
                        {
                            *merge_size = (*merge_size << 16) >> 16;
                            *merge_size = (*merge_size > 4 * 1024 * 1024) ? 4 * 1024 * 1024 : (*merge_size < 64 * 1024) ? 64 * 1024 : *merge_size;
                        }
                        LOG_TRACE("merge_size = %llu", *merge_size);
                    }
                    else if (!strVec[0].compare("verbose") && verbose != NULL)
                    {
                        *verbose = stoi(REMOVE_WITESPACE(strVec[1]));
                    }
                    else if (!strVec[0].compare("skip_read_error"))
                        *is_skip_read_error = (stoi(REMOVE_WITESPACE(strVec[1])) == 1) ? 1 : 0;
                }
                else if (strVec.size() == 3)
                {
                    if (!strVec[0].compare("cow_config"))
                    {
                        if (snapshots_cow_space_user_config != NULL)
                        {
                            (*snapshots_cow_space_user_config)[REMOVE_WITESPACE(strVec[1])] = stoi(REMOVE_WITESPACE(strVec[2]));
                            LOG_TRACE("%s", REMOVE_WITESPACE(strVec[1]).c_str());
                            LOG_TRACE("%d", stoi(REMOVE_WITESPACE(strVec[2])));
                        }
                    }
                }
            }
            fs.close();
        }
    }
}

void common_service_handler::init_config()
{
    char line[1024];
    /*gen the uuid and save it to the specific file*/
    /*open the file first*/
    LOG_RECORD("common_service_handler::init_config");
    boost::filesystem::path p = boost::filesystem::path(system_tools::get_execution_file_path_linux()).parent_path();
    p = p / ".hostConfig";
    version = std::to_string(PRODUCT_MAJOR) + "." + std::to_string(PRODUCT_MINOR) + "." + std::to_string(PRODUCT_BUILD);
    create_config_file();
    read_config_file(&_hc.host_version,&_hc.host_uuid, &global_verbose, &is_skip_read_error, &snapshots_cow_space_user_config,&merge_size);
    _hc.host_version = version;
    write_config_file_with_context(&_hc.host_version, &_hc.host_uuid, &global_verbose, &is_skip_read_error, &snapshots_cow_space_user_config, &merge_size);
    for (auto & sh : snapshots_cow_space_user_config)
    {
        LOG_TRACE("%s : %d", sh.first.c_str(), sh.second);
    }
    /*init the bus_type_index here*/
    INIT_BUS_TYPE_MAP;
    /*this is just test*/
    get_host_general_info_fake();
    //str = storage::get_storage();
    /*int total_memory;
    if (!system_tools::get_total_memory(total_memory))
    {
        LOG_TRACE("total_memory = %d\r\n", total_memory);
    }
    string cmd_result = system_tools::execute_command("sudo dmidecode - t memory - q");
    LOG_TRACE("cmd_result1 = %s\r\n", cmd_result.c_str());
    std::system("echo tuck >test.txt");
    std::clog << std::ifstream("test.txt").rdbuf();
    std::clog.flush();*/
}

void common_service_handler::get_host_general_info_fake()
{
    FUNC_TRACER;
    system_tools::cpu_info ci;
    if (!system_tools::get_cpu_info(ci))
    {
        LOG_TRACE("ci.logical_core_number = %d\r\n", ci.logical_core_number);
        LOG_TRACE("ci.physical_core.size() = %d\r\n", ci.physical_core.size());
    }
    string suuid;
    if (!system_tools::get_machine_id(suuid))
    {
        LOG_TRACE("machine-id:%s\r\n", suuid.c_str());
    }

    int total_memory = 0;
    if (!system_tools::get_total_memory(total_memory))
    {
        LOG_TRACE("total_memory:%d\r\n", total_memory);
    }
    LOG_TRACE("%s\r\n client_name:%s\r\n %s\r\n %s\r\n %s\r\n %s\r\n", _st.get_sysname(), _st.get_nodename(), _st.get_release(), _st.get_version(), _st.get_machine(), _st.get_domainname());
    /*got info every time*/
    dcs.clear();
    str = storage::get_storage();
    dcs = str->get_all_disk();
    /*this is for test to list all disk info*/
    for (auto &d : dcs)
    {
        printf_disk(d);
    }
    string pretty_name, arch;
    int major, minor;
    system_tools::get_os_pretty_name_and_info(pretty_name, arch, major,minor);
    if (arch.find("64") != std::string::npos)
    {
        arch = "amd64";
    }
    LOG_TRACE("pretty_name = %s, arch = %s", pretty_name.c_str(), arch.c_str());
    //dcs = disk::get_disk_info();
    /*std::vector<std::pair<uint64_t, uint64_t>> exts;
    get_physicak_location_of_specify_file_by_command(exts, "/.snapshot4", "/dev/sda1", false);
    for (auto & ext : exts)
    {
        printf("0 = %llu , 1 = %llu\r\n", ext.first,ext.second);
    }*/
}
void common_service_handler::get_host_general_info(physical_machine_info& _info)
{
    FUNC_TRACER;
    system_tools::cpu_info ci;
    vector<string> dnss;
    vector<string> gateways;
    set<network_info> ninfos;
    network_info ninfo;
    vector<string> ip_addr;
    if (!system_tools::get_cpu_info(ci))
    {
        _info.logical_processors = ci.logical_core_number;
        _info.processors = (ci.physical_core.size() == 0)? ci.logical_core_number: ci.physical_core.size();
        /*printf("ci.logical_core_number = %d\r\n", ci.logical_core_number);
        printf("ci.physical_core.size() = %d\r\n", ci.physical_core.size());*/
    }
    /*string suuid;
    if (!system_tools::get_machine_id(suuid))
    {
        _info.machine_id = suuid;
        LOG_TRACE("machine-id:%s\r\n", suuid.c_str());
    }*/
    int total_memory = 0;
    if (!system_tools::get_total_memory(total_memory))
    {
        _info.physical_memory = total_memory;
        LOG_TRACE("total_memory:%d\r\n", total_memory);
    }
    string pretty_name;
    if (!system_tools::get_os_pretty_name_and_info(pretty_name, _info.architecture, _info.os_version.major_version, _info.os_version.minor_version))
    {
        if (_info.architecture.find("64") != std::string::npos)
        {
            _info.architecture = "amd64";
        }
        _info.os_name = pretty_name;
        LOG_TRACE("pretty_name:%s, arch:%s", pretty_name.c_str(), _info.architecture.c_str());
    }
    if (!_hc.host_uuid.empty())
    {
        _info.machine_id = _hc.host_uuid;
        LOG_TRACE("machine_id:%s\r\n", _hc.host_uuid.c_str());
    }

    ip_addr = system_tools::get_ip_address();
    if (!ip_addr.empty())
    {
        ninfo.ip_addresses = ip_addr;// push_back(ip_addr);
        ninfo.__set_ip_addresses(ninfo.ip_addresses);
        for (auto & i : ip_addr)
        {
            LOG_TRACE("ip_address:%s\r\n", i.c_str());
        }
    }
    if (!system_tools::get_dns_servers(ninfo.dnss))
    {
        ninfo.__set_dnss(ninfo.dnss);
        for (auto & s : ninfo.dnss)
        {
            LOG_TRACE("dnss:%s\r\n", s.c_str());
        }
    }
    if (!system_tools::get_gateway(ninfo.gateways))
    {
        ninfo.__set_gateways(ninfo.gateways);
        for (auto & g : ninfo.gateways)
        {
            LOG_TRACE("get_gateway:%s\r\n", g.c_str());
        }
    }
    ninfos.insert(ninfo);
    _info.__set_network_infos(ninfos);
    for (auto & mi : _info.network_infos)
    {
        for (auto & i : mi.ip_addresses)
        {
            LOG_TRACE("ip_addresses = %s\r\n", i.c_str());
        }
        for (auto & gt : mi.gateways)
        {
            LOG_TRACE("gateways = %s\r\n", gt.c_str());
        }

        for (auto & dns : mi.dnss)
        {
            LOG_TRACE("dns = %s\r\n", dns.c_str());
        }
    }

    //finish
    _info.client_name = string(_st.get_nodename());
    LOG_TRACE("client_name:%s\r\n", _info.client_name.c_str());
    //printf("%s\r\n client_name:%s\r\n %s\r\n %s\r\n %s\r\n %s\r\n", _st.get_sysname(), _st.get_nodename(), _st.get_release(), _st.get_version(), _st.get_machine(), _st.get_domainname());
    //vector<disk_collection> dcs;
    dcs.clear();
    str = storage::get_storage();
    dcs = str->get_all_disk();
    //ot_disk_info(dcs);
    std::set<disk_info>  disk_infos;
    std::set<partition_info> partition_infos;
    for (auto &dc : dcs)
    {
        disk_infos.insert(fill_disk_info(dc));
        for (auto &pc : dc->partitions)
        {
            partition_infos.insert(fill_partition_info(pc));
        }
    }
    _info.__set_partition_infos(partition_infos);
    _info.__set_disk_infos(disk_infos);
}

void common_service_handler::ping(service_info& _return)
{
    FUNC_TRACER;
}
void common_service_handler::get_host_detail(physical_machine_info& _return, const std::string& session_id, const machine_detail_filter::type filter)
{
    FUNC_TRACER;
    /*now not process session_id and filter*/
    get_host_general_info(_return);
}
void common_service_handler::get_service_list(std::set<service_info> & _return, const std::string& session_id)
{
    FUNC_TRACER;
    service_info empty;
    boost::filesystem::path p = boost::filesystem::path(system_tools::get_execution_file_path_linux()).parent_path();
    if (boost::filesystem::exists(p / "server.key") && boost::filesystem::exists(p / "server.crt"))
    {
        _return.insert(get_service(saasame::transport::g_saasame_constants.CARRIER_SERVICE_SSL_PORT));
    }
    else
    {
        _return.insert(get_service(saasame::transport::g_saasame_constants.CARRIER_SERVICE_PORT));
    }
    _return.insert(get_service(saasame::transport::g_saasame_constants.LAUNCHER_SERVICE_PORT));
    _return.insert(get_service(saasame::transport::g_saasame_constants.LOADER_SERVICE_PORT));
    _return.insert(get_service(saasame::transport::g_saasame_constants.PHYSICAL_PACKER_SERVICE_PORT));
    _return.insert(get_service(saasame::transport::g_saasame_constants.SCHEDULER_SERVICE_PORT));
    _return.insert(get_service(saasame::transport::g_saasame_constants.VIRTUAL_PACKER_SERVICE_PORT));
    _return.erase(empty);
}
void common_service_handler::enumerate_disks(std::set<disk_info> & _return, const enumerate_disk_filter_style::type filter)
{
    FUNC_TRACER;
    printf("enumerate_disk_filter_style::type = %d\r\n", filter);
    str->scan_disk();
    disk::vtr all_disks = str->get_all_disk();
    for (auto &dc : all_disks)
    {
        _return.insert(fill_disk_info(dc));
    }
}
extern bool b_polling_mode;
bool common_service_handler::verify_carrier(const std::string& carrier, const bool is_ssl)
{
    FUNC_TRACER;
    std::shared_ptr<TSocket> socket;
    std::shared_ptr<apache::thrift::protocol::TProtocol> protocol;
    std::shared_ptr<saasame::transport::carrier_serviceClient> client;
    std::shared_ptr<AccessManager> accessManager(new MyAccessManager());// = g_accessManager;// (new MyAccessManager());
    std::shared_ptr<TSSLSocketFactory> factory;
    std::shared_ptr<TSSLSocket> ssl_socket;
    LOG_TRACE("carrier = %s, is_ssl = %d", carrier.c_str(), is_ssl);
    //carrier = std::string("61.216.157.52");

    /**/
    /*struct addrinfo hints, *res, *res0;
    char port[sizeof("65536")];
    sprintf(port, "%d", saasame::transport::g_saasame_constants.CARRIER_SERVICE_PORT);
    LOG_TRACE("3.1");
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG;
    LOG_TRACE("carrier.c_str() = %s\r\n", carrier.c_str());
    LOG_TRACE("port.c_str() = %s\r\n", port);
    int error = getaddrinfo(carrier.c_str(), port, &hints, &res0);
    LOG_TRACE("error = %d", error);*/

    /**/


    if (b_polling_mode)
    {
        std::vector<int> ports = { 443,18443 };
        for (auto & port : ports)
        {
            http_carrier_op::ptr p =
                http_carrier_op::ptr(new http_carrier_op({}, system_tools::analysis_ip_address(carrier), port, true));
            if (p && p->open()) {
                p->close();
                LOG_TRACE("true");
                return true;
            }
        }
    }
    else 
    {
        std::shared_ptr<TTransport> transport;
        if (is_ssl)
        {
            LOG_TRACE("enter ssl");
            boost::filesystem::path p = boost::filesystem::path(system_tools::get_execution_file_path_linux()).parent_path();
            if (boost::filesystem::exists(p / "server.key") && boost::filesystem::exists(p / "server.crt"))
            {
                try
                { 
                    factory = /*g_factory;*/std::shared_ptr<TSSLSocketFactory>(new TSSLSocketFactory());
                    factory->authenticate(false);
                    factory->loadCertificate((p / "server.crt").string().c_str());
                    factory->loadPrivateKey((p / "server.key").string().c_str());
                    factory->loadTrustedCertificates((p / "server.crt").string().c_str());
                    factory->access(accessManager);
                    ssl_socket = std::shared_ptr<TSSLSocket>(factory->createSocket(system_tools::analysis_ip_address(carrier), saasame::transport::g_saasame_constants.CARRIER_SERVICE_SSL_PORT));
                    ssl_socket->setConnTimeout(30*1000);
                    /*ssl_socket->setSendTimeout(1000);
                    ssl_socket->setRecvTimeout(1000);*/
                    transport = std::shared_ptr<TTransport>(new TBufferedTransport(ssl_socket));
                }
                catch (TException& ex) {
                    LOG_ERROR("%s", ex.what());
                }
            }
        }
        else
        {
            try
            {
                LOG_TRACE("enter no ssl");
                socket = std::shared_ptr<TSocket>(new TSocket(system_tools::analysis_ip_address(carrier), saasame::transport::g_saasame_constants.CARRIER_SERVICE_PORT));
                socket->setConnTimeout(30*1000);
                /*socket->setSendTimeout(1000);
                socket->setRecvTimeout(1000);*/
                transport = std::shared_ptr<TTransport>(new TBufferedTransport(socket));
            }
            catch (TException& ex) {
                LOG_ERROR("%s", ex.what());
            }
        }
        if (transport /*&& transport->open()*/)
        {
            protocol = std::shared_ptr<apache::thrift::protocol::TProtocol>(new apache::thrift::protocol::TBinaryProtocol(transport));
            client = std::shared_ptr<saasame::transport::carrier_serviceClient>(new saasame::transport::carrier_serviceClient(protocol));
            try
            {
                saasame::transport::service_info svc_info;
                transport->open();
                client->ping(svc_info);
                transport->close();
                return true;
            }
            catch (TException& ex){
                LOG_ERROR("%s", ex.what());
            }
        }
    }
    return false;
    //return true;
}

bool common_service_handler::create_mutex(const std::string& session, const int16_t timeout) {
    FUNC_TRACER;
    //_tcs.try_lock_for(seconds(timeout));
    return false;
}
bool common_service_handler::delete_mutex(const std::string& session) {
    FUNC_TRACER;
    //_tcs.unlock();
    return false;
}
disk_info common_service_handler::fill_disk_info(disk::ptr & dc)
{
    disk_info di;
    di.path = dc->ab_path;
    di.location = dc->ab_path;
    di.size = dc->blocks;
    di.serial_number = dc->serial_number;
    di.uri = dc->string_uri;
    di.number = dc->disk_index;
    di.friendly_name = dc->ab_path;
    di.bus_type = bus_type_index[dc->bus_type];
    string boot_disk = system_tools::execute_command("df -P /boot | tail -1 | cut -d' ' -f 1");
    di.boot_from_disk = dc->is_include_string(boot_disk);
    di.partition_style = dc->partition_style;
    di.guid = dc->str_guid;
    di.signature = dc->MBRSignature;
    if (di.boot_from_disk)
    {
        LOG_TRACE("TRUE");
    }
    else
    {
        LOG_TRACE("FALSE");
    }

    di.is_boot = di.boot_from_disk || dc->is_root();
    di.is_system = dc->is_system();

    if (di.bus_type == 0)
        di.bus_type = bus_type::type::ATA;
    LOG_TRACE("di.path = %s", di.path.c_str());
    LOG_TRACE("di.size = %llu", di.size);
    LOG_TRACE("di.serial_number = %s", di.serial_number.c_str());
    LOG_TRACE("di.uri = %s", di.uri.c_str());
    LOG_TRACE("di.number = %d", di.number);
    LOG_TRACE("di.friendly_name = %s", di.friendly_name.c_str());
    LOG_TRACE("di.bus_type = %d", di.bus_type);
    LOG_TRACE("di.boot_from_disk = %d", di.boot_from_disk);
    LOG_TRACE("di.is_boot = %d", di.is_boot);
    LOG_TRACE("di.is_system = %d", di.is_system);


    return di;
}
partition_info common_service_handler::fill_partition_info(partitionA::ptr &pc)
{
    partition_info pi;
    pi.__set_access_paths({ pc->ab_path });
    pi.disk_number = pc->parent_disk->disk_index;
    pi.partition_number = pc->disk_index;
    pi.offset = pc->partition_start_offset;
    pi.size = pc->blocks;
    pi.is_boot = pc->is_boot() || pc->is_root();
    pi.is_system = pc->BootIndicator;

    for (auto & ap : pi.access_paths)
    {
        LOG_TRACE("ap = %s", ap.c_str());
    }
    LOG_TRACE("pi.disk_number = %d", pi.disk_number);
    LOG_TRACE("pi.partition_number = %d", pi.partition_number);
    LOG_TRACE("pi.offset = %llu", pi.offset);
    LOG_TRACE("pi.size = %llu", pi.size);
    return pi;
}

service_info common_service_handler::get_service(int port) {
    FUNC_TRACER;
    service_info _return;
    ping(_return);
    //macho::windows::registry reg;
    /*boost::filesystem::path p = boost::filesystem::path(system_tools::get_execution_file_path_linux()).parent_path();
    LOG_TRACE("2");
    std::shared_ptr<TTransport> transport;
    LOG_TRACE("3");
    if (boost::filesystem::exists(p / "server.key") && boost::filesystem::exists(p / "server.crt")) {
        try
        {
            LOG_TRACE("4");
            std::shared_ptr<TSSLSocketFactory> factory;
            factory = std::shared_ptr<TSSLSocketFactory>(new TSSLSocketFactory());
            factory->authenticate(false);
            factory->loadCertificate((p / "server.crt").string().c_str());
            factory->loadPrivateKey((p / "server.key").string().c_str());
            factory->loadTrustedCertificates((p / "server.crt").string().c_str());
            std::shared_ptr<AccessManager> accessManager(new MyAccessManager());
            factory->access(accessManager);
            std::shared_ptr<TSSLSocket> ssl_socket = std::shared_ptr<TSSLSocket>(factory->createSocket("127.0.0.1", port));
            ssl_socket->setConnTimeout(1000);
            transport = std::shared_ptr<TTransport>(new TBufferedTransport(ssl_socket));
        }
        catch (TException& ex) {
            LOG_TRACE("6");
        }
    }
    else {
        std::shared_ptr<TSocket> socket(new TSocket("127.0.0.1", port));
        socket->setConnTimeout(1000);
        transport = std::shared_ptr<TTransport>(new TBufferedTransport(socket));
    }
    if (transport) {
        std::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
        common_serviceClient client(protocol);
        try {
            LOG_TRACE("7");
            transport->open();
            LOG_TRACE("8");
            client.ping(_return);
            LOG_TRACE("9");
            transport->close();
            LOG_TRACE("10");
        }
        catch (TException& ex) {
            LOG(LOG_LEVEL_ERROR, "%s", ex.what());
        }
    }*/
    return _return;
}

void common_service_handler::take_xray_local() {
    FUNC_TRACER;
    bool has_data = false;
    boost::filesystem::path working_dir = boost::filesystem::path(system_tools::get_execution_file_path_linux()).parent_path();
    string filename = "linux_packer_log_" + boost::posix_time::to_simple_string(boost::posix_time::microsec_clock::universal_time()) + ".zip";
    boost::filesystem::path logzip = working_dir / filename;
    linux_tools::archive::zip::ptr zip_ptr = linux_tools::archive::zip::open(logzip);

    if (zip_ptr) {
        if (zip_ptr->add(working_dir / ("logs"), MZ_DEFAULT_LEVEL))
            has_data = true;
        zip_ptr->close();
    }
}


#define WRITE_COMMAND_RESULT(fs,command_)  \
do{                                        \
    std::string cmd = std::string(command_) + std::string(":\n");    \
    std::string _result = system_tools::execute_command(command_);\
    fs.write(cmd.c_str(), cmd.size());\
    _result += "\n";\
    fs.write(_result.c_str(), _result.size());\
}while(0)
void common_service_handler::take_xray(std::string& _return) {
    FUNC_TRACER;
    bool has_data = false;
    boost::filesystem::path working_dir = boost::filesystem::path(system_tools::get_execution_file_path_linux()).parent_path();
    std::string result_string = system_tools::execute_command("dmesg > /usr/local/linux_packer/logs/dmesg");
    std::stringstream data;
    linux_tools::archive::zip::ptr zip_ptr = linux_tools::archive::zip::open(data);
    /*std::clog.rdbuf(backup);
    out.close();*/
    /*1. wirte the /proc/datto-info to the firstcheck*/
    /*only this use it so i think it shouldn't need to write a function*/
    /*open the file*/
    boost::filesystem::path firstcheck = working_dir / ("logs") / ("firstcheck");
    fstream fs; 
    fs.open(firstcheck.string().c_str(), fstream::out);
    fs.write(version.c_str(), version.size());
    fs.write("\n", 1);
    fs.write("\n", 1);

    string out;
    string result = system_tools::execute_command("uname -r");
    fs.write(result.c_str(), result.size());
    fs.write("\n", 1);
    fs.write("\n", 1);
    string ks_path = "/usr/src/kernels/" + result;
    string ks2_path = "/usr/src/linux";
    if ((opendir(ks_path.c_str()) == NULL)
        && (opendir(ks2_path.c_str()) == NULL))
    {
        out = "kernel devel not instsalled.\n";
        fs.write(out.c_str(), out.size());
        fs.write("\n", 1);
    }
    else
    {
        out = "kernel devel instsalled.\n";
        fs.write(out.c_str(), out.size());
        fs.write("\n", 1);
    }
    WRITE_COMMAND_RESULT(fs, "pgrep linux_packer");
    WRITE_COMMAND_RESULT(fs, "cat /proc/datto-info");
    WRITE_COMMAND_RESULT(fs, "cat /etc/*-release");
    WRITE_COMMAND_RESULT(fs, "lsblk");
    WRITE_COMMAND_RESULT(fs, "df -hT");
    WRITE_COMMAND_RESULT(fs, "ip addr");
    WRITE_COMMAND_RESULT(fs, "iptables -L ¡Vn");
    WRITE_COMMAND_RESULT(fs, "ip rule show");
    WRITE_COMMAND_RESULT(fs, "ip route show table local");
    WRITE_COMMAND_RESULT(fs, "ip route show table main");
    WRITE_COMMAND_RESULT(fs, "ip route show table default");
    WRITE_COMMAND_RESULT(fs, "cat /etc/sysconfig/network-scripts/ifcfg-*");
    WRITE_COMMAND_RESULT(fs, "cat /etc/network/interfaces");
    WRITE_COMMAND_RESULT(fs, "cat /etc/sysconfig/network/ifcfg-*");
    fs.close();

    if (zip_ptr) {
        if (zip_ptr->add(working_dir / ("connections"), MZ_DEFAULT_LEVEL))
            has_data = true;
        if (zip_ptr->add(working_dir / ("logs"), MZ_DEFAULT_LEVEL))
            has_data = true;
        if (zip_ptr->add(working_dir / ("jobs"), MZ_DEFAULT_LEVEL))
            has_data = true;
        if (zip_ptr->close() && has_data)
            _return = data.str();
        LOG_INFO("take_xray, working dir: %s, return data length:%d", working_dir.string().c_str(), _return.length());
    }
    remove("/usr/local/linux_packer/logs/dmesg");
    /*boost::filesystem::path appfile = boost::filesystem::path(system_tools::get_execution_file_path_linux());
    string logfilename = appfile.filename().string() + ".log";
    boost::filesystem::path abs_logfilename = working_dir / ("logs") / (logfilename);
    out.open(abs_logfilename.string().c_str(), std::fstream::out | std::fstream::app);
    std::clog.rdbuf(out.rdbuf());*/
    remove(firstcheck.string().c_str());
}

void common_service_handler::take_xrays(std::string& _return) {
    FUNC_TRACER;
    std::stringstream data;
    linux_tools::archive::zip::ptr zip_ptr = linux_tools::archive::zip::open(data);
    if (zip_ptr) {
        bool     has_data = false;
        boost::filesystem::path p = boost::filesystem::path(system_tools::get_execution_file_path_linux()).parent_path();
        if (boost::filesystem::exists(p / "server.key") && boost::filesystem::exists(p / "server.crt")) {
            std::string carrier_data = get_xray(saasame::transport::g_saasame_constants.CARRIER_SERVICE_SSL_PORT);
            if (!carrier_data.empty())
            {
                has_data = true;
                zip_ptr->add("Carrier.zip", carrier_data, MZ_DEFAULT_LEVEL);
            }
        }
        else {
            std::string carrier_data = get_xray(saasame::transport::g_saasame_constants.CARRIER_SERVICE_PORT);
            if (!carrier_data.empty())
            {
                has_data = true;
                zip_ptr->add("Carrier.zip", carrier_data, MZ_DEFAULT_LEVEL);
            }
        }
        std::string launcher_data = get_xray(saasame::transport::g_saasame_constants.LAUNCHER_SERVICE_PORT);
        if (!launcher_data.empty())
        {
            has_data = true;
            zip_ptr->add("Launcher.zip", launcher_data, MZ_DEFAULT_LEVEL);
        }
        std::string loader_data = get_xray(saasame::transport::g_saasame_constants.LOADER_SERVICE_PORT);
        if (!loader_data.empty())
        {
            has_data = true;
            zip_ptr->add("Loader.zip", loader_data, MZ_DEFAULT_LEVEL);
        }
        std::string phypac_data = get_xray(saasame::transport::g_saasame_constants.PHYSICAL_PACKER_SERVICE_PORT);
        if (!phypac_data.empty())
        {
            has_data = true;
            zip_ptr->add("PhysicalPacker.zip", phypac_data, MZ_DEFAULT_LEVEL);
        }
        std::string scheduler_data = get_xray(saasame::transport::g_saasame_constants.SCHEDULER_SERVICE_PORT);
        if (!scheduler_data.empty())
        {
            has_data = true;
            zip_ptr->add("Scheduler.zip", scheduler_data, MZ_DEFAULT_LEVEL);
        }
        std::string vpac_data = get_xray(saasame::transport::g_saasame_constants.VIRTUAL_PACKER_SERVICE_PORT);
        if (!vpac_data.empty())
        {
            has_data = true;
            zip_ptr->add("VirtualPacker.zip", vpac_data, MZ_DEFAULT_LEVEL);
        }
        if (zip_ptr->close() && has_data)
            _return = data.str();
        LOG_INFO("take_xrays , return data length:%d", _return.length());
    }
}

std::string common_service_handler::get_xray(int port, const std::string& host) {
    FUNC_TRACER;
    std::string _return;
    boost::filesystem::path p(system_tools::get_execution_file_path_linux());
    std::shared_ptr<TTransport> transport;
    if (boost::filesystem::exists(p / "server.key") && boost::filesystem::exists(p / "server.crt")) {
        try
        {
            std::shared_ptr<TSSLSocketFactory> factory;
            factory = /*g_factory;*/ std::shared_ptr<TSSLSocketFactory>(new TSSLSocketFactory());
            factory->authenticate(false);
            factory->loadCertificate((p / "server.crt").string().c_str());
            factory->loadPrivateKey((p / "server.key").string().c_str());
            factory->loadTrustedCertificates((p / "server.crt").string().c_str());
            std::shared_ptr<AccessManager> accessManager(new MyAccessManager());// = g_accessManager;// (new MyAccessManager());
            factory->access(accessManager);
            std::shared_ptr<TSSLSocket> ssl_socket = std::shared_ptr<TSSLSocket>(factory->createSocket(host, port));
            ssl_socket->setConnTimeout(1000);
            transport = std::shared_ptr<TTransport>(new TBufferedTransport(ssl_socket));
        }
        catch (TException& ex) {
        }
    }
    else {
        std::shared_ptr<TSocket> socket(new TSocket(host, port));
        socket->setConnTimeout(1000);
        transport = std::shared_ptr<TTransport>(new TBufferedTransport(socket));
    }
    if (transport) {
        std::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
        common_serviceClient client(protocol);
        try {
            transport->open();
            client.take_xray(_return);
            transport->close();
            LOG_INFO("get_xray function(port:%d), return data length:%d\r\n", port, _return.length());
        }
        catch (TException& tx) {
        }
    }
    return _return;
}

