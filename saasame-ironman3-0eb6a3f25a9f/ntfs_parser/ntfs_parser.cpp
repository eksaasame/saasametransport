#include <Windows.h>
#include "..\irm_converter\irm_disk.h"
#include "ntfs_parser.h"
#include <macho.h>

using namespace ntfs;
using namespace macho;
volume::vtr volume::get(universal_disk_rw::ptr rw){
    std::string buf;
    volume::vtr results;
#define BUFSIZE 512
    if (rw->read(0, BUFSIZE * 34, buf)){
        PLEGACY_MBR                pLegacyMBR = (PLEGACY_MBR)&buf[0];
        if (pLegacyMBR->Signature == 0xAA55){
            PGPT_PARTITIONTABLE_HEADER pGptPartitonHeader = (PGPT_PARTITIONTABLE_HEADER)&buf[BUFSIZE];
            if ((pLegacyMBR->PartitionRecord[0].PartitionType == 0xEE) && (pGptPartitonHeader->Signature == 0x5452415020494645)){
                PGPT_PARTITIONTABLE_HEADER                     pGptPartitonHeader = (PGPT_PARTITIONTABLE_HEADER)&buf[BUFSIZE];
                PGPT_PARTITION_ENTRY                           pGptPartitionEntries = (PGPT_PARTITION_ENTRY)&buf[BUFSIZE * pGptPartitonHeader->PartitionEntryLBA];
                for (int index = 0; index < pGptPartitonHeader->NumberOfPartitionEntries; index++){
                    volume::ptr v = get(rw, ((uint64_t)pGptPartitionEntries[index].StartingLBA) * BUFSIZE);
                    if (v) results.push_back(v);
                }
            }
            else{
                for (int index = 0; index < 4; index++){
                    uint64_t start = ((uint64_t)pLegacyMBR->PartitionRecord[index].StartingLBA) * BUFSIZE;
                    if (pLegacyMBR->PartitionRecord[index].PartitionType == PARTITION_IFS){
                        volume::ptr v = get(rw, start);
                        if (v) results.push_back(v);
                    }
                    else if (pLegacyMBR->PartitionRecord[index].PartitionType == PARTITION_XINT13_EXTENDED ||
                        pLegacyMBR->PartitionRecord[index].PartitionType == PARTITION_EXTENDED){
                        std::string buf2;
                        bool loop = true;
                        while (loop){
                            loop = false;
                            if (rw->read(start, BUFSIZE, buf2)){
                                PLEGACY_MBR pMBR = (PLEGACY_MBR)&buf2[0];
                                if (pMBR->Signature != 0xAA55)
                                    break;
                                for (int i = 0; i < 2; i++){
                                    if (pMBR->PartitionRecord[i].PartitionType == PARTITION_XINT13_EXTENDED ||
                                        pMBR->PartitionRecord[i].PartitionType == PARTITION_EXTENDED){
                                        start = ((uint64_t)((uint64_t)pLegacyMBR->PartitionRecord[index].StartingLBA + (uint64_t)pMBR->PartitionRecord[i].StartingLBA) * BUFSIZE);
                                        loop = true;
                                        break;
                                    }
                                    else if (pMBR->PartitionRecord[i].StartingLBA != 0 && pMBR->PartitionRecord[i].SizeInLBA != 0){
                                        if (pMBR->PartitionRecord[i].PartitionType == PARTITION_IFS){
                                            volume::ptr v = get(rw, start + ((uint64_t)pMBR->PartitionRecord[i].StartingLBA * BUFSIZE));
                                            if (v) results.push_back(v);
                                        }
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
        }
    }
    return results;
}

volume::ptr volume::get(universal_disk_rw::ptr& rw, ULONGLONG _offset){
    volume::ptr v;
    std::string buf;
    if (rw->read(_offset, 512, buf)){
        PPARTITION_BOOT_SECTOR pPartitionBootSector = (PPARTITION_BOOT_SECTOR)&buf[0];
        // We only apply the change on windows file system partiton.
        if ((pPartitionBootSector->EndofSectorMarker == 0xAA55) &&
            (((pPartitionBootSector->OEM_Identifier[0] == 'N') &&
            (pPartitionBootSector->OEM_Identifier[1] == 'T') &&
            (pPartitionBootSector->OEM_Identifier[2] == 'F') &&
            (pPartitionBootSector->OEM_Identifier[3] == 'S')))) // NTFS
        {
            v = volume::ptr(new volume(rw, _offset));
            if (NULL != v){
                v->sector_size = pPartitionBootSector->BPB.BytesPerSector;
                v->cluster_size = pPartitionBootSector->BPB.BytesPerSector * pPartitionBootSector->BPB.SectorsPerCluster;
                int sz = (char)(pPartitionBootSector->ExtendedBPB.ClustersPerMFTRecord);
                if (sz > 0)
                    v->file_record_size = v->cluster_size * sz;
                else
                    v->file_record_size = 1 << (-sz);
                sz = (char)pPartitionBootSector->ExtendedBPB.ClustersPerIndexBuffer;
                if (sz > 0)
                    v->index_block_size = v->cluster_size * sz;
                else
                    v->index_block_size = 1 << (-sz);
                v->mft_addr = pPartitionBootSector->ExtendedBPB.MFTFirstClusterNumber * v->cluster_size;
                v->total_size = pPartitionBootSector->ExtendedBPB.TotalSectors * v->sector_size;

                file_record::ptr fs = v->get_file_record( MFT_IDX_VOLUME, MASK_VOLUME_NAME | MASK_VOLUME_INFORMATION);
                if (fs){
                    v->version = v->get_volume_version(*fs.get());
                    v->mft = v->get_file_record(MFT_IDX_MFT, MASK_DATA);
                    if (v->mft){
                        v->mft_data = v->mft->attrs[ATTR_INDEX(ATTR_TYPE_DATA)].at(0);
                        if (!v->mft_data)
                            v->mft = NULL;
                    }
                    else{
                        LOG(LOG_LEVEL_WARNING, L"Cannot find MFT file info.");
                    }
                }
                else{
                    LOG(LOG_LEVEL_WARNING, L"Cannot find volume info.");
                }
            }
        }
    }
    return v;
}

file_record::ptr volume::get_file_record(ULONGLONG fileRef, std::wstring name, DWORD mask){
    file_record::ptr ntfr = NULL;
    mask = mask | MASK_STANDARD_INFORMATION | MASK_ATTRIBUTE_LIST;
    boost::shared_ptr<FILE_RECORD_HEADER> fr = read_file_record(fileRef);
    if (NULL != fr){
        if (fr->Magic == FILE_RECORD_MAGIC){
            // Patch US
            WORD *usnaddr = (WORD*)((BYTE*)fr.get() + fr->OffsetOfUS);
            WORD usn = *usnaddr;
            WORD *usarray = usnaddr + 1;
            if (true == patch_us((WORD*)fr.get(), file_record_size / sector_size, usn, usarray)){
                ntfr = file_record::ptr(new file_record(fileRef, fr));
                if (ntfr){
                    ntfr->name = name;
                    ntfr->rw = rw;
                    ntfr->offset = offset;
                    ntfr->cluster_size = cluster_size;
                    ntfr->file_record_size = file_record_size;
                    if (true == ntfr->parse_attrs(mask))
                        return ntfr;
                }
            }
        }
    }
    return ntfr;
}

boost::shared_ptr<FILE_RECORD_HEADER> volume::read_file_record(ULONGLONG fileRef){

    boost::shared_ptr<FILE_RECORD_HEADER> fr = NULL;
    if (fileRef < MFT_IDX_USER || mft_data == NULL){
        uint32_t len = 0;
        // Take as continuous disk allocation
        LONGLONG addr = mft_addr + (file_record_size) * fileRef;
        fr = boost::shared_ptr<FILE_RECORD_HEADER>( (PFILE_RECORD_HEADER) new BYTE[file_record_size] );
        if (fr){
            memset(fr.get(), 0, file_record_size);
            if (rw->read(addr + offset, file_record_size, fr.get(), len) && len == file_record_size)
                return fr;
        }
    }
    else{
        //// May be fragmented $MFT
        DWORD len = 0;
        ULONGLONG frAddr;
        frAddr = (file_record_size) * fileRef;
        fr = boost::shared_ptr<FILE_RECORD_HEADER>((PFILE_RECORD_HEADER) new BYTE[file_record_size]);
        if (fr){
            memset(fr.get(), 0, file_record_size);
            if (mft_data->read_data(frAddr, fr.get(), file_record_size, &len) &&
                len == file_record_size)
                return fr;
        }
    }
    return fr;
}

bool volume::patch_us(WORD *sector, int sectors, WORD usn, WORD *usarray){
    int i;
    for (i = 0; i<sectors; i++){
        sector += ((sector_size >> 1) - 1);
        if (*sector != usn)
            return false;	// USN error
        *sector = usarray[i];	// Write back correct data
        sector++;
    }
    return true;
}

bool file_record::is_sparse(){
    if (attrs.count(ATTR_INDEX(ATTR_TYPE_STANDARD_INFORMATION))){
        PATTR_STANDARD_INFORMATION stdInfo = (PATTR_STANDARD_INFORMATION)attrs[ATTR_INDEX(ATTR_TYPE_STANDARD_INFORMATION)].at(0)->attr_body.get();
        return stdInfo->Permission & ATTR_STDINFO_PERMISSION_SPARSE;
    }
    else return false;
}

bool file_record::is_encrypted(){
    if (attrs.count(ATTR_INDEX(ATTR_TYPE_STANDARD_INFORMATION))){
        PATTR_STANDARD_INFORMATION stdInfo = (PATTR_STANDARD_INFORMATION)attrs[ATTR_INDEX(ATTR_TYPE_STANDARD_INFORMATION)].at(0)->attr_body.get();
        return stdInfo->Permission & ATTR_STDINFO_PERMISSION_ENCRYPTED;
    }
    else return false;
}

bool file_record::is_compressed(){
    if (attrs.count(ATTR_INDEX(ATTR_TYPE_STANDARD_INFORMATION))){
        PATTR_STANDARD_INFORMATION stdInfo = (PATTR_STANDARD_INFORMATION)attrs[ATTR_INDEX(ATTR_TYPE_STANDARD_INFORMATION)].at(0)->attr_body.get();
        return stdInfo->Permission & ATTR_STDINFO_PERMISSION_COMPRESSED;
    }
    else return false;
}

bool file_record::is_deleted(){
    return !(fr->Flags  & FILE_RECORD_FLAG_INUSE);
}

bool file_record::is_directory(){
    return (fr->Flags  & FILE_RECORD_FLAG_DIR);
}

bool file_record::parse_attrs(DWORD mask){
    DWORD dataPtr = 0;	// guard if data exceeds FileRecordSize bounds
    PNTFS_ATTRIBUTE ahc = (PNTFS_ATTRIBUTE)((BYTE*)fr.get() + fr->OffsetOfAttr);
    dataPtr += fr->OffsetOfAttr;
    while (ahc->Type != (DWORD)-1 && (dataPtr + ahc->TotalSize) <= file_record_size){
        if (ATTR_MASK(ahc->Type) & mask)	// Skip unwanted attributes
        {
            if (!parse_attr(ahc))	// Parse error
                return false;
            if (is_encrypted() || is_compressed()){
                LOG(LOG_LEVEL_RECORD, L"Compressed and Encrypted file not supported yet!");
                return false;
            }
        }
        dataPtr += ahc->TotalSize;
        ahc = (PNTFS_ATTRIBUTE)((BYTE*)ahc + ahc->TotalSize);	// next attribute
    }
    return true;
}

bool file_record::parse_attr(PNTFS_ATTRIBUTE ahc){
    DWORD attrIndex = ATTR_INDEX(ahc->Type);
    if (attrIndex < ATTR_NUMS){
        attributes::ptr attr = attributes::ptr(new attributes());
        if (attr){
            attr->rw = rw;
            attr->offset = offset;
            attr->cluster_size = cluster_size;
            attr->ahc = ahc;
            if (ahc->NonResident){
                if (!parse_non_resident_attr(ahc, *attr.get())){
                    return false;
                }
            }
            else{
                PATTR_VOLUME_INFORMATION attr_body = (PATTR_VOLUME_INFORMATION)(void*)((BYTE*)ahc + (ahc->Attr.Resident.AttrOffset));
                attr->attr_body_size = ahc->Attr.Resident.AttrSize;
                attr->attr_body = boost::shared_ptr<BYTE>( new BYTE[attr->attr_body_size]);
                if (attr->attr_body){
                    memset(attr->attr_body.get(), 0, attr->attr_body_size);
                    memcpy(attr->attr_body.get(), attr_body, attr->attr_body_size);
                }
            }
            attrs[attrIndex].push_back(attr);
            return true;
        }
        else{
            LOG(LOG_LEVEL_ERROR, L"Attribute Parse error: 0x%04X", ahc->Type);
        }
    }
    else{
        LOG(LOG_LEVEL_ERROR, L"Invalid Attribute Type: 0x%04X", ahc->Type);
    }
    return false;
}

bool file_record::parse_non_resident_attr(PNTFS_ATTRIBUTE ahc, attributes& attr){
    const BYTE *dataRun = (BYTE*)ahc + ahc->Attr.NonResident.DataRunOffset;
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
            data_run::ptr dr = data_run::ptr(new data_run());
            if (dr){
                dr->lcn = (LCNOffset == 0) ? ULLONG_MAX : LCN;
                dr->clusters = length;
                dr->start_vcn = VCN;
                VCN += length;
                dr->last_vcn = VCN - 1;
                if (dr->last_vcn <= (ahc->Attr.NonResident.LastVCN - ahc->Attr.NonResident.StartVCN)){
                    attr.data_runs.push_back(dr);
                }
                else{
                    LOG(LOG_LEVEL_ERROR, L"DataRun decode error: VCN exceeds bound");
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

bool file_record::pick_data(const BYTE **dataRun, LONGLONG *length, LONGLONG *LCNOffset){
    BYTE size = **dataRun;
    int lengthBytes = size & 0x0F;
    int offsetBytes = size >> 4;

    (*dataRun)++;

    if (lengthBytes > 8 || offsetBytes > 8){
        LOG(LOG_LEVEL_ERROR, L"DataRun decode error 1: 0x%02X", size);
        return false;
    }
    *length = 0;
    memcpy(length, *dataRun, lengthBytes);
    if (*length < 0){
        LOG(LOG_LEVEL_ERROR, L"DataRun length error: %I64d", *length);
        return false;
    }

    (*dataRun) += lengthBytes;
    *LCNOffset = 0;
    if (offsetBytes)	// Not Sparse File
    {
        if ((*dataRun)[offsetBytes - 1] & 0x80)
            *LCNOffset = ULLONG_MAX;
        memcpy(LCNOffset, *dataRun, offsetBytes);
        (*dataRun) += offsetBytes;
    }
    return true;
}

bool attributes::read_data(const ULONGLONG offset, void *bufv, DWORD bufLen, DWORD *actural){
    if (ahc->NonResident){
        return read_non_resident_data(offset, bufv, bufLen, actural);
    }
    else{
        *actural = 0;
        if (bufLen != 0){
            DWORD offsetd = (DWORD)offset;
            if (offsetd >= attr_body_size)
                return false;	// offset parameter error

            if ((offsetd + bufLen) > attr_body_size)
                *actural = attr_body_size - offsetd;	// Beyond scope
            else
                *actural = bufLen;
            memcpy(bufv, (BYTE*)attr_body.get() + offsetd, *actural);
        }
    }
    return true;
}

bool attributes::read_non_resident_data(const ULONGLONG offset, void *bufv, DWORD bufLen, DWORD *actural){
    // Hard disks can only be accessed by sectors
    // To be simple and efficient, only implemented cluster based accessing
    // So cluster unaligned data address should be processed carefully here

    *actural = 0;
    if (bufLen == 0)
        return true;

    // Bounds check
    if (offset > ahc->Attr.NonResident.RealSize)
        return false;
    if ((offset + bufLen) > ahc->Attr.NonResident.RealSize)
        bufLen = (DWORD)(ahc->Attr.NonResident.RealSize - offset);

    DWORD len;
    BYTE *buf = (BYTE*)bufv;

    // First cluster Number
    ULONGLONG startVCN = offset / cluster_size;
    // Bytes in first cluster
    DWORD startBytes = cluster_size - (DWORD)(offset % cluster_size);
    // Read first cluster
    std::auto_ptr<BYTE> UnalignedBuf = std::auto_ptr<BYTE>(new BYTE[cluster_size]);
    if (NULL != UnalignedBuf.get()){
        if (startBytes != cluster_size){
            // First cluster, Unaligned
            if (read_virtual_clusters(startVCN, 1, UnalignedBuf.get(), cluster_size, &len)
                && len == cluster_size){
                len = (startBytes < bufLen) ? startBytes : bufLen;
                memcpy(buf, UnalignedBuf.get() + cluster_size - startBytes, len);
                buf += len;
                bufLen -= len;
                *actural += len;
                startVCN++;
            }
            else{
                return false;
            }
        }
        if (bufLen == 0){
            return true;
        }

        DWORD alignedClusters = bufLen / cluster_size;
        if (alignedClusters){
            // Aligned clusters
            DWORD alignedSize = alignedClusters*cluster_size;
            if (read_virtual_clusters(startVCN, alignedClusters, buf, alignedSize, &len)
                && len == alignedSize){
                startVCN += alignedClusters;
                buf += alignedSize;
                bufLen %= cluster_size;
                *actural += len;

                if (bufLen == 0){
                    return true;
                }
            }
            else{
                return false;
            }
        }

        // Last cluster, Unaligned
        if (read_virtual_clusters(startVCN, 1, UnalignedBuf.get(), cluster_size, &len)
            && len == cluster_size){
            memcpy(buf, UnalignedBuf.get(), bufLen);
            *actural += bufLen;
            return true;
        }
    }
    return false;
}

bool attributes::read_virtual_clusters(ULONGLONG vcn, DWORD clusters,
    void *bufv, DWORD bufLen, DWORD *actural){

    *actural = 0;
    BYTE *buf = (BYTE*)bufv;

    // Verify if clusters exceeds DataRun bounds
    if (vcn + clusters > (ahc->Attr.NonResident.LastVCN - ahc->Attr.NonResident.StartVCN + 1)){
        LOG(LOG_LEVEL_ERROR, L"Cluster exceeds DataRun bounds");
        return false;
    }

    // Verify buffer size
    if (bufLen < clusters*cluster_size){
        LOG(LOG_LEVEL_ERROR, L"Buffer size too small");
        return false;
    }

    // Traverse the DataRun List to find the according LCN
    
    foreach(data_run::ptr dr, data_runs){
        if (vcn >= dr->start_vcn && vcn <= dr->last_vcn){
            DWORD clustersToRead;
            ULONGLONG vcns = dr->last_vcn - vcn + 1;	// Clusters from read pointer to the end
            if ((ULONGLONG)clusters > vcns)	// Fragmented data, we must go on
                clustersToRead = (DWORD)vcns;
            else
                clustersToRead = clusters;
            if (read_clusters( buf, clustersToRead, dr->lcn + (vcn - dr->start_vcn))){
                buf += clustersToRead * cluster_size;
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
    *actural *= cluster_size;
    return true;
}

bool attributes::read_clusters(void *buf, DWORD clusters, LONGLONG lcn){
    if (lcn == ULLONG_MAX){	// sparse data
        LOG(LOG_LEVEL_RECORD, L"Sparse Data, Fill the buffer with 0");
        // Fill the buffer with 0
        memset(buf, 0, clusters * cluster_size);
        return true;
    }
    uint32_t len = 0;
    ULONGLONG addr = lcn * cluster_size;
    if (rw->read(addr + offset, clusters*cluster_size, buf, len) &&
        len == clusters*cluster_size){
        return true;
    }
    else{
        LOG(LOG_LEVEL_ERROR, L"Cannot read cluster with LCN %I64d", lcn);
    }
    return false;
}

WORD volume::get_volume_version(file_record &fs){
    WORD version = 0;
    if (fs.attrs.count(ATTR_INDEX(ATTR_TYPE_VOLUME_INFORMATION))){
        PATTR_VOLUME_INFORMATION attr_body = (PATTR_VOLUME_INFORMATION)fs.attrs[ATTR_INDEX(ATTR_TYPE_VOLUME_INFORMATION)].at(0)->attr_body.get();
        return MAKEWORD(attr_body->MinorVersion, attr_body->MajorVersion);
    }
    return version;
}

file_record::vtr   volume::find_sub_entry(const std::wregex filter, const file_record::ptr dir){
    file_record::vtr result;
    file_record::ptr _dir = dir;
    if (NULL == _dir)
        _dir = root();
    if (_dir->attrs.count(ATTR_INDEX(ATTR_TYPE_INDEX_ROOT))){
        foreach(attributes::ptr attr, _dir->attrs[ATTR_INDEX(ATTR_TYPE_INDEX_ROOT)]){
            PATTR_INDEX_ROOT IndexRoot = (PATTR_INDEX_ROOT)attr->attr_body.get();
            if (IndexRoot->AttrType == ATTR_TYPE_FILE_NAME){
                PINDEX_ENTRY ie;
                ie = (PINDEX_ENTRY)((BYTE*)(&(IndexRoot->EntryOffset)) + IndexRoot->EntryOffset);
                DWORD ieTotal = ie->Size;
                while (ieTotal <= IndexRoot->TotalEntrySize){
                    if (ie->Flags & INDEX_ENTRY_FLAG_SUBNODE){
                        visit_index_block(_dir, *(ULONGLONG*)((BYTE*)ie + ie->Size - 8), filter, result);
                    }
                    if (ie->StreamSize){
                        PATTR_FILE_NAME filename = (PATTR_FILE_NAME)ie->Stream;
#if __DEBUG_NTFS     
                        wchar_t fns[MAX_PATH];
                        memset(fns, 0, sizeof(fns));
                        wcsncpy_s(fns, MAX_PATH, (wchar_t*)filename->Name, filename->NameLength);
                        DoTraceMsg(TRACE_LEVEL_VERBOSE, L"%ws", fns);
#endif
                        std::wstring name((wchar_t*)filename->Name, filename->NameLength);
                        if (std::regex_match(name, filter))
                            result.push_back(get_file_record(ie->FileReference & 0x0000FFFFFFFFFFFFUL, name));
                    }
                    if (ie->Flags & INDEX_ENTRY_FLAG_LAST){
                        break;
                    }
                    ie = (PINDEX_ENTRY)(((BYTE*)ie) + ie->Size);	// Pick next
                    ieTotal += ie->Size;
                }
            }
        }
    }
    typedef std::map<ULONGLONG, file_record::ptr> file_record_map;
    file_record_map result_map;
    foreach(file_record::ptr r, result){
        if (!result_map.count(r->fileRef) || (result_map[r->fileRef]->name.length() < r->name.length()))
            result_map[r->fileRef] = r;
    }
    result.clear();
    foreach(file_record_map::value_type &r, result_map)
        result.push_back(r.second);
    return result;
}

bool volume::visit_index_block(const file_record::ptr dir, const ULONGLONG vcn, const std::wregex filter, file_record::vtr& result){
    if (dir->attrs.count(ATTR_INDEX(ATTR_TYPE_INDEX_ALLOCATION))){
        attributes::ptr attr = dir->attrs[ATTR_INDEX(ATTR_TYPE_INDEX_ALLOCATION)].at(0);
        std::auto_ptr<INDEX_BLOCK> ibBuf = std::auto_ptr<INDEX_BLOCK>((PINDEX_BLOCK) new BYTE[index_block_size]);
        if (NULL != ibBuf.get()){
            DWORD len;
            DWORD sectors = index_block_size / sector_size;
            if (attr->read_data(vcn *index_block_size, ibBuf.get(), index_block_size, &len) &&
                len == index_block_size){
                if (ibBuf->Magic != INDEX_BLOCK_MAGIC){
                    return false;
                }
                // Patch US
                WORD *usnaddr = (WORD*)((BYTE*)ibBuf.get() + ibBuf->OffsetOfUS);
                WORD usn = *usnaddr;
                WORD *usarray = usnaddr + 1;
                if (!patch_us((WORD*)ibBuf.get(), sectors, usn, usarray)){
                    LOG(LOG_LEVEL_ERROR, L"Index Block parse error: Update Sequence Number");
                    return false;
                }
                INDEX_ENTRY *ie;
                ie = (INDEX_ENTRY*)((BYTE*)(&(ibBuf->EntryOffset)) + ibBuf->EntryOffset);
                DWORD ieTotal = ie->Size;
                while (ieTotal <= ibBuf->TotalEntrySize){
                    if (ie->Flags & INDEX_ENTRY_FLAG_SUBNODE){
                        visit_index_block(dir, *(ULONGLONG*)((BYTE*)ie + ie->Size - 8), filter, result);
                    }
                    if (ie->StreamSize){
                        PATTR_FILE_NAME filename = (PATTR_FILE_NAME)ie->Stream;
#if __DEBUG_NTFS                   
                        wchar_t fns[MAX_PATH];
                        memset(fns, 0, sizeof(fns));
                        wcsncpy_s(fns, MAX_PATH, (wchar_t*)filename->Name, filename->NameLength);
                        DoTraceMsg(TRACE_LEVEL_VERBOSE, L"%ws", fns);
#endif
                        std::wstring name((wchar_t*)filename->Name, filename->NameLength);
                        if (std::regex_match(name, filter)){
                            result.push_back(get_file_record(ie->FileReference & 0x0000FFFFFFFFFFFFUL, name));
                        }
                    }
                    if (ie->Flags & INDEX_ENTRY_FLAG_LAST){
                        break;
                    }
                    ie = (INDEX_ENTRY*)((BYTE*)ie + ie->Size);	// Pick next
                    ieTotal += ie->Size;
                }
            }
        }
    }
    return false;
}

file_record::ptr volume::find_sub_entry(const std::wstring file_name, const file_record::ptr dir){
    file_record::ptr _dir = dir;
    if (NULL == _dir)
        _dir = root();
    if (_dir->attrs.count(ATTR_INDEX(ATTR_TYPE_INDEX_ROOT))){
        foreach(attributes::ptr attr, _dir->attrs[ATTR_INDEX(ATTR_TYPE_INDEX_ROOT)]){
            PATTR_INDEX_ROOT IndexRoot = (PATTR_INDEX_ROOT)attr->attr_body.get();
            if (IndexRoot->AttrType == ATTR_TYPE_FILE_NAME){
                PINDEX_ENTRY ie;
                ie = (PINDEX_ENTRY)((BYTE*)(&(IndexRoot->EntryOffset)) + IndexRoot->EntryOffset);
                DWORD ieTotal = ie->Size;
                while (ieTotal <= IndexRoot->TotalEntrySize){
                    if (ie->Flags & INDEX_ENTRY_FLAG_SUBNODE){
                        ULONGLONG fileRef;
                        if (visit_index_block(_dir, *(ULONGLONG*)((BYTE*)ie + ie->Size - 8), file_name, fileRef)){
                            return get_file_record(fileRef, file_name);
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
                        if ((file_name.length() == filename->NameLength) &&
                            (0 == _wcsnicmp((wchar_t*)filename->Name, file_name.c_str(), filename->NameLength))){
                            return get_file_record(ie->FileReference & 0x0000FFFFFFFFFFFFUL, file_name);
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
    }
    return NULL;
}

bool volume::visit_index_block(const file_record::ptr dir, const ULONGLONG vcn, const std::wstring& file_name, ULONGLONG& fileRef){
    if (dir->attrs.count(ATTR_INDEX(ATTR_TYPE_INDEX_ALLOCATION))){
        attributes::ptr attr = dir->attrs[ATTR_INDEX(ATTR_TYPE_INDEX_ALLOCATION)].at(0);
        std::auto_ptr<INDEX_BLOCK> ibBuf = std::auto_ptr<INDEX_BLOCK>((PINDEX_BLOCK) new BYTE[index_block_size]);
        if (NULL != ibBuf.get()){
            DWORD len;
            DWORD sectors = index_block_size / sector_size;
            if (attr->read_data(vcn *index_block_size, ibBuf.get(), index_block_size, &len) &&
                len == index_block_size){
                if (ibBuf->Magic != INDEX_BLOCK_MAGIC){
                    return false;
                }
                // Patch US
                WORD *usnaddr = (WORD*)((BYTE*)ibBuf.get() + ibBuf->OffsetOfUS);
                WORD usn = *usnaddr;
                WORD *usarray = usnaddr + 1;
                if (!patch_us((WORD*)ibBuf.get(), sectors, usn, usarray)){
                    LOG(LOG_LEVEL_ERROR, L"Index Block parse error: Update Sequence Number");
                    return false;
                }
                INDEX_ENTRY *ie;
                ie = (INDEX_ENTRY*)((BYTE*)(&(ibBuf->EntryOffset)) + ibBuf->EntryOffset);
                DWORD ieTotal = ie->Size;
                while (ieTotal <= ibBuf->TotalEntrySize){
                    if (ie->Flags & INDEX_ENTRY_FLAG_SUBNODE){
                        if (visit_index_block(dir, *(ULONGLONG*)((BYTE*)ie + ie->Size - 8), file_name, fileRef)){
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
                        if ((file_name.length() == filename->NameLength) &&
                            (0 == _wcsnicmp((wchar_t*)filename->Name, file_name.c_str(), filename->NameLength))){
                            fileRef = ie->FileReference & 0x0000FFFFFFFFFFFFUL;
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
        }
    }
    return false;
}

file_extent::vtr file_record::extents(){
    file_extent::vtr extents;
    if (attrs.count(ATTR_INDEX(ATTR_TYPE_DATA))){
        attributes::ptr attr = attrs[ATTR_INDEX(ATTR_TYPE_DATA)].at(0);
        if (attr->ahc->NonResident){
            foreach(data_run::ptr dr, attr->data_runs){
                extents.push_back(file_extent(dr->start_vcn * cluster_size, dr->lcn == ULLONG_MAX ? dr->lcn : (dr->lcn * cluster_size) + offset, dr->clusters * cluster_size));
            }
            std::sort(extents.begin(), extents.end(), file_extent::compare());
        }
    }
    return extents;
}

bool file_record::read(const ULONGLONG offset, void *bufv, DWORD bufLen, DWORD *actural){
    if (attrs.count(ATTR_INDEX(ATTR_TYPE_DATA))){
        attributes::ptr attr = attrs[ATTR_INDEX(ATTR_TYPE_DATA)].at(0);
        return attr->read_data(offset, bufv, bufLen, actural);
    }
    return false;
}

io_range::vtr  volume::file_system_ranges(){
    io_range::vtr ranges;
    file_record::ptr bitmap = get_file_record(MFT_IDX_BITMAP, MASK_DATA);
    if (bitmap){
        ULONGLONG bits = total_size / cluster_size;
        ULONGLONG bufsize = (bits >> 3);
        boost::shared_ptr<BYTE> map = boost::shared_ptr<BYTE>(new BYTE[bufsize+1]);
        if (map){
            DWORD len = 0;
            memset(map.get(), 0, bufsize+1);
            if (bitmap->read(0, map.get(), bufsize, &len) && bufsize == len){
                uint64_t newBit, oldBit = 0;
                bool     dirty_range = false;
                for (newBit = 0; newBit < bits; newBit++){
                    if (map.get()[newBit >> 3] & (1 << (newBit & 7))){
                        if (!dirty_range){
                            dirty_range = true;
                            oldBit = newBit;
                        }
                    }
                    else {
                        if (dirty_range){
                            ULONGLONG start = (oldBit * cluster_size) + offset;
                            ULONGLONG length = (newBit - oldBit) * cluster_size;
                            ranges.push_back(io_range(start, length));
                            dirty_range = false;
                        }
                    }
                }
                if (dirty_range){
                    ULONGLONG start = (oldBit * cluster_size) + offset;
                    ULONGLONG length = (newBit - oldBit) * cluster_size;
                    ranges.push_back(io_range(start, length));
                }
            }
        }
    }
    return ranges;
}

ULONGLONG file_record::file_size(){
    ULONGLONG size = 0;
    if (attrs.count(ATTR_INDEX(ATTR_TYPE_FILE_NAME))){
        PATTR_FILE_NAME fileName = (PATTR_FILE_NAME)attrs[ATTR_INDEX(ATTR_TYPE_FILE_NAME)].at(0)->attr_body.get();
        size = fileName ? fileName->RealSize : 0;
    }
    if (size == 0)
        return file_allocated_size();
    return size;
}

ULONGLONG file_record::file_allocated_size(){
    ULONGLONG size = 0;
    if (attrs.count(ATTR_INDEX(ATTR_TYPE_FILE_NAME))){
        PATTR_FILE_NAME fileName = (PATTR_FILE_NAME)attrs[ATTR_INDEX(ATTR_TYPE_FILE_NAME)].at(0)->attr_body.get();
        size = fileName ? fileName->AllocSize : 0;
    }
    if (size == 0){
        file_extent::vtr exts = extents();
        if (exts.size()){
            return exts[exts.size() - 1].start + exts[exts.size() - 1].length;
        }
    }
    return size;
}