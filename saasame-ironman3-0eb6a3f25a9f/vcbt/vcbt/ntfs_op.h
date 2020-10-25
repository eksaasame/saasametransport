#ifndef _NTFS_OP_
#define _NTFS_OP_

#include "type.h"
#include <ntddk.h>
#include "ntfs.h"
#include "file.h"

#define __DEBUG_NTFS 0
#define	ATTR_NUMS		16				// Attribute Types count
#define	ATTR_INDEX(at)	(((at)>>4)-1)	// Attribute Type to Index, eg. 0x10->0, 0x30->2
#define	ATTR_MASK(at)	(((DWORD)1)<<ATTR_INDEX(at))	// Attribute Bit Mask

// Bit masks of Attributes
#define	MASK_STANDARD_INFORMATION	ATTR_MASK(ATTR_TYPE_STANDARD_INFORMATION)
#define	MASK_ATTRIBUTE_LIST			ATTR_MASK(ATTR_TYPE_ATTRIBUTE_LIST)
#define	MASK_FILE_NAME				ATTR_MASK(ATTR_TYPE_FILE_NAME)
#define	MASK_OBJECT_ID				ATTR_MASK(ATTR_TYPE_OBJECT_ID)
#define	MASK_SECURITY_DESCRIPTOR	ATTR_MASK(ATTR_TYPE_SECURITY_DESCRIPTOR)
#define	MASK_VOLUME_NAME			ATTR_MASK(ATTR_TYPE_VOLUME_NAME)
#define	MASK_VOLUME_INFORMATION		ATTR_MASK(ATTR_TYPE_VOLUME_INFORMATION)
#define	MASK_DATA					ATTR_MASK(ATTR_TYPE_DATA)
#define	MASK_INDEX_ROOT				ATTR_MASK(ATTR_TYPE_INDEX_ROOT)
#define	MASK_INDEX_ALLOCATION		ATTR_MASK(ATTR_TYPE_INDEX_ALLOCATION)
#define	MASK_BITMAP					ATTR_MASK(ATTR_TYPE_BITMAP)
#define	MASK_REPARSE_POINT			ATTR_MASK(ATTR_TYPE_REPARSE_POINT)
#define	MASK_EA_INFORMATION			ATTR_MASK(ATTR_TYPE_EA_INFORMATION)
#define	MASK_EA						ATTR_MASK(ATTR_TYPE_EA)
#define	MASK_LOGGED_UTILITY_STREAM	ATTR_MASK(ATTR_TYPE_LOGGED_UTILITY_STREAM)

#define	MASK_ALL					((DWORD)-1)

typedef struct _NTFS_FILE_RECORD{
    ULONGLONG                   fileRef;
    PFILE_RECORD_HEADER         fr;
    LIST_ENTRY                  attrs[ATTR_NUMS];
}NTFS_FILE_RECORD, *PNTFS_FILE_RECORD;

typedef struct _DATA_RUN_ENTRY{
    LIST_ENTRY          bind;
    LONGLONG			lcn;		// -1 to indicate sparse data
    ULONGLONG			clusters;
    ULONGLONG			start_vcn;
    ULONGLONG			last_vcn;
} DATA_RUN_ENTRY, *PDATA_RUN_ENTRY;

typedef struct _NTFS_ATTR{
    LIST_ENTRY          bind;
    PNTFS_ATTRIBUTE     ahc;
    VOID*               attr_body;
    DWORD               attr_body_size;
    LIST_ENTRY          data_runs;
}NTFS_ATTR, *PNTFS_ATTR;

typedef struct _NTFS_VOLUME{
    PDEVICE_OBJECT      target;
    WORD                sector_size;
    WORD                version;
    DWORD               cluster_size;
    DWORD               file_record_size;
    DWORD               index_block_size;
    ULONGLONG           mft_addr;
    PNTFS_ATTR          mft_data;
    PNTFS_FILE_RECORD   mft;
}NTFS_VOLUME, *PNTFS_VOLUME;

PNTFS_VOLUME               get_ntfs_volume_info(PDEVICE_OBJECT target);
PNTFS_FILE_RECORD          get_file_record(const PNTFS_VOLUME volume, ULONGLONG fileRef, DWORD mask);
void                       free_file_record(PNTFS_FILE_RECORD* pfr);
void                       free_ntfs_volume(PNTFS_VOLUME *pv);
bool                       find_sub_entry(const PNTFS_VOLUME volume, const PNTFS_FILE_RECORD fr, const wchar_t *fileName, PULONGLONG fileRef);
PRETRIEVAL_POINTERS_BUFFER ntfs_get_file_retrieval_pointers(PNTFS_FILE_RECORD fr);
ULONGLONG                  ntfs_get_file_clusters(PNTFS_FILE_RECORD fr);
bool                       is_deleted(PNTFS_FILE_RECORD fr);
bool                       is_directory(PNTFS_FILE_RECORD fr);
#endif