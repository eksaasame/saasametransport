/****************************** Module Header ******************************\
* Module Name:  scsi.c
* Project:      CppWDKStorPortVirtualMiniport
*
* Copyright (c) Microsoft Corporation.
* 
* a.       ScsiExecuteMain()
* Handles SCSI SRBs with opcodes needed to support file system operations by 
* calling subroutines. Fails SRBs with other opcodes.
* Note: In a real-world virtual miniport, it may be necessary to handle other opcodes.
* 
* b.      ScsiOpInquiry()
* Handles Inquiry, including creating a new LUN as needed.
* 
* c.       ScsiOpVPD()
* Handles Vital Product Data.
* 
* d.      ScsiOpRead()
* Beginning of a SCSI Read operation.
* 
* e.      ScsiOpWrite()
* Beginning of a SCSI Write operation.
* 
* f.        ScsiReadWriteSetup()
* Sets up a work element for SCSI Read or Write and enqueues the element.
* 
* g.       ScsiOpReportLuns()
* Handles Report LUNs.
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

#define MPScsiFile     "2.025"

#include "mp.h"
#include "storport.h"

#pragma warning(push)
#pragma warning(disable : 4204)                       /* Prevent C4204 messages from stortrce.h. */
#include <stortrce.h>
#pragma warning(pop)

#include "trace.h"
#include "scsi.tmh"

VOID
ScsiIoControl(
__in PHW_HBA_EXT            pHBAExt,    // Adapter device-object extension from port driver.
__in PSCSI_REQUEST_BLOCK    pSrb,
__in PUCHAR                 pResult
){
    UNREFERENCED_PARAMETER(pHBAExt);
    UNREFERENCED_PARAMETER(pSrb);
    //PSRB_IO_CONTROL  srb_io_control = (PSRB_IO_CONTROL)pSrb->DataBuffer;
    *pResult = ResultDone;

    return;
}

/**************************************************************************************************/     
/*                                                                                                */     
/**************************************************************************************************/     
UCHAR
ScsiExecuteMain(
                __in PHW_HBA_EXT          pHBAExt,    // Adapter device-object extension from StorPort.
                __in PSCSI_REQUEST_BLOCK  pSrb,
                __in PUCHAR               pResult
               )
{
    PHW_LU_EXTENSION pLUExt;
    UCHAR            status = SRB_STATUS_INVALID_REQUEST;

    DoStorageTraceEtw(DbgLvlLoud, MpDemoDebugInfo, "ScsiExecute: pSrb = 0x%p, CDB = 0x%x Path: %x TID: %x Lun: %x\n",
                      pSrb, pSrb->Cdb[0], pSrb->PathId, pSrb->TargetId, pSrb->Lun);

    *pResult = ResultDone;

    pLUExt = StorPortGetLogicalUnit(pHBAExt,          // Get the LU extension from StorPort.
                                    pSrb->PathId,
                                    pSrb->TargetId,
                                    pSrb->Lun 
                                   );

    if (!pLUExt) {
        DoStorageTraceEtw(DbgLvlErr, MpDemoDebugInfo, "Unable to get LUN extension for device %d:%d:%d\n",
                   pSrb->PathId, pSrb->TargetId, pSrb->Lun);

        status = SRB_STATUS_NO_DEVICE;
        goto Done;
    }

    
    // Handle sufficient opcodes to support a LUN suitable for a file system. Other opcodes are failed.

    switch (pSrb->Cdb[0]) {

        case SCSIOP_TEST_UNIT_READY:
        case SCSIOP_SYNCHRONIZE_CACHE:
        case SCSIOP_START_STOP_UNIT:
        case SCSIOP_VERIFY:
            status = SRB_STATUS_SUCCESS;
            break;

        case SCSIOP_INQUIRY:
            status = ScsiOpInquiry(pHBAExt, pLUExt, pSrb);
            break;

        case SCSIOP_READ_CAPACITY:
        case SCSIOP_READ_CAPACITY16:
            status = ScsiOpReadCapacity(pHBAExt, pLUExt, pSrb);
            break;
        
        case SCSIOP_READ:
        case SCSIOP_READ16:
            status = ScsiOpRead(pHBAExt, pLUExt, pSrb, pResult);
            break;

        case SCSIOP_WRITE:
        case SCSIOP_WRITE16:
            status = ScsiOpWrite(pHBAExt, pLUExt, pSrb, pResult);
            break;

        case SCSIOP_MODE_SENSE:
            status = ScsiOpModeSense(pHBAExt, pLUExt, pSrb);
            break;

        case SCSIOP_REPORT_LUNS:                      
            status = ScsiOpReportLuns(pHBAExt, pLUExt, pSrb);
            break;

        default:
            status = SRB_STATUS_INVALID_REQUEST;
            break;

    } // switch (pSrb->Cdb[0])

Done:
    return status;
}                                                     // End ScsiExecuteMain.

/**************************************************************************************************/     
/*                                                                                                */     
/**************************************************************************************************/     
UCHAR
ScsiOpInquiry(
              __in PHW_HBA_EXT          pHBAExt,      // Adapter device-object extension from StorPort.
              __in PHW_LU_EXTENSION     pLUExt,       // LUN device-object extension from StorPort.
              __in PSCSI_REQUEST_BLOCK  pSrb
             )
{
    PINQUIRYDATA          pInqData = pSrb->DataBuffer;// Point to Inquiry buffer.
    UCHAR                 deviceType,
                          status = SRB_STATUS_SUCCESS;
    PCDB                  pCdb;

    DoStorageTraceEtw(DbgLvlInfo, MpDemoDebugInfo, "Path: %d TID: %d Lun: %d\n",
                      pSrb->PathId, pSrb->TargetId, pSrb->Lun);

    RtlZeroMemory((PUCHAR)pSrb->DataBuffer, pSrb->DataTransferLength);
    PVMP_DEVICE_INFO device;
    deviceType = MpGetDeviceType(pHBAExt, pSrb->PathId, pSrb->TargetId, pSrb->Lun, &device);

    if (DEVICE_NOT_FOUND==deviceType) {
       pSrb->DataTransferLength = 0;
       status = SRB_STATUS_INVALID_LUN;

       goto done;
    }
    pCdb = (PCDB)pSrb->Cdb;

    if (1==pCdb->CDB6INQUIRY3.EnableVitalProductData) {
        DoStorageTraceEtw(DbgLvlLoud, MpDemoDebugInfo, "Received VPD request for page 0x%x\n",
                          pCdb->CDB6INQUIRY.PageCode);

        status = ScsiOpVPD(pHBAExt, device, pSrb);

        goto done;
    }

    pInqData->DeviceType = deviceType;
    pInqData->RemovableMedia = FALSE;
    pInqData->CommandQueue = TRUE;

    RtlMoveMemory(pInqData->VendorId, pHBAExt->VendorId, 8);
    RtlMoveMemory(pInqData->ProductId, pHBAExt->ProductId, 16);
    RtlMoveMemory(pInqData->ProductRevisionLevel, pHBAExt->ProductRevision, 4);

    if (deviceType!=DISK_DEVICE) {                    // Shouldn't happen.
        goto done;
    }

    // Check if the device has already been seen.

    if (GET_FLAG(pLUExt->LUFlags, LU_DEVICE_INITIALIZED)) {
        // This is an existing device.          
        goto done;
    }

    //
    // A new LUN.
    //
    device->LuExt      = pLUExt;
    pLUExt->Dev        = device;
    pLUExt->DeviceType = deviceType;
    pLUExt->TargetId   = pSrb->TargetId;
    pLUExt->Lun        = pSrb->Lun; 
    pLUExt->MergeIOs   = device->MergeIOs;
    pLUExt->Event      = device->Event;
    pLUExt->MaxBlocks = (ULONGLONG)(device->DiskSizeMB / MP_BLOCK_SIZE) * 1024 * 1024;
    pLUExt->Requested = FALSE;
    RtlZeroMemory(pLUExt->DiskId, sizeof(pLUExt->DiskId));
    RtlCopyMemory(pLUExt->DiskId, device->DiskId, min(wcslen(device->DiskId), MAX_DISK_ID_LENGTH) * sizeof(WCHAR));

    lock_init(&pLUExt->ServiceLock);
    KeQueryTickCount(&pLUExt->UpdateTimeStamp);
    InitializeListHead(&pLUExt->List);

    KeInitializeGuardedMutex(&pLUExt->RequestListLock);   // Initialize spin lock.
    InitializeListHead(&pLUExt->RequestList);    // Initialize list head.

    KeInitializeGuardedMutex(&pLUExt->WaitingListLock);   // Initialize spin lock.
    InitializeListHead(&pLUExt->WaitingList);    // Initialize list head.

    SET_FLAG(pLUExt->LUFlags, LU_DEVICE_INITIALIZED);
    DoStorageTraceEtw(DbgLvlInfo, MpDemoDebugInfo, "New device created. %d:%d:%d\n", pSrb->PathId, pSrb->TargetId, pSrb->Lun);

done:
    return status;
}                                                     // End ScsiOpInquiry.

/**************************************************************************************************/     
/*                                                                                                */     
/**************************************************************************************************/     
UCHAR
ScsiOpVPD(
          __in PHW_HBA_EXT          pHBAExt,          // Adapter device-object extension from StorPort.
          __in PVMP_DEVICE_INFO     pDev,           // LUN device-object extension from StorPort.
          __in PSCSI_REQUEST_BLOCK  pSrb
         )
{
    UCHAR                  status;
    ULONG                  len;
    struct _CDB6INQUIRY3 * pVpdInquiry = (struct _CDB6INQUIRY3 *)&pSrb->Cdb;;
    UNREFERENCED_PARAMETER(pHBAExt);
    ASSERT(pSrb->DataTransferLength>0);

    if (0==pSrb->DataTransferLength) {
        DoStorageTraceEtw(DbgLvlErr, MpDemoDebugInfo, "pSrb->DataTransferLength = 0\n");

        return SRB_STATUS_DATA_OVERRUN;
      }

    RtlZeroMemory((PUCHAR)pSrb->DataBuffer,           // Clear output buffer.
                  pSrb->DataTransferLength);

    if (VPD_SUPPORTED_PAGES==pVpdInquiry->PageCode) { // Inquiry for supported pages?
      PVPD_SUPPORTED_PAGES_PAGE pSupportedPages;

      len = sizeof(VPD_SUPPORTED_PAGES_PAGE) + 8;

      if (pSrb->DataTransferLength < len) {
        return SRB_STATUS_DATA_OVERRUN;
      }

      pSupportedPages = pSrb->DataBuffer;             // Point to output buffer.

      pSupportedPages->DeviceType = DISK_DEVICE;
      pSupportedPages->DeviceTypeQualifier = 0;
      pSupportedPages->PageCode = VPD_SERIAL_NUMBER;
      pSupportedPages->PageLength = 8;                // Enough space for 4 VPD values.
      pSupportedPages->SupportedPageList[0] =         // Show page 0x80 supported.
        VPD_SERIAL_NUMBER;
      pSupportedPages->SupportedPageList[1] =         // Show page 0x83 supported.
        VPD_DEVICE_IDENTIFIERS;

      status = SRB_STATUS_SUCCESS;
    }
    else
    if (VPD_SERIAL_NUMBER==pVpdInquiry->PageCode) {   // Inquiry for serial number?
      PVPD_SERIAL_NUMBER_PAGE pVpd;
      len = sizeof(VPD_SERIAL_NUMBER_PAGE) + MAX_DISK_ID_LENGTH;
      if (pSrb->DataTransferLength < len) {
        return SRB_STATUS_DATA_OVERRUN;
      }

      pVpd = pSrb->DataBuffer;                        // Point to output buffer.

      pVpd->DeviceType = DISK_DEVICE;
      pVpd->DeviceTypeQualifier = 0;
      pVpd->PageCode = VPD_SERIAL_NUMBER;                
      pVpd->PageLength = MAX_DISK_ID_LENGTH;


    //      /* Generate a changing serial number. */
          //sprintf((char *)pVpd->SerialNumber, "%03d%02d%03d0123456789abcdefghijABCDEFGHIJxx\n", 
    //            pHBAExt->pMPDrvObj->DrvInfoNbrMPHBAObj, pLUExt->TargetId, pLUExt->Lun);
      RtlCopyMemory((PCHAR)pVpd->SerialNumber, pDev->SzDiskId, (ULONG)pVpd->PageLength);

      DoStorageTraceEtw(DbgLvlLoud, MpDemoDebugInfo,
                        "ScsiOpVPD:  VPD Page: %d Serial No.: %s", pVpd->PageCode, (const char *)pVpd->SerialNumber);

      status = SRB_STATUS_SUCCESS;
    }
    else
    if (VPD_DEVICE_IDENTIFIERS==pVpdInquiry->PageCode) { // Inquiry for device ids?
        PVPD_IDENTIFICATION_PAGE pVpid;
        PVPD_IDENTIFICATION_DESCRIPTOR pVpidDesc;

        #define VPIDNameSize 32

        len = sizeof(VPD_IDENTIFICATION_PAGE) + sizeof(VPD_IDENTIFICATION_DESCRIPTOR) + VPIDNameSize;

        if (pSrb->DataTransferLength < len) {
          return SRB_STATUS_DATA_OVERRUN;
        }

        pVpid = pSrb->DataBuffer;                     // Point to output buffer.

        pVpid->PageCode = VPD_DEVICE_IDENTIFIERS;

        pVpidDesc =                                   // Point to first (and only) descriptor.
            (PVPD_IDENTIFICATION_DESCRIPTOR)pVpid->Descriptors;

        pVpidDesc->CodeSet = VpdCodeSetAscii;         // Identifier contains ASCII.
        pVpidDesc->IdentifierType =                   // 
            VpdIdentifierTypeFCPHName;

        {
            /* Generate a changing serial number. */
            //sprintf((char *)pVpidDesc->Identifier, "%03d%02d%03d0123456789abcdefghij\n", 
            //        pHBAExt->pMPDrvObj->DrvInfoNbrMPHBAObj, pLUExt->TargetId, pLUExt->Lun);
            RtlCopyMemory((char *)pVpidDesc->Identifier, pDev->SzDiskId, 28);
        }

        pVpidDesc->IdentifierLength =                 // Size of Identifier.
            (UCHAR)strlen((const char *)pVpidDesc->Identifier) - 1;
        pVpid->PageLength =                           // Show length of remainder.
            (UCHAR)(FIELD_OFFSET(VPD_IDENTIFICATION_PAGE, Descriptors) + 
                    FIELD_OFFSET(VPD_IDENTIFICATION_DESCRIPTOR, Identifier) + 
                    pVpidDesc->IdentifierLength);

        DoStorageTraceEtw(DbgLvlLoud, MpDemoDebugInfo,
                          "ScsiOpVPD:  VPD Page 0x83");

        status = SRB_STATUS_SUCCESS;
    }
    else {
      status = SRB_STATUS_INVALID_REQUEST;
      len = 0;
    }

    pSrb->DataTransferLength = len;

    return status;
}                                                     // End ScsiOpVPD().

/**************************************************************************************************/     
/*                                                                                                */     
/**************************************************************************************************/     
UCHAR
ScsiOpReadCapacity(
                   __in PHW_HBA_EXT          pHBAExt, // Adapter device-object extension from StorPort.
                   __in PHW_LU_EXTENSION     pLUExt,  // LUN device-object extension from StorPort.
                   __in PSCSI_REQUEST_BLOCK  pSrb
                  )
{
    PREAD_CAPACITY_DATA  readCapacity = pSrb->DataBuffer;
    ULARGE_INTEGER       maxBlocks;
    LONG                 blockSize;

    UNREFERENCED_PARAMETER(pHBAExt);
    UNREFERENCED_PARAMETER(pLUExt);

    ASSERT(pLUExt != NULL);

    RtlZeroMemory((PUCHAR)pSrb->DataBuffer, pSrb->DataTransferLength );

    // Claim 512-byte blocks (big-endian).

    blockSize = MP_BLOCK_SIZE;

    //readCapacity->BytesPerBlock =
    //  (((PUCHAR)&blockSize)[0] << 24) |  (((PUCHAR)&blockSize)[1] << 16) |
    //  (((PUCHAR)&blockSize)[2] <<  8) | ((PUCHAR)&blockSize)[3];

    DoStorageTraceEtw(DbgLvlInfo, MpDemoDebugInfo, "Block Size: 0x%x\n", blockSize);
    //maxBlocks = (pHBAExt->pMPDrvObj->MPRegInfo.VirtualDiskSize/MP_BLOCK_SIZE)-1;

    maxBlocks.QuadPart = pLUExt->MaxBlocks;
    DoStorageTraceEtw(DbgLvlLoud, MpDemoDebugInfo, "Max Blocks: 0x%I64u\n", maxBlocks.QuadPart);

   /* readCapacity->LogicalBlockAddress =
      (((PUCHAR)&maxBlocks)[0] << 24) | (((PUCHAR)&maxBlocks)[1] << 16) |
      (((PUCHAR)&maxBlocks)[2] <<  8) | ((PUCHAR)&maxBlocks)[3];*/

    if (pSrb->Cdb[0] == SCSIOP_READ_CAPACITY)
    {
        REVERSE_BYTES(&readCapacity->BytesPerBlock, &blockSize);
        REVERSE_BYTES(&readCapacity->LogicalBlockAddress, &maxBlocks.LowPart);
    }
    else if (pSrb->Cdb[0] == SCSIOP_READ_CAPACITY16)
    {
        PREAD_CAPACITY16_DATA readCapacity16 = (PREAD_CAPACITY16_DATA)readCapacity;
        REVERSE_BYTES(&readCapacity16->BytesPerBlock, &blockSize);
        REVERSE_BYTES_QUAD(&readCapacity16->LogicalBlockAddress, &maxBlocks);

        if ((LONG)pSrb->DataTransferLength >= FIELD_OFFSET(READ_CAPACITY16_DATA, Reserved3))
        {
            readCapacity16->LBPME = FALSE;
            readCapacity16->LBPRZ = FALSE;
        }
    }
    return SRB_STATUS_SUCCESS;
}                                                     // End ScsiOpReadCapacity.

/**************************************************************************************************/     
/*                                                                                                */     
/**************************************************************************************************/     
UCHAR
ScsiOpRead(
           __in PHW_HBA_EXT          pHBAExt,         // Adapter device-object extension from StorPort.
           __in PHW_LU_EXTENSION     pLUExt,          // LUN device-object extension from StorPort.
           __in PSCSI_REQUEST_BLOCK  pSrb,
           __in PUCHAR               pResult
          )
{
    UCHAR                        status;

    status = ScsiReadWriteSetup(pHBAExt, pLUExt, pSrb, ActionRead, pResult);

    return status;
}                                                     // End ScsiOpRead.

/**************************************************************************************************/     
/*                                                                                                */     
/**************************************************************************************************/     
UCHAR
ScsiOpWrite(
            __in PHW_HBA_EXT          pHBAExt,        // Adapter device-object extension from StorPort.
            __in PHW_LU_EXTENSION     pLUExt,         // LUN device-object extension from StorPort.
            __in PSCSI_REQUEST_BLOCK  pSrb,
            __in PUCHAR               pResult
           )
{
    UCHAR                        status;

    status = ScsiReadWriteSetup(pHBAExt, pLUExt, pSrb, ActionWrite, pResult);

    return status;
}                                                     // End ScsiOpWrite.

/**************************************************************************************************/     
/*                                                                                                */     
/* This routine does the setup for reading or writing. The reading/writing could be effected      */     
/* here rather than in MpGeneralWkRtn, but in the general case MpGeneralWkRtn is going to be the  */     
/* place to do the work since it gets control at PASSIVE_LEVEL and so could do real I/O, could    */     
/* wait, etc, etc.                                                                                */     
/*                                                                                                */     
/**************************************************************************************************/     
UCHAR
ScsiReadWriteSetup(
                   __in PHW_HBA_EXT          pHBAExt, // Adapter device-object extension from StorPort.
                   __in PHW_LU_EXTENSION     pLUExt,  // LUN device-object extension from StorPort.        
                   __in PSCSI_REQUEST_BLOCK  pSrb,
                   __in VMP_REQUEST_ACTION   WkRtnAction,
                   __in PUCHAR               pResult
                  )
{
    PCDB                         pCdb = (PCDB)pSrb->Cdb;
    ULONGLONG                    startingSector;
    ULONG                        numBlocks;
    UCHAR						 Status = SRB_STATUS_SUCCESS;

    ASSERT(pLUExt != NULL);
    UNREFERENCED_PARAMETER(pHBAExt);

    *pResult = ResultDone;                            // Assume no queuing.
    if ((pCdb->AsByte[0] == SCSIOP_READ16) |
        (pCdb->AsByte[0] == SCSIOP_WRITE16))
    {
        REVERSE_BYTES_QUAD(&startingSector, pCdb->CDB16.LogicalBlock);
    }
    else
    {
        startingSector = 0;
        REVERSE_BYTES(&startingSector, &pCdb->CDB10.LogicalBlockByte0);
    }

    numBlocks = (ULONG)(pSrb->DataTransferLength / MP_BLOCK_SIZE);

    DoStorageTraceEtw(DbgLvlInfo, MpDemoDebugInfo, "ScsiReadWriteSetup action: %X, starting sector:  0x%I64u, number of blocks: 0x%X\n", WkRtnAction, startingSector, numBlocks);
    DoStorageTraceEtw(DbgLvlInfo, MpDemoDebugInfo, "ScsiReadWriteSetup pSrb: 0x%p, pSrb->DataBuffer: 0x%p\n", pSrb, pSrb->DataBuffer);

    if ( startingSector >= pLUExt->MaxBlocks ) {      // Starting sector beyond the bounds?
        DoStorageTraceEtw(DbgLvlErr, MpDemoDebugInfo, "*** ScsiReadWriteSetup Starting sector: 0x%I64u, number of blocks: %d\n", startingSector, numBlocks);
        Status = SRB_STATUS_INVALID_REQUEST;   
    }
    else{
        PMP_SRB_WORK_PARMS             pWkRtnParms;
        if (pLUExt->bIsMissing || NULL == pLUExt->Dev){
            DoStorageTraceEtw(DbgLvlErr, MpDemoDebugInfo, "ScsiReadWriteSetup Dev == NULL\n");
            Status = SRB_STATUS_ERROR;
        }
        else{
            pWkRtnParms =                                     // Allocate parm area for work routine.
                (PMP_SRB_WORK_PARMS)__malloc(sizeof(MP_SRB_WORK_PARMS));
            if (NULL == pWkRtnParms) {
                DoStorageTraceEtw(DbgLvlErr, MpDemoDebugInfo, "ScsiReadWriteSetup Failed to allocate work parm structure\n");
                Status = SRB_STATUS_ERROR;
            }
            else{
                RtlZeroMemory(pWkRtnParms, sizeof(MP_SRB_WORK_PARMS));
                pWkRtnParms->pHBAExt = pHBAExt;
                pWkRtnParms->pLUExt = pLUExt;
                pWkRtnParms->pSrb = pSrb;
                pWkRtnParms->Action = ActionRead == WkRtnAction ? ActionRead : ActionWrite;
                pWkRtnParms->StartingSector = startingSector;
                pWkRtnParms->EndSector = startingSector + (pSrb->DataTransferLength >> 9);
                pWkRtnParms->pQueueWorkItem = IoAllocateWorkItem((PDEVICE_OBJECT)pHBAExt->pDrvObj);
                if (NULL == pWkRtnParms->pQueueWorkItem) {
                    DoStorageTraceEtw(DbgLvlErr, MpDemoDebugInfo, "ScsiReadWriteSetup: Failed to allocate work item\n");
                    __free(pWkRtnParms);
                    Status = SRB_STATUS_ERROR;
                }
                else{
                    // Queue work item, which will run in the System process.
                    IoQueueWorkItem(pWkRtnParms->pQueueWorkItem, MpGeneralWkRtn, DelayedWorkQueue, pWkRtnParms);
                    *pResult = ResultQueued;
                }
            }
        }
    }
    return Status ;
}                                                     // End ScsiReadWriteSetup.

/**************************************************************************************************/     
/*                                                                                                */     
/**************************************************************************************************/     
UCHAR
ScsiOpModeSense(
                __in PHW_HBA_EXT          pHBAExt,    // Adapter device-object extension from StorPort.
                __in PHW_LU_EXTENSION     pLUExt,     // LUN device-object extension from StorPort.
                __in PSCSI_REQUEST_BLOCK  pSrb
               )
{
    UNREFERENCED_PARAMETER(pHBAExt);
    UNREFERENCED_PARAMETER(pLUExt);

    RtlZeroMemory((PUCHAR)pSrb->DataBuffer, pSrb->DataTransferLength);

    return SRB_STATUS_SUCCESS;
}

/**************************************************************************************************/     
/*                                                                                                */     
/**************************************************************************************************/     
UCHAR
ScsiOpReportLuns(                                     
                 __in __out PHW_HBA_EXT         pHBAExt,   // Adapter device-object extension from StorPort.
                 __in       PHW_LU_EXTENSION    pLUExt,    // LUN device-object extension from StorPort.
                 __in       PSCSI_REQUEST_BLOCK pSrb
                )
{
    UCHAR     status = SRB_STATUS_SUCCESS;
    PLUN_LIST pLunList = (PLUN_LIST)pSrb->DataBuffer; // Point to LUN list.
    ULONG     GoodLunIdx = 0, index = 0;

    UNREFERENCED_PARAMETER(pLUExt);

    if (FALSE==pHBAExt->bReportAdapterDone) {         // This opcode will be one of the earliest I/O requests for a new HBA (and may be received later, too).
        MpHwReportAdapter(pHBAExt);                   // WMIEvent test.

        MpHwReportLink(pHBAExt);                      // WMIEvent test.

        MpHwReportLog(pHBAExt);                       // WMIEvent test.

        pHBAExt->bReportAdapterDone = TRUE;
    }
    
    RtlZeroMemory((PUCHAR)pSrb->DataBuffer, pSrb->DataTransferLength);
    lock_acquire(&pHBAExt->DeviceListLock);
    LIST_ENTRY* entry;
    LIST_FOR_EACH(entry, &pHBAExt->DeviceList){
        PVMP_DEVICE_INFO device = CONTAINING_RECORD(entry, VMP_DEVICE_INFO, Bind);
        if (pSrb->PathId == device->PathId &&
            pSrb->TargetId == device->TargetId) {
            index++;
        }
    }
    if (pSrb->DataTransferLength >= FIELD_OFFSET(LUN_LIST, Lun) + (sizeof(pLunList->Lun[0])*index)) {
        LIST_FOR_EACH(entry, &pHBAExt->DeviceList){
            PVMP_DEVICE_INFO device = CONTAINING_RECORD(entry, VMP_DEVICE_INFO, Bind);
            if (pSrb->PathId == device->PathId &&
                pSrb->TargetId == device->TargetId) {			
                pLunList->Lun[GoodLunIdx++][1] = (UCHAR)device->Lun;
            }
        }
    }
    else{
        status = SRB_STATUS_DATA_OVERRUN;
    }
    pLunList->LunListLength[3] =                  // Set length needed for LUNs.
        (UCHAR)(8 * index);
    lock_release(&pHBAExt->DeviceListLock);

    return status;
}                                                     // End ScsiOpReportLuns.

