#include "macho.h"
#include "mirror_disk.h"
#include "ntfs_parser.h"
#include <ntddvol.h>
#include "..\gen-cpp\temp_drive_letter.h"
#include "..\vcbt\vcbt\fatlbr.h"
#include <VersionHelpers.h>

using namespace macho;
using namespace macho::windows;
using namespace ntfs;

bool mirror_disk::replicate_changed_areas(universal_disk_rw::ptr &rw, macho::windows::storage::disk& d, uint64_t& start_offset, uint64_t offset, uint64_t& backup_size, changed_area::vtr &changeds, universal_disk_rw::ptr& output){
    bool result = false;
    foreach(changed_area &c, changeds){
        if (result = replicate(rw, c.start, c.length, output, offset)){
            if (output && _backup_size){
                start_offset = c.start + c.length + offset;
                backup_size += c.length;
                int barWidth = 68;
                double progress = ((double)backup_size) / _backup_size;
                std::cout << "[";
                int pos = barWidth * progress;
                for (int i = 0; i < barWidth; ++i) {
                    if (i < pos) std::cout << "=";
                    else if (i == pos) std::cout << ">";
                    else std::cout << " ";
                }
                std::cout << "] " << std::fixed << std::setw(6) << std::setprecision(2) << float(progress * 100.0) << "%\r";
                std::cout.flush();
            }
        }
    }
    return result;
}

changed_area::vtr       mirror_disk::get_changed_areas(macho::windows::storage::partition& p, const uint64_t& start_offset){
    ULONG             full_mini_block_size = 8 * 1024 * 1024;
    changed_area::vtr changes;
    uint64_t start = start_offset;
    uint64_t partition_end = p.offset() + p.size();
    while (start < partition_end){
        uint64_t length = partition_end - start;
        if (length > full_mini_block_size)
            length = full_mini_block_size;
        changed_area changed(start, length);
        changes.push_back(changed);
        start += length;
    }
    return changes;
}

changed_area::vtr       mirror_disk::get_changed_areas(universal_disk_rw::ptr &disk_rw, macho::windows::storage::disk& d, macho::windows::storage::partition& p, const uint64_t& start_offset){
    ULONG             full_mini_block_size = 8 * 1024 * 1024;
    changed_area::vtr changes;
    if (d.partition_style() == storage::ST_PARTITION_STYLE::ST_PST_GPT){
        macho::guid_ partition_type = macho::guid_(p.gpt_type());
        if (partition_type == macho::guid_("0657FD6D-A4AB-43C4-84E5-0933C84B4F4F")){}// Linux SWAP partition
        else{
            changes = get_changed_areas(p, start_offset);
        }
    }
    else{
        if (p.mbr_type() == PARTITION_SWAP){}// Linux SWAP partition
        else{
            changes = get_changed_areas(p, start_offset);
        }
    }
    return changes;
}

macho::windows::storage::volume::ptr  mirror_disk::get_volume_info(macho::string_array_w access_paths, macho::windows::storage::volume::vtr volumes){
    FUN_TRACE;
    foreach(std::wstring access_path, access_paths){
        foreach(macho::windows::storage::volume::ptr _v, volumes){
            foreach(std::wstring a, _v->access_paths()){
                if (a.length() >= access_path.length() && wcsstr(a.c_str(), access_path.c_str()) != NULL){
                    return _v;
                }
                else if (a.length() < access_path.length() && wcsstr(access_path.c_str(), a.c_str()) != NULL){
                    return _v;
                }
            }
        }
    }
    return NULL;
}

bool mirror_disk::exclude_file(LPBYTE buff, boost::filesystem::path file){
    bool result = false;
    if (boost::filesystem::exists(file)){
        LOG(LOG_LEVEL_INFO, L"Try to exclude the data range for the file (%s).", file.wstring().c_str());
        macho::windows::auto_handle handle = CreateFileA(file.string().c_str(),
            GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);
        if (handle.is_invalid()){
            LOG(LOG_LEVEL_ERROR, L"CreateFile (%s) failed. error: %d", file.wstring().c_str(), GetLastError());
            return result;
        }
        else{
            STARTING_VCN_INPUT_BUFFER starting;
            std::auto_ptr<RETRIEVAL_POINTERS_BUFFER> pVcnPairs;
            starting.StartingVcn.QuadPart = 0;
            ULONG ulOutPutSize = 0;
            ULONG uCounts = 200;
            ulOutPutSize = sizeof(RETRIEVAL_POINTERS_BUFFER) + uCounts * sizeof(LARGE_INTEGER) * 2 + sizeof(LARGE_INTEGER);
            pVcnPairs = std::auto_ptr<RETRIEVAL_POINTERS_BUFFER>((PRETRIEVAL_POINTERS_BUFFER)new BYTE[ulOutPutSize]);
            if (NULL == pVcnPairs.get()){
                return result;
            }
            else{
                DWORD BytesReturned = 0;
                while (!DeviceIoControl(handle, FSCTL_GET_RETRIEVAL_POINTERS, &starting, sizeof(STARTING_VCN_INPUT_BUFFER), pVcnPairs.get(), ulOutPutSize, &BytesReturned, NULL)){
                    DWORD err = GetLastError();
                    if (ERROR_MORE_DATA == err || ERROR_BUFFER_OVERFLOW == err){
                        uCounts += 200;
                        ulOutPutSize = sizeof(RETRIEVAL_POINTERS_BUFFER) + uCounts * sizeof(LARGE_INTEGER) * 2 + sizeof(LARGE_INTEGER);
                        pVcnPairs = std::auto_ptr<RETRIEVAL_POINTERS_BUFFER>((PRETRIEVAL_POINTERS_BUFFER)new BYTE[ulOutPutSize]);
                        if (pVcnPairs.get() == NULL)
                            return result;
                    }
                    else{
                        LOG(LOG_LEVEL_ERROR, L"DeviceIoControl(FSCTL_GET_RETRIEVAL_POINTERS) failed. Error: %d", err);
                        return result;
                    }
                }
                LARGE_INTEGER liVcnPrev = pVcnPairs->StartingVcn;
                for (DWORD extent = 0; extent < pVcnPairs->ExtentCount; extent++){
                    LONG      _Length = (ULONG)(pVcnPairs->Extents[extent].NextVcn.QuadPart - liVcnPrev.QuadPart);
                    clear_umap(buff, pVcnPairs->Extents[extent].Lcn.QuadPart, _Length, 1);
                    liVcnPrev = pVcnPairs->Extents[extent].NextVcn;
                }
                LOG(LOG_LEVEL_RECORD, L"Excluded the data replication for the file (%s).", file.wstring().c_str());
                result = true;
            }
        }
    }
    return result;
}

changed_area::vtr mirror_disk::_get_changed_areas(ULONGLONG file_area_offset, LPBYTE buff, uint64_t start_lcn, uint64_t total_number_of_bits, uint32_t bytes_per_cluster, uint64_t& total_number_of_delta){
    changed_area::vtr results;
    total_number_of_delta = 0;
    uint32_t mini_number_of_bits = _mini_block_size / bytes_per_cluster;
    uint32_t number_of_bits = _block_size / bytes_per_cluster;
    if (file_area_offset){
        results.push_back(changed_area(0, file_area_offset));
        total_number_of_delta += file_area_offset;
    }

#ifdef _DEBUG
    LOG(LOG_LEVEL_RECORD, L"file_area_offset: %I64u, mini_block_size: %d, bytes_per_cluster: %d, block_size: %d", file_area_offset, _mini_block_size, bytes_per_cluster, _block_size);
#endif
    uint64_t newBit, oldBit = 0, next_dirty_bit = 0;
    bool     dirty_range = false;
    for (newBit = 0; newBit < (uint64_t)total_number_of_bits; newBit++){
        if (buff[newBit >> 3] & (1 << (newBit & 7))){
            uint64_t num = (newBit - oldBit);
            if (!dirty_range){
                dirty_range = true;
                oldBit = next_dirty_bit = newBit;
            }
            else if (num == number_of_bits){
                total_number_of_delta += (num * bytes_per_cluster);
                //#ifdef _DEBUG
                //                changed_area c(file_area_offset + ((start_lcn + oldBit) * bytes_per_cluster), num * bytes_per_cluster);
                //                LOG(LOG_LEVEL_RECORD, L"1: start: %I64u, length: %I64u, end:  %I64u", c.start_offset, c.length, c.start_offset + c.length);
                //#endif
                results.push_back(changed_area(file_area_offset + ((start_lcn + oldBit) * bytes_per_cluster), num * bytes_per_cluster));
                oldBit = next_dirty_bit = newBit;
            }
            else{
                next_dirty_bit = newBit;
            }
        }
        else {
            if (dirty_range && ((newBit - oldBit) >= mini_number_of_bits)){
                uint64_t lenght = (next_dirty_bit - oldBit + 1) * bytes_per_cluster;
                total_number_of_delta += lenght;
                //#ifdef _DEBUG
                //                changed_area c(file_area_offset + ((start_lcn + oldBit) * bytes_per_cluster), lenght);
                //                LOG(LOG_LEVEL_RECORD, L"2: start: %I64u, length: %I64u, end:  %I64u", c.start_offset, c.length, c.start_offset + c.length);
                //#endif
                results.push_back(changed_area(file_area_offset + ((start_lcn + oldBit) * bytes_per_cluster), lenght));
                dirty_range = false;
            }
        }
    }
    if (dirty_range){
        //#ifdef _DEBUG
        //        changed_area c(file_area_offset + ((start_lcn + oldBit) * bytes_per_cluster), (next_dirty_bit - oldBit + 1) * bytes_per_cluster);
        //        LOG(LOG_LEVEL_RECORD, L"3: start: %I64u, length: %I64u, end:  %I64u", c.start_offset, c.length, c.start_offset + c.length);
        //#endif
        total_number_of_delta += ((next_dirty_bit - oldBit + 1) * bytes_per_cluster);
        results.push_back(changed_area(file_area_offset + ((start_lcn + oldBit) * bytes_per_cluster), (next_dirty_bit - oldBit + 1) * bytes_per_cluster));
    }
    return results;
}

BOOL mirror_disk::get_fat_first_sector_offset(HANDLE handle, ULONGLONG& file_area_offset){

    FAT_LBR		fatLBR = { 0 };
    BOOL        bFnCall = FALSE;
    BOOL        bRetVal = FALSE;
    DWORD       dwRead = 0;
    if ((bFnCall = ReadFile(handle, &fatLBR, sizeof(FAT_LBR), &dwRead, NULL)
        && dwRead == sizeof(fatLBR))){
        DWORD dwRootDirSectors = 0;
        DWORD dwFATSz = 0;

        // Validate jump instruction to boot code. This field has two
        // allowed forms: 
        // jmpBoot[0] = 0xEB, jmpBoot[1] = 0x??, jmpBoot[2] = 0x90 
        // and
        // jmpBoot[0] = 0xE9, jmpBoot[1] = 0x??, jmpBoot[2] = 0x??
        // 0x?? indicates that any 8-bit value is allowed in that byte.
        // JmpBoot[0] = 0xEB is the more frequently used format.

        if ((fatLBR.wTrailSig != 0xAA55) ||
            ((fatLBR.pbyJmpBoot[0] != 0xEB ||
            fatLBR.pbyJmpBoot[2] != 0x90) &&
            (fatLBR.pbyJmpBoot[0] != 0xE9))){
            bRetVal = FALSE;
            goto __faild;
        }

        // Compute first sector offset for the FAT volumes:		


        // First, we determine the count of sectors occupied by the
        // root directory. Note that on a FAT32 volume the BPB_RootEntCnt
        // value is always 0, so on a FAT32 volume dwRootDirSectors is
        // always 0. The 32 in the above is the size of one FAT directory
        // entry in bytes. Note also that this computation rounds up.

        dwRootDirSectors =
            (((fatLBR.bpb.wRootEntCnt * 32) +
            (fatLBR.bpb.wBytsPerSec - 1)) /
            fatLBR.bpb.wBytsPerSec);

        // The start of the data region, the first sector of cluster 2,
        // is computed as follows:

        dwFATSz = fatLBR.bpb.wFATSz16;
        if (!dwFATSz)
            dwFATSz = fatLBR.ebpb.ebpb32.dwFATSz32;

        if (!dwFATSz){
            bRetVal = FALSE;
            goto __faild;
        }
        // 
        file_area_offset =
            (fatLBR.bpb.wRsvdSecCnt +
            (fatLBR.bpb.byNumFATs * dwFATSz) +
            dwRootDirSectors) * fatLBR.bpb.wBytsPerSec;
    }
    bRetVal = TRUE;
__faild:
    return bRetVal;
}

changed_area::vtr mirror_disk::get_changed_areas(std::string path, uint64_t start){
    FUN_TRACE;
    changed_area::vtr results;
    macho::windows::auto_handle handle = CreateFileA(path.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    if (handle.is_invalid()){
    }
    else{
        DWORD SectorsPerCluster = 0;
        DWORD BytesPerSector = 0;
        DWORD BytesPerCluster = 0;
        std::string device_object = path + "\\";
        if (!GetDiskFreeSpaceA(device_object.c_str(), &SectorsPerCluster, &BytesPerSector, NULL, NULL)){
        }
        else{
            uint32_t bytes_per_cluster = BytesPerCluster = SectorsPerCluster * BytesPerSector;
            STARTING_LCN_INPUT_BUFFER StartingLCN;
            std::auto_ptr<VOLUME_BITMAP_BUFFER> Bitmap;
            UINT32 BitmapSize;
            DWORD BytesReturned;
            BOOL Result;
            ULONGLONG ClusterCount = 0;
            StartingLCN.StartingLcn.QuadPart = 0;
            BitmapSize = sizeof(VOLUME_BITMAP_BUFFER) + 4;
            Bitmap = std::auto_ptr<VOLUME_BITMAP_BUFFER>((VOLUME_BITMAP_BUFFER*)new BYTE[BitmapSize]);
            Result = DeviceIoControl(
                handle,
                FSCTL_GET_VOLUME_BITMAP,
                &StartingLCN,
                sizeof(StartingLCN),
                Bitmap.get(),
                BitmapSize,
                &BytesReturned,
                NULL);
            DWORD err = GetLastError();
            if (Result == FALSE && err != ERROR_MORE_DATA){
                LOG(LOG_LEVEL_ERROR, L"DeviceIoControl(FSCTL_GET_VOLUME_BITMAP, %I64u) faided for %s. (Error : 0x%08x)", 0, macho::stringutils::convert_ansi_to_unicode(path).c_str(), err);
            }
            else{
                BitmapSize = (ULONG)(sizeof(VOLUME_BITMAP_BUFFER) + (Bitmap->BitmapSize.QuadPart / 8) + 1);
                Bitmap = std::auto_ptr<VOLUME_BITMAP_BUFFER>((VOLUME_BITMAP_BUFFER*)new BYTE[BitmapSize]);
                Result = DeviceIoControl(
                    handle,
                    FSCTL_GET_VOLUME_BITMAP,
                    &StartingLCN,
                    sizeof(StartingLCN),
                    Bitmap.get(),
                    BitmapSize,
                    &BytesReturned,
                    NULL);
                if (Result == FALSE){
                    LOG(LOG_LEVEL_ERROR, L"DeviceIoControl(FSCTL_GET_VOLUME_BITMAP, %I64u) failed for %s. (Error : 0x%08x)", 0, macho::stringutils::convert_ansi_to_unicode(path).c_str(), GetLastError());
                }
                else{
                    ULONGLONG total_number_of_delta = 0;
                    ULONG     delta_mini_block_size = MINI_BLOCK_SIZE;
                    ULONG     full_mini_block_size = MAX_BLOCK_SIZE;
                    ULONGLONG file_area_offset = 0;

                    if (IsWindows7OrGreater()){
                        RETRIEVAL_POINTER_BASE retrieval_pointer_base;
                        if (!(Result = DeviceIoControl(
                            handle,
                            FSCTL_GET_RETRIEVAL_POINTER_BASE,
                            NULL,
                            0,
                            &retrieval_pointer_base,
                            sizeof(retrieval_pointer_base),
                            &BytesReturned,
                            NULL))){
                            LOG(LOG_LEVEL_ERROR, L"DeviceIoControl(FSCTL_GET_RETRIEVAL_POINTER_BASE) failed for %s. (Error : 0x%08x)", macho::stringutils::convert_ansi_to_unicode(path).c_str(), GetLastError());
                        }
                        else{
                            file_area_offset = retrieval_pointer_base.FileAreaOffset.QuadPart * BytesPerSector;
                        }
                    }
                    else{
                        get_fat_first_sector_offset(handle, file_area_offset);
                    }

                    _get_changed_areas(file_area_offset, Bitmap->Buffer, Bitmap->StartingLcn.QuadPart, Bitmap->BitmapSize.QuadPart, BytesPerCluster, total_number_of_delta);
                    LOG(LOG_LEVEL_RECORD, L"Before excluding, Delta Size for volume(%s): %I64u", macho::stringutils::convert_ansi_to_unicode(path).c_str(), total_number_of_delta);

                    exclude_file(Bitmap->Buffer, boost::filesystem::path(path) / L"pagefile.sys");
                    exclude_file(Bitmap->Buffer, boost::filesystem::path(path) / L"hiberfil.sys");
                    exclude_file(Bitmap->Buffer, boost::filesystem::path(path) / L"swapfile.sys");
                    exclude_file(Bitmap->Buffer, boost::filesystem::path(path) / L"windows" / L"memory.dmp");

                    std::vector<boost::filesystem::path> files = environment::get_files(boost::filesystem::path(path) / L"System Volume Information", L"*{3808876b-c176-4e48-b7ae-04046e6cc752}");
                    if (files.size()){
                        universal_disk_rw::ptr rw = universal_disk_rw::ptr(new general_io_rw(handle, macho::stringutils::convert_ansi_to_unicode(path)));
                        ntfs::volume::ptr volume = ntfs::volume::get(rw, 0ULL);
                        if (volume){
                            ntfs::file_record::ptr system_volume_information = volume->find_sub_entry(L"System Volume Information");
                            if (system_volume_information){
                                foreach(boost::filesystem::path &file, files){
                                    ntfs::file_record::ptr _file = volume->find_sub_entry(file.filename().wstring(), system_volume_information);
                                    if (_file){
                                        ntfs::file_extent::vtr _exts = _file->extents();
                                        foreach(auto e, _exts){
                                            LOG(LOG_LEVEL_INFO, L"(%s): %I64u(%I64u):%I64u", file.wstring().c_str(), e.start, e.physical, e.length);
                                            if (ULLONG_MAX != e.physical)
                                                clear_umap(Bitmap->Buffer, e.physical, e.length, BytesPerCluster);
                                        }
                                        LOG(LOG_LEVEL_RECORD, L"Excluded the data replication for the file (%s).", file.wstring().c_str());
                                    }
                                }
                            }
                        }
                    }

                    results = _get_changed_areas(file_area_offset, Bitmap->Buffer, Bitmap->StartingLcn.QuadPart, Bitmap->BitmapSize.QuadPart, BytesPerCluster, total_number_of_delta);
                    LOG(LOG_LEVEL_RECORD, L"After excluding, Delta Size for volume(%s): %I64u", macho::stringutils::convert_ansi_to_unicode(path).c_str(), total_number_of_delta);
                    if (start){
                        for (changed_area::vtr::iterator it = results.begin(); it != results.end() /* !!! */;){
                            if ((it->start + it->length) < start){
                                it = results.erase(it);  // Returns the new iterator to continue from.
                            }
                            else{
                                ++it;
                            }
                        }
                    }
                }
            }
        }
    }
    return results;
}

bool mirror_disk::replicate_changed_areas(macho::windows::storage::disk& d, macho::windows::storage::partition& p, std::string path, uint64_t& start_offset, uint64_t& backup_size, universal_disk_rw::ptr& output){
    FUN_TRACE;
    bool is_replicated = false;
    if (path.length()){
        if (path[path.length() - 1] == '\\')
            path.erase(path.length() - 1);
        if (temp_drive_letter::is_drive_letter(path))
            path = boost::str(boost::format("\\\\?\\%s") % path);
        universal_disk_rw::ptr rw = general_io_rw::open(path);
        if (!rw){
            LOG(LOG_LEVEL_ERROR, _T("Cannot open(\"%s\")"), stringutils::convert_ansi_to_unicode(path).c_str());
        }
        else if ((start_offset >= (uint64_t)p.offset()) && (start_offset < ((uint64_t)p.offset() + (uint64_t)p.size()))){
            uint64_t offset = p.offset();
            changed_area::vtr  changeds = get_changed_areas(path, start_offset - offset);
            is_replicated = replicate_changed_areas(rw, d, start_offset, offset, backup_size, changeds, output);
        }
        else{
            is_replicated = true;
        }
    }
    return is_replicated;
}

changed_area::vtr  mirror_disk::get_epbrs(universal_disk_rw::ptr &disk_rw, macho::windows::storage::disk& d, macho::windows::storage::partition::vtr& partitions, const uint64_t& start_offset){
    changed_area::vtr changeds;
    int length = 4096;
    if (d.partition_style() == macho::windows::storage::ST_PARTITION_STYLE::ST_PST_MBR){
        std::auto_ptr<BYTE> pBuf = std::auto_ptr<BYTE>(new BYTE[length]);
        if (NULL != pBuf.get()){
            foreach(macho::windows::storage::partition::ptr p, partitions){
                if (!(p->mbr_type() == PARTITION_XINT13_EXTENDED || p->mbr_type() == PARTITION_EXTENDED))
                    continue;
                uint64_t start = p->offset();
                bool loop = true;
                while (loop){
                    loop = false;
                    memset(pBuf.get(), 0, length);
                    uint32_t nRead = 0;
                    BOOL result = FALSE;
                    if (result = disk_rw->read(start, length, pBuf.get(), nRead)){
                        PLEGACY_MBR pMBR = (PLEGACY_MBR)pBuf.get();
                        if (pMBR->Signature != 0xAA55)
                            break;
                        if (start > start_offset)
                            changeds.push_back(changed_area(start, d.logical_sector_size()));
                        for (int i = 0; i < 2; i++){
                            if (pMBR->PartitionRecord[i].PartitionType == PARTITION_XINT13_EXTENDED ||
                                pMBR->PartitionRecord[i].PartitionType == PARTITION_EXTENDED){
                                start = p->offset() + ((uint64_t)pMBR->PartitionRecord[i].StartingLBA * d.logical_sector_size());
                                loop = true;
                                break;
                            }
                            else if (pMBR->PartitionRecord[i].StartingLBA != 0 && pMBR->PartitionRecord[i].SizeInLBA != 0){
                            }
                            else{
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
    std::sort(changeds.begin(), changeds.end(), changed_area::compare());
    return changeds;
}

bool mirror_disk::replicate_beginning_of_disk(universal_disk_rw::ptr &disk_rw, macho::windows::storage::disk& d, uint64_t& start_offset, uint64_t& backup_size, universal_disk_rw::ptr& output){
    FUN_TRACE;
    bool result = false;
    bool is_replicated = true;
    uint32_t length = _block_size;
    uint64_t offset = 0;
    if (disk_rw){
        macho::windows::storage::partition::vtr parts = d.get_partitions();
        foreach(macho::windows::storage::partition::ptr p, parts){
            if (1 == p->partition_number()){
                if (p->offset() < _block_size)
                    length = (uint32_t)p->offset();
                if (!(is_replicated = (start_offset >= length))){
                    while ((!_is_interrupted) && offset < (uint64_t)p->offset()){
                        uint64_t _offset = offset;
                        if (!(is_replicated = replicate(disk_rw, offset, length, output)))
                            break;
                        else{
                            start_offset = offset;
                            backup_size += length;
                            break;
                            /*
                            if ((length == _block_size) && (p->offset() - offset) < _block_size)
                            length = (uint32_t)(p->offset() - offset);
                            */
                        }
                    }
                }
            }
        }
        result = is_replicated && (!_is_interrupted);
    }
    return result;
}

bool mirror_disk::replicate(universal_disk_rw::ptr &rw, uint64_t& start, const uint32_t length, universal_disk_rw::ptr& output, const uint64_t target_offset){
    FUN_TRACE;
    bool result = false;
    std::auto_ptr<BYTE> buf = std::auto_ptr<BYTE>(new BYTE[length]);
    uint32_t nRead = 0;
    if (output){
        if (_vhd_preparation){
            uint32_t number_of_bytes_written = 0;
            if (!(result = output->write(target_offset + start, NULL, length, number_of_bytes_written))){
                LOG(LOG_LEVEL_ERROR, _T("Cannot write( %I64u, %d)"), target_offset + start, length);
            }
            else{
                start += number_of_bytes_written;
            }
        }
        else{
            if (!(result = rw->read(start, length, buf.get(), nRead))){
                LOG(LOG_LEVEL_ERROR, _T("Cannot Read( %I64u, %d). Error: 0x%08X"), start, length, GetLastError());
            }
            else {
                uint32_t number_of_bytes_written = 0;
                if (!(result = output->write(target_offset + start, buf.get(), nRead, number_of_bytes_written))){
                    LOG(LOG_LEVEL_ERROR, _T("Cannot write( %I64u, %d)"), target_offset + start, nRead);
                }
                else{
                    start += number_of_bytes_written;
                }
            }
        }
    }
    else{
        _backup_size += length;
        start += length;
        result = true;
    }
    return result;
}

bool mirror_disk::replicate_partitions_of_disk(universal_disk_rw::ptr &disk_rw, macho::windows::storage::disk& d, uint64_t& start_offset, uint64_t& backup_size, universal_disk_rw::ptr& output){
    FUN_TRACE;
    bool result = false;
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
                    if (c->start <= start_offset){
                        if (!(is_replicated = replicate(disk_rw, c->start, c->length, output, 0)))
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
                        if (!(is_replicated = replicate_changed_areas(disk_rw, d, start_offset, 0, backup_size, changeds, output)))
                            break;
                    }
                    else{
                        if (!(is_replicated = replicate_changed_areas(d, *p, drive_letter, start_offset, backup_size, output)))
                            break;
                    }
                    if (is_replicated && (!_is_interrupted))
                        start_offset = end_of_partiton;
                }
                else if (!p->access_paths().size() && start_offset < end_of_partiton){
                    if (d.partition_style() == macho::windows::storage::ST_PARTITION_STYLE::ST_PST_MBR &&
                        (p->mbr_type() == PARTITION_XINT13_EXTENDED || p->mbr_type() == PARTITION_EXTENDED)){
                    }
                    else{
                        changed_area::vtr  changeds = get_changed_areas(disk_rw, d, (*p.get()), start_offset);
                        if (!(is_replicated = replicate_changed_areas(disk_rw, d, start_offset, 0, backup_size, changeds, output)))
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
                        if (!(is_replicated = replicate_changed_areas(disk_rw, d, start_offset, 0, backup_size, changeds, output)))
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

                        if (!(is_replicated = replicate_changed_areas(d, *p, volume_path, start_offset, backup_size, output)))
                            break;
                    }
                    if (is_replicated && (!_is_interrupted))
                        start_offset = end_of_partiton;
                }
            }
        }

        result = is_replicated && (!_is_interrupted);
    }
    return result;
}