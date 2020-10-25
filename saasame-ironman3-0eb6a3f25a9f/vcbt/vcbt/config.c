
#include "type.h"
#include "config.h"
#include "trace.h"
#include "ntstrsafe.h"
#include "config.tmh"
#include "..\..\buildenv.h"

bool is_disable_vcbt(){
    bool                     is_disable = false;
    NTSTATUS                 Status;
    UNICODE_STRING           KeyValue;
    RTL_QUERY_REGISTRY_TABLE RegTable[2];
    KeyValue.Buffer = __malloc(sizeof(UNICODE_NULL) + 256);
    if (NULL != KeyValue.Buffer){
        KeyValue.Length = sizeof(UNICODE_NULL) + 256;
        KeyValue.MaximumLength = KeyValue.Length;
        memset(KeyValue.Buffer, 0, KeyValue.Length);
        memset(RegTable, 0, sizeof(RegTable));
        RegTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
        RegTable[0].Name = L"SystemStartOptions";
        RegTable[0].EntryContext = &KeyValue;
        RegTable[0].DefaultType = REG_SZ;
        RegTable[0].DefaultData = &KeyValue;
        RegTable[0].DefaultLength = 0;
        if (NT_SUCCESS(Status = RtlQueryRegistryValues(RTL_REGISTRY_CONTROL, NULL, &RegTable[0], NULL, NULL))){
            _wcsupr((wchar_t *)KeyValue.Buffer);
            if (wcsstr((wchar_t *)KeyValue.Buffer, L"VCBT:OFF"))
                is_disable = true;
        }
        __free(KeyValue.Buffer);
    }
    return is_disable;
}

bool is_only_enable_umap(IN PUNICODE_STRING RegistryPath){
    bool is_disable_journal_file = false;
    // Make zero terminated copy of driver registry path
    USHORT FromLen = RegistryPath->Length;
    PUCHAR wstrDriverRegistryPath = (PUCHAR)ExAllocatePool(PagedPool, FromLen + sizeof(WCHAR));
    if (wstrDriverRegistryPath != NULL){
        RtlCopyMemory(wstrDriverRegistryPath, RegistryPath->Buffer, FromLen);
        RtlZeroMemory(wstrDriverRegistryPath + FromLen, sizeof(WCHAR));
        
        // Initialize ULONG value
        ULONG longValue = 0;

        // Build up our registry query table
        RTL_QUERY_REGISTRY_TABLE QueryTable[3];
        RtlZeroMemory(QueryTable, sizeof(QueryTable));
        QueryTable[0].Name = L"Parameters";
        QueryTable[0].Flags = RTL_QUERY_REGISTRY_SUBKEY;
        QueryTable[0].EntryContext = NULL;
        QueryTable[1].Name = L"DisableJournalFile";
        QueryTable[1].Flags = RTL_QUERY_REGISTRY_DIRECT;
        QueryTable[1].EntryContext = &longValue;

        // Issue query
        NTSTATUS status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE, (PWSTR)wstrDriverRegistryPath, QueryTable, NULL, NULL);
        if (NT_SUCCESS(status) && longValue > 0)
            is_disable_journal_file = true;

        // Do not forget to free buffers
        ExFreePool(wstrDriverRegistryPath);
    }
    return is_disable_journal_file;
}

bool is_batch_update_journal_meta_data(IN PUNICODE_STRING RegistryPath){
    bool _is_batch_update_journal_meta_data = false;
    // Make zero terminated copy of driver registry path
    USHORT FromLen = RegistryPath->Length;
    PUCHAR wstrDriverRegistryPath = (PUCHAR)ExAllocatePool(PagedPool, FromLen + sizeof(WCHAR));
    if (wstrDriverRegistryPath != NULL){
        RtlCopyMemory(wstrDriverRegistryPath, RegistryPath->Buffer, FromLen);
        RtlZeroMemory(wstrDriverRegistryPath + FromLen, sizeof(WCHAR));

        // Initialize ULONG value
        ULONG longValue = 0;

        // Build up our registry query table
        RTL_QUERY_REGISTRY_TABLE QueryTable[3];
        RtlZeroMemory(QueryTable, sizeof(QueryTable));
        QueryTable[0].Name = L"Parameters";
        QueryTable[0].Flags = RTL_QUERY_REGISTRY_SUBKEY;
        QueryTable[0].EntryContext = NULL;
        QueryTable[1].Name = L"BatchUpdateJournalMetaData";
        QueryTable[1].Flags = RTL_QUERY_REGISTRY_DIRECT;
        QueryTable[1].EntryContext = &longValue;

        // Issue query
        NTSTATUS status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE, (PWSTR)wstrDriverRegistryPath, QueryTable, NULL, NULL);
        if (NT_SUCCESS(status) && longValue > 0)
            _is_batch_update_journal_meta_data = true;

        // Do not forget to free buffers
        ExFreePool(wstrDriverRegistryPath);
    }
    return _is_batch_update_journal_meta_data;
}

bool is_umap_flush_disabled(IN PUNICODE_STRING RegistryPath){
    bool _is_umap_flush_disabled = false;
    // Make zero terminated copy of driver registry path
    USHORT FromLen = RegistryPath->Length;
    PUCHAR wstrDriverRegistryPath = (PUCHAR)ExAllocatePool(PagedPool, FromLen + sizeof(WCHAR));
    if (wstrDriverRegistryPath != NULL){
        RtlCopyMemory(wstrDriverRegistryPath, RegistryPath->Buffer, FromLen);
        RtlZeroMemory(wstrDriverRegistryPath + FromLen, sizeof(WCHAR));

        // Initialize ULONG value
        ULONG longValue = 0;

        // Build up our registry query table
        RTL_QUERY_REGISTRY_TABLE QueryTable[3];
        RtlZeroMemory(QueryTable, sizeof(QueryTable));
        QueryTable[0].Name = L"Parameters";
        QueryTable[0].Flags = RTL_QUERY_REGISTRY_SUBKEY;
        QueryTable[0].EntryContext = NULL;
        QueryTable[1].Name = L"DisableUMAPFlush";
        QueryTable[1].Flags = RTL_QUERY_REGISTRY_DIRECT;
        QueryTable[1].EntryContext = &longValue;

        // Issue query
        NTSTATUS status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE, (PWSTR)wstrDriverRegistryPath, QueryTable, NULL, NULL);
        if (NT_SUCCESS(status) && longValue > 0)
            _is_umap_flush_disabled = true;

        // Do not forget to free buffers
        ExFreePool(wstrDriverRegistryPath);
    }
    return _is_umap_flush_disabled;
}

bool is_verbose_debug(IN PUNICODE_STRING RegistryPath){
    bool _is_verbose_debug = false;
    // Make zero terminated copy of driver registry path
    USHORT FromLen = RegistryPath->Length;
    PUCHAR wstrDriverRegistryPath = (PUCHAR)ExAllocatePool(PagedPool, FromLen + sizeof(WCHAR));
    if (wstrDriverRegistryPath != NULL){
        RtlCopyMemory(wstrDriverRegistryPath, RegistryPath->Buffer, FromLen);
        RtlZeroMemory(wstrDriverRegistryPath + FromLen, sizeof(WCHAR));

        // Initialize ULONG value
        ULONG longValue = 0;

        // Build up our registry query table
        RTL_QUERY_REGISTRY_TABLE QueryTable[3];
        RtlZeroMemory(QueryTable, sizeof(QueryTable));
        QueryTable[0].Name = L"Parameters";
        QueryTable[0].Flags = RTL_QUERY_REGISTRY_SUBKEY;
        QueryTable[0].EntryContext = NULL;
        QueryTable[1].Name = L"VerboseDebug";
        QueryTable[1].Flags = RTL_QUERY_REGISTRY_DIRECT;
        QueryTable[1].EntryContext = &longValue;

        // Issue query
        NTSTATUS status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE, (PWSTR)wstrDriverRegistryPath, QueryTable, NULL, NULL);
        if (NT_SUCCESS(status) && longValue > 0)
            _is_verbose_debug = true;

        // Do not forget to free buffers
        ExFreePool(wstrDriverRegistryPath);
    }
    return _is_verbose_debug;
}

void update_driver_version_info(IN PUNICODE_STRING RegistryPath){
    USHORT FromLen = RegistryPath->Length;
    PUCHAR wstrDriverRegistryPath = (PUCHAR)ExAllocatePool(PagedPool, FromLen + sizeof(WCHAR));
    if (wstrDriverRegistryPath != NULL){
        RtlCopyMemory(wstrDriverRegistryPath, RegistryPath->Buffer, FromLen);
        RtlZeroMemory(wstrDriverRegistryPath + FromLen, sizeof(WCHAR));
#define BUFF_SIZE 40
        WCHAR value[BUFF_SIZE];
        memset(value, 0, sizeof(value));
        RtlStringCchPrintfW(value, BUFF_SIZE, L"%s", STAMPINF_VERSION_WSTR);
        NTSTATUS status;
        status = RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE, (PWSTR)wstrDriverRegistryPath, L"Version", REG_SZ, &value, sizeof(value));
        if (!NT_SUCCESS(status)){
            DoTraceMsg(TRACE_LEVEL_ERROR, "update_driver_version_info: Failed to update driver version info. (running version: %ws).", value);
        }
        else{
            DoTraceMsg(TRACE_LEVEL_VERBOSE, "Running version: %ws", value);
        }
        // Do not forget to free buffers
        ExFreePool(wstrDriverRegistryPath);
    }
}

bool is_disk_copy_data_disabled(IN PUNICODE_STRING RegistryPath){
    bool _is_disk_copy_data_disabled = false;
    // Make zero terminated copy of driver registry path
    USHORT FromLen = RegistryPath->Length;
    PUCHAR wstrDriverRegistryPath = (PUCHAR)ExAllocatePool(PagedPool, FromLen + sizeof(WCHAR));
    if (wstrDriverRegistryPath != NULL){
        RtlCopyMemory(wstrDriverRegistryPath, RegistryPath->Buffer, FromLen);
        RtlZeroMemory(wstrDriverRegistryPath + FromLen, sizeof(WCHAR));

        // Initialize ULONG value
        ULONG longValue = 0;

        // Build up our registry query table
        RTL_QUERY_REGISTRY_TABLE QueryTable[3];
        RtlZeroMemory(QueryTable, sizeof(QueryTable));
        QueryTable[0].Name = L"Parameters";
        QueryTable[0].Flags = RTL_QUERY_REGISTRY_SUBKEY;
        QueryTable[0].EntryContext = NULL;
        QueryTable[1].Name = L"DiskCopyDataDisabled";
        QueryTable[1].Flags = RTL_QUERY_REGISTRY_DIRECT;
        QueryTable[1].EntryContext = &longValue;

        // Issue query
        NTSTATUS status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE, (PWSTR)wstrDriverRegistryPath, QueryTable, NULL, NULL);
        if (NT_SUCCESS(status) && longValue > 0)
            _is_disk_copy_data_disabled = true;

        // Do not forget to free buffers
        ExFreePool(wstrDriverRegistryPath);
    }
    return _is_disk_copy_data_disabled;
}