#include "stdafx.h"
#include "ProfileModule.h"
#include <fstream>
#include <iostream>
#include <iterator>
#include <algorithm>
#include "windows\cabinet.hpp"
#include <codecvt>

#include "json_spirit.h"
using namespace json_spirit;
#pragma comment(lib, "json_spirit_lib.lib") 

bool ProfileModule::exec_console_application( stdstring command, stdstring &ret, bool is_hidden )
{
    HANDLE              hStdInRead, hStdInWrite, hStdInWriteTemp;
    HANDLE              hStdOutRead, hStdOutWrite, hStdOutReadTemp;
    SECURITY_ATTRIBUTES sa;
    STARTUPINFO         si;
    PROCESS_INFORMATION pi;
    HRESULT             hr = S_OK;

    if ( !command.length() )
		return false;
    ZeroMemory( &sa, sizeof( SECURITY_ATTRIBUTES ) );
    sa.nLength = sizeof( SECURITY_ATTRIBUTES );
    sa.bInheritHandle = TRUE;
    ret.empty();
    if ( CreatePipe( &hStdOutRead, &hStdOutWrite, &sa, 0 ) ){
        if ( DuplicateHandle( GetCurrentProcess(), hStdOutRead, GetCurrentProcess(), &hStdOutReadTemp, 0, FALSE, DUPLICATE_SAME_ACCESS ) ){
            if ( CreatePipe( &hStdInRead, &hStdInWrite, &sa, 0 ) ){
                if ( DuplicateHandle( GetCurrentProcess(), hStdInWrite, GetCurrentProcess(), &hStdInWriteTemp, 0, FALSE, DUPLICATE_SAME_ACCESS ) ){
                    ZeroMemory( &si, sizeof( STARTUPINFO ) );
                    si.cb           = sizeof( STARTUPINFO );
                    si.dwFlags      = STARTF_USESTDHANDLES;
                    si.hStdOutput   = hStdOutWrite;
                    si.hStdInput    = hStdInRead;
                    si.hStdError    = GetStdHandle( STD_ERROR_HANDLE );

                    ZeroMemory( &pi, sizeof( PROCESS_INFORMATION ) );

                    if ( CreateProcess( NULL,               /*ApplicationName*/
                                        (LPTSTR) command.c_str(),          /*CommandLine*/
                                        NULL,               /*ProcessAttributes*/
                                        NULL,               /*ThreadAttributes*/ 
                                        TRUE,               /*InheritHandles*/
                                        is_hidden ? CREATE_NO_WINDOW : 0,   /*CreationFlags*/ 
                                        NULL,               /*Environment*/
                                        NULL,               /*CurrentDirectory*/
                                        &si,                /*StartupInfo*/
                                        &pi ) )  {           /*ProcessInformation*/
#define BUFFSIZE 512                        
                        CHAR    szBuffer[ BUFFSIZE + 1 ];
                        DWORD   procRetCode = 0;
                        unsigned long exit=0;  //process exit code
                        unsigned long bread;   //bytes read
                        unsigned long avail;   //bytes available
                        while ( TRUE ){
                            GetExitCodeProcess(pi.hProcess,&exit);      //while the process is running
                            if (exit != STILL_ACTIVE)
                              break;
                            
                            PeekNamedPipe( hStdOutReadTemp, szBuffer, BUFFSIZE, &bread, &avail, NULL);
                            if ( bread == 0 ){
 								FILETIME    ftCreate, ftExit, ftKernel, ftUser;
								memset(&ftExit, 0 , sizeof( FILETIME ) );                           
								
								if( !GetProcessTimes( pi.hProcess, &ftCreate, &ftExit, &ftKernel, &ftUser ) )
									break;
								else if ( ( ftExit.dwHighDateTime != 0 ) || ( ftExit.dwLowDateTime != 0 ) )
									break;
								else{
									//SYSTEMTIME localTimeNow;
									//FILETIME fileTimeNow;
									//GetLocalTime(&localTimeNow);
									//SystemTimeToFileTime(&localTimeNow, &fileTimeNow);
									//ULARGE_INTEGER ulCreate, ulNow;
									//ulCreate.HighPart = ftCreate.dwHighDateTime;
									//ulCreate.LowPart  = ftCreate.dwLowDateTime;
									//ulNow.HighPart	  = fileTimeNow.dwHighDateTime;
									//ulNow.LowPart     = fileTimeNow.dwLowDateTime;
								}
								Sleep(500);
							}
							else{
                                ZeroMemory( szBuffer, sizeof( CHAR ) * ( BUFFSIZE + 1 ) );
                                if ( avail > BUFFSIZE ){
                                    while ( bread >= BUFFSIZE ){
                                        ReadFile( hStdOutReadTemp, szBuffer, BUFFSIZE, &bread, NULL );
#if _UNICODE
                                        ret.append( stringutils::convert_ansi_to_unicode(szBuffer) );
#else
                                        ret.append(szBuffer);
#endif
                                        ZeroMemory( szBuffer, sizeof( CHAR ) * ( BUFFSIZE + 1 ) );
                                    }
                                }
                                else {
                                    ReadFile( hStdOutReadTemp, szBuffer, BUFFSIZE, &bread, NULL );
#if _UNICODE
                                        ret.append( stringutils::convert_ansi_to_unicode(szBuffer) );
#else
                                        ret.append(szBuffer);
#endif                             
                                }
                            }
                        }

                        if ( GetExitCodeProcess( pi.hProcess, &procRetCode ) )            
                            hr = procRetCode;
                        else
                            hr = GetLastError();

                        CloseHandle( pi.hProcess );
                        CloseHandle( pi.hThread );
                    }
                    else
                        hr = GetLastError();
                    CloseHandle( hStdInWriteTemp );
                }            
                else
                    hr = GetLastError();
                CloseHandle( hStdInRead );
                CloseHandle( hStdInWrite );
            }
            else
                hr = GetLastError();
            CloseHandle( hStdOutReadTemp );
        }    
        else
            hr = GetLastError();
        CloseHandle( hStdOutRead );
        CloseHandle( hStdOutWrite );
    }
    else
        hr = GetLastError();
    SetLastError( hr );
    return HRESULT_FROM_WIN32(hr) == 0;
}

void ProfileModule::extract_binary_resource( stdstring custom_resource_name, int resource_id, stdstring output_file_path, bool append ){
	HGLOBAL hResourceLoaded;		// handle to loaded resource 
	HRSRC hRes;						// handle/ptr. to res. info. 
	char *lpResLock;				// pointer to resource data 
	DWORD dwSizeRes;
	// find location of the resource and get handle to it
	hRes = FindResource( NULL, MAKEINTRESOURCE(resource_id), custom_resource_name.c_str() );
	// loads the specified resource into global memory. 
	hResourceLoaded = LoadResource( NULL, hRes ); 
	// get a pointer to the loaded resource!
	lpResLock = (char*)LockResource( hResourceLoaded ); 
	// determine the size of the resource, so we know how much to write out to file!  
	dwSizeRes = SizeofResource( NULL, hRes );
	std::ofstream outputFile(output_file_path.c_str(), append ? std::ios::binary | std::ios::app : std::ios::binary );
	outputFile.write((const char*)lpResLock, dwSizeRes);
	outputFile.close();
}

bool ProfileModule::create_self_extracting_file( stdstring& output_file_path, stdstring& zip_file ){
    extract_binary_resource( _T("BINARIES"), IDR_7ZS, output_file_path );
    std::ofstream outputFile(output_file_path.c_str(), std::ios::binary | std::ios::app );
    std::ifstream inputFile( zip_file.c_str(), std::ios::binary );
    char ch;
    while( inputFile && inputFile.get(ch) ) outputFile.put(ch);
    inputFile.close();
    outputFile.close();
    return true;
}

void ProfileModule::get_system_hal_info(dr_profile& profile){
    registry reg( REGISTRY_READONLY );
    if ( reg.open(_T("SYSTEM\\CurrentControlSet\\Enum\\Root\\PCI_HAL\\0000" ), HKEY_LOCAL_MACHINE  ) ){
        if ( reg[_T("HardwareID")].exists() && reg[_T("HardwareID")].get_multi_count() )
            profile.hal = reg[_T("HardwareID")].get_multi_at(0);
        reg.close();
    }
    if (!profile.system_model.length() && reg.open(_T("SYSTEM\\CurrentControlSet\\Enum\\Root\\ACPI_HAL\\0000" ), HKEY_LOCAL_MACHINE ) ){
        if (reg[_T("HardwareID")].exists() && reg[_T("HardwareID")].get_multi_count())
            profile.hal = reg[_T("HardwareID")].get_multi_at(0);
        reg.close();
    } 
}

void ProfileModule::get_module_info(dr_profile& profile){
    wmi_services wmi;
    wmi.connect( L"cimv2" );
    wmi_object_table objs = wmi.exec_query( L"Select * from Win32_ComputerSystem");
    if ( objs.size() > 0 ){
        profile.system_model = (std::wstring) objs[0][L"Model"];
        profile.manufacturer = (std::wstring) objs[0][L"Manufacturer"];
    }
}

bool ProfileModule::dump_drivers_files(dr_profile& profile, boost::filesystem::path& output_folder, hardware_device::vtr& selected_devices ){
    try{
        std::map<stdstring,boost::filesystem::path> drivers_path_map;
        foreach( hardware_device::ptr d, selected_devices ){ // foreach( drivers_map::value_type &drv, _drivers_map )
            if ( _stop_flag ) break ;
            if ( !d->is_oem() ) continue;
            stdstring key = d->hardware_ids.size() ? d->hardware_ids[0] : d->compatible_ids[0];
            hardware_driver::ptr drv = _drivers_map[key];
			boost::filesystem::path driver_sub_path = boost::filesystem::path(profile.os_folder()) / profile.os.sz_cpu_architecture() / boost::str(boost::wformat(L"%s_%s_%s") % drv->original_inf_name %drv->driver_version %drv->driver_date);
            drivers_path_map[drv->driver] = driver_sub_path;
            boost::filesystem::path drv_folder = output_folder/driver_sub_path;
            boost::filesystem::create_directories(drv_folder);
            save_action(PROFILE_STATE_ON_PROGRESS, d, drv, drv->original_inf_name);

			bool is_driver_store = false;
			boost::filesystem::path driver_store_sub_path;
			boost::filesystem::path driver_store_path = boost::filesystem::path(environment::get_system_directory()) / "DriverStore" / "FileRepository";
			if (drv->catalog_file.length() && drv->original_catalog_name.length()){
				if ((drv->catalog_file.length() > driver_store_path.wstring().length()) &&
					0 == _wcsnicmp(driver_store_path.wstring().c_str(), drv->catalog_file.c_str(), driver_store_path.wstring().length())){
					is_driver_store = true;
					driver_store_sub_path = drv->catalog_file;
				}
			}
			else{
				foreach(driver_file_path::ptr file, drv->driver_files){
					if ((file->source.length() > driver_store_path.wstring().length()) &&
						0 == _wcsnicmp(driver_store_path.wstring().c_str(), file->source.c_str(), driver_store_path.wstring().length())){
						driver_store_sub_path = file->source;
						is_driver_store = true;
						break;
					}
				}
			}
			boost::filesystem::path sub_folder;
			if (is_driver_store){
				while (0 != _wcsicmp(driver_store_path.wstring().c_str(), driver_store_sub_path.wstring().c_str())){
					sub_folder = driver_store_sub_path.filename();
					driver_store_sub_path = driver_store_sub_path.parent_path();
				}
			}
			if (!sub_folder.empty()){
				copy_directory(driver_store_path / sub_folder, drv_folder);
			}
			else{
				boost::filesystem::copy_file(drv->inf_path, drv_folder / drv->original_inf_name, boost::filesystem::copy_option::overwrite_if_exists);
				if (drv->catalog_file.length() && drv->original_catalog_name.length()){
					save_action(PROFILE_STATE_ON_PROGRESS, d, drv, drv->original_catalog_name);
					boost::filesystem::copy_file(drv->catalog_file, drv_folder / drv->original_catalog_name, boost::filesystem::copy_option::overwrite_if_exists);
				}
				setup_inf_file inf;
				setup_inf_file::source_disk_file::map src_disk_files_map;
				if (inf.load(drv->inf_path))
					src_disk_files_map = inf.get_source_disk_files(profile.os);
				foreach(driver_file_path::ptr file, drv->driver_files){
					if (_stop_flag) break;
					stdstring filename, sub_dir;
					boost::filesystem::path source(file->source);
					boost::filesystem::path target(file->target);
					if (file->source.length()){
						filename = source.filename().wstring();
					}
					else if (file->target.length()){
						filename = target.filename().wstring();
					}
					if (filename.length() && src_disk_files_map.count(filename)){
						if (src_disk_files_map[filename].sub_dir.length()){
							sub_dir = src_disk_files_map[filename].sub_dir;
							if ((sub_dir.length() > 1) && (sub_dir[0] == _T('.')) && ((sub_dir[1] == _T('\\')) || (sub_dir[1] == _T('/'))))
								sub_dir = sub_dir.substr(2, -1);
							else if ((sub_dir[0] == _T('\\')) || (sub_dir[0] == _T('/')))
								sub_dir = sub_dir.substr(1, -1);
						}
						if (sub_dir.length()){
							save_action(PROFILE_STATE_ON_PROGRESS, d, drv, filename);
							boost::filesystem::create_directories(drv_folder / sub_dir);
							boost::filesystem::copy_file(target, drv_folder / sub_dir / filename, boost::filesystem::copy_option::overwrite_if_exists);
						}
						else{
							save_action(PROFILE_STATE_ON_PROGRESS, d, drv, filename);
							boost::filesystem::copy_file(target, drv_folder / filename, boost::filesystem::copy_option::overwrite_if_exists);
						}
					}
				}
			}
        }
        // Dump devices hardware info.
        foreach( hardware_device::ptr d, _profile_devices ){
            if ( _stop_flag ) break ;
            hardware_device_ex dx(*d);
            stdstring key = d->hardware_ids.size() ? d->hardware_ids[0] : d->compatible_ids[0];
            dx.folder         = drivers_path_map[d->driver].wstring();
            if ( !d->is_oem() )
                profile.devices.push_back(dx);
            else if ( dx.folder.length() ){
                dx.inf_name       = _drivers_map[key]->original_inf_name;
                dx.driver_date    = _drivers_map[key]->driver_date;
                dx.driver_version = _drivers_map[key]->driver_version;
                dx.inf_section    = _drivers_map[key]->inf_section;
                profile.devices.push_back(dx);
            }
        }
    }
    catch(const boost::filesystem::filesystem_error& e){
        save_action( PROFILE_STATE_ON_PROGRESS, stringutils::convert_ansi_to_unicode(std::string(e.what())) );
        return false;
    }
    catch(...){
        return false;
    }
    return !_stop_flag;
}

void ProfileModule::save_profile(  PROFILE_TYPE_ENUM type, stdstring path, hardware_device::vtr selected_devices ){
    macho::windows::auto_lock lock( _cs );
    save_action(PROFILE_STATE_START);
    try{
        com_init _init;
        dr_profile profile;
		storage::ptr stg = storage::get();
        profile.computer_name = environment::get_computer_name();
        profile.os            = environment::get_os_version();
        get_system_hal_info(profile);
        get_module_info(profile);
        boost::filesystem::path output_folder = ( type == PROFILE_TYPE_FOLDER ) ? path : environment::create_temp_folder(_T("prf"));
        //Dump drivers files;
        if ( !dump_drivers_files( profile, output_folder, selected_devices ) ){
        }
        else{
            save_action(PROFILE_STATE_SYSTEM_INFO);
            extract_binary_resource( _T("BINARIES"), IDR_DPINST, (output_folder/"dpinst.exe").wstring() );
            extract_binary_resource( _T("BINARIES"), IDR_DPINST_XML, (output_folder/"dpinst.xml").wstring() );
            dr_profiles_table profiles;
            profiles.push_back(profile);
            write_profiles_to_json( profiles, (output_folder/"machine.cfg").wstring() );         
            if ( type != PROFILE_TYPE_FOLDER )
                save_action(PROFILE_STATE_COMPRESS);
			if ( type == PROFILE_TYPE_EXE ){
                save_action(PROFILE_STATE_COMPRESS);
				stdstring zip_file = environment::create_temp_file(_T("zip"));
				boost::filesystem::remove(zip_file);
				archive::zip::ptr zip_ptr = archive::zip::open(zip_file);
				if (zip_ptr){
					zip_ptr->add_dir(output_folder);
					zip_ptr->close();
					zip_ptr = nullptr;
					create_self_extracting_file(path, zip_file);
				}
            }
        }
        if ( _stop_flag || ( type == PROFILE_TYPE_EXE ) || ( type == PROFILE_TYPE_CAB ) ) {
            boost::filesystem::remove_all(output_folder);
        }
    }
    catch(...){
    }
    save_action(PROFILE_STATE_FINISHED);
}

void ProfileModule::enum_devices(){
    macho::windows::auto_lock lock( _cs );
    _stop_flag = false;
    action( PROFILE_STATE_START );
    _classes = _devmgr.get_classes();
    foreach( hardware_class::ptr c, _classes ){
        if ( _stop_flag ) break ;
        hardware_device::vtr devices = _devmgr.get_devices(c->name);
        if ( devices.size() ) _devices_map[c->name] = devices;
    }
    foreach( hardware_class::ptr c, _classes ){
        if ( _stop_flag ) break ;
        if ( _devices_map.count( c->name ) > 0 ){
            hardware_device::vtr devices;
            foreach( hardware_device::ptr d,  _devices_map[c->name] ){
                if ( ( !_tcsicmp( d->sz_class.c_str(), _T("SCSIAdapter") ) || !_tcsicmp( d->sz_class.c_str(), _T("Hdc") ) || d->is_oem() ) 
                    && _tcsicmp( d->enumeter.c_str(), _T("ROOT") ) && _tcsicmp( d->enumeter.c_str(), _T("PCIIDE") ) ){
                    devices.push_back(d);
                }
            }
            foreach( hardware_device::ptr d, devices ){
                if ( _stop_flag ) break ;
                stdstring key = d->hardware_ids.size() ? d->hardware_ids[0] : d->compatible_ids[0];
                if ( 0 == _drivers_map.count( key) ){
                    hardware_driver::ptr driver = _devmgr.get_device_driver(*d);
                    _drivers_map[key] = driver;
                    _profile_devices.push_back(d);
                    if ( d->is_oem() ){ 
                        action( PROFILE_STATE_ON_PROGRESS, c, d, driver );
                    }
                }
            }
        }
    }
    action( PROFILE_STATE_FINISHED );
}

stdstring const ProfileModule::get_default_path( PROFILE_TYPE_ENUM type ){
    if ( _default_path.empty() ) _default_path = boost::filesystem::path(environment::get_environment_variable(_T("USERPROFILE")))/_T("Desktop");
    if ( boost::filesystem::exists( _default_path ) ){
        if ( boost::filesystem::is_directory( _default_path ) )
            return _default_path.wstring();
        else
            return _default_path.parent_path().wstring();
    }
    else if ( _default_path.has_extension() ){    
        return  _default_path.parent_path().wstring(); 
    }
    return _default_path.wstring();
}

stdstring ProfileModule::guid_to_string( const GUID& guid ){
    stdstring str;
    OLECHAR* bstrGuid;
    StringFromCLSID(guid, &bstrGuid);
    str = bstrGuid;
    CoTaskMemFree(bstrGuid);
  return str;
}

void ProfileModule::write_profiles_to_json( dr_profiles_table &dr_profiles, stdstring output_file ){
    try{
        wArray  profiles;
        foreach( dr_profile dr_profile,  dr_profiles ){
            wObject profile;
            wArray devices, services, disks, volumes;
            bytes system_info( (LPBYTE)&dr_profile.os.system_info, sizeof( SYSTEM_INFO ) );
            bytes version_info( (LPBYTE)&dr_profile.os.version_info, sizeof( OSVERSIONINFOEX ) );
            profile.push_back( wPair( L"computer_name",	    dr_profile.computer_name ) );
            profile.push_back( wPair( L"hal",	            dr_profile.hal ) );
            profile.push_back( wPair( L"system_model",	    dr_profile.system_model ) );
            profile.push_back( wPair( L"manufacturer",	    dr_profile.manufacturer ) );
            profile.push_back( wPair( L"os_name",	        dr_profile.os.name ) );
            profile.push_back( wPair( L"os_type",	        (int)dr_profile.os.type ) );
            profile.push_back( wPair( L"os_system_info",    system_info.get() ) );
            profile.push_back( wPair( L"os_version_info",   version_info.get() ) );
            
            foreach( hardware_device_ex d, dr_profile.devices ){
                wObject obj;
                wValue  hardware_ids( d.hardware_ids.begin(), d.hardware_ids.end());	
                wValue  compatible_ids( d.compatible_ids.begin(), d.compatible_ids.end());	
                wValue  childs( d.childs.begin(), d.childs.end());	
                obj.push_back( wPair( L"device_description", d.device_description ) );
                obj.push_back( wPair( L"device_instance_id", d.device_instance_id ) );
                obj.push_back( wPair( L"driver",             d.driver ) );
                obj.push_back( wPair( L"driver_date",        d.driver_date ) );
                obj.push_back( wPair( L"driver_version",     d.driver_version ) );
                obj.push_back( wPair( L"enumeter",           d.enumeter ) );
                obj.push_back( wPair( L"folder",             d.folder ) );
                obj.push_back( wPair( L"inf_name",           d.inf_name ) );
                obj.push_back( wPair( L"inf_section",        d.inf_section ) );
                obj.push_back( wPair( L"install_state",      (int)d.install_state ) );
                obj.push_back( wPair( L"location",           d.location ) );
                obj.push_back( wPair( L"location_path",      d.location_path ) );
                obj.push_back( wPair( L"manufacturer",       d.manufacturer ) );
                obj.push_back( wPair( L"parent",             d.parent ) );
                obj.push_back( wPair( L"physical_device_object", d.physical_device_object ) );
                obj.push_back( wPair( L"problem",            (int)d.problem ) );
                obj.push_back( wPair( L"service",            d.service ) );
                obj.push_back( wPair( L"status",             (int)d.status ) );
                obj.push_back( wPair( L"sz_class",           d.sz_class ) );
                obj.push_back( wPair( L"sz_class_guid",      d.sz_class_guid ) );
                obj.push_back( wPair( L"type",               d.type ) );
                obj.push_back( wPair( L"hardware_ids",       hardware_ids ) );
                obj.push_back( wPair( L"compatible_ids",     compatible_ids ) );
                obj.push_back( wPair( L"childs",             childs ) );
                devices.push_back( obj  );
            }
            profile.push_back( wPair( L"devices",	 devices ) );
            profiles.push_back(profile);
        }
        std::wofstream output( output_file, std::ios::out|std::ios::trunc );
        std::locale loc(std::locale::classic(), new std::codecvt_utf8<wchar_t>);
        output.imbue(loc);
        write( profiles, output, pretty_print | raw_utf8 );
    }
    catch( boost::exception& ex ){
        std::cout << boost::exception_detail::get_diagnostic_information( ex, "can't output machine info." ) << std::endl;
    }
    catch(...){
    }
}

bool ProfileModule::copy_directory(boost::filesystem::path const & source, boost::filesystem::path const & destination){
	namespace fs = boost::filesystem;
	try
	{
		// Check whether the function call is valid
		if (
			!fs::exists(source) ||
			!fs::is_directory(source)
			)
		{
			std::cerr << "Source directory " << source.string()
				<< " does not exist or is not a directory." << '\n'
				;
			return false;
		}
		if (fs::exists(destination))
		{
			std::clog << "Destination directory " << destination.string()
				<< " already exists." << '\n'
				;
			//return false;
		}
		// Create the destination directory
		else if (!fs::create_directories(destination))
		{
			std::cerr << "Unable to create destination directory"
				<< destination.string() << '\n'
				;
			return false;
		}
	}
	catch (fs::filesystem_error const & e)
	{
		std::cerr << e.what() << '\n';
		return false;
	}
	// Iterate through the source directory
	for (
		fs::directory_iterator file(source);
		file != fs::directory_iterator(); ++file
		)
	{
		try
		{
			fs::path current(file->path());
			if (fs::is_directory(current))
			{
				// Found directory: Recursion
				if (
					!copy_directory(
					current,
					destination / current.filename()
					)
					)
				{
					return false;
				}
			}
			else if (current.extension() != ".PNF")
			{				
				// Found file: Copy
				fs::copy_file(
					current,
					destination / current.filename(),
					boost::filesystem::copy_option::overwrite_if_exists);
			}
		}
		catch (fs::filesystem_error const & e)
		{
			std::cerr << e.what() << '\n';
		}
	}
	return true;
}
