#pragma once
#ifndef __irm_os_image_info__
#define __irm_os_image_info__

#include "reg_hive_edit.h"

struct os_image_info{
    os_image_info() : machine_type(0), major_version(0), minor_version(0), product_type(0), suite_mask(0), concurrent_limit(0), mode(0), is_pending_windows_update(false), is_eval_key(false){}
    bool is_dc()       const { return product_type == VER_NT_DOMAIN_CONTROLLER; }
    bool has_domain()  const { return domain.length() > 0; }
    bool is_valid()    const { return major_version > 0; }
    bool is_amd64()    const { return machine_type == IMAGE_FILE_MACHINE_AMD64; }
    bool is_ia64()     const { return machine_type == IMAGE_FILE_MACHINE_IA64; }
    bool is_x86()      const { return machine_type == IMAGE_FILE_MACHINE_I386; }
    DWORD					    machine_type;
    DWORD					    major_version;
    DWORD					    minor_version;
    DWORD   			        product_type;
    DWORD					    suite_mask;
    std::wstring                product_name;
    std::wstring                lic;
    boost::filesystem::path     win_dir;
    std::wstring 				version;
    std::wstring 				architecture;
    std::wstring                system_root;
    std::wstring                system_hal;
    std::wstring                computer_name;
    std::wstring                domain;
    std::wstring                time_zone;
    std::wstring                full_name;
    std::wstring                orgname;
    std::wstring                work_group;
    std::wstring                csd_version;
    std::wstring                csd_build_number;
    DWORD                       concurrent_limit;
    DWORD                       mode;
    bool                        is_pending_windows_update;
    bool                        is_eval_key;
    reg_hive_edit::ptr          system_hive_edit_ptr;

    operator macho::windows::operating_system() { return to_operating_system(); }
    macho::windows::operating_system to_operating_system(){
        macho::windows::operating_system os;
        os.name = product_name;
        switch (machine_type){
        case IMAGE_FILE_MACHINE_AMD64:
            os.system_info.wProcessorArchitecture = PROCESSOR_ARCHITECTURE_AMD64;
            break;
        case IMAGE_FILE_MACHINE_IA64:
            os.system_info.wProcessorArchitecture = PROCESSOR_ARCHITECTURE_IA64;
            break;
        case IMAGE_FILE_MACHINE_I386:
            os.system_info.wProcessorArchitecture = PROCESSOR_ARCHITECTURE_INTEL;
            break;
        }
        os.version_info.dwPlatformId = VER_PLATFORM_WIN32_NT;       
        os.version_info.wProductType = product_type;
        os.version_info.dwMajorVersion = major_version;
        os.version_info.dwMinorVersion = minor_version;
        os.version_info.wSuiteMask = suite_mask;
        os.version_info.dwOSVersionInfoSize = sizeof(os.version_info);
        return os;
    }

	boost::filesystem::path to_path(){
		boost::filesystem::path os_folder;
		if (major_version == 10){
			os_folder = product_type == VER_NT_WORKSTATION ? L"w10" : L"2k16";
		}
		else if (major_version == 6){
			switch (minor_version){
			case 0:
				os_folder = product_type == VER_NT_WORKSTATION ? L"w7" : L"2k8";
				break;
			case 1:
				os_folder = product_type == VER_NT_WORKSTATION ? L"w7" : L"2k8r2";
				break;
			case 2:
				os_folder = product_type == VER_NT_WORKSTATION ? L"w8" : L"2k12";
				break;
			case 3:
				os_folder = product_type == VER_NT_WORKSTATION ? L"w8.1" : L"2k12r2";
				break;
			default:
				os_folder = product_type == VER_NT_WORKSTATION ? L"w8.1" : L"2k12r2";
				break;
			}
		}
		else if (major_version == 5){
			switch (minor_version){
			case 1:
				os_folder = product_type == VER_NT_WORKSTATION ? L"xp" : L"xp";
				break;
			case 2:
				os_folder = product_type == VER_NT_WORKSTATION ? L"2k3" : L"2k3";
				break;
			}
		}

		if (is_amd64()){
			os_folder /= L"AMD64";
		}
		else if (is_x86()){
			os_folder /= L"X86";
		}
		return os_folder;
	}
};

#endif