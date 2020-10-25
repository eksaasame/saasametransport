#ifndef _FILE_
#define _FILE_

#include "type.h"
#include <ntddk.h>

#define FILE_DEVICE_FILE_SYSTEM            0x00000009
#define FSCTL_QUERY_RETRIEVAL_POINTERS     CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 14,  METHOD_NEITHER, FILE_ANY_ACCESS)
#define FSCTL_GET_RETRIEVAL_POINTERS       CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 28,  METHOD_NEITHER, FILE_ANY_ACCESS) // STARTING_VCN_INPUT_BUFFER, RETRIEVAL_POINTERS_BUFFER
#define FSCTL_GET_RETRIEVAL_POINTER_BASE   CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 141, METHOD_BUFFERED, FILE_ANY_ACCESS) // RETRIEVAL_POINTER_BASE
#define FSCTL_MARK_HANDLE                  CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 63, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define USN_SOURCE_DATA_MANAGEMENT 0x00000001
#define MARK_HANDLE_PROTECT_CLUSTERS 0x00000001

NTSTATUS ZwFsControlFile(
    _In_      HANDLE           FileHandle,
    _In_opt_  HANDLE           Event,
    _In_opt_  PIO_APC_ROUTINE  ApcRoutine,
    _In_opt_  PVOID            ApcContext,
    _Out_     PIO_STATUS_BLOCK IoStatusBlock,
    _In_      ULONG            FsControlCode,
    _In_opt_  PVOID            InputBuffer,
    _In_      ULONG            InputBufferLength,
    _Out_opt_ PVOID            OutputBuffer,
    _In_      ULONG            OutputBufferLength
    );

NTSTATUS ZwFlushBuffersFile(
    _In_  HANDLE           FileHandle,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock
    );

typedef struct {
    DWORD  UsnSourceInfo;
    HANDLE VolumeHandle;
    DWORD  HandleInfo;
} MARK_HANDLE_INFO, *PMARK_HANDLE_INFO;

typedef struct {
    LARGE_INTEGER StartingVcn;
} STARTING_VCN_INPUT_BUFFER, *PSTARTING_VCN_INPUT_BUFFER;

typedef struct RETRIEVAL_POINTERS_BUFFER {
    ULONG ExtentCount;
    LARGE_INTEGER StartingVcn;
    struct {
        LARGE_INTEGER NextVcn;
        LARGE_INTEGER Lcn;
    } Extents[1];
} RETRIEVAL_POINTERS_BUFFER, *PRETRIEVAL_POINTERS_BUFFER;

typedef struct _MAPPING_PAIR {
    LONGLONG  SectorLengthInBytes;
    LONGLONG  StartingLogicalOffsetInBytes;
} MAPPING_PAIR, *PMAPPING_PAIR;;

NTSTATUS create_directory(IN PUNICODE_STRING path);
NTSTATUS open_file(OUT PHANDLE handle, IN PUNICODE_STRING path);
NTSTATUS open_file_with_share(OUT PHANDLE handle, IN PUNICODE_STRING path);
NTSTATUS open_file_read_only(OUT PHANDLE handle, IN PUNICODE_STRING path);
NTSTATUS create_file(OUT PHANDLE handle, IN PUNICODE_STRING path);
NTSTATUS create_file_with_arrtibutes(OUT PHANDLE handle, IN PUNICODE_STRING path, IN LONG attributes);
NTSTATUS get_file_base_info(IN HANDLE handle, PFILE_BASIC_INFORMATION fbi);
NTSTATUS set_file_base_info(IN HANDLE handle, PFILE_BASIC_INFORMATION fbi);
NTSTATUS set_end_of_file(IN HANDLE handle, PLARGE_INTEGER end_of_file);
NTSTATUS set_file_pointer(IN HANDLE handle, PLARGE_INTEGER current_byte_offset);
NTSTATUS delete_file(IN HANDLE handle);
NTSTATUS delete_file_ex(IN PUNICODE_STRING path);
NTSTATUS read_file(IN HANDLE handle, OUT void * buffer, IN UINT32 length, IN UINT64 start_offset);
NTSTATUS read_file_ex(IN HANDLE handle, OUT void * buffer, IN UINT32 length, IN UINT64 start_offset, OUT PUINT32 number_of_bytes_read);
NTSTATUS write_file(IN HANDLE handle, IN const void * buffer, IN UINT32 length, IN UINT64 start_offset);
NTSTATUS rename_file(IN HANDLE handle, IN PUNICODE_STRING new_path);
NTSTATUS query_file_retrieval_pointers( IN HANDLE handle, OUT PMAPPING_PAIR* buffer );
NTSTATUS get_file_retrieval_pointers(IN HANDLE handle, IN LONGLONG start_vcn, OUT PRETRIEVAL_POINTERS_BUFFER* buffer);
NTSTATUS get_retrieval_pointer_base(IN HANDLE handle, IN LONG bytes_per_sector, OUT PULONGLONG file_area_offset);
NTSTATUS get_fs_cluster_size(IN HANDLE handle, PLONG cluster_size, PLONG bytes_per_sector);
NTSTATUS get_file_size(IN HANDLE handle, PLARGE_INTEGER file_size);

NTSTATUS close_file(IN HANDLE handle);

NTSTATUS create_file_section(OUT PHANDLE section, IN PUNICODE_STRING filename, IN HANDLE handle, IN OUT PLARGE_INTEGER maximum_size);
NTSTATUS file_map(IN HANDLE section, OUT PVOID* address, OUT PLARGE_INTEGER offset, PSIZE_T view_size);
NTSTATUS file_unmap(IN PVOID address);

#define _close_file(h) if( NULL != h ) { close_file(h); h = NULL; }
#define _delete_file(h) if( NULL != h ) { delete_file(h); h = NULL; }
#endif