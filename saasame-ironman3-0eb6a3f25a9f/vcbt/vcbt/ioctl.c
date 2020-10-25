
#include "ioctl.h"
#include "trace.h"
#include "ioctl.tmh"

NTSTATUS execute_ioctl(IN PDEVICE_OBJECT Target, IN ULONG IoControlCode, IN PVOID InBuf, IN ULONG InBufLen, IN PVOID OutBuf, IN ULONG OutBufLen, OUT IO_STATUS_BLOCK * IoStatusBlock){
    PIRP NewIrp;
    KEVENT Event;
    NTSTATUS Status = STATUS_SUCCESS;
    IO_STATUS_BLOCK DummyIoStatusBlock;

    if (IoStatusBlock == NULL)
        IoStatusBlock = &DummyIoStatusBlock;

    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    NewIrp = IoBuildDeviceIoControlRequest(IoControlCode, Target, InBuf, InBufLen, OutBuf, OutBufLen, FALSE, &Event, IoStatusBlock);
    if (NewIrp == NULL){
        Status = STATUS_INSUFFICIENT_RESOURCES;
        DoTraceMsg(3, "execute_ioctl: Failed to build irp");
    }
    else{
        Status = IoCallDriver(Target, NewIrp);
        if (Status == STATUS_PENDING){
            KeWaitForSingleObject(&Event, Suspended, KernelMode, FALSE, NULL);
            Status = IoStatusBlock->Status;
        }
    }
    return Status;
}

typedef enum {
    IRP_LOCK_CANCELABLE,
    IRP_LOCK_CANCEL_STARTED,
    IRP_LOCK_CANCEL_COMPLETE,
    IRP_LOCK_COMPLETED
} IRP_LOCK;

NTSTATUS
ioctl_with_timeout_completion(
IN PDEVICE_OBJECT   DeviceObject,
IN PIRP             Irp,
IN PVOID            Context
){
    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Irp);
    PLONG lock;
    lock = (PLONG)Context;
    if (InterlockedExchange((PVOID)&lock, IRP_LOCK_COMPLETED) == IRP_LOCK_CANCEL_STARTED) {
        // 
        // Main line code has got the control of the IRP. It will
        // now take the responsibility of completing the IRP. 
        // Therefore...
        return STATUS_MORE_PROCESSING_REQUIRED;
    }
    return STATUS_CONTINUE_COMPLETION;
}

NTKERNELAPI
PDEVICE_OBJECT
IoGetAttachedDevice(
IN PDEVICE_OBJECT DeviceObject
);

NTSTATUS execute_ioctl_with_timeout(IN PDEVICE_OBJECT Target, IN ULONG IoControlCode, IN PVOID InBuf, IN ULONG InBufLen, IN PVOID OutBuf, IN ULONG OutBufLen, OUT IO_STATUS_BLOCK * IoStatusBlock, UINT32 ulTimeOut){
    PIRP            NewIrp;
    KEVENT          Event;
    IRP_LOCK        lock;
    NTSTATUS        Status = STATUS_SUCCESS;
    IO_STATUS_BLOCK DummyIoStatusBlock;
    LARGE_INTEGER   wait;
    PDEVICE_OBJECT	topDev = NULL;

    if (NULL==(topDev = IoGetAttachedDevice(Target))){
        Status = STATUS_INVALID_DEVICE_STATE;
        DoTraceMsg(3, "execute_ioctl_with_timeout: Failed to get the attached device.");
        return Status;
    }

    if (IoStatusBlock == NULL)
        IoStatusBlock = &DummyIoStatusBlock;

    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    NewIrp = IoBuildDeviceIoControlRequest(IoControlCode, Target, InBuf, InBufLen, OutBuf, OutBufLen, FALSE, &Event, IoStatusBlock);
    if (NewIrp == NULL){
        Status = STATUS_INSUFFICIENT_RESOURCES;
        DoTraceMsg(3, "execute_ioctl_with_timeout: Failed to build irp");
        return Status;
    }

    lock = IRP_LOCK_CANCELABLE;

    IoSetCompletionRoutine(
        NewIrp,
        ioctl_with_timeout_completion,
        &lock,
        TRUE,
        TRUE,
        TRUE
        );

    Status = IoCallDriver(Target, NewIrp);
    if (Status == STATUS_PENDING){
        wait.QuadPart = (__int64)(-1 * 1000 * 10) * (__int64)(ulTimeOut);
        if ((Status = KeWaitForSingleObject(&Event, Suspended, KernelMode, FALSE, &wait)) == STATUS_TIMEOUT){
            if (InterlockedExchange((PVOID)&lock, IRP_LOCK_CANCEL_STARTED) == IRP_LOCK_CANCELABLE){
                // 
                // You got it to the IRP before it was completed. You can cancel
                // the IRP without fear of losing it, because the completion routine
                // does not let go of the IRP until you allow it.
                // 
                IoCancelIrp(NewIrp);
                // 
                // Release the completion routine. If it already got there,
                // then you need to complete it yourself. Otherwise, you got
                // through IoCancelIrp before the IRP completed entirely.
                // 
                if (InterlockedExchange((LONG volatile *)&lock, IRP_LOCK_CANCEL_COMPLETE) == IRP_LOCK_COMPLETED){
                    IoCompleteRequest(NewIrp, IO_NO_INCREMENT);
                }
            }
            KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
            IoStatusBlock->Status = STATUS_TIMEOUT; // Return STATUS_TIMEOUT
        }
        else{
            Status = IoStatusBlock->Status;
        }
    }
    return Status;
}

NTSTATUS get_device_number(IN PDEVICE_OBJECT Target, STORAGE_DEVICE_NUMBER* number){
    RtlZeroMemory(number, sizeof(STORAGE_DEVICE_NUMBER));
    return execute_ioctl(Target, IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, 0, number, sizeof(STORAGE_DEVICE_NUMBER), NULL);
}

NTSTATUS query_device_name(IN PDEVICE_OBJECT Target, PMOUNTDEV_NAME* name){
    NTSTATUS        status;
    ULONG           outputSize = sizeof(MOUNTDEV_NAME);
    PMOUNTDEV_NAME  output;

    output = ExAllocatePool(PagedPool, outputSize);
    if (!output) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    status = execute_ioctl(Target, IOCTL_MOUNTDEV_QUERY_DEVICE_NAME, NULL, 0, output, outputSize, NULL); 
    if (status == STATUS_BUFFER_OVERFLOW) {
        outputSize = sizeof(MOUNTDEV_NAME) + output->NameLength;
        ExFreePool(output);
        output = ExAllocatePool(PagedPool, outputSize);
        if (!output) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        status = execute_ioctl(Target, IOCTL_MOUNTDEV_QUERY_DEVICE_NAME, NULL, 0, output, outputSize, NULL);
    }

    if (!NT_SUCCESS(status)) {
        ExFreePool(output);
        return status;
    }
    else{
        *name = output;
    }
    return status;
}

NTSTATUS query_volume_number(IN PDEVICE_OBJECT Target, VOLUME_NUMBER* volume_number){
    RtlZeroMemory(volume_number, sizeof(VOLUME_NUMBER));
    return execute_ioctl(Target, IOCTL_VOLUME_QUERY_VOLUME_NUMBER, NULL, 0, volume_number, sizeof(VOLUME_NUMBER), NULL);
}

NTSTATUS get_volume_length(IN PDEVICE_OBJECT Target, LARGE_INTEGER* volume_length){
    GET_LENGTH_INFORMATION length;
    RtlZeroMemory(&length, sizeof(GET_LENGTH_INFORMATION));
    NTSTATUS status = execute_ioctl(Target, IOCTL_DISK_GET_LENGTH_INFO, NULL, 0, &length, sizeof(GET_LENGTH_INFORMATION), NULL);
    if (NT_SUCCESS(status))
        volume_length->QuadPart = length.Length.QuadPart;
    return status;
}

bool is_volume_device(PDEVICE_OBJECT Target){
    VOLUME_NUMBER volNumber;
    VOLUME_DISK_EXTENTS extents[2];
    NTSTATUS extentStatus = execute_ioctl(Target, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, NULL, 0, extents, sizeof(extents), NULL);
    return NT_SUCCESS(execute_ioctl(Target, IOCTL_VOLUME_SUPPORTS_ONLINE_OFFLINE, NULL, 0, NULL, 0, NULL))
        || NT_SUCCESS(execute_ioctl(Target, IOCTL_VOLUME_IS_OFFLINE, NULL, 0, NULL, 0, NULL))
        || NT_SUCCESS(execute_ioctl(Target, IOCTL_VOLUME_IS_IO_CAPABLE, NULL, 0, NULL, 0, NULL))
        || NT_SUCCESS(execute_ioctl(Target, IOCTL_VOLUME_IS_PARTITION, NULL, 0, NULL, 0, NULL))
        || NT_SUCCESS(execute_ioctl(Target, IOCTL_VOLUME_QUERY_VOLUME_NUMBER, NULL, 0, &volNumber, sizeof(volNumber), NULL))
        || NT_SUCCESS(extentStatus) || extentStatus == STATUS_BUFFER_OVERFLOW || extentStatus == STATUS_BUFFER_TOO_SMALL;
}

NTSTATUS get_sector_size(IN PDEVICE_OBJECT Target, DWORD* BytesPerSector){
    DISK_GEOMETRY geometry;
    RtlZeroMemory(&geometry, sizeof(DISK_GEOMETRY));
    NTSTATUS status = execute_ioctl(Target, IOCTL_DISK_GET_DRIVE_GEOMETRY, NULL, 0, &geometry, sizeof(DISK_GEOMETRY), NULL);
    if (NT_SUCCESS(status))
        *BytesPerSector = geometry.BytesPerSector;
    return status;
}