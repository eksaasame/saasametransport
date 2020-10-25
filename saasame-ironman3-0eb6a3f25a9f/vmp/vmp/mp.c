/****************************** Module Header ******************************\
* Module Name:  mp.c
* Project:      CppWDKStorPortVirtualMiniport

* Copyright (c) Microsoft Corporation.
* 
* a.       DriverEntry()
* Gets some resources, call StorPortInitialize().
*
* b.      MpHwFindAdapter()
* Gets more resources, sets configuration parameters.
*
* c.       MpHwStartIo()
* Entry point for an I/O. This calls the appropriate support routine, e.g., ScsiExecuteMain().
*
* 
* This source is subject to the Microsoft Public License.
* See http://www.microsoft.com/opensource/licenses.mspx#Ms-PL.
* All other rights reserved.
* 
* THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, 
* EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED 
* WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
\***************************************************************************/   

#define MPDriverVer     "5.022"

#include "mp.h"

#pragma warning(push)
#pragma warning(disable : 4204)                       /* Prevent C4204 messages from stortrce.h. */
#include <stortrce.h>
#pragma warning(pop)

#include <wdf.h>
#include "trace.h"
#include "mp.tmh"
#include "hbapiwmi.h"
#include <initguid.h>

#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, DriverEntry )
#endif // ALLOC_PRAGMA

/**************************************************************************************************/ 
/*                                                                                                */ 
/* Globals.                                                                                       */ 
/*                                                                                                */ 
/**************************************************************************************************/ 

#define USE_STORPORT_GET_SYSTEM_ADDRESS 1
#ifdef MP_DrvInfo_Inline

VMP_DRIVER_INFO  lclDriverInfo;

#endif

PVMP_DRIVER_INFO pMPDrvInfoGlobal = NULL;
ULONG AssignedScsiIds[(((SCSI_MAXIMUM_LUNS_PER_TARGET + 1) / 8) / sizeof(ULONG))*MAX_TARGETS];
extern RTL_BITMAP      ScsiBitMapHeader = { 0 };
static ULONG PathId = 0;

/**************************************************************************************************/ 
/*                                                                                                */ 
/*                                                                                                */ 
/**************************************************************************************************/ 
__declspec(dllexport)                                 // Ensure DriverEntry entry point visible to WinDbg even without a matching .pdb.            
ULONG                                                                                                                                              
DriverEntry(
            __in PVOID           pDrvObj,
            __in PUNICODE_STRING pRegistryPath
           )
{
    NTSTATUS                       status = STATUS_SUCCESS;
    VIRTUAL_HW_INITIALIZATION_DATA hwInitData;
    PVMP_DRIVER_INFO                  pMPDrvInfo;

#ifdef MP_DrvInfo_Inline

    // Because there's no good way to clean up the allocation of the global driver information, 
    // the global information is kept in an inline structure.

    pMPDrvInfo = &lclDriverInfo;

#else

    //
    // Allocate basic miniport driver object (shared across instances of miniport). The pointer is kept in the driver binary's static storage.
    //
    // Because there's no good way to clean up the allocation of the global driver information, 
    // the global information will be leaked.  This is deemed acceptable since it's not expected
    // that DriverEntry will be invoked often in the life of a Windows boot.
    //

    pMPDrvInfo = ExAllocatePoolWithTag(NonPagedPool, sizeof(VMP_DRIVER_INFO), MP_TAG_GENERAL);

    if (!pMPDrvInfo) {                                // No good?
        status = STATUS_INSUFFICIENT_RESOURCES;

        goto Done;
    }

#endif

    pMPDrvInfoGlobal = pMPDrvInfo;                    // Save pointer in binary's storage.

    RtlZeroMemory(pMPDrvInfo, sizeof(VMP_DRIVER_INFO));  // Set pMPDrvInfo's storage to a known state.

    pMPDrvInfo->pDriverObj = pDrvObj;                 // Save pointer to driver object.

    lock_init(&pMPDrvInfo->Lock);   // Initialize spin lock.

    InitializeListHead(&pMPDrvInfo->ListMPHBAObj);    // Initialize list head.

    // Get registry parameters.

    MpQueryRegParameters(pRegistryPath, &pMPDrvInfo->MPRegInfo);

    // Set up information for StorPortInitialize().

    RtlZeroMemory(&hwInitData, sizeof(VIRTUAL_HW_INITIALIZATION_DATA));

    hwInitData.HwInitializationDataSize = sizeof(VIRTUAL_HW_INITIALIZATION_DATA);

    hwInitData.HwInitialize             = MpHwInitialize;       // Required.
    hwInitData.HwStartIo                = MpHwStartIo;          // Required.
    hwInitData.HwFindAdapter            = MpHwFindAdapter;      // Required.
    hwInitData.HwResetBus               = MpHwResetBus;         // Required.
    hwInitData.HwAdapterControl         = MpHwAdapterControl;   // Required.
    hwInitData.HwFreeAdapterResources   = MpHwFreeAdapterResources;
    hwInitData.HwInitializeTracing      = MPTracingInit;
    hwInitData.HwCleanupTracing         = MPTracingCleanup;
    hwInitData.HwProcessServiceRequest  = MpProcServReq;

    hwInitData.AdapterInterfaceType     = Internal;

    hwInitData.DeviceExtensionSize      = sizeof(HW_HBA_EXT);
    hwInitData.SpecificLuExtensionSize  = sizeof(HW_LU_EXTENSION);
    hwInitData.SrbExtensionSize = sizeof(HW_SRB_EXTENSION);

    status =  StorPortInitialize(                     // Tell StorPort we're here.
                                 pDrvObj,
                                 pRegistryPath,
                                 (PHW_INITIALIZATION_DATA)&hwInitData,     // Note: Have to override type!
                                 NULL
                                );

    if (STATUS_SUCCESS!=status) {                     // Port driver said not OK?                                        
      goto Done;
    }                                                 // End 'port driver said not OK'?

    RtlZeroMemory(AssignedScsiIds, sizeof(AssignedScsiIds));
    RtlInitializeBitMap(&ScsiBitMapHeader, AssignedScsiIds, MAX_LUNS*MAX_TARGETS);

Done:    
    if (STATUS_SUCCESS!=status) {                     // A problem?
    
#ifndef MP_DrvInfo_Inline

      if (NULL!=pMPDrvInfo) {
        ExFreePoolWithTag(pMPDrvInfo, MP_TAG_GENERAL);
      }

#endif

    }
    
    return status;
}                                                     // End DriverEntry().

/**************************************************************************************************/ 
/*                                                                                                */ 
/* Callback for a new HBA.                                                                        */ 
/*                                                                                                */ 
/**************************************************************************************************/ 
ULONG                                                 
MpHwFindAdapter(
                __in       PHW_HBA_EXT                     pHBAExt,           // Adapter device-object extension from StorPort.
                __in       PVOID                           pHwContext,        // Pointer to context.
                __in       PVOID                           pBusInformation,   // Miniport's FDO.
                __in       PVOID                           pLowerDO,          // Device object beneath FDO.
                __in       PCHAR                           pArgumentString,
                __in __out PPORT_CONFIGURATION_INFORMATION pConfigInfo,
                __in       PBOOLEAN                        pBAgain            
               )
{
    ULONG              i,
                       len,
                       status = SP_RETURN_FOUND;
    PCHAR              pChar;

    UNREFERENCED_PARAMETER(pHwContext);
    UNREFERENCED_PARAMETER(pBusInformation);
    UNREFERENCED_PARAMETER(pLowerDO);
    UNREFERENCED_PARAMETER(pArgumentString);

    DoStorageTraceEtw(DbgLvlLoud, MpDemoDebugInfo,
                      "MpHwFindAdapter:  pHBAExt = 0x%p, pConfigInfo = 0x%p\n", pHBAExt, pConfigInfo);

    pHBAExt->pMPDrvObj = pMPDrvInfoGlobal;            // Copy master object from static variable.
    pHBAExt->MaxTransferBlocks = pMPDrvInfoGlobal->MPRegInfo.MaxTransferLengthInMB << 11;
    InitializeListHead(&pHBAExt->LUList);
    InitializeListHead(&pHBAExt->DeviceList);
    lock_init(&pHBAExt->DeviceListLock);
    KeInitializeGuardedMutex(&pHBAExt->LUListLock);

    pHBAExt->HostTargetId = (UCHAR)pHBAExt->pMPDrvObj->MPRegInfo.InitiatorID;
    pHBAExt->pDrvObj = pHBAExt->pMPDrvObj->pDriverObj;
          
    pConfigInfo->VirtualDevice                  = TRUE;                        // Inidicate no real hardware.
    pConfigInfo->WmiDataProvider                = TRUE;                        // Indicate WMI provider.
    pConfigInfo->MaximumTransferLength          = pHBAExt->MaxTransferBlocks << 9;  // SP_UNINITIALIZED_VALUE;      // Indicate unlimited.
    pConfigInfo->AlignmentMask                  = FILE_LONG_ALIGNMENT;         // Indicate DWORD alignment.
    pConfigInfo->CachesData                     = FALSE;                       // Indicate miniport wants flush and shutdown notification.
    pConfigInfo->MaximumNumberOfTargets         = MAX_TARGETS;                 // Indicate maximum targets.
    pConfigInfo->NumberOfBuses                  = 1;                           // Indicate number of busses.
    pConfigInfo->SynchronizationModel           = StorSynchronizeFullDuplex;   // Indicate full-duplex.
    pConfigInfo->ScatterGather                  = TRUE;                        // Indicate scatter-gather (explicit setting needed for Win2003 at least).
#if (NTDDI_VERSION >= NTDDI_WIN8)
    pConfigInfo->MaxNumberOfIO                  = 1000;
    pConfigInfo->MaxIOsPerLun = pMPDrvInfoGlobal->MPRegInfo.MaxIOsPerLun;
#endif
    // Save Vendor Id, Product Id, Revision in device extension.

    pChar = (PCHAR)pHBAExt->pMPDrvObj->MPRegInfo.VendorId.Buffer;
    len = min(8, (pHBAExt->pMPDrvObj->MPRegInfo.VendorId.Length/2));
    for ( i = 0; i < len; i++, pChar+=2)
      pHBAExt->VendorId[i] = *pChar;

    pChar = (PCHAR)pHBAExt->pMPDrvObj->MPRegInfo.ProductId.Buffer;
    len = min(16, (pHBAExt->pMPDrvObj->MPRegInfo.ProductId.Length/2));
    for ( i = 0; i < len; i++, pChar+=2)
      pHBAExt->ProductId[i] = *pChar;

    pChar = (PCHAR)pHBAExt->pMPDrvObj->MPRegInfo.ProductRevision.Buffer;
    len = min(4, (pHBAExt->pMPDrvObj->MPRegInfo.ProductRevision.Length/2));
    for ( i = 0; i < len; i++, pChar+=2)
      pHBAExt->ProductRevision[i] = *pChar;

    // Add HBA extension to master driver object's linked list.

    lock_acquire(&pHBAExt->pMPDrvObj->Lock);

    InsertTailList(&pHBAExt->pMPDrvObj->ListMPHBAObj, &pHBAExt->Bind);

    InterlockedIncrement(&(LONG)pHBAExt->pMPDrvObj->DrvInfoNbrMPHBAObj);

    lock_release(&pHBAExt->pMPDrvObj->Lock);
    KeInitializeGuardedMutex(&pHBAExt->pMPDrvObj->ConnectionMutex);

    InitializeWmiContext(pHBAExt);

    *pBAgain = FALSE;    
    // request a timer call to the TimerReinit routine in 1000 msec
    StorPortNotification(RequestTimerCall, pHBAExt,
        MPStorTimer, (ULONG)1000000 * 30);

    return status;
}                                                     // End MpHwFindAdapter().

/**************************************************************************************************/ 
/*                                                                                                */ 
/**************************************************************************************************/ 
BOOLEAN
MpHwInitialize(__in PHW_HBA_EXT pHBAExt)
{
    UNREFERENCED_PARAMETER(pHBAExt);

    return TRUE;
}                                                     // End MpHwInitialize().

#define StorPortMaxWMIEventSize 0x80                  // Maximum WMIEvent size StorPort will support.
#define InstName L"vHBA"

/**************************************************************************************************/ 
/*                                                                                                */ 
/**************************************************************************************************/ 
void
MpHwReportAdapter(__in PHW_HBA_EXT pHBAExt)
{
    NTSTATUS               status;
    PWNODE_SINGLE_INSTANCE pWnode;
    ULONG                  WnodeSize,
                           WnodeSizeInstanceName,
                           WnodeSizeDataBlock,
                           length,
                           size;
    GUID                   lclGuid = MSFC_AdapterEvent_GUID;
    UNICODE_STRING         lclInstanceName;
    UCHAR                  myPortWWN[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    PMSFC_AdapterEvent     pAdapterArr;

    // With the instance name used here and with the rounding-up to 4-byte alignment of the data portion used here,
    // 0x34 (52) bytes are available for the actual data of the WMI event.  (The 0x34 bytes result from the fact that
    // StorPort at present (August 2008) allows 0x80 bytes for the entire WMIEvent (header, instance name and data);
    // the header is 0x40 bytes; the instance name used here results in 0xA bytes, and the rounding up consumes 2 bytes;
    // in other words, 0x80 - (0x40 + 0x0A + 0x02)).

    RtlInitUnicodeString(&lclInstanceName, InstName); // Set Unicode descriptor for instance name.

    // A WMIEvent structure consists of header, instance name and data block.

    WnodeSize             = sizeof(WNODE_SINGLE_INSTANCE);

    // Because the first field in the data block, EventType, is a ULONG, ensure that the data block begins on a
    // 4-byte boundary (as will be calculated in DataBlockOffset).

    WnodeSizeInstanceName = sizeof(USHORT) +          // Size of USHORT at beginning plus
                            lclInstanceName.Length;   //   size of instance name.
    WnodeSizeInstanceName =                           // Round length up to multiple of 4 (if needed).
      (ULONG)WDF_ALIGN_SIZE_UP(WnodeSizeInstanceName, sizeof(ULONG));

    WnodeSizeDataBlock    = MSFC_AdapterEvent_SIZE;   // Size of data block.

    size = WnodeSize             +                    // Size of WMIEvent.         
           WnodeSizeInstanceName + 
           WnodeSizeDataBlock;

    pWnode = ExAllocatePoolWithTag(NonPagedPool, size, MP_TAG_GENERAL);

    if (NULL!=pWnode) {                               // Good?
        RtlZeroMemory(pWnode, size);
        
        // Fill out most of header. StorPort will set the ProviderId and TimeStamp in the header.

        pWnode->WnodeHeader.BufferSize = size;
        pWnode->WnodeHeader.Version    = 1;
        RtlCopyMemory(&pWnode->WnodeHeader.Guid, &lclGuid, sizeof(lclGuid));  
        pWnode->WnodeHeader.Flags      = WNODE_FLAG_EVENT_ITEM |
                                         WNODE_FLAG_SINGLE_INSTANCE;

        // Say where to find instance name and the data block and what is the data block's size.

        pWnode->OffsetInstanceName     = WnodeSize;
        pWnode->DataBlockOffset        = WnodeSize + WnodeSizeInstanceName;
        pWnode->SizeDataBlock          = WnodeSizeDataBlock;

        // Copy the instance name.
                   
        size -= WnodeSize;                            // Length remaining and available.
        status = WDF_WMI_BUFFER_APPEND_STRING(        // Copy WCHAR string, preceded by its size.
            WDF_PTR_ADD_OFFSET(pWnode, pWnode->OffsetInstanceName),
            size,                                     // Length available for copying.
            &lclInstanceName,                         // Unicode string whose WCHAR buffer is to be copied.
            &length                                   // Variable to receive size needed.
            );

        if (STATUS_SUCCESS!=status) {                 // A problem?
            ASSERT(FALSE);
        }

        pAdapterArr =                                 // Point to data block.
          WDF_PTR_ADD_OFFSET(pWnode, pWnode->DataBlockOffset);

        // Copy event code and WWN.

        pAdapterArr->EventType = HBA_EVENT_ADAPTER_ADD;

        RtlCopyMemory(pAdapterArr->PortWWN, myPortWWN, sizeof(myPortWWN));

        // Ask StorPort to announce the event.

        StorPortNotification(WMIEvent, 
                             pHBAExt, 
                             pWnode, 
                             0xFF);                   // Notification pertains to an HBA.

        ExFreePoolWithTag(pWnode, MP_TAG_GENERAL);
    }
    else {
    }
}                                                     // End MpHwReportAdapter().

/**************************************************************************************************/ 
/*                                                                                                */ 
/**************************************************************************************************/ 
void
MpHwReportLink(__in PHW_HBA_EXT pHBAExt)
{
    NTSTATUS               status;
    PWNODE_SINGLE_INSTANCE pWnode;
    PMSFC_LinkEvent        pLinkEvent;
    ULONG                  WnodeSize,
                           WnodeSizeInstanceName,
                           WnodeSizeDataBlock,
                           length,
                           size;
    GUID                   lclGuid = MSFC_LinkEvent_GUID;
    UNICODE_STRING         lclInstanceName;
             
    #define RLIRBufferArraySize 0x10                  // Define 16 entries in MSFC_LinkEvent.RLIRBuffer[].
             
    UCHAR                  myAdapterWWN[8] = {1, 2, 3, 4, 5, 6, 7, 8},
                           myRLIRBuffer[RLIRBufferArraySize] = {10, 11, 12, 13, 14, 15, 16, 17, 20, 21, 22, 23, 24, 25, 26, 0xFF};

    RtlInitUnicodeString(&lclInstanceName, InstName); // Set Unicode descriptor for instance name.

    WnodeSize             = sizeof(WNODE_SINGLE_INSTANCE);
    WnodeSizeInstanceName = sizeof(USHORT) +          // Size of USHORT at beginning plus
                            lclInstanceName.Length;   //   size of instance name.
    WnodeSizeInstanceName =                           // Round length up to multiple of 4 (if needed).
      (ULONG)WDF_ALIGN_SIZE_UP(WnodeSizeInstanceName, sizeof(ULONG));
    WnodeSizeDataBlock    =                           // Size of data.
                            FIELD_OFFSET(MSFC_LinkEvent, RLIRBuffer) +
                            sizeof(myRLIRBuffer);

    size = WnodeSize             +                    // Size of WMIEvent.         
           WnodeSizeInstanceName + 
           WnodeSizeDataBlock;

    pWnode = ExAllocatePoolWithTag(NonPagedPool, size, MP_TAG_GENERAL);

    if (NULL!=pWnode) {                               // Good?
        RtlZeroMemory(pWnode, size);
        
        // Fill out most of header. StorPort will set the ProviderId and TimeStamp in the header.

        pWnode->WnodeHeader.BufferSize = size;
        pWnode->WnodeHeader.Version    = 1;
        RtlCopyMemory(&pWnode->WnodeHeader.Guid, &lclGuid, sizeof(lclGuid));  
        pWnode->WnodeHeader.Flags      = WNODE_FLAG_EVENT_ITEM |
                                         WNODE_FLAG_SINGLE_INSTANCE;

        // Say where to find instance name and the data block and what is the data block's size.

        pWnode->OffsetInstanceName     = WnodeSize;
        pWnode->DataBlockOffset        = WnodeSize + WnodeSizeInstanceName;
        pWnode->SizeDataBlock          = WnodeSizeDataBlock;

        // Copy the instance name.
                   
        size -= WnodeSize;                            // Length remaining and available.
        status = WDF_WMI_BUFFER_APPEND_STRING(        // Copy WCHAR string, preceded by its size.
            WDF_PTR_ADD_OFFSET(pWnode, pWnode->OffsetInstanceName),
            size,                                     // Length available for copying.
            &lclInstanceName,                         // Unicode string whose WCHAR buffer is to be copied.
            &length                                   // Variable to receive size needed.
            );

        if (STATUS_SUCCESS!=status) {                 // A problem?
            ASSERT(FALSE);
        }

        pLinkEvent =                                  // Point to data block.
          WDF_PTR_ADD_OFFSET(pWnode, pWnode->DataBlockOffset);

        // Copy event code, WWN, buffer size and buffer contents.

        pLinkEvent->EventType = HBA_EVENT_LINK_INCIDENT;

        RtlCopyMemory(pLinkEvent->AdapterWWN, myAdapterWWN, sizeof(myAdapterWWN));

        pLinkEvent->RLIRBufferSize = sizeof(myRLIRBuffer);

        RtlCopyMemory(pLinkEvent->RLIRBuffer, myRLIRBuffer, sizeof(myRLIRBuffer));

        StorPortNotification(WMIEvent, 
                             pHBAExt, 
                             pWnode, 
                             0xFF);                   // Notification pertains to an HBA.

        ExFreePoolWithTag(pWnode, MP_TAG_GENERAL);
    }
    else {
    }
}                                                     // End MpHwReportLink().

/**************************************************************************************************/ 
/*                                                                                                */ 
/**************************************************************************************************/ 
void
MpHwReportLog(__in PHW_HBA_EXT pHBAExt)
{
    NTSTATUS               status;
    PWNODE_SINGLE_INSTANCE pWnode;
    ULONG                  WnodeSize,
                           WnodeSizeInstanceName,
                           WnodeSizeDataBlock,
                           length,
                           size;
    UNICODE_STRING         lclInstanceName;
    PIO_ERROR_LOG_PACKET   pLogError;

    RtlInitUnicodeString(&lclInstanceName, InstName); // Set Unicode descriptor for instance name.

    WnodeSize             = sizeof(WNODE_SINGLE_INSTANCE);
    WnodeSizeInstanceName = sizeof(USHORT) +          // Size of USHORT at beginning plus
                            lclInstanceName.Length;   //   size of instance name.
    WnodeSizeInstanceName =                           // Round length up to multiple of 4 (if needed).
      (ULONG)WDF_ALIGN_SIZE_UP(WnodeSizeInstanceName, sizeof(ULONG));
    WnodeSizeDataBlock    = sizeof(IO_ERROR_LOG_PACKET);       // Size of data.

    size = WnodeSize             +                    // Size of WMIEvent.         
           WnodeSizeInstanceName + 
           WnodeSizeDataBlock;

    pWnode = ExAllocatePoolWithTag(NonPagedPool, size, MP_TAG_GENERAL);

    if (NULL!=pWnode) {                               // Good?
        RtlZeroMemory(pWnode, size);
        
        // Fill out most of header. StorPort will set the ProviderId and TimeStamp in the header.

        pWnode->WnodeHeader.BufferSize = size;
        pWnode->WnodeHeader.Version    = 1;
        pWnode->WnodeHeader.Flags      = WNODE_FLAG_EVENT_ITEM |
                                         WNODE_FLAG_LOG_WNODE;

        pWnode->WnodeHeader.HistoricalContext = 9;

        // Say where to find instance name and the data block and what is the data block's size.

        pWnode->OffsetInstanceName     = WnodeSize;
        pWnode->DataBlockOffset        = WnodeSize + WnodeSizeInstanceName;
        pWnode->SizeDataBlock          = WnodeSizeDataBlock;

        // Copy the instance name.
                   
        size -= WnodeSize;                            // Length remaining and available.
        status = WDF_WMI_BUFFER_APPEND_STRING(        // Copy WCHAR string, preceded by its size.
            WDF_PTR_ADD_OFFSET(pWnode, pWnode->OffsetInstanceName),
            size,                                     // Length available for copying.
            &lclInstanceName,                         // Unicode string whose WCHAR buffer is to be copied.
            &length                                   // Variable to receive size needed.
            );

        if (STATUS_SUCCESS!=status) {                 // A problem?
            ASSERT(FALSE);
        }

        pLogError =                                    // Point to data block.
          WDF_PTR_ADD_OFFSET(pWnode, pWnode->DataBlockOffset);

        pLogError->UniqueErrorValue = 0x40;
        pLogError->FinalStatus = 0x41;
        pLogError->ErrorCode = 0x42;

        StorPortNotification(WMIEvent, 
                             pHBAExt, 
                             pWnode, 
                             0xFF);                   // Notification pertains to an HBA.

        ExFreePoolWithTag(pWnode, MP_TAG_GENERAL);
    }
    else {
    }
}                                                     // End MpHwReportLog().

/**************************************************************************************************/ 
/*                                                                                                */ 
/**************************************************************************************************/ 
BOOLEAN
MpHwResetBus(
             __in PHW_HBA_EXT          pHBAExt,       // Adapter device-object extension from StorPort.
             __in ULONG                BusId
            )
{
    UNREFERENCED_PARAMETER(pHBAExt);
    UNREFERENCED_PARAMETER(BusId);

    // To do: At some future point, it may be worthwhile to ensure that any SRBs being handled be completed at once.
    //        Practically speaking, however, it seems that the only SRBs that would not be completed very quickly
    //        would be those handled by the worker thread. In the future, therefore, there might be a global flag
    //        set here to instruct the thread to complete outstanding I/Os as they appear; but a period for that
    //        happening would have to be devised (such completion shouldn't be unbounded).

    return TRUE;
}                                                     // End MpHwResetBus().

/**************************************************************************************************/ 
/*                                                                                                */ 
/**************************************************************************************************/ 
NTSTATUS                                              
MpHandleRemoveDevice(
                     __in PHW_HBA_EXT             pHBAExt,// Adapter device-object extension from StorPort.
                     __in PSCSI_PNP_REQUEST_BLOCK pSrb
                    )
{
    UNREFERENCED_PARAMETER(pHBAExt);

    pSrb->SrbStatus = SRB_STATUS_BAD_FUNCTION;

    return STATUS_UNSUCCESSFUL;
}                                                     // End MpHandleRemoveDevice().

/**************************************************************************************************/ 
/*                                                                                                */ 
/**************************************************************************************************/ 
NTSTATUS                                           
MpHandleQueryCapabilities(
                          __in PHW_HBA_EXT             pHBAExt,// Adapter device-object extension from StorPort.
                          __in PSCSI_PNP_REQUEST_BLOCK pSrb
                         )
{
    NTSTATUS                  status = STATUS_SUCCESS;
    PSTOR_DEVICE_CAPABILITIES pStorageCapabilities = (PSTOR_DEVICE_CAPABILITIES)pSrb->DataBuffer;

    UNREFERENCED_PARAMETER(pHBAExt);

    RtlZeroMemory(pStorageCapabilities, pSrb->DataTransferLength);

    pStorageCapabilities->Removable = TRUE;
    pStorageCapabilities->SurpriseRemovalOK = FALSE;

    pSrb->SrbStatus = SRB_STATUS_SUCCESS;

    return status;
}                                                     // End MpHandleQueryCapabilities().

/**************************************************************************************************/ 
/*                                                                                                */ 
/**************************************************************************************************/ 
NTSTATUS                                              
MpHwHandlePnP(
              __in PHW_HBA_EXT              pHBAExt,  // Adapter device-object extension from StorPort.
              __in PSCSI_PNP_REQUEST_BLOCK  pSrb
             )
{
    NTSTATUS status = STATUS_SUCCESS;

    switch(pSrb->PnPAction) {

      case StorRemoveDevice:
        status = MpHandleRemoveDevice(pHBAExt, pSrb);

        break;

      case StorQueryCapabilities:
        status = MpHandleQueryCapabilities(pHBAExt, pSrb);

        break;

      default:
        pSrb->SrbStatus = SRB_STATUS_SUCCESS;         // Do nothing.
    }

    if (STATUS_SUCCESS!=status) {
    }

    return status;
}                                                     // End MpHwHandlePnP().

/**************************************************************************************************/ 
/*                                                                                                */ 
/**************************************************************************************************/ 
BOOLEAN
MpHwStartIo(
            __in       PHW_HBA_EXT          pHBAExt,  // Adapter device-object extension from StorPort.
            __in __out PSCSI_REQUEST_BLOCK  pSrb
           )
{
    UCHAR                     srbStatus = SRB_STATUS_INVALID_REQUEST;
    BOOLEAN                   bFlag;
    NTSTATUS                  status;
    UCHAR                     Result = ResultDone;

    //DoStorageTraceEtw(DbgLvlLoud, MpDemoDebug04,
    //                  "MpHwStartIo:  SCSI Request Block = %!SRB!\n",
    //                  pSrb);

    _InterlockedExchangeAdd((volatile LONG *)&pHBAExt->SRBsSeen, 1);   // Bump count of SRBs encountered.

    // Next, if true, will cause StorPort to remove the associated LUNs if, for example, devmgmt.msc is asked "scan for hardware changes."

    switch (pSrb->Function) {
        
    case SRB_FUNCTION_IO_CONTROL:
            ScsiIoControl(pHBAExt, pSrb, &Result);
            break;

        case SRB_FUNCTION_EXECUTE_SCSI:
            srbStatus = ScsiExecuteMain(pHBAExt, pSrb, &Result);
            break;

        case SRB_FUNCTION_WMI:
            _InterlockedExchangeAdd((volatile LONG *)&pHBAExt->WMISRBsSeen, 1);
            bFlag = HandleWmiSrb(pHBAExt, (PSCSI_WMI_REQUEST_BLOCK)pSrb);
            srbStatus = TRUE==bFlag ? SRB_STATUS_SUCCESS : SRB_STATUS_INVALID_REQUEST;
            break;
            
        case SRB_FUNCTION_RESET_LOGICAL_UNIT:
            StorPortCompleteRequest(
                                    pHBAExt,
                                    pSrb->PathId,
                                    pSrb->TargetId,
                                    pSrb->Lun,
                                    SRB_STATUS_BUSY
                                   );
            srbStatus = SRB_STATUS_SUCCESS;
            break;
            
        case SRB_FUNCTION_RESET_DEVICE:
            StorPortCompleteRequest(
                                    pHBAExt,
                                    pSrb->PathId,
                                    pSrb->TargetId,
                                    SP_UNTAGGED,
                                    SRB_STATUS_TIMEOUT
                                   );
            srbStatus = SRB_STATUS_SUCCESS;
            break;
            
        case SRB_FUNCTION_PNP:                        
            status = MpHwHandlePnP(pHBAExt, (PSCSI_PNP_REQUEST_BLOCK)pSrb);
            srbStatus = pSrb->SrbStatus;
            
            break;

        case SRB_FUNCTION_POWER:                      
            // Do nothing.
            srbStatus = SRB_STATUS_SUCCESS;

            break;

        case SRB_FUNCTION_SHUTDOWN:                   
            // Do nothing.
            srbStatus = SRB_STATUS_SUCCESS;

            break;

        default:
            DoStorageTraceEtw(DbgLvlErr, MpDemoDebugInfo, "MpHwStartIo: Unknown Srb Function = 0x%x\n", pSrb->Function);
            srbStatus = SRB_STATUS_INVALID_REQUEST;
            break;

    } // switch (pSrb->Function)

    if (ResultDone==Result) {                         // Complete now?
      pSrb->SrbStatus = srbStatus;

      // Note:  A miniport with real hardware would not always be calling RequestComplete from HwStorStartIo.  Rather,
      //        the miniport would typically be doing real I/O and would call RequestComplete only at the end of that
      //        real I/O, in its HwStorInterrupt or in a DPC routine.

        __try {
            StorPortNotification(RequestComplete, pHBAExt, pSrb);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            NTSTATUS Status = GetExceptionCode();
            DoStorageTraceEtw(DbgLvlErr, MpDemoDebugInfo,
                __FUNCTION__": Exception Code = 0x%x. \n",
                Status);
        }
    }
     
    DoStorageTraceEtw(DbgLvlInfo, MpDemoDebugInfo, "MpHwStartIo - OUT\n");

    return TRUE;
}                                                     // End MpHwStartIo().

/**************************************************************************************************/ 
/*                                                                                                */ 
/**************************************************************************************************/ 
SCSI_ADAPTER_CONTROL_STATUS
MpHwAdapterControl(
                   __in PHW_HBA_EXT               pHBAExt, // Adapter device-object extension from StorPort.
                   __in SCSI_ADAPTER_CONTROL_TYPE ControlType,
                   __in PVOID                     pParameters
                  )
{
    PSCSI_SUPPORTED_CONTROL_TYPE_LIST pCtlTypList;
    ULONG                             i;

    DoStorageTraceEtw(DbgLvlLoud, MpDemoDebugInfo,
                      "MpHwAdapterControl:  ControlType = %d\n", ControlType);

    pHBAExt->AdapterState = ControlType;

    switch (ControlType) {
        case ScsiQuerySupportedControlTypes:
            DoStorageTraceEtw(DbgLvlLoud, MpDemoDebugInfo, "MpHwAdapterControl: ScsiQuerySupportedControlTypes\n");

            // Ggt pointer to control type list
            pCtlTypList = (PSCSI_SUPPORTED_CONTROL_TYPE_LIST)pParameters;

            // Cycle through list to set TRUE for each type supported
            // making sure not to go past the MaxControlType
            for (i = 0; i < pCtlTypList->MaxControlType; i++)
                if ( i == ScsiQuerySupportedControlTypes ||
                     i == ScsiStopAdapter   || i == ScsiRestartAdapter ||
                     i == ScsiSetBootConfig || i == ScsiSetRunningConfig )
                {
                    pCtlTypList->SupportedTypeList[i] = TRUE;
                }
            break;

        case ScsiStopAdapter:
            DoStorageTraceEtw(DbgLvlLoud, MpDemoDebugInfo, "MpHwAdapterControl:  ScsiStopAdapter\n");

            // Free memory allocated for disk
            MpStopAdapter(pHBAExt);

            break;

        case ScsiRestartAdapter:
            DoStorageTraceEtw(DbgLvlInfo, MpDemoDebugInfo, "MpHwAdapterControl:  ScsiRestartAdapter\n");

            /* To Do: Add some function. */

            break;

        case ScsiSetBootConfig:
            DoStorageTraceEtw(DbgLvlInfo, MpDemoDebugInfo, "MpHwAdapterControl:  ScsiSetBootConfig\n");

            break;
            
        case ScsiSetRunningConfig:
            DoStorageTraceEtw(DbgLvlInfo, MpDemoDebugInfo, "MpHwAdapterControl:  ScsiSetRunningConfig\n");

            break;

        default:
            DoStorageTraceEtw(DbgLvlInfo, MpDemoDebugInfo, "MpHwAdapterControl:  UNKNOWN\n");

            break;
    } 

    DoStorageTraceEtw(DbgLvlInfo, MpDemoDebugInfo, "MpHwAdapterControl - OUT\n");

    return ScsiAdapterControlSuccess;
}                                                     // End MpHwAdapterControl().

/**************************************************************************************************/ 
/*                                                                                                */ 
/**************************************************************************************************/ 
VOID
MpStopAdapter(__in PHW_HBA_EXT pHBAExt)               // Adapter device-object extension from StorPort.
{
    PHW_LU_EXTENSION      pLUExt;
    PLIST_ENTRY           pNextEntry;

    LIST_FOR_EACH(pNextEntry, &pHBAExt->LUList) {	// Go through linked list of LUN extensions for this HBA.
        pLUExt = CONTAINING_RECORD(pNextEntry, HW_LU_EXTENSION, List);
        pLUExt->bIsMissing = TRUE;
        //__free(pLUExt->pDiskBuf);
    }
//done:
    return;
}                                                     // End MpStopAdapter().

/**************************************************************************************************/ 
/*                                                                                                */ 
/* Return device type for specified device on specified HBA extension.                            */ 
/*                                                                                                */ 
/**************************************************************************************************/ 
UCHAR 
MpGetDeviceType(
                __in PHW_HBA_EXT          pHBAExt,    // Adapter device-object extension from StorPort.
                __in UCHAR                PathId,
                __in UCHAR                TargetId,
                __in UCHAR                Lun,
                __out			          PVMP_DEVICE_INFO* ppDev
               )
{
    UCHAR           type = DEVICE_NOT_FOUND;
    *ppDev = NULL;
    lock_acquire(&pHBAExt->DeviceListLock);
    LIST_ENTRY* entry;
    LIST_FOR_EACH(entry, &pHBAExt->DeviceList){
        PVMP_DEVICE_INFO device = CONTAINING_RECORD(entry, VMP_DEVICE_INFO, Bind);
        if (PathId == device->PathId &&
            TargetId == device->TargetId &&
            Lun == device->Lun) {
            type = device->DeviceType;
            if (ppDev){
                *ppDev = device;
            }
            break;
        }
    }
    lock_release(&pHBAExt->DeviceListLock);
    return type;
}                                                     // End MpGetDeviceType().

/**************************************************************************************************/                         
/*                                                                                                */                         
/* MPTracingInit.                                                                                 */                         
/*                                                                                                */                         
/**************************************************************************************************/                         
VOID                                                                                                                         
MPTracingInit(                                                                                                            
              __in PVOID pArg1,                                                                                  
              __in PVOID pArg2
             )                                                                                                            
{                                                                                                                            
    WPP_INIT_TRACING(pArg1, pArg2);
}                                                     // End MPTracingInit().

/**************************************************************************************************/                         
/*                                                                                                */                         
/* MPTracingCleanUp.                                                                              */                         
/*                                                                                                */                         
/* This is called when the driver is being unloaded.                                              */                         
/*                                                                                                */                         
/**************************************************************************************************/                         
VOID                                                                                                                         
MPTracingCleanup(__in PVOID pArg1)                                                                                                            
{                                                                                                                            
    DoStorageTraceEtw(DbgLvlLoud, MpDemoDebugInfo, "MPTracingCleanUp entered\n");                                                                       

    WPP_CLEANUP(pArg1);
}                                                     // End MPTracingCleanup().

/**************************************************************************************************/                         
/*                                                                                                */                         
/* MpHwFreeAdapterResources.                                                                      */                         
/*                                                                                                */                         
/**************************************************************************************************/                         
VOID
MpHwFreeAdapterResources(__in PHW_HBA_EXT pHBAExt)
{
    PLIST_ENTRY           pNextEntry; 
    PHW_HBA_EXT           pLclHBAExt;
    LIST_ENTRY	          RemoveList;
    LIST_ENTRY*           entry;
    LIST_ENTRY*           request;
    InitializeListHead(&RemoveList);
    DoStorageTraceEtw(DbgLvlLoud, MpDemoDebugInfo, "MpHwFreeAdapterResources entered, pHBAExt = 0x%p\n", pHBAExt);
    if (pHBAExt->pMPDrvObj){
        lock_acquire(&pHBAExt->pMPDrvObj->Lock);
        for (                                             // Go through linked list of HBA extensions.
            pNextEntry = pHBAExt->pMPDrvObj->ListMPHBAObj.Flink;
            pNextEntry != &pHBAExt->pMPDrvObj->ListMPHBAObj;
        pNextEntry = pNextEntry->Flink
            ) {
            pLclHBAExt = CONTAINING_RECORD(pNextEntry, HW_HBA_EXT, Bind);

            if (pLclHBAExt == pHBAExt) { // Is this entry the same as pHBAExt?
                RemoveEntryList(pNextEntry);
                InterlockedDecrement(&(LONG)pHBAExt->pMPDrvObj->DrvInfoNbrMPHBAObj);
                break;
            }
        }
        lock_release(&pHBAExt->pMPDrvObj->Lock);
    }
    lock_acquire(&pHBAExt->DeviceListLock);
    LIST_FOR_EACH(entry, &pHBAExt->DeviceList){
        PVMP_DEVICE_INFO device = CONTAINING_RECORD(entry, VMP_DEVICE_INFO, Bind);
        if (device->LuExt){
            lock_acquire(&device->LuExt->ServiceLock);
            device->LuExt->bIsMissing = TRUE;
            lock_release(&device->LuExt->ServiceLock);
        }
    }
    lock_release(&pHBAExt->DeviceListLock);
    KeAcquireGuardedMutex(&pHBAExt->LUListLock);
    while (&pHBAExt->LUList != (entry = RemoveHeadList(&pHBAExt->LUList))){
        PHW_LU_EXTENSION LuExt = CONTAINING_RECORD(entry, HW_LU_EXTENSION, List);
        KeAcquireGuardedMutex(&LuExt->WaitingListLock);
        while (&LuExt->WaitingList != (request = RemoveHeadList(&LuExt->WaitingList))){
            PVMP_REQUEST _req = CONTAINING_RECORD(request, VMP_REQUEST, Bind);
            InsertTailList(&RemoveList, &_req->Bind);
        }
        KeReleaseGuardedMutex(&LuExt->WaitingListLock);
        KeAcquireGuardedMutex(&LuExt->RequestListLock);
        while (&LuExt->RequestList != (request = RemoveHeadList(&LuExt->RequestList))){
            PVMP_REQUEST _req = CONTAINING_RECORD(request, VMP_REQUEST, Bind);
            InsertTailList(&RemoveList, &_req->Bind);
        }
        KeReleaseGuardedMutex(&LuExt->RequestListLock);
    }
    KeReleaseGuardedMutex(&pHBAExt->LUListLock);
    while (&RemoveList != (request = RemoveHeadList(&RemoveList))){
        PVMP_REQUEST _req = CONTAINING_RECORD(request, VMP_REQUEST, Bind);
        LIST_ENTRY* e;
        while (&_req->SrbList != (e = RemoveHeadList(&_req->SrbList))){
            PMP_SRB_WORK_PARMS pSrbExt = CONTAINING_RECORD(e, MP_SRB_WORK_PARMS, Bind);
            if (pSrbExt->pSrb->Length == sizeof(SCSI_REQUEST_BLOCK)){
                pSrbExt->pSrb->SrbStatus = SRB_STATUS_ERROR;
                __try {
                    StorPortNotification(RequestComplete, pHBAExt, pSrbExt->pSrb);
                }
                __except (EXCEPTION_EXECUTE_HANDLER) {
                    NTSTATUS Status = GetExceptionCode();
                    DoStorageTraceEtw(DbgLvlErr, MpDemoDebugInfo,
                        __FUNCTION__": Exception Code = 0x%x. \n",
                        Status);
                }
            }
            __free(pSrbExt);
        }
        __free(_req);
    }
    lock_acquire(&pHBAExt->DeviceListLock);
    while (&pHBAExt->DeviceList != (entry = RemoveHeadList(&pHBAExt->DeviceList))){
        PVMP_DEVICE_INFO device = CONTAINING_RECORD(entry, VMP_DEVICE_INFO, Bind);
        InterlockedDecrement(&(LONG)pHBAExt->NbrLUNsperHBA);
        RtlClearBits(&ScsiBitMapHeader, (ULONG)device->Lun + (((ULONG)device->TargetId)*MAX_LUNS), 1);
        if (device->Event){
            KeSetEvent(device->Event, 0, FALSE);
            ObDereferenceObjectDeferDelete(device->Event);
        }    
        __free(device);
    }
    lock_release(&pHBAExt->DeviceListLock);
    StorPortNotification(RequestTimerCall, pHBAExt,
        MPStorTimer, (ULONG)0);
    if (NULL != pHBAExt->pMPDrvObj && IsListEmpty(&pHBAExt->pMPDrvObj->ListMPHBAObj)) {
        ExFreePoolWithTag(pHBAExt->pMPDrvObj, MP_TAG_GENERAL);
        pHBAExt->pMPDrvObj = NULL;
    }
}                                                     
// End MpHwFreeAdapterResources().

/**************************************************************************************************/                         
/*                                                                                                */                         
/* MpProcServReq.                                                                                 */                         
/*                                                                                                */                         
/**************************************************************************************************/                         
VOID
MpProcServReq(
              __in PHW_HBA_EXT          pHBAExt,      // Adapter device-object extension from StorPort.
              __in PIRP                 pIrp          // IRP pointer received.
             )
{
    DoStorageTraceEtw(DbgLvlLoud, MpDemoDebugInfo, "MpProcServReq entered\n");
    PIO_STACK_LOCATION		irpSp = IoGetCurrentIrpStackLocation(pIrp);
    NTSTATUS				status = STATUS_INVALID_DEVICE_REQUEST;

    if (irpSp->MajorFunction == IRP_MJ_DEVICE_CONTROL) {
#if 0
        __try {
            status = MpProcessIoCtl(pHBAExt, pIrp);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            status = GetExceptionCode();
            DoStorageTraceEtw(DbgLvlErr, MpDemoDebugInfo,
                __FUNCTION__": Exception Code = 0x%x. \n",
                status);
        }
#else
        status = MpProcessIoCtl(pHBAExt, pIrp);
#endif
    }

    if (status != STATUS_PENDING) {
        pIrp->IoStatus.Status = status;
        //IoCompleteRequest(pIrp, IO_NO_INCREMENT);
        StorPortCompleteServiceIrp(pHBAExt, pIrp);
    }
    DoStorageTraceEtw(DbgLvlLoud, MpDemoDebugInfo, ("MpProcServReq Exit\n"));
}                                                     // End MpProcServReq().

NTSTATUS
MpProcessIoCtl(
__in PHW_HBA_EXT          pHBAExt,      // Adapter device-object extension from StorPort.
__in PIRP                 pIrp          // IRP pointer received.
)
{
    NTSTATUS           Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(pIrp);
    ULONG              inputBufferLength;

    inputBufferLength =
        irpStack->Parameters.DeviceIoControl.InputBufferLength;

    switch (irpStack->Parameters.DeviceIoControl.IoControlCode) {
        case IOCTL_MINIPORT_PROCESS_SERVICE_IRP:

        {
            PCOMMAND_IN command = (PCOMMAND_IN)pIrp->AssociatedIrp.SystemBuffer;

            if (!pIrp->AssociatedIrp.SystemBuffer ||
                irpStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(COMMAND_IN)) {
                DoStorageTraceEtw(DbgLvlErr, MpDemoDebugInfo,
                    __FUNCTION__": IOCTL = 0x%x. SystemBuffer Error\n",
                    irpStack->Parameters.DeviceIoControl.IoControlCode);
                return STATUS_INVALID_PARAMETER;
            }
            switch (command->IoControlCode) {

                case IOCTL_VMPORT_SCSIPORT: {
                    Status = STATUS_SUCCESS;
                }
                break;

                case IOCTL_VMPORT_CONNECT: {
                    PCONNECT_IN pConnectInfo = (PCONNECT_IN)pIrp->AssociatedIrp.SystemBuffer;
                    CHAR        SzDiskId[MAX_DISK_ID_LENGTH];
                    PKEVENT     pKEvent = NULL;
                    ULONG       length = 0;
                    RtlZeroMemory(SzDiskId, sizeof(SzDiskId));
                    if (!pConnectInfo ||
                        irpStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(CONNECT_IN)) {
                        DoStorageTraceEtw(DbgLvlErr, MpDemoDebugInfo,
                            __FUNCTION__": IOCTL = 0x%x. SystemBuffer Error\n",
                            irpStack->Parameters.DeviceIoControl.IoControlCode);
                        Status = STATUS_INVALID_PARAMETER;
                        break;
                    }
                    if (!wcslen(pConnectInfo->DiskId)){
                         DoStorageTraceEtw(DbgLvlErr, MpDemoDebugInfo,
                             __FUNCTION__": IOCTL = 0x%x. Pathname Error\n",
                            irpStack->Parameters.DeviceIoControl.IoControlCode);
                        Status = STATUS_INVALID_PARAMETER;
                        break;
                    }

                    Status = ObReferenceObjectByHandle(pConnectInfo->NotifyEvent,
                        SYNCHRONIZE | EVENT_MODIFY_STATE,
                        *ExEventObjectType,
                        pIrp->RequestorMode,
                        &pKEvent,
                        NULL
                        );
                    if (NT_SUCCESS(Status)){
                        if (STATUS_SUCCESS != (Status = RtlUnicodeToUTF8N((PCHAR)SzDiskId, sizeof(SzDiskId), (PULONG)&length, (PCWCH)pConnectInfo->DiskId, (ULONG)sizeof(pConnectInfo->DiskId)))){
                            DoStorageTraceEtw(DbgLvlErr, MpDemoDebugInfo, __FUNCTION__": Unable to RtlUnicodeToUTF8N, Error = 0x%x\n", Status);
                        }
                    }
                    if (NT_SUCCESS(Status)){
                        KeAcquireGuardedMutex(&pHBAExt->pMPDrvObj->ConnectionMutex);
                        //
                        // See if we already have a connection with this 
                        // information.   If we do, then we will not
                        // create a new connection.
                        if (FindMatchLun(pHBAExt, pConnectInfo->DiskId, NULL))
                        {
                            //
                            // We already have a connection to this, so
                            // we indicate that point and exit.
                            //
                            ObDereferenceObjectDeferDelete(pKEvent);
                            KeReleaseGuardedMutex(&pHBAExt->pMPDrvObj->ConnectionMutex);
                            Status = STATUS_DEVICE_ALREADY_ATTACHED;
                            DoStorageTraceEtw(DbgLvlErr, MpDemoDebugInfo,
                                __FUNCTION__": IOCTL = 0x%x. Connection Exists Error\n",
                                irpStack->Parameters.DeviceIoControl.IoControlCode);
                            break;
                        }

                        //
                        // This looks like a unique connection, Attempt to create a 
                        // connection to the Filer.
                        //
                        PVMP_DEVICE_INFO device = NULL;
                        Status = CreateLun(pHBAExt, pConnectInfo, pKEvent, SzDiskId, pConnectInfo->MergeIOs, &device);
                        if (Status == STATUS_SUCCESS && irpStack->Parameters.DeviceIoControl.OutputBufferLength >= (sizeof(CONNECT_IN_RESULT))){
                            PCONNECT_IN_RESULT      pResult = (PCONNECT_IN_RESULT)pIrp->AssociatedIrp.SystemBuffer;
                            pResult->TargetId = device->TargetId;
                            pResult->PathId = device->PathId;
                            pResult->Lun = device->Lun;
                            pResult->MaxTransferLength = pHBAExt->MaxTransferBlocks << 9;
                            pResult->Device = device;
                            pIrp->IoStatus.Information = (sizeof(CONNECT_IN_RESULT));
                        }
                        KeReleaseGuardedMutex(&pHBAExt->pMPDrvObj->ConnectionMutex);
                    }
                    NT_ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
                }
                break;

                case IOCTL_VMPORT_DISCONNECT: {
                    PVMP_DEVICE_INFO device = NULL;
                    PCONNECT_IN pDisconnectInfo = (PCONNECT_IN)pIrp->AssociatedIrp.SystemBuffer;
                    if (!pDisconnectInfo ||
                        irpStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(CONNECT_IN)) {
                        DoStorageTraceEtw(DbgLvlErr, MpDemoDebugInfo,
                            __FUNCTION__": IOCTL = 0x%x. SystemBuffer Error\n",
                            irpStack->Parameters.DeviceIoControl.IoControlCode);
                        Status = STATUS_INVALID_PARAMETER;
                        break;

                    }

                    //
                    // Validate the connection Information before passing it on.
                    //
                    if (!wcslen(pDisconnectInfo->DiskId)){
                        DoStorageTraceEtw(DbgLvlErr, MpDemoDebugInfo,
                            __FUNCTION__": IOCTL = 0x%x. SystemBuffer Error\n",
                            irpStack->Parameters.DeviceIoControl.IoControlCode);
                        Status = STATUS_INVALID_PARAMETER;
                        break;
                    }

                    KeAcquireGuardedMutex(&pHBAExt->pMPDrvObj->ConnectionMutex);
                    //
                    // See if we already have a connection with this 
                    // information.   If we do, then we will not
                    // create a new connection.

                    if (!FindMatchLun(pHBAExt, pDisconnectInfo->DiskId, &device)){
                        KeReleaseGuardedMutex(&pHBAExt->pMPDrvObj->ConnectionMutex);
                        Status = STATUS_OBJECT_NAME_NOT_FOUND;
                        DoStorageTraceEtw(DbgLvlErr, MpDemoDebugInfo,
                            __FUNCTION__": IOCTL = 0x%x. No Connection Match Error\n",
                            irpStack->Parameters.DeviceIoControl.IoControlCode);
                        break;
                    }

                    //
                    // This looks like a unique connection, Attempt to delete a 
                    // connection to the Filer.
                    //

                    DeleteDevice(pHBAExt, device);

                    KeReleaseGuardedMutex(&pHBAExt->pMPDrvObj->ConnectionMutex);
                    

                    NT_ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
                }
                break;

                case IOCTL_VMPORT_GETACTIVELIST: {

                    if (!pIrp->AssociatedIrp.SystemBuffer ||
                        (irpStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(GETACTIVELIST_OUT))) {
                        DoStorageTraceEtw(DbgLvlErr, MpDemoDebugInfo,
                            __FUNCTION__": IOCTL = 0x%x. System Buffer Error\n",
                            irpStack->Parameters.DeviceIoControl.IoControlCode);
                        Status = STATUS_INVALID_PARAMETER;
                        break;

                    }

                    //
                    // Enumerate the active connections.
                    //
                    KeAcquireGuardedMutex(&pHBAExt->pMPDrvObj->ConnectionMutex);

                    Status = EnumerateActiveLuns(pHBAExt, pIrp);

                    KeReleaseGuardedMutex(&pHBAExt->pMPDrvObj->ConnectionMutex);
                    NT_ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
                }
                break;
                case IOCTL_VMPORT_SEND_AND_GET:	
                case IOCTL_VMPORT_GET_REQUEST:{
#if USE_STORPORT_GET_SYSTEM_ADDRESS
                    ULONG                     lclStatus;
                    PVOID                     pX = NULL;
#endif
                    PVMP_REQUEST			  req = NULL;
                    PVMP_REQUEST			  new_req = NULL;
                    PVMPORT_CONTROL_IN pCtrlIn = (PVMPORT_CONTROL_IN)pIrp->AssociatedIrp.SystemBuffer;
                    PVMPORT_CONTROL_OUT pCtrlout = (PVMPORT_CONTROL_OUT)pIrp->AssociatedIrp.SystemBuffer;
                    PHW_LU_EXTENSION pLuEx = NULL;
                    if (!pCtrlIn ||
                        irpStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(VMPORT_CONTROL_IN)) {
                        DoStorageTraceEtw(DbgLvlErr, MpDemoDebugInfo,
                            __FUNCTION__": IOCTL = 0x%x. SystemBuffer Error\n",
                            irpStack->Parameters.DeviceIoControl.IoControlCode);
                        Status = STATUS_INVALID_PARAMETER;
                        break;

                    }
#if 0
                    if (!FindMatchLunFastMutex(pHBAExt, pCtrlIn->Device, &pLuEx)){
                        Status = STATUS_OBJECT_NAME_NOT_FOUND;
                        DoStorageTraceEtw(DbgLvlErr, MpDemoDebugInfo,
                            __FUNCTION__": IOCTL = 0x%x. No Connection Match Error\n",
                            irpStack->Parameters.DeviceIoControl.IoControlCode);
                        break;
                    }
#else
                    if (pCtrlIn->Device && pCtrlIn->Device->Magic == VMP_DEVICE_INFO_MAGIC)
                        pLuEx = pCtrlIn->Device->LuExt;
                    else{
                        Status = STATUS_OBJECT_NAME_NOT_FOUND;
                        DoStorageTraceEtw(DbgLvlErr, MpDemoDebugInfo,
                            __FUNCTION__": IOCTL = 0x%x. No Connection Match Error\n",
                            irpStack->Parameters.DeviceIoControl.IoControlCode);
                        break;
                    }
#endif
                    if (!pLuEx){
                        pCtrlout->RequestType = VMPORT_NONE_REQUEST;
                        lclStatus = STOR_STATUS_SUCCESS;
                        break;
                    }
                    else if (pLuEx->bIsMissing){
                        Status = STATUS_OBJECT_NAME_NOT_FOUND;
                        break;
                    }
                    else{
                        //
                        // This looks like a unique connection, Attempt to delete a 
                        // connection to the Filer.
                        //
                        KeQueryTickCount(&pLuEx->UpdateTimeStamp);
                        if (IOCTL_VMPORT_SEND_AND_GET == command->IoControlCode){
                            KeAcquireGuardedMutex(&pLuEx->WaitingListLock);
                            LIST_ENTRY* entry;
                            LIST_FOR_EACH(entry, &pLuEx->WaitingList){
                                PVMP_REQUEST _req = CONTAINING_RECORD(entry, VMP_REQUEST, Bind);
                                if (_req->RequestID == pCtrlIn->RequestID){
                                    RemoveEntryList(&_req->Bind); 
                                    req = _req;
                                    break;
                                }
                            }
                            KeReleaseGuardedMutex(&pLuEx->WaitingListLock);
                        }
                        KeAcquireGuardedMutex(&pLuEx->RequestListLock);
                        if (IsListEmpty(&pLuEx->RequestList)){
                            new_req = NULL;
                            pCtrlout->RequestType = VMPORT_NONE_REQUEST;
                            if (pLuEx->Event)
                                KeClearEvent(pLuEx->Event);
                        }
                        else{
                            LIST_ENTRY* entry = RemoveHeadList(&pLuEx->RequestList);
                            if (&pLuEx->RequestList != entry){
                                new_req = CONTAINING_RECORD(entry, VMP_REQUEST, Bind);
                                KeAcquireGuardedMutexUnsafe(&pLuEx->WaitingListLock);
                                InsertTailList(&pLuEx->WaitingList, &new_req->Bind);
                                KeReleaseGuardedMutexUnsafe(&pLuEx->WaitingListLock);
                            }
                        }
                        KeReleaseGuardedMutex(&pLuEx->RequestListLock);
                        if (req){
                            LIST_ENTRY* e;
                            lclStatus = STOR_STATUS_SUCCESS;
                            if (pCtrlIn->ErrorCode){
                                while (&req->SrbList != (e = RemoveHeadList(&req->SrbList))){
                                    PMP_SRB_WORK_PARMS pSrbExt = CONTAINING_RECORD(e, MP_SRB_WORK_PARMS, Bind);
                                    if (pSrbExt->pSrb->Length == sizeof(SCSI_REQUEST_BLOCK)){
                                        pSrbExt->pSrb->SrbStatus = SRB_STATUS_ERROR;
                                        __try {
                                            StorPortNotification(RequestComplete, pHBAExt, pSrbExt->pSrb);
                                        }
                                        __except (EXCEPTION_EXECUTE_HANDLER) {
                                            DoStorageTraceEtw(DbgLvlErr, MpDemoDebugInfo,
                                                __FUNCTION__": (Send) Exception Code = 0x%x. \n",
                                                GetExceptionCode());
                                        }
                                    }
                                    __free(pSrbExt);
                                }
                            }
                            else{
                                if (ActionRead == req->Action) {
                                    while (&req->SrbList != (e = RemoveHeadList(&req->SrbList))){
                                        PMP_SRB_WORK_PARMS pSrbExt = CONTAINING_RECORD(e, MP_SRB_WORK_PARMS, Bind);
                                        if (pSrbExt->pSrb->Length == sizeof(SCSI_REQUEST_BLOCK)){
                                            pSrbExt->pSrb->SrbStatus = SRB_STATUS_SUCCESS;
                                            PMDL pMdl = NULL;
                                            lclStatus = StorPortGetOriginalMdl(pHBAExt, pSrbExt->pSrb, (PVOID*)&pMdl);
                                            if (STOR_STATUS_SUCCESS != lclStatus || !pMdl) {
                                                DoStorageTraceEtw(DbgLvlInfo, MpDemoDebugInfo, "(Send) Failed to get system address for pSrb = 0x%p, pSrb->DataBuffer=0x%p, status = 0x%08x, pX = 0x%p\n",
                                                    pSrbExt->pSrb, pSrbExt->pSrb->DataBuffer, lclStatus, pX);
                                                pSrbExt->pSrb->SrbStatus = SRB_STATUS_ERROR;
                                            }
                                            else{
                                                PVOID pBuffer = MmGetSystemAddressForMdlSafe(pMdl, NormalPagePriority);
                                                if (!pBuffer) {
                                                    pSrbExt->pSrb->SrbStatus = SRB_STATUS_ERROR;
                                                }
                                                else{
                                                    RtlCopyMemory(pBuffer, &pCtrlIn->ResponseBuffer[(pSrbExt->StartingSector - req->StartingSector) << 9], pSrbExt->pSrb->DataTransferLength);
                                                }
                                            }
                                            if (STOR_STATUS_INVALID_PARAMETER != lclStatus){
                                                __try {
                                                    StorPortNotification(RequestComplete, pHBAExt, pSrbExt->pSrb);
                                                }
                                                __except (EXCEPTION_EXECUTE_HANDLER) {
                                                    DoStorageTraceEtw(DbgLvlErr, MpDemoDebugInfo,
                                                        __FUNCTION__": (Send) Exception Code = 0x%x. \n",
                                                        GetExceptionCode());
                                                }
                                            }
                                        }
                                        __free(pSrbExt);
                                    }
                                }
                                else{
                                    while (&req->SrbList != (e = RemoveHeadList(&req->SrbList))){
                                        PMP_SRB_WORK_PARMS pSrbExt = CONTAINING_RECORD(e, MP_SRB_WORK_PARMS, Bind);
                                        if (pSrbExt->pSrb->Length == sizeof(SCSI_REQUEST_BLOCK)){
                                            pSrbExt->pSrb->SrbStatus = SRB_STATUS_SUCCESS;
                                            __try {
                                                StorPortNotification(RequestComplete, pHBAExt, pSrbExt->pSrb);
                                            }
                                            __except (EXCEPTION_EXECUTE_HANDLER) {
                                                DoStorageTraceEtw(DbgLvlErr, MpDemoDebugInfo,
                                                    __FUNCTION__": (Send) Exception Code = 0x%x. \n",
                                                    GetExceptionCode());
                                            }
                                        }
                                        __free(pSrbExt);
                                    }
                                }
                            }
                            __free(req);
                        }
                        pIrp->IoStatus.Information = sizeof(VMPORT_CONTROL_OUT);
                        if (new_req){
                            pCtrlout->RequestID = new_req->RequestID;
                            if (ActionRead == new_req->Action) {
                                pCtrlout->RequestType = VMPORT_READ_REQUEST;
                                pCtrlout->StartSector = new_req->StartingSector;
                                pCtrlout->RequestBufferLength = new_req->Length;
                            }
                            else{
                                LIST_ENTRY* e;
                                INT IS_ERROR = FALSE;
                                ULONGLONG EndSector = 0;
                                pCtrlout->RequestType = VMPORT_NONE_REQUEST;
                                pCtrlout->StartSector = new_req->EndSector;
                                pCtrlout->RequestBufferLength = 0;
                                LIST_FOR_EACH(e, &new_req->SrbList){
                                    PMP_SRB_WORK_PARMS pSrbExt = CONTAINING_RECORD(e, MP_SRB_WORK_PARMS, Bind);
                                    if (FALSE == IS_ERROR){
                                        pX = NULL;
                                        lclStatus = StorPortGetSystemAddress(pHBAExt, pSrbExt->pSrb, &pX);
                                        if (STOR_STATUS_SUCCESS != lclStatus || !pX) {
                                            IS_ERROR = TRUE;
                                            DoStorageTraceEtw(DbgLvlInfo, MpDemoDebugInfo, "(Write) Failed to get system address for pSrb = 0x%p, pSrb->DataBuffer=0x%p, status = 0x%08x, pX = 0x%p\n",
                                                pSrbExt->pSrb, pSrbExt->pSrb->DataBuffer, lclStatus, pX);
                                            pSrbExt->pSrb->SrbStatus = SRB_STATUS_ERROR;
                                            __try {
                                                StorPortNotification(RequestComplete, pHBAExt, pSrbExt->pSrb);
                                            }
                                            __except (EXCEPTION_EXECUTE_HANDLER) {
                                                DoStorageTraceEtw(DbgLvlErr, MpDemoDebugInfo,
                                                    __FUNCTION__": (Write) Exception Code = 0x%x. \n",
                                                    GetExceptionCode());
                                            }
                                            RemoveEntryList(&pSrbExt->Bind);
                                            __free(pSrbExt);
                                        }
                                        else{
                                            pCtrlout->RequestType = VMPORT_WRITE_REQUEST;
                                            if (pCtrlout->StartSector > pSrbExt->StartingSector)
                                                pCtrlout->StartSector = pSrbExt->StartingSector;
                                            if (pSrbExt->EndSector > EndSector)
                                                EndSector = pSrbExt->EndSector;
                                            RtlCopyMemory(&pCtrlout->RequestBuffer[(pSrbExt->StartingSector - new_req->StartingSector) << 9], pX, pSrbExt->pSrb->DataTransferLength);
                                        }
                                    }
                                    else{
                                        RemoveEntryList(&pSrbExt->Bind);
                                        pSrbExt->pQueueWorkItem = IoAllocateWorkItem((PDEVICE_OBJECT)pHBAExt->pDrvObj);
                                        if (NULL == pSrbExt->pQueueWorkItem) {
                                            DoStorageTraceEtw(DbgLvlErr, MpDemoDebugInfo, "ScsiReadWriteSetup: Failed to allocate work item\n");
                                            __free(pSrbExt);
                                            pSrbExt->pSrb->SrbStatus = SRB_STATUS_ERROR;
                                            __try {
                                                StorPortNotification(RequestComplete, pHBAExt, pSrbExt->pSrb);
                                            }
                                            __except (EXCEPTION_EXECUTE_HANDLER) {
                                                DoStorageTraceEtw(DbgLvlErr, MpDemoDebugInfo,
                                                    __FUNCTION__": (Write) Exception Code = 0x%x. \n",
                                                    GetExceptionCode());
                                            }
                                        }
                                        else{
                                            // Queue work item, which will run in the System process.
                                            IoQueueWorkItem(pSrbExt->pQueueWorkItem, MpGeneralWkRtn, DelayedWorkQueue, pSrbExt);
                                        }
                                    }
                                }
                                if (EndSector > 0){
                                    pCtrlout->RequestBufferLength = (ULONG)((EndSector - pCtrlout->StartSector) << 9);
                                    pIrp->IoStatus.Information = sizeof(VMPORT_CONTROL_OUT) + pCtrlout->RequestBufferLength;
                                    if (TRUE == IS_ERROR){
                                        if (pCtrlout->StartSector > new_req->StartingSector){
                                            RtlMoveMemory(pCtrlout->RequestBuffer, &pCtrlout->RequestBuffer[(pCtrlout->StartSector - new_req->StartingSector) << 9], pCtrlout->RequestBufferLength);
                                            new_req->StartingSector = pCtrlout->StartSector;
                                            new_req->MaxEndSector = new_req->StartingSector + pHBAExt->MaxTransferBlocks;
                                        }
                                        new_req->Length = pCtrlout->RequestBufferLength;
                                        new_req->EndSector = EndSector;
                                    }
                                }
                                else{
                                    KeAcquireGuardedMutex(&pLuEx->WaitingListLock);
                                    RemoveEntryList(&new_req->Bind);
                                    KeReleaseGuardedMutex(&pLuEx->WaitingListLock);
                                    __free(new_req);
                                    pIrp->IoStatus.Information = sizeof(VMPORT_CONTROL_OUT) + pCtrlout->RequestBufferLength;
                                }
                            }
                        }
                    }
                    NT_ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
                }
                break;
                default:
                    DoStorageTraceEtw(DbgLvlErr, MpDemoDebugInfo,
                        "ScsiPortDeviceControl: Unsupported IOCTL (%x)\n",
                        irpStack->Parameters.DeviceIoControl.IoControlCode);
                    Status = STATUS_INVALID_DEVICE_REQUEST;
                    break;
                }
            }
            NT_ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
            break;

        default:
            Status = STATUS_INVALID_DEVICE_REQUEST;

            break;
    }
    return Status;
}

BOOLEAN FindMatchLun(
    __in    PHW_HBA_EXT          pHBAExt,
    __in	PCWCHAR              pDiskId,
    __in	PVMP_DEVICE_INFO*    PPFoundDev){
    BOOLEAN Result = FALSE;
    lock_acquire(&pHBAExt->DeviceListLock);
    LIST_ENTRY* entry;
    LIST_FOR_EACH(entry, &pHBAExt->DeviceList){
        PVMP_DEVICE_INFO device = CONTAINING_RECORD(entry, VMP_DEVICE_INFO, Bind);
        if ((!wcsncmp((const PWCHAR)device->DiskId,
            (const PWCHAR)pDiskId, MAX_DISK_ID_LENGTH))) {
            Result = TRUE;
            if (PPFoundDev) {
                *PPFoundDev = device;
            }
            break;
        }
    }
    lock_release(&pHBAExt->DeviceListLock);
    return Result;
}

VOID DeleteDevice(
    __in PHW_HBA_EXT pHBAExt,
    __in PVMP_DEVICE_INFO device){
    if (device->LuExt){
        if (!device->LuExt->bIsMissing){
            lock_acquire(&device->LuExt->ServiceLock);
            device->LuExt->bIsMissing = TRUE;
            lock_release(&device->LuExt->ServiceLock);
            PMP_SRB_WORK_PARMS             pWkRtnParms;
            pWkRtnParms =                                     // Allocate parm area for work routine.
                (PMP_SRB_WORK_PARMS)__malloc(sizeof(MP_SRB_WORK_PARMS));
            if (NULL == pWkRtnParms) {
                DoStorageTraceEtw(DbgLvlErr, MpDemoDebugInfo, "DeleteDevice Failed to allocate work parm structure\n");
            }
            else{
                RtlZeroMemory(pWkRtnParms, sizeof(MP_SRB_WORK_PARMS));
                pWkRtnParms->pHBAExt = pHBAExt;
                pWkRtnParms->Action = ActionRemove;
                pWkRtnParms->pLUExt = device->LuExt;
                pWkRtnParms->pQueueWorkItem = IoAllocateWorkItem((PDEVICE_OBJECT)pHBAExt->pDrvObj);
                if (NULL == pWkRtnParms->pQueueWorkItem) {
                    DoStorageTraceEtw(DbgLvlErr, MpDemoDebugInfo, "DeleteDevice: Failed to allocate work item\n");
                    __free(pWkRtnParms);
                }
                else{
                    // Queue work item, which will run in the System process.
                    IoQueueWorkItem(pWkRtnParms->pQueueWorkItem, MpGeneralWkRtn, DelayedWorkQueue, pWkRtnParms);
                }
            }
        }
    }
    else{
        if (device->Event){
            KeSetEvent(device->Event, 0, FALSE);
            ObDereferenceObjectDeferDelete(device->Event);
        }
        lock_acquire(&pHBAExt->DeviceListLock);
        InterlockedDecrement(&(LONG)pHBAExt->NbrLUNsperHBA);
        RtlClearBits(&ScsiBitMapHeader, (ULONG)device->Lun + (((ULONG)device->TargetId)*MAX_LUNS), 1);
        RemoveEntryList(&device->Bind);
        lock_release(&pHBAExt->DeviceListLock);
        __free(device);
        __try {
            StorPortNotification(BusChangeDetected, pHBAExt, 0);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            DoStorageTraceEtw(DbgLvlErr, MpDemoDebugInfo,
                __FUNCTION__": Exception Code = 0x%x. \n",
                GetExceptionCode());
        }
    }
}

VOID DeleteLunExt(
    __in PHW_HBA_EXT pHBAExt,
    __in PHW_LU_EXTENSION pLuExt){
    LIST_ENTRY	          RemoveList;
    LIST_ENTRY*           request;
    LIST_ENTRY*           e;
    InitializeListHead(&RemoveList);
    PVMP_DEVICE_INFO device = NULL; 
    lock_acquire(&pLuExt->ServiceLock);
    device = pLuExt->Dev;
    pLuExt->bIsMissing = TRUE;
    pLuExt->Dev = NULL;
    pLuExt->Event = NULL;
    lock_release(&pLuExt->ServiceLock);
    KeAcquireGuardedMutex(&pLuExt->WaitingListLock);
    while (&pLuExt->WaitingList != (request = RemoveHeadList(&pLuExt->WaitingList))){
        PVMP_REQUEST _req = CONTAINING_RECORD(request, VMP_REQUEST, Bind);
        InsertTailList(&RemoveList, &_req->Bind);
    }
    KeReleaseGuardedMutex(&pLuExt->WaitingListLock);
    KeAcquireGuardedMutex(&pLuExt->RequestListLock);
    while (&pLuExt->RequestList != (request = RemoveHeadList(&pLuExt->RequestList))){
        PVMP_REQUEST _req = CONTAINING_RECORD(request, VMP_REQUEST, Bind);
        InsertTailList(&RemoveList, &_req->Bind);
    }
    KeReleaseGuardedMutex(&pLuExt->RequestListLock);
    KeAcquireGuardedMutex(&pHBAExt->LUListLock);
    RemoveEntryList(&pLuExt->List);
    KeReleaseGuardedMutex(&pHBAExt->LUListLock);
    while (&RemoveList != (request = RemoveHeadList(&RemoveList))){
        PVMP_REQUEST _req = CONTAINING_RECORD(request, VMP_REQUEST, Bind);
        while (&_req->SrbList != (e = RemoveHeadList(&_req->SrbList))){
            PMP_SRB_WORK_PARMS pSrbExt = CONTAINING_RECORD(e, MP_SRB_WORK_PARMS, Bind);
            if (pSrbExt->pSrb->Length == sizeof(SCSI_REQUEST_BLOCK)){
                pSrbExt->pSrb->SrbStatus = SRB_STATUS_ERROR;
                __try {
                    StorPortNotification(RequestComplete, pHBAExt, pSrbExt->pSrb);
                }
                __except (EXCEPTION_EXECUTE_HANDLER) {
                    DoStorageTraceEtw(DbgLvlErr, MpDemoDebugInfo,
                        __FUNCTION__": (Send) Exception Code = 0x%x. \n",
                        GetExceptionCode());
                }
            }
            __free(pSrbExt);
        }
        __free(_req);
    }
    if (device->Event){
        KeSetEvent(device->Event, 0, FALSE);
        ObDereferenceObjectDeferDelete(device->Event);
    }
    lock_acquire(&pHBAExt->DeviceListLock);
    InterlockedDecrement(&(LONG)pHBAExt->NbrLUNsperHBA);
    RtlClearBits(&ScsiBitMapHeader, (ULONG)device->Lun + (((ULONG)device->TargetId)*MAX_LUNS), 1);
    RemoveEntryList(&device->Bind);
    lock_release(&pHBAExt->DeviceListLock);
    __free(device);
    __try {
        StorPortNotification(BusChangeDetected, pHBAExt, 0);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        DoStorageTraceEtw(DbgLvlErr, MpDemoDebugInfo,
            __FUNCTION__": Exception Code = 0x%x. \n",
            GetExceptionCode());
    }
}

NTSTATUS EnumerateActiveLuns(
    __in PHW_HBA_EXT pHBAExt,
    __in PIRP Irp){
    PIO_STACK_LOCATION      irpStack = IoGetCurrentIrpStackLocation(Irp);
    PGETACTIVELIST_OUT      pActiveList = (PGETACTIVELIST_OUT)Irp->AssociatedIrp.SystemBuffer;
    PACTIVELIST_ENTRY_OUT   pNextEntry = &pActiveList->ActiveEntry[0];
    ULONG                   countRemaining;
    NTSTATUS                Status = STATUS_BUFFER_OVERFLOW;
    pActiveList->ActiveListCount = 0;
    //
    // Determine how many entries can be kept in the list.
    //
    countRemaining = (irpStack->Parameters.DeviceIoControl.OutputBufferLength -
        sizeof(ULONG)) / sizeof(ACTIVELIST_ENTRY_OUT);
    lock_acquire(&pHBAExt->DeviceListLock);
    LIST_ENTRY* entry;
    LIST_FOR_EACH(entry, &pHBAExt->DeviceList){
        PVMP_DEVICE_INFO device = CONTAINING_RECORD(entry, VMP_DEVICE_INFO, Bind);
        pNextEntry = &pActiveList->ActiveEntry[pActiveList->ActiveListCount];
        RtlZeroMemory(pNextEntry, sizeof(ACTIVELIST_ENTRY_OUT));
        RtlCopyMemory(pNextEntry->DiskId, device->DiskId, min(wcslen(device->DiskId), MAX_DISK_ID_LENGTH) * sizeof(WCHAR));
        pNextEntry->PathId = device->PathId;
        pNextEntry->DiskSizeMB = device->DiskSizeMB;
        pNextEntry->TargetId = device->TargetId;
        pNextEntry->Lun = device->Lun;
        pNextEntry->Connected = !device->Missing;
        pNextEntry->MergeIOs = device->MergeIOs;
        pNextEntry->Device = device;
        pActiveList->ActiveListCount++;
        countRemaining--;
        //
        // Move to the next and continue matching
        //
        if (!countRemaining)
            break;
    }
    lock_release(&pHBAExt->DeviceListLock);
    if (entry == pHBAExt->DeviceList.Flink) {		 // Need to check here 
        Status = STATUS_SUCCESS;
    }
    Irp->IoStatus.Information = (pActiveList->ActiveListCount * sizeof(ACTIVELIST_ENTRY_OUT)) + sizeof(ULONG);
    return Status;
}

NTSTATUS CreateLun(
    __in PHW_HBA_EXT pHBAExt,
    __in PCONNECT_IN PConnectInfo,
    __in PKEVENT pkEvent,
    __in PCSTR   szDiskId,
    __in BOOLEAN MergeIOs,
    __out PVMP_DEVICE_INFO* ppDevice){
    NTSTATUS           Status = STATUS_INSUFFICIENT_RESOURCES;
    ULONG bitNumber = RtlFindClearBitsAndSet(&ScsiBitMapHeader, 1, 0);
    if (bitNumber != 0xFFFFFFFF) {
        ULONG Lun = bitNumber % MAX_LUNS;
        ULONG TargetId = bitNumber / MAX_LUNS;
        //
        //  Get the address of the logical unit for this device.
        //
        PHW_LU_EXTENSION luExt = (PHW_LU_EXTENSION)StorPortGetLogicalUnit(pHBAExt,
            (UCHAR)PathId,
            (UCHAR)TargetId,
            (UCHAR)Lun);

        if (luExt) {
            DoStorageTraceEtw(DbgLvlErr, MpDemoDebugInfo, __FUNCTION__": LuExt %p exists for %d:%d:%d\n", luExt, PathId, TargetId, Lun);
        }
        else{
            PVMP_DEVICE_INFO device = __malloc(sizeof(VMP_DEVICE_INFO));
            if (device){
                RtlZeroMemory(device, sizeof(VMP_DEVICE_INFO));
                RtlCopyMemory(device->DiskId, PConnectInfo->DiskId, min(wcslen(PConnectInfo->DiskId), MAX_DISK_ID_LENGTH) * sizeof(WCHAR));
                RtlCopyMemory(device->SzDiskId, szDiskId, min(strlen(szDiskId), MAX_DISK_ID_LENGTH));
                device->DiskSizeMB = PConnectInfo->DiskSizeMB;
                device->Lun = (UCHAR)Lun;
                device->PathId = (UCHAR)PathId;
                device->TargetId = (UCHAR)TargetId;
                device->Magic = VMP_DEVICE_INFO_MAGIC;
                device->Event = pkEvent;
                device->MergeIOs = MergeIOs;
                KeQueryTickCount(&device->CreatedTimeStamp);
                lock_acquire(&pHBAExt->DeviceListLock);
                InsertTailList(&pHBAExt->DeviceList, &device->Bind);
                InterlockedIncrement(&(LONG)pHBAExt->NbrLUNsperHBA);
                lock_release(&pHBAExt->DeviceListLock);
                Status = STATUS_SUCCESS;
                __try {
                    StorPortNotification(BusChangeDetected, pHBAExt, 0);
                }
                __except (EXCEPTION_EXECUTE_HANDLER) {
                    DoStorageTraceEtw(DbgLvlErr, MpDemoDebugInfo,
                        __FUNCTION__": Exception Code = 0x%x. \n",
                        GetExceptionCode());
                }
                *ppDevice = device;
            }
        }
    }
    return Status;
}

VOID MPStorTimer(
    _In_ PHW_HBA_EXT pHBAExt
    ){

    PMP_SRB_WORK_PARMS             pWkRtnParms;
    pWkRtnParms =                                     // Allocate parm area for work routine.
        (PMP_SRB_WORK_PARMS)__malloc(sizeof(MP_SRB_WORK_PARMS));
    if (NULL == pWkRtnParms) {
        DoStorageTraceEtw(DbgLvlErr, MpDemoDebugInfo, "ScsiReadWriteSetup Failed to allocate work parm structure\n");
    }
    else{
        RtlZeroMemory(pWkRtnParms, sizeof(MP_SRB_WORK_PARMS));
        pWkRtnParms->pHBAExt = pHBAExt;
        pWkRtnParms->Action = ActionTimer;
        pWkRtnParms->pQueueWorkItem = IoAllocateWorkItem((PDEVICE_OBJECT)pHBAExt->pDrvObj);
        if (NULL == pWkRtnParms->pQueueWorkItem) {
            DoStorageTraceEtw(DbgLvlErr, MpDemoDebugInfo, "ScsiReadWriteSetup: Failed to allocate work item\n");
            __free(pWkRtnParms);
        }
        else{
            // Queue work item, which will run in the System process.
            IoQueueWorkItem(pWkRtnParms->pQueueWorkItem, MpGeneralWkRtn, DelayedWorkQueue, pWkRtnParms);
        }
    }
}

BOOLEAN FindMatchLunFastMutex(
    __in    PHW_HBA_EXT          pHBAExt,
    __in	PVMP_DEVICE_INFO     Device,
    __in	PHW_LU_EXTENSION*    PPFoundLun){
    LIST_ENTRY* entry;
    BOOLEAN Result = FALSE;
    KeAcquireGuardedMutex(&pHBAExt->LUListLock);
    LIST_FOR_EACH(entry, &pHBAExt->LUList){
        PHW_LU_EXTENSION lun = CONTAINING_RECORD(entry, HW_LU_EXTENSION, List);
        if (lun->Dev == Device) {
            Result = TRUE;
            if (PPFoundLun) {
                *PPFoundLun = lun;
            }
            break;
        }
    }
    KeReleaseGuardedMutex(&pHBAExt->LUListLock);
    return Result;
}