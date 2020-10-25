#pragma once

#ifndef __IRM_HOST_VSS_SNAPSHOT__
#define __IRM_HOST_VSS_SNAPSHOT__
#include <vector>
#include <string>
#include <memory>
#include <atlbase.h>
#include <atlcom.h>
#include <initguid.h>
#include <comdef.h>
#include <comutil.h>
#include <vss.h>
#include <vswriter.h>
#include <vsbackup.h>
#include <macho.h>
#include <locale>
#include <codecvt>
#include <VersionHelpers.h>

using namespace macho;

// TODO: reference additional headers your program requires here
#pragma comment (lib, "VssApi.lib")
#pragma comment (lib, "Rpcrt4.lib")
static const GUID g_vss_sw_prv = macho::guid_(L"{b5946137-7b9f-4925-af80-51abd60b20d5}");
//{ 0x3315302f, 0xf5e2, 0x4b90, { 0xb0, 0x70, 0xb4, 0x1, 0x60, 0xaa, 0xf1, 0xda } };

struct irm_vss_snapshot_result{
    typedef boost::shared_ptr<irm_vss_snapshot_result> ptr;
    typedef std::vector<ptr> vtr;
    irm_vss_snapshot_result() : snapshots_count(0) {}
    macho::guid_                snapshot_id;
    macho::guid_                snapshot_set_id;
    int                         snapshots_count;
    std::wstring                snapshot_device_object;
    std::wstring                original_volume_name;
    boost::posix_time::ptime    creation_time_stamp;
    void copy(const irm_vss_snapshot_result& _res) {
        snapshots_count = _res.snapshots_count;
        snapshot_id = _res.snapshot_id;
        snapshot_set_id = _res.snapshot_set_id;
        snapshot_device_object = _res.snapshot_device_object;
        original_volume_name = _res.original_volume_name;
        creation_time_stamp = _res.creation_time_stamp;
    }
    const irm_vss_snapshot_result &operator =(const irm_vss_snapshot_result& _res) {
        if (this != &_res)
            copy(_res);
        return(*this);
    }
};

class irm_vss_snapshot
{
public:
    irm_vss_snapshot() : _is_change_key(false){
    }

    void init(std::wstring output_folder = macho::windows::environment::get_working_directory() + L"\\output" )
    {
        FUN_TRACE;
        macho::windows::registry reg;
        if (reg.open(L"SYSTEM\\CurrentControlSet\\Services\\VSS\\VssAccessControl")){
            if (reg[L"NT Authority\\NetworkService"].exists() && reg[L"NT Authority\\NetworkService"].operator DWORD() != (DWORD) 0x0 ){
                _is_change_key = true;
                reg[L"NT Authority\\NetworkService"] = 0x0;
                macho::windows::service vss = macho::windows::service::get_service(L"vss");
                vss.stop();
                vss.start();
            }
   /*         else{
                macho::windows::service vss = macho::windows::service::get_service(L"vss");
                vss.start();
            }*/
        }
        _output_folder = output_folder;
    }

    virtual ~irm_vss_snapshot()
    {
        release();
    }

    HRESULT delete_snapshot(std::wstring snapshot_id, LONG&  deleted_snapshots, VSS_ID& non_deleted_snapshot_id){
        FUN_TRACE;
        deleted_snapshots = 0;
        non_deleted_snapshot_id = GUID_NULL;
        HRESULT result = S_OK;
        ATL::CComPtr<IVssBackupComponents>  _backup;     // IVssBackupComponents pointer
        if (S_OK != (result = CreateVssBackupComponents(&_backup)))
            LOG(LOG_LEVEL_ERROR, _T("CreateVssBackupComponents - Returned HRESULT = 0x%08lx\n"), result);
        else if (S_OK != (result = _backup->InitializeForBackup()))
            LOG(LOG_LEVEL_ERROR, _T("InitializeForBackup - Returned HRESULT = 0x%08lx\n"), result);
        else if (S_OK != (result = _backup->SetContext(VSS_CTX_ALL)))
            LOG(LOG_LEVEL_ERROR, _T("SetContext - Returned HRESULT = 0x%08lx\n"), result);
        else if (_backup){
            return  result = _backup->DeleteSnapshots(macho::guid_(snapshot_id), VSS_OBJECT_SNAPSHOT, TRUE, &deleted_snapshots, &non_deleted_snapshot_id);
        }
        return result;
    }

    HRESULT delete_snapshot_set(std::wstring snapshot_set_id, LONG&  deleted_snapshots, VSS_ID& non_deleted_snapshot_id){
        FUN_TRACE;
        deleted_snapshots = 0;
        non_deleted_snapshot_id = GUID_NULL;
        HRESULT result = S_OK;
        ATL::CComPtr<IVssBackupComponents>  _backup;     // IVssBackupComponents pointer
        if (S_OK != (result = CreateVssBackupComponents(&_backup)))
            LOG(LOG_LEVEL_ERROR, _T("CreateVssBackupComponents - Returned HRESULT = 0x%08lx\n"), result);
        else if (S_OK != (result = _backup->InitializeForBackup()))
            LOG(LOG_LEVEL_ERROR, _T("InitializeForBackup - Returned HRESULT = 0x%08lx\n"), result);
        else if (S_OK != (result = _backup->SetContext(VSS_CTX_ALL)))
            LOG(LOG_LEVEL_ERROR, _T("SetContext - Returned HRESULT = 0x%08lx\n"), result);
        else if (_backup){
            return  result = _backup->DeleteSnapshots(macho::guid_(snapshot_set_id), VSS_OBJECT_SNAPSHOT_SET, TRUE, &deleted_snapshots, &non_deleted_snapshot_id);
        }
        return result;
    }

    irm_vss_snapshot_result::vtr query_snapshots(){
        FUN_TRACE;
        irm_vss_snapshot_result::vtr _snapshot_results;
        HRESULT result = S_OK;
        ATL::CComPtr<IVssBackupComponents>  _backup;     // IVssBackupComponents pointer
        if (S_OK != (result = CreateVssBackupComponents(&_backup)))
            LOG(LOG_LEVEL_ERROR, _T("CreateVssBackupComponents - Returned HRESULT = 0x%08lx\n"), result);
        else if (S_OK != (result = _backup->InitializeForBackup()))
            LOG(LOG_LEVEL_ERROR, _T("InitializeForBackup - Returned HRESULT = 0x%08lx\n"), result);
        else if (S_OK != (result = _backup->SetContext(VSS_CTX_ALL)))
            LOG(LOG_LEVEL_ERROR, _T("SetContext - Returned HRESULT = 0x%08lx\n"), result);
        else if (_backup){
            ATL::CComPtr<IVssEnumObject>             _enum;
            if (S_OK == (result = _backup->Query(GUID_NULL, VSS_OBJECT_NONE, VSS_OBJECT_SNAPSHOT, &_enum))){
                VSS_OBJECT_PROP prop;
                VSS_SNAPSHOT_PROP& snapshot = prop.Obj.Snap;
                while (true){
                    // Get the next element
                    ULONG ulFetched;
                    result = _enum->Next(1, &prop, &ulFetched);                 
                    // We reached the end of list
                    if (ulFetched == 0)
                        break;
                    irm_vss_snapshot_result::ptr result = irm_vss_snapshot_result::ptr(new irm_vss_snapshot_result());
                    result->snapshot_id = snapshot.m_SnapshotId;
                    result->snapshot_set_id = snapshot.m_SnapshotSetId;
                    result->snapshots_count = snapshot.m_lSnapshotsCount;
                    result->snapshot_device_object = snapshot.m_pwszSnapshotDeviceObject;
                    result->original_volume_name = snapshot.m_pwszOriginalVolumeName;
                    FILETIME ft;
                    // Now put the time back inside filetime.
                    ft.dwHighDateTime = snapshot.m_tsCreationTimestamp >> 32;
                    ft.dwLowDateTime = snapshot.m_tsCreationTimestamp & 0x00000000FFFFFFFF;
                    result->creation_time_stamp = boost::posix_time::from_ftime<boost::posix_time::ptime>(ft);
                    _snapshot_results.push_back(result);
                    VssFreeSnapshotProperties(&snapshot);
                }
            }
        }
        return _snapshot_results;
    }

    irm_vss_snapshot_result::vtr take_snapshot(std::vector<std::wstring> &volumes){
        FUN_TRACE;
        irm_vss_snapshot_result::vtr        _snapshot_results;
        ATL::CComPtr<IVssAsync>             _prepare;
        ATL::CComPtr<IVssAsync>             _async;
        ATL::CComPtr<IVssAsync>             _do_shadow_copy;
        ATL::CComPtr<IVssBackupComponents>  _backup;     // IVssBackupComponents pointer
        std::map<std::wstring, GUID>        _snapshots_map;

        HRESULT result = S_OK;

#ifdef _WIN2K3
        LONG context = (VSS_CTX_BACKUP | VSS_CTX_APP_ROLLBACK);
#else
        LONG context = IsWindows7OrGreater() ? 
            (VSS_CTX_BACKUP | VSS_CTX_APP_ROLLBACK | VSS_VOLSNAP_ATTR_NO_AUTORECOVERY | VSS_VOLSNAP_ATTR_TXF_RECOVERY) :
            (VSS_CTX_BACKUP | VSS_CTX_APP_ROLLBACK | VSS_VOLSNAP_ATTR_NO_AUTORECOVERY);
#endif
        if (S_OK != (result = CreateVssBackupComponents(&_backup)))
            LOG(LOG_LEVEL_ERROR, _T("CreateVssBackupComponents - Returned HRESULT = 0x%08lx\n"), result);
        else if (S_OK != (result = _backup->InitializeForBackup()))
            LOG(LOG_LEVEL_ERROR, _T("InitializeForBackup - Returned HRESULT = 0x%08lx\n"), result);
        else if (S_OK != (result = _backup->SetContext(context)))
            LOG(LOG_LEVEL_ERROR, _T("SetContext - Returned HRESULT = 0x%08lx\n"), result);
        else if (_backup){
            // Prompts each writer to send the metadata they have collected
            result = _backup->GatherWriterMetadata(&_async);
            if (result != S_OK){
                LOG(LOG_LEVEL_ERROR, _T("_backup->GatherWriterMetadata - Returned HRESULT = 0x%08lx\n"), result);
            }
            else{
                LOG(LOG_LEVEL_INFO, _T("Gathering metadata from writers...\n"));
                result = _async->Wait();
                if (result != S_OK){
                    LOG(LOG_LEVEL_ERROR, _T("_async->Wait() - Returned HRESULT = 0x%08lx\n"), result);
                }
                else{
                    //
                    // Creates a new, empty shadow copy set
                    //
                    LOG(LOG_LEVEL_INFO, _T("calling StartSnapshotSet...\n"));
                    VSS_ID snapshotSetId;
                    result = _backup->StartSnapshotSet(&snapshotSetId);
                    if (result != S_OK){
                        LOG(LOG_LEVEL_ERROR, _T("StartSnapshotSet - Returned HRESULT = 0x%08lx\n"), result);
                    }
                    else{
                        LOG(LOG_LEVEL_INFO, _T("AddToSnapshotSet SnapshtSetId (%s)...\n"), GuidToWString(snapshotSetId).c_str());
                        for (std::vector<std::wstring>::iterator iVolume = volumes.begin(); iVolume != volumes.end(); iVolume++){
                            if (_snapshots_map.count((*iVolume)))
                                continue;
                            VSS_ID snapshotId;
                            LOG(LOG_LEVEL_INFO, _T("AddToSnapshotSet (%s) "), (*iVolume).c_str());
                            result = _backup->AddToSnapshotSet((VSS_PWSZ)(*iVolume).c_str(), g_vss_sw_prv, &snapshotId);
                            if (result != S_OK){
                                LOG(LOG_LEVEL_ERROR, _T("AddToSnapshotSet (%s) - Returned HRESULT = 0x%08lx\n"), (*iVolume).c_str(), result);
                                if (result == VSS_E_OBJECT_ALREADY_EXISTS)
                                    continue;
                                else
                                    break;
                            }
                            else{
                                LOG(LOG_LEVEL_INFO, _T("AddToSnapshotSet SnapshotId(%s)...\n"), GuidToWString(snapshotId).c_str());
                                _snapshots_map[(*iVolume)] = snapshotId;
                            }
                        }

                        if (result == S_OK){
                            //
                            // Configure the backup operation for Copy with no backup history
                            //

                            result = _backup->SetBackupState(false, true, VSS_BT_FULL);
                            if (result != S_OK){
                                LOG(LOG_LEVEL_ERROR, _T("SetBackupState - Returned HRESULT = 0x%08lx\n"), result);
                            }
                            else{
                                //
                                // Make VSS generate a PrepareForBackup event
                                //
                                result = _backup->PrepareForBackup(&_prepare);
                                if (result != S_OK){
                                    LOG(LOG_LEVEL_ERROR, _T("PrepareForBackup - Returned HRESULT = 0x%08lx\n"), result);
                                }
                                else{
                                    LOG(LOG_LEVEL_INFO, _T("Preparing for backup...\n"));
                                    result = _prepare->Wait();
                                    if (result != S_OK){
                                        LOG(LOG_LEVEL_ERROR, _T("_prepare->Wait - Returned HRESULT = 0x%08lx\n"), result);
                                    }
                                    else{
                                        result = _backup->DoSnapshotSet(&_do_shadow_copy);
                                        if (result != S_OK){
                                            LOG(LOG_LEVEL_ERROR, _T("DoSnapshotSet - Returned HRESULT = 0x%08lx\n"), result);
                                        }
                                        else{
                                            LOG(LOG_LEVEL_INFO, _T("Taking snapshots...\n"));
                                            result = _do_shadow_copy->Wait();
                                            if (result != S_OK){
                                                LOG(LOG_LEVEL_ERROR, _T("_do_shadow_copy->Wait - Returned HRESULT = 0x%08lx\n"), result);
                                            }
                                            else{
                                                for (std::map<std::wstring, GUID>::iterator iSnapshotId = _snapshots_map.begin(); iSnapshotId != _snapshots_map.end(); iSnapshotId++){
                                                    VSS_SNAPSHOT_PROP snapshot;
                                                    result = _backup->GetSnapshotProperties(iSnapshotId->second, &snapshot);
                                                    if (result == S_OK){
                                                        irm_vss_snapshot_result::ptr result = irm_vss_snapshot_result::ptr(new irm_vss_snapshot_result());
                                                        result->snapshot_id = snapshot.m_SnapshotId;
                                                        result->snapshot_set_id = snapshot.m_SnapshotSetId;
                                                        result->snapshots_count = snapshot.m_lSnapshotsCount;
                                                        result->snapshot_device_object = snapshot.m_pwszSnapshotDeviceObject;
                                                        result->original_volume_name = snapshot.m_pwszOriginalVolumeName;
                                                        FILETIME ft;
                                                        // Now put the time back inside filetime.
                                                        ft.dwHighDateTime = snapshot.m_tsCreationTimestamp >> 32;
                                                        ft.dwLowDateTime = snapshot.m_tsCreationTimestamp & 0x00000000FFFFFFFF;
                                                        result->creation_time_stamp = boost::posix_time::from_ftime<boost::posix_time::ptime>(ft);
                                                        _snapshot_results.push_back(result);
                                                        VssFreeSnapshotProperties(&snapshot);
                                                    }
                                                    else{
                                                        LOG(LOG_LEVEL_ERROR, _T("_backup->GetSnapshotProperties - Returned HRESULT = 0x%08lx\n"), result);
                                                    }
                                                }
                                                /*****
                                                UINT nWriters = 0;
                                                result = _backup->GetWriterMetadataCount(&nWriters);

                                                if (result == S_OK)
                                                {
                                                CComBSTR    bstrReqXml;
                                                result = _backup->SaveAsXML(&bstrReqXml);
                                                if (result == S_OK)
                                                {
                                                std::wstring outputfile = boost::str(boost::wformat(L"%s\\request.xml") % _output_folder);
                                                write_file(outputfile, (BSTR)bstrReqXml ? (BSTR)bstrReqXml : L"");
                                                }

                                                for (UINT iWriter = 0; iWriter < nWriters; iWriter++)
                                                {
                                                VSS_ID                              idInstance = GUID_NULL;
                                                CComPtr<IVssExamineWriterMetadata>  pMetadata;
                                                result = _backup->GetWriterMetadata(iWriter, &idInstance, &pMetadata);
                                                if (result == S_OK)
                                                {
                                                VSS_ID          idInstance = GUID_NULL;
                                                VSS_ID          idWriter = GUID_NULL;
                                                VSS_USAGE_TYPE  usage = VSS_UT_UNDEFINED;
                                                VSS_SOURCE_TYPE source = VSS_ST_UNDEFINED;
                                                CComBSTR        bstrWriterName;
                                                result = pMetadata->GetIdentity(
                                                &idInstance,
                                                &idWriter,
                                                &bstrWriterName,
                                                &usage,
                                                &source);
                                                if (result == S_OK)
                                                {
                                                CComBSTR    bstrMatadataXml;
                                                pMetadata->SaveAsXML(&bstrMatadataXml);

                                                std::wstring outputfile;
                                                if ((BSTR)bstrWriterName)
                                                outputfile = boost::str(boost::wformat(L"%s\\%s.xml") % _output_folder % (BSTR)bstrWriterName);
                                                else
                                                outputfile = boost::str(boost::wformat(L"%s\\%d.xml") % _output_folder %iWriter);
                                                write_file(outputfile, (BSTR)bstrMatadataXml ? (BSTR)bstrMatadataXml : L"");
                                                }
                                                }
                                                }
                                                }
                                                *****/
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                _backup->FreeWriterMetadata();
            }
        }
        return _snapshot_results;
    }

    void release(){
        FUN_TRACE;
        macho::windows::registry reg;
        if (_is_change_key){
            if (reg.open(L"SYSTEM\\CurrentControlSet\\Services\\VSS\\VssAccessControl")){
                reg[L"NT Authority\\NetworkService"] = 0x1;
                _is_change_key = false;
            }
        }
    }
private:

    void write_file(std::wstring output_file, std::wstring output_data )
    {
        std::wofstream output(output_file, std::ios::out | std::ios::trunc);
        std::locale loc(std::locale::classic(), new std::codecvt_utf8<wchar_t>);
        output.imbue(loc);
        output.write(output_data.c_str(), output_data.length());
        output.close();
    }

    std::wstring
        GuidToWString(
        GUID& guid
        )
    {
        RPC_STATUS rs;
        wchar_t* s;
        std::wstring r;

        rs = ::UuidToStringW(&guid, (RPC_WSTR *)&s);
        if (rs == RPC_S_OK) {
            r = s;
            ::RpcStringFreeW((RPC_WSTR *)&s);
        }
        return r;
    }

    bool                                _is_change_key;
    std::wstring                        _output_folder;
};

#endif
