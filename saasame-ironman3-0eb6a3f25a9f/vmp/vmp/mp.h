/**************************************************************************************************/     
/*                                                                                                */     
/* Copyright (c) 2008-2011 Microsoft Corporation.  All Rights Reserved.                           */     
/*                                                                                                */     
/**************************************************************************************************/    

/*++

Module Name:

    mp.h

Abstract:

Author:

Environment:

--*/


#ifndef _MP_H_
#define _MP_H_

#if !defined(_MP_User_Mode_Only)                      // User-mode only.

#if       !defined(_MP_H_skip_WDM_includes)

#include <wdm.h>

#endif // !defined(_MP_H_skip_WDM_includes)

#include <ntdef.h>  
#include <storport.h>  
#include <devioctl.h>
#include <ntddscsi.h>
#include <scsiwmi.h>
#include "lock.h"

#if !defined(_MP_H_skip_includes)

#include <stdio.h>
#include <stdarg.h>

#endif // !defined(_MP_H_skip_includes)

#define VENDOR_ID                   L"SAASAME "
#define VENDOR_ID_ascii             "SAASAME "
#define PRODUCT_ID                  L"Transport         "
#define PRODUCT_ID_ascii            "Transport         "
#define PRODUCT_REV                 L"8888"
#define PRODUCT_REV_ascii           "8888"
#define MP_TAG_GENERAL              'VMPD'
#define MAX_TARGETS                 8
#define MAX_LUNS                    8
#define MAX_IOS_PER_LUN             250
#define TIME_INTERVAL               (1 * 1000 * 1000) //1 second.
#define DEVLIST_BUFFER_SIZE         1024
#define DEVICE_NOT_FOUND            0xFF
#define SECTOR_NOT_FOUND            0xFFFF
#define MAX_DISK_ID_LENGTH          40
#define MP_BLOCK_SIZE               (512)

#define DEFAULT_INITIATOR_ID           7
#define DEFAULT_MAX_TRANSFER_LEN_IN_MB 4
#define GET_FLAG(Flags, Bit)        ((Flags) & (Bit))
#define SET_FLAG(Flags, Bit)        ((Flags) |= (Bit))
#define CLEAR_FLAG(Flags, Bit)      ((Flags) &= ~(Bit))

#define LIST_FOR_EACH(pos, head) \
    for (pos = (head)->Blink ; pos != (head); pos = pos->Blink )

#define LIST_FOR_EACH_SAFE(pos, n, head) \
    for (pos = (head)->Blink , n = pos->Blink ; pos != (head); pos = n, n = pos->Blink )

#define __malloc(size)   ( size > 0 ) ? ExAllocatePoolWithTag(NonPagedPool, size, MP_TAG_GENERAL) : NULL;
#define __free(p)        if(NULL != p) { ExFreePoolWithTag(p, MP_TAG_GENERAL); p = NULL; }

typedef struct _DEVICE_LIST          DEVICE_LIST, *PDEVICE_LIST;
typedef struct _VMP_DRIVER_INFO      VMP_DRIVER_INFO, *PVMP_DRIVER_INFO;
typedef struct _VMP_REG_INFO         VMP_REG_INFO, *PVMP_REG_INFO;
typedef struct _HW_LU_EXTENSION      HW_LU_EXTENSION, *PHW_LU_EXTENSION;
typedef struct _LBA_LIST             LBA_LIST, *PLBA_LIST;

extern 
PVMP_DRIVER_INFO pMPDrvInfoGlobal;

typedef struct _VMP_REG_INFO {
    UNICODE_STRING   VendorId;
    UNICODE_STRING   ProductId;
    UNICODE_STRING   ProductRevision;
    ULONG            InitiatorID;        // Adapter's target ID
    ULONG            MaxTransferLengthInMB;
    ULONG            MaxIOsPerLun;
} VMP_REG_INFO, *PVMP_REG_INFO;

typedef struct _VMP_DRIVER_INFO {                        // The master miniport object. In effect, an extension of the driver object for the miniport.
    VMP_REG_INFO                   MPRegInfo;
    spin_lock                      Lock;
    LIST_ENTRY                     ListMPHBAObj;      // Header of list of HW_HBA_EXT objects.
    PDRIVER_OBJECT                 pDriverObj;
    ULONG                          DrvInfoNbrMPHBAObj;// Count of items in ListMPHBAObj.
    KGUARDED_MUTEX                 ConnectionMutex;
} VMP_DRIVER_INFO, *PVMP_DRIVER_INFO;

typedef enum {
    ActionRead,
    ActionWrite,
    ActionTimer,
    ActionRemove
} VMP_REQUEST_ACTION;

typedef struct _VMP_REQUEST{
    LIST_ENTRY           Bind; // Request List
    LIST_ENTRY	         SrbList;
    ULONG                RequestID;
    VMP_REQUEST_ACTION   Action;
    ULONGLONG            StartingSector;
    ULONGLONG            EndSector;
    ULONGLONG            MaxEndSector;
    ULONG			     Length;
}VMP_REQUEST, *PVMP_REQUEST;

#define	VMP_DEVICE_INFO_MAGIC		'DPMV'
typedef struct _VMP_DEVICE_INFO {
    ULONG			 Magic;
    LIST_ENTRY       Bind;
    UCHAR            DeviceType;
    UCHAR		     PathId;
    UCHAR		     TargetId;
    UCHAR		     Lun;
    WCHAR            DiskId[MAX_DISK_ID_LENGTH];
    CHAR             SzDiskId[MAX_DISK_ID_LENGTH];
    ULONG            DiskSizeMB;
    LARGE_INTEGER    CreatedTimeStamp;
    BOOLEAN		     Missing;
    BOOLEAN          MergeIOs;
    PKEVENT          Event;
    PHW_LU_EXTENSION LuExt;
} VMP_DEVICE_INFO, *PVMP_DEVICE_INFO;

typedef struct _HW_HBA_EXT {                          // Adapter device-object extension allocated by StorPort.
    LIST_ENTRY                     Bind;              // Pointers to next and previous HW_HBA_EXT objects.
    KGUARDED_MUTEX                 LUListLock;
    LIST_ENTRY                     LUList;            // Pointers to HW_LU_EXTENSION objects.
    PVMP_DRIVER_INFO               pMPDrvObj;
    PDRIVER_OBJECT                 pDrvObj;
    spin_lock                      DeviceListLock;
    LIST_ENTRY					   DeviceList;
    SCSI_WMILIB_CONTEXT            WmiLibContext;
    ULONG                          SRBsSeen;
    ULONG                          WMISRBsSeen;
    ULONG                          NbrLUNsperHBA;
    ULONG                          Test;        
    UCHAR                          HostTargetId;
    UCHAR                          AdapterState;
    UCHAR                          VendorId[9];
    UCHAR                          ProductId[17];
    UCHAR                          ProductRevision[5];
    BOOLEAN                        bReportAdapterDone;
    ULONG                          MaxTransferBlocks;
} HW_HBA_EXT, * PHW_HBA_EXT;

// Flag definitions for LUFlags.

#define LU_DEVICE_INITIALIZED   0x0001

typedef struct _HW_LU_EXTENSION {                     // LUN extension allocated by StorPort.
    LIST_ENTRY            List;                       // Pointers to next and previous HW_LU_EXTENSION objects, used in HW_HBA_EXT.
    ULONG                 LUFlags;
    ULONGLONG             MaxBlocks;
    BOOLEAN               bIsMissing;                 // At present, this is set only by a kernel debugger, for testing.
    UCHAR                 DeviceType;
    UCHAR		          PathId;
    UCHAR                 TargetId;
    UCHAR                 Lun;
    WCHAR                 DiskId[MAX_DISK_ID_LENGTH];
    spin_lock             ServiceLock;
    KGUARDED_MUTEX        RequestListLock;
    LIST_ENTRY	          RequestList;
    KGUARDED_MUTEX        WaitingListLock;
    LIST_ENTRY	          WaitingList;
    LARGE_INTEGER         UpdateTimeStamp;
    PKEVENT               Event;
    PVMP_DEVICE_INFO      Dev;
    BOOLEAN				  Requested;
    BOOLEAN               MergeIOs;
} HW_LU_EXTENSION, * PHW_LU_EXTENSION;

typedef struct _HW_SRB_EXTENSION {
    SCSIWMI_REQUEST_CONTEXT WmiRequestContext;
} HW_SRB_EXTENSION, * PHW_SRB_EXTENSION; 

typedef struct _MP_SRB_WORK_PARMS {
    LIST_ENTRY           Bind;
    PIO_WORKITEM         pQueueWorkItem;
    PHW_HBA_EXT          pHBAExt;
    PHW_LU_EXTENSION     pLUExt;
    PSCSI_REQUEST_BLOCK  pSrb;
    VMP_REQUEST_ACTION   Action;
    ULONGLONG            StartingSector;
    ULONGLONG            EndSector;
    ULONGLONG            MaxEndSector;
} MP_SRB_WORK_PARMS, *PMP_SRB_WORK_PARMS;

#include "common.h"

enum ResultType {
  ResultDone,
  ResultQueued
} ;

__declspec(dllexport)                                 // Ensure DriverEntry entry point visible to WinDbg even without a matching .pdb.            
ULONG                                                                                                                                              
DriverEntry(
            IN PVOID,
            IN PUNICODE_STRING 
           );

ULONG
MpHwFindAdapter(
    __in       PHW_HBA_EXT DevExt,
    __in       PVOID HwContext,
    __in       PVOID BusInfo,
    __in       PVOID LowerDevice,
    __in       PCHAR ArgumentString,
    __in __out PPORT_CONFIGURATION_INFORMATION ConfigInfo,
         __out PBOOLEAN Again
);

VOID
MpHwTimer(
    __in PHW_HBA_EXT DevExt
);

BOOLEAN
MpHwInitialize(
    __in PHW_HBA_EXT 
);

void
MpHwReportAdapter(
                  __in PHW_HBA_EXT
                 );

void
MpHwReportLink(
               __in PHW_HBA_EXT
              );

void
MpHwReportLog(__in PHW_HBA_EXT);

VOID
MpHwFreeAdapterResources(
    __in PHW_HBA_EXT
);

BOOLEAN
MpHwStartIo(
            __in PHW_HBA_EXT,
            __in PSCSI_REQUEST_BLOCK
);

BOOLEAN 
MpHwResetBus(
             __in PHW_HBA_EXT,
             __in ULONG       
            );

SCSI_ADAPTER_CONTROL_STATUS
MpHwAdapterControl(
    __in PHW_HBA_EXT DevExt,
    __in SCSI_ADAPTER_CONTROL_TYPE ControlType, 
    __in PVOID Parameters 
);

VOID
ScsiIoControl(
__in PHW_HBA_EXT            pHBAExt,    // Adapter device-object extension from port driver.
__in PSCSI_REQUEST_BLOCK    pSrb,
__in PUCHAR            pResult
);

UCHAR
ScsiExecuteMain(
                __in PHW_HBA_EXT DevExt,
                __in PSCSI_REQUEST_BLOCK,
                __in PUCHAR             
               );

UCHAR
ScsiExecute(
    __in PHW_HBA_EXT DevExt,
    __in PSCSI_REQUEST_BLOCK Srb
    );

UCHAR
ScsiOpInquiry(
    __in PHW_HBA_EXT DevExt,
    __in PHW_LU_EXTENSION LuExt,
    __in PSCSI_REQUEST_BLOCK Srb
    );

UCHAR
ScsiOpReadCapacity(
    IN PHW_HBA_EXT DevExt,
    IN PHW_LU_EXTENSION LuExt,
    IN PSCSI_REQUEST_BLOCK Srb
    );

UCHAR
ScsiOpRead(
    IN PHW_HBA_EXT          DevExt,
    IN PHW_LU_EXTENSION     LuExt,
    IN PSCSI_REQUEST_BLOCK  Srb,
    IN PUCHAR               Action
    );

UCHAR
ScsiOpWrite(
    IN PHW_HBA_EXT          DevExt,
    IN PHW_LU_EXTENSION     LuExt,
    IN PSCSI_REQUEST_BLOCK  Srb,
    IN PUCHAR               Action
    );

UCHAR
ScsiOpModeSense(
    IN PHW_HBA_EXT         DevExt,
    IN PHW_LU_EXTENSION    LuExt,
    IN PSCSI_REQUEST_BLOCK pSrb
    );

UCHAR                                                                                   
ScsiOpReportLuns(                                 
    IN PHW_HBA_EXT          DevExt,                                                     
    IN PHW_LU_EXTENSION     LuExt,                                                      
    IN PSCSI_REQUEST_BLOCK  Srb                                                         
    );                                                                                   

VOID
MpQueryRegParameters(
    IN PUNICODE_STRING,
    IN PVMP_REG_INFO       
    );

UCHAR
MpGetDeviceType(
    __in PHW_HBA_EXT DevExt,
    __in UCHAR PathId,
    __in UCHAR TargetId,
    __in UCHAR Lun,
    __out PVMP_DEVICE_INFO* ppDev
    );

UCHAR MpFindRemovedDevice(
    __in PHW_HBA_EXT,
    __in PSCSI_REQUEST_BLOCK
    );

VOID MpStopAdapter(
    __in PHW_HBA_EXT DevExt
    );

VOID                                                                                                                         
MPTracingInit(                                                                                                            
              __in PVOID,                                                                                  
              __in PVOID
             );

VOID                                                                                                                         
MPTracingCleanup(__in PVOID);

VOID
MpProcServReq(
              __in PHW_HBA_EXT,
              __in PIRP                 
             );
 
NTSTATUS
MpProcessIoCtl(
    __in PHW_HBA_EXT,
    __in PIRP
);

UCHAR
ScsiOpVPD(
    __in PHW_HBA_EXT,
    __in PVMP_DEVICE_INFO,
    __in PSCSI_REQUEST_BLOCK
    );

void
InitializeWmiContext(__in PHW_HBA_EXT);

BOOLEAN
HandleWmiSrb(
    __in       PHW_HBA_EXT,
    __in __out PSCSI_WMI_REQUEST_BLOCK
    );

UCHAR
ScsiReadWriteSetup(
           __in PHW_HBA_EXT          pDevExt,
           __in PHW_LU_EXTENSION     pLUExt,
           __in PSCSI_REQUEST_BLOCK  pSrb,
           __in VMP_REQUEST_ACTION   WkRtnAction,
           __in PUCHAR               pResult
          );

VOID
MpProcServReq(
              __in PHW_HBA_EXT          pDevExt,
              __in PIRP                 pIrp            
             );
VOID
MpGeneralWkRtn(
__in PVOID,
__in PVOID
);

VOID
MpWkRtn(IN PVOID);

BOOLEAN MergeIOs(
    __in    PHW_LU_EXTENSION pLUExt,
    __in    PMP_SRB_WORK_PARMS pWkRtnParms,
    __in    PVMP_REQUEST _req);

BOOLEAN FindMatchLun(
    __in    PHW_HBA_EXT          pDevExt, 
    __in	PCWCHAR              pDiskId,
    __in	PVMP_DEVICE_INFO*    PPFoundDev);

VOID DeleteLunExt(
    __in PHW_HBA_EXT pHBAExt,
    __in PHW_LU_EXTENSION pLuExt);

VOID DeleteDevice(
    __in PHW_HBA_EXT pHBAExt,
    __in PVMP_DEVICE_INFO pDev);

NTSTATUS EnumerateActiveLuns(
    __in PHW_HBA_EXT pDevExt, 
    __in PIRP Irp);

NTSTATUS CreateLun(
    __in PHW_HBA_EXT pDevExt, 
    __in PCONNECT_IN PConnectInfo, 
    __in PKEVENT pkEvent,
    __in PCSTR   szDiskId,
    __in BOOLEAN MergeIOs,
    __out PVMP_DEVICE_INFO* ppDevice);

BOOLEAN FindMatchLunFastMutex(
    __in PHW_HBA_EXT          pHBAExt,
    __in PVMP_DEVICE_INFO     Device,
    __in PHW_LU_EXTENSION*    PPFoundLun);

VOID MPStorTimer(
    _In_ PHW_HBA_EXT pHBAExt
    );
#endif    //   #if !defined(_MP_User_Mode_Only)
#endif    // _MP_H_

