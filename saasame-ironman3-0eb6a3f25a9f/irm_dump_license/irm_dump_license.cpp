// irm_dump_license.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "common\license.hpp"

bool command_line_parser(po::variables_map &vm){
    bool result = false;
    std::string title;
#ifndef _CONSOLE
    title = boost::str(boost::format("%s\n") % GetCommandLineA());
#endif
    //title += boost::str(boost::format("\r\n------------ %s, Version: %d.%d Build: %d ------------\r\n\r\n") % PRODUCT_NAME_STR %PRODUCT_MAJOR %PRODUCT_MINOR %PRODUCT_BUILD);
    //guid_ g = guid_::create();
    po::options_description input("Input");
    input.add_options()
        ("db,d", po::value<std::string>(), "db file")
        ("id,i", po::value<std::string>(), "Machine ID")
        ("mac,m", po::value<std::string>(), "MAC")
        ;
    po::options_description general("General");
    general.add_options()
        ("help,h", "produce help message (option)");
    ;
    po::options_description all("Allowed options");
    all.add(input).add(general);

    try{
        std::wstring c = GetCommandLine();
#if _UNICODE
        po::store(po::wcommand_line_parser(po::split_winmain(GetCommandLine())).options(all).run(), vm);
#else
        po::store(po::command_line_parser(po::split_winmain(GetCommandLine())).options(all).run(), vm);
#endif
        po::notify(vm);
        if (vm.count("help")){
            std::cout << title << all << std::endl;
        }
        else if (vm.count("db")) {
            result = true;
        }
    }
    catch (const boost::program_options::multiple_occurrences& e) {
        std::cout << title << all << "\n";
        std::cout << e.what() << " from option: " << e.get_option_name() << std::endl;
    }
    catch (const boost::program_options::error& e) {
        std::cout << title << all << "\n";
        std::cout << e.what() << std::endl;
    }
    catch (boost::exception &e){
        std::cout << title << all << "\n";
        std::cout << boost::exception_detail::get_diagnostic_information(e, "Invalid command parameter format.") << std::endl;
    }
    catch (...){
        std::cout << title << all << "\n";
        std::cout << "Invalid command parameter format." << std::endl;
    }
    return result;
}

struct license_info{
    typedef std::vector<license_info> vtr;
    license_info() : count(0), consumed(0){}
    std::string key;
    int         count;
    std::string expired_date;
    int         consumed;
    std::string activated;
    std::string status;
};

struct workload_history{
    typedef std::vector<workload_history> vtr;
    std::string machine_id;
    std::string name;
    std::string type;
    std::vector<int32_t>  histories;
};

typedef std::map<uint32_t, uint32_t> consumed_map_type;
typedef std::map<std::string, consumed_map_type> workload_consumed_map_type;

workload_consumed_map_type calculated_consumed(workload::map& workloads){
    workload_consumed_map_type workload_consumeds;
    foreach(workload::map::value_type& w, workloads){
        consumed_map_type consumeds;
        foreach(workload::ptr workload, w.second){
            boost::posix_time::ptime::date_type updated = boost::posix_time::from_time_t(workload->updated).date();
            boost::posix_time::ptime::date_type created = boost::posix_time::from_time_t(workload->created).date();
            boost::posix_time::ptime::date_type current = boost::posix_time::second_clock::universal_time().date();
            boost::posix_time::ptime::date_type deleted;
            if (-1 == workload->deleted){
                if (updated > current)
                    deleted = updated;
                else
                    deleted = current;
            }
            else{
                deleted = boost::posix_time::from_time_t(workload->deleted).date();
                if (updated > deleted)
                    deleted = updated;
            }
            for (uint32_t s = (created.year() * 12 + created.month()); s <= (deleted.year() * 12 + deleted.month()); ++s)
                consumeds[s]++;
        }
        workload_consumeds[w.first] = consumeds;
    }
    return workload_consumeds;
}

workload_consumed_map_type calculated_license_consumed(workload::map workloads, std::vector<license_info> &licenses){
    bool can_be_run = false;
    boost::posix_time::ptime datetime = boost::posix_time::second_clock::universal_time();
    workload_consumed_map_type consumeds = calculated_consumed(workloads);
    boost::posix_time::ptime::date_type current = datetime.date();
    uint32_t _current = current.year() * 12 + current.month();
    workload_consumed_map_type backup_consumeds = consumeds;
    if (consumeds.size()){
        uint32_t min = -1, max = 0;
        foreach(workload_consumed_map_type::value_type &wc, consumeds){
            for (auto it = wc.second.cbegin(); it != wc.second.cend(); it++){
                if (it->first < min)
                    min = it->first;
                if (it->first > max)
                    max = it->first;
            }
        }
        for (uint32_t d = min; d <= max; d++){
            foreach(license_info &info, licenses){
                if (!info.expired_date.empty()){
                    boost::posix_time::ptime::date_type expired = boost::posix_time::time_from_string(info.expired_date).date();
                    uint32_t _expired = expired.year() * 12 + expired.month();
                    foreach(workload_consumed_map_type::value_type &wc, consumeds){
                        if (info.consumed < info.count){
                            if (d <= _expired){
                                if (wc.second.count(d)){
                                    info.consumed++;
                                    wc.second.erase(d);
                                }
                            }
                            else{
                                break;
                            }
                        }
                        else{
                            break;
                        }
                    }
                    if (_current > _expired){
                        info.status = "expired";
                        can_be_run = false;
                    }
                    else
                        can_be_run = info.consumed < info.count;
                }
                else{
                    foreach(workload_consumed_map_type::value_type &wc, consumeds){
                        if (info.consumed < info.count){
                            if (wc.second.count(d)){
                                info.consumed++;
                                wc.second.erase(d);
                            }
                        }
                        else{
                            break;
                        }
                    }
                    can_be_run = info.consumed < info.count;
                }
            }
        }
    }
    else{
        foreach(license_info &info, licenses){
            if (!info.expired_date.empty()){
                boost::posix_time::ptime::date_type expired = boost::posix_time::time_from_string(info.expired_date).date();
                uint32_t _expired = expired.year() * 12 + expired.month();
                if (_current > _expired)
                    can_be_run = false;
                else
                    can_be_run = info.consumed < info.count;
            }
            else{
                can_be_run = info.consumed < info.count;
            }
        }
    }
    uint32_t bill = 0;
    foreach(workload_consumed_map_type::value_type &wc, consumeds){
        for (auto it = wc.second.cbegin(); it != wc.second.cend()/* not hoisted */; /* no increment */){
            wc.second.erase(it++);
            bill++;
        }
    }
    if (bill){
        license_info bill_lic_info;
        bill_lic_info.key = "0000000000000000000000000";
        bill_lic_info.count = 0;
        bill_lic_info.consumed = bill;
        licenses.push_back(bill_lic_info);
    }
    return backup_consumeds;
}

int _tmain(int argc, _TCHAR* argv[])
{
    po::variables_map vm;
    if (command_line_parser(vm)){
        boost::filesystem::path db_file = vm["db"].as<std::string>();
        std::string mac = vm.count("mac") ? vm["mac"].as<std::string>() : "";
        std::string id = vm.count("id") ? vm["id"].as<std::string>() : "";
        if (!boost::filesystem::exists(db_file)){
            std::cout << db_file.string() << " doesn't exist." << std::endl;
        }
        else{
            while (id.empty()){
                std::cout << "Please enter the machine id:\n>";
                std::getline(std::cin, id);
            }
            while (mac.empty()){
                std::cout << "Please enter the mac address:\n>";
                std::getline(std::cin, mac);
            }
            license_db::ptr db = license_db::open(db_file.string(), macho::stringutils::tolower(mac) + std::string("SaaSaMeFTW"));
            if (!db){
                std::cout << "Cannot open the db file (" << db_file.string() << ")"<< std::endl;
            }
            else{
                macho::windows::protected_data _ps = macho::stringutils::toupper(id) + macho::stringutils::tolower(mac);
                saasame::transport::license::vtr _lics = db->get_licenses();
                saasame::transport::license::vtr activated_lics, non_activated_lics;
                std::remove_copy_if(_lics.begin(), _lics.end(), std::back_inserter(activated_lics), [](const saasame::transport::license::ptr& obj){return obj->lic.empty() || obj->status == "x"; });
                std::remove_copy_if(_lics.begin(), _lics.end(), std::back_inserter(non_activated_lics), [](const saasame::transport::license::ptr& obj){return !obj->lic.empty() || obj->status == "x"; });     
                license_info::vtr licenses;
                license_info::vtr invalid_licenses;
                foreach(saasame::transport::license::ptr lic, activated_lics){
                    license_info lic_info;
                    lic_info.key = lic->key;
                    lic_info.activated = macho::xor_crypto::decrypt(lic->active, _ps);
                    macho::LIC_INFO_V1 _lic;
                    memset(&_lic, 0, sizeof(macho::LIC_INFO_V1));
                    if (macho::license::unpack_keycode2(lic->lic, (std::string)_ps + lic_info.activated, _lic) && _lic.product == PRODUCT_CODE::Product_Transport){
                        lic_info.count = _lic.count;
                        if (_lic.year && _lic.mon && _lic.day)
                            lic_info.expired_date = boost::posix_time::to_simple_string(boost::posix_time::ptime(boost::gregorian::date(2000 + _lic.year, _lic.mon, _lic.day)));
                        licenses.push_back(lic_info);
                    }
                    else{
                        invalid_licenses.push_back(lic_info);
                    }
                }
                workload_consumed_map_type consumeds = calculated_license_consumed(db->get_workloads_map(), licenses);
                std::cout << "<< Licenses >>:" << std::endl;
                foreach(license_info& lic, licenses){
                    std::cout << "Activation key  : " << (lic.key == "0000000000000000000000000" ? "License count exceeded" : lic.key) << std::endl;
                    std::cout << "Activation Date : " << lic.activated << std::endl;
                    std::cout << "Credit          : " << lic.count << std::endl;
                    std::cout << "Used            : " << lic.consumed << std::endl;
                    if (!lic.expired_date.empty()){
                        std::cout << "Expiration Date : " << boost::posix_time::time_from_string(lic.expired_date).date().end_of_month() << std::endl;
                    }
                    std::cout << "Status            : " << lic.status << std::endl << std::endl;
                }
                std::cout << "<< Invalid Licenses >>:" << std::endl;
                foreach(license_info& lic, invalid_licenses){
                    std::cout << "Activation key  : " << lic.key << std::endl;
                    std::cout << "Activation Date : " << lic.activated << std::endl;
                }
                std::cout << "<< Invalid Licenses >>:" << std::endl;
                foreach(license_info& lic, invalid_licenses){
                    std::cout << "Activation key  : " << lic.key << std::endl;
                    std::cout << "Activation Date : " << lic.activated << std::endl;
                }
                std::cout << std::endl << "<< Workloads >>:" << std::endl;
                workload_history::vtr histories;
                foreach(workload_consumed_map_type::value_type &wc, consumeds){
                    workload_history wh;
                    wh.machine_id = wc.first;
                    workload::vtr workloads = db->get_workload(wc.first);
                    foreach(workload::ptr w, workloads){
                        wh.name = w->name;
                        wh.type = w->type;
                        break;
                    }
                    for (auto it = wc.second.cbegin(); it != wc.second.cend()/* not hoisted */; /* no increment */){
                        wh.histories.push_back(it->first);
                        wc.second.erase(it++);
                    }
                    histories.push_back(wh);
                }
                foreach(workload_history& w, histories){
                    std::cout << "Host            : " << w.name << std::endl;
                    std::cout << "Machine id      : " << w.machine_id << std::endl;
                    std::cout << "Histories       : " << std::endl;
                    int i = 0;
                    foreach(int h, w.histories){
                        int y = (h - 1) / 12;
                        int m = ((h - 1) % 12) + 1;
                        std::string o = boost::str(boost::format("%d/%02d") % y % m);
                        std::cout << std::left << std::setw(8) << o;
                        i++;
                        if (0 == i % 8)
                            std::cout << std::endl;
                    }
                    if (i % 8)
                        std::cout << std::endl << std::endl;
                    else
                        std::cout << std::endl;
                }
                std::cout << std::endl << "<< History >>:" << std::endl;
                workload::vtr workloads = db->get_workloads();
                foreach(workload::ptr w, workloads){
                    std::cout << "name    : " << w->name << std::endl;
                    std::cout << "created : " << boost::posix_time::to_simple_string(boost::posix_time::from_time_t(w->created)) << std::endl;
                    std::cout << "updated : " << boost::posix_time::to_simple_string(boost::posix_time::from_time_t(w->updated)) << std::endl;
                    std::cout << "deleted : " << boost::posix_time::to_simple_string(boost::posix_time::from_time_t(w->deleted)) << std::endl;
                    std::cout << "type    : " << w->type<< std::endl;
                    std::cout << "host    : " << w->host << std::endl;
                    std::cout << "comment : " << w->comment << std::endl << std::endl;
                }
            }
        }
    }
    return 0;
}

