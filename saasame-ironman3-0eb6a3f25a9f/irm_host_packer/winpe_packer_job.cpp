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
        record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_E_FAIL, (record_format("Failed to initiate replication. Packer type mismatched.")));
        _state |= mgmt_job_state::job_state_error;
        return false;
    }
#endif
    return true;
}

replication_block::map   winpe_packer_job::get_replication_blocks(const std::map<std::string, storage::disk::ptr>& disks){
    replication_block::map results;
    physical_vcbt_journal null_journal;
    std::vector<universal_disk_rw::ptr> rws;
    std::map<std::wstring, std::string > disks_ids_map;
    std::map<std::string, std::wstring > disk_rws_path_map;
    std::map<std::wstring, bool>         processed_dynamic_volumes;
    macho::guid_ efi_sys("c12a7328-f81f-11d2-ba4b-00a0c93ec93b"); // EFI System Partition
    macho::guid_ bios_boot("21686148-6449-6E6F-744E-656564454649"); // BIOS Boot Partition

    typedef std::map<std::string, storage::disk::ptr> disk_key_map;
    foreach(disk_key_map::value_type _d, disks){
        std::wstring path = boost::str(boost::wformat(L"\\\\.\\PhysicalDrive%d") % _d.second->number());
        disks_ids_map[path] = _d.first;
        disk_rws_path_map[_d.first] = path;
        universal_disk_rw::ptr rw = general_io_rw::open(path);
        if (rw){
            rws.push_back(rw);
        }
    }
    if (_create_job_detail.file_system_filter_enable){
        _io_ranges_map = linuxfs::volume::get_file_system_ranges(rws);
    }
    _lvm_groups_devices_map = linuxfs::lvm_mgmt::get_groups(rws);
    rws.clear();
    foreach(disk_key_map::value_type _d, disks){
        bool is_bootable_disk = false;
        storage::disk::ptr d = _d.second;
        macho::windows::storage::partition::vtr partitions = d->get_partitions();
        foreach(macho::windows::storage::partition::ptr p, partitions){
            if (d->partition_style() == macho::windows::storage::ST_PARTITION_STYLE::ST_PST_MBR){
                is_bootable_disk = p->is_active();
                break;
            }
            else if (d->partition_style() == macho::windows::storage::ST_PARTITION_STYLE::ST_PST_GPT){
                macho::guid_ g(p->gpt_type());
                if (g == efi_sys || g == bios_boot){
                    is_bootable_disk = true;
                    break;
                }
            }
        }
        if (is_bootable_disk){
            std::wstring path = disk_rws_path_map[_d.first];;
            if (_boot_disk.empty())
                _boot_disk = _d.first;
            _system_disks.insert(_d.first);
            foreach(linuxfs::lvm_mgmt::groups_map::value_type& g, _lvm_groups_devices_map){
                bool found = false;
                foreach(std::wstring p, g.second){
                    if (path == p){
                        found = true;
                        break;
                    }
                }
                if (found){
                    foreach(std::wstring p, g.second){
                        _system_disks.insert(disks_ids_map[p]);
                    }
                }
            }

            if (_create_job_detail.is_only_single_system_disk && _system_disks.size() > 1){
                LOG(LOG_LEVEL_ERROR, L"Multiple system disks found. Target only supports a single system disk.");
                record((job_state::type)(_state & ~mgmt_job_state::job_state_error),
                    saasame::transport::error_codes::SAASAME_E_INTERNAL_FAIL,
                    record_format("Multiple system disks found. Target only supports a single system disk."));
                _state |= mgmt_job_state::job_state_error;
            }
        }
    }

    foreach(disk_key_map::value_type _d, disks){
        if (_progress.count(_d.first) && _progress[_d.first]->is_completed())
            continue;
        storage::disk::ptr d = _d.second;
        replication_block::vtr& _result = results[d->number()];
        uint64_t end_of_partiton = 0, backup_image_offset = 0;
        if (_progress.count(_d.first))
            backup_image_offset = _progress[_d.first]->backup_image_offset;
        universal_disk_rw::ptr rw = general_io_rw::open(boost::str(boost::wformat(L"\\\\.\\PhysicalDrive%d") % d->number()));
        if (!rw){
            record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_E_IMAGE_WRITE, (record_format("Cannot calculate backup size of disk %1%.") % d->number()));
        }
        else{
            if (!_progress[_d.first]->backup_size)
                record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_S_OK, (record_format("Calculating backup size of disk %1%.") % d->number()));
            if (d->partition_style() != storage::ST_PARTITION_STYLE::ST_PST_UNKNOWN){
                macho::windows::storage::partition::vtr parts = d->get_partitions();
                if (0 == parts.size()){
                    _result.push_back(replication_block::ptr(new replication_block(0, 0, 64 * 1024, 0)));
                }
                else{
                    foreach(macho::windows::storage::partition::ptr p, parts){
                        if (1 == p->partition_number()){
                            _result.push_back(replication_block::ptr(new replication_block(0, 0, p->offset() < _block_size ? p->offset() : _block_size, 0)));
                            break;
                        }
                    }
                    replication_block::vtr changeds = get_epbrs(rw, *d, parts);
                    _result.insert(_result.end(), changeds.begin(), changeds.end());

                    foreach(macho::windows::storage::partition::ptr p, parts){
                        end_of_partiton = p->offset() + p->size();
                        if (backup_image_offset >= end_of_partiton){
                        }
                        else if ((d->partition_style() == macho::windows::storage::ST_PARTITION_STYLE::ST_PST_MBR && p->is_active() && p->access_paths().empty() && p->mbr_type() == 0x27) ||
                            (d->partition_style() == macho::windows::storage::ST_PARTITION_STYLE::ST_PST_GPT && (macho::guid_(p->gpt_type()) == macho::guid_(PARTITION_SYSTEM_GUID)))){
                            temp_drive_letter::ptr temp;
                            std::wstring drive_letter;
                            if (p->access_paths().empty()){
                                temp = temp_drive_letter::assign(p->disk_number(), p->partition_number(), false);
                                if (temp)
                                    drive_letter = temp->drive_letter();
                            }
                            else{
                                drive_letter = p->access_paths()[0];
                            }
                            replication_block::vtr  changeds = drive_letter.empty() ? get_replication_blocks(rw, *d, (*p.get()), backup_image_offset) : physical_packer_job::get_replication_blocks(*p, macho::stringutils::convert_unicode_to_utf8(drive_letter), backup_image_offset, null_journal);
                            if (p->access_paths().empty()){
                                foreach(replication_block::ptr c, changeds){
                                    c->start = c->offset;
                                    c->path = NULL;
                                }
                            }
                            _result.insert(_result.end(), changeds.begin(), changeds.end());
                        }
                        else if (d->partition_style() == macho::windows::storage::ST_PARTITION_STYLE::ST_PST_MBR &&
                            (p->mbr_type() == PARTITION_XINT13_EXTENDED || p->mbr_type() == PARTITION_EXTENDED)){
                        }
                        else if (d->partition_style() == macho::windows::storage::ST_PARTITION_STYLE::ST_PST_GPT && (macho::guid_(p->gpt_type()) == macho::guid_(PARTITION_LDM_METADATA_GUID))){
                            replication_block::vtr  changeds = get_partition_replication_blocks((*p.get()), backup_image_offset);
                            _result.insert(_result.end(), changeds.begin(), changeds.end());
                        }
                        else if ((d->partition_style() == macho::windows::storage::ST_PARTITION_STYLE::ST_PST_MBR && p->access_paths().empty() && p->mbr_type() == 0x42) ||
                            (d->partition_style() == macho::windows::storage::ST_PARTITION_STYLE::ST_PST_GPT && (macho::guid_(p->gpt_type()) == macho::guid_(PARTITION_LDM_METADATA_GUID)))){
                        }
                        else if (p->access_paths().empty()){
                            replication_block::vtr   changeds = get_replication_blocks(rw, *d, (*p.get()), backup_image_offset);
                            _result.insert(_result.end(), changeds.begin(), changeds.end());
                        }
                        else if ((!d->is_dynmaic())){
                            macho::windows::storage::volume::ptr v = get_volume_info(p->access_paths(), d->get_volumes());
                            if (!v){
                                replication_block::vtr   changeds = get_replication_blocks(rw, *d, (*p.get()), backup_image_offset);
                                _result.insert(_result.end(), changeds.begin(), changeds.end());
                            }
                            else{ // Replicate the boot partition.
                                std::vector<std::wstring> access_paths = p->access_paths();
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
                                replication_block::vtr  changeds = physical_packer_job::get_replication_blocks(*p, macho::stringutils::convert_unicode_to_utf8(volume_path), backup_image_offset, null_journal);
                                _result.insert(_result.end(), changeds.begin(), changeds.end());
                            }
                        }
                    }
                }
                if (d->is_dynmaic()){
                    foreach(storage::volume::ptr v, d->get_volumes()){
                        if (v->file_system() == L"NTFS" || v->file_system() == L"ReFS" || v->file_system() == L"CSVFS"){
                            if (!processed_dynamic_volumes.count(v->path())){
                                foreach(std::wstring path, v->access_paths()){
                                    if ((path[0] == _T('\\')) &&
                                        (path[1] == _T('\\')) &&
                                        (path[2] == _T('?')) &&
                                        (path[3] == _T('\\'))){
                                        processed_dynamic_volumes[v->path()] = true;
                                        replication_block::map changeds = physical_packer_job::get_replication_blocks(*v, macho::stringutils::convert_unicode_to_utf8(path), null_journal, std::set<std::wstring>(), std::set<std::wstring>());
                                        foreach(replication_block::map::value_type& c, changeds){
                                            results[c.first].insert(results[c.first].end(), c.second.begin(), c.second.end());
                                        }
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    if (d->partition_style() == macho::windows::storage::ST_PARTITION_STYLE::ST_PST_MBR)
                        _result.push_back(replication_block::ptr(new replication_block(0, d->size() - (1024 * 1024), (1024 * 1024), d->size() - (1024 * 1024))));
                }
            }
            else{
                _result = get_replication_blocks(rw, *d, backup_image_offset);
            }
            calculate_backup_size(*d, *_progress[_d.first], _result);
        }
    }
    _io_ranges_map.clear();
    _lvm_groups_devices_map.clear();
    return results;
}

replication_block::vtr winpe_packer_job::get_replication_blocks(universal_disk_rw::ptr &disk_rw, macho::windows::storage::disk& d, macho::windows::storage::partition& p, const uint64_t& start_offset){
    ULONG             full_mini_block_size = 8 * 1024 * 1024;
    replication_block::vtr changes;
    if (d.partition_style() == storage::ST_PARTITION_STYLE::ST_PST_GPT){
        macho::guid_ partition_type = macho::guid_(p.gpt_type());
        if (partition_type == macho::guid_("0657FD6D-A4AB-43C4-84E5-0933C84B4F4F")){
            _guest_os_type = saasame::transport::hv_guest_os_type::type::HV_OS_LINUX;
        }
        else{
            replication_block::vtr ranges = _get_replication_blocks(disk_rw->path(), start_offset, p.offset() + p.size());
            if (ranges.size()){
                _guest_os_type = saasame::transport::hv_guest_os_type::type::HV_OS_LINUX;
                foreach(replication_block::ptr &r, ranges){
                    uint64_t start = r->start;
                    uint64_t end = r->start + r->length;
                    while (end > start_offset && start < end){
                        uint64_t length = end - start;
                        if (length > full_mini_block_size)
                            length = full_mini_block_size;
                        changes.push_back(replication_block::ptr(new replication_block(0, start, length, start)));
                        start += length;
                    }
                }
            }
            else{
                changes = physical_packer_job::get_partition_replication_blocks(p, start_offset);
            }
        }
    }
    else{
        if (p.mbr_type() == PARTITION_SWAP){
            _guest_os_type = saasame::transport::hv_guest_os_type::type::HV_OS_LINUX;
        }// Linux SWAP partition
        else{
            replication_block::vtr ranges = _get_replication_blocks(disk_rw->path(), start_offset, p.offset() + p.size());
            if (ranges.size()){
                _guest_os_type = saasame::transport::hv_guest_os_type::type::HV_OS_LINUX;
                foreach(replication_block::ptr &r, ranges){
                    uint64_t start = r->start;
                    uint64_t end = r->start + r->length;
                    while (end > start_offset && start < end){
                        uint64_t length = end - start;
                        if (length > full_mini_block_size)
                            length = full_mini_block_size;
                        changes.push_back(replication_block::ptr(new replication_block(0, start, length, start)));
                        start += length;
                    }
                }
            }
            else{
                changes = physical_packer_job::get_partition_replication_blocks(p, start_offset);
            }
        }
    }
    return changes;
}

replication_block::vtr winpe_packer_job::_get_replication_blocks(const std::wstring path, const uint64_t start, const uint64_t end){
    replication_block::vtr changes;
    if (_io_ranges_map.count(path)){
        foreach(io_range& r, _io_ranges_map[path]){
            //uint64_t r_end = r.start + r.length;
            if (r.start >= start && r.start < end){
                changes.push_back(replication_block::ptr(new replication_block(0, r.start, r.length, r.start)));
            }
            else if (r.start > end)
                break;
        }
    }
    return changes;
}

replication_block::vtr  winpe_packer_job::get_replication_blocks(universal_disk_rw::ptr &disk_rw, macho::windows::storage::disk& d, const uint64_t& start_offset){
    replication_block::vtr changes;
    ULONG    full_mini_block_size = 8 * 1024 * 1024;
    linuxfs::volume::ptr v = linuxfs::volume::get(disk_rw, 0, d.size());
    if (v){
        io_range::vtr ranges = v->file_system_ranges();
        foreach(io_range &r, ranges){
            uint64_t start = r.start;
            uint64_t end = r.start + r.length;
            while (end > start_offset && start < end){
                uint64_t length = end - start;
                if (length > full_mini_block_size)
                    length = full_mini_block_size;
                changes.push_back(replication_block::ptr(new replication_block(0, start, length, start)));
                start += length;
            }
        }
    }
    else{
        uint64_t start = start_offset;
        uint64_t disk_end = d.size();
        while (start < disk_end){
            uint64_t length = disk_end - start;
            if (length > full_mini_block_size)
                length = full_mini_block_size;
            changes.push_back(replication_block::ptr(new replication_block(0, start, length, start)));
            start += length;
        }
    }
    return changes;
}
