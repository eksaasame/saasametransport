#ifndef _VCBT_HEADER_
#define _VCBT_HEADER_

#define INITGUID

#include "ntddk.h"
#include "ntdddisk.h"
#include "stdarg.h"
#include "stdio.h"
#include <ntddvol.h>

#include <mountdev.h>
#include "wmistr.h"
#include "wmidata.h"
#include "wmiguid.h"
#include "wmilib.h"

#include "ntstrsafe.h"
#include "lock.h"
#include "type.h"
#include "journal.h"
#include "umap.h"
#include "file.h"
#include "ntfs_op.h"

#define VCBT_MAX_NUM_STR        22
#define VCBT_MAXSTR             64
#define VCBT_MAX_PATH           VCBT_MAXSTR * 2
#define VCBT_VOLUME_SYS_INFO    L"\\System Volume Information"
#define VCBT_VOLUME_PREFIX      L"\\??\\Volume"

typedef struct _IO_RANGE {
    LIST_ENTRY              Bind;
    ULONG                   Flags;
    LONGLONG                Start;
    ULONG                   Length;
}IO_RANGE, *PIO_RANGE;

typedef struct _JOURNAL_DATA{
    BOOLEAN                          Ready;
    BOOLEAN                          Check;
    BOOLEAN                          Initialized;
    spin_lock                        Lock;
    HANDLE                           JournalMetaDataFileHandle;
    HANDLE                           JournalFileHandle;
    HANDLE                           UmapFileHandle;
    UNICODE_STRING                   JournalMetaDataFileName;
    WCHAR                            JournalMetaDataFileNameBuffer[VCBT_MAX_PATH];
    UNICODE_STRING                   JournalFileName;
    WCHAR                            JournalFileNameBuffer[VCBT_MAX_PATH];
    UNICODE_STRING                   UmapFileName;
    WCHAR                            UmapFileNameBuffer[VCBT_MAX_PATH];
    LONG                             FsClusterSize;
    LONG                             BytesPerSector;
    BOOLEAN                          NeedUpdateRetrievalPointers;
    PRETRIEVAL_POINTERS_BUFFER       JournalMetaDataRetrievalPointers;
    PRETRIEVAL_POINTERS_BUFFER       JournalRetrievalPointers;
    PRETRIEVAL_POINTERS_BUFFER       UmapRetrievalPointers;
    PRETRIEVAL_POINTERS_BUFFER       NewJournalMetaDataRetrievalPointers;
    PRETRIEVAL_POINTERS_BUFFER       NewJournalRetrievalPointers;
    PRETRIEVAL_POINTERS_BUFFER       NewUmapRetrievalPointers;
    LIST_ENTRY                       WrittenList;
    ULONG                            Resolution;
    VCBT_JOURNAL_META_DATA           JournalMetaData;
    VCBT_JOURNAL_META_DATA           BackupJournalMetaData;
    BYTE                             UMAP[RESOLUTION_UMAP_SIZE];
    PVCBT_JOURNAL_BLOK               JournalBlockPtr;
    BYTE*                            JournalView;
    LONG                             JournalViewSize;
    LONGLONG                         JournalViewOffset;
    ULONGLONG                        FileAreaOffset;
}JOURNAL_DATA, *PJOURNAL_DATA;

//
// Device Extension
//

typedef struct _DEVICE_EXTENSION {
    LIST_ENTRY              Bind;
    //
    // Back pointer to device object
    //

    PDEVICE_OBJECT DeviceObject;

    //
    // Target Device Object
    //

    PDEVICE_OBJECT TargetDeviceObject;

    //
    // Physical device object
    //
    PDEVICE_OBJECT PhysicalDeviceObject;

    //
    // Disk number for reference in WMI
    //

    STORAGE_DEVICE_NUMBER   Number;
    ULONG                   VolumeNumber;
    //
    // If device is enabled for counting always
    //

    bool                    Enabled;
    
    //
    // Use to keep track of Volume info from ntddvol.h
    //
    WCHAR                   StorageManagerName[8];
    WCHAR                   MountDeviceLink[VCBT_MAXSTR];
    
    //
    // must synchronize paging path notifications
    //
    KEVENT                  PagingPathCountEvent;
    LONG                    PagingPathCount;

    //
    // Physical Device name or WMI Instance Name
    //
    UNICODE_STRING          PhysicalDeviceName;
    WCHAR                   PhysicalDeviceNameBuffer[VCBT_MAXSTR];

    WCHAR                   FileSystemName[8];
    LIST_ENTRY              ExcludedList;
    spin_lock               ExcludedLock;
    LIST_ENTRY              QueueList;
    KSPIN_LOCK              QueueLock;
    KEVENT                  QueueEvent;
    KEVENT                  KillEvent;
    KTIMER                  Timer;
    PKTHREAD                Thread;
    CLIENT_ID               ThreadId;
    PKTHREAD                CheckThread;
    CLIENT_ID               CheckThreadId;
    PJOURNAL_DATA           Journal;

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

#define DEVICE_EXTENSION_SIZE sizeof(DEVICE_EXTENSION)


//
// Control Device Extension
//

typedef struct _CONTROL_DEVICE_EXTENSION {
    LIST_ENTRY              DeviceList;
    spin_lock               DeviceListLock;
    ULONG                   SystemProcessId;
    BOOLEAN                 JournalDisabled;
    BOOLEAN                 BatchUpdateJournalMetaData;
    BOOLEAN                 UmapFlushDisabled;
    BOOLEAN                 VerboseDebug;
    BOOLEAN                 DiskCopyDataDisabled;
} CONTROL_DEVICE_EXTENSION, *PCONTROL_DEVICE_EXTENSION;


typedef struct _IO_EXCLUDE_RANGE {
    LIST_ENTRY              Bind;
    LONGLONG                Start;
    LONGLONG                End;
}IO_EXCLUDE_RANGE, *PIO_EXCLUDE_RANGE;

#define LIST_FOR_EACH(pos, head) \
	for (pos = (head)->Blink ; pos != (head); pos = pos->Blink )

#define LIST_FOR_EACH_SAFE(pos, n, head) \
	for (pos = (head)->Blink , n = pos->Blink ; pos != (head); pos = n, n = pos->Blink )

UNICODE_STRING VcbtRegistryPath;

//
// Function declarations
//

DRIVER_INITIALIZE DriverEntry;

DRIVER_ADD_DEVICE VcbtAddDevice;

DRIVER_DISPATCH VcbtForwardIrpSynchronous;

__drv_dispatchType(IRP_MJ_PNP)
DRIVER_DISPATCH VcbtDispatchPnp;

__drv_dispatchType(IRP_MJ_POWER)
DRIVER_DISPATCH VcbtDispatchPower;

DRIVER_DISPATCH VcbtSendToNextDriver;

__drv_dispatchType(IRP_MJ_CREATE)
DRIVER_DISPATCH VcbtCreate;

__drv_dispatchType(IRP_MJ_READ)
__drv_dispatchType(IRP_MJ_WRITE)
DRIVER_DISPATCH VcbtReadWrite;

__drv_dispatchType(IRP_MJ_DEVICE_CONTROL)
DRIVER_DISPATCH VcbtDeviceControl;

__drv_dispatchType(IRP_MJ_FLUSH_BUFFERS)
__drv_dispatchType(IRP_MJ_SHUTDOWN)
DRIVER_DISPATCH VcbtShutdownFlush;

DRIVER_DISPATCH VcbtStartDevice;
DRIVER_DISPATCH VcbtRemoveDevice;

IO_COMPLETION_ROUTINE VcbtIoCompletion;
IO_COMPLETION_ROUTINE VcbtIrpCompletion;

DRIVER_UNLOAD VcbtUnload;


VOID
VcbtLogError(
IN PDEVICE_OBJECT DeviceObject,
IN ULONG UniqueId,
IN NTSTATUS ErrorCode,
IN NTSTATUS Status
);

NTSTATUS
VcbtRegisterDevice(
IN PDEVICE_OBJECT DeviceObject
);

VOID
VcbtSyncFilterWithTarget(
IN PDEVICE_OBJECT FilterDevice,
IN PDEVICE_OBJECT TargetDevice
);

NTSTATUS StartThread(PDEVICE_EXTENSION pdx);
VOID     StopThread(PDEVICE_EXTENSION pdx);
VOID     ThreadProc(PDEVICE_EXTENSION pdx);
VOID     InitialJournal(PDEVICE_EXTENSION pdx);

VOID     FreeJournal(PDEVICE_EXTENSION pdx);
NTSTATUS EnableJournal(PDEVICE_EXTENSION pdx, PLONG sizeInMegaBytes, LONGLONG* JournalId);
NTSTATUS DisableJournal(PDEVICE_EXTENSION pdx);

VOID     CheckJournal(PDEVICE_EXTENSION pdx);
NTSTATUS GetFilesRetrievalPointers(PDEVICE_EXTENSION pdx);
NTSTATUS FlushUmapData(PDEVICE_EXTENSION pdx, LONGLONG offset, LONG length, BOOLEAN full);

NTSTATUS InitialJournalData(PDEVICE_EXTENSION pdx);
NTSTATUS WriteJournalData(PDEVICE_EXTENSION pdx, ULONGLONG offset, ULONG length);
//NTSTATUS FlushJournalData(PDEVICE_EXTENSION pdx, BYTE* buff, LONGLONG offset, LONG length); Internal usage

NTSTATUS Snapshot(PDEVICE_EXTENSION pdx);
NTSTATUS PostSnapshot(PDEVICE_EXTENSION pdx);
NTSTATUS UndoSnapshot(PDEVICE_EXTENSION pdx);

NTSTATUS OpenUmapFile(PDEVICE_EXTENSION pdx);

NTSTATUS fastFsdRequest(
    IN PDEVICE_OBJECT DeviceObject,
    ULONG majorFunction,
    IN LONGLONG ByteOffset,
    OUT PVOID Buffer,
    IN ULONG Length,
    IN BOOLEAN Wait);

NTSTATUS fastFsdRequest2(
    IN PDEVICE_OBJECT DeviceObject,
    ULONG majorFunction,
    IN LONGLONG ByteOffset,
    OUT PVOID Buffer,
    IN ULONG Length,
    IN BOOLEAN Wait);

//
// Define the sections that allow for discarding (i.e. paging) some of
// the code.
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (PAGE, VcbtCreate)
#pragma alloc_text (PAGE, VcbtAddDevice)
#pragma alloc_text (PAGE, VcbtDispatchPnp)
#pragma alloc_text (PAGE, VcbtStartDevice)
#pragma alloc_text (PAGE, VcbtRemoveDevice)
#pragma alloc_text (PAGE, VcbtUnload)
#pragma alloc_text (PAGE, VcbtRegisterDevice)
#pragma alloc_text (PAGE, VcbtSyncFilterWithTarget)
#endif

#endif