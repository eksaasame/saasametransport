#pragma once
#ifndef __irm_conv__
#define __irm_conv__

#include "os_image_info.h"
#include "universal_disk_rw.h"
#include "irm_converter.h"

class irm_conv{
public:
    struct  exception : virtual public macho::exception_base {};
    static bool          set_privileges();
    static bool          get_win_dir(__in const boost::filesystem::path bcd_folder, __out windows_element &system_device_element);
    static bool          get_win_dir_ini(__in const boost::filesystem::path boot_ini_file, __in macho::windows::storage::disk::ptr d, __out windows_element &system_device_element);
    static bool          set_bcd_boot_device_entry(__in const boost::filesystem::path bcd_folder, __in const windows_element &system_device_element);
    static os_image_info get_offline_os_info(boost::filesystem::path win_dir);
    static bool          fix_disk_geometry_issue(__in DWORD disk_number, DISK_GEOMETRY &geometry);
    static bool          fix_mbr_disk_signature_issue(__in DWORD disk_number, DWORD signature);
    static bool          fix_boot_volume_geometry_issue(__in const boost::filesystem::path boot_volume, DISK_GEOMETRY &geometry);
    static void          set_active_directory_recovery(__in const os_image_info &image, __in bool is_sysvol_authoritative_restore);
    static void          set_machine_password_change(__in const os_image_info &image, __in bool is_disable);
    static void          disable_unexpected_shutdown_message(__in const os_image_info &image);
    static bool          add_windows_service(__in const os_image_info &image, __in const std::wstring short_service_name, __in const std::wstring display_name, __in const std::wstring image_path);
    static bool          clone_key_entry(macho::windows::reg_edit_base& reg, HKEY key, std::wstring source_path, std::wstring target_path, uint32_t level = -1);
    static bool          add_credential_provider(__in const os_image_info &image, __in macho::guid_ clsid, __in const std::wstring name = L"irm_credential_provider", __in const std::wstring dll = L"irm_credential_provider.dll");
    static void          windows_service_control(__in const os_image_info &image, const std::map<std::wstring,int>& services);
    static bool          copy_directory(boost::filesystem::path const & source, boost::filesystem::path const & destination);
    inline static bool   flush_and_dismount_fs(std::wstring volume_path){
        return temp_drive_letter::flush_and_dismount_fs(volume_path);
    }
    static bool          is_drive_letter(std::wstring volume_path);
    static bool          create_hybrid_mbr(__in const int disk_number, __out std::string& gpt);
    static bool          remove_hybrid_mbr(__in const int disk_number, __in std::string gpt);
    static boost::shared_ptr<DRIVE_LAYOUT_INFORMATION_EX> get_drive_layout(__in const int disk_number);
    static bool          add_drivers(__in const boost::filesystem::path system_volume, __in const boost::filesystem::path path, bool force_unsigned = false);
    static bool          remove_drivers(__in const boost::filesystem::path system_volume, __in const std::vector<boost::filesystem::path> paths);
	inline static bool   exec_console_application_with_timeout(std::wstring command, std::wstring &ret, ULONGLONG seconds, bool is_hidden = true){
		return macho::windows::process::exec_console_application_with_timeout(command, ret, seconds, is_hidden);
	}
    static DWORD         write_to_file(boost::filesystem::path file_path, LPCVOID buffer, DWORD number_of_bytes_to_write, DWORD mode = CREATE_ALWAYS);
    static DWORD         write_to_file(boost::filesystem::path file_path, std::string content, DWORD mode = CREATE_ALWAYS){
        return write_to_file(file_path, content.c_str(), content.length(), mode);
    }
    static std::string  read_from_file(boost::filesystem::path file_path);
private:
    static boost::shared_ptr<DRIVE_LAYOUT_INFORMATION_EX> get_drive_layout(HANDLE handle);

};

#endif