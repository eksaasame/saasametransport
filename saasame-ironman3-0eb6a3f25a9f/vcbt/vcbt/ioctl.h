
#ifndef _IOCTL_
#define _IOCTL_

#include "type.h"
#include <ntddk.h>
#include "stdio.h"
#include "ntddk.h"
#include "ntdddisk.h"
#include "stdarg.h"
#include "stdio.h"
#include <ntddvol.h>
#include <mountdev.h>

NTSTATUS execute_ioctl(IN PDEVICE_OBJECT Target, IN ULONG IoControlCode, IN PVOID InBuf, IN ULONG InBufLen, IN PVOID OutBuf, IN ULONG OutBufLen, OUT IO_STATUS_BLOCK * IoStatusBlock);
NTSTATUS execute_ioctl_with_timeout(IN PDEVICE_OBJECT Target, IN ULONG IoControlCode, IN PVOID InBuf, IN ULONG InBufLen, IN PVOID OutBuf, IN ULONG OutBufLen, OUT IO_STATUS_BLOCK * IoStatusBlock, UINT32 ulTimeOut);

NTSTATUS get_device_number(IN PDEVICE_OBJECT Target, STORAGE_DEVICE_NUMBER* number);
NTSTATUS query_device_name(IN PDEVICE_OBJECT Target, PMOUNTDEV_NAME* name);
NTSTATUS query_volume_number(IN PDEVICE_OBJECT Target, VOLUME_NUMBER* volume_number);
NTSTATUS get_volume_length(IN PDEVICE_OBJECT Target, LARGE_INTEGER* volume_length);
bool     is_volume_device(PDEVICE_OBJECT Target);
NTSTATUS get_sector_size(IN PDEVICE_OBJECT Target, DWORD* BytesPerSector);

#endif