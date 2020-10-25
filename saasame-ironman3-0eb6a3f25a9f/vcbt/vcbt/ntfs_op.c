#include "ntfs_op.h"
#include "vcbt.h"
#include "trace.h"
#include "ioctl.h"
#include "ntfs_op.tmh"

#define BUFSIZE 4096
#define MAX_PATH 256
static BYTE chBuffer[BUFSIZE];

void remove_all_dataruns(PNTFS_ATTR attr);
PFILE_RECORD_HEADER read_file_record(const PNTFS_VOLUME volume, ULONGLONG fileRef);
bool patch_us(const PNTFS_VOLUME volume, WORD *sector, int sectors, WORD usn, WORD *usarray);
WORD get_volume_version(const PNTFS_FILE_RECORD ntfr);
bool is_compressed(const PNTFS_FILE_RECORD ntfr);
bool is_encrypted(const PNTFS_FILE_RECORD ntfr);
bool is_sparse(const PNTFS_FILE_RECORD ntfr);
bool parse_attrs(const PNTFS_VOLUME volume, PNTFS_FILE_RECORD ntfr, DWORD mask);
bool parse_attr(PNTFS_FILE_RECORD ntfr, PNTFS_ATTRIBUTE ahc);
bool parse_non_resident_attr(PNTFS_ATTR attr);
bool pick_data(const BYTE **dataRun, LONGLONG *length, LONGLONG *LCNOffset);
bool non_resident_attr_read_data(const PNTFS_VOLUME volume, const PNTFS_ATTR attr, const ULONGLONG offset, void *bufv, DWORD bufLen, DWORD *actural);
bool non_resident_attr_read_virtual_clusters(const PNTFS_VOLUME volume, const PNTFS_ATTR attr, ULONGLONG vcn, DWORD clusters, void *bufv, DWORD bufLen, DWORD *actural);
bool non_resident_attr_read_clusters(const PNTFS_VOLUME volume, void *buf, DWORD clusters, LONGLONG lcn);
bool visit_index_block(const PNTFS_VOLUME volume, const PNTFS_FILE_RECORD fr, const ULONGLONG vcn, const wchar_t *fileName, PULONGLONG fileRef);
void find_extent(PNTFS_ATTR attr, ULONGLONG startVNC, PLONGLONG nextVNC, PLONGLONG lcn);

void free_ntfs_volume(PNTFS_VOLUME *pv){
    if (*pv){
        if ((*pv)->mft){
            free_file_record(&(*pv)->mft);
        }
        //if ((*pv)->mft_data){
        //    remove_all_dataruns((*pv)->mft_data);
        //    __free((*pv)->mft_data);
        //}
        __free(*pv);
    }
}

void free_file_record(PNTFS_FILE_RECORD* ntfr){
    if (*ntfr){
        for (int i = 0; i < ATTR_NUMS; i++){
            PLIST_ENTRY e;
            while (&(*ntfr)->attrs[i] != (e = RemoveHeadList(&(*ntfr)->attrs[i]))){
                PNTFS_ATTR attr = CONTAINING_RECORD(e, NTFS_ATTR, bind);
                if (attr->ahc->NonResident)
                    remove_all_dataruns(attr);
                if (NULL != attr->attr_body)
                    __free(attr->attr_body);
                __free(attr);
            }
        }
        __free((*ntfr)->fr);
        __free(*ntfr);
    }
}

void remove_all_dataruns(PNTFS_ATTR attr){
    PLIST_ENTRY e;
    while (&attr->data_runs != (e = RemoveHeadList(&attr->data_runs))){
        PDATA_RUN_ENTRY datarun = CONTAINING_RECORD(e, DATA_RUN_ENTRY, bind);
        __free(datarun);
    }
}

PNTFS_VOLUME get_ntfs_volume_info(PDEVICE_OBJECT target){
    DWORD sector_size = BUFSIZE;
    memset(chBuffer, 0, BUFSIZE);
    NTSTATUS Status = get_sector_size(target, &sector_size);
    Status = fastFsdRequest(target, IRP_MJ_READ, 0, chBuffer, sector_size, TRUE);
    if (NT_SUCCESS(Status)){
        PPARTITION_BOOT_SECTOR pPartitionBootSector = (PPARTITION_BOOT_SECTOR)&chBuffer;
        // We only apply the change on windows file system partiton.
        if ((pPartitionBootSector->EndofSectorMarker == 0xAA55) &&
            (((pPartitionBootSector->OEM_Identifier[0] == 'N') &&
            (pPartitionBootSector->OEM_Identifier[1] == 'T') &&
            (pPartitionBootSector->OEM_Identifier[2] == 'F') &&
            (pPartitionBootSector->OEM_Identifier[3] == 'S')))) // NTFS
        {
            PNTFS_VOLUME volume = (PNTFS_VOLUME)__malloc(sizeof(NTFS_VOLUME));
            if (NULL != volume){
                memset(volume, 0, sizeof(NTFS_VOLUME));
                volume->target = target;
                volume->sector_size = pPartitionBootSector->BPB.BytesPerSector;
                volume->cluster_size = pPartitionBootSector->BPB.BytesPerSector * pPartitionBootSector->BPB.SectorsPerCluster;
                int sz = (char)pPartitionBootSector->ExtendedBPB.ClustersPerMFTRecord;
                if (sz > 0)
                    volume->file_record_size = volume->cluster_size * sz;
                else
                    volume->file_record_size = 1 << (-sz);
                sz = (char)pPartitionBootSector->ExtendedBPB.ClustersPerIndexBuffer;
                if (sz > 0)
                    volume->index_block_size = volume->cluster_size * sz;
                else
                    volume->index_block_size = 1 << (-sz);
                volume->mft_addr = pPartitionBootSector->ExtendedBPB.MFTFirstClusterNumber * volume->cluster_size;

                PNTFS_FILE_RECORD fs = get_file_record(volume, MFT_IDX_VOLUME, MASK_VOLUME_NAME | MASK_VOLUME_INFORMATION);
                if (fs){
                    volume->version = get_volume_version(fs);
                    free_file_record(&fs);

                    volume->mft = get_file_record(volume, MFT_IDX_MFT, MASK_DATA);
                    if (volume->mft){
                        //volume->mft_data = CONTAINING_RECORD(RemoveHeadList(&volume->mft->attrs[ATTR_INDEX(ATTR_TYPE_DATA)]), NTFS_ATTR, bind);
                        volume->mft_data = CONTAINING_RECORD(volume->mft->attrs[ATTR_INDEX(ATTR_TYPE_DATA)].Flink, NTFS_ATTR, bind);
                        if (NULL == volume->mft_data){
                            free_file_record(&volume->mft);
                            volume->mft = NULL;
                            DoTraceMsg(TRACE_LEVEL_INFORMATION, L"mft_data is NULL - DeviceObject %p", target);
                        }
                    }
                    else{
                        DoTraceMsg(TRACE_LEVEL_WARNING, L"Cannot find MFT_IDX_MFT info - DeviceObject %p", target);
                    }
                }
                else{
                    DoTraceMsg(TRACE_LEVEL_WARNING, L"Cannot find MFT_IDX_VOLUME info - DeviceObject %p", target);
                }
                return volume;
            }
        }
    }
    return NULL;
}

PNTFS_FILE_RECORD get_file_record(const PNTFS_VOLUME volume, ULONGLONG fileRef, DWORD mask){
    PNTFS_FILE_RECORD ntfr = NULL;
    mask = mask | MASK_STANDARD_INFORMATION | MASK_ATTRIBUTE_LIST;
    PFILE_RECORD_HEADER fr = read_file_record(volume, fileRef);
    if (NULL != fr){
        if (fr->Magic == FILE_RECORD_MAGIC){
            // Patch US
            WORD *usnaddr = (WORD*)((BYTE*)fr + fr->OffsetOfUS);
            WORD usn = *usnaddr;
            WORD *usarray = usnaddr + 1;
            if (true == patch_us(volume, (WORD*)fr, volume->file_record_size / volume->sector_size, usn, usarray)){
                ntfr = (PNTFS_FILE_RECORD)__malloc(sizeof(NTFS_FILE_RECORD));
                if (NULL != ntfr){
                    memset(ntfr, 0, sizeof(NTFS_FILE_RECORD));
                    ntfr->fr = fr;
                    ntfr->fileRef = fileRef;
                    for (int i = 0; i < ATTR_NUMS; i++)
                        InitializeListHead(&ntfr->attrs[i]);
                    if (true == parse_attrs(volume, ntfr, mask))
                        return ntfr;
                    else{
                        free_file_record(&ntfr);
                        ntfr = NULL;
                    }
                }
            }
        }
        __free(fr);
    }
    return ntfr;
}

PFILE_RECORD_HEADER read_file_record(const PNTFS_VOLUME volume, ULONGLONG fileRef){
    PFILE_RECORD_HEADER fr = NULL;
    DWORD len = 0;
    if (fileRef < MFT_IDX_USER || volume->mft_data == NULL){
        // Take as continuous disk allocation
        LONGLONG addr = volume->mft_addr + (volume->file_record_size) * fileRef;
        fr = (PFILE_RECORD_HEADER)__malloc(volume->file_record_size);
        if (NULL != fr){
            memset(fr, 0, volume->file_record_size);
            if (NT_SUCCESS(fastFsdRequest(volume->target, IRP_MJ_READ, addr, fr, volume->file_record_size, TRUE)))
                return fr;
        }
    }
    else
    {  
        //// May be fragmented $MFT
        ULONGLONG frAddr;
        frAddr = (volume->file_record_size) * fileRef;
        fr = (PFILE_RECORD_HEADER)__malloc(volume->file_record_size);
        if (NULL != fr){
            memset(fr, 0, volume->file_record_size);
            if (non_resident_attr_read_data(volume, volume->mft_data, frAddr, fr, volume->file_record_size, &len) &&
                len == volume->file_record_size)
                return fr;
        }
    }
    __free(fr);
    return fr;
}

bool patch_us(const PNTFS_VOLUME volume, WORD *sector, int sectors, WORD usn, WORD *usarray){
    int i;
    for (i = 0; i<sectors; i++){
        sector += ((volume->sector_size >> 1) - 1);
        if (*sector != usn)
            return false;	// USN error
        *sector = usarray[i];	// Write back correct data
        sector++;
    }
    return true;
}

ULONGLONG  ntfs_get_file_clusters(PNTFS_FILE_RECORD ntfr){
    if (FALSE == IsListEmpty(&ntfr->attrs[ATTR_INDEX(ATTR_TYPE_DATA)])){
        PNTFS_ATTR attr = CONTAINING_RECORD(ntfr->attrs[ATTR_INDEX(ATTR_TYPE_DATA)].Flink, NTFS_ATTR, bind);
        if (attr->ahc->NonResident){
            LIST_ENTRY* entry;
            ULONGLONG last_vnc = 0;
            int depth = 0;
            LIST_FOR_EACH(entry, &attr->data_runs){
                PDATA_RUN_ENTRY dr = CONTAINING_RECORD(entry, DATA_RUN_ENTRY, bind);
                if (depth == 0)
                    last_vnc = dr->last_vcn;
                else{
                    if (last_vnc < dr->last_vcn)
                        last_vnc = dr->last_vcn;
                }
                depth++;
            }
            return last_vnc + 1;
        }
    }
    return 0;
}

bool is_sparse(const PNTFS_FILE_RECORD ntfr){  
    if (FALSE == IsListEmpty(&ntfr->attrs[ATTR_INDEX(ATTR_TYPE_STANDARD_INFORMATION)])){
        PLIST_ENTRY entry = ntfr->attrs[ATTR_INDEX(ATTR_TYPE_STANDARD_INFORMATION)].Flink;
        PNTFS_ATTR attr = CONTAINING_RECORD(entry, NTFS_ATTR, bind);
        PATTR_STANDARD_INFORMATION stdInfo = (PATTR_STANDARD_INFORMATION)attr->attr_body;
        return stdInfo->Permission & ATTR_STDINFO_PERMISSION_SPARSE;
    }
    return false;
}

bool is_encrypted(const PNTFS_FILE_RECORD ntfr){
    if (FALSE == IsListEmpty(&ntfr->attrs[ATTR_INDEX(ATTR_TYPE_STANDARD_INFORMATION)])){
        PLIST_ENTRY entry = ntfr->attrs[ATTR_INDEX(ATTR_TYPE_STANDARD_INFORMATION)].Flink;
        PNTFS_ATTR attr = CONTAINING_RECORD(entry, NTFS_ATTR, bind);
        PATTR_STANDARD_INFORMATION stdInfo = (PATTR_STANDARD_INFORMATION)attr->attr_body;
        return stdInfo->Permission & ATTR_STDINFO_PERMISSION_ENCRYPTED;
    }
    return false;
}

bool is_compressed(const PNTFS_FILE_RECORD ntfr){
    if (FALSE == IsListEmpty(&ntfr->attrs[ATTR_INDEX(ATTR_TYPE_STANDARD_INFORMATION)])){
        PLIST_ENTRY entry = ntfr->attrs[ATTR_INDEX(ATTR_TYPE_STANDARD_INFORMATION)].Flink;
        PNTFS_ATTR attr = CONTAINING_RECORD(entry, NTFS_ATTR, bind);
        PATTR_STANDARD_INFORMATION stdInfo = (PATTR_STANDARD_INFORMATION)attr->attr_body;
        return stdInfo->Permission & ATTR_STDINFO_PERMISSION_COMPRESSED;
    }
    return false;
}

#define MAKEWORD(a, b)      ((WORD)(((BYTE)(((DWORD_PTR)(a)) & 0xff)) | ((WORD)((BYTE)(((DWORD_PTR)(b)) & 0xff))) << 8))
WORD get_volume_version(const PNTFS_FILE_RECORD ntfr){
    WORD version = 0;
    if (FALSE == IsListEmpty(&ntfr->attrs[ATTR_INDEX(ATTR_TYPE_VOLUME_INFORMATION)])){
        PLIST_ENTRY entry = ntfr->attrs[ATTR_INDEX(ATTR_TYPE_VOLUME_INFORMATION)].Flink;
        PNTFS_ATTR attr = CONTAINING_RECORD(entry, NTFS_ATTR, bind);
        PATTR_VOLUME_INFORMATION attr_body = (PATTR_VOLUME_INFORMATION)attr->attr_body;
        return MAKEWORD(attr_body->MinorVersion, attr_body->MajorVersion);
    }
    return version;
}

bool parse_attrs(const PNTFS_VOLUME volume, PNTFS_FILE_RECORD ntfr, DWORD mask){
    DWORD dataPtr = 0;	// guard if data exceeds FileRecordSize bounds
    PNTFS_ATTRIBUTE ahc = (PNTFS_ATTRIBUTE)((BYTE*)ntfr->fr + ntfr->fr->OffsetOfAttr);
    dataPtr += ntfr->fr->OffsetOfAttr;
    while (ahc->Type != (DWORD)-1 && (dataPtr + ahc->TotalSize) <= volume->file_record_size){
        if (ATTR_MASK(ahc->Type) & mask)	// Skip unwanted attributes
        {
            if (!parse_attr(ntfr, ahc))	// Parse error
                return false;
            if (is_encrypted(ntfr) || is_compressed(ntfr)){
                DoTraceMsg(TRACE_LEVEL_VERBOSE, L"Compressed and Encrypted file not supported yet!");
                return false;
            }
        }
        dataPtr += ahc->TotalSize;
        ahc = (PNTFS_ATTRIBUTE)((BYTE*)ahc + ahc->TotalSize);	// next attribute
    }
    return true;
}

bool parse_attr(PNTFS_FILE_RECORD ntfr, PNTFS_ATTRIBUTE ahc){
    DWORD attrIndex = ATTR_INDEX(ahc->Type);
    if (attrIndex < ATTR_NUMS){
        PNTFS_ATTR attr = (PNTFS_ATTR)__malloc(sizeof(NTFS_ATTR));
        if (NULL != attr){
            memset(attr, 0, sizeof(NTFS_ATTR));
            attr->ahc = ahc;
            InitializeListHead(&attr->data_runs);
            if (ahc->NonResident){              
                if (!parse_non_resident_attr(attr)){
                    remove_all_dataruns(attr);
                    __free(attr);
                    return false;
                }
            }
            else{
                PATTR_VOLUME_INFORMATION attr_body = (PATTR_VOLUME_INFORMATION)(void*)((BYTE*)ahc + (ahc->Attr.Resident.AttrOffset));
                attr->attr_body_size = ahc->Attr.Resident.AttrSize;
                attr->attr_body = __malloc(attr->attr_body_size);
                if (NULL != attr->attr_body){
                    memset(attr->attr_body, 0, attr->attr_body_size);
                    memcpy(attr->attr_body, attr_body, attr->attr_body_size);
                }
            }
            InsertTailList(&ntfr->attrs[attrIndex], &attr->bind);
            return true;
        }
        else{
            DoTraceMsg(TRACE_LEVEL_ERROR, L"Attribute Parse error: 0x%04X", ahc->Type);
        }
    }
    else{
        DoTraceMsg(TRACE_LEVEL_ERROR, L"Invalid Attribute Type: 0x%04X", ahc->Type);       
    }
    return false;
}

bool parse_non_resident_attr(PNTFS_ATTR attr){
    const BYTE *dataRun = (BYTE*)attr->ahc + attr->ahc->Attr.NonResident.DataRunOffset;
    LONGLONG length;
    LONGLONG LCNOffset;
    LONGLONG LCN = 0;
    ULONGLONG VCN = 0;
    while (*dataRun){
        if (pick_data(&dataRun, &length, &LCNOffset)){
            LCN += LCNOffset;
            if (LCN < 0) {
                return false;
            }
            // NTFS_TRACE2("Data length = %I64d clusters, LCN = %I64d", length, LCN);
            // NTFS_TRACE(LCNOffset == 0 ? ", Sparse Data\n" : "\n");
            // Store LCN, Data size (clusters) into list
            PDATA_RUN_ENTRY dr = (PDATA_RUN_ENTRY)__malloc(sizeof(DATA_RUN_ENTRY));
            if (NULL != dr){
                memset(dr, 0, sizeof(DATA_RUN_ENTRY));
                dr->lcn = (LCNOffset == 0) ? -1 : LCN;
                dr->clusters = length;
                dr->start_vcn = VCN;
                VCN += length;
                dr->last_vcn = VCN - 1;
                if (dr->last_vcn <= (attr->ahc->Attr.NonResident.LastVCN - attr->ahc->Attr.NonResident.StartVCN)){
                    InsertTailList(&attr->data_runs, &dr->bind);
                }
                else{
                    DoTraceMsg(TRACE_LEVEL_ERROR, L"DataRun decode error: VCN exceeds bound");
                    return false;
                }
            }
            else
                return false;
        }
        else
            break;
    }
    return true;
}

bool pick_data(const BYTE **dataRun, LONGLONG *length, LONGLONG *LCNOffset){
    BYTE size = **dataRun;
    int lengthBytes = size & 0x0F;
    int offsetBytes = size >> 4;

    (*dataRun)++;

    if (lengthBytes > 8 || offsetBytes > 8){
        DoTraceMsg(TRACE_LEVEL_ERROR, L"DataRun decode error 1: 0x%02X", size);
        return false;
    }
    *length = 0;
    memcpy(length, *dataRun, lengthBytes);
    if (*length < 0){
        DoTraceMsg(TRACE_LEVEL_ERROR, L"DataRun length error: %I64d", *length);
        return false;
    }

    (*dataRun) += lengthBytes;
    *LCNOffset = 0;
    if (offsetBytes)	// Not Sparse File
    {
        if ((*dataRun)[offsetBytes - 1] & 0x80)
            *LCNOffset = -1;
        memcpy(LCNOffset, *dataRun, offsetBytes);
        (*dataRun) += offsetBytes;
    }
    return true;
}

bool non_resident_attr_read_data(const PNTFS_VOLUME volume, const PNTFS_ATTR attr, const ULONGLONG offset, void *bufv, DWORD bufLen, DWORD *actural){
    // Hard disks can only be accessed by sectors
    // To be simple and efficient, only implemented cluster based accessing
    // So cluster unaligned data address should be processed carefully here

    *actural = 0;
    if (bufLen == 0)
        return true;

    // Bounds check
    if (offset > attr->ahc->Attr.NonResident.RealSize)
        return false;
    if ((offset + bufLen) > attr->ahc->Attr.NonResident.RealSize)
        bufLen = (DWORD)(attr->ahc->Attr.NonResident.RealSize - offset);

    DWORD len;
    BYTE *buf = (BYTE*)bufv;

    // First cluster Number
    ULONGLONG startVCN = offset / volume->cluster_size;
    // Bytes in first cluster
    DWORD startBytes = volume->cluster_size - (DWORD)(offset % volume->cluster_size);
    // Read first cluster
    BYTE* UnalignedBuf = __malloc(volume->cluster_size);
    if (NULL != UnalignedBuf){
        memset(UnalignedBuf, 0, volume->cluster_size);
        if (startBytes != volume->cluster_size){
            // First cluster, Unaligned
            if (non_resident_attr_read_virtual_clusters(volume, attr, startVCN, 1, UnalignedBuf, volume->cluster_size, &len)
                && len == volume->cluster_size){
                len = (startBytes < bufLen) ? startBytes : bufLen;
                memcpy(buf, UnalignedBuf + volume->cluster_size - startBytes, len);
                buf += len;
                bufLen -= len;
                *actural += len;
                startVCN++;
            }
            else{
                __free(UnalignedBuf);
                return false;
            }
        }
    }
    if (bufLen == 0){
        __free(UnalignedBuf);
        return true;
    }

    DWORD alignedClusters = bufLen / volume->cluster_size;
    if (alignedClusters){
        // Aligned clusters
        DWORD alignedSize = alignedClusters*volume->cluster_size;
        if (non_resident_attr_read_virtual_clusters(volume, attr, startVCN, alignedClusters, buf, alignedSize, &len)
            && len == alignedSize){
            startVCN += alignedClusters;
            buf += alignedSize;
            bufLen %= volume->cluster_size;
            *actural += len;

            if (bufLen == 0){
                __free(UnalignedBuf);
                return true;
            }
        }
        else{
            __free(UnalignedBuf);
            return false;
        }
    }

    // Last cluster, Unaligned
    if (non_resident_attr_read_virtual_clusters(volume, attr, startVCN, 1, UnalignedBuf, volume->cluster_size, &len)
        && len == volume->cluster_size){
        memcpy(buf, UnalignedBuf, bufLen);
        *actural += bufLen;
        __free(UnalignedBuf);
        return true;
    }
    __free(UnalignedBuf);
    return false;
}

bool non_resident_attr_read_virtual_clusters(const PNTFS_VOLUME volume, const PNTFS_ATTR attr, ULONGLONG vcn, DWORD clusters,
    void *bufv, DWORD bufLen, DWORD *actural){

    *actural = 0;
    BYTE *buf = (BYTE*)bufv;

    // Verify if clusters exceeds DataRun bounds
    if (vcn + clusters > (attr->ahc->Attr.NonResident.LastVCN - attr->ahc->Attr.NonResident.StartVCN + 1)){
        DoTraceMsg(TRACE_LEVEL_ERROR, L"Cluster exceeds DataRun bounds");
        return false;
    }

    // Verify buffer size
    if (bufLen < clusters*volume->cluster_size){
        DoTraceMsg(TRACE_LEVEL_ERROR, L"Buffer size too small");
        return false;
    }

    // Traverse the DataRun List to find the according LCN
    LIST_ENTRY* entry;
    LIST_FOR_EACH(entry, &attr->data_runs){
        PDATA_RUN_ENTRY dr = CONTAINING_RECORD(entry, DATA_RUN_ENTRY, bind);;
        if (vcn >= dr->start_vcn && vcn <= dr->last_vcn){
            DWORD clustersToRead;
            ULONGLONG vcns = dr->last_vcn - vcn + 1;	// Clusters from read pointer to the end
            if ((ULONGLONG)clusters > vcns)	// Fragmented data, we must go on
                clustersToRead = (DWORD)vcns;
            else
                clustersToRead = clusters;
            if (non_resident_attr_read_clusters(volume, buf, clustersToRead, dr->lcn + (vcn - dr->start_vcn))){
                buf += clustersToRead * volume->cluster_size;
                clusters -= clustersToRead;
                *actural += clustersToRead;
                vcn += clustersToRead;
            }
            else
                break;

            if (clusters == 0)
                break;
        }
    }

    *actural *= volume->cluster_size;
    return true;
}

bool non_resident_attr_read_clusters(const PNTFS_VOLUME volume, void *buf, DWORD clusters, LONGLONG lcn){
    if (lcn == -1){	// sparse data
        DoTraceMsg(TRACE_LEVEL_VERBOSE, L"Sparse Data, Fill the buffer with 0");
        // Fill the buffer with 0
        memset(buf, 0, clusters * volume->cluster_size);
        return true;
    }
    LONGLONG addr = lcn * volume->cluster_size;
    if (NT_SUCCESS(fastFsdRequest(volume->target, IRP_MJ_READ, (LONGLONG)addr, buf, clusters*volume->cluster_size, TRUE))){
       // DoTraceMsg(TRACE_LEVEL_VERBOSE, L"Successfully read %u clusters from LCN %I64d", clusters, lcn);
        return true;
    }
    else{
        DoTraceMsg(TRACE_LEVEL_ERROR, L"Cannot read cluster with LCN %I64d", lcn);
    }
    return false;
}

bool find_sub_entry(const PNTFS_VOLUME volume, const PNTFS_FILE_RECORD fr, const wchar_t *fileName, PULONGLONG fileRef){   
    LIST_ENTRY* entry;
    LIST_FOR_EACH(entry, &fr->attrs[ATTR_INDEX(ATTR_TYPE_INDEX_ROOT)]){
        PNTFS_ATTR attr = CONTAINING_RECORD(entry, NTFS_ATTR, bind);
        PATTR_INDEX_ROOT IndexRoot = (PATTR_INDEX_ROOT)attr->attr_body;
        if (IndexRoot->AttrType == ATTR_TYPE_FILE_NAME){
            PINDEX_ENTRY ie;
            ie = (PINDEX_ENTRY)((BYTE*)(&(IndexRoot->EntryOffset)) + IndexRoot->EntryOffset);
            DWORD ieTotal = ie->Size;
            while (ieTotal <= IndexRoot->TotalEntrySize){
                if (ie->Flags & INDEX_ENTRY_FLAG_SUBNODE){                  
                    if (visit_index_block(volume, fr, *(ULONGLONG*)((BYTE*)ie + ie->Size - 8), fileName, fileRef)){
                        return true;
                    }
                }
                if (ie->StreamSize){
                    PATTR_FILE_NAME filename = (PATTR_FILE_NAME)ie->Stream;
#if __DEBUG_NTFS     
                    wchar_t fns[MAX_PATH];
                    memset(fns, 0, sizeof(fns));
                    wcsncpy_s(fns, MAX_PATH,(wchar_t*)filename->Name, filename->NameLength);
                    DoTraceMsg(TRACE_LEVEL_VERBOSE, L"%ws", fns);
#endif
                    if ((wcslen(fileName) == filename->NameLength) &&
                        (0 == _wcsnicmp((wchar_t*)filename->Name, fileName, filename->NameLength))){
                        *fileRef = ie->FileReference & 0x0000FFFFFFFFFFFFUL;
                        return true;
                    }
                }
                if (ie->Flags & INDEX_ENTRY_FLAG_LAST){
                    break;
                }
                ie = (PINDEX_ENTRY)(((BYTE*)ie) + ie->Size);	// Pick next
                ieTotal += ie->Size;
            }
        }
    }
    return false;
}

bool visit_index_block(const PNTFS_VOLUME volume, const PNTFS_FILE_RECORD fr, const ULONGLONG vcn, const wchar_t *fileName, PULONGLONG fileRef){

    if (FALSE == IsListEmpty(&fr->attrs[ATTR_INDEX(ATTR_TYPE_INDEX_ALLOCATION)])){
        PNTFS_ATTR attr = CONTAINING_RECORD(fr->attrs[ATTR_INDEX(ATTR_TYPE_INDEX_ALLOCATION)].Flink, NTFS_ATTR, bind);
        PINDEX_BLOCK ibBuf = (PINDEX_BLOCK)__malloc(volume->index_block_size);
        if (NULL != ibBuf){
            memset(ibBuf, 0, volume->index_block_size);
            DWORD len;
            DWORD sectors = volume->index_block_size / volume->sector_size;
            if (non_resident_attr_read_data(volume, attr, vcn *volume->index_block_size, ibBuf, volume->index_block_size, &len) &&
                len == volume->index_block_size){
                if (ibBuf->Magic != INDEX_BLOCK_MAGIC){
                    __free(ibBuf);
                    return false;
                }
                // Patch US
                WORD *usnaddr = (WORD*)((BYTE*)ibBuf + ibBuf->OffsetOfUS);
                WORD usn = *usnaddr;
                WORD *usarray = usnaddr + 1;
                if (!patch_us(volume, (WORD*)ibBuf, sectors, usn, usarray)){
                    DoTraceMsg(TRACE_LEVEL_ERROR, L"Index Block parse error: Update Sequence Number\n");
                    __free(ibBuf);
                    return false;
                }
                INDEX_ENTRY *ie;
                ie = (INDEX_ENTRY*)((BYTE*)(&(ibBuf->EntryOffset)) + ibBuf->EntryOffset);
                DWORD ieTotal = ie->Size;
                while (ieTotal <= ibBuf->TotalEntrySize){
                    if (ie->Flags & INDEX_ENTRY_FLAG_SUBNODE){
                        if (visit_index_block(volume, fr, *(ULONGLONG*)((BYTE*)ie + ie->Size - 8), fileName, fileRef)){
                            __free(ibBuf);
                            return true;
                        }
                    }
                    if (ie->StreamSize){
                        PATTR_FILE_NAME filename = (PATTR_FILE_NAME)ie->Stream;
#if __DEBUG_NTFS                   
                        wchar_t fns[MAX_PATH];
                        memset(fns, 0, sizeof(fns));
                        wcsncpy_s(fns, MAX_PATH, (wchar_t*)filename->Name, filename->NameLength);
                        DoTraceMsg(TRACE_LEVEL_VERBOSE, L"%ws", fns);
#endif
                        if ((wcslen(fileName) == filename->NameLength) &&
                            (0 == _wcsnicmp((wchar_t*)filename->Name, fileName, filename->NameLength))){
                            *fileRef = ie->FileReference & 0x0000FFFFFFFFFFFFUL;
                            __free(ibBuf);
                            return true;
                        }
                        }
                    if (ie->Flags & INDEX_ENTRY_FLAG_LAST){
                        break;
                    }
                    ie = (INDEX_ENTRY*)((BYTE*)ie + ie->Size);	// Pick next
                    ieTotal += ie->Size;
                }
            }
            __free(ibBuf);
        }
    }
    return false;
}

void find_extent(PNTFS_ATTR attr, ULONGLONG startVNC, PLONGLONG nextVNC, PLONGLONG lcn){
    LIST_ENTRY* entry;
    LIST_FOR_EACH(entry, &attr->data_runs){
        PDATA_RUN_ENTRY dr = CONTAINING_RECORD(entry, DATA_RUN_ENTRY, bind);
        if (dr->start_vcn == startVNC){
            *nextVNC = dr->last_vcn + 1;
            *lcn = dr->lcn;
            break;
        }
    }
}

PRETRIEVAL_POINTERS_BUFFER ntfs_get_file_retrieval_pointers(PNTFS_FILE_RECORD fr){
    if (FALSE == IsListEmpty(&fr->attrs[ATTR_INDEX(ATTR_TYPE_DATA)])){
        PNTFS_ATTR attr = CONTAINING_RECORD(fr->attrs[ATTR_INDEX(ATTR_TYPE_DATA)].Flink, NTFS_ATTR, bind);
        if (attr->ahc->NonResident){
            LIST_ENTRY* entry;
            ULONGLONG start_vnc = 0;
            int depth = 0;
            LIST_FOR_EACH(entry, &attr->data_runs){         
                PDATA_RUN_ENTRY dr = CONTAINING_RECORD(entry, DATA_RUN_ENTRY, bind);
                if ( depth == 0 )
                    start_vnc = dr->start_vcn;
                else{
                    if (start_vnc > dr->start_vcn)
                        start_vnc = dr->start_vcn;
                }
                depth++;
            }
            size_t s = sizeof(LARGE_INTEGER) *depth * 2 + sizeof(RETRIEVAL_POINTERS_BUFFER);
            PRETRIEVAL_POINTERS_BUFFER p = (PRETRIEVAL_POINTERS_BUFFER)__malloc(s);
            if (NULL != p){
                memset(p, 0, s);
                p->ExtentCount = depth;

                p->StartingVcn.QuadPart = 0;
                ULONGLONG startVNC = start_vnc;
                for (ULONG i = 0; i < p->ExtentCount; i++){
                    find_extent(attr, startVNC, &p->Extents[i].NextVcn.QuadPart, &p->Extents[i].Lcn.QuadPart);
                    startVNC = p->Extents[i].NextVcn.QuadPart;
                }
                return p;
            }
        }
    }
    return NULL;
}

bool is_deleted(PNTFS_FILE_RECORD fr){
    return !(fr->fr->Flags  & FILE_RECORD_FLAG_INUSE);
}

bool is_directory(PNTFS_FILE_RECORD fr){
    return (fr->fr->Flags  & FILE_RECORD_FLAG_DIR);
}