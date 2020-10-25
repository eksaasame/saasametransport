
#include <windows.h>
#include <Winternl.h>
#define _NTDEF_
typedef LONG NTSTATUS, *PNTSTATUS;
#include "shrink_volume.h"
#include "ntfs_parser.h"
#include <ntddvol.h>
#include "..\vcbt\vcbt\fatlbr.h"
#include <VersionHelpers.h>

#define round_up(what, on) ((((what) + (on) - 1) / (on)) * (on))
#pragma comment(lib, "ntdll.lib")

using namespace macho::windows;
using namespace macho;

shrink_volume::ptr shrink_volume::open(boost::filesystem::path p, size_t size){
    TCHAR _volume_path_name[MAX_PATH + 1] = { 0 };
    TCHAR _volume_name[MAX_PATH + 1] = { 0 };
    std::wstring _p = p.wstring();
    std::wstring _name;
    if (_p[_p.length() - 1] == L'\\')
        _p.erase(_p.length() - 1);
    if (GetVolumePathNameW(std::wstring(_p + L"\\").c_str(), _volume_path_name, MAX_PATH)){
        if ((5 < _p.length()) &&
            (_p[0] == _T('\\') &&
            _p[1] == _T('\\') &&
            _p[2] == _T('?') &&
            _p[3] == _T('\\')))
            _name = _p + L"\\";
        else if (GetVolumeNameForVolumeMountPointW(_volume_path_name, _volume_name, MAX_PATH)) {
            _name = _volume_name;
        }
        shrink_volume::ptr s(new shrink_volume(_name, size));
        if (_name[_name.length() - 1] == L'\\')
            _name.erase(_name.length() - 1);
        s->_handle = CreateFileW(_name.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS,
            NULL);
        if (s->_handle.is_valid()){
            DWORD SectorsPerCluster = 0;
            DWORD BytesPerSector = 0;
            ULARGE_INTEGER capacity, free_space;
            TCHAR _volume_name_buffer[MAX_PATH + 1] = { 0 };
            TCHAR _file_system_name[MAX_PATH + 1] = { 0 };
            if (!GetVolumeInformation(_volume_path_name, _volume_name_buffer, MAX_PATH, &s->_serial_number, NULL, &s->_file_system_flags, _file_system_name, MAX_PATH)){
            }
            else if (!GetDiskFreeSpaceW(_volume_path_name, &SectorsPerCluster, &BytesPerSector, &s->_number_of_free_clusters, &s->_total_number_of_clusters)){
            }
            else if (!GetDiskFreeSpaceExW(_volume_path_name, NULL, &capacity, &free_space)){
            }
            else{
                s->_name = _name;
                s->_size = capacity.QuadPart;
                s->_size_remaining = free_space.QuadPart;
                s->_file_system_type = _file_system_name;
                s->_label = _volume_name_buffer;
                s->_bytes_per_cluster = SectorsPerCluster * BytesPerSector;
                s->_aligned_cluster = ((0x4000 - 1) / s->_bytes_per_cluster) + 1;
                if (IsWindows7OrGreater()){
                    DWORD BytesReturned;
                    BOOL  Result;
                    RETRIEVAL_POINTER_BASE retrieval_pointer_base;
                    if (!(Result = DeviceIoControl(
                        s->_handle,
                        FSCTL_GET_RETRIEVAL_POINTER_BASE,
                        NULL,
                        0,
                        &retrieval_pointer_base,
                        sizeof(retrieval_pointer_base),
                        &BytesReturned,
                        NULL))){
                        LOG(LOG_LEVEL_ERROR, L"DeviceIoControl(FSCTL_GET_RETRIEVAL_POINTER_BASE) failed for %s. (Error : 0x%08x)", _p.c_str(), GetLastError());
                    }
                    else{
                        s->_file_area_offset = retrieval_pointer_base.FileAreaOffset.QuadPart * BytesPerSector;
                    }
                }
                else{
                    _get_fat_first_sector_offset(s->_handle, s->_file_area_offset);
                }
                DWORD output_size = 0;
                if (!DeviceIoControl(s->_handle, FSCTL_IS_VOLUME_DIRTY, NULL, 0, &s->_dirty_flag, sizeof(ULONG), &output_size, NULL)){
                    LOG(LOG_LEVEL_ERROR, L"DeviceIoControl(FSCTL_IS_VOLUME_DIRTY) failed for %s. (Error : 0x%08x)", _p.c_str(), GetLastError());
                }
                else{
                    if (!_wcsicmp(s->_file_system_type.c_str(), L"Ntfs")){
                        if (!DeviceIoControl(s->_handle, FSCTL_GET_NTFS_VOLUME_DATA, NULL, 0, &s->_ntfs_info, sizeof(NTFS_DATA), &output_size, NULL)){
                            LOG(LOG_LEVEL_ERROR, L"DeviceIoControl(FSCTL_GET_NTFS_VOLUME_DATA) failed for %s. (Error : 0x%08x)", _p.c_str(), GetLastError());
                        }
                    }
                    return s;
                }
            }
        }
    }
    return NULL;
}

BOOL shrink_volume::_get_fat_first_sector_offset(HANDLE handle, ULONGLONG& file_area_offset){
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

shrink_volume::data_range::vtr shrink_volume::_get_used_space_ranges(){
    data_range::vtr ranges;
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
        _handle,
        FSCTL_GET_VOLUME_BITMAP,
        &StartingLCN,
        sizeof(StartingLCN),
        Bitmap.get(),
        BitmapSize,
        &BytesReturned,
        NULL);
    DWORD err = GetLastError();
    if (Result == FALSE && err != ERROR_MORE_DATA){
        LOG(LOG_LEVEL_ERROR, L"DeviceIoControl(FSCTL_GET_VOLUME_BITMAP, %I64u) faided for %s. (Error : 0x%08x)", 0, _path.wstring().c_str(), err);
    }
    else{
        BitmapSize = (ULONG)(sizeof(VOLUME_BITMAP_BUFFER) + (Bitmap->BitmapSize.QuadPart / 8) + 1);
        Bitmap = std::auto_ptr<VOLUME_BITMAP_BUFFER>((VOLUME_BITMAP_BUFFER*)new BYTE[BitmapSize]);
        Result = DeviceIoControl(
            _handle,
            FSCTL_GET_VOLUME_BITMAP,
            &StartingLCN,
            sizeof(StartingLCN),
            Bitmap.get(),
            BitmapSize,
            &BytesReturned,
            NULL);
        if (Result == FALSE){
            LOG(LOG_LEVEL_ERROR, L"DeviceIoControl(FSCTL_GET_VOLUME_BITMAP, %I64u) failed for %s. (Error : 0x%08x)", 0, _path.wstring().c_str(), GetLastError());
        }
        else{
            ranges = _get_data_ranges(Bitmap.get()->Buffer, Bitmap.get()->StartingLcn.QuadPart, Bitmap.get()->BitmapSize.QuadPart);
        }
    }
    return ranges;
}

shrink_volume::data_range::vtr shrink_volume::_get_free_space_ranges(data_range& _latest_used_data_range){
    data_range::vtr free_ranges;
    data_range::vtr ranges = _get_used_space_ranges();
    if (ranges.size() > 0)
        _latest_used_data_range = ranges[ranges.size() - 1];
    uint64_t next_start = 0;
    foreach(data_range &d, ranges){
        if (d.start > next_start){
            free_ranges.push_back(data_range(next_start, d.start - next_start));
        }
        next_start = d.start + d.length;
        if (next_start >= _new_size)
            break;
    }
    return free_ranges;
}

shrink_volume::file_data_range::vtr shrink_volume::_get_file_retrieval_pointers(HANDLE handle){
    file_data_range::vtr ranges;
    STARTING_VCN_INPUT_BUFFER starting;
    std::auto_ptr<RETRIEVAL_POINTERS_BUFFER> pVcnPairs;
    starting.StartingVcn.QuadPart = 0;
    ULONG ulOutPutSize = 0;
    ULONG uCounts = 200;
    ulOutPutSize = sizeof(RETRIEVAL_POINTERS_BUFFER) + uCounts * sizeof(LARGE_INTEGER) * 2 + sizeof(LARGE_INTEGER);
    pVcnPairs = std::auto_ptr<RETRIEVAL_POINTERS_BUFFER>((PRETRIEVAL_POINTERS_BUFFER)new BYTE[ulOutPutSize]);
    if (NULL != pVcnPairs.get()){
        memset(pVcnPairs.get(), 0, ulOutPutSize);
        DWORD BytesReturned = 0;
        while (!DeviceIoControl(handle, FSCTL_GET_RETRIEVAL_POINTERS, &starting, sizeof(STARTING_VCN_INPUT_BUFFER), pVcnPairs.get(), ulOutPutSize, &BytesReturned, NULL)){
            DWORD err = GetLastError();
            if (ERROR_MORE_DATA == err || ERROR_BUFFER_OVERFLOW == err || ERROR_INSUFFICIENT_BUFFER == err){
                uCounts += 200;
                ulOutPutSize = sizeof(RETRIEVAL_POINTERS_BUFFER) + uCounts * sizeof(LARGE_INTEGER) * 2 + sizeof(LARGE_INTEGER);
                pVcnPairs = std::auto_ptr<RETRIEVAL_POINTERS_BUFFER>((PRETRIEVAL_POINTERS_BUFFER)new BYTE[ulOutPutSize]);
                if (pVcnPairs.get() == NULL)
                    break;
                memset(pVcnPairs.get(), 0, ulOutPutSize);
            }
            else{
                if (err != ERROR_HANDLE_EOF)
                    LOG(LOG_LEVEL_ERROR, L"DeviceIoControl(FSCTL_GET_RETRIEVAL_POINTERS) failed. Error: %d", err);
                pVcnPairs.release();
                break;
            }
        }
        if (NULL != pVcnPairs.get()){
            LARGE_INTEGER liVcnPrev = pVcnPairs->StartingVcn;
            for (DWORD extent = 0; extent < pVcnPairs->ExtentCount; extent++){
                LONG      _Length = (ULONG)(pVcnPairs->Extents[extent].NextVcn.QuadPart - liVcnPrev.QuadPart);
                ranges.push_back(file_data_range((uint64_t)liVcnPrev.QuadPart,
                    (uint64_t)pVcnPairs->Extents[extent].Lcn.QuadPart, 
                    (uint64_t)_Length));
                liVcnPrev = pVcnPairs->Extents[extent].NextVcn;
            }
        }
    }
    return ranges;
}

bool shrink_volume::defragment(){
    shrink_volume::fragment_file::vtr fragment_files;
    MAKE_AUTO_HANDLE_CLASS_EX(auto_defreg_file_handle, HANDLE, defrag_file_close, INVALID_HANDLE_VALUE);
    std::vector<boost::filesystem::path> remove_files;
    remove_files.push_back(_path / "pagefile.sys");
    remove_files.push_back(_path / "hiberfil.sys");
    remove_files.push_back(_path / "swapfile.sys");
    remove_files.push_back(_path / "Windows" / "memory.dmp");
    foreach(boost::filesystem::path& f, remove_files){
        if (boost::filesystem::exists(f)){
            SetFileAttributesW(f.wstring().c_str(), FILE_ATTRIBUTE_NORMAL);
            DeleteFileW(f.wstring().c_str());
        }
    }
    data_range _latest_used_data_range;
    data_range::vtr free_ranges = _get_free_space_ranges(_latest_used_data_range);
    int count = 3;
    if (_new_size < (_latest_used_data_range.start + _latest_used_data_range.length)){
        if (!_wcsicmp(_file_system_type.c_str(), L"Ntfs")){
            //_scan_mft(fragment_files);
            _defragment(fragment_files);
            _enum_mft(fragment_files);
        }
        else     
        {
            _defragment(fragment_files);
            _defragment(_path, fragment_files);
        }
        std::sort(fragment_files.begin(), fragment_files.end(), shrink_volume::fragment_file::compare());
        std::sort(free_ranges.begin(), free_ranges.end(), data_range::compare());
        while ( count > 0 && _new_size < (_latest_used_data_range.start + _latest_used_data_range.length)){
            count--;
            for (fragment_file::vtr::iterator f = fragment_files.begin(); f != fragment_files.end();){
                auto_defreg_file_handle handle = defrag_file_open(f->file.wstring(), f->attributes);
                if (handle.is_valid()){
                    for (data_range::vtr::iterator p = free_ranges.begin(); p != free_ranges.end();){
                        //uint64_t start = ((uint64_t)(p->start - _file_area_offset) / _bytes_per_cluster);
                        //uint64_t aligned = round_up(start, _aligned_cluster);
                        for (file_data_range::vtr::iterator e = f->extents.begin(); e != f->extents.end();){
                            uint64_t ext_length = e->count * _bytes_per_cluster;
                            if (p->length >= ext_length){
                                if (_move_file(handle, e->vcn, ((uint64_t)(p->start - _file_area_offset) / _bytes_per_cluster), (DWORD)e->count)){
                                    p->start += ext_length;
                                    p->length -= ext_length;
                                    e = f->extents.erase(e);
                                }
                                else{
                                    ++e;
                                }
                            }
                            else{
                                ext_length = p->length;
                                uint64_t count = ext_length / _bytes_per_cluster;
                                if (_move_file(handle, e->vcn, ((uint64_t)(p->start - _file_area_offset) / _bytes_per_cluster), (DWORD)count)){
                                    p->start += ext_length;
                                    p->length -= ext_length;
                                    e->count -= count;
                                    e->vcn += count;
                                    break;
                                }
                                else{
                                    ++e;
                                }
                            }
                            if (0 == p->length)
                                break;
                        }
                        if (0 == p->length)
                            p = free_ranges.erase(p);
                        else
                            ++p;
                    }
                }
                if (f->extents.empty())
                    f = fragment_files.erase(f);
                else
                    ++f;
            }
            BOOL r = FlushFileBuffers(_handle);
            if (fragment_files.empty()){
                _lookup_stream_from_clusters(fragment_files);
            }
            else{
                free_ranges = _get_free_space_ranges(_latest_used_data_range);
            }
            if (!fragment_files.size())
                break;
        }
    }
    return fragment_files.empty();
}

void shrink_volume::_defragment(fragment_file::vtr& fragment_files){
    std::vector<boost::filesystem::path> system_files;
    system_files.push_back(_path / "::$BITMAP");
    system_files.push_back(_path / "::$INDEX_ALLOCATION");
    system_files.push_back(_path / "$MFT::$DATA");
    system_files.push_back(_path / "$MFT::$ATTRIBUTE_LIST");
    system_files.push_back(_path / "$MFT::$BITMAP");
    system_files.push_back(_path / "$AttrDef::$DATA");
    system_files.push_back(_path / "$AttrDef::$ATTRIBUTE_LIST");
    system_files.push_back(_path / "$Secure:$SDS:$DATA");
    system_files.push_back(_path / "$Secure::$ATTRIBUTE_LIST");
    system_files.push_back(_path / "$Secure:$SDH:$INDEX_ALLOCATION");
    system_files.push_back(_path / "$Secure:$SDH:$BITMAP");
    system_files.push_back(_path / "$Secure:$SII:$INDEX_ALLOCATION");
    system_files.push_back(_path / "$Secure:$SII:$BITMAP");
    system_files.push_back(_path / "$UpCase::$DATA");
    system_files.push_back(_path / "$UpCase::$ATTRIBUTE_LIST");
    system_files.push_back(_path / "$Extend:$I30:$INDEX_ALLOCATION");
    system_files.push_back(_path / "$Extend::$ATTRIBUTE_LIST");
    system_files.push_back(_path / "$Extend:$I30:$BITMAP");
    system_files.push_back(_path / "$Extend\\$UsnJrnl:$J:$DATA");
    system_files.push_back(_path / "$Extend\\$UsnJrnl::$ATTRIBUTE_LIST");
    system_files.push_back(_path / "$Extend\\$UsnJrnl:$Max:$DATA");
    system_files.push_back(_path / "$Extend\\$Quota:$Q:$INDEX_ALLOCATION");
    system_files.push_back(_path / "$Extend\\$Quota::$ATTRIBUTE_LIST");
    system_files.push_back(_path / "$Extend\\$Quota:$Q:$BITMAP");
    system_files.push_back(_path / "$Extend\\$Quota:$O:$INDEX_ALLOCATION");
    system_files.push_back(_path / "$Extend\\$Quota:$O:$BITMAP");
    system_files.push_back(_path / "$Extend\\$ObjId:$O:$INDEX_ALLOCATION");
    system_files.push_back(_path / "$Extend\\$ObjId::$ATTRIBUTE_LIST");
    system_files.push_back(_path / "$Extend\\$ObjId:$O:$BITMAP");
    system_files.push_back(_path / "$Extend\\$Reparse:$R:$INDEX_ALLOCATION");
    system_files.push_back(_path / "$Extend\\$Reparse::$ATTRIBUTE_LIST");
    system_files.push_back(_path / "$Extend\\$Reparse:$R:$BITMAP");
    system_files.push_back(_path / "$Extend\\$RmMetadata:$I30:$INDEX_ALLOCATION");
    system_files.push_back(_path / "$Extend\\$RmMetadata:$I30:$BITMAP");
    system_files.push_back(_path / "$Extend\\$RmMetadata::$ATTRIBUTE_LIST");
    system_files.push_back(_path / "$Extend\\$RmMetadata\\$Repair::$DATA");
    system_files.push_back(_path / "$Extend\\$RmMetadata\\$Repair::$ATTRIBUTE_LIST");
    system_files.push_back(_path / "$Extend\\$RmMetadata\\$Repair:$Config:$DATA");
    system_files.push_back(_path / "$Extend\\$RmMetadata\\$Txf:$I30:$INDEX_ALLOCATION");
    system_files.push_back(_path / "$Extend\\$RmMetadata\\$Txf::$ATTRIBUTE_LIST");
    system_files.push_back(_path / "$Extend\\$RmMetadata\\$Txf:$I30:$BITMAP");
    system_files.push_back(_path / "$Extend\\$RmMetadata\\$Txf:$TXF_DATA:$LOGGED_UTILITY_STREAM");
    system_files.push_back(_path / "$Extend\\$RmMetadata\\$TxfLog:$I30:$INDEX_ALLOCATION");
    system_files.push_back(_path / "$Extend\\$RmMetadata\\$TxfLog::$ATTRIBUTE_LIST");
    system_files.push_back(_path / "$Extend\\$RmMetadata\\$TxfLog:$I30:$BITMAP");
    system_files.push_back(_path / "$Extend\\$RmMetadata\\$TxfLog\\$Tops::$DATA");
    system_files.push_back(_path / "$Extend\\$RmMetadata\\$TxfLog\\$Tops::$ATTRIBUTE_LIST");
    system_files.push_back(_path / "$Extend\\$RmMetadata\\$TxfLog\\$Tops:$T:$DATA");
    system_files.push_back(_path / "$Extend\\$RmMetadata\\$TxfLog\\$TxfLog.blf::$DATA");
    system_files.push_back(_path / "$Extend\\$RmMetadata\\$TxfLog\\$TxfLog.blf::$ATTRIBUTE_LIST");

    foreach(boost::filesystem::path& f, system_files){
        _check_file(f, fragment_files);
    }
}

void shrink_volume::_check_file(boost::filesystem::path f, shrink_volume::fragment_file::vtr& fragment_files, DWORD attributes){
    MAKE_AUTO_HANDLE_CLASS_EX(auto_defreg_file_handle, HANDLE, defrag_file_close, INVALID_HANDLE_VALUE);
    auto_defreg_file_handle handle = defrag_file_open(f.wstring(), attributes);
    if (handle.is_valid()){
        file_data_range::vtr need_be_moved_extents;
        file_data_range::vtr extents = _get_file_retrieval_pointers(handle);
        foreach(file_data_range& extent, extents){
            if (_new_size < (_file_area_offset + ((extent.lcn + extent.count) * _bytes_per_cluster))){
                need_be_moved_extents.push_back(extent);
            }
        }
        if (need_be_moved_extents.size()){
            fragment_file fragment;
            fragment.file = f;
            fragment.attributes = attributes;
            fragment.extents = need_be_moved_extents;
            fragment_files.push_back(fragment);
        }
    }
}

bool shrink_volume::_enum_mft(fragment_file::vtr& fragment_files){
    MAKE_AUTO_HANDLE_CLASS_EX(auto_defreg_file_handle, HANDLE, defrag_file_close, INVALID_HANDLE_VALUE);
    bool result = false;
    DWORD err = 0, cb = 0, in = 65536;
    MFT_ENUM_DATA_V0 med = { 0 };
    med.StartFileReferenceNumber = 0;
    med.LowUsn = 0;
    med.HighUsn = MAXLONGLONG;
    boost::shared_ptr<BYTE> data(new BYTE[in]);
    memset(data.get(), 0, in);
    while (DeviceIoControl(_handle, FSCTL_ENUM_USN_DATA, &med, sizeof(med), (PVOID)data.get(), in, &cb, NULL)){
        if (cb <= sizeof(USN)){
            break;
        }
        result = true;
        med.StartFileReferenceNumber = *((DWORDLONG*)data.get());    // data.get() contains FRN for next FSCTL_ENUM_USN_DATA
        // The first returned record is just after the first sizeof(USN) bytes
        USN_RECORD_COMMON_HEADER *pUsnRecord = (PUSN_RECORD_COMMON_HEADER)&(((PBYTE)(PVOID)data.get())[sizeof(USN)]);
        int z = sizeof(USN_RECORD_V2);
        // Walk the output buffer
        while ((PBYTE)pUsnRecord < (((PBYTE)((PUSN_RECORD)(PVOID)data.get())) + cb)) {
            record r;
            switch (pUsnRecord->MajorVersion){
                case 2:{
                    USN_RECORD_V2 *pUsnRecord_v2 = (PUSN_RECORD_V2)(((PBYTE)(PVOID)pUsnRecord));
                    r.usn = pUsnRecord_v2->Usn;
                    r.reason = pUsnRecord_v2->Reason;
                    r.parent_frn.FileId.FileId64.Value = pUsnRecord_v2->ParentFileReferenceNumber;
                    r.frn.FileId.FileId64.Value = pUsnRecord_v2->FileReferenceNumber;
                    r.filename = &pUsnRecord_v2->FileName[0];
                    if (r.filename.length() > pUsnRecord_v2->FileNameLength)
                        r.filename.erase(pUsnRecord_v2->FileNameLength - 1);
                    r.attributes = pUsnRecord_v2->FileAttributes;
                    break;
                }
                case 3:{
                    USN_RECORD_V3 *pUsnRecord_v3 = (PUSN_RECORD_V3)(((PBYTE)(PVOID)pUsnRecord));
                    r.usn = pUsnRecord_v3->Usn;
                    r.reason = pUsnRecord_v3->Reason;
                    r.parent_frn.FileId.ExtendedFileId = pUsnRecord_v3->ParentFileReferenceNumber;
                    r.frn.FileId.ExtendedFileId = pUsnRecord_v3->FileReferenceNumber;
                    r.filename = &pUsnRecord_v3->FileName[0];
                    if (r.filename.length() > pUsnRecord_v3->FileNameLength)
                        r.filename.erase(pUsnRecord_v3->FileNameLength -1);
                    r.attributes = pUsnRecord_v3->FileAttributes;
                    break;
                }
            }
      
            {
                FILE_ID_DESCRIPTOR file_id_desc;
                memset(&file_id_desc, 0, sizeof(FILE_ID_DESCRIPTOR));
                file_id_desc.dwSize = sizeof(FILE_ID_DESCRIPTOR);
                file_id_desc.Type = FileIdType;
                /*if (Refs) file_id_desc.Type = ExtendedFileIdType;*/
                file_id_desc.FileId.QuadPart = r.frn.FileId.FileId64.Value;
                DWORD err = 0;
                macho::windows::auto_file_handle handle = OpenFileById(_handle, &file_id_desc, 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT);
                if (handle.is_invalid()){
                    LOG(LOG_LEVEL_ERROR, L"OpenFileById(%s) : error : %d", r.filename.c_str(), GetLastError());
                }
                else{
                    file_data_range::vtr need_be_moved_extents;
                    file_data_range::vtr extents = _get_file_retrieval_pointers(handle);
                    foreach(file_data_range& extent, extents){
                        if (_new_size < (_file_area_offset + ((extent.lcn + extent.count) * _bytes_per_cluster))){
                            need_be_moved_extents.push_back(extent);
                        }
                    }
                    if (need_be_moved_extents.size()){
                        std::wstring name = _get_filename_by_handle(handle);
                        if (!name.empty()){
                            fragment_file fragment;
                            fragment.file = boost::filesystem::path(_name) / name;
                            fragment.attributes = r.attributes;
                            fragment.extents = need_be_moved_extents;
                            fragment_files.push_back(fragment);
                        }
                    }
                }
            }
            // Move to next record
            pUsnRecord = (PUSN_RECORD_COMMON_HEADER)
                ((PBYTE)pUsnRecord + pUsnRecord->RecordLength);
        }
    }
    return result;
}

bool shrink_volume::_scan_mft(fragment_file::vtr& fragment_files){
    bool result = false;
    universal_disk_rw::ptr rw = universal_disk_rw::ptr(new general_io_rw(_handle, _path.wstring()));
    ntfs::volume::ptr vol = ntfs::volume::get(rw, 0ULL);
    if (vol){
        result = true;
        _scan_mft(vol, vol->root(), _path, fragment_files);
    }
    return result;
}

void shrink_volume::_scan_mft(ntfs::volume::ptr vol, ntfs::file_record::ptr dir, boost::filesystem::path p, fragment_file::vtr& fragment_files){
    ntfs::file_record::vtr files = vol->find_sub_entry(std::wregex(L"^.*", std::regex_constants::icase), dir);
    foreach(ntfs::file_record::ptr& f, files){
        if (!f->is_deleted()){
            if (f->is_directory()){
                if (f->file_name() != L"." && f->file_name() != L"..")
                    _scan_mft(vol, f, p / f->file_name(), fragment_files);
            }
            else{
                file_data_range::vtr need_be_moved_extents;
                ntfs::file_extent::vtr extents = f->extents();
                foreach(ntfs::file_extent& e, extents){
                    if (_new_size < (e.physical + e.length)){
                        file_data_range extent;
                        extent.count = e.length / _bytes_per_cluster;
                        extent.lcn = (e.physical - _file_area_offset) / _bytes_per_cluster;
                        extent.vcn = e.start / _bytes_per_cluster;
                        need_be_moved_extents.push_back(extent);
                    }
                }
                if (need_be_moved_extents.size()){
                    fragment_file fragment;
                    fragment.file = p / f->file_name();
                    fragment.extents = need_be_moved_extents;
                    fragment_files.push_back(fragment);
                }
            }
        }
    }
}

bool shrink_volume::_defragment(boost::filesystem::path dir, shrink_volume::fragment_file::vtr& fragment_files){
    bool result = false;
    boost::filesystem::path                dest_file;
    WIN32_FIND_DATAW                       FindFileData;
    macho::windows::auto_find_file_handle  hFind;
    DWORD                                  dwRet = ERROR_SUCCESS;
    ZeroMemory(&FindFileData, sizeof(FindFileData));
    boost::filesystem::path              file_name = dir / "*";
    hFind = FindFirstFileW(file_name.wstring().c_str(), &FindFileData);
    if (result = (hFind.is_valid())){
        do{
            DWORD dwRes = _tcscmp(_T("."), FindFileData.cFileName);
            DWORD dwRes1 = _tcscmp(_T(".."), FindFileData.cFileName);
            if (0 == dwRes || 0 == dwRes1)
                continue;
            dest_file = dir / FindFileData.cFileName;
            _check_file(dest_file, fragment_files, FindFileData.dwFileAttributes);
            if (FILE_ATTRIBUTE_DIRECTORY & FindFileData.dwFileAttributes){
                if (!(FILE_ATTRIBUTE_REPARSE_POINT & FindFileData.dwFileAttributes)){
                    if (!(result = _defragment(dest_file, fragment_files)))
                        break;
                }
            }
        } while (FindNextFile(hFind, &FindFileData) != 0);
    }
    return result;
}

bool shrink_volume::can_be_shrink(){
    bool result = false;
    if (_size > _new_size){
        if ((_size - _size_remaining) < _new_size)
            result = true;
    }
    else{
        result = true;
    }
    return result;
}

bool shrink_volume::shrink(){
    bool result = false;
    if ( _size > _new_size){
        if ((_size - _size_remaining) >= _new_size)
            return result;
        if ((result = shrink_prepare())){
            if((result = defragment()) && (result = shrink_commit())){
                result = grow_partition();
            }
            else{
                shrink_abort();
            }
        }
    }
    else{
        result = true;
    }
    return result;
}

shrink_volume::data_range::vtr shrink_volume::_get_data_ranges(LPBYTE buff, uint64_t start_lcn, uint64_t total_number_of_bits){
    data_range::vtr results;
    if (_file_area_offset)
        results.push_back(data_range(0, _file_area_offset));    
    uint64_t newBit, oldBit = 0, next_dirty_bit = 0;
    bool     dirty_range = false;
    for (newBit = 0; newBit < (uint64_t)total_number_of_bits; newBit++){
        if (buff[newBit >> 3] & (1 << (newBit & 7))){
            uint64_t num = (newBit - oldBit);
            if (!dirty_range){
                dirty_range = true;
                oldBit = next_dirty_bit = newBit;
            }
            else{
                next_dirty_bit = newBit;
            }
        }
        else {
            if (dirty_range){
                uint64_t lenght = (next_dirty_bit - oldBit + 1) * _bytes_per_cluster;
                results.push_back(data_range(_file_area_offset + ((start_lcn + oldBit) * _bytes_per_cluster), lenght));
                dirty_range = false;
            }
        }
    }
    if (dirty_range){
        results.push_back(data_range(_file_area_offset + ((start_lcn + oldBit) * _bytes_per_cluster), (next_dirty_bit - oldBit + 1) * _bytes_per_cluster));
    }
    return results;
}

bool shrink_volume::_move_file(HANDLE handle, uint64_t start_vcn, uint64_t start_lcn, DWORD count){
    DWORD BytesReturned = 0;
    MOVE_FILE_DATA data;
    memset(&data, 0, sizeof(MOVE_FILE_DATA));
    data.FileHandle = handle;
    data.StartingLcn.QuadPart = start_lcn;
    data.StartingVcn.QuadPart = start_vcn;
    data.ClusterCount = count;
    if (!DeviceIoControl((HANDLE)_handle,         // handle to volume
        (DWORD)FSCTL_MOVE_FILE, // dwIoControlCode
        (LPVOID)&data,      // MOVE_FILE_DATA structure
        (DWORD)sizeof(MOVE_FILE_DATA),   // size of input buffer
        (LPVOID)NULL,            // lpOutBuffer
        (DWORD)0,               // nOutBufferSize
        (LPDWORD)&BytesReturned, // number of bytes returned
        (LPOVERLAPPED)NULL)){  // OVERLAPPED structure
        LOG(LOG_LEVEL_ERROR, L"DeviceIoControl(FSCTL_MOVE_FILE) failed. Error: %d", GetLastError());
        return false;
    }
    return true;
}

bool shrink_volume::shrink_prepare(){
    DWORD BytesReturned = 0;
    SHRINK_VOLUME_INFORMATION volume_info;
    memset(&volume_info, 0, sizeof(SHRINK_VOLUME_INFORMATION));
    volume_info.ShrinkRequestType = ShrinkPrepare;
    volume_info.NewNumberOfSectors = _new_size >> 9;
    if (!DeviceIoControl((HANDLE)_handle,         // handle to volume
        (DWORD)FSCTL_SHRINK_VOLUME, // dwIoControlCode
        (LPVOID)&volume_info,      // MOVE_FILE_DATA structure
        (DWORD)sizeof(SHRINK_VOLUME_INFORMATION),   // size of input buffer
        (LPVOID)NULL,            // lpOutBuffer
        (DWORD)0,               // nOutBufferSize
        (LPDWORD)&BytesReturned, // number of bytes returned
        (LPOVERLAPPED)NULL)){  // OVERLAPPED structure
        LOG(LOG_LEVEL_ERROR, L"DeviceIoControl(FSCTL_SHRINK_VOLUME) failed. Error: %d", GetLastError());
        return false;
    }
    return true;
}

bool shrink_volume::shrink_commit(){
    DWORD BytesReturned = 0;
    SHRINK_VOLUME_INFORMATION volume_info;
    memset(&volume_info, 0, sizeof(SHRINK_VOLUME_INFORMATION));
    volume_info.ShrinkRequestType = ShrinkCommit;
    if (!DeviceIoControl((HANDLE)_handle,         // handle to volume
        (DWORD)FSCTL_SHRINK_VOLUME, // dwIoControlCode
        (LPVOID)&volume_info,      // MOVE_FILE_DATA structure
        (DWORD)sizeof(SHRINK_VOLUME_INFORMATION),   // size of input buffer
        (LPVOID)NULL,            // lpOutBuffer
        (DWORD)0,               // nOutBufferSize
        (LPDWORD)&BytesReturned, // number of bytes returned
        (LPOVERLAPPED)NULL)){  // OVERLAPPED structure
        LOG(LOG_LEVEL_ERROR, L"DeviceIoControl(FSCTL_SHRINK_VOLUME) failed. Error: %d", GetLastError());
        return false;
    }
    return true;
}

bool shrink_volume::shrink_abort(){
    DWORD BytesReturned = 0;
    SHRINK_VOLUME_INFORMATION volume_info;
    memset(&volume_info, 0, sizeof(SHRINK_VOLUME_INFORMATION));
    volume_info.ShrinkRequestType = ShrinkAbort;
    if (!DeviceIoControl((HANDLE)_handle,         // handle to volume
        (DWORD)FSCTL_SHRINK_VOLUME, // dwIoControlCode
        (LPVOID)&volume_info,      // MOVE_FILE_DATA structure
        (DWORD)sizeof(SHRINK_VOLUME_INFORMATION),   // size of input buffer
        (LPVOID)NULL,            // lpOutBuffer
        (DWORD)0,               // nOutBufferSize
        (LPDWORD)&BytesReturned, // number of bytes returned
        (LPOVERLAPPED)NULL)){  // OVERLAPPED structure
        LOG(LOG_LEVEL_ERROR, L"DeviceIoControl(FSCTL_SHRINK_VOLUME) failed. Error: %d", GetLastError());
        return false;
    }
    return true;
}

bool shrink_volume::grow_partition(){
    DWORD BytesReturned = 0;
    PARTITION_INFORMATION_EX partition_entry;
    memset(&partition_entry, 0, sizeof(PARTITION_INFORMATION_EX));
    if (!DeviceIoControl((HANDLE)_handle,         // handle to volume
        (DWORD)IOCTL_DISK_GET_PARTITION_INFO_EX, // dwIoControlCode
        (LPVOID)NULL,      // MOVE_FILE_DATA structure
        (DWORD)0,   // size of input buffer
        (LPVOID)&partition_entry,            // lpOutBuffer
        (DWORD)sizeof(PARTITION_INFORMATION_EX),               // nOutBufferSize
        (LPDWORD)&BytesReturned, // number of bytes returned
        (LPOVERLAPPED)NULL)){  // OVERLAPPED structure
        LOG(LOG_LEVEL_ERROR, L"DeviceIoControl(IOCTL_DISK_GET_PARTITION_INFO_EX) failed. Error: %d", GetLastError());
        return false;
    }
    DISK_GROW_PARTITION grow_partition;
    memset(&grow_partition, 0, sizeof(DISK_GROW_PARTITION));
    grow_partition.BytesToGrow.QuadPart = (_new_size - (ULONGLONG)partition_entry.PartitionLength.QuadPart);
    grow_partition.PartitionNumber = partition_entry.PartitionNumber;
    if (!DeviceIoControl((HANDLE)_handle,         // handle to volume
        (DWORD)IOCTL_DISK_GROW_PARTITION, // dwIoControlCode
        (LPVOID)&grow_partition,      // MOVE_FILE_DATA structure
        (DWORD)sizeof(DISK_GROW_PARTITION),   // size of input buffer
        (LPVOID)NULL,            // lpOutBuffer
        (DWORD)0,               // nOutBufferSize
        (LPDWORD)&BytesReturned, // number of bytes returned
        (LPOVERLAPPED)NULL)){  // OVERLAPPED structure
        LOG(LOG_LEVEL_ERROR, L"DeviceIoControl(IOCTL_DISK_GROW_PARTITION) failed. Error: %d", GetLastError());
        return false;
    }
    DISK_GEOMETRY geo;
    memset(&geo, 0, sizeof(DISK_GEOMETRY));
    if (!DeviceIoControl((HANDLE)_handle,         // handle to volume
        (DWORD)IOCTL_DISK_UPDATE_DRIVE_SIZE, // dwIoControlCode
        (LPVOID)NULL,      // MOVE_FILE_DATA structure
        (DWORD)0,   // size of input buffer
        (LPVOID)&geo,            // lpOutBuffer
        (DWORD)sizeof(DISK_GEOMETRY),               // nOutBufferSize
        (LPDWORD)&BytesReturned, // number of bytes returned
        (LPOVERLAPPED)NULL)){  // OVERLAPPED structure
        LOG(LOG_LEVEL_ERROR, L"DeviceIoControl(IOCTL_DISK_UPDATE_DRIVE_SIZE) failed. Error: %d", GetLastError());
        return false;
    }
    return true;
}

HANDLE shrink_volume::defrag_file_open(std::wstring file, DWORD attributes){
    HANDLE handle = INVALID_HANDLE_VALUE;
    UNICODE_STRING us;
    OBJECT_ATTRIBUTES oa;
    IO_STATUS_BLOCK iosb;
    NTSTATUS status;
    ACCESS_MASK access_rights = SYNCHRONIZE;
    ULONG flags = FILE_SYNCHRONOUS_IO_NONALERT;
    if (!file.empty()){        
        if ((attributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY){
            flags |= FILE_OPEN_FOR_BACKUP_INTENT;
        }
        else {
            flags |= FILE_NO_INTERMEDIATE_BUFFERING;
            if (IsWindowsVistaOrGreater())
                flags |= FILE_NON_DIRECTORY_FILE;
        }
        if ((FILE_ATTRIBUTE_REPARSE_POINT & attributes) == FILE_ATTRIBUTE_REPARSE_POINT){
            /* open the point itself, not its target */
            flags |= FILE_OPEN_REPARSE_POINT;
        }
        if (!IsWindowsVistaOrGreater()){
            /*
            * All files can be opened with a single SYNCHRONIZE access.
            * More advanced FILE_GENERIC_READ rights prevent opening
            * of $mft file as well as other internal NTFS files.
            * http://forum.sysinternals.com/topic23950.html
            */
        }
        else {
            /*
            * $Mft may require more advanced rights,
            * than a single SYNCHRONIZE.
            */
            access_rights |= FILE_READ_ATTRIBUTES;
        }
        std::wstring obj_name = boost::str(boost::wformat(L"\\??\\%s") % &file[4]);
        RtlInitUnicodeString(&us, obj_name.c_str());
        InitializeObjectAttributes(&oa, &us, 0, NULL, NULL);
        status = NtCreateFile(&handle, access_rights, &oa, &iosb, NULL, 0,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            FILE_OPEN, flags, NULL, 0);
#define STATUS_SUCCESS               0x0
#define STATUS_OBJECT_NAME_NOT_FOUND 0xC0000034
#define STATUS_ACCESS_DENIED         0xc0000022
        if (status != STATUS_SUCCESS && status != STATUS_OBJECT_NAME_NOT_FOUND){
            if (status == STATUS_ACCESS_DENIED)
                LOG(LOG_LEVEL_ERROR, L"NtCreateFile(%s) failed with access denied.", file.c_str());
            else
                LOG(LOG_LEVEL_ERROR, L"NtCreateFile(%s) failed. Error: 0x%08X", file.c_str(), status);
        }
    }
    return handle;
}

#define NtCloseSafe(h) { if(h) { NtClose(h); h = NULL; } }
void shrink_volume::defrag_file_close(HANDLE h){
    NtCloseSafe(h);
}

bool shrink_volume::_lookup_stream_from_clusters(shrink_volume::fragment_file::vtr& fragment_files){
    BOOL result = FALSE;
    DWORD BytesReturned = 0;
    data_range::vtr ranges = _get_used_space_ranges();
    std::set<uint64_t> clusters;
    foreach(data_range r, ranges){
        if ((r.start + r.length) >= _new_size){
            uint64_t start = (((r.start > _new_size) ? r.start : _new_size) - _file_area_offset) / _bytes_per_cluster;
            uint64_t end = ((r.start + r.length) - _file_area_offset) / _bytes_per_cluster;
            for (; start <= end; ++start)
                clusters.insert(start);
        }
    }
    if (!clusters.size()){
        result = TRUE;
    }
    else{
        DWORD out_size = 65536, in_size = sizeof(LOOKUP_STREAM_FROM_CLUSTER_INPUT) + (sizeof(LARGE_INTEGER) * (DWORD)clusters.size());
        std::auto_ptr<LOOKUP_STREAM_FROM_CLUSTER_INPUT> in((PLOOKUP_STREAM_FROM_CLUSTER_INPUT)(new BYTE[in_size]));
        memset(in.get(), 0, in_size);
        std::auto_ptr<BYTE> out(new BYTE[out_size]);
        memset(out.get(), 0, out_size);
        in.get()->NumberOfClusters = (DWORD)clusters.size();
        int i = 0;
        foreach(uint64_t c, clusters){
            in.get()->Cluster[i++].QuadPart = c;
        }
        while (!(result = DeviceIoControl((HANDLE)_handle,         // handle to volume
            (DWORD)FSCTL_LOOKUP_STREAM_FROM_CLUSTER, // dwIoControlCode
            (LPVOID)in.get(),      // MOVE_FILE_DATA structure
            (DWORD)in_size,   // size of input buffer
            (LPVOID)out.get(),            // lpOutBuffer
            (DWORD)out_size,               // nOutBufferSize
            (LPDWORD)&BytesReturned, // number of bytes returned
            (LPOVERLAPPED)NULL))){
            DWORD err = GetLastError();
            if (ERROR_MORE_DATA == err){
                out_size = ((PLOOKUP_STREAM_FROM_CLUSTER_OUTPUT)(LPVOID)out.get())->BufferSizeRequired;
                out = std::auto_ptr<BYTE>(new BYTE[out_size]);
                memset(out.get(), 0, out_size);
            }
            else{
                LOG(LOG_LEVEL_ERROR, L"DeviceIoControl(IOCTL_DISK_UPDATE_DRIVE_SIZE) failed. Error: %d", err);
                break;
            }
        }
        PLOOKUP_STREAM_FROM_CLUSTER_OUTPUT output = ((PLOOKUP_STREAM_FROM_CLUSTER_OUTPUT)(LPVOID)out.get());
        typedef std::map<std::wstring, std::vector<uint64_t>> files_clusters_map_type;
        files_clusters_map_type files_clusters_map;
        if (result && output->NumberOfMatches){
            PLOOKUP_STREAM_FROM_CLUSTER_ENTRY entry = (PLOOKUP_STREAM_FROM_CLUSTER_ENTRY)(&(out.get()[output->Offset]));
            do{
                switch (entry->Flags){
                case LOOKUP_STREAM_FROM_CLUSTER_ENTRY_FLAG_PAGE_FILE:
                    break;
                case LOOKUP_STREAM_FROM_CLUSTER_ENTRY_FLAG_DENY_DEFRAG_SET:
                    break;
                case LOOKUP_STREAM_FROM_CLUSTER_ENTRY_FLAG_FS_SYSTEM_FILE:
                    break;
                case LOOKUP_STREAM_FROM_CLUSTER_ENTRY_FLAG_TXF_SYSTEM_FILE:
                    break;
                case LOOKUP_STREAM_FROM_CLUSTER_ENTRY_ATTRIBUTE_DATA:
                    break;
                case LOOKUP_STREAM_FROM_CLUSTER_ENTRY_ATTRIBUTE_INDEX:
                    break;
                case LOOKUP_STREAM_FROM_CLUSTER_ENTRY_ATTRIBUTE_SYSTEM:
                    break;
                }
                files_clusters_map[entry->FileName].push_back(entry->Cluster.QuadPart);
                entry = (PLOOKUP_STREAM_FROM_CLUSTER_ENTRY)((PBYTE)entry + entry->OffsetToNext);
            } while (entry->OffsetToNext);
        }

        foreach(files_clusters_map_type::value_type &f, files_clusters_map){
            _check_file(boost::filesystem::path(_name) / f.first, fragment_files);
        }
    }
    return result == TRUE;
}

std::wstring shrink_volume::_get_filename_by_handle(HANDLE handle){
    DWORD name_length = MAX_PATH;
    DWORD cChars = 0;
    std::auto_ptr<WCHAR> name(new WCHAR[name_length]);
    memset(name.get(), 0, name_length * sizeof(WCHAR));
    while (!(cChars = GetFinalPathNameByHandleW(handle, name.get(), name_length, VOLUME_NAME_NONE))){
        if (ERROR_NOT_ENOUGH_MEMORY != GetLastError())
            break;
        name_length += MAX_PATH;
        name = std::auto_ptr<WCHAR>(new WCHAR[name_length]);
        memset(name.get(), 0, name_length * sizeof(WCHAR));
    }
    return cChars ? std::wstring(name.get(), cChars) : L"";
}