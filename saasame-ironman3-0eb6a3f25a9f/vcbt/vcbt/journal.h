
#ifndef _JOURNAL_
#define _JOURNAL_

#ifdef _NTDDK_
#include "type.h"
#include "file.h"
#include <ntddk.h>
#else
#ifndef _WINIOCTL
#define _WINIOCTL
#include <winioctl.h>
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define VCBT_DEVICE_FLT			0xA000
#define VCBT_IOCTL_BASE			0xA00

#define CTL_CODE_VCBT_FLT(code)	\
	CTL_CODE(VCBT_DEVICE_FLT, VCBT_IOCTL_BASE+code, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_VCBT_ENABLE				CTL_CODE_VCBT_FLT(0)
#define IOCTL_VCBT_DISABLE  			CTL_CODE_VCBT_FLT(1)
#define IOCTL_VCBT_SNAPSHOT				CTL_CODE_VCBT_FLT(2)
#define IOCTL_VCBT_POST_SNAPSHOT	    CTL_CODE_VCBT_FLT(3)
#define IOCTL_VCBT_UNDO_SNAPSHOT  	    CTL_CODE_VCBT_FLT(4)
#define IOCTL_VCBT_IS_ENABLE   	        CTL_CODE_VCBT_FLT(5)
#define IOCTL_VCBT_RUNTIME            	CTL_CODE_VCBT_FLT(6)

#define VCBT_WIN32_DEVICE_NAME_A	"\\\\.\\VCBT"
#define VCBT_WIN32_DEVICE_NAME_W	L"\\\\.\\VCBT"

#define VCBT_DOS_DEVICE_NAME_W	L"\\DosDevices\\VCBT"

#define VCBT_DEVICE_NAME_W		L"\\Device\\VCBT"

#ifdef _UNICODE
#define VCBT_WIN32_DEVICE_NAME	VCBT_WIN32_DEVICE_NAME_W
#else
#define VCBT_WIN32_DEVICE_NAME	VCBT_WIN32_DEVICE_NAME_A
#endif

#define CBT_JOURNAL_METADATA    L"\\journal.metadata"
#define CBT_JOURNAL_FILE1       L"\\v1.journal"
#define CBT_JOURNAL_METADATA_   L"journal.metadata"
#define CBT_JOURNAL_FILE1_      L"v1.journal"

#define JOURNAL_MIN_SIZE_IN_BYTES        1048576
#define JOURNAL_MAX_SIZE_IN_MB           100 
#define JOURNAL_MAX_SIZE_IN_BYTES        JOURNAL_MAX_SIZE_IN_MB * JOURNAL_MIN_SIZE_IN_BYTES

#pragma pack(push)  /* push current alignment to stack */
#pragma pack(2)     /* set alignment to 2 byte boundary */

typedef struct _VCBT_RECORD{
    ULONGLONG        Key;
    ULONGLONG        Start;
    ULONG            Length;
}VCBT_RECORD, *PVCBT_RECORD;

typedef struct _VCBT_JOURNAL
{
    ULONGLONG        JournalId;
    ULONG            Size;
    ULONG            CheckSum;
    unsigned char    Signature[4];  //Signature[0]='c'; Signature[1]='b'; Signature[2]='t'; Signature[3]='1';
} VCBT_JOURNAL, *PVCBT_JOURNAL;

typedef struct _VCBT_JOURNAL_BLOK{
        VCBT_RECORD  r;
        VCBT_JOURNAL j;
}VCBT_JOURNAL_BLOK, *PVCBT_JOURNAL_BLOK;

typedef struct _VCBT_JOURNAL_META_DATA
{
    ULONGLONG           FirstKey;
    VCBT_JOURNAL_BLOK   Block;
    unsigned char       Reversed[4];
} VCBT_JOURNAL_META_DATA, *PVCBT_JOURNAL_META_DATA;

#pragma pack(pop)   /* restore original alignment from stack */

typedef struct _VCBT_COMMAND{
    GUID        VolumeId;
    union{
        LONG        JournalSizeInMegaBytes;
        BOOLEAN     Creatable;
    }Detail;
}VCBT_COMMAND, *PVCBT_COMMAND;

typedef struct _VCOMMAND_RESULT{
    GUID            VolumeId;
    NTSTATUS        Status;
    LONGLONG        JournalId;
    union{
        LONG        JournalSizeInMegaBytes;
        BOOLEAN     Created;
        BOOLEAN     Enabled;
        BOOLEAN     Done;
    }Detail;
}VCOMMAND_RESULT, *PVCOMMAND_RESULT;

typedef struct _VCBT_COMMAND_INPUT{
    LONG            NumberOfCommands;
    VCBT_COMMAND    Commands[1];
} VCBT_COMMAND_INPUT, *PVCBT_COMMAND_INPUT;

typedef struct _VCBT_COMMAND_RESULT{
    LONG            NumberOfResults;
    VCOMMAND_RESULT Results[1];
} VCBT_COMMAND_RESULT, *PVCBT_COMMAND_RESULT;

typedef enum _VCBT_RUNTIME_FLAG{
    JOURNAL = 0,
    JOURNAL_META,
    UMAP,
    JOURNAL_FILE,
    JOURNAL_META_FILE,
    UMAP_FILE
}VCBT_RUNTIME_FLAG;

typedef struct _VCBT_RUNTIME_COMMAND{
    GUID              VolumeId;
    VCBT_RUNTIME_FLAG Flag;
}VCBT_RUNTIME_COMMAND, *PVCBT_RUNTIME_COMMAND;

typedef struct _VCBT_RUNTIME_RESULT{
    BOOLEAN                    Ready;
    BOOLEAN                    Check;
    BOOLEAN                    Initialized;
    LONG                       FsClusterSize;
    LONG                       BytesPerSector;
    ULONG                      Resolution;
    LONG                       JournalViewSize;
    LONGLONG                   JournalViewOffset;
    ULONGLONG                  FileAreaOffset;
    VCBT_JOURNAL_META_DATA     JournalMetaData;
    RETRIEVAL_POINTERS_BUFFER  RetrievalPointers;
}VCBT_RUNTIME_RESULT, *PVCBT_RUNTIME_RESULT;

#ifdef __cplusplus
}
#endif

#endif