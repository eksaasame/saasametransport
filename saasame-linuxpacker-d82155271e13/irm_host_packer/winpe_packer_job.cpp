#include "physical_packer_job.h"
#include "temp_drive_letter.h"
#include "carrier_rw.h"
#include "common_service_handler.h"

winpe_packer_job::winpe_packer_job(std::wstring id, saasame::transport::create_packer_job_detail detail) : physical_packer_job(id, detail){
    _mini_block_size = 8 * 1024 * 1024;
}

winpe_packer_job::winpe_packer_job(saasame::transport::create_packer_job_detail detail) : physical_packer_job(detail){
    _mini_block_size = 8 * 1024 * 1024;
}

bool  winpe_packer_job::verify_and_fix_snapshots(){

#if _DEBUG 
#else
    if (!macho::windows::environment::is_winpe()){
        record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_E_FAIL, (record_format("WinPE required.")));
        _state |= mgmt_job_state::job_state_error;
        return false;
    }
#endif
    return true;
}

bool winpe_packer_job::replicate_beginning_of_disk(universal_disk_rw::ptr &disk_rw, macho::windows::storage::disk& d, uint64_t& start_offset, uint64_t& backup_size, universal_disk_rw::vtr& outputs){
    if (d.partition_style() != storage::ST_PARTITION_STYLE::ST_PST_UNKNOWN){
        return physical_packer_job::replicate_beginning_of_disk(disk_rw, d, start_offset, backup_size, outputs);
    }
    return true;
}

bool winpe_packer_job::replicate_partitions_of_disk(universal_disk_rw::ptr &disk_rw, macho::windows::storage::disk& d, uint64_t& start_offset, uint64_t& backup_size, universal_disk_rw::vtr& outputs){
    FUN_TRACE;
    bool result = false;
    physical_vcbt_journal null_journal;
    bool is_replicated = true;
    uint64_t end_of_partiton = 0;
    uint64_t length = _block_size;
    if (disk_rw){
        if (d.partition_style() != storage::ST_PARTITION_STYLE::ST_PST_UNKNOWN){
            macho::windows::storage::partition::vtr parts = d.get_partitions();
            changed_area::vtr changed = get_epbrs(disk_rw, d, parts, start_offset);
            foreach(macho::windows::storage::partition::ptr p, parts){
                length = _block_size;
                if (!is_replicated)
                    break;
                if (start_offset < (uint64_t)p->offset()){
                    start_offset = (uint64_t)p->offset();
                }
                end_of_partiton = (uint64_t)p->offset() + (uint64_t)p->size();

                for (changed_area::vtr::iterator c = changed.begin(); c != changed.end();){
                    if (c->start_offset <= start_offset){
                        if (!(is_replicated = replicate(disk_rw, c->start_offset, c->length, outputs, 0)))
                            break;
                        c = changed.erase(c);
                    }
                    else
                        break;
                }
                if (!is_replicated)
                    break;

                if (start_offset < end_of_partiton && d.partition_style() == macho::windows::storage::ST_PARTITION_STYLE::ST_PST_GPT && (macho::guid_(p->gpt_type()) == macho::guid_(PARTITION_SYSTEM_GUID))){
                    temp_drive_letter::ptr temp;
                    std::wstring drive_letter;
                    if (!p->access_paths().size()){
                        temp = temp_drive_letter::assign(p->disk_number(), p->partition_number(), false);
                        if (temp)
                            drive_letter = temp->drive_letter();
                    }
                    else{
                        drive_letter = p->access_paths()[0];
                    }

                    if (drive_letter.empty()){
                        changed_area::vtr  changeds = get_changed_areas(disk_rw, d, (*p.get()), start_offset);
                        if (!(is_replicated = replicate_changed_areas(disk_rw, d, start_offset, 0, backup_size, changeds, outputs)))
                            break;
                    }
                    else{
                        if (!(is_replicated = replicate_changed_areas(d, *p, drive_letter, start_offset, backup_size, outputs, null_journal, false)))
                            break;
                    }
                    if (is_replicated && (!_is_interrupted))
                        start_offset = end_of_partiton;
                }else if (!p->access_paths().size() && start_offset < end_of_partiton){
                    if (d.partition_style() == macho::windows::storage::ST_PARTITION_STYLE::ST_PST_MBR &&
                        (p->mbr_type() == PARTITION_XINT13_EXTENDED || p->mbr_type() == PARTITION_EXTENDED)){
                        is_replicated = replicate(disk_rw, start_offset, 1024 * 1024, outputs);
                    }
                    else{
                        changed_area::vtr  changeds = get_changed_areas(disk_rw, d, (*p.get()), start_offset);
                        if (!(is_replicated = replicate_changed_areas(disk_rw, d, start_offset, 0, backup_size, changeds, outputs)))
                            break;
                        if (is_replicated && (!_is_interrupted))
                            start_offset = end_of_partiton;
                    }                   
                }
                else if (start_offset >= (uint64_t)p->offset() && start_offset < end_of_partiton){
                    std::vector<std::wstring> access_paths = p->access_paths();
                    macho::windows::storage::volume::ptr v = get_volume_info(p->access_paths(), d.get_volumes());
                    if (!v){
                        changed_area::vtr  changeds = get_changed_areas(disk_rw, d, (*p.get()), start_offset);
                        if (!(is_replicated = replicate_changed_areas(disk_rw, d, start_offset, 0, backup_size, changeds, outputs)))
                            break;
                        if (is_replicated && (!_is_interrupted))
                            start_offset = end_of_partiton;
                    }
                    else{
                        std::wstring volume_path;
                        foreach(std::wstring volume_name, access_paths){
                            if (volume_name.length() < 5 ||
                                volume_name[0] != L'\\' ||
                                volume_name[1] != L'\\' ||
                                volume_name[2] != L'?' ||
                                volume_name[3] != L'\\' ||
                                volume_name[volume_name.length() - 1] != L'\\'){
                            }
                            else{
                                volume_path = volume_name;
                                break;
                            }
                        }

                        if (!(is_replicated = replicate_changed_areas(d, *p, volume_path, start_offset, backup_size, outputs, null_journal, false)))
                            break;
                    }
                    if (is_replicated && (!_is_interrupted))
                        start_offset = end_of_partiton;
                }
                save_status();
            }
        }
        else{
            changed_area::vtr  changeds = get_changed_areas(disk_rw, d, start_offset);
            is_replicated = replicate_changed_areas(disk_rw, d, start_offset, 0, backup_size, changeds, outputs);
        }
        result = is_replicated && (!_is_interrupted);
    }
    return result;
}

changed_area::vtr       winpe_packer_job::get_changed_areas(universal_disk_rw::ptr &disk_rw, macho::windows::storage::disk& d, macho::windows::storage::partition& p, const uint64_t& start_offset){
    ULONG             full_mini_block_size = 8 * 1024 * 1024;
    changed_area::vtr changes;
    if (d.partition_style() == storage::ST_PARTITION_STYLE::ST_PST_GPT){
        macho::guid_ partition_type = macho::guid_(p.gpt_type());
        if (partition_type == macho::guid_("0657FD6D-A4AB-43C4-84E5-0933C84B4F4F")){}// Linux SWAP partition
#if 0
        else if (partition_type == macho::guid_("0FC63DAF-8483-4772-8E79-3D69D8477DE4") || // "GNU/Linux filesystem data"
            partition_type == macho::guid_("933AC7E1-2EB4-4F13-B844-0E14E2AEF915") || //"GNU/Linux /home"
            partition_type == macho::guid_("EBD0A0A2-B9E5-4433-87C0-68B6B72699C7") // MS basic data partition 
            )
        {
            linuxfs::volume::ptr v = linuxfs::volume::get(disk_rw, p.offset(), p.size());
            if (v){
                linuxfs::io_range::vtr ranges = v->file_system_ranges();
                foreach(linuxfs::io_range &r, ranges){
                    uint64_t start = r.start;
                    uint64_t end = r.start + r.length;
                    while (end > start_offset && start < end){
                        uint64_t length = end - start;
                        if (length > full_mini_block_size)
                            length = full_mini_block_size;
                        changed_area changed(start, length);
                        changes.push_back(changed);
                        start += length;
                    }
                }
            }
            else{
                changes = physical_packer_job::get_changed_areas(p, start_offset);
            }
        }
        else{
            changes = physical_packer_job::get_changed_areas(p, start_offset);
        }
#else
        else{
            changed_area::vtr ranges = _get_changed_areas(disk_rw->path(), start_offset, p.offset() + p.size());
            if (ranges.size()){
                foreach(changed_area &r, ranges){
                    uint64_t start = r.start_offset;
                    uint64_t end = r.start_offset + r.length;
                    while (end > start_offset && start < end){
                        uint64_t length = end - start;
                        if (length > full_mini_block_size)
                            length = full_mini_block_size;
                        changed_area changed(start, length);
                        changes.push_back(changed);
                        start += length;
                    }
                }
            }
            else
                changes = physical_packer_job::get_changed_areas(p, start_offset);
        }
#endif
    }
    else{
        if (p.mbr_type() == PARTITION_SWAP){}// Linux SWAP partition
#if 0
        else if (p.mbr_type() == PARTITION_EXT2){
            linuxfs::volume::ptr v = linuxfs::volume::get(disk_rw, p.offset(), p.size());
            if (v){
                linuxfs::io_range::vtr ranges = v->file_system_ranges();
                foreach(linuxfs::io_range &r, ranges){
                    uint64_t start = r.start;
                    uint64_t end = r.start + r.length;
                    while (end > start_offset && start < end){
                        uint64_t length = end - start;
                        if (length > full_mini_block_size)
                            length = full_mini_block_size;
                        changed_area changed(start, length);
                        changes.push_back(changed);
                        start += length;
                    }
                }
            }
            else{
                changes = physical_packer_job::get_changed_areas(p, start_offset);
            }
        }
        else{
            changes = physical_packer_job::get_changed_areas(p, start_offset);
        }
#else
        else{
            changed_area::vtr ranges = _get_changed_areas(disk_rw->path(), start_offset, p.offset() + p.size());
            if (ranges.size()){
                foreach(changed_area &r, ranges){
                    uint64_t start = r.start_offset;
                    uint64_t end = r.start_offset + r.length;
                    while (end > start_offset && start < end){
                        uint64_t length = end - start;
                        if (length > full_mini_block_size)
                            length = full_mini_block_size;
                        changed_area changed(start, length);
                        changes.push_back(changed);
                        start += length;
                    }
                }
            }
            else
                changes = physical_packer_job::get_changed_areas(p, start_offset);
        }
#endif
    }
    return changes;
}

changed_area::vtr       winpe_packer_job::get_changed_areas(universal_disk_rw::ptr &disk_rw, macho::windows::storage::disk& d, const uint64_t& start_offset){
    changed_area::vtr changes;
    ULONG    full_mini_block_size = 8 * 1024 * 1024;
    linuxfs::volume::ptr v = linuxfs::volume::get(disk_rw, 0, d.size());
    if (v){
        linuxfs::io_range::vtr ranges = v->file_system_ranges();
        foreach(linuxfs::io_range &r, ranges){
            uint64_t start = r.start;
            uint64_t end = r.start + r.length;
            while (end > start_offset && start < end){
                uint64_t length = end - start;
                if (length > full_mini_block_size)
                    length = full_mini_block_size;
                changed_area changed(start, length);
                changes.push_back(changed);
                start += length;
            }
        }
    }
    else{
        uint64_t start = start_offset;
        uint64_t disk_end = d.size();
        while (start < disk_end ){
            uint64_t length = disk_end - start;
            if (length > full_mini_block_size)
                length = full_mini_block_size;
            changed_area changed(start, length);
            changes.push_back(changed);
            start += length;
        }
    }
    return changes;
}

void winpe_packer_job::execute(){
    FUN_TRACE;
    if ((_state & mgmt_job_state::job_state_finished) || _is_canceled || (_state & mgmt_job_state::job_state_error) || _is_removing){
        return;
    }
    macho::windows::environment::set_token_privilege(L"SE_MANAGE_VOLUME_NAME", true);
    _state &= ~mgmt_job_state::job_state_error;
    if (!_is_interrupted && verify_and_fix_snapshots()){
        _state = mgmt_job_state::job_state_replicating;
        save_status();
        bool result = false;

        registry reg;
        if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
            if (reg[L"MaxBlockSize"].exists()){
                _block_size = (DWORD)reg[L"MaxBlockSize"];
            }
            if (reg[L"QueueNumber"].exists() && (DWORD)reg[L"QueueNumber"] > 0){
                _queue_size = (DWORD)reg[L"QueueNumber"] * CARRIER_RW_IMAGE_BLOCK_SIZE;
            }
        }

        if (_block_size < 1024 * 1024)
            _block_size = 1024 * 1024;

        try{
            macho::windows::com_init  com;
            string_array volumes;
            std::vector<universal_disk_rw::ptr> rws;
            macho::windows::storage::ptr stg = macho::windows::storage::get();
            macho::windows::storage::disk::vtr _disks = stg->get_disks();
            foreach(std::string disk, _create_job_detail.detail.p.disks){
                disk_universal_identify uri(disk);
                foreach(macho::windows::storage::disk::ptr d, _disks){
                    if (disk_universal_identify(*d) == uri){
                        physical_packer_job_progress::ptr p;
                        uint64_t offset = 0;
                        if (d->is_boot())
                            _boot_disk = disk;
                        if (_progress.count(disk)){
                            p = _progress[disk];
                        }
                        else{
                            p = physical_packer_job_progress::ptr(new physical_packer_job_progress());
                            p->uri = disk_universal_identify(*d);
                            p->total_size = d->size();
                            if (_create_job_detail.detail.p.backup_size.count(disk))
                                p->backup_size = _create_job_detail.detail.p.backup_size[disk];
                            if (_create_job_detail.detail.p.backup_progress.count(disk))
                                p->backup_progress = _create_job_detail.detail.p.backup_progress[disk];
                            if (_create_job_detail.detail.p.backup_image_offset.count(disk))
                                p->backup_image_offset = _create_job_detail.detail.p.backup_image_offset[disk];
                            _progress[disk] = p;
                        }
                        if (!(result = get_output_image_info(disk, p))){
                            _state |= mgmt_job_state::job_state_error;
                            break;
                        }
                        else{
                            std::wstring path = boost::str(boost::wformat(L"\\\\.\\PhysicalDrive%d") % (*d).number());
                            universal_disk_rw::ptr rw = general_io_rw::open(path);
                            if (rw){
                                rws.push_back(rw);
                            }
                        }
                    }
                }
                if (!result)
                    break;
            }

            if (result){
                if (_create_job_detail.file_system_filter_enable){
                    _io_ranges_map = linuxfs::volume::get_file_system_ranges(rws);
                }
                rws.clear();
                foreach(std::string disk, _create_job_detail.detail.p.disks){
                    disk_universal_identify uri(disk);
                    foreach(macho::windows::storage::disk::ptr d, _disks){
                        if (disk_universal_identify(*d) == uri){
                            physical_packer_job_progress::ptr p = _progress[disk];
                            uint64_t offset = 0;
                            if (p->backup_size == 0 && !(result = calculate_disk_backup_size(*d, offset, p->backup_size)))
                                break;
                        }
                    }
                    if (!result)
                        break;
                }
            }

            if (result){
                foreach(std::string disk, _create_job_detail.detail.p.disks){
                    disk_universal_identify uri(disk);
                    foreach(macho::windows::storage::disk::ptr d, _disks){
                        if (disk_universal_identify(*d) == uri){
                            physical_packer_job_progress::ptr p = _progress[disk];
                            if (!(result = replicate_disk(*d, p->output, p->base, p->backup_image_offset, p->backup_progress, p->parent)))
                                break;
                        }
                    }
                    if (!result)
                        break;
                }
            }
            _changed_areas.clear();
            if (result && _create_job_detail.detail.p.disks.size() == _progress.size())
                _state = mgmt_job_state::job_state_finished;
        }
        catch (macho::exception_base& ex)
        {
            LOG(LOG_LEVEL_ERROR, L"%s", macho::get_diagnostic_information(ex).c_str());
            record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_E_INITIAL_FAIL, record_format(macho::stringutils::convert_unicode_to_utf8(macho::get_diagnostic_information(ex))));
            result = false;
            _state |= mgmt_job_state::job_state_error;
        }
        catch (const boost::filesystem::filesystem_error& ex)
        {
            LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
            record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_E_INITIAL_FAIL, record_format(ex.what()));
            result = false;
            _state |= mgmt_job_state::job_state_error;
        }
        catch (const boost::exception &ex)
        {
            LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_ansi_to_unicode(boost::exception_detail::get_diagnostic_information(ex, "error:")).c_str());
            record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_E_INITIAL_FAIL, record_format(boost::exception_detail::get_diagnostic_information(ex, "error:")));
            result = false;
            _state |= mgmt_job_state::job_state_error;
        }
        catch (const std::exception& ex)
        {
            LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
            record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_E_INITIAL_FAIL, record_format(ex.what()));
            result = false;
            _state |= mgmt_job_state::job_state_error;
        }
        catch (...)
        {
            LOG(LOG_LEVEL_ERROR, L"Unknown exception");
            record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_E_INITIAL_FAIL, record_format("Unknown exception"));
            result = false;
            _state |= mgmt_job_state::job_state_error;
        }
    }
    save_status();
}

changed_area::vtr       winpe_packer_job::_get_changed_areas(const std::wstring path, const uint64_t start, const uint64_t end){
    changed_area::vtr changes;
    if (_io_ranges_map.count(path)){
        foreach(linuxfs::io_range& r, _io_ranges_map[path]){
            //uint64_t r_end = r.start + r.length;
            if (r.start >= start && r.start < end){
                changes.push_back(changed_area(r.start, r.length));
            }
            else if (r.start > end)
                break;
        }
    }
    return changes;
}