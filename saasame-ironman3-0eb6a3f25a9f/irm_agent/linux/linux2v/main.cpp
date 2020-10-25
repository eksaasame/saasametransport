#include <cstdio>
#include <sstream>
#include <map>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/string_generator.hpp>
#include <boost/thread.hpp>
#include <boost/date_time.hpp>
#include <boost/filesystem.hpp>
#include <errno.h> 
#include <stdio.h> 
#include <unistd.h>
#include "storage.hpp"
#include "operation_system.hpp"
#include "tracelog.hpp"
#include <locale>
#include <sstream>
#include <fstream>

namespace po = boost::program_options;
using boost::property_tree::ptree;
using boost::property_tree::basic_ptree;
using namespace macho;
static macho::tracelog g_log(_T("global"));

void macho::log(TRACE_LOG_LEVEL level, const char* fmt, ...) {
    va_list arg_list;
    va_start(arg_list, fmt);
    g_log.log(level, fmt, arg_list);
    va_end(arg_list);
}

bool macho::is_log(TRACE_LOG_LEVEL level) {
    return level <= g_log.get_level();
}

struct partition_path {
    partition_path() : partition_style(0), signature(0), partition_number(0), is_system_disk(false), partition_size(0), partition_offset(0) {}
    typedef boost::shared_ptr<partition_path> ptr;
    typedef  std::vector<ptr>                 vtr;
    int                                       partition_style;
    uint32_t                                  signature;
    std::string                               guid;
    uint32_t								  partition_number;
    std::string							      path;
    bool									  is_system_disk;
    uint64_t								  partition_size;
    uint64_t                                  partition_offset;
    static bool							      save(const boost::filesystem::path& cfg, const partition_path::vtr& partitions);
    static partition_path::vtr                load(const boost::filesystem::path& cfg);
};

partition_path::vtr  partition_path::load(const boost::filesystem::path& cfg) {
    partition_path::vtr parts;
    if (boost::filesystem::exists(cfg)) {
        try {
            ptree root;
            std::ifstream is(cfg.string());
            boost::property_tree::read_json(is, root);
            for (ptree::value_type &part : root.get_child("partitions")) {
                partition_path::ptr _p(new partition_path());
                _p->partition_number = part.second.get<uint32_t>("partition_number");
                _p->partition_style = part.second.get<uint32_t>("partition_style");
                _p->signature = part.second.get<uint32_t>("signature");
                _p->path = part.second.get<std::string>("path");
                _p->guid = part.second.get<std::string>("guid");
                _p->is_system_disk = part.second.get<bool>("is_system_disk");
                _p->partition_size = part.second.get<uint64_t>("partition_size");
                _p->partition_offset = part.second.get<uint64_t>("partition_offset");
                parts.push_back(_p);
            }
        }
        catch (boost::exception& ex) {
        }
        catch (...) {
        }
    }
    return parts;
}

bool partition_path::save(const boost::filesystem::path& cfg, const partition_path::vtr& partitions) {
    bool   result = false;
    try {
        ptree root;
        ptree parts_node;
        for  (partition_path::ptr p : partitions) {
            ptree part;
            part.put<uint32_t>("partition_style", p->partition_style);
            part.put<uint32_t>("signature", p->signature);
            part.put<std::string>("guid", p->guid);
            part.put<uint32_t>("partition_number", p->partition_number);
            part.put<std::string>("path", p->path);
            part.put<uint64_t>("partition_size", p->partition_size);
            part.put<uint64_t>("partition_offset", p->partition_offset);
            part.put<bool>("is_system_disk", p->is_system_disk);
            parts_node.push_back(std::make_pair("", part));
        }
        root.add_child("partitions", parts_node);
        std::ofstream output(cfg.string(), std::ios::out | std::ios::trunc);
        boost::property_tree::write_json(output, root);
        result = true;
    }
    catch (boost::exception& ex) {
    }
    catch (...) {
    }
    return result;
}

bool command_line_parser(int argc, char* argv[], po::variables_map &vm) {
    bool result = false;
    std::string title;
    po::options_description command("General");
    command.add_options()
        ("boot,b", "run command on next windows boot up")
        ("terminate,u", "terminate to run command on next windows boot up")
        ("time,t", po::value<int>()->default_value(0, "0"), "wait seconds")
        ("level,l", po::value<int>()->default_value(2, "2"), "log level ( 0 ~ 5 )")
        ("help,h", "produce help message (option)");
    ;

    po::options_description all("Allowed options");
    all.add(command);

    try {
        po::store(po::command_line_parser(argc, argv).options(all).run(), vm);
        po::notify(vm);
        if (vm.count("help") || (vm["level"].as<int>() > 5) || (vm["level"].as<int>() < 0)) {
            std::cout << title << all << std::endl;
        }
        else {
            result = true;
        }
    }
    catch (const boost::program_options::multiple_occurrences& e) {
        std::cout << title << command << "\n";
        std::cout << e.what() << " from option: " << e.get_option_name() << std::endl;
    }
    catch (const boost::program_options::error& e) {
        std::cout << title << command << "\n";
        std::cout << e.what() << std::endl;
    }
    catch (boost::exception &e) {
        std::cout << title << command << "\n";
        std::cout << boost::exception_detail::get_diagnostic_information(e, "Invalid command parameter format.") << std::endl;
    }
    catch (...) {
        std::cout << title << command << "\n";
        std::cout << "Invalid command parameter format." << std::endl;
    }
    return result;
}

boost::filesystem::path get_execution_full_path(){
    return boost::filesystem::read_symlink("/proc/self/exe");
}

bool remove_runonced(boost::filesystem::path etc) {
    operation_system::ptr os = operation_system::get();
    boost::filesystem::path rclocal_file = etc / "rc.local";
    if (os && os->os_family() == "SuSE") {
        rclocal_file = etc / "init.d" / "boot.local";
    }
    if (boost::filesystem::exists(rclocal_file)) { 
        console::execute(boost::str(boost::format("sed -i '/\\/etc\\/runonce.d\\/bin\\/runonce.sh/d' %s") % rclocal_file.string()));
        console::execute(boost::str(boost::format("sed -i '/^$/d' %s") % rclocal_file.string()));
    }
    boost::filesystem::remove( etc / "runonce.d" / "bin" / "runonce.sh");
    return true;
}

bool inject_runonced(boost::filesystem::path etc) {
    try {
        operation_system::ptr os = operation_system::get();
        boost::filesystem::path rclocal_file = etc / "rc.local";
        if (os && os->os_family() == "SuSE") {
            rclocal_file = etc / "init.d" / "boot.local";
        }
        boost::filesystem::create_directories(etc / "runonce.d" / "bin");
        boost::filesystem::create_directories(etc / "runonce.d" / "run");
        boost::filesystem::create_directories(etc / "runonce.d" / "ran");

        if (boost::filesystem::exists(rclocal_file)) {
            if (std::string::npos != read_from_file(rclocal_file).find("exit 0")) {
                console::execute(boost::str(boost::format("sed -i '/\\/etc\\/runonce.d\\/bin\\/runonce.sh/d' %s") % rclocal_file.string()));
                console::execute(boost::str(boost::format("sed -i '/^$/d' %s") % rclocal_file.string()));
                std::string runsh = ("\\/etc\\/runonce.d\\/bin\\/runonce.sh\\n\\1exit 0");
                console::execute(boost::str(boost::format("sed -i 's/^\\(\\s*\\)exit 0/%s/g' %s") % runsh % rclocal_file.string()));
            }
        }
        else {
            console::execute(boost::str(boost::format("touch %s") % rclocal_file.string()));
            console::execute(boost::str(boost::format("chmod +x %s") % rclocal_file.string()));
            std::ofstream out(rclocal_file.string(), std::ios::out | std::ios::binary);
            out << "#!/bin/bash\n" << std::endl;
            out << "\n/etc/runonce.d/bin/runonce.sh\nexit 0" << std::endl;
            out.close();
        }
        boost::filesystem::path execution_path = get_execution_full_path();
        boost::filesystem::path runoncesh_file = etc / "runonce.d" / "bin" / "runonce.sh";
        std::ofstream runoncesh(runoncesh_file.string(), std::ios::out | std::ios::trunc | std::ios::binary);
        runoncesh << "#!/bin/bash\n" << std::endl;
        runoncesh << boost::str(boost::format("SCRIPT_PATH=\"%s\"")% execution_path.string()) <<std::endl;
        runoncesh << "if [ -f \"$SCRIPT_PATH\" ]; then" << std::endl;
        runoncesh << "    sleep 5s" << std::endl;
        runoncesh << "    echo -e \"Run $OUTPUT\" >> \"/etc/runonce.d/scheduler.log\"" << std::endl;
        runoncesh << "    exec $SCRIPT_PATH & #this doesn't blocks!" << std::endl;
        runoncesh << "fi" << std::endl;
        runoncesh.close();
        console::execute(boost::str(boost::format("chmod +x %s") % runoncesh_file.string()));
        return true;
    }
    catch(...){
    }
    return false;
}

bool is_mounted(boost::filesystem::path p) {
    std::string ret = console::execute(boost::str(boost::format("mount | grep %s") % p.string()));
    return std::string::npos != ret.find(p.string());
}

std::vector<std::wstring> read_file(boost::filesystem::path p)
{
    std::vector<std::wstring> result;
    std::wifstream wif(p.string());
    std::locale loc("en_US.UTF-8");
    wif.imbue(loc);
    std::wstring str;
    while (std::getline(wif, str) && str.length()) {
        str = str.substr(str.find_first_of(L"/"));
        if (!str.empty())
            result.push_back(str);
    }
    return result;
}

void remove_excluded_paths(boost::filesystem::path volume)
{
    boost::filesystem::path excluded = volume / L".excluded";
    if (boost::filesystem::exists(excluded)) {
        std::vector<std::wstring> paths = read_file(excluded);
        for(std::wstring path : paths) {
            boost::filesystem::path p = volume / path.substr(1);
            if (boost::filesystem::exists(p)) {
                if (boost::filesystem::is_directory(p)) {
                    boost::filesystem::directory_iterator end_itr;
                    // cycle through the directory
                    LOG(LOG_LEVEL_WARNING, _T("Remove subfolders and files from excluded folder (%s)."), p.wstring().c_str());
                    for (boost::filesystem::directory_iterator itr(p); itr != end_itr; ++itr) {
                        try {
                            if (boost::filesystem::is_directory(itr->path())) {
                                boost::filesystem::remove_all(itr->path());
                            }
                            else {
                                boost::filesystem::remove(itr->path());
                            }
                        }
                        catch (...) {
                        }
                    }
                }
                else {
                    try {
                        boost::filesystem::remove(p);
                        LOG(LOG_LEVEL_WARNING, _T("Remove excluded file (%s)."), p.wstring().c_str());
                    }
                    catch (...) {
                    }
                }
            }
        }
        try {
            boost::filesystem::remove(excluded);
        }
        catch (...) {
        }
    }
}

int main(int argc, char* argv[])
{
    po::variables_map vm;
    if (command_line_parser(argc, argv, vm)) {
        boost::filesystem::path execution_path = get_execution_full_path();
        g_log.set_file(std::string(get_execution_full_path().string()).append(".log"));
        g_log.set_level(macho::TRACE_LOG_LEVEL::LOG_LEVEL_INFO);
        if ((!vm.count("boot")) && vm["time"].as<int>()) {
            std::wcout << L"Waitting (" << vm["time"].as<int>() << L") seconds..." << std::endl;
            boost::this_thread::sleep(boost::posix_time::seconds(vm["time"].as<int>()));
        }
        if (vm.count("boot")) {
            inject_runonced("/etc");
        }     
        boost::filesystem::path config = boost::filesystem::path(get_execution_full_path()).parent_path() / L"agent.cfg";
        boost::filesystem::path batch = boost::filesystem::path(get_execution_full_path()).parent_path() / L"run.sh";
        if (!boost::filesystem::exists(config)) {
            LOG(LOG_LEVEL_ERROR, "Cannot load the config file %s", config.c_str());
        }
        else{
            partition_path::vtr parts = partition_path::load(config);
            while (parts.size()) {
                storage::rescan();
                storage::ptr stg = storage::get();
                if (stg) {
                    boost::uuids::string_generator gen;
                    stg->online_vgvols(stg->find_all_inavtive_vgvols());
                    for (storage::disk::ptr d : stg->disks) {
                        if (d->is_system()) {
                            LOG(LOG_LEVEL_RECORD, "found system disk %s", d->path.c_str());
                            continue;
                        }
                        partition_path::vtr::iterator p = parts.begin();
                        while (p != parts.end()) {
                            bool done = false;
                            if ((*p)->partition_style == d->partition_style) {
                                bool matched = false;
                                switch (d->partition_style) {
                                case storage::ST_PST_UNKNOWN:
                                    break;
                                case storage::ST_PST_MBR:
                                    if ((*p)->guid.empty()) {
                                        matched = (int)d->signature == (int)(*p)->signature;
                                    }
                                    break;
                                case storage::ST_PST_GPT:
                                    if (!(*p)->guid.empty()) {
                                        boost::uuids::uuid g1 = gen(d->guid);
                                        boost::uuids::uuid g2 = gen((*p)->guid);
                                        matched = g1 == g2;
                                    }
                                    break;
                                }
                                if (matched) {
                                    for (storage::partition::ptr _p : d->partitions) {
                                        if (_p->size * 512 == (uint64_t)(*p)->partition_size &&
                                            _p->start * 512 == (uint64_t)(*p)->partition_offset) {
                                            LOG(LOG_LEVEL_RECORD, "found %s", _p->path.c_str());
                                            if (!_p->mount_point.empty()) {
                                                LOG(LOG_LEVEL_WARNING, "umount %s", _p->mount_point.c_str());
                                                console::execute(boost::str(boost::format("umount %s") % _p->mount_point));
                                            }
                                            if (!(*p)->path.empty()) {
                                                if (boost::filesystem::exists((*p)->path) && is_mounted((*p)->path)) {
                                                    LOG(LOG_LEVEL_WARNING, L"The path (%s) is already existed.", (*p)->path.c_str());
                                                    done = true;
                                                }
                                                else {
                                                    LOG(LOG_LEVEL_RECORD, "mount %s %s", _p->path.c_str(), (*p)->path.c_str());
                                                    boost::filesystem::create_directories(boost::filesystem::path((*p)->path));
                                                    console::execute(boost::str(boost::format("mount %s %s") % _p->path % (*p)->path));
                                                    remove_excluded_paths((*p)->path);
                                                    done = boost::filesystem::exists((*p)->path);
                                                }
                                            }
                                            else {
                                                done = true;
                                            }
                                        }
                                    }
                                }
                            }
                            if (done)
                                p = parts.erase(p);
                            else
                                p++;
                        }
                    }
                    stg->rescan();
                    for (storage::volume::ptr v : stg->volumes) {
                        if (v->mount_point.empty()) {
                            std::string mount_point = boost::str(boost::format("/mnt/%s") % v->name);
                            try {
                                boost::filesystem::create_directories(mount_point);
                            }
                            catch (...) {
                            }
                            if (boost::filesystem::exists(mount_point)) {
                                console::execute(boost::str(boost::format("mount %s %s") % v->path %mount_point));
                                remove_excluded_paths(mount_point);
                                console::execute(boost::str(boost::format("umount %s") % mount_point));
                                try {
                                    boost::filesystem::remove(mount_point);
                                }
                                catch (...) {
                                }
                            }
                        }
                        else {
                            remove_excluded_paths(v->mount_point);
                        }
                    }
                }
                boost::this_thread::sleep(boost::posix_time::seconds(15));
            }

            if (boost::filesystem::exists(batch)) {
                console::execute(boost::str(boost::format("chmod +x %s") % batch.string()));
                console::execute(batch.string());
            }
        }
        if (vm.count("terminate")) {
            remove_runonced("/etc");
        }
    }
    return 0;
}