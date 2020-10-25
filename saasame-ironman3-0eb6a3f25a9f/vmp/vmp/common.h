/*++

Copyright (c) 1990-2011 Microsoft Corporation.  All Rights Reserved

Module Name:
    common.h

Abstract: 

Author:

Enviroment:

Revision History:

--*/

#ifndef _COMMON_H_
#define _COMMON_H_

#ifdef _NTDDK_
#include <ntdef.h>
#else
#ifndef _WINIOCTL
#define _WINIOCTL
#include <winioctl.h>
#endif
typedef void *PVMP_DEVICE_INFO;
#define MAX_DISK_ID_LENGTH 40
#endif
#ifdef __cplusplus
extern "C" {
#endif

#define DISK_DEVICE         0x00
#define FILE_DEVICE_VMPORT  63273
#define USER_VM_IOCTL_START 3085

typedef struct _COMMAND_IN {
    ULONG		IoControlCode;
} COMMAND_IN, *PCOMMAND_IN;

#define IOCTL_VMPORT_SCSIPORT CTL_CODE(FILE_DEVICE_VMPORT,USER_VM_IOCTL_START,METHOD_BUFFERED,FILE_ALL_ACCESS)
#define IOCTL_VMPORT_CONNECT  CTL_CODE(FILE_DEVICE_VMPORT,USER_VM_IOCTL_START+2,METHOD_BUFFERED,FILE_READ_ACCESS|FILE_WRITE_ACCESS)

typedef struct _CONNECT_IN {
    COMMAND_IN		Command;
    WCHAR           DiskId[MAX_DISK_ID_LENGTH];
    ULONG           DiskSizeMB;
    HANDLE			NotifyEvent;
    BOOLEAN         MergeIOs;
} CONNECT_IN, *PCONNECT_IN;

typedef struct _CONNECT_IN_RESULT {
    USHORT            PathId;
    USHORT            TargetId;
    USHORT            Lun;
    ULONG             MaxTransferLength;
    PVMP_DEVICE_INFO  Device;
} CONNECT_IN_RESULT, *PCONNECT_IN_RESULT;

#define IOCTL_VMPORT_DISCONNECT CTL_CODE(FILE_DEVICE_VMPORT,USER_VM_IOCTL_START+3,METHOD_BUFFERED,FILE_WRITE_ACCESS)

//
// USE THE CONNECT_IN structure for DISCONNECT.
//

#define IOCTL_VMPORT_GETACTIVELIST CTL_CODE(FILE_DEVICE_VMPORT,USER_VM_IOCTL_START+4,METHOD_BUFFERED,FILE_WRITE_ACCESS)

typedef struct _ACTIVELIST_ENTRY_OUT {
    WCHAR             DiskId[MAX_DISK_ID_LENGTH];
    USHORT            Connected;
    USHORT            PathId;
    USHORT            TargetId;
    USHORT            Lun;
    ULONG             DiskSizeMB;
    BOOLEAN           MergeIOs;
    PVMP_DEVICE_INFO  Device;
} ACTIVELIST_ENTRY_OUT, *PACTIVELIST_ENTRY_OUT;

typedef struct _GETACTIVELIST_OUT {
    ULONG                   ActiveListCount;
    ACTIVELIST_ENTRY_OUT    ActiveEntry[1];
} GETACTIVELIST_OUT, *PGETACTIVELIST_OUT;

#define IOCTL_VMPORT_GET_REQUEST CTL_CODE(FILE_DEVICE_VMPORT, USER_VM_IOCTL_START+5, METHOD_BUFFERED, FILE_READ_ACCESS|FILE_WRITE_ACCESS)
#define IOCTL_VMPORT_SEND_AND_GET CTL_CODE(FILE_DEVICE_VMPORT, USER_VM_IOCTL_START+6, METHOD_BUFFERED, FILE_READ_ACCESS|FILE_WRITE_ACCESS)

//
// Data structures used for communicating between service and driver
//

typedef struct _VMPORT_CONTROL_IN {
    COMMAND_IN		     Command;
    PVMP_DEVICE_INFO     Device;
    //
    // The request ID is used to match up the response to the original request
    //
    ULONG RequestID;
    //
    // The response error code
    //
    ULONG   ErrorCode;
    //
    // This specifies the size of the request buffer
    //
    ULONG   ResponseBufferLength;
    UCHAR   ResponseBuffer[1];
} VMPORT_CONTROL_IN, *PVMPORT_CONTROL_IN;

#define VMPORT_NONE_REQUEST  0x00
#define VMPORT_READ_REQUEST  0x10
#define VMPORT_WRITE_REQUEST 0x20

typedef struct _VMPORT_CONTROL_OUT {
    //
    // The request ID is used to match up this response to the original request
    //
    ULONG       RequestID;
    ULONG       RequestType;
    ULONGLONG   StartSector;
    ULONG       RequestBufferLength;
    UCHAR       RequestBuffer[1];

}VMPORT_CONTROL_OUT, *PVMPORT_CONTROL_OUT;

#ifdef __cplusplus
}
#endif

#endif    // _COMMON_H_
