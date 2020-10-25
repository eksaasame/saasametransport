#pragma once

#include "macho.h"
#include "boost\thread.hpp"
#include "boost\signals2\signal.hpp"
#include "DrProfile.h"

using namespace macho;
using namespace macho::windows;

typedef enum _PROFILE_STATE_ENUM{
PROFILE_STATE_UNKNOWN               = 0x00000000,
PROFILE_STATE_START                 = 0x00000001,
PROFILE_STATE_ON_PROGRESS           = 0x00000002,
PROFILE_STATE_SYSTEM_INFO           = 0x00000003,
PROFILE_STATE_COMPRESS              = 0x00000004,
PROFILE_STATE_CREATE_SELFEXTRACT    = 0x00000005,
PROFILE_STATE_FINISHED              = 0x00000006
}PROFILE_STATE_ENUM;

typedef enum _PROFILE_TYPE_ENUM{
PROFILE_TYPE_EXE           = 0x00000000,
PROFILE_TYPE_CAB           = 0x00000001,
PROFILE_TYPE_FOLDER        = 0x00000002
}PROFILE_TYPE_ENUM;

typedef boost::signals2::signal<void ( PROFILE_STATE_ENUM, hardware_class::ptr&, hardware_device::ptr&, hardware_driver::ptr& )> ON_ENUM_PROGRESS;
typedef ON_ENUM_PROGRESS::slot_type ON_ENUM_PROGRESS_TYPE;

typedef boost::signals2::signal<void ( PROFILE_STATE_ENUM, hardware_device::ptr&, hardware_driver::ptr&, stdstring&, stdstring& )> ON_SAVE_PROGRESS;
typedef ON_SAVE_PROGRESS::slot_type ON_SAVE_PROGRESS_TYPE;
typedef std::map<stdstring, hardware_driver::ptr, stringutils::no_case_string_less> drivers_map;
typedef std::map<stdstring, hardware_device::vtr, stringutils::no_case_string_less> devices_map;
class ProfileModule{
public:
    void set_default_type( PROFILE_TYPE_ENUM type ){ _default_type = type; }
    void set_default_path( stdstring path ) { _default_path = path ; }
    PROFILE_TYPE_ENUM get_default_type() const { return _default_type; }
    stdstring  const  get_default_path( PROFILE_TYPE_ENUM type ) ;
    void set_enumerate_devices_callback(const ON_ENUM_PROGRESS_TYPE &slot ) { _enum_progress.connect(slot); }
    void set_save_profile_callback( const ON_SAVE_PROGRESS_TYPE &slot ){ _save_progress.connect(slot); }
    void start_to_show_devices(){
        _thread = boost::thread( &ProfileModule::enum_devices, this ); 
    }

    bool start_to_save_profile( PROFILE_TYPE_ENUM type, stdstring path, hardware_device::vtr& selected_devices ){
        if ( path.length() && _cs.trylock() ){ 
            _cs.unlock();
            _thread = boost::thread( &ProfileModule::save_profile, this, type, path, selected_devices ); 
            return true;
        }
        return false;
    }

	void inline action(PROFILE_STATE_ENUM state, hardware_class::ptr hclass = hardware_class::ptr(), hardware_device::ptr& device = hardware_device::ptr(), hardware_driver::ptr& driver = hardware_driver::ptr()){
        _state = state;
        _enum_progress( state, hclass, device, driver  ); 
    }
    
    void inline save_action( PROFILE_STATE_ENUM state, hardware_device::ptr& device, hardware_driver::ptr& driver, stdstring& file_name = stdstring(), stdstring &err = stdstring()){ 
        _state = state;
        _save_progress( state, device, driver, file_name, err ); 
    }

	void inline save_action(PROFILE_STATE_ENUM state, stdstring &err = stdstring()){
        _state = state;
		_save_progress(state, hardware_device::ptr(), hardware_driver::ptr(), stdstring(), err);
    }

    void save_profile(PROFILE_TYPE_ENUM type, stdstring path, hardware_device::vtr selected_devices);
    void enum_devices();
    bool is_stopping() {
        return _stop_flag;
    }
    bool is_stopped() {
        return _state == PROFILE_STATE_FINISHED ;
    }
    void stop(){
        _stop_flag = true;
    }
    ProfileModule():_stop_flag(false), _state( PROFILE_STATE_FINISHED ), _default_type(PROFILE_TYPE_EXE) {}

    static void extract_binary_resource( stdstring custom_resource_name, int resource_id, stdstring output_file_path, bool append = false );
    static bool exec_console_application( stdstring command, stdstring &ret = stdstring(), bool is_hidden = true );
private:
	static bool copy_directory(boost::filesystem::path const & source, boost::filesystem::path const & destination);
    static bool create_self_extracting_file( stdstring& output_file_path, stdstring& zip_file );
    static void write_profiles_to_json( dr_profiles_table &dr_profiles, stdstring output_file );
    static void get_system_hal_info(dr_profile& profile);
    static void get_module_info(dr_profile& profile);
    static stdstring guid_to_string( const GUID& guid );
    bool dump_drivers_files(dr_profile& profile, boost::filesystem::path& output_folder, hardware_device::vtr& selected_devices);
    hardware_class::vtr                          _classes;
    device_manager                               _devmgr;
    PROFILE_STATE_ENUM                           _state;
    devices_map                                  _devices_map;
    drivers_map                                  _drivers_map;
    hardware_device::vtr                         _profile_devices;
    macho::windows::critical_section             _cs;
    bool                                         _stop_flag;
    boost::thread	                             _thread;
    ON_ENUM_PROGRESS                             _enum_progress;
    ON_SAVE_PROGRESS                             _save_progress;
    PROFILE_TYPE_ENUM                            _default_type;
    boost::filesystem::path                      _default_path;
};

