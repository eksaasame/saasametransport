
#pragma once
#include "macho.h"
using namespace macho;
using namespace macho::windows;

struct hardware_device_ex : hardware_device{
    stdstring                 folder;
    stdstring                 inf_section;
    stdstring                 driver_version;
    stdstring                 driver_date;
    hardware_device_ex ( const hardware_device& _device ){
        hardware_device::copy(_device);
    }
    hardware_device_ex ( const hardware_device_ex& _device ){
        copy(_device);
    }
    void copy ( const hardware_device_ex& _device ){
        folder                = _device.folder;  
        inf_section           = _device.inf_section; 
        driver_version        = _device.driver_version; 
        driver_date           = _device.driver_date; 
        hardware_device::copy(_device);     
    }
    const hardware_device_ex &operator =( const hardware_device_ex& _device ){
        if ( this != &_device )
            copy( _device );
        return( *this );
    }
};
typedef std::vector<hardware_device_ex> hardware_devices_ex_table;

struct dr_profile{
    stdstring                       computer_name;
    stdstring                       hal;
    stdstring                       system_model;
    stdstring                       manufacturer;
    operating_system                os;
    hardware_devices_ex_table       devices;
    //IP settings
    //Cluster Info
	stdstring						os_folder(){
		stdstring os_folder;
		if (os.major_version() == 10){
			os_folder = os.product_type() == VER_NT_WORKSTATION ? _T("w10") : _T("2k16");
		}
		else if (os.major_version() == 6){
			switch (os.minor_version()){
			case 0:
				os_folder = os.product_type() == VER_NT_WORKSTATION ? _T("w7") : _T("2k8");
				break;
			case 1:
				os_folder = os.product_type() == VER_NT_WORKSTATION ? _T("w7") : _T("2k8r2");
				break;
			case 2:
				os_folder = os.product_type() == VER_NT_WORKSTATION ? _T("w8") : _T("2k12");
				break;
			case 3:
				os_folder = os.product_type() == VER_NT_WORKSTATION ? _T("w8.1") : _T("2k12r2");
				break;
			default:
				os_folder = os.product_type() == VER_NT_WORKSTATION ? _T("w8.1") : _T("2k12r2");
				break;
			}
		}
		else if (os.major_version() == 5){
			switch (os.minor_version()){
			case 1:
				os_folder = os.product_type() == VER_NT_WORKSTATION ? _T("xp") : _T("xp");
				break;
			case 2:
				os_folder = os.product_type() == VER_NT_WORKSTATION ? _T("2k3") : _T("2k3");
				break;
			}
		}
		return os_folder;
	}
};
typedef std::vector<dr_profile> dr_profiles_table;
