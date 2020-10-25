// setup_inf_file.hpp ----------------------------------------------//
// -----------------------------------------------------------------------------

// Copyright 2013-2014 Albert Wei

// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt

// -----------------------------------------------------------------------------

// Revision History
// 04-29-2014 - Initial Release

// -----------------------------------------------------------------------------

#pragma once

#ifndef __MACHO_WINDOWS_SETUP_INF_FILE__
#define __MACHO_WINDOWS_SETUP_INF_FILE__
#include "..\config\config.hpp"
#include "environment.hpp"
#include "common\stringutils.hpp"
#include <map>
#include <vector>

namespace macho{

namespace windows{

class setup_inf_file{
public:
    struct target_os_version{
        typedef std::vector<target_os_version> vtr;
        target_os_version( stdstring str = _T(""));
        static bool sort_compare(const target_os_version& d1, const target_os_version& d2);
        stdstring decoration;
        stdstring architecture;
        DWORD     major;
        DWORD     minor;
        DWORD     product_type;
        DWORD     suite_mask;
    };

    struct manufacture_identifier{
        typedef std::vector<manufacture_identifier> vtr;
        stdstring  name; 
        stdstring  section_name;                   
        std::vector<target_os_version> target_os_versions;
        stdstring inline get_models_section_name() { 
            return ( target_os_versions.size() > 0 ) ?
            section_name + stdstring(_T(".")) + target_os_versions[0].decoration : section_name ;          
        }
    };

    struct device{
        typedef std::vector<device> vtr;
        stdstring       description;
        stdstring       install_section_name;
        stdstring       hardware_id;
        string_table    compatible_ids; 
    };
    
    struct source_disk{
        typedef std::map<int, source_disk > map;
        source_disk( stdstring str = _T(""));
        int              disk_id;
        stdstring        disk_description;
        stdstring        tag_or_cab_file;
        stdstring        unused;
        stdstring        path;
        DWORD            flags;
        stdstring        tag_file;
        bool             is_cab_file;
    };
    
    struct source_disk_file{
        typedef std::map<stdstring, source_disk_file, stringutils::no_case_string_less > map;
        source_disk_file( stdstring str = _T(""));
        stdstring file_name;
        int       disk_id;
        stdstring sub_dir;
        stdstring size; 
    };

    struct destination_directory{
        typedef std::map<stdstring, destination_directory, stringutils::no_case_string_less > map;
        destination_directory( stdstring str =_T(""));
        stdstring   list_section;
        int         dir_id;
        stdstring   sub_dir;
    };  

    struct setup_action{
        typedef boost::shared_ptr<setup_action> ptr;
        typedef std::vector<ptr> vtr;
        virtual ~setup_action(){}
        virtual bool is_valid() = 0;
        virtual bool install(reg_edit_base& reg_edit) = 0;
        virtual bool exists(reg_edit_base& reg_edit) = 0;
        virtual bool remove(reg_edit_base& reg_edit) = 0;
    };

    struct setup_action_file : public setup_action{
        enum actions{
            FILE_NO_ACTION = 0x0,
            FILE_COPY = 0x1,
            FILE_DELETE,
            FILE_RENAME
        };
        typedef boost::shared_ptr<setup_action_file> ptr;
        setup_action_file() : action(FILE_NO_ACTION), flags(0), is_system_driver(false), is_in_cab(false){}
        stdstring               source_file_name;
        stdstring               target_file_name;
        boost::filesystem::path source_path;
        boost::filesystem::path target_path;
        stdstring               sub_dir;
        actions                 action;
        DWORD                   flags;
        bool                    is_system_driver;
        bool                    is_in_cab;
        bool                    is_copy_replace_only(); 
        bool                    is_copy_no_overwrite(); 
        virtual ~setup_action_file(){}
        virtual bool is_valid(){ return target_file_name.length() && target_path.string().length(); }
        virtual bool install(reg_edit_base& reg_edit);
        virtual bool exists(reg_edit_base& reg_edit);
        virtual bool remove(reg_edit_base& reg_edit);
        static bool need_overwrite(const boost::filesystem::path source, const boost::filesystem::path target);
        static bool is_execution_file(const boost::filesystem::path file);
    };

    struct setup_action_registry : public setup_action{
        typedef boost::shared_ptr<setup_action_registry> ptr;
        setup_action_registry() : flags(0), is_removed(false), root(0){}
        HKEY                      root;
        stdstring                 root_key;
        stdstring                 key_path;
        stdstring                 value_name;
        DWORD                     flags;
        bool                      is_removed;
        stdstring                 value;
        virtual ~setup_action_registry(){}
        virtual bool is_valid() { return root > 0; }
        virtual bool install(reg_edit_base& reg_edit);
        virtual bool exists(reg_edit_base& reg_edit);
        virtual bool remove(reg_edit_base& reg_edit);
    };

    struct setup_action_service_entry : public setup_action {
        typedef boost::shared_ptr<setup_action_service_entry> ptr;
        typedef std::vector<ptr> vtr;
        setup_action_service_entry() : flags(0){}
        stdstring                       entry_name;
        stdstring                       service_path;
        DWORD                           flags;
        std::vector<stdstring>          values;
        virtual bool is_valid() { return entry_name.length() > 0; }
        virtual bool install(reg_edit_base& reg_edit);
        virtual bool exists(reg_edit_base& reg_edit);
        virtual bool remove(reg_edit_base& reg_edit);
    };

    struct setup_action_service : public setup_action{
        typedef boost::shared_ptr<setup_action_service> ptr;
        setup_action_service() : start(0), flags(0) {}
        stdstring                          name;
        stdstring                          class_guid;
        DWORD                              start;
        DWORD                              flags;
        setup_action_service_entry::vtr    entries;
        virtual ~setup_action_service(){}
        virtual bool is_valid() { return name.length() > 0; }
        virtual bool install(reg_edit_base& reg_edit);
        virtual bool exists(reg_edit_base& reg_edit);
        virtual bool remove(reg_edit_base& reg_edit);
    };

    struct match_device_info{
        typedef boost::shared_ptr<match_device_info> ptr;
        typedef std::vector<ptr> vtr;
        stdstring                   manufacturer;
        stdstring                   architecture;
        stdstring                   matched_id;
        stdstring                   matched_inf_path;
        stdstring                   device_description;
        stdstring                   driver_version;
        ULONG                       rank;
        bool                        is_compatible_id;
        ULONGLONG                   version;
        boost::posix_time::ptime    date_time;
        stdstring                   install_section;
        bool                        is_driver_signed;
        stdstring                   signer_name;
        DWORD                       signer_score;
        bool                        has_driver_store;
        match_device_info() : is_driver_signed(false), is_compatible_id(false), has_driver_store(false), version(0), signer_score(0){}
    };

    typedef boost::shared_ptr<setup_inf_file> ptr;
    typedef std::vector<ptr> vtr;
    static bool sort_compare(const setup_inf_file::ptr& f1, const setup_inf_file::ptr& f2);
 
    setup_inf_file(){}
    virtual ~setup_inf_file(){ }
    bool load( const stdstring file );

    stdstring inline signature(){
        return _versions.count(_T("Signature")) > 0 ? _versions[_T("Signature")] : _T("");
    }

    stdstring inline class_name(){
        return _versions.count(_T("Class")) > 0 ? _versions[_T("Class")] : _T("");
    }

    stdstring inline class_guid(){
        return _versions.count(_T("ClassGuid")) > 0 ? _versions[_T("ClassGuid")] : _T("");
    }

    bool inline is_scsi_raid_controllers(){
        return  (0 == _tcsicmp(class_guid().c_str(), _T("{4D36E97B-E325-11CE-BFC1-08002BE10318}")));
    }

    bool inline is_hard_disk_controllers(){
        return  (0 == _tcsicmp(class_guid().c_str(), _T("{4D36E96A-E325-11CE-BFC1-08002BE10318}")));
    }

    stdstring inline driver_version_string(){
        return _versions.count(_T("DriverVer")) > 0 ? _versions[_T("DriverVer")] : _T("");
    }

    stdstring inline catalog_file(){
        return _versions.count(_T("CatalogFile")) > 0 ? _versions[_T("CatalogFile")] : _T("");
    }

    stdstring inline driver_version() const {
        return _driver_version;
    }      
    
    stdstring inline driver_date() const {
        using namespace boost::posix_time;
#if _UNICODE
        static std::locale loc(std::wcout.getloc(),
            new wtime_facet(L"%Y-%m-%d"));
        std::basic_stringstream<wchar_t> wss;
        wss.imbue(loc);
        wss << _datetime;
        return wss.str();
#else
        static std::locale loc(std::cout.getloc(),
            new time_facet("%Y-%m-%d"));
        std::basic_stringstream<char_t> ss;
        ss.imbue(loc);
        ss << _datetime;
        return ss.str();
#endif
    }
    
    stdstring inline driver_package_display_name(){
        return _versions.count(_T("DriverPackageDisplayName")) > 0 ? expand_string_internal( _strings, _versions[_T("DriverPackageDisplayName")]) : _T("");
    }
        
    stdstring inline driver_package_type(){
        return _versions.count(_T("DriverPackageType")) > 0 ? expand_string_internal( _strings, _versions[_T("DriverPackageType")] ) : _T("");
    }

    stdstring inline provider(){
        return _versions.count(_T("Provider")) > 0 ? expand_string_internal( _strings, _versions[_T("Provider")] ) : _T("");
    }

    std::vector<stdstring> layout_files(){
        std::vector<stdstring> layout_files;
        if ( _versions.count(_T("LayoutFile")) > 0 )
            layout_files = stringutils::tokenize2( _versions[_T("LayoutFile")], _T(",") );
        return layout_files;
    }
    
    string_table get_section_lines( stdstring name );
    
    //in some cases, it may not enumerate all sections' name.
    std::vector<stdstring> get_section_names();

    std::map< stdstring, stdstring, stringutils::no_case_string_less> inline strings() const { return _strings; }

    source_disk::map   inline       get_source_disks(operating_system& os){
        return get_source_disks(os.sz_cpu_architecture());
    }
    
    source_disk_file::map inline    get_source_disk_files(operating_system& os){
        return get_source_disk_files(os.sz_cpu_architecture());
    }

    destination_directory::map      get_destination_directories();
    manufacture_identifier::vtr     get_manufacture_identifiers( operating_system& os );
    device::vtr                     get_devices( manufacture_identifier& mf );
    static stdstring                expand_string(std::map<stdstring, stdstring, stringutils::no_case_string_less>& maps, const stdstring str);
    static string_array             expand_strings(std::map<stdstring, stdstring, stringutils::no_case_string_less>& maps, const string_array arr);

    static match_device_info::ptr verify(operating_system& os, boost::filesystem::path inf, const string_array &hardware_ids, const string_array& compatible_ids){
        setup_inf_file _inf;
        if (_inf.load(inf.wstring()))
            return _inf.verify(os, hardware_ids, compatible_ids);
        return NULL;
    }

    match_device_info::ptr               verify(operating_system& os, const string_array &hardware_ids, const string_array& compatible_ids);
    setup_action::vtr                    get_setup_actions(match_device_info::ptr match_device, boost::filesystem::path win_dir = environment::get_windows_directory(), reg_edit_base& reg_edit = reg_native_edit(), stdstring device_instance_id = _T(""));
    static void                          earse_unused_oem_driver_packages(boost::filesystem::path win_dir = environment::get_windows_directory(), reg_edit_base& reg_edit = reg_native_edit());
private:
    static void                          remove_subkey_value(reg_edit_base& reg_edit, stdstring key_path, stdstring value_name);
    typedef struct _DRIVER_VERSION
    {
        ULONGLONG    Flag;
        UUID         Class;
        FILETIME     Date;
        ULONGLONG    Version;
        DWORD        Reserved;
    }DRIVER_VERSION, *PDRIVER_VERSION;

    source_disk::map                     get_source_disks(stdstring architecture);
    source_disk_file::map                get_source_disk_files(stdstring architecture);
    setup_action::vtr                    device_driver_installs(stdstring install_section, boost::filesystem::path& win_dir, stdstring architecture, stdstring hw_hkr, stdstring sw_hkr);
    setup_action::vtr                    device_driver_install(stdstring install_section, const boost::filesystem::path& win_dir, stdstring architecture, stdstring hkr = _T(""));
    setup_action::vtr                    add_service(string_array& values);
    setup_action::vtr                    needs(setup_inf_file::vtr& infs, string_array& values, const boost::filesystem::path& win_dir, stdstring architecture);
    setup_inf_file::vtr                  include(string_array& values, const boost::filesystem::path& win_dir);
    setup_action::vtr                    reg_edit(string_array& values, stdstring hkr_path, bool is_remove);
    setup_action::vtr                    copy_files(string_array& values, const boost::filesystem::path& win_dir, stdstring architecture);

    ULONGLONG                       version();
    void                            cleanup(){
        //_sections.clear();
        _file_path.clear();
        _driver_version.clear();
        _versions.clear();
        _strings.clear();
        _datetime = boost::posix_time::ptime();
    }
    stdstring                       remove_comment( const stdstring str );
    
    std::map<stdstring, stdstring, stringutils::no_case_string_less>   get_key_string_map(string_table& lines);
    
    static stdstring                expand_string_internal(std::map<stdstring, stdstring, stringutils::no_case_string_less>& maps, const stdstring str);
    

    stdstring                                   _file_path;
    stdstring                                   _driver_version;
    boost::posix_time::ptime                    _datetime;
    std::map< stdstring, stdstring, stringutils::no_case_string_less >             _versions;
    std::map< stdstring, stdstring, stringutils::no_case_string_less >             _strings;
    std::map<stdstring, string_table, stringutils::no_case_string_less >           _sections;
    bool is_driver_signed(operating_system& os, stdstring &signer_name, DWORD &signer_score);
    static stdstring get_message(DWORD dw);
};

#ifndef MACHO_HEADER_ONLY
#include <SetupAPI.h>

bool setup_inf_file::sort_compare(const setup_inf_file::ptr& f1, const setup_inf_file::ptr& f2){
    if (f1->version() > f2->version())
        return true;
    else
        return false;
}

stdstring setup_inf_file::get_message(DWORD dw)
{
    // Retrieve the system error message for the system error code
    LPVOID      lpMsgBuf;
    stdstring   message;
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&lpMsgBuf,
        0, NULL);

    if (lpMsgBuf){
        message = (LPTSTR)lpMsgBuf;
        message = stringutils::erase_trailing_whitespaces(message);
        LocalFree(lpMsgBuf);
    }
    stringutils::format(message, _T("( 0x%08X : %s )"), dw, message.c_str());
    return message;
}

ULONGLONG setup_inf_file::version(){
    string_array values = stringutils::tokenize(_driver_version, _T("."), 5);
    WORD    version[4] = { 0, 0, 0, 0 };
    switch (values.size()){
    case 5:
    case 4:
        version[3] = values[3].length() ? _ttoi(values[3].c_str()) : 0;
    case 3:
        version[2] = values[2].length() ? _ttoi(values[2].c_str()) : 0;
    case 2:
        version[1] = values[1].length() ? _ttoi(values[1].c_str()) : 0;
    case 1:
        version[0] = values[0].length() ? _ttoi(values[0].c_str()) : 0;
    };
    return MAKEDLLVERULL(version[0], version[1], version[2], version[3]);
}

setup_inf_file::target_os_version::target_os_version( stdstring str ) : major(0), minor(0), product_type(0), suite_mask(0) {
    if ( str.length() ){
        string_table values = stringutils::tokenize2(str, _T("."));
        decoration = str;
        switch (values.size()){
        case 5:
            if (values[4].length() > 0)
                suite_mask = _tcstol(values[4].c_str(), NULL, 0);
        case 4:
            if (values[3].length() > 0)
                product_type = _tcstol(values[3].c_str(), NULL, 0);
        case 3:
            if (values[2].length() > 0)
                minor = _ttol(values[2].c_str());
        case 2:
            if (values[1].length() > 0)
                major = _ttol(values[1].c_str());
        case 1:
            if (((values[0].at(0) == _T('N')) || (values[0].at(0) == _T('n'))) &&
                ((values[0].at(1) == _T('T')) || (values[0].at(1) == _T('t'))))    {
                architecture = values[0].substr(2, values[0].length() - 2);
            }
        default:
            ;
        }    
    }    
}

bool setup_inf_file::target_os_version::sort_compare(const target_os_version& d1, const target_os_version& d2){
    if ( d1.major > d2.major )
        return true;
    else if ( ( d1.major == d2.major ) && ( d1.minor > d2.minor ) )
        return true;
    else  if ( ( d1.major == d2.major ) && ( d1.minor == d2.minor ) ){    
        if ( d1.architecture.length() == d2.architecture.length() ){
            if( ( ( d1.suite_mask == 0 ) && (d2.suite_mask == 0 ) ) || ( d1.suite_mask & d2.suite_mask ) ){
                if ( d1.product_type == d2.product_type )
                    return false;
                else if ( d1.product_type != 0 )
                    return true;
                else
                    return false;
            }
            else if ( d1.suite_mask != 0 )
                return true;
            else
                return false;
        }
        else if ( d1.architecture.length() > 0 )
            return true;
        else
            return false;
    }
    else
        return false;    
}
     
setup_inf_file::source_disk::source_disk( stdstring str ): disk_id(0), flags(0), is_cab_file(false) {
    string_table tb_key_value = stringutils::tokenize2( str, _T("=") );
    if ( tb_key_value.size() > 1){
        disk_id = _ttol(tb_key_value[0].c_str());
        if ( tb_key_value[1].length() ){
            string_table values = stringutils::tokenize2(tb_key_value[1], _T(","), 7);
            switch (values.size()){
            case 7:
            case 6:
                if (values[5].length() > 0)
                    tag_file = stringutils::parser_double_quotation_mark(values[5]);
            case 5:
                if (values[4].length() > 0)
                    flags = _tcstol(values[4].c_str(), NULL, 0);
            case 4:
                if (values[3].length() > 0)
                    path = stringutils::parser_double_quotation_mark(values[3]);
            case 3:
                if (values[2].length() > 0)
                    unused = values[2];
            case 2:
                if (values[1].length() > 0){
                    stdstring::size_type pos = 0; 
                    stdstring extension;
                    tag_or_cab_file = stringutils::parser_double_quotation_mark(values[1]);
                    if( stdstring::npos != ( pos = tag_or_cab_file.find(_T(".") ) ) )
                        extension = tag_or_cab_file.substr(pos + 1, tag_or_cab_file.length() - pos - 1);
                    if (0 == _tcsicmp(extension.c_str(), _T("cab")))
                        is_cab_file = true;
                    else{
                        if ( 0x10 == flags )
                            is_cab_file = true;
                    }
                }
            case 1:
                disk_description = values[0];
            default:
                ;
            }    
        }
    }
}

setup_inf_file::source_disk_file::source_disk_file( stdstring str ) : disk_id(0){
    string_table tb_file_disk = stringutils::tokenize2( str, _T("=") );
    if ( tb_file_disk.size() > 1){
        file_name = tb_file_disk[0];
        if ( tb_file_disk[1].length() ){
            string_table values = stringutils::tokenize2(tb_file_disk[1], _T(","), 4);
            switch (values.size()){
            case 4:
            case 3:
                if (values[2].length() > 0)
                    size = values[2];
            case 2:
                if (values[1].length() > 0)
                    sub_dir = stringutils::parser_double_quotation_mark(values[1]);
            case 1:
                disk_id = _ttol(values[0].c_str());
            default:
                ;
            }    
        }
    }  
}

setup_inf_file::destination_directory::destination_directory( stdstring str ) : dir_id(0){
    string_table tb_section_dir = stringutils::tokenize2( str, _T("=") );
    if ( tb_section_dir.size() > 1){
        list_section = tb_section_dir[0];
        if ( tb_section_dir[1].length() )    {
            string_table values = stringutils::tokenize2(tb_section_dir[1], _T(","), 3);
            switch (values.size()){
            case 3:
            case 2:
                if (values[1].length() > 0)
                    sub_dir = stringutils::parser_double_quotation_mark(values[1]);
            case 1:
                dir_id = _ttol(values[0].c_str());
            default:
                ;
            }    
        }
    }
}

setup_inf_file::source_disk::map setup_inf_file::get_source_disks(stdstring architecture){
    stdstring section_name = _T("SourceDisksNames");
    setup_inf_file::source_disk::map source_disks;

    if (architecture.length() > 0){
        section_name.append(_T("."));
        section_name.append(architecture);
        foreach( stdstring str, get_section_lines(section_name)){
            setup_inf_file::source_disk source_disk(str);
            source_disks[source_disk.disk_id] = source_disk;
        }
    }

    if ( source_disks.size() == 0 ){
        section_name = _T("SourceDisksNames");
        foreach( stdstring str, get_section_lines(section_name)){
            setup_inf_file::source_disk source_disk(str);
            source_disks[source_disk.disk_id] = source_disk;
        }
    }

    return source_disks;
}

setup_inf_file::source_disk_file::map setup_inf_file::get_source_disk_files(stdstring architecture){
    stdstring section_name = _T("SourceDisksFiles");
    setup_inf_file::source_disk_file::map source_disk_files;

    if (architecture.length() > 0){
        section_name.append(_T("."));
        section_name.append(architecture);
        foreach( stdstring str, get_section_lines(section_name)){
            setup_inf_file::source_disk_file source_disk_file(str);
            source_disk_files[source_disk_file.file_name] = source_disk_file;
        }
    }

    if ( source_disk_files.size() == 0 ){ 
        section_name = _T("SourceDisksFiles");
        foreach( stdstring str, get_section_lines(section_name)){
            setup_inf_file::source_disk_file source_disk_file(str);
            source_disk_files[source_disk_file.file_name] = source_disk_file;
        }
    }
    return source_disk_files;
}

setup_inf_file::destination_directory::map setup_inf_file::get_destination_directories(){
    stdstring section_name = _T("DestinationDirs");
    setup_inf_file::destination_directory::map destination_directories;
    foreach( stdstring str, get_section_lines(section_name)){
        setup_inf_file::destination_directory destination_directory(str);
        if ( destination_directory.list_section.length() && (  destination_directory.dir_id > 0 ) )
            destination_directories[destination_directory.list_section] = destination_directory;
    }
    return destination_directories;
}

setup_inf_file::manufacture_identifier::vtr setup_inf_file::get_manufacture_identifiers( operating_system& os ){
    stdstring section_name = _T("Manufacturer");
    setup_inf_file::manufacture_identifier::vtr identifiers;
    foreach( stdstring str, get_section_lines(section_name)){
        std::vector<stdstring> txtArray = stringutils::tokenize2( str, _T("=") );
        if ( 0 != txtArray.size() ){
            setup_inf_file::manufacture_identifier identifier;
            identifier.name =  expand_string_internal( _strings, txtArray[0] );
            if ( 1 == txtArray.size() ){
                identifier.section_name = identifier.name;
            }
            else{
                std::vector<stdstring> txtModules = stringutils::tokenize2( txtArray[1], _T(",") );
                if ( os.is_win2000() || ( 1 == txtModules.size() ) ){
                    identifier.section_name = txtModules[0];
                }
                else{
                    std::vector<target_os_version> versions, temp_versions;
                    for( size_t index = 1; index < txtModules.size(); index++ ){
                        target_os_version version( txtModules[index] );
                        versions.push_back( version );
                    }
                    foreach( target_os_version target_version, versions ){
                        if ( ( target_version.architecture.length() == 0 ) || 
                            (    0 == _tcsicmp( target_version.architecture.c_str(), os.sz_cpu_architecture().c_str() ) ) ){
                                if ( ( ( target_version.product_type == 0 ) || ( target_version.product_type == os.version_info.wProductType ) ) &&
                                    ( ( target_version.suite_mask == 0 ) || ( target_version.suite_mask == os.version_info.wSuiteMask ) ) )    {
                                        if ( target_version.major <= os.major_version() ){
                                            if ( target_version.major == os.major_version() ){
                                                if ( target_version.minor <= os.minor_version() )
                                                    temp_versions.push_back(target_version);
                                            }
                                            else
                                                temp_versions.push_back(target_version);
                                        }
                                }                                                           
                        }
                    }
                    versions.clear();
                    foreach( target_os_version target_version, temp_versions ){
                        if ( target_version.architecture.length() == 0  ){
                            if ( ( ( os.major_version() == 5 ) && ( os.minor_version() < 2 ) ) || 
                                ( os.is_win2003() && ( os.servicepack_major() < 1 ) ) )
                                versions.push_back(target_version);
                            else if ( os.is_x86() )
                                versions.push_back(target_version);
                        }
                        else
                            versions.push_back(target_version);
                    }
                    std::sort( versions.begin(), versions.end(), target_os_version::sort_compare );
                    if ( versions.size() > 0 ){
                        identifier.section_name = txtModules[0];
                        identifier.target_os_versions = versions; 
                    }
                    else if( ( ( os.major_version() == 5 ) && ( os.minor_version() < 2 ) ) || 
                        ( os.is_win2003() && ( os.servicepack_major() < 1 ) ) || 
                        os.is_x86() ){
                            identifier.section_name = txtModules[0];
                    }
                }
            }
            identifiers.push_back(identifier);
        }
    }  
    return identifiers;
}

setup_inf_file::device::vtr setup_inf_file::get_devices( setup_inf_file::manufacture_identifier& mf ){
    setup_inf_file::device::vtr devices;
    stdstring section_name = mf.get_models_section_name();
    foreach( stdstring str, get_section_lines(section_name)){
        setup_inf_file::device device;
        string_table txtArray = stringutils::tokenize2( str, _T("="), 2 );
        if ( 2 == txtArray.size() ){
            string_table txtIds   = stringutils::tokenize2( txtArray[1], _T(",") );
            if ( txtIds.size() > 1 ){
                device.description = expand_string_internal( _strings, txtArray[0] );
                device.install_section_name = txtIds[0];
                device.hardware_id          = txtIds[1];
                for( size_t index = 2; index < txtIds.size() ; index ++ ){
                    device.compatible_ids.push_back(txtIds[index]);
                }
                devices.push_back( device );
            }
        }   
    }  
    return devices;
}

bool setup_inf_file::load( stdstring file ){
    cleanup(); 
    if ( file.length() > 0 && boost::filesystem::exists(file) ){
        _file_path = file;
        _strings = get_key_string_map( get_section_lines(_T("strings")) );
        _versions = get_key_string_map( get_section_lines(_T("version")) );
        string_table drv_versions =  stringutils::tokenize2( driver_version_string(), _T(","), 2 );
        if ( drv_versions.size() > 1 ) _driver_version = drv_versions[1];
        if ( drv_versions.size() > 0 ){
            DWORD   datetime[3] = {0,0,0};
            SYSTEMTIME date;
            FILETIME   file_time;
            memset(&date, 0, sizeof(SYSTEMTIME));
            memset(&file_time, 0, sizeof(FILETIME));
            if( 3 == _stscanf_s( drv_versions[0].c_str(), _T("%d/%d/%d"), &datetime[0], &datetime[1], &datetime[2] ) ){
                date.wMonth = (WORD)datetime[0];
                date.wDay = (WORD)datetime[1];
                date.wYear = (WORD)datetime[2];
            }
            SystemTimeToFileTime(&date, &file_time);
            _datetime = boost::posix_time::from_ftime<boost::posix_time::ptime, FILETIME>(file_time);
        }  
    }
    std::vector<stdstring> sections = get_section_names();
    foreach(stdstring section, sections){
        _sections[section] = get_section_lines(section);
    }
    return _versions.size() > 0 ? true : false;
}

stdstring setup_inf_file::remove_comment( const stdstring str ){
    stdstring temp;
    size_t found;
    stdstring delimiters (_T(";\""));
    if ( str.length() > 0 ){
        found = str.find_last_of( delimiters );
        if ( found != stdstring::npos )    {
            if ( str.at( found ) == _T('\"') )
                temp = str;
            else if ( str.at( found ) == _T(';') )
                temp = str.substr( 0, found );
        }
        else{
            temp = str;
        }
        temp = stringutils::erase_trailing_whitespaces( temp );
        temp = stringutils::remove_begining_whitespaces( temp );
    }
    return temp;
}

string_table    setup_inf_file::get_section_lines( stdstring section_name ){

    DWORD            size = 1024, ret = 0;
    bool            append = false;
    stdstring       line;
    string_table    keys, lines;

    std::auto_ptr<TCHAR> p = std::auto_ptr<TCHAR>(new TCHAR[size]);
    if ( p.get() != NULL ){
        memset( p.get(), 0, size * sizeof(TCHAR) );
        ret = GetPrivateProfileSection( section_name.c_str(), p.get(), size, _file_path.c_str() );
        while ( ret == ( size - 2 ) ){
            size = ret * 2;
            p = std::auto_ptr<TCHAR>(new TCHAR[size]);
            if ( p.get() != NULL ){
                memset( p.get(), 0, size * sizeof(TCHAR) );
                ret = GetPrivateProfileSection( section_name.c_str(), p.get(), size, _file_path.c_str() ); 
            }
            else
                break;
        }

        if (  p.get() != NULL )    {    
            if ( ret > 0 ){
                for( TCHAR*   Key = p.get();   *Key != _T('\0');   Key +=_tcslen(Key) + 1 ){
                    keys.push_back( Key );
                }
            }
        }
    }
    foreach( stdstring key, keys ){
        stdstring temp = remove_comment(key);      
        if ( temp.length() ){
            if ( append ){
                line = line.substr( 0, line.length() - 1 );
                line += temp;
            }
            else
                line = temp;

            if ( line.at( line.length() -1 ) == _T('\\'))
                append = true;
            else{
                append = false;
                lines.push_back( line );
                line.clear();
            }
        }
    }
    return lines;
}

std::map<stdstring, stdstring, stringutils::no_case_string_less>   setup_inf_file::get_key_string_map(string_table& lines){
    std::map<stdstring, stdstring, stringutils::no_case_string_less>  key_values;
    foreach( stdstring line, lines ){
        stdstring key;
        stdstring value;
        size_t    found = line.find_first_of(_T("="));
        if ( found != stdstring::npos ){
            key = line.substr( 0 , found );
            value = line.substr( found + 1 , line.length() - found );
            key = stringutils::parser_double_quotation_mark( key );
            value = stringutils::parser_double_quotation_mark( value );
            key_values[key] = value;
        }            
    }
    return key_values;
}

stdstring setup_inf_file::expand_string(std::map<stdstring, stdstring, stringutils::no_case_string_less>& maps, const stdstring str){
    std::map<stdstring, stdstring, stringutils::no_case_string_less> _maps;
    for( std::map<stdstring,stdstring>::iterator p = maps.begin(); p != maps.end(); p++ ){
        _maps[p->first] = stringutils::parser_double_quotation_mark( p->second ); 
    }
    return stringutils::parser_double_quotation_mark(expand_string_internal(_maps, str));
}

string_array setup_inf_file::expand_strings(std::map<stdstring, stdstring, stringutils::no_case_string_less>& maps, const string_array arr){
    string_array results;
    std::map<stdstring, stdstring, stringutils::no_case_string_less> _maps;
    for (std::map<stdstring, stdstring>::iterator p = maps.begin(); p != maps.end(); p++){
        _maps[p->first] = stringutils::parser_double_quotation_mark(p->second);
    }
    foreach (stdstring s, arr){
        results.push_back(stringutils::parser_double_quotation_mark(expand_string_internal(_maps, s)));
    }
    return results;
}

stdstring setup_inf_file::expand_string_internal(std::map<stdstring, stdstring, stringutils::no_case_string_less>& maps, const stdstring str){
    
    static std::map<stdstring, stdstring> dir_ids_map = {
        { _T("10"), _T("\\SystemRoot") },
        { _T("11"), _T("\\SystemRoot\\System32") },
        { _T("12"), _T("\\SystemRoot\\System32\\Drivers") },
        { _T("17"), _T("\\SystemRoot\\INF") },
        { _T("18"), _T("\\SystemRoot\\Help") },
        { _T("20"), _T("\\SystemRoot\\Fonts") },
        { _T("50"), _T("\\SystemRoot\\System") },
    };

    stdstring::size_type pos = 0; 
    stdstring             result;
    if( stdstring::npos != ( pos = str.find(_T("%") ) ) ){
        if( ( str.length() > (pos + 1) ) && ( str.at( pos + 1 ) == _T('%') ) ){
            result = str.substr( 0, pos + 1 ) + expand_string( maps, str.substr( pos + 2 ) );
        }
        else{ 
            if ( pos ) result = str.substr( 0, pos );
            stdstring post = str.substr( pos + 1 );
            if( stdstring::npos != ( pos = post.find(_T("%") ) ) ){
                if ( pos ){
                    stdstring key = post.substr( 0, pos );
                    if ( dir_ids_map.count(key) > 0 )
                        result.append(dir_ids_map[key]);
                    else if ( maps.count(key) > 0 )
                        result.append(maps[key]);
                    else{
                        result.append( _T("%"));
                        result.append( key );
                        result.append( _T("%"));
                    }
                }
                result.append( expand_string( maps, post.substr( pos + 1 ) ) );
            }
        }
    }
    else
        result = str;
    return result;
}

std::vector<stdstring> setup_inf_file::get_section_names(){
    std::vector<stdstring>        names;
    //for( std::map<stdstring, string_table, stringutils::no_case_string_less >::iterator p = _sections.begin(); p != _sections.end(); p++ )
    //    names.push_back( p->first );

    DWORD size = 1024, ret = 0;
    std::auto_ptr<TCHAR> p = std::auto_ptr<TCHAR>(new TCHAR[size]);
    if (p.get() != NULL){
        memset(p.get(), 0, size * sizeof(TCHAR));
        ret = GetPrivateProfileSectionNames(p.get(), size, _file_path.c_str());

        while (ret == (size - 2)){
            size = ret * 2;
            std::auto_ptr<TCHAR> p = std::auto_ptr<TCHAR>(new TCHAR[size]);
            if (p.get() != NULL){
                memset(p.get(), 0, size * sizeof(TCHAR));
                ret = GetPrivateProfileSectionNames(p.get(), size, _file_path.c_str());
            }
            else
                break;
        }

        if (p.get() != NULL){
            if (ret){
                for (TCHAR* name = p.get(); *name != _T('\0'); name += _tcslen(name) + 1) {
                    names.push_back(name);
                }
            }
        }
    }
    return names;
}

bool setup_inf_file::is_driver_signed(operating_system& os, stdstring &signer_name, DWORD &signer_score){

    DWORD               dwRet = ERROR_SUCCESS;
    SP_INF_SIGNER_INFO  InfFileName;
    SP_ALTPLATFORM_INFO AltPlatformInfo;
    bool                bIsInfSigned = false;
    bool                bIsDriverFileSigned = false;
    operating_system    current_os = environment::get_os_version();
    memset(&InfFileName, 0, sizeof(SP_INF_SIGNER_INFO));
    memset(&AltPlatformInfo, 0, sizeof(SP_ALTPLATFORM_INFO));

    InfFileName.cbSize = sizeof(SP_INF_SIGNER_INFO);
    AltPlatformInfo.cbSize = sizeof(SP_ALTPLATFORM_INFO);
    if (os.is_x86())
        AltPlatformInfo.ProcessorArchitecture = PROCESSOR_ARCHITECTURE_INTEL;
    else if (os.is_ia64())
        AltPlatformInfo.ProcessorArchitecture = PROCESSOR_ARCHITECTURE_IA64;
    else if (os.is_amd64())
        AltPlatformInfo.ProcessorArchitecture = PROCESSOR_ARCHITECTURE_AMD64;

    AltPlatformInfo.MajorVersion = current_os.major_version();
    AltPlatformInfo.FirstValidatedMajorVersion = os.major_version();
    AltPlatformInfo.MinorVersion = current_os.minor_version();
    AltPlatformInfo.FirstValidatedMinorVersion = os.minor_version();
    AltPlatformInfo.Platform = VER_PLATFORM_WIN32_NT;
    if ((os.major_version() == 5 && os.minor_version() > 0) || os.major_version() > 5)
        AltPlatformInfo.Flags = SP_ALTPLATFORM_FLAGS_VERSION_RANGE;

    if (!SetupVerifyInfFile(_file_path.c_str(), &AltPlatformInfo, &InfFileName)){
        dwRet = GetLastError();     
        if (dwRet == ERROR_AUTHENTICODE_TRUSTED_PUBLISHER){
            signer_name = InfFileName.DigitalSigner;
            signer_score = InfFileName.SignerScore;
            LOG(LOG_LEVEL_INFO, _T("SetupVerifyInfFile(%s) Failed : Indicates that the publisher is trusted because the publisher's certificate is installed in the Trusted Publishers certificate store."), _file_path.c_str());
            bIsInfSigned = true;
            dwRet = ERROR_SUCCESS;
        }
        else if (dwRet == ERROR_AUTHENTICODE_TRUST_NOT_ESTABLISHED){
            signer_name = InfFileName.DigitalSigner;
            signer_score = InfFileName.SignerScore;
            LOG(LOG_LEVEL_INFO, _T("SetupVerifyInfFile(%s) Failed : Indicates that trust cannot be automatically established because the publisher's signing certificate is not installed in the trusted publisher certificates store. However, this does not necessarily indicate an error. Instead it indicates that the caller must apply a caller-specific policy to establish trust in the publisher."), _file_path.c_str());
            bIsInfSigned = true;
            dwRet = ERROR_SUCCESS;
        }
    }
    else{
        signer_name = InfFileName.DigitalSigner;
        signer_score = InfFileName.SignerScore;
        LOG(LOG_LEVEL_DEBUG, _T("SetupVerifyInfFile(%s) is succeeded."), _file_path.c_str());
        bIsInfSigned = true;
    }
    //if ( not is Inf File Only ){
    ////SetupScanFileQueue and SPQ_SCAN_USE_CALLBACK_SIGNERINFO
    //}
    bIsDriverFileSigned = true;

    return (bIsInfSigned && bIsDriverFileSigned);
}

setup_inf_file::match_device_info::ptr setup_inf_file::verify(operating_system& os, const string_array &hardware_ids, const string_array& compatible_ids){
    setup_inf_file::match_device_info::ptr device_match = setup_inf_file::match_device_info::ptr(new setup_inf_file::match_device_info());
    if (os.is_win2003_or_later()){
        device_match->has_driver_store = os.is_win8_or_later();
        device_match->is_driver_signed = is_driver_signed(os, device_match->signer_name, device_match->signer_score);
        device_match->architecture = os.sz_cpu_architecture();
        manufacture_identifier::vtr manufactures = get_manufacture_identifiers(os);
        if (manufactures.size()){
            foreach(manufacture_identifier &m, manufactures){
                device::vtr devices = get_devices(m);
                foreach(device &d, devices){
                    DWORD dwRank = device_match->is_driver_signed ? 0x0000 : (m.target_os_versions.size() ? 0x8000 : 0xC000);     
                    for (size_t index = 0; index < hardware_ids.size(); index++){
                        if (0 == _tcsicmp(d.hardware_id.c_str(), hardware_ids[index].c_str())){
                            device_match->manufacturer = m.name;
                            device_match->device_description = d.description;
                            device_match->install_section = d.install_section_name;
                            device_match->matched_id = hardware_ids[index];
                            device_match->matched_inf_path = _file_path;
                            device_match->driver_version = driver_version();
                            device_match->date_time = _datetime;
                            device_match->version = version();
                            device_match->is_compatible_id = false;
                            device_match->rank = dwRank + (DWORD)index;
                            return device_match;
                        }
                    }
  
                    for (size_t i = 0; i < d.compatible_ids.size(); i++){
                        for (size_t index = 0; index < compatible_ids.size(); index++){
                            if (0 == _tcsicmp(d.compatible_ids[i].c_str(), compatible_ids[index].c_str())){
                                device_match->manufacturer = m.name;
                                device_match->device_description = d.description;
                                device_match->install_section = d.install_section_name;
                                device_match->matched_id = compatible_ids[index];
                                device_match->matched_inf_path = _file_path;
                                device_match->driver_version = driver_version();
                                device_match->date_time = _datetime;
                                device_match->version = version();
                                device_match->is_compatible_id = true;
                                device_match->rank = dwRank + 0x2000 + (DWORD)index + (DWORD)(i > 0 ? 0x1000 + (i * 0x100) : 0x0000);
                                return device_match;
                            }
                        }
                    }
                }
            }
        }
    }
    return NULL;
}

setup_inf_file::setup_action::vtr setup_inf_file::get_setup_actions(match_device_info::ptr match_device, boost::filesystem::path win_dir, reg_edit_base& reg_edit, stdstring device_instance_id){
    setup_inf_file::setup_action::vtr actions;
    stdstring install_section;
    stdstring install_section_ext = _T(".nt") + match_device->architecture;
    if (_sections.count(match_device->install_section + install_section_ext)){
        install_section = match_device->install_section + install_section_ext;
    }
    else if (_sections.count(match_device->install_section + _T(".nt"))){
        install_section = match_device->install_section + _T(".nt");
    }
    else if (_sections.count(match_device->install_section)){
        install_section = match_device->install_section;
    }
    if (install_section.length()){       
        stdstring hkr_hw = (device_instance_id.length()) ? boost::str(boost::wformat(_T("SYSTEM\\CurrentControlSet\\Enum\\%s\\Device Parameters")) % device_instance_id) : _T("");
        stdstring hkr_sw;
        registry reg(reg_edit);
        if (reg.open(boost::str(boost::wformat(_T("SYSTEM\\CurrentControlSet\\Control\\Class\\%s")) % class_guid()))){
            reg.refresh_subkeys();
            LONG        increase_num = 0;
            bool        is_found = false;
            for (int i = 0; i < reg.subkeys_count(); i++){
                LONG num = _ttol(reg.subkey(i).key_name().c_str());
                if (increase_num < num)
                    increase_num = num;
                if (reg.subkey(i)[_T("MatchingDeviceId")].exists() &&
                    reg.subkey(i)[_T("MatchingDeviceId")].is_string() &&
#if _UNICODE
                    !_tcsicmp(reg.subkey(i)[_T("MatchingDeviceId")].wstring().c_str(), match_device->matched_id.c_str())){
#else
                    !_tcsicmp(reg.subkey(i)[_T("MatchingDeviceId")].string().c_str(), match_device->matched_id.c_str())){
#endif
                    is_found = true;
                    hkr_sw = reg.subkey(i).key_path();
                    break;
                }
            }
            if (!is_found &&  device_instance_id.length()){
                if (reg.subkeys_count())
                    ++increase_num;
#if _UNICODE
                hkr_sw = boost::str(boost::wformat(_T("SYSTEM\\CurrentControlSet\\Control\\Class\\%s\\%04i")) % class_guid() % increase_num);
#else
                hkr_sw = boost::str(boost::format(_T("SYSTEM\\CurrentControlSet\\Control\\Class\\%s\\%04i")) % class_guid() % increase_num);
#endif
            }
        }

        actions = device_driver_installs(install_section, win_dir, match_device->architecture, hkr_hw, hkr_sw);

        if (match_device->has_driver_store){

#if _UNICODE
            stdstring drv_inf = stringutils::tolower((stdstring)(boost::filesystem::path(_file_path).filename().wstring()));
#else
            stdstring drv_inf = stringutils::tolower((stdstring)(boost::filesystem::path(_file_path).filename().string()));
#endif      
            int oem_index = 0;
            registry reg(reg_edit, REGISTRY_READONLY);
            if (reg.open(_T("SYSTEM\\DriverDatabase\\DriverInfFiles"))){
                if (reg.refresh_subkeys()){
                    DWORD index = 0;
                    for (int i = 0; i < reg.subkeys_count(); i++){
                        if (1 == _stscanf_s(macho::stringutils::tolower(reg.subkey(i).key_name()).c_str(), _T("oem%d.inf"), &index)){
                            if (index > oem_index)
                                oem_index = index;
                        }
                    }
                }
                reg.close();
            }
            std::vector<boost::filesystem::path> oem_infs = macho::windows::environment::get_files(win_dir / _T("inf"), _T("oem*.inf"));
            foreach(boost::filesystem::path inf, oem_infs){
                DWORD index = 0;
#if _UNICODE
                if (1 == _stscanf_s(macho::stringutils::tolower((std::wstring)inf.filename().wstring()).c_str(), _T("oem%d.inf"), &index)){
#else
                if (1 == _stscanf_s(macho::stringutils::tolower((std::wstring)inf.filename().string()).c_str(), _T("oem%d.inf"), &index)){
#endif
                    if (index > oem_index)
                        oem_index = index;
                }
            }
            stdstring oem_inf = macho::stringutils::format(_T("oem%d.inf"), oem_index+1);
            FILETIME date = environment::ptime_to_file_time(this->_datetime);
#if _UNICODE
            stdstring drv_inf_label = stringutils::tolower(boost::str(boost::wformat(_T("%s_%s_%016llx"))%drv_inf %match_device->architecture % (version() * date.dwHighDateTime + date.dwLowDateTime)));
#else
            stdstring drv_inf_label = stringutils::tolower(boost::str(boost::format(_T("%s_%s_%016llx")) %drv_inf %match_device->architecture % (version() * date.dwHighDateTime + date.dwLowDateTime)));
#endif
            setup_inf_file::setup_action::vtr _actions;
  
            foreach(setup_inf_file::setup_action::ptr action, actions){
                setup_inf_file::setup_action_service* svc = dynamic_cast<setup_inf_file::setup_action_service*> (action.get());
                if (svc){
#if _UNICODE
                    std::wstring driver_packages_configurations = boost::str(boost::wformat(L"SYSTEM\\DriverDatabase\\DriverPackages\\%s\\Configurations\\%s") % drv_inf_label % install_section);
#else
                    std::string driver_packages_configurations = boost::str(boost::format("SYSTEM\\DriverDatabase\\DriverPackages\\%s\\Configurations\\%s") % drv_inf_label % install_section);
#endif
                    setup_action_registry::ptr driver_packages_configurations_action1 = setup_action_registry::ptr(new setup_action_registry());
                    driver_packages_configurations_action1->root = HKEY_LOCAL_MACHINE;
                    driver_packages_configurations_action1->root_key = _T("HKEY_LOCAL_MACHINE");
                    driver_packages_configurations_action1->key_path = driver_packages_configurations;
                    driver_packages_configurations_action1->value_name = _T("Service");
                    driver_packages_configurations_action1->value = svc->name;
                    driver_packages_configurations_action1->flags = FLG_ADDREG_TYPE_SZ;
                    _actions.push_back(driver_packages_configurations_action1);

                    setup_action_registry::ptr driver_packages_configurations_action2 = setup_action_registry::ptr(new setup_action_registry());
                    driver_packages_configurations_action2->root = HKEY_LOCAL_MACHINE;
                    driver_packages_configurations_action2->root_key = _T("HKEY_LOCAL_MACHINE");
                    driver_packages_configurations_action2->key_path = driver_packages_configurations;
                    driver_packages_configurations_action2->value_name = _T("ConfigFlags");
                    driver_packages_configurations_action2->value = _T("0x00000000");
                    driver_packages_configurations_action2->flags = FLG_ADDREG_TYPE_DWORD;
                    _actions.push_back(driver_packages_configurations_action2);

                    //setup_action_registry::ptr driver_packages_configurations_action3 = setup_action_registry::ptr(new setup_action_registry());
                    //driver_packages_configurations_action3->root = HKEY_LOCAL_MACHINE;
                    //driver_packages_configurations_action3->root_key = _T("HKEY_LOCAL_MACHINE");
                    //driver_packages_configurations_action3->key_path = driver_packages_configurations;
                    //driver_packages_configurations_action3->value_name = _T("ConfigScope");
                    //driver_packages_configurations_action3->value = _T("0x00000007");
                    //driver_packages_configurations_action3->flags = FLG_ADDREG_TYPE_DWORD;
                    //_actions.push_back(driver_packages_configurations_action3);

                }
                else{
                    setup_inf_file::setup_action_registry* reg = dynamic_cast<setup_inf_file::setup_action_registry*> (action.get());
                    if (reg){
                        stdstring services_key_path = _T("SYSTEM\\CurrentControlSet\\services");
                        if (reg->key_path.find(_T("SYSTEM\\CurrentControlSet\\services\\EventLog")) == std::wstring::npos && reg->key_path.find(services_key_path) != std::wstring::npos){
#if _UNICODE
                            std::wstring driver_packages_services = boost::str(boost::wformat(L"SYSTEM\\DriverDatabase\\DriverPackages\\%s\\Configurations\\%s\\Services") % drv_inf_label % install_section);
#else
                            std::string driver_packages_services = boost::str(boost::format("SYSTEM\\DriverDatabase\\DriverPackages\\%s\\Configurations\\%s\\Services") % drv_inf_label % install_section);
#endif
                            setup_action_registry::ptr reg_driver_packages_services_action = setup_action_registry::ptr(new setup_action_registry());
                            reg_driver_packages_services_action->root = HKEY_LOCAL_MACHINE;
                            reg_driver_packages_services_action->root_key = _T("HKEY_LOCAL_MACHINE");
                            reg_driver_packages_services_action->key_path = reg->key_path;
                            reg_driver_packages_services_action->key_path.replace(0, services_key_path.length(), driver_packages_services);
                            reg_driver_packages_services_action->value_name = reg->value_name;
                            reg_driver_packages_services_action->value = reg->value;
                            reg_driver_packages_services_action->flags = reg->flags;
                            _actions.push_back(reg_driver_packages_services_action);
                        }
                        else if (reg->root == 0 && reg->root_key == L"HKR"){
                            setup_action_registry::ptr reg_driver_packages_device_action = setup_action_registry::ptr(new setup_action_registry());
                            reg_driver_packages_device_action->root = HKEY_LOCAL_MACHINE;
                            reg_driver_packages_device_action->root_key = _T("HKEY_LOCAL_MACHINE");
#if _UNICODE
                            reg_driver_packages_device_action->key_path = boost::str(boost::wformat(L"SYSTEM\\DriverDatabase\\DriverPackages\\%s\\Configurations\\%s\\Device\\%s") % drv_inf_label % install_section %reg->key_path);
#else
                            reg_driver_packages_device_action->key_path = boost::str(boost::format("SYSTEM\\DriverDatabase\\DriverPackages\\%s\\Configurations\\%s\\Device\\%s") % drv_inf_label % install_section %reg->key_path);
#endif
                            reg_driver_packages_device_action->value_name = reg->value_name;
                            reg_driver_packages_device_action->value = reg->value;
                            reg_driver_packages_device_action->flags = reg->flags;
                            _actions.push_back(reg_driver_packages_device_action);
                        }
                    }
                }
            }
            actions.insert(actions.end(), _actions.begin(), _actions.end());
#if _UNICODE
            std::wstring descriptors_path = boost::str(boost::wformat(L"SYSTEM\\DriverDatabase\\DriverPackages\\%s\\Descriptors\\%s") % drv_inf_label % match_device->matched_id);
#else
            std::string descriptors_path = boost::str(boost::format("SYSTEM\\DriverDatabase\\DriverPackages\\%s\\Descriptors\\%s") % drv_inf_label % match_device->matched_id);
#endif
            setup_action_registry::ptr reg_driver_description_action1 = setup_action_registry::ptr(new setup_action_registry());
            reg_driver_description_action1->root = HKEY_LOCAL_MACHINE;
            reg_driver_description_action1->root_key = _T("HKEY_LOCAL_MACHINE");
            reg_driver_description_action1->key_path = descriptors_path;
            reg_driver_description_action1->value_name = _T("Configuration");
            reg_driver_description_action1->value = install_section;
            reg_driver_description_action1->flags = FLG_ADDREG_TYPE_SZ;
            actions.push_back(reg_driver_description_action1);

            typedef std::map< stdstring, stdstring, stringutils::no_case_string_less > _string_map;
            _string_map strings;
            if (!match_device->device_description.empty()){
                std::wstring value;
                foreach(_string_map::value_type s, _strings){
                    if (s.second == match_device->device_description){
                        value = stringutils::tolower(stdstring(_T("%")) + s.first + _T("%"));
                        strings[stringutils::tolower((stdstring)s.first)] = s.second;
                        break;
                    }
                }
                setup_action_registry::ptr reg_driver_description_action2 = setup_action_registry::ptr(new setup_action_registry());
                reg_driver_description_action2->root = HKEY_LOCAL_MACHINE;
                reg_driver_description_action2->root_key = _T("HKEY_LOCAL_MACHINE");
                reg_driver_description_action2->key_path = descriptors_path;
                reg_driver_description_action2->value_name = _T("Description");
                reg_driver_description_action2->value = value.empty() ? match_device->device_description : value;
                reg_driver_description_action2->flags = FLG_ADDREG_TYPE_SZ;
                actions.push_back(reg_driver_description_action2);
            }
            
            if (!match_device->manufacturer.empty()){
                std::wstring value;
                foreach(_string_map::value_type s, _strings){
                    if (s.second == match_device->manufacturer){
                        value = stringutils::tolower(stdstring(_T("%")) + s.first + _T("%"));
                        strings[stringutils::tolower((stdstring)s.first)] = s.second;
                        break;
                    }
                }
                setup_action_registry::ptr reg_driver_description_action3 = setup_action_registry::ptr(new setup_action_registry());
                reg_driver_description_action3->root = HKEY_LOCAL_MACHINE;
                reg_driver_description_action3->root_key = _T("HKEY_LOCAL_MACHINE");
                reg_driver_description_action3->key_path = descriptors_path;
                reg_driver_description_action3->value_name = _T("Manufacturer");
                reg_driver_description_action3->value = value.empty() ? match_device->manufacturer : value;
                reg_driver_description_action3->flags = FLG_ADDREG_TYPE_SZ;
                actions.push_back(reg_driver_description_action3);
            }

            foreach(_string_map::value_type s, strings){
                setup_action_registry::ptr reg_driver_strings_action = setup_action_registry::ptr(new setup_action_registry());
                reg_driver_strings_action->root = HKEY_LOCAL_MACHINE;
                reg_driver_strings_action->root_key = _T("HKEY_LOCAL_MACHINE");
#if _UNICODE
                reg_driver_strings_action->key_path = boost::str(boost::wformat(L"SYSTEM\\DriverDatabase\\DriverPackages\\%s\\Strings") % drv_inf_label);
#else
                reg_driver_strings_action->key_path = boost::str(boost::format("SYSTEM\\DriverDatabase\\DriverPackages\\%s\\Strings") % drv_inf_label);
#endif
                reg_driver_strings_action->value_name = s.first;
                reg_driver_strings_action->value = s.second;
                reg_driver_strings_action->flags = FLG_ADDREG_TYPE_SZ;
                actions.push_back(reg_driver_strings_action);
            }

            setup_action_registry::ptr reg_driver_inf_files_action1 = setup_action_registry::ptr(new setup_action_registry());
            reg_driver_inf_files_action1->root = HKEY_LOCAL_MACHINE;
            reg_driver_inf_files_action1->root_key = _T("HKEY_LOCAL_MACHINE");
#if _UNICODE
            reg_driver_inf_files_action1->key_path = std::wstring(L"SYSTEM\\DriverDatabase\\DriverInfFiles\\") + oem_inf;
#else
            reg_driver_inf_files_action1->key_path = std::string("SYSTEM\\DriverDatabase\\DriverInfFiles\\") + oem_inf;
#endif
            reg_driver_inf_files_action1->value_name = _T("");
            reg_driver_inf_files_action1->value = drv_inf_label;
            reg_driver_inf_files_action1->flags = FLG_ADDREG_TYPE_MULTI_SZ | FLG_ADDREG_APPEND;
            actions.push_back(reg_driver_inf_files_action1);
       
            setup_action_registry::ptr reg_driver_inf_files_action2 = setup_action_registry::ptr(new setup_action_registry());
            reg_driver_inf_files_action2->root = HKEY_LOCAL_MACHINE;
            reg_driver_inf_files_action2->root_key = _T("HKEY_LOCAL_MACHINE");
#if _UNICODE
            reg_driver_inf_files_action2->key_path = std::wstring(L"SYSTEM\\DriverDatabase\\DriverInfFiles\\") + oem_inf;
#else
            reg_driver_inf_files_action2->key_path = std::string("SYSTEM\\DriverDatabase\\DriverInfFiles\\") + oem_inf;
#endif
            reg_driver_inf_files_action2->value_name = _T("Active");
            reg_driver_inf_files_action2->value = drv_inf_label;
            reg_driver_inf_files_action2->flags = FLG_ADDREG_TYPE_SZ;
            actions.push_back(reg_driver_inf_files_action2);

            setup_action_registry::ptr reg_driver_inf_files_action3 = setup_action_registry::ptr(new setup_action_registry());
            reg_driver_inf_files_action3->root = HKEY_LOCAL_MACHINE;
            reg_driver_inf_files_action3->root_key = _T("HKEY_LOCAL_MACHINE");
#if _UNICODE
            reg_driver_inf_files_action3->key_path = std::wstring(L"SYSTEM\\DriverDatabase\\DriverInfFiles\\") + oem_inf;
#else
            reg_driver_inf_files_action3->key_path = std::string("SYSTEM\\DriverDatabase\\DriverInfFiles\\") + oem_inf;
#endif
            reg_driver_inf_files_action3->value_name = _T("Configurations");
            reg_driver_inf_files_action3->value = install_section;
            reg_driver_inf_files_action3->flags = FLG_ADDREG_TYPE_MULTI_SZ | FLG_ADDREG_APPEND;
            actions.push_back(reg_driver_inf_files_action3);

            setup_action_registry::ptr reg_device_ids_action = setup_action_registry::ptr(new setup_action_registry());
            reg_device_ids_action->root = HKEY_LOCAL_MACHINE;
            reg_device_ids_action->root_key = _T("HKEY_LOCAL_MACHINE");
#if _UNICODE
            reg_device_ids_action->key_path = std::wstring(L"SYSTEM\\DriverDatabase\\DeviceIds\\") + match_device->matched_id;
#else
            reg_device_ids_action->key_path = std::string("SYSTEM\\DriverDatabase\\DeviceIds\\") + match_device->matched_id;
#endif
            reg_device_ids_action->value_name = oem_inf;
            reg_device_ids_action->value = _T("01,ff,00,00");
            reg_device_ids_action->flags = FLG_ADDREG_BINVALUETYPE;
            actions.push_back(reg_device_ids_action);

            setup_action_registry::ptr reg_device_ids_action2 = setup_action_registry::ptr(new setup_action_registry());
            reg_device_ids_action2->root = HKEY_LOCAL_MACHINE;
            reg_device_ids_action2->root_key = _T("HKEY_LOCAL_MACHINE");
            reg_device_ids_action2->key_path = stdstring(_T("SYSTEM\\DriverDatabase\\DeviceIds\\")) + this->class_guid();
            reg_device_ids_action2->value_name = oem_inf;
            reg_device_ids_action2->flags = FLG_ADDREG_TYPE_NONE;
            actions.push_back(reg_device_ids_action2);

#if _UNICODE
            stdstring reg_driver_packages_path = boost::str(boost::wformat(L"SYSTEM\\DriverDatabase\\DriverPackages\\%s") % drv_inf_label);
#else
            stdstring reg_driver_packages_path = boost::str(boost::format("SYSTEM\\DriverDatabase\\DriverPackages\\%s") % drv_inf_label);
#endif
            setup_action_registry::ptr reg_driver_packages_action1 = setup_action_registry::ptr(new setup_action_registry());
            reg_driver_packages_action1->root = HKEY_LOCAL_MACHINE;
            reg_driver_packages_action1->root_key = _T("HKEY_LOCAL_MACHINE");
            reg_driver_packages_action1->key_path = reg_driver_packages_path;
            reg_driver_packages_action1->value_name = _T("");
            reg_driver_packages_action1->value = oem_inf;
            reg_driver_packages_action1->flags = FLG_ADDREG_TYPE_SZ;
            actions.push_back(reg_driver_packages_action1);
            
            setup_action_registry::ptr reg_driver_packages_action2 = setup_action_registry::ptr(new setup_action_registry());
            reg_driver_packages_action2->root = HKEY_LOCAL_MACHINE;
            reg_driver_packages_action2->root_key = _T("HKEY_LOCAL_MACHINE");
            reg_driver_packages_action2->key_path = reg_driver_packages_path;
            reg_driver_packages_action2->value_name = _T("InfName");
            reg_driver_packages_action2->value = drv_inf;
            reg_driver_packages_action2->flags = FLG_ADDREG_TYPE_SZ;
            actions.push_back(reg_driver_packages_action2);

            setup_action_registry::ptr reg_driver_packages_action3 = setup_action_registry::ptr(new setup_action_registry());
            reg_driver_packages_action3->root = HKEY_LOCAL_MACHINE;
            reg_driver_packages_action3->root_key = _T("HKEY_LOCAL_MACHINE");
            reg_driver_packages_action3->key_path = reg_driver_packages_path;
            reg_driver_packages_action3->value_name = _T("Provider");
            reg_driver_packages_action3->value = this->provider();
            reg_driver_packages_action3->flags = FLG_ADDREG_TYPE_SZ;
            actions.push_back(reg_driver_packages_action3);

            if (match_device->is_driver_signed){
                setup_action_registry::ptr reg_driver_packages_action4 = setup_action_registry::ptr(new setup_action_registry());
                reg_driver_packages_action4->root = HKEY_LOCAL_MACHINE;
                reg_driver_packages_action4->root_key = _T("HKEY_LOCAL_MACHINE");
                reg_driver_packages_action4->key_path = reg_driver_packages_path;
                reg_driver_packages_action4->value_name = _T("SignerName");
                reg_driver_packages_action4->value = match_device->signer_name;
                reg_driver_packages_action4->flags = FLG_ADDREG_TYPE_SZ;
                actions.push_back(reg_driver_packages_action4);

                setup_action_registry::ptr reg_driver_packages_action5 = setup_action_registry::ptr(new setup_action_registry());
                reg_driver_packages_action5->root = HKEY_LOCAL_MACHINE;
                reg_driver_packages_action5->root_key = _T("HKEY_LOCAL_MACHINE");
                reg_driver_packages_action5->key_path = reg_driver_packages_path;
                reg_driver_packages_action5->value_name = _T("SignerScore");
                reg_driver_packages_action5->value = macho::stringutils::format(_T("%d"), match_device->signer_score);
                reg_driver_packages_action5->flags = FLG_ADDREG_TYPE_DWORD;
                actions.push_back(reg_driver_packages_action5);
            }
            DRIVER_VERSION version;
            memset(&version, 0, sizeof(DRIVER_VERSION));
            version.Flag = 0x09ff00;
            version.Class = macho::guid_(this->class_guid());
            version.Version = this->version();
            version.Date = environment::ptime_to_file_time(this->_datetime);
            macho::bytes v((LPBYTE)&version, sizeof(version));
            setup_action_registry::ptr reg_driver_packages_action6 = setup_action_registry::ptr(new setup_action_registry());
            reg_driver_packages_action6->root = HKEY_LOCAL_MACHINE;
            reg_driver_packages_action6->root_key = _T("HKEY_LOCAL_MACHINE");
            reg_driver_packages_action6->key_path = reg_driver_packages_path;
            reg_driver_packages_action6->value_name = _T("Version");
            reg_driver_packages_action6->value = v.get();
            reg_driver_packages_action6->flags = FLG_ADDREG_TYPE_BINARY;
            actions.push_back(reg_driver_packages_action6);
        }
        else{
            foreach(setup_inf_file::setup_action::ptr action, actions){
                setup_inf_file::setup_action_service* svc = dynamic_cast<setup_inf_file::setup_action_service*> (action.get());
                if (svc && svc->flags & SPSVCINST_ASSOCSERVICE){
                    stdstring pnp_id = match_device->matched_id;
                    while (pnp_id.find(_T("\\")) != stdstring::npos)
                        pnp_id.replace(pnp_id.find(_T("\\")), 1, _T("#"));
#if _UNICODE
                    stdstring device_pnp_path = boost::str(boost::wformat(_T("SYSTEM\\CurrentControlSet\\Control\\CriticalDeviceDatabase\\%s")) % pnp_id);
#else
                    stdstring device_pnp_path = boost::str(boost::format(_T("SYSTEM\\CurrentControlSet\\Control\\CriticalDeviceDatabase\\%s")) % pnp_id);
#endif               
                    setup_action_registry::ptr reg_action1 = setup_action_registry::ptr(new setup_action_registry());
                    reg_action1->root = HKEY_LOCAL_MACHINE;
                    reg_action1->root_key = _T("HKEY_LOCAL_MACHINE");
                    reg_action1->key_path = device_pnp_path;
                    reg_action1->value_name = _T("Service");
                    reg_action1->value = svc->name;
                    actions.push_back(reg_action1);

                    setup_action_registry::ptr reg_action2 = setup_action_registry::ptr(new setup_action_registry());
                    reg_action2->root = HKEY_LOCAL_MACHINE;
                    reg_action2->root_key = _T("HKEY_LOCAL_MACHINE");
                    reg_action2->key_path = device_pnp_path;
                    reg_action2->value_name = _T("ClassGUID");
                    reg_action2->value = svc->class_guid;
                    actions.push_back(reg_action2);

                    break;
                }
            }
        }
    }
    return actions;
}

void setup_inf_file::earse_unused_oem_driver_packages(boost::filesystem::path win_dir, reg_edit_base& reg_edit){
    registry reg(reg_edit);
    if (reg.open(_T("SYSTEM\\DriverDatabase\\DriverInfFiles"))){
        if (reg.refresh_subkeys()){
            DWORD index = 0;
            for (int i = 0; i < reg.subkeys_count(); i++){
                if ((1 == _stscanf_s(macho::stringutils::tolower(reg.subkey(i).key_name()).c_str(), _T("oem%d.inf"), &index)) &&
                    (!boost::filesystem::exists(win_dir / _T("INF") / reg.subkey(i).key_name()))){
                    remove_subkey_value(reg_edit, _T("SYSTEM\\DriverDatabase\\DeviceIds"), reg.subkey(i).key_name());
                    registry reg_drv(reg_edit);
                    if (reg.subkey(i)[_T("Active")].exists() && reg.subkey(i)[_T("Active")].is_string()){
#if _UNICODE
                        if (reg_drv.open(std::wstring(_T("SYSTEM\\DriverDatabase\\DriverPackages\\") + reg.subkey(i)[_T("Active")].wstring()))){
#else
                        if (reg_drv.open(std::string(_T("SYSTEM\\DriverDatabase\\DriverPackages\\") + reg.subkey(i)[_T("Active")].string()))){
#endif  
                            if (reg_drv[_T("")].exists() &&
                                reg_drv[_T("")].is_string() &&
#if _UNICODE
                                reg_drv[_T("")].wstring() == reg.subkey(i).key_name()){
#else
                                reg_drv[_T("")].string() == reg.subkey(i).key_name()){
#endif
                                    reg_drv.delete_key();
                            }
                        }
                    }
                    else{
                        if (reg_drv.open(_T("SYSTEM\\DriverDatabase\\DriverPackages"))){
                            if (reg_drv.refresh_subkeys()){
                                for (int j = 0; j < reg_drv.subkeys_count(); j++){
                                    if (reg_drv.subkey(j)[_T("")].exists() &&
                                        reg_drv.subkey(j)[_T("")].is_string() &&
#if _UNICODE
                                        reg_drv.subkey(j)[_T("")].wstring() == reg.subkey(i).key_name()){
#else
                                        reg_drv.subkey(j)[_T("")].string() == reg.subkey(i).key_name()){
#endif
                                            reg_drv.subkey(j).delete_key();
                                    }
                                }
                            }
                        }
                    }
                    reg.subkey(i).delete_key();
                }
            }
        }
        reg.close();
    }
}

void setup_inf_file::remove_subkey_value(reg_edit_base& reg_edit, stdstring key_path, stdstring value_name){
    registry reg(reg_edit);
    if (reg.open(key_path)){
        reg[value_name].delete_value();
        if (reg.refresh_subkeys()){
            for (int i = 0; i < reg.subkeys_count(); i++){
                remove_subkey_value(reg_edit, reg.subkey(i).key_path(), value_name);
            }
        }
    }
}

setup_inf_file::setup_action::vtr setup_inf_file::device_driver_installs(stdstring install_section, boost::filesystem::path& win_dir, stdstring architecture, stdstring hw_hkr, stdstring sw_hkr){
    setup_inf_file::setup_action::vtr actions;
    setup_inf_file::setup_action::vtr _actions;
    _actions = device_driver_install(install_section, win_dir, architecture, sw_hkr);
    actions.insert(actions.end(), _actions.begin(), _actions.end());
    _actions = device_driver_install(install_section + _T(".Services"), win_dir, architecture);
    actions.insert(actions.end(), _actions.begin(), _actions.end());
    _actions = device_driver_install(install_section + _T(".HW"), win_dir, architecture, hw_hkr);
    actions.insert(actions.end(), _actions.begin(), _actions.end());
    _actions = device_driver_install(install_section + _T(".CoInstallers"), win_dir, architecture, sw_hkr);
    actions.insert(actions.end(), _actions.begin(), _actions.end());
    _actions = device_driver_install(install_section + _T(".LogConfigOverride"), win_dir, architecture);
    actions.insert(actions.end(), _actions.begin(), _actions.end());
    _actions = device_driver_install(install_section + _T(".Interfaces"), win_dir, architecture);
    actions.insert(actions.end(), _actions.begin(), _actions.end());
    _actions = device_driver_install(install_section + _T(".WMI"), win_dir, architecture);
    actions.insert(actions.end(), _actions.begin(), _actions.end());
    _actions = device_driver_install(install_section + _T(".FactDef"), win_dir, architecture);
    actions.insert(actions.end(), _actions.begin(), _actions.end());
    return actions;
}

setup_inf_file::setup_action::vtr setup_inf_file::device_driver_install(stdstring install_section, const boost::filesystem::path& win_dir, stdstring architecture, stdstring hkr){
    setup_inf_file::setup_action::vtr actions;
    setup_inf_file::setup_action::vtr _actions;
    setup_inf_file::vtr               _includes;
    if (_sections.count(install_section)){
        foreach(stdstring &line, _sections[install_section]){
            string_array keyvalue = stringutils::tokenize2(line, _T("="), 2);
            string_array values = stringutils::tokenize2(keyvalue[1], _T(","));

            if (0 == _tcsicmp(keyvalue[0].c_str(), _T("CopyFiles"))){
                _actions = copy_files(values, win_dir, architecture);
                actions.insert(actions.end(), _actions.begin(), _actions.end());
            }
            else if (0 == _tcsicmp(keyvalue[0].c_str(), _T("AddReg"))){
                _actions = reg_edit(values, hkr, false);
                actions.insert(actions.end(), _actions.begin(), _actions.end());
            }
            else if (0 == _tcsicmp(keyvalue[0].c_str(), _T("DelReg"))){
                _actions = reg_edit(values, hkr, true);
                actions.insert(actions.end(), _actions.begin(), _actions.end());
            }
            else if (0 == _tcsicmp(keyvalue[0].c_str(), _T("Include"))){
                _includes = include(values, win_dir);
            }
            else if (0 == _tcsicmp(keyvalue[0].c_str(), _T("Needs"))){
                _actions = needs(_includes, values, win_dir, architecture);
                actions.insert(actions.end(), _actions.begin(), _actions.end());
            }
            // Service Section
            else if (0 == _tcsicmp(keyvalue[0].c_str(), _T("AddService"))){
                _actions = add_service(values);
                actions.insert(actions.end(), _actions.begin(), _actions.end());
            }
        }
    }
    return actions;
}

setup_inf_file::vtr setup_inf_file::include(string_array& values, const boost::filesystem::path& win_dir){
    setup_inf_file::vtr inf_files;
    foreach(stdstring &value, values){
        std::vector<boost::filesystem::path> _files = environment::get_files( win_dir/_T("System32\\DriverStore\\FileRepository"), value, true);
        if (!_files.size()){
            _files = environment::get_files(win_dir / _T("inf"), value);
        }
        if (_files.size()){
            setup_inf_file::ptr inf = setup_inf_file::ptr(new setup_inf_file());
            if (inf->load(_files[0].wstring()))
                inf_files.push_back(inf);
        }
    }
    return inf_files;
}

setup_inf_file::setup_action::vtr setup_inf_file::needs(setup_inf_file::vtr& infs, string_array& values, const boost::filesystem::path& win_dir, stdstring architecture){
    setup_inf_file::setup_action::vtr actions;
    foreach(setup_inf_file::ptr &inf, infs){
        foreach(stdstring &value, values){
            if (inf->_sections.count(value)){
                setup_inf_file::setup_action::vtr _actions = inf->device_driver_install(value, win_dir, architecture);
                actions.insert(actions.end(), _actions.begin(), _actions.end());
            }
        }
    }
    return actions;
}

setup_inf_file::setup_action::vtr setup_inf_file::add_service(string_array& values){
    setup_inf_file::setup_action::vtr actions;
    stdstring                       service_name;
    DWORD                           dwFlags;
    stdstring                       service_install_section, event_log_install_section, event_log_type, event_name;
    string_array                    add_services_elements = expand_strings(strings(), values);
    TCHAR *                         pSzStop;
    event_log_type = _T("System");
    switch (add_services_elements.size()){
    case 6:
        event_name = add_services_elements[5];
    case 5:
        event_log_type = add_services_elements[4];
    case 4:
        event_log_install_section = add_services_elements[3];
    case 3:
        service_install_section = add_services_elements[2];
    case 2:
        dwFlags = add_services_elements[1].c_str() ? _tcstoul(add_services_elements[1].c_str(), &pSzStop, 0) : 0;
    case 1:
        service_name = add_services_elements[0];
    }
    if (!event_name.length()) event_name = service_name;

    setup_action_service::ptr add_service_action = setup_action_service::ptr(new setup_action_service());
    add_service_action->name = service_name;
    add_service_action->flags = dwFlags;
    add_service_action->class_guid = stringutils::toupper(class_guid());
    actions.push_back(add_service_action);
#if _UNICODE
    stdstring service_path = boost::str(boost::wformat(L"SYSTEM\\CurrentControlSet\\services\\%s") % service_name);
#else
    stdstring service_path = boost::str(boost::format("SYSTEM\\CurrentControlSet\\services\\%s") % service_name);
#endif
    foreach(stdstring &entry, _sections[service_install_section]){
        string_array key_value      = stringutils::tokenize2(entry, _T("="));
        string_array service_values = stringutils::tokenize2(key_value[1], _T(","));
        string_array service_elements = expand_strings(strings(), service_values);
        if (0 == _tcsicmp(key_value[0].c_str(), _T("DelReg"))){
            setup_inf_file::setup_action::vtr _actions = reg_edit(service_values, service_path, true);
            actions.insert(actions.end(), _actions.begin(), _actions.end());
        }
        else if (0 == _tcsicmp(key_value[0].c_str(), _T("AddReg"))){
            setup_inf_file::setup_action::vtr _actions = reg_edit(service_values, service_path, false);
            actions.insert(actions.end(), _actions.begin(), _actions.end());
        }
        else if (0 == _tcsicmp(key_value[0].c_str(), _T("BitReg"))){
        }
        else
        {
            if (0 == _tcsicmp(key_value[0].c_str(), _T("StartType")))
                add_service_action->start = (DWORD)_tcstoul(service_elements[0].c_str(), &pSzStop, 0);

            setup_action_service_entry::ptr entry_ptr = setup_action_service_entry::ptr(new setup_action_service_entry());
            entry_ptr->entry_name = key_value[0];
            entry_ptr->values = service_elements;
            entry_ptr->flags = dwFlags;
            entry_ptr->service_path = service_path;
            add_service_action->entries.push_back(entry_ptr);
        }
    }
#if _UNICODE
    stdstring event_log_path = boost::str(boost::wformat(L"SYSTEM\\CurrentControlSet\\services\\EventLog\\%s\\%s") % event_log_type % event_name);
#else
    stdstring event_log_path = boost::str(boost::format("SYSTEM\\CurrentControlSet\\services\\EventLog\\%s\\%s") % event_log_type % event_name);
#endif
    if (_sections.count(event_log_install_section)){
        foreach(stdstring &entry, _sections[event_log_install_section]){
            string_array key_value = stringutils::tokenize2(entry, _T("="));
            string_array event_values = stringutils::tokenize2(key_value[1], _T(","));       
            if (0 == _tcsicmp(key_value[0].c_str(), _T("DelReg"))){
                setup_inf_file::setup_action::vtr _actions = reg_edit(event_values, event_log_path, true);
                actions.insert(actions.end(), _actions.begin(), _actions.end());
            }
            else if (0 == _tcsicmp(key_value[0].c_str(), _T("AddReg"))){
                setup_inf_file::setup_action::vtr _actions = reg_edit(event_values, event_log_path, false);
                actions.insert(actions.end(), _actions.begin(), _actions.end());
            }
            else if (0 == _tcsicmp(key_value[0].c_str(), _T("BitReg"))){
            }
        }
    }
    return actions;
}

setup_inf_file::setup_action::vtr setup_inf_file::reg_edit(string_array& values, stdstring hkr_path, bool is_remove){

    static std::map<stdstring, HKEY, stringutils::no_case_string_less> reg_keys_map = {
        { _T("HKEY_CLASSES_ROOT"), HKEY_CLASSES_ROOT },
        { _T("HKEY_CURRENT_CONFIG"), HKEY_CURRENT_CONFIG },
        { _T("HKEY_CURRENT_USER"), HKEY_CURRENT_USER },
        { _T("HKCU"), HKEY_CURRENT_USER },
        { _T("HKEY_LOCAL_MACHINE"), HKEY_LOCAL_MACHINE },
        { _T("HKLM"), HKEY_LOCAL_MACHINE },
        { _T("HKEY_USERS"), HKEY_USERS },
        { _T("HKU"), HKEY_USERS }
        };

    setup_inf_file::setup_action::vtr actions;
    foreach(stdstring &value, values){
        foreach(stdstring &line, _sections[value]){
            string_array reg_elements = expand_strings(strings(), stringutils::tokenize2(line, _T(","), 5));
            setup_action_registry::ptr reg_action = setup_action_registry::ptr(new setup_action_registry());
            stdstring reg_root, sub_key;
            TCHAR*    pSzStop;
            switch (reg_elements.size()){
            case 5:
                reg_action->value = reg_elements[4];
            case 4:
                reg_action->flags = _tcstoul(reg_elements[3].c_str(), &pSzStop, 0);
            case 3:
                reg_action->value_name = reg_elements[2];
            case 2:
                sub_key = reg_elements[1];
            case 1:
                reg_root = reg_elements[0];
                break;
            default:
                ;
            }
            if ((0 == _tcsicmp(reg_root.c_str(), _T("HKR"))) && hkr_path.length()){
                reg_root = _T("HKEY_LOCAL_MACHINE");
                sub_key = boost::str(boost::wformat(_T("%s\\%s")) % hkr_path% sub_key);
            }

            if (reg_keys_map.count(reg_root)) 
                reg_action->root = reg_keys_map[reg_root];
            
            reg_action->root_key = reg_root;
            reg_action->key_path = sub_key;
            reg_action->is_removed = is_remove;
            actions.push_back(reg_action);
        }
    }
    return actions;
}

setup_inf_file::setup_action::vtr setup_inf_file::copy_files(string_array& values, const boost::filesystem::path& win_dir, stdstring architecture){

    static std::map<int, stdstring> win_dir_id_map = {
        { 10, _T("") },
        { 11, _T("System32") },
        { 12, _T("System32\\Drivers") },
        { 17, _T("INF") },
        { 18, _T("Help") },
        { 20, _T("Fonts") },
        { 50, _T("System") },
    };

    setup_inf_file::source_disk::map source_disks_map = get_source_disks(architecture);
    setup_inf_file::source_disk_file::map source_disk_files_map = get_source_disk_files(architecture);
    setup_inf_file::destination_directory::map destination_dirs_map = get_destination_directories();
    setup_inf_file::destination_directory default_dest = destination_dirs_map[_T("DefaultDestDir")];

    setup_inf_file::setup_action::vtr actions;
    foreach(stdstring &file_section, values){
        setup_inf_file::setup_action_file::ptr _file_action = setup_inf_file::setup_action_file::ptr(new setup_inf_file::setup_action_file());
        _file_action->action = setup_action_file::FILE_COPY;
        if (file_section.at(0) == _T('@')){
            _file_action->source_file_name = _file_action->target_file_name = file_section.substr(1, file_section.length() - 1);
            if (destination_dirs_map.count(_T("DefaultDestDir"))){
                _file_action->target_path = win_dir / win_dir_id_map[destination_dirs_map[_T("DefaultDestDir")].dir_id] / destination_dirs_map[_T("DefaultDestDir")].sub_dir;

                if (!source_disk_files_map.count(_file_action->source_file_name)){
                    _file_action->source_path = win_dir;
                    _file_action->is_system_driver = true;
                }
                else{
                    if (source_disks_map.count(source_disk_files_map[_file_action->source_file_name].disk_id)){
                        source_disk source = source_disks_map[source_disk_files_map[_file_action->source_file_name].disk_id];
                        if (source.is_cab_file){
                            _file_action->is_in_cab = true;
                            _file_action->source_path = boost::filesystem::path(_file_path).parent_path() / source.tag_or_cab_file;
                        }
                        else{
                            _file_action->source_path = boost::filesystem::path(_file_path).parent_path() / source.path / source_disk_files_map[_file_action->source_file_name].sub_dir;
#if _UNICODE
                            _file_action->sub_dir = boost::filesystem::path(boost::filesystem::path(source.path) / source_disk_files_map[_file_action->source_file_name].sub_dir).wstring();
#else
                            _file_action->sub_dir = boost::filesystem::path(boost::filesystem::path(source.path) / source_disk_files_map[_file_action->source_file_name].sub_dir).string();
#endif
                        }
                    }
                }

                actions.push_back(_file_action);
            }
        }
        else{
            setup_inf_file::destination_directory destination_dir;
            if (!destination_dirs_map.count(file_section))
                destination_dir = default_dest;
            else
                destination_dir = destination_dirs_map[file_section];

            foreach(stdstring &line, _sections[file_section]){
                string_array copy_files = stringutils::tokenize2(line, _T(","), 5);

                switch (copy_files.size()){
                case 5:
                    // security-descriptor-string;
                case 4:
                    _file_action->flags = _tcstol(copy_files[3].c_str(), NULL, 0);
                case 3:
                case 2:
                    _file_action->source_file_name = copy_files[1];
                case 1:
                    _file_action->target_file_name = copy_files[0];
                }
                if (!_file_action->source_file_name.length())
                    _file_action->source_file_name = _file_action->target_file_name;
                _file_action->target_path = win_dir / win_dir_id_map[destination_dir.dir_id] / destination_dir.sub_dir;

                if (source_disks_map.count(source_disk_files_map[_file_action->source_file_name].disk_id)){
                    source_disk source = source_disks_map[source_disk_files_map[_file_action->source_file_name].disk_id];
                    if (source.is_cab_file){
                        _file_action->is_in_cab = true;
                        _file_action->source_path = boost::filesystem::path(_file_path).parent_path() / source.tag_or_cab_file;
                    }
                    else{
                        _file_action->source_path = boost::filesystem::path(_file_path).parent_path() / source.path / source_disk_files_map[_file_action->source_file_name].sub_dir;
#if _UNICODE
                        _file_action->sub_dir = boost::filesystem::path(boost::filesystem::path(source.path) / source_disk_files_map[_file_action->source_file_name].sub_dir).wstring();
#else
                        _file_action->sub_dir = boost::filesystem::path(boost::filesystem::path(source.path) / source_disk_files_map[_file_action->source_file_name].sub_dir).string();
#endif
                    }
                }
                actions.push_back(_file_action);
            }
        }
    }
    return actions;
}

bool setup_inf_file::setup_action_file::is_copy_replace_only() { 
    return (flags & COPYFLG_REPLACEONLY) ? true : false; 
}

bool setup_inf_file::setup_action_file::is_copy_no_overwrite() { 
    return (flags & COPYFLG_NO_OVERWRITE) ? true : false; 
}

bool setup_inf_file::setup_action_file::need_overwrite(const boost::filesystem::path source, const boost::filesystem::path target){
    bool result = false;
    if (!is_execution_file(target)){
        result = true;
    }
    else if (!boost::filesystem::exists(target)){
        result = true;
    }
    else{
        file_version_info source_file = file_version_info::get_file_version_info(source.wstring());
        file_version_info target_file = file_version_info::get_file_version_info(target.wstring());
        if ((source_file.product_version() > target_file.product_version()) ||
            ((source_file.product_version() == target_file.product_version()) && (source_file.file_version() > target_file.file_version())))
            result = true;
    }
    return result;
}

bool setup_inf_file::setup_action_file::is_execution_file(const boost::filesystem::path file){
    std::wstring extension = file.extension().wstring();
    if( extension.length() && (!_wcsicmp(extension.c_str(), L"exe") ||
        !_wcsicmp(extension.c_str(), L"dll") ||
        !_wcsicmp(extension.c_str(), L"sys") ||
        !_wcsicmp(extension.c_str(), L"ocx"))
        )
        return true;
    else
        return false;
}

bool setup_inf_file::setup_action_file::install(reg_edit_base& reg_edit){ 
    switch (action){
        case FILE_COPY:  {
            DWORD dwAttrs = 0;
            boost::filesystem::path target = target_path / target_file_name;
            bool target_exists = boost::filesystem::exists(target);
            boost::filesystem::path source = source_path / source_file_name;
            boost::filesystem::path temp;
            if (is_copy_replace_only() && !target_exists){}
            else if (is_copy_no_overwrite() && target_exists){}
            else{
                if (target_exists){
                    dwAttrs = GetFileAttributesW(target.wstring().c_str());
                    SetFileAttributesW(target.wstring().c_str(), FILE_ATTRIBUTE_NORMAL);
                }
                if (is_system_driver || is_in_cab){
                    source.clear();
                    temp = environment::create_temp_folder(); 
                    if (is_system_driver){
                        //copy the system_driver to 
                        LOG(LOG_LEVEL_INFO, _T("Need to extract the system driver file (%s)."), source.wstring().c_str());
                    }
                    else if (is_in_cab){
                        // extract the driver file from cab file.
                        LOG(LOG_LEVEL_INFO, _T("Need to extract driver file (%s) in cab file(%s) to (%s)."), source_file_name.c_str(), source_path.wstring().c_str());
                    }
                }
                if (!source.empty() && !target.empty()){
                    LOG(LOG_LEVEL_DEBUG, _T("Copy file from (%s) to (%s)."), source.wstring().c_str(), target.wstring().c_str());
                    if (!need_overwrite(source, target)){
                        LOG(LOG_LEVEL_INFO, _T("Skip to copy file from (%s) to (%s)."), source.wstring().c_str(), target.wstring().c_str());
                    }
                    else{
                        try{
                            boost::filesystem::copy_file(source, target, boost::filesystem::copy_option::overwrite_if_exists);
                            LOG(LOG_LEVEL_INFO, _T("Succeeded to copy file from (%s) to (%s)."), source.wstring().c_str(), target.wstring().c_str());
                        }
                        catch (const boost::filesystem::filesystem_error& e){
                            if (e.code() == boost::system::errc::permission_denied){
                                LOG(LOG_LEVEL_ERROR, _T("Permission is denied to copy file from (%s) to (%s)."), source.wstring().c_str(), target.wstring().c_str());
                            }
                            else{
                                LOG(LOG_LEVEL_ERROR, _T("Failed to copy file from (%s) to (%s) with %s."), source.wstring().c_str(), target.wstring().c_str(), stringutils::convert_ansi_to_unicode(e.code().message()).c_str());
                            }
                        }
                    }
                }
                if ( !temp.empty())
                    boost::filesystem::remove_all(temp);
                if (dwAttrs)
                    SetFileAttributesW(target.wstring().c_str(), dwAttrs);
            }
            break;
        }
    }
    return true; 
}

bool setup_inf_file::setup_action_file::exists(reg_edit_base& reg_edit){
    boost::filesystem::path target = target_path / target_file_name;
    return boost::filesystem::exists(target);
}

bool setup_inf_file::setup_action_file::remove(reg_edit_base& reg_edit){
    if (exists(reg_edit)){
        boost::filesystem::path target = target_path / target_file_name;
        SetFileAttributesW(target.wstring().c_str(), FILE_ATTRIBUTE_NORMAL);
        return boost::filesystem::remove(target);
    }
    return true;
}

bool setup_inf_file::setup_action_registry::install(reg_edit_base& reg_edit){
    TCHAR*          pSzStop;
    REGISTRY_FLAGS_ENUM reg_flags = (is_removed || (flags & FLG_ADDREG_OVERWRITEONLY) || (flags & FLG_ADDREG_DELVAL)) ? REGISTRY_FLAGS_ENUM::REGISTRY_NONE : REGISTRY_FLAGS_ENUM::REGISTRY_CREATE;
    registry reg(reg_edit, reg_flags);
    if (is_valid() && reg.open(root, key_path)){

        LOG(LOG_LEVEL_INFO, _T("%s Key (%s\\%s): Value Name (%s), Value(%s)."),
            is_removed ? _T("Remove") : _T("Add"),
            root_key.c_str(),
            key_path.c_str(),
            value_name.c_str(),
            value.c_str());

        if (is_removed){
            //case FLG_DELREG_32BITKEY:
            if (flags & FLG_DELREG_KEYONLY_COMMON)
                reg.delete_key();
            else if (flags & FLG_DELREG_MULTI_SZ_DELSTRING && reg[value_name].is_multi_sz())
                reg[value_name].remove_multi(value);
        }
        else{
            if ((flags & FLG_ADDREG_KEYONLY_COMMON) || (flags & FLG_ADDREG_KEYONLY)){
            }
            else if (flags & FLG_ADDREG_DELVAL){
                if (value_name.length())
                    reg[value_name].delete_value();
                else
                    reg.delete_key();
            }
            else if ((flags & FLG_ADDREG_OVERWRITEONLY) && !reg[value_name].exists()){
            }
            else if ((flags & FLG_ADDREG_NOCLOBBER) && reg[value_name].exists()){
            }
            else{
                //case FLG_ADDREG_64BITKEY:
                //case FLG_ADDREG_32BITKEY:

                switch (flags & FLG_ADDREG_TYPE_MASK) {
                case FLG_ADDREG_TYPE_SZ:
                    reg[value_name] = value;
                    break;
                case FLG_ADDREG_TYPE_MULTI_SZ:
                    if (flags & FLG_ADDREG_APPEND)
                        reg[value_name].remove_multi(value);
                    else
                        reg[value_name].clear_multi();
                    reg[value_name].add_multi(value);
                    break;
                case FLG_ADDREG_TYPE_EXPAND_SZ:
                    reg[value_name].set_expand_sz(value.c_str());
                    break;
                case FLG_ADDREG_TYPE_DWORD:
                    reg[value_name] = (DWORD)_tcstoul(value.c_str(), &pSzStop, 0);
                    break;
                case FLG_ADDREG_TYPE_NONE:
                    reg[value_name].set_value(REG_NONE, NULL, 0);
                    break;
                case FLG_ADDREG_BINVALUETYPE:
                default:
                    bytes b(value, _T(","));
                    reg[value_name].set_binary(b.ptr(), b.length());
                    break;
                }
            }
        }        
    }
    else if (is_valid() && !(is_removed || (flags & FLG_ADDREG_OVERWRITEONLY) || (flags & FLG_ADDREG_DELVAL))){
        LOG(LOG_LEVEL_ERROR, _T("Open/Create registry Path (%s\\%s) failed (%d)."),
            root_key.c_str(),
            key_path.c_str(),
            GetLastError());
        return false;
    }
    return true;
}

bool setup_inf_file::setup_action_registry::exists(reg_edit_base& reg_edit){
    registry reg(reg_edit);
    if (is_valid() && reg.open(root, key_path)){
        if (value_name.length())
            return reg[value_name].exists();
        else
            return true;
    }
    return false;
}

bool setup_inf_file::setup_action_registry::remove(reg_edit_base& reg_edit){
    registry reg(reg_edit);
    if (is_valid() && !is_removed && reg.open(root, key_path) && reg[value_name].exists())
        return reg[value_name].delete_value();
    return true;
}

bool setup_inf_file::setup_action_service_entry::install(reg_edit_base& reg_edit){
    TCHAR*          pSzStop;
    registry reg(reg_edit, REGISTRY_CREATE);
    stdstring value = values[0];
    if (reg.open(service_path)){
        if (0 == _tcsicmp(entry_name.c_str(), _T("DisplayName")))
            (flags & SPSVCINST_NOCLOBBER_DISPLAYNAME && reg[_T("DisplayName")].exists()) ? __noop : reg[_T("DisplayName")] = values[0];
        else if (0 == _tcsicmp(entry_name.c_str(), _T("Description")))
            (flags & SPSVCINST_NOCLOBBER_DESCRIPTION && reg[_T("Description")].exists()) ? __noop : reg[_T("Description")] = values[0];
        else if (0 == _tcsicmp(entry_name.c_str(), _T("ServiceType")))
            reg[_T("Type")] = (DWORD)_tcstoul(values[0].c_str(), &pSzStop, 0);
        else if (0 == _tcsicmp(entry_name.c_str(), _T("StartType")))
            (flags & SPSVCINST_NOCLOBBER_STARTTYPE && reg[_T("Start")].exists()) ? __noop : reg[_T("Start")] = (DWORD)_tcstoul(values[0].c_str(), &pSzStop, 0);
        else if (0 == _tcsicmp(entry_name.c_str(), _T("ErrorControl")))
            (flags & SPSVCINST_NOCLOBBER_ERRORCONTROL && reg[_T("ErrorControl")].exists()) ? __noop : reg[_T("ErrorControl")] = (DWORD)_tcstoul(values[0].c_str(), &pSzStop, 0);
        else if (0 == _tcsicmp(entry_name.c_str(), _T("ServiceBinary")))
            reg[_T("ImagePath")].set_expand_sz(values[0].c_str());
        else if (0 == _tcsicmp(entry_name.c_str(), _T("StartName")))
            reg[_T("ObjectName")] = values[0];
        else if (0 == _tcsicmp(entry_name.c_str(), _T("LoadOrderGroup")))
            (flags & SPSVCINST_NOCLOBBER_LOADORDERGROUP && reg[_T("Group")].exists()) ? __noop : reg[_T("Group")] = values[0];
        else if (0 == _tcsicmp(entry_name.c_str(), _T("Dependencies"))){
            
            for (size_t i = 0; i < values.size(); ++i){
                if (i > 0)
                    value.append(_T(","));
                value.append(values[i]);

                if (values[i].length()){
                    if (values[i][0] == _T('+'))
                        (flags & SPSVCINST_NOCLOBBER_DEPENDENCIES && reg[_T("DependOnGroup")].exists()) ? __noop : reg[_T("DependOnGroup")].add_multi(values[i].substr(1, values[i].length() - 1));
                    else
                        (flags & SPSVCINST_NOCLOBBER_DEPENDENCIES && reg[_T("DependOnService")].exists()) ? __noop : reg[_T("DependOnService")].add_multi(values[i]);
                }
            }
        }
        else if (0 == _tcsicmp(entry_name.c_str(), _T("Security"))){
            // dwFlags & SPSVCINST_CLOBBER_SECURITY 
        }
        LOG(LOG_LEVEL_INFO, _T("Service Path (%s) : add Entry(%s), value(%s)."),
            service_path.c_str(),
            entry_name.c_str(),
            value.c_str());
        return true;
    }
    else{
        LOG(LOG_LEVEL_ERROR, _T("Open/Create Service Path (%s) failed (%d)."),
            service_path.c_str(), GetLastError());
    }
    return false;
}

bool setup_inf_file::setup_action_service_entry::exists(reg_edit_base& reg_edit){

    bool        bExisted = false;
    registry reg(reg_edit);
    stdstring   value = values[0];
    if (reg.open(service_path)){
        if (0 == _tcsicmp(entry_name.c_str(), _T("DisplayName")))
            bExisted = reg[_T("DisplayName")].exists();
        else if (0 == _tcsicmp(entry_name.c_str(), _T("Description")))
            bExisted = reg[_T("Description")].exists();
        else if (0 == _tcsicmp(entry_name.c_str(), _T("ServiceType")))
            bExisted = reg[_T("Type")].exists();
        else if (0 == _tcsicmp(entry_name.c_str(), _T("StartType")))
            bExisted = reg[_T("Start")].exists();
        else if (0 == _tcsicmp(entry_name.c_str(), _T("ErrorControl")))
            bExisted = reg[_T("ErrorControl")].exists();
        else if (0 == _tcsicmp(entry_name.c_str(), _T("ServiceBinary")))
            bExisted = reg[_T("ImagePath")].exists();
        else if (0 == _tcsicmp(entry_name.c_str(), _T("StartName")))
            bExisted = reg[_T("ObjectName")].exists();
        else if (0 == _tcsicmp(entry_name.c_str(), _T("LoadOrderGroup")))
            bExisted = reg[_T("Group")].exists();
        else if (0 == _tcsicmp(entry_name.c_str(), _T("Dependencies"))){
            bExisted = true;
            for (size_t i = 0; i < values[i].size(); ++i)
            {
                if (i > 0)
                    value.append(_T(","));
                value.append(values[i]);

                if (values[i].length() && bExisted)
                {
                    if (values[i][0] == _T('+'))
                        bExisted = reg[_T("DependOnGroup")].exists();
                    else
                        bExisted = reg[_T("DependOnService")].exists();
                }
            }
        }
        else if (0 == _tcsicmp(entry_name.c_str(), _T("Security")))
        {
            bExisted = true;
            // dwFlags & SPSVCINST_CLOBBER_SECURITY 
        }
    }

    LOG(LOG_LEVEL_INFO, _T("Service Path (%s): Entry(%s) %s existed, Value(%s)."),
        service_path.c_str(),
        entry_name.c_str(),
        bExisted ? _T("") : _T("not"),
        value.c_str());

    return bExisted;
}

bool setup_inf_file::setup_action_service_entry::remove(reg_edit_base& reg_edit){
    registry reg(reg_edit);
    if (reg.open(service_path)){
        return reg.delete_key();
    }
    return true;
}

bool setup_inf_file::setup_action_service::install(reg_edit_base& reg_edit){
    bool result = false;
    foreach(setup_inf_file::setup_action_service_entry::ptr &entry, entries){
        if (!(result = entry->install(reg_edit)))
            break;
    }
    return result;
}

bool setup_inf_file::setup_action_service::exists(reg_edit_base& reg_edit){
    bool result = false;
    foreach(setup_inf_file::setup_action_service_entry::ptr &entry, entries){
        if (!(result = entry->exists(reg_edit)))
            break;
    }
    return result;
}

bool setup_inf_file::setup_action_service::remove(reg_edit_base& reg_edit){
    bool result = false;
    foreach(setup_inf_file::setup_action_service_entry::ptr &entry, entries){
        if (!(result = entry->remove(reg_edit)))
            break;
    }
    return result;
}

#endif

};//namespace windows
};//namespace macho

#endif