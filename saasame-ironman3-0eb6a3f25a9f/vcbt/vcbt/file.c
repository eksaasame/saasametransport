#include "file.h"
//#include <Ntifs.h>
#include "trace.h"
#include "file.tmh"

typedef struct _RETRIEVAL_POINTER_BASE {
    LARGE_INTEGER FileAreaOffset;
} RETRIEVAL_POINTER_BASE, *PRETRIEVAL_POINTER_BASE;

typedef struct _FILE_RENAME_INFORMATION {
    BOOLEAN ReplaceIfExists;
    HANDLE  RootDirectory;
    ULONG   FileNameLength;
    WCHAR   FileName[1];
} FILE_RENAME_INFORMATION, *PFILE_RENAME_INFORMATION;

NTSTATUS create_directory(IN PUNICODE_STRING path){

    NTSTATUS status;
    OBJECT_ATTRIBUTES directoryAttributesObject;
    IO_STATUS_BLOCK directoryIoStatusBlock;
    LARGE_INTEGER allocate;
    HANDLE hDirectory;
    allocate.QuadPart = 0x1000;

    InitializeObjectAttributes(
        &directoryAttributesObject,
        path,
        OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    /* You can't use FltCreateFile here */
    /* Instance may not be the same instance we want to write to eg a
    flash drive writing a file to a ntfs partition */
    status = ZwCreateFile(
        &hDirectory,
        0,
        &directoryAttributesObject,
        &directoryIoStatusBlock,
        &allocate,
        0,
        0,
        FILE_CREATE,
        FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_WRITE_THROUGH,
        NULL,
        0
        );
   
    if (NT_SUCCESS(status) && NT_SUCCESS(directoryIoStatusBlock.Status)){
        close_file(hDirectory);
    }
    else {
        if (status != STATUS_OBJECT_NAME_COLLISION){
            DoTraceMsg(TRACE_LEVEL_ERROR, "create_directory: create directory (%wZ) . ERROR: 0x%08x & 0x%08x", path, status, directoryIoStatusBlock.Status);
        }
        else{
            status = STATUS_SUCCESS;
        }
    }
    return status;
}

NTSTATUS open_file(OUT PHANDLE handle, IN PUNICODE_STRING path){
    NTSTATUS            Status;
    IO_STATUS_BLOCK     IoStatus;
    OBJECT_ATTRIBUTES   objectAttributes;
    InitializeObjectAttributes(&objectAttributes,
        path,
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
        NULL,
        NULL);
    Status = ZwCreateFile(handle,
        GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
        &objectAttributes,
        &IoStatus,
        0,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ,
        FILE_OPEN,
        FILE_NO_INTERMEDIATE_BUFFERING | FILE_SYNCHRONOUS_IO_NONALERT | FILE_WRITE_THROUGH,
        NULL,
        0);
    if (!NT_SUCCESS(Status)){
        DoTraceMsg(TRACE_LEVEL_WARNING, "Failed to open file (%wZ), status: 0x%08X", path, Status);
    }
    return Status;
}

NTSTATUS open_file_with_share(OUT PHANDLE handle, IN PUNICODE_STRING path){
    NTSTATUS            Status;
    IO_STATUS_BLOCK     IoStatus;
    OBJECT_ATTRIBUTES   objectAttributes;
    InitializeObjectAttributes(&objectAttributes,
        path,
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
        NULL,
        NULL);
    Status = ZwCreateFile(handle,
        GENERIC_ALL | SYNCHRONIZE,
        &objectAttributes,
        &IoStatus,
        0,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_SYNCHRONOUS_IO_NONALERT,
        NULL,
        0);
    if (!NT_SUCCESS(Status)){
        DoTraceMsg(TRACE_LEVEL_WARNING, "Failed to open file (%wZ) with share, status: 0x%08X", path, Status);
    }
    return Status;
}

NTSTATUS open_file_read_only(OUT PHANDLE handle, IN PUNICODE_STRING path){

    NTSTATUS            Status;
    OBJECT_ATTRIBUTES   objectAttributes;
    IO_STATUS_BLOCK     IoStatus;

    InitializeObjectAttributes(&objectAttributes,
        path,
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
        NULL,
        NULL);

    Status = ZwCreateFile(handle,
        GENERIC_READ | SYNCHRONIZE,
        &objectAttributes,
        &IoStatus,
        NULL,
        0,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_SYNCHRONOUS_IO_NONALERT,
        NULL,
        0);

    if (!NT_SUCCESS(Status)){
        DoTraceMsg(TRACE_LEVEL_ERROR, "Failed to open %wZ with readonly, status: 0x%08X", path, Status);
    }

    return Status;
}

NTSTATUS create_file(OUT PHANDLE handle, IN PUNICODE_STRING path){

    NTSTATUS            Status;
    OBJECT_ATTRIBUTES   objectAttributes;
    IO_STATUS_BLOCK     IoStatus;

    InitializeObjectAttributes(&objectAttributes,
        path,
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
        NULL,
        NULL);

    Status = ZwCreateFile(handle,
        GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
        &objectAttributes,
        &IoStatus,
        0,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ,
        FILE_OPEN_IF,
        FILE_NO_INTERMEDIATE_BUFFERING | FILE_SYNCHRONOUS_IO_NONALERT | FILE_WRITE_THROUGH,
        NULL,
        0);

    if (!NT_SUCCESS(Status)){
        DoTraceMsg(TRACE_LEVEL_ERROR, "Failed to create %wZ, status: 0x%08X", path, Status);
    }

    return Status;
}

NTSTATUS create_file_with_arrtibutes(OUT PHANDLE handle, IN PUNICODE_STRING path, IN LONG attributes){

    NTSTATUS            Status;
    OBJECT_ATTRIBUTES   objectAttributes;
    IO_STATUS_BLOCK     IoStatus;

    InitializeObjectAttributes(&objectAttributes,
        path,
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
        NULL,
        NULL);

    Status = ZwCreateFile(handle,
        GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
        &objectAttributes,
        &IoStatus,
        0,
        FILE_ATTRIBUTE_NORMAL|attributes,
        FILE_SHARE_READ,
        FILE_OPEN_IF,
        FILE_NO_INTERMEDIATE_BUFFERING | FILE_SYNCHRONOUS_IO_NONALERT | FILE_WRITE_THROUGH,
        NULL,
        0);

    if (!NT_SUCCESS(Status)){
        DoTraceMsg(TRACE_LEVEL_ERROR, "Failed to create %wZ, status: 0x%08X", path, Status);
    }

    return Status;
}

NTSTATUS get_file_base_info(IN HANDLE handle, PFILE_BASIC_INFORMATION fbi){
    NTSTATUS        Status;
    IO_STATUS_BLOCK IoStatus;
    if (!NT_SUCCESS(Status = ZwQueryInformationFile(handle, &IoStatus, fbi, sizeof(FILE_BASIC_INFORMATION), FileBasicInformation))){
        DoTraceMsg(TRACE_LEVEL_ERROR, "Failed to get file attirbutes, status: 0x%08X", Status);
    }
    return Status;
}

NTSTATUS set_file_base_info(IN HANDLE handle, PFILE_BASIC_INFORMATION fbi){
    NTSTATUS        Status;
    IO_STATUS_BLOCK IoStatus;
    if (!NT_SUCCESS(Status = ZwSetInformationFile(handle, &IoStatus, fbi, sizeof(FILE_BASIC_INFORMATION), FileBasicInformation))){
        DoTraceMsg(TRACE_LEVEL_ERROR, "Failed to set file attirbutes, status: 0x%08X", Status);
    }
    return Status;
}

NTSTATUS set_end_of_file(IN HANDLE handle, PLARGE_INTEGER end_of_file){
    NTSTATUS        Status;
    IO_STATUS_BLOCK IoStatus;
    FILE_END_OF_FILE_INFORMATION endfile;
    endfile.EndOfFile.QuadPart = end_of_file->QuadPart;
    if (!NT_SUCCESS(Status = ZwSetInformationFile(handle, &IoStatus, &endfile, sizeof(FILE_END_OF_FILE_INFORMATION), FileEndOfFileInformation))){
        DoTraceMsg(TRACE_LEVEL_ERROR, "Failed to set end of file (%I64u), status: 0x%08X", end_of_file->QuadPart, Status);
    }
    return Status;
}

NTSTATUS set_file_pointer(IN HANDLE handle, PLARGE_INTEGER current_byte_offset){
    NTSTATUS        Status;
    IO_STATUS_BLOCK IoStatus;
    FILE_POSITION_INFORMATION file_position_info;
    file_position_info.CurrentByteOffset.QuadPart = current_byte_offset->QuadPart;
    if (!NT_SUCCESS(Status = ZwSetInformationFile(handle, &IoStatus, &file_position_info, sizeof(FILE_POSITION_INFORMATION), FilePositionInformation))){
        DoTraceMsg(TRACE_LEVEL_ERROR, "Failed to set file position information (%I64u), status: 0x%08X", current_byte_offset->QuadPart, Status);
    }
    return Status;
}

NTSTATUS delete_file(IN HANDLE handle){
    NTSTATUS        Status;
    IO_STATUS_BLOCK IoStatus;
    FILE_DISPOSITION_INFORMATION fdi;

    fdi.DeleteFile = true;
    Status = ZwSetInformationFile(handle, &IoStatus, &fdi, sizeof(fdi), FileDispositionInformation);

    if (!NT_SUCCESS(Status)){
        DoTraceMsg(TRACE_LEVEL_ERROR, "Failed to delete file, status: 0x%08X", Status);
    }
    return Status;
}

NTSTATUS delete_file_ex(IN PUNICODE_STRING path){
    HANDLE          handle;
    NTSTATUS        Status;
    if (NT_SUCCESS(Status = open_file(&handle, path))){
        Status = delete_file(handle);
        close_file(handle);
    }
    else if (Status == STATUS_OBJECT_NAME_NOT_FOUND || Status == STATUS_OBJECT_PATH_NOT_FOUND){
        Status = STATUS_SUCCESS;
    }
    return Status;
}

NTSTATUS read_file(IN HANDLE handle, OUT void * buffer, IN UINT32 length, IN UINT64 start_offset){

    NTSTATUS            Status;
    LARGE_INTEGER       Offset;
    IO_STATUS_BLOCK     IoStatus;
    Offset.QuadPart = start_offset;
    Status = ZwReadFile(handle,
        NULL,
        NULL,
        NULL,
        &IoStatus,
        buffer,
        length,
        &Offset,
        NULL);

    if (NT_SUCCESS(Status) && IoStatus.Information != length){
        DoTraceMsg(TRACE_LEVEL_WARNING, "The expected size %d is not equal to the return size %d.", length, (UINT32)IoStatus.Information);
        Status = STATUS_DATA_ERROR;
    }
    else if (!NT_SUCCESS(Status)){
        DoTraceMsg(TRACE_LEVEL_ERROR, "Failed to read file, status: 0x%08X", Status);
    } 
    return Status;
}

NTSTATUS read_file_ex(IN HANDLE handle, OUT void * buffer, IN UINT32 length, IN UINT64 start_offset, OUT PUINT32 number_of_bytes_read){

    NTSTATUS            Status;
    LARGE_INTEGER       Offset;
    IO_STATUS_BLOCK     IoStatus;
    Offset.QuadPart = start_offset;
    Status = ZwReadFile(handle,
        NULL,
        NULL,
        NULL,
        &IoStatus,
        buffer,
        length,
        &Offset,
        NULL);

    if (NT_SUCCESS(Status) ){
        if( IoStatus.Information != length )
            DoTraceMsg(TRACE_LEVEL_WARNING, "The expected size %d is not equal to the return size %d.", length, (UINT32)IoStatus.Information);
        if (NULL != number_of_bytes_read)
            *number_of_bytes_read = (UINT32)IoStatus.Information;
    }
    else if (!NT_SUCCESS(Status)){
        DoTraceMsg(TRACE_LEVEL_ERROR, "Failed to read file, status: 0x%08X", Status);
    }
    return Status;
}

NTSTATUS write_file(IN HANDLE handle, IN const void * buffer, IN UINT32 length, IN UINT64 start_offset){

    NTSTATUS            Status;
    IO_STATUS_BLOCK     IoStatus;
    LARGE_INTEGER       Offset;
    Offset.QuadPart = start_offset;
    Status = ZwWriteFile(handle,
        NULL,
        NULL,
        NULL,
        &IoStatus,
        (PVOID)buffer,
        length,
        &Offset,
        NULL);

    if (NT_SUCCESS(Status) && IoStatus.Information != length){
        DoTraceMsg(TRACE_LEVEL_WARNING, "Written size %u is not equal to the expected size %u, status: 0x%08X", (UINT32)IoStatus.Information, length, Status);
    }
    else if (!NT_SUCCESS(Status)){
        DoTraceMsg(TRACE_LEVEL_ERROR, "Failed to write size %u, status: 0x%08X", length, Status);
    }
    return Status;
}

NTSTATUS rename_file(IN HANDLE handle, IN PUNICODE_STRING new_path){
    NTSTATUS            Status = STATUS_SUCCESS;
    IO_STATUS_BLOCK     IoStatus;
    ULONG               length = sizeof(FILE_RENAME_INFORMATION) + new_path->Length;
    PFILE_RENAME_INFORMATION file_rename_info = (PFILE_RENAME_INFORMATION)ExAllocatePoolWithTag(NonPagedPool, length, 'file');
    if (file_rename_info == NULL){
        Status = STATUS_INSUFFICIENT_RESOURCES;
        DoTraceMsg(TRACE_LEVEL_ERROR, "Failed to allocate (FILE_RENAME_INFORMATION), status: 0x%08X", Status);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    memcpy(file_rename_info->FileName, new_path->Buffer, new_path->Length);
    file_rename_info->ReplaceIfExists = TRUE;
    file_rename_info->RootDirectory = NULL;
    file_rename_info->FileNameLength = new_path->Length;
    Status = ZwSetInformationFile(handle, &IoStatus, file_rename_info, length, FileRenameInformation);
    if (!NT_SUCCESS(Status)){
        DoTraceMsg(TRACE_LEVEL_ERROR, "Failed to rename file to (%wZ), status: 0x%08X", new_path, Status);
    }
    ExFreePoolWithTag(file_rename_info, 'file');
    return Status;
}

NTSTATUS query_file_retrieval_pointers(IN HANDLE handle, OUT PMAPPING_PAIR* buffer){
    NTSTATUS            Status = STATUS_SUCCESS;
    PMAPPING_PAIR       mapping = NULL;
    IO_STATUS_BLOCK     IoStatus;
    ULONG ulOutPutSize = 0;
    ULONG uCounts = 200;
    ulOutPutSize = uCounts * sizeof(MAPPING_PAIR);
    mapping = (PMAPPING_PAIR)ExAllocatePoolWithTag(NonPagedPool, ulOutPutSize, 'file');
    if (mapping == NULL){
        Status = STATUS_INSUFFICIENT_RESOURCES;
        DoTraceMsg(TRACE_LEVEL_ERROR, "Failed to ZwFsControlFile(FSCTL_QUERY_RETRIEVAL_POINTERS), status: 0x%08X", Status);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    while ((Status = ZwFsControlFile(handle, NULL, NULL, NULL, &IoStatus, FSCTL_QUERY_RETRIEVAL_POINTERS, NULL, 0, mapping, ulOutPutSize)) == STATUS_BUFFER_OVERFLOW){
        uCounts += 200;
        ulOutPutSize = uCounts * sizeof(MAPPING_PAIR);
        ExFreePoolWithTag(mapping, 'file');
        mapping = (PMAPPING_PAIR)ExAllocatePoolWithTag(NonPagedPool, ulOutPutSize, 'file');
        if (mapping == NULL)
            return STATUS_INSUFFICIENT_RESOURCES;
    }
    if (!NT_SUCCESS(Status)){
        ExFreePoolWithTag(mapping, 'file'); 
        DoTraceMsg(TRACE_LEVEL_ERROR, "Failed to ZwFsControlFile(FSCTL_QUERY_RETRIEVAL_POINTERS), status: 0x%08X", Status);
    }
    else
        *buffer = mapping;
    return Status;
}

NTSTATUS get_file_retrieval_pointers(IN HANDLE handle, IN LONGLONG start_vcn, OUT PRETRIEVAL_POINTERS_BUFFER* buffer){
    NTSTATUS            Status = STATUS_SUCCESS;
    IO_STATUS_BLOCK     IoStatus;
    STARTING_VCN_INPUT_BUFFER starting;
    PRETRIEVAL_POINTERS_BUFFER pVcnPairs;
    starting.StartingVcn.QuadPart = start_vcn;
    ULONG ulOutPutSize = 0;
    ULONG uCounts = 200;
    ulOutPutSize = sizeof(RETRIEVAL_POINTERS_BUFFER) + uCounts * sizeof(LARGE_INTEGER) *2 + sizeof(LARGE_INTEGER);
    pVcnPairs = (PRETRIEVAL_POINTERS_BUFFER)ExAllocatePoolWithTag(NonPagedPool, ulOutPutSize, 'file');
    if (pVcnPairs == NULL){
        Status = STATUS_INSUFFICIENT_RESOURCES;
        DoTraceMsg(TRACE_LEVEL_ERROR, "Failed to ZwFsControlFile(FSCTL_GET_RETRIEVAL_POINTERS), status: 0x%08X", Status);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    while ((Status = ZwFsControlFile(handle, NULL, NULL, NULL, &IoStatus, FSCTL_GET_RETRIEVAL_POINTERS, &starting, sizeof(STARTING_VCN_INPUT_BUFFER), pVcnPairs, ulOutPutSize)) == STATUS_BUFFER_OVERFLOW){
        uCounts += 200;
        ulOutPutSize = sizeof(RETRIEVAL_POINTERS_BUFFER) + uCounts * sizeof(LARGE_INTEGER) * 2 + sizeof(LARGE_INTEGER);
        ExFreePoolWithTag(pVcnPairs, 'file');
        pVcnPairs = (PRETRIEVAL_POINTERS_BUFFER)ExAllocatePoolWithTag(NonPagedPool, ulOutPutSize, 'file');
        if (pVcnPairs == NULL)
            return STATUS_INSUFFICIENT_RESOURCES;
    }
    if (!NT_SUCCESS(Status)){
        ExFreePoolWithTag(pVcnPairs, 'file');
        DoTraceMsg(TRACE_LEVEL_ERROR, "Failed to ZwFsControlFile(FSCTL_GET_RETRIEVAL_POINTERS), status: 0x%08X", Status);
    }
    else
        *buffer = pVcnPairs;
    return Status;
}

NTSTATUS get_retrieval_pointer_base(IN HANDLE handle, IN LONG bytes_per_sector, OUT PULONGLONG file_area_offset){
    NTSTATUS            Status = STATUS_SUCCESS;
    IO_STATUS_BLOCK     IoStatus;
    RETRIEVAL_POINTER_BASE retrieval_pointer_base;
    if (!NT_SUCCESS(Status = ZwFsControlFile(handle, NULL, NULL, NULL, &IoStatus, FSCTL_GET_RETRIEVAL_POINTER_BASE, NULL, 0, &retrieval_pointer_base, sizeof(retrieval_pointer_base)))){
        DoTraceMsg(TRACE_LEVEL_ERROR, "Failed to ZwFsControlFile(FSCTL_GET_RETRIEVAL_POINTER_BASE), status: 0x%08X", Status);
    }
    else{
        *file_area_offset = retrieval_pointer_base.FileAreaOffset.QuadPart * bytes_per_sector;
        DoTraceMsg(TRACE_LEVEL_INFORMATION, "get_retrieval_pointer_base: FileAreaOffset: %I64d", *file_area_offset);
    }
    return Status;
}

NTSTATUS get_fs_cluster_size(IN HANDLE handle, PLONG cluster_size, PLONG bytes_per_sector){
    NTSTATUS Status;
    FILE_FS_SIZE_INFORMATION	sizeoInfo;
    IO_STATUS_BLOCK             IoStatusBlock;
    if (NT_SUCCESS(Status = ZwQueryVolumeInformationFile(handle,
        &IoStatusBlock,
        &sizeoInfo,
        sizeof(sizeoInfo),
        FileFsSizeInformation))){
        if ( NULL != cluster_size )
            *cluster_size = sizeoInfo.BytesPerSector * sizeoInfo.SectorsPerAllocationUnit;
        if (NULL != bytes_per_sector)
            *bytes_per_sector = sizeoInfo.BytesPerSector;
    }
    else{
        DoTraceMsg(TRACE_LEVEL_ERROR, "Failed to ZwQueryVolumeInformationFile(FileFsSizeInformation), status: 0x%08X", Status);
    }
    return Status;
}

NTSTATUS get_file_size(IN HANDLE handle, PLARGE_INTEGER file_size){
    NTSTATUS Status;
    FILE_STANDARD_INFORMATION	sizeoInfo;
    IO_STATUS_BLOCK             IoStatusBlock;
    if (NT_SUCCESS(Status = ZwQueryInformationFile(handle,
        &IoStatusBlock,
        &sizeoInfo,
        sizeof(sizeoInfo),
        FileStandardInformation))){
        if (NULL != file_size)
            (*file_size).QuadPart = sizeoInfo.AllocationSize.QuadPart;
    }
    else{
        DoTraceMsg(TRACE_LEVEL_ERROR, "Failed to ZwQueryInformationFile(FileStandardInformation), status: 0x%08X", Status);
    }
    return Status;
}

NTSTATUS close_file(IN HANDLE handle){
    NTSTATUS            Status;
    //IO_STATUS_BLOCK     IoStatusBlock;
    //if (!NT_SUCCESS(Status = ZwFlushBuffersFile(handle, &IoStatusBlock))){
    //    DoTraceMsg(TRACE_LEVEL_ERROR, "Failed to flush file, status: 0x%08X", Status);
    //}
    if (!NT_SUCCESS(Status = ZwClose(handle))){
        DoTraceMsg(TRACE_LEVEL_ERROR, "Failed to close file, status: 0x%08X", Status);
    }
    return Status;
}

NTSTATUS create_file_section(OUT PHANDLE section, IN PUNICODE_STRING filename, IN HANDLE handle, IN OUT PLARGE_INTEGER maximum_size){

    NTSTATUS            Status = STATUS_SUCCESS;
    OBJECT_ATTRIBUTES   objectAttributes;

    InitializeObjectAttributes(&objectAttributes,
        filename,
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
        NULL,
        NULL);

    Status = ZwCreateSection(section, SECTION_ALL_ACCESS, &objectAttributes, maximum_size, PAGE_EXECUTE_READWRITE, SEC_COMMIT, handle);
    if (!NT_SUCCESS(Status)){
        DoTraceMsg(TRACE_LEVEL_ERROR, "Failed to open section %wZ, status: 0x%08X", filename, Status);
    }

    return Status;
}

NTSTATUS file_map(IN HANDLE section, OUT PVOID* address, OUT PLARGE_INTEGER offset, PSIZE_T view_size){
    NTSTATUS		Status = STATUS_SUCCESS;
    Status = ZwMapViewOfSection(
        section,				// Section 
        ZwCurrentProcess(),		// Process
        address,			    // Mapping Address
        0,						// 
        0,			            // Commit Size
        offset,		            // Start Offset
        view_size,				// View Size
        (SECTION_INHERIT)1,
        0,
        PAGE_READWRITE);

    if (!NT_SUCCESS(Status)){
        DoTraceMsg(TRACE_LEVEL_ERROR, "Failed to map file, status: 0x%08X", Status);
    }
    return Status;
}

NTSTATUS file_unmap(IN PVOID address){
    NTSTATUS Status = STATUS_SUCCESS;
    Status = ZwUnmapViewOfSection(ZwCurrentProcess(), address);
    if (!NT_SUCCESS(Status)){
        DoTraceMsg(TRACE_LEVEL_ERROR, "Failed to un-mapping file, status: 0x%08X", Status);
    }
    return Status;
}