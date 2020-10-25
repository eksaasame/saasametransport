#include "physical_packer_service_handler.h"
#include "json_storage.hpp"
#include "..\vcbt\vcbt\journal.h"
#include "temp_drive_letter.h"
#include "mgmt_op.h"

extern volatile bool g_is_pooling_mode;

void physical_packer_service_handler::take_snapshots(std::vector<snapshot> & _return, const std::string& session_id, const std::set<std::string> & disks) {
    FUN_TRACE;
    macho::windows::com_init  com;
    macho::windows::auto_lock lock(_lock);
    string_array volumes;
    std::vector<macho::guid_> guids, snapshot_results;
    macho::windows::storage::ptr stg = macho::windows::storage::get();
    macho::windows::storage::disk::vtr _disks = stg->get_disks();
    foreach(std::string disk, disks){
        foreach(macho::windows::storage::disk::ptr d, _disks){
            if (disk_universal_identify(*d) == disk_universal_identify(disk)){
                macho::windows::storage::volume::vtr vols = d->get_volumes();
                foreach(macho::windows::storage::volume::ptr v, vols){
                    if (v->file_system() == L"NTFS" || v->file_system() == L"ReFS" || v->file_system() == L"CSVFS"){
                        volumes.push_back(v->path());
                        guids.push_back(v->id());
                    }
                }
            }
        }
    }

    if (volumes.size()){
        macho::windows::json_storage j_storage(stg->get_disks(), stg->get_partitions(), stg->get_volumes());
        j_storage.save(boost::filesystem::path(macho::windows::environment::get_windows_directory()) / L"storage.json");
        boost::filesystem::path snapshot_file = boost::filesystem::path(macho::windows::environment::get_working_directory()) / L"snapshot.txt";
        irm_vss_snapshot::write_file(snapshot_file.string(), boost::posix_time::to_simple_string(boost::posix_time::microsec_clock::local_time()));
        send_vcbt_command(IOCTL_VCBT_SNAPSHOT, guids, snapshot_results);
        send_vcbt_command(IOCTL_VCBT_ENABLE, guids);
        _req.init();
        irm_vss_snapshot_result::vtr snapshots = _req.take_snapshot(volumes);
        boost::filesystem::remove(boost::filesystem::path(macho::windows::environment::get_windows_directory()) / L"storage.json");
        boost::filesystem::remove(snapshot_file);

        if (snapshots.size()){
            send_vcbt_command(IOCTL_VCBT_POST_SNAPSHOT, snapshot_results);
        }
        else{
            send_vcbt_command(IOCTL_VCBT_UNDO_SNAPSHOT, snapshot_results);
        }

        foreach(irm_vss_snapshot_result::ptr &r, snapshots){
            snapshot        s;
            s.creation_time_stamp = to_simple_string(r->creation_time_stamp);
            s.snapshots_count = r->snapshots_count;
            s.snapshot_id = r->snapshot_id;
            s.snapshot_set_id = r->snapshot_set_id;
            s.original_volume_name = stringutils::convert_unicode_to_ansi(r->original_volume_name);
            s.snapshot_device_object = stringutils::convert_unicode_to_ansi(r->snapshot_device_object);
            _return.push_back(s);
        }
    }
}

void physical_packer_service_handler::take_snapshots_ex(std::vector<snapshot> & _return, const std::string& session_id, const std::set<std::string> & disks, const std::string& pre_script, const std::string& post_script){
    if (!pre_script.empty()){
		std::wstring result;
        boost::filesystem::path pre_script_file = boost::filesystem::path(macho::windows::environment::get_working_directory()) / L"pre_script.cmd";
        irm_vss_snapshot::write_file(pre_script_file.string(), pre_script);
        process::exec_console_application_with_timeout(pre_script_file.wstring(), result, -1);
        LOG(LOG_LEVEL_RECORD, _T("Pre Snapshot Script result : %s"), result.c_str());
    }
    
    take_snapshots(_return, session_id, disks);

    if (!post_script.empty()){
		std::wstring result;
        boost::filesystem::path post_script_file = boost::filesystem::path(macho::windows::environment::get_working_directory()) / L"post_script.cmd";
        irm_vss_snapshot::write_file(post_script_file.string(), post_script);
        process::exec_console_application_with_timeout(post_script_file.wstring(), result, -1);
        LOG(LOG_LEVEL_RECORD, _T("Post Snapshot Script result : %s"), result.c_str());
    }
}

void physical_packer_service_handler::delete_snapshot(delete_snapshot_result& _return, const std::string& session_id, const std::string& snapshot_id) {
    FUN_TRACE;
    macho::windows::com_init  com;
    macho::windows::auto_lock lock(_lock);
    _req.init();
    LONG deleted_snapshots;
    VSS_ID non_deleted_snapshot_id;
    _return.code = _req.delete_snapshot(macho::guid_(snapshot_id), deleted_snapshots, non_deleted_snapshot_id);
    _return.deleted_snapshots = deleted_snapshots;
    _return.non_deleted_snapshot_id = macho::guid_(non_deleted_snapshot_id);
}

void physical_packer_service_handler::delete_snapshot_set(delete_snapshot_result& _return, const std::string& session_id, const std::string& snapshot_set_id) {
    FUN_TRACE;
    macho::windows::com_init  com;
    macho::windows::auto_lock lock(_lock);
    _req.init();
    LONG deleted_snapshots;
    VSS_ID non_deleted_snapshot_id;
    _return.code = _req.delete_snapshot_set(macho::guid_(snapshot_set_id), deleted_snapshots, non_deleted_snapshot_id);
    _return.deleted_snapshots = deleted_snapshots;
    _return.non_deleted_snapshot_id = macho::guid_(non_deleted_snapshot_id);
}

void physical_packer_service_handler::get_all_snapshots(std::map<std::string, std::vector<snapshot> > & _return, const std::string& session_id) {
    FUN_TRACE;
    macho::windows::com_init  com;
    macho::windows::auto_lock lock(_lock);
    _req.init();
    irm_vss_snapshot_result::vtr snapshots = _req.query_snapshots();
    foreach(irm_vss_snapshot_result::ptr &r, snapshots){
        snapshot        s;
        s.creation_time_stamp = to_simple_string(r->creation_time_stamp);
        s.snapshots_count = r->snapshots_count;
        s.snapshot_id = r->snapshot_id;
        s.snapshot_set_id = r->snapshot_set_id;
        s.original_volume_name = stringutils::convert_unicode_to_ansi(r->original_volume_name);
        s.snapshot_device_object = stringutils::convert_unicode_to_ansi(r->snapshot_device_object);
        _return[s.snapshot_set_id].push_back(s);
        LOG(LOG_LEVEL_RECORD, _T("Snapshot : volume(%s), snapshot id(%s), snapshot set id(%s), created time(%s) "), r->original_volume_name.c_str(), std::wstring(r->snapshot_id).c_str(), std::wstring(r->snapshot_set_id).c_str(), to_simple_wstring(r->creation_time_stamp).c_str());
    }
}

void physical_packer_service_handler::send_vcbt_command(DWORD ioctl, std::vector<macho::guid_> &volumes, std::vector<macho::guid_>& snapshot_results){
    if (volumes.size()){
        macho::windows::auto_handle hDevice = CreateFileW(VCBT_WIN32_DEVICE_NAME,          // drive to open
            0,                // no access to the drive
            FILE_SHARE_READ | // share mode
            FILE_SHARE_WRITE,
            NULL,             // default security attributes
            OPEN_EXISTING,    // disposition
            0,                // file attributes
            NULL);            // do not copy file attributes      
        if (hDevice.is_invalid()){
            LOG(LOG_LEVEL_WARNING, L"VCBT driver is not ready.");
        }
        else{
            DWORD input_length = sizeof(VCBT_COMMAND_INPUT) + (sizeof(VCBT_COMMAND) * volumes.size());
            DWORD output_length = sizeof(VCBT_COMMAND_RESULT) + (sizeof(VCOMMAND_RESULT) * volumes.size());
            std::auto_ptr<VCBT_COMMAND_INPUT> command = std::auto_ptr<VCBT_COMMAND_INPUT>((PVCBT_COMMAND_INPUT)new BYTE[input_length]);
            std::auto_ptr<VCBT_COMMAND_RESULT> result = std::auto_ptr<VCBT_COMMAND_RESULT>((PVCBT_COMMAND_RESULT)new BYTE[output_length]);
            memset(command.get(), 0, input_length);
            memset(result.get(), 0, output_length);

            command->NumberOfCommands = volumes.size();
            for (int i = 0; i < volumes.size(); i++){
                command->Commands[i].VolumeId = volumes[i];
            }

            if (!(DeviceIoControl(hDevice,                        // device to be queried
                ioctl,                                            // operation to perform
                command.get(), input_length,                      // input buffer
                result.get(), output_length,                      // output buffer
                &output_length,                                   // # bytes returned
                (LPOVERLAPPED)NULL)))                             // synchronous I/O
            {
                LOG(LOG_LEVEL_ERROR, L"Failed to DeviceIoControl(%d), Error(0x%08x).", ioctl, GetLastError());
            }
            else{

                switch (ioctl){
                case IOCTL_VCBT_ENABLE:
                    LOG(LOG_LEVEL_INFO, L"Send command(ENABLE).");
                    break;
                case IOCTL_VCBT_DISABLE:
                    LOG(LOG_LEVEL_INFO, L"Send command(DISABLE).");
                    break;
                case IOCTL_VCBT_SNAPSHOT:
                    LOG(LOG_LEVEL_INFO, L"Send command(SNAPSHOT).");
                    break;
                case IOCTL_VCBT_POST_SNAPSHOT:
                    LOG(LOG_LEVEL_INFO, L"Send command(POST_SNAPSHOT).");
                    break;
                case IOCTL_VCBT_UNDO_SNAPSHOT:
                    LOG(LOG_LEVEL_INFO, L"Send command(UNDO_SNAPSHOT).");
                    break;
                case IOCTL_VCBT_IS_ENABLE:
                    LOG(LOG_LEVEL_INFO, L"Send command(IS_ENABLE).");
                    break;
                }

                for (int i = 0; i < result->NumberOfResults; i++){
                    LOG(LOG_LEVEL_INFO, L"[Volume ID] : %s", ((std::wstring)macho::guid_(result->Results[i].VolumeId)).c_str());
                    if (result->Results[i].Status >= 0){
                        switch (ioctl){
                        case IOCTL_VCBT_ENABLE:
                            LOG(LOG_LEVEL_INFO, L"Journal Id : %I64u", result->Results[i].JournalId);
                            LOG(LOG_LEVEL_INFO, L"Journal size : %d MBs", result->Results[i].Detail.JournalSizeInMegaBytes);
                            break;
                        case IOCTL_VCBT_DISABLE:
                            LOG(LOG_LEVEL_INFO, L"Done : %s", (result->Results[i].Detail.Done ? L"true" : L"false"));
                            break;
                        case IOCTL_VCBT_SNAPSHOT:
                            LOG(LOG_LEVEL_INFO, L"Journal Id : %I64u", result->Results[i].JournalId);
                            LOG(LOG_LEVEL_INFO, L"Done : %s", (result->Results[i].Detail.Done ? L"true" : L"false"));
                            snapshot_results.push_back(result->Results[i].VolumeId);
                            break;
                        case IOCTL_VCBT_POST_SNAPSHOT:
                            LOG(LOG_LEVEL_INFO, L"Journal Id : %I64u", result->Results[i].JournalId);
                            LOG(LOG_LEVEL_INFO, L"Done : %s", (result->Results[i].Detail.Done ? L"true" : L"false"));
                            break;
                        case IOCTL_VCBT_UNDO_SNAPSHOT:
                            LOG(LOG_LEVEL_INFO, L"Journal Id : %I64u", result->Results[i].JournalId);
                            LOG(LOG_LEVEL_INFO, L"Done : %s", (result->Results[i].Detail.Done ? L"true" : L"false"));
                            break;
                        case IOCTL_VCBT_IS_ENABLE:
                            LOG(LOG_LEVEL_INFO, L"Journal Id : %I64u", result->Results[i].JournalId);
                            LOG(LOG_LEVEL_INFO, L"Enable : %s", (result->Results[i].Detail.Done ? L"true" : L"false"));
                            break;
                        }
                    }
                    else{
                        LOG(LOG_LEVEL_ERROR, L"Command(%d) error: 0x%08x, Volume: %s", ioctl, result->Results[i].Status, ((std::wstring)macho::guid_(result->Results[i].VolumeId)).c_str());
                    }
                }
            }
        }
    }
}

bool physical_packer_service_handler::verify_carrier(const std::string& carrier, const bool is_ssl){
    bool result;
    LOG(LOG_LEVEL_RECORD, L"Carrier: %s", g_is_pooling_mode ? L"T" : L"F");
    if (g_is_pooling_mode){
        std::vector<int> ports = { 443, 18443 };
        foreach(int _port, ports){
            try{
                std::set<std::string> addrs;
				saasame::transport::service_info svc_info;
				mgmt_op_base<common_serviceClient> op({ carrier }, carrier, _port, true, saasame::transport::g_saasame_constants.CARRIER_SERVICE_PATH);
                if ( op.open() )
					op.client()->ping(svc_info);
                result = true;
                break;
            }
            catch (TException& ex) {
                LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
            }
            catch (...){
                LOG(LOG_LEVEL_ERROR, L"Unknown");
            }
        }
    }
    else{
        result = common_service_handler::verify_carrier(carrier, is_ssl);
    }
    return result;
}