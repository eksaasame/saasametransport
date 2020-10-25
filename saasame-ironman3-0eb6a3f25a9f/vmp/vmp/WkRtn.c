/****************************** Module Header ******************************\
* Module Name:  WkRtn.c
* Project:      CppWDKStorPortVirtualMiniport
*
* Copyright (c) Microsoft Corporation.
* 
* a.      MpGeneralWkRtn()
* Handles queued work elements by calling MpWkRtn.
*
* b.      MpWkRtn()
* Handles work elements, completes them.
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
#define WkRtnVer     "1.013"

#define _MP_H_skip_includes

#include "mp.h"

#pragma warning(push)
#pragma warning(disable : 4204)                       /* Prevent C4204 messages from stortrce.h. */
#include <stortrce.h>
#pragma warning(pop)

#include "trace.h"
#include "WkRtn.tmh"

static LONG RequestID = 0xFFFFFFFE; // checks for "roll-over" problems

/**************************************************************************************************/
/*                                                                                                */
/* Globals, forward definitions, etc.                                                             */
/*                                                                                                */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                                                                                */
/* This is the work routine, which always runs in System under an OS-supplied worker thread.      */
/*                                                                                                */
/**************************************************************************************************/
VOID
MpGeneralWkRtn(
__in PVOID           pDummy,           // Not used.
__in PVOID           pWkParms          // Parm list pointer.
)
{
    PMP_SRB_WORK_PARMS        pWkRtnParms = (PMP_SRB_WORK_PARMS)pWkParms;

    UNREFERENCED_PARAMETER(pDummy);

    IoFreeWorkItem(pWkRtnParms->pQueueWorkItem);      // Free queue item.

    pWkRtnParms->pQueueWorkItem = NULL;               // Be neat.

    MpWkRtn(pWkParms);                                // Do the actual work.
}                                                     // End MpGeneralWkRtn().

BOOLEAN MergeIOs(__in PHW_LU_EXTENSION pLUExt, __in PMP_SRB_WORK_PARMS pWkRtnParms, __in PVMP_REQUEST _req){
    BOOLEAN found = FALSE;
    if (pWkRtnParms->Action == _req->Action){
        if (pWkRtnParms->StartingSector >= _req->StartingSector){
            if (pWkRtnParms->StartingSector <= _req->EndSector && pWkRtnParms->EndSector < _req->MaxEndSector){
                InsertTailList(&_req->SrbList, &pWkRtnParms->Bind);
                found = TRUE;
                DoStorageTraceEtw(DbgLvlInfo, MpDemoDebugInfo, "pSrbExtension->StartingSector >= _req->StartingSector (Hit)\n");
                if (pWkRtnParms->EndSector > _req->EndSector){
                    _req->EndSector = pWkRtnParms->EndSector;
                    _req->Length = (ULONG)((_req->EndSector - _req->StartingSector) << 9);
                }
                if (pLUExt->Event)
                    KeSetEvent(pLUExt->Event, 0, FALSE);
            }
        }
        else if ( // pSrbExtension->StartingSector < _req->StartingSector &&
            pWkRtnParms->EndSector >= _req->StartingSector &&
            pWkRtnParms->EndSector < _req->MaxEndSector &&
            _req->EndSector < pWkRtnParms->MaxEndSector){
            InsertTailList(&_req->SrbList, &pWkRtnParms->Bind);
            found = TRUE;
            DoStorageTraceEtw(DbgLvlInfo, MpDemoDebugInfo, "pSrbExtension->StartingSector < _req->StartingSector (Hit)\n");
            _req->StartingSector = pWkRtnParms->StartingSector;
            _req->MaxEndSector = pWkRtnParms->MaxEndSector;
            if (pWkRtnParms->EndSector > _req->EndSector){
                _req->EndSector = pWkRtnParms->EndSector;
            }
            _req->Length = (ULONG)((_req->EndSector - _req->StartingSector) << 9);
            if (pLUExt->Event)
                KeSetEvent(pLUExt->Event, 0, FALSE);
        }
    }
    return found;
}

/**************************************************************************************************/
/*                                                                                                */
/* This routine does the "read"/"write" work, by copying to/from the miniport's buffers.          */
/*                                                                                                */
/**************************************************************************************************/
VOID
MpWkRtn(__in PVOID pWkParms)                          // Parm list pointer.
{
    PMP_SRB_WORK_PARMS        pWkRtnParms = (PMP_SRB_WORK_PARMS)pWkParms;
    PHW_HBA_EXT               pHBAExt = pWkRtnParms->pHBAExt;	
    PHW_LU_EXTENSION          pLUExt = pWkRtnParms->pLUExt;
    PSCSI_REQUEST_BLOCK       pSrb = pWkRtnParms->pSrb;
    if (pWkRtnParms->Action == ActionRemove){
        DeleteLunExt(pHBAExt, pLUExt);
    }
    else if (pWkRtnParms->Action == ActionTimer){
        ULONG timerInterval = (ULONG)1000000 * 30;
        ULONG tickIncrement;
        LONGLONG  ticks, ticks2, timeout, timeout2;
        timeout = (LONGLONG)120 * 10000000;
        timeout2 = (LONGLONG)30 * 10000000;	    
        LARGE_INTEGER CurrentTimeStamp;
        tickIncrement = KeQueryTimeIncrement();
        ticks = (timeout / tickIncrement);
        ticks2 = (timeout2 / tickIncrement);
        KeQueryTickCount(&CurrentTimeStamp);
        LIST_ENTRY* entry;
        lock_acquire(&pHBAExt->DeviceListLock);
        PVMP_DEVICE_INFO dev = NULL;
        LIST_FOR_EACH(entry, &pHBAExt->DeviceList){
            PVMP_DEVICE_INFO device = CONTAINING_RECORD(entry, VMP_DEVICE_INFO, Bind);
            if (NULL == device->LuExt){
                if ((CurrentTimeStamp.QuadPart - device->CreatedTimeStamp.QuadPart) > ticks2){
                    timerInterval = 1000000;
                    dev = device;
                    break;
                }
            }
            else if ((CurrentTimeStamp.QuadPart - device->LuExt->UpdateTimeStamp.QuadPart) > ticks){
                timerInterval = 1000000;
                dev = device;
                break;
            }
        }
        lock_release(&pHBAExt->DeviceListLock);
        if (dev) {
            DeleteDevice(pHBAExt, dev); 
        }	
       

        StorPortNotification(RequestTimerCall, pHBAExt,
            MPStorTimer, timerInterval);
        __free(pWkParms);      // Free parm list.
    }
    else if (pLUExt->bIsMissing){
        __try {
            pSrb->SrbStatus = SRB_STATUS_ERROR;
            StorPortNotification(RequestComplete, pHBAExt, pSrb);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            DoStorageTraceEtw(DbgLvlErr, MpDemoDebugInfo,
                __FUNCTION__":  (Get) Exception Code = 0x%x. \n",
                GetExceptionCode());
        }
        __free(pWkParms);      // Free parm list.
    }
    else{
        BOOLEAN found = FALSE;
        pWkRtnParms->MaxEndSector = pWkRtnParms->StartingSector + pHBAExt->MaxTransferBlocks;
        KeAcquireGuardedMutex(&pLUExt->RequestListLock);
        LIST_ENTRY* entry;

        if (pLUExt->MergeIOs){
            LIST_FOR_EACH(entry, &pLUExt->RequestList){
                PVMP_REQUEST _req = CONTAINING_RECORD(entry, VMP_REQUEST, Bind);
                if (TRUE == (found = MergeIOs(pLUExt, pWkRtnParms, _req)))
                    break;
            }
        }
        //else if (!IsListEmpty(&pLUExt->RequestList)){
        //	entry = pLUExt->RequestList.Flink;
        //	PVMP_REQUEST _req = CONTAINING_RECORD(entry, VMP_REQUEST, Bind);	
        //	found = MergeIOs(pLUExt, pWkRtnParms, _req);
        //}

        if (TRUE == found){
            KeReleaseGuardedMutex(&pLUExt->RequestListLock);
        }
        else{
            PVMP_REQUEST pRequest = __malloc(sizeof(VMP_REQUEST));
            if (NULL == pRequest){
                DoStorageTraceEtw(DbgLvlErr, MpDemoDebugInfo, "ScsiReadWriteSetup Failed to allocate request structure\n");
                KeReleaseGuardedMutex(&pLUExt->RequestListLock);
                __try {
                    pSrb->SrbStatus = SRB_STATUS_ERROR;
                    StorPortNotification(RequestComplete, pHBAExt, pSrb);
                }
                __except (EXCEPTION_EXECUTE_HANDLER) {
                    DoStorageTraceEtw(DbgLvlErr, MpDemoDebugInfo,
                        __FUNCTION__":  (Get) Exception Code = 0x%x. \n",
                        GetExceptionCode());
                }
            }
            else{
                RtlZeroMemory(pRequest, sizeof(VMP_REQUEST));
                pRequest->RequestID = InterlockedIncrement(&(LONG)RequestID);
                InitializeListHead(&pRequest->SrbList);
                InsertTailList(&pRequest->SrbList, &pWkRtnParms->Bind);
                pRequest->StartingSector = pWkRtnParms->StartingSector;
                pRequest->EndSector = pWkRtnParms->EndSector;
                pRequest->MaxEndSector = pWkRtnParms->MaxEndSector;
                pRequest->Length = pSrb->DataTransferLength;

                if (ActionRead == pWkRtnParms->Action){
                    pRequest->Action = ActionRead;
                }
                else{
                    pRequest->Action = ActionWrite;
                }                
                InsertTailList(&pLUExt->RequestList, &pRequest->Bind);
                if (pLUExt->Event)
                    KeSetEvent(pLUExt->Event, 0, FALSE);               
                KeReleaseGuardedMutex(&pLUExt->RequestListLock);
                if (!pLUExt->Requested){
                    KeAcquireGuardedMutex(&pHBAExt->LUListLock);
                    InsertTailList(&pHBAExt->LUList, &pLUExt->List);  // Add LUN extension to list in HBA extension.
                    pLUExt->Requested = TRUE;
                    KeReleaseGuardedMutex(&pHBAExt->LUListLock);
                }
            }
        }
    }  
}// End MpWkRtn().

