
#include "trace.h"
#include "vcbt.h"
#include "ioctl.h"
#include "ntddscsi.h"
#include "config.h"
#include "vcbt.tmh"
#include "evtmsg.h"
#include "fat.h"

void Sleep(unsigned ms);
NTSTATUS ReadUmapData(PDEVICE_EXTENSION pdx, BYTE* buff, LONGLONG offset, LONG length);
NTSTATUS ReadJournalMetaData(PDEVICE_EXTENSION pdx, BYTE* buff, LONGLONG offset, LONG length);
NTSTATUS ReadFromJournalData(PDEVICE_EXTENSION pdx, BYTE* buff, LONGLONG offset, LONG length);
NTSTATUS ProtectJournalFilesClusters(PDEVICE_EXTENSION pdx);

extern PSHORT               NtBuildNumber;
PDEVICE_OBJECT              _ControlDeviceObject = NULL;
PCONTROL_DEVICE_EXTENSION	_ControlDeviceExtension = NULL;
ULONG RtlRandomEx(
    _Inout_ PULONG Seed
    );

static ULONG GetStringSize(IN PWSTR String){
    UNICODE_STRING TempString;
    RtlInitUnicodeString(&TempString, String);
    return TempString.Length + sizeof(WCHAR); /* add in the terminating NULL */
}

VOID LogEvent( 
    IN PDEVICE_OBJECT IoObject,
    IN NTSTATUS ErrorCode,
    IN ULONG UniqueErrorValue,
    IN ULONG DumpData[],
    IN ULONG DumpDataCount,
    IN PWSTR Strings[],
    IN ULONG StringCount
    )
{

#define MAX_INSERTION_STRINGS 16
    
    PIO_ERROR_LOG_PACKET Packet;
    PUCHAR pInsertionString;
    UCHAR PacketSize;
    UCHAR StringSize[MAX_INSERTION_STRINGS];
    ULONG i;

    // Start with minimum required packet size
    PacketSize = sizeof(IO_ERROR_LOG_PACKET);

    // Add in any dump data. Remember that the standard error log packet 
    // already has one slot in its DumpData array.
    if (DumpDataCount > 0)
        PacketSize += (UCHAR)(sizeof(ULONG) * (DumpDataCount - 1));

    // Determine total space needed for any insertion strings.
    if (StringCount > 0){
        // Take the lesser of what the caller sent and what this routine can handle
        if (StringCount > MAX_INSERTION_STRINGS)
            StringCount = MAX_INSERTION_STRINGS;

        for (i = 0; i < StringCount; i++){
            // Keep track of individual string sizes
            StringSize[i] = (UCHAR)GetStringSize(Strings[i]);
            PacketSize += StringSize[i];
        }
    }

    // Try to allocate the packet
    Packet = IoAllocateErrorLogEntry(IoObject, PacketSize);

    if (Packet == NULL)
        return;
    // Fill in standard parts of the packet
    Packet->ErrorCode = ErrorCode;
    Packet->UniqueErrorValue = UniqueErrorValue;
    Packet->MajorFunctionCode = 0;
    Packet->RetryCount = 0;
    Packet->FinalStatus = 0;
    Packet->SequenceNumber = 0;
    Packet->IoControlCode = 0;

    // Add the dump data
    if (DumpDataCount > 0){
        Packet->DumpDataSize = (USHORT)(sizeof(ULONG) * DumpDataCount);
        for (i = 0; i < DumpDataCount; i++)
            Packet->DumpData[i] = DumpData[i];
    }
    else
        Packet->DumpDataSize = 0;

    // Add the insertion strings
    Packet->NumberOfStrings = (USHORT)StringCount;
    if (StringCount > 0){
        // The strings always go just after the DumpData array in the packet
        Packet->StringOffset =
            (USHORT)(sizeof(IO_ERROR_LOG_PACKET) + DumpDataCount *
            sizeof(ULONG) - sizeof(ULONG));
        pInsertionString = (PUCHAR)Packet + Packet->StringOffset;
        for (i = 0; i < StringCount; i++){
            // Add each new string to the end of the existing stuff
            RtlCopyBytes(pInsertionString, Strings[i], StringSize[i]);
            pInsertionString += StringSize[i];
        }
    }
    // Log the message
    IoWriteErrorLogEntry(Packet);
}

VOID LogEventMessage(
    IN PDEVICE_OBJECT IoObject,
    IN NTSTATUS ErrorCode,
    IN LONG     StringCount,
    ...){
    PWSTR Strings[MAX_INSERTION_STRINGS];
    memset(Strings, 0, sizeof(Strings));
    va_list ap;
    va_start(ap, StringCount);
    for (int i = 0; i<StringCount; i++){
        Strings[i] = va_arg(ap, PWSTR);
    }
    va_end(ap);
    LogEvent(IoObject, ErrorCode, 0, NULL, 0, Strings, StringCount);
}

PDEVICE_OBJECT
CreateControlDevice(
IN PDRIVER_OBJECT DriverObject,
IN PUNICODE_STRING RegistryPath
){
    NTSTATUS			            status;
    PDEVICE_OBJECT		            deviceObject = NULL;
    BOOLEAN				            symbolicLink = FALSE;
    UNICODE_STRING		            ntDeviceName;
    PCONTROL_DEVICE_EXTENSION	    deviceExtension;
    UNICODE_STRING		            dosDeviceName;

    UNREFERENCED_PARAMETER(RegistryPath);
    
    RtlInitUnicodeString(&ntDeviceName, VCBT_DEVICE_NAME_W);

    status = IoCreateDevice(
        DriverObject,
        sizeof(CONTROL_DEVICE_EXTENSION),		// DeviceExtensionSize
        &ntDeviceName,					        // DeviceName
        VCBT_DEVICE_FLT,			            // DeviceType
        0,								        // DeviceCharacteristics
        TRUE,							        // Exclusive 
        &deviceObject					        // [OUT]
        );

    if (!NT_SUCCESS(status)){
        DoTraceMsg(TRACE_LEVEL_ERROR, "IoCreateDevice failed(0x%x)", status);
    }
    else{
        deviceExtension = (PCONTROL_DEVICE_EXTENSION)deviceObject->DeviceExtension;
        RtlInitUnicodeString(&dosDeviceName, VCBT_DOS_DEVICE_NAME_W);
        status = IoCreateSymbolicLink(&dosDeviceName, &ntDeviceName);
        if (!NT_SUCCESS(status)){
            DoTraceMsg(TRACE_LEVEL_ERROR, "IoCreateSymbolicLink failed(0x%x)", status);
        }
        else{
            
            InitializeListHead(&deviceExtension->DeviceList);
            lock_init(&deviceExtension->DeviceListLock);
            deviceExtension->SystemProcessId = (ULONG)PsGetCurrentProcessId();
            deviceExtension->JournalDisabled = is_only_enable_umap(RegistryPath);
            deviceExtension->BatchUpdateJournalMetaData = is_batch_update_journal_meta_data(RegistryPath);
            deviceExtension->UmapFlushDisabled = is_umap_flush_disabled(RegistryPath);
            deviceExtension->VerboseDebug = is_verbose_debug(RegistryPath);
            deviceExtension->DiskCopyDataDisabled = is_disk_copy_data_disabled(RegistryPath);
            _ControlDeviceExtension = deviceExtension;
            LogEventMessage(deviceObject, VCBT_DRIVER_LOAD_SUCCESS, 0);
            
            // Can't find the pagefile.sys handle during os booting. Comment out this action. 
             //IoRegisterBootDriverReinitialization(
             //   DriverObject,
             //   BootDriverReinitializationRoutine,
             //   NULL
             //   );

            if (NT_SUCCESS(status))
                return deviceObject;
        }
    }

    if (symbolicLink)
        IoDeleteSymbolicLink(&dosDeviceName);

    if (deviceObject)
        IoDeleteDevice(deviceObject);

    return deviceObject;
}

NTSTATUS
DriverEntry(
IN PDRIVER_OBJECT DriverObject,
IN PUNICODE_STRING RegistryPath
)

/*++

Routine Description:

Installable driver initialization entry point.
This entry point is called directly by the I/O manager to set up the disk
performance driver. The driver object is set up and then the Pnp manager
calls VcbtAddDevice to attach to the boot devices.

Arguments:

DriverObject - The disk performance driver object.

RegistryPath - pointer to a unicode string representing the path,
to driver-specific key in the registry.

Return Value:

STATUS_SUCCESS if successful

--*/

{
    //
    // Initialize WPP Tracing
    //
    WPP_INIT_TRACING(DriverObject, RegistryPath);

    DoTraceMsg(TRACE_LEVEL_INFORMATION,
        ("DriverEntry: Entered\n"));

    ULONG               ulIndex;
    PDRIVER_DISPATCH  * dispatch;

    //
    // Remember registry path
    //

    VcbtRegistryPath.MaximumLength = RegistryPath->Length
        + sizeof(UNICODE_NULL);
    VcbtRegistryPath.Buffer = ExAllocatePool(
        PagedPool,
        VcbtRegistryPath.MaximumLength);
    if (VcbtRegistryPath.Buffer != NULL)
    {
        RtlCopyUnicodeString(&VcbtRegistryPath, RegistryPath);
    }
    else {
        VcbtRegistryPath.Length = 0;
        VcbtRegistryPath.MaximumLength = 0;
    }

    //
    // Create dispatch points
    //
    for (ulIndex = 0, dispatch = DriverObject->MajorFunction;
        ulIndex <= IRP_MJ_MAXIMUM_FUNCTION;
        ulIndex++, dispatch++) {

        *dispatch = VcbtSendToNextDriver;
    }
    
    update_driver_version_info(RegistryPath);

    if (!is_disable_vcbt()){
        //
        // Set up the device driver entry points.
        //
        DriverObject->MajorFunction[IRP_MJ_CREATE] = VcbtCreate;
        //  DriverObject->MajorFunction[IRP_MJ_READ] = VcbtReadWrite;
        DriverObject->MajorFunction[IRP_MJ_WRITE] = VcbtReadWrite;
        DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = VcbtDeviceControl;

        DriverObject->MajorFunction[IRP_MJ_SHUTDOWN] = VcbtShutdownFlush;
        DriverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS] = VcbtShutdownFlush;
        DriverObject->MajorFunction[IRP_MJ_PNP] = VcbtDispatchPnp;
        DriverObject->MajorFunction[IRP_MJ_POWER] = VcbtDispatchPower;

        DriverObject->DriverExtension->AddDevice = VcbtAddDevice;
        DriverObject->DriverUnload = VcbtUnload;

        _ControlDeviceObject = CreateControlDevice(DriverObject, RegistryPath);
    }
    else{
        DoTraceMsg(TRACE_LEVEL_WARNING, "VCBT Filter Driver is disabled by BCD Boot Option.");
    }
    return(STATUS_SUCCESS);

} // end DriverEntry()

#define FILTER_DEVICE_PROPOGATE_FLAGS            0
#define FILTER_DEVICE_PROPOGATE_CHARACTERISTICS (FILE_REMOVABLE_MEDIA |  \
                                                 FILE_READ_ONLY_DEVICE | \
                                                 FILE_FLOPPY_DISKETTE    \
                                                 )

VOID
VcbtSyncFilterWithTarget(
IN PDEVICE_OBJECT FilterDevice,
IN PDEVICE_OBJECT TargetDevice
)
{
    ULONG                   propFlags;

    PAGED_CODE();

    //
    // Propogate all useful flags from target to diskperf. MountMgr will look
    // at the diskperf object capabilities to figure out if the disk is
    // a removable and perhaps other things.
    //
    propFlags = TargetDevice->Flags & FILTER_DEVICE_PROPOGATE_FLAGS;
    FilterDevice->Flags |= propFlags;

    propFlags = TargetDevice->Characteristics & FILTER_DEVICE_PROPOGATE_CHARACTERISTICS;
    FilterDevice->Characteristics |= propFlags;


}

NTSTATUS
VcbtAddDevice(
IN PDRIVER_OBJECT DriverObject,
IN PDEVICE_OBJECT PhysicalDeviceObject
)
/*++
Routine Description:

Creates and initializes a new filter device object FiDO for the
corresponding PDO.  Then it attaches the device object to the device
stack of the drivers for the device.

Arguments:

DriverObject - Disk performance driver object.
PhysicalDeviceObject - Physical Device Object from the underlying layered driver

Return Value:

NTSTATUS
--*/

{
    NTSTATUS                status;
    PDEVICE_OBJECT          filterDeviceObject;
    PDEVICE_EXTENSION       deviceExtension;

    PAGED_CODE();

    //
    // Create a filter device object for this device (partition).
    //

    DoTraceMsg(2, "VcbtAddDevice: Driver %p Device %p",
        DriverObject, PhysicalDeviceObject);

    status = IoCreateDevice(DriverObject,
        DEVICE_EXTENSION_SIZE,
        NULL,
        FILE_DEVICE_DISK,
        FILE_DEVICE_SECURE_OPEN,
        FALSE,
        &filterDeviceObject);

    if (!NT_SUCCESS(status)) {
        DoTraceMsg(1, "VcbtAddDevice: Cannot create filterDeviceObject");
        return status;
    }

    filterDeviceObject->Flags |= DO_DIRECT_IO;

    deviceExtension = (PDEVICE_EXTENSION)filterDeviceObject->DeviceExtension;

    RtlZeroMemory(deviceExtension, DEVICE_EXTENSION_SIZE);
 

    //
    // Attaches the device object to the highest device object in the chain and
    // return the previously highest device object, which is passed to
    // IoCallDriver when pass IRPs down the device stack
    //

    deviceExtension->PhysicalDeviceObject = PhysicalDeviceObject;

    deviceExtension->TargetDeviceObject =
        IoAttachDeviceToDeviceStack(filterDeviceObject, PhysicalDeviceObject);

    if (deviceExtension->TargetDeviceObject == NULL) {
        IoDeleteDevice(filterDeviceObject);
        DoTraceMsg(1, "VcbtAddDevice: Unable to attach %p to target %p",
            filterDeviceObject, PhysicalDeviceObject);
        return STATUS_NO_SUCH_DEVICE;
    }

    //
    // Save the filter device object in the device extension
    //
    deviceExtension->DeviceObject = filterDeviceObject;

    deviceExtension->PhysicalDeviceName.Buffer
        = deviceExtension->PhysicalDeviceNameBuffer;

    KeInitializeEvent(&deviceExtension->PagingPathCountEvent,
        NotificationEvent, TRUE);

    /*
    The system also provides atomic versions of the list operations, 
    ExInterlockedInsertHeadList, 
    ExInterlockedInsertTailList, 
    and ExInterlockedRemoveHeadList. 
    CONTAINING_RECORD(temp,MYSTRUCT,list_entry)->num1 in the source code would refer to num1 of structure MYSTRUCT.  
    */
    
    InitializeListHead(&deviceExtension->QueueList);
    KeInitializeSpinLock(&deviceExtension->QueueLock);
    InitializeListHead(&deviceExtension->ExcludedList);
    lock_init(&deviceExtension->ExcludedLock);

    lock_acquire(&_ControlDeviceExtension->DeviceListLock);
    InsertTailList(&_ControlDeviceExtension->DeviceList, &deviceExtension->Bind);
    lock_release(&_ControlDeviceExtension->DeviceListLock);
    //
    // default to DO_POWER_PAGABLE
    //

    filterDeviceObject->Flags |= DO_POWER_PAGABLE;

    //
    // Clear the DO_DEVICE_INITIALIZING flag
    //

    filterDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    return STATUS_SUCCESS;

} // end VcbtAddDevice()


NTSTATUS
VcbtDispatchPnp(
IN PDEVICE_OBJECT DeviceObject,
IN PIRP Irp
)
/*++

Routine Description:

Dispatch for PNP

Arguments:

DeviceObject    - Supplies the device object.

Irp             - Supplies the I/O request packet.

Return Value:

NTSTATUS

--*/

{
    PIO_STACK_LOCATION  irpSp = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS            status;
    PDEVICE_EXTENSION   deviceExtension;

    PAGED_CODE();

    //DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDispatchPnp: Device %p Irp %p",
    //    DeviceObject, Irp);

    switch (irpSp->MinorFunction) {

    case IRP_MN_START_DEVICE:
        //
        // Call the Start Routine handler to schedule a completion routine
        //
        DoTraceMsg(3,
            "VcbtDispatchPnp: Schedule completion for START_DEVICE - DeviceObject %p, Irp %p", DeviceObject, Irp);
        status = VcbtStartDevice(DeviceObject, Irp);
        break;

    case IRP_MN_REMOVE_DEVICE:
    {
        //
        // Call the Remove Routine handler to schedule a completion routine
        //
        DoTraceMsg(3,
            "VcbtDispatchPnp: Schedule completion for REMOVE_DEVICE - DeviceObject %p, Irp %p", DeviceObject, Irp);
        status = VcbtRemoveDevice(DeviceObject, Irp);
        break;
    }
    case IRP_MN_DEVICE_USAGE_NOTIFICATION:
    {
        PIO_STACK_LOCATION irpStack;
        BOOLEAN setPagable;

        DoTraceMsg(3,
            "VcbtDispatchPnp: Processing DEVICE_USAGE_NOTIFICATION - DeviceObject %p, Irp %p", DeviceObject, Irp);
        irpStack = IoGetCurrentIrpStackLocation(Irp);

        if (irpStack->Parameters.UsageNotification.Type != DeviceUsageTypePaging) {
            status = VcbtSendToNextDriver(DeviceObject, Irp);
            break; // out of case statement
        }

        deviceExtension = DeviceObject->DeviceExtension;

        //
        // wait on the paging path event
        //

        status = KeWaitForSingleObject(&deviceExtension->PagingPathCountEvent,
            Executive, KernelMode,
            FALSE, NULL);

        //
        // if removing last paging device, need to set DO_POWER_PAGABLE
        // bit here, and possible re-set it below on failure.
        //

        setPagable = FALSE;
        if (!irpStack->Parameters.UsageNotification.InPath &&
            deviceExtension->PagingPathCount == 1) {

            //
            // removing the last paging file
            // must have DO_POWER_PAGABLE bits set
            //

            if (DeviceObject->Flags & DO_POWER_INRUSH) {
                DoTraceMsg(3, "VcbtDispatchPnp: last paging file "
                    "removed but DO_POWER_INRUSH set, so not "
                    "setting PAGABLE bit "
                    "for DO %p", DeviceObject);
            }
            else {
                DoTraceMsg(2, "VcbtDispatchPnp: Setting  PAGABLE "
                    "bit for DO %p", DeviceObject);
                DeviceObject->Flags |= DO_POWER_PAGABLE;
                setPagable = TRUE;
            }

        }

        //
        // send the irp synchronously
        //

        status = VcbtForwardIrpSynchronous(DeviceObject, Irp);

        //
        // now deal with the failure and success cases.
        // note that we are not allowed to fail the irp
        // once it is sent to the lower drivers.
        //

        if (NT_SUCCESS(status)) {

            IoAdjustPagingPathCount(
                &deviceExtension->PagingPathCount,
                irpStack->Parameters.UsageNotification.InPath);

            if (irpStack->Parameters.UsageNotification.InPath) {
                if (deviceExtension->PagingPathCount == 1) {

                    //
                    // first paging file addition
                    //

                    DoTraceMsg(3, "VcbtDispatchPnp: Clearing PAGABLE bit "
                        "for DO %p", DeviceObject);
                    DeviceObject->Flags &= ~DO_POWER_PAGABLE;
                }
            }

        }
        else {

            //
            // cleanup the changes done above
            //

            if (setPagable == TRUE) {
                DeviceObject->Flags &= ~DO_POWER_PAGABLE;
                setPagable = FALSE;
            }

        }

        //
        // set the event so the next one can occur.
        //

        KeSetEvent(&deviceExtension->PagingPathCountEvent,
            IO_NO_INCREMENT, FALSE);

        //
        // and complete the irp
        //

        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;
        break;

    }

    default:
        //DoTraceMsg(TRACE_LEVEL_VERBOSE,
        //    "VcbtDispatchPnp: Forwarding irp - DeviceObject %p, Irp %p", DeviceObject, Irp);
        //
        // Simply forward all other Irps
        //
        return VcbtSendToNextDriver(DeviceObject, Irp);

    }

    return status;

} // end VcbtDispatchPnp()


NTSTATUS
VcbtIrpCompletion(
IN PDEVICE_OBJECT DeviceObject,
IN PIRP Irp,
IN PVOID Context
)

/*++

Routine Description:

Forwarded IRP completion routine. Set an event and return
STATUS_MORE_PROCESSING_REQUIRED. Irp forwarder will wait on this
event and then re-complete the irp after cleaning up.

Arguments:

DeviceObject is the device object of the WMI driver
Irp is the WMI irp that was just completed
Context is a PKEVENT that forwarder will wait on

Return Value:

STATUS_MORE_PORCESSING_REQUIRED

--*/

{
    PKEVENT Event = (PKEVENT)Context;

    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Irp);

    KeSetEvent(Event, IO_NO_INCREMENT, FALSE);

    return(STATUS_MORE_PROCESSING_REQUIRED);

} // end VcbtIrpCompletion()


NTSTATUS
VcbtStartDevice(
IN PDEVICE_OBJECT DeviceObject,
IN PIRP Irp
)
/*++

Routine Description:

This routine is called when a Pnp Start Irp is received.
It will schedule a completion routine to initialize and register with WMI.

Arguments:

DeviceObject - a pointer to the device object

Irp - a pointer to the irp


Return Value:

Status of processing the Start Irp

--*/
{
    PDEVICE_EXTENSION   deviceExtension;
    NTSTATUS            status;

    PAGED_CODE();

    deviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;

    status = VcbtForwardIrpSynchronous(DeviceObject, Irp);

    VcbtSyncFilterWithTarget(DeviceObject,
        deviceExtension->TargetDeviceObject);

    //
    // Complete WMI registration
    //
    VcbtRegisterDevice(DeviceObject);

    //
    // Complete the Irp
    //
    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return status;
}


NTSTATUS
VcbtRemoveDevice(
IN PDEVICE_OBJECT DeviceObject,
IN PIRP Irp
)
/*++

Routine Description:

This routine is called when the device is to be removed.
It will de-register itself from WMI first, detach itself from the
stack before deleting itself.

Arguments:

DeviceObject - a pointer to the device object

Irp - a pointer to the irp


Return Value:

Status of removing the device

--*/
{
    NTSTATUS            status;
    PDEVICE_EXTENSION   deviceExtension;

    PAGED_CODE();

    deviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
    
    lock_acquire(&_ControlDeviceExtension->DeviceListLock);
    RemoveEntryList(&deviceExtension->Bind);
    lock_release(&_ControlDeviceExtension->DeviceListLock);

    StopThread(deviceExtension);
    status = VcbtForwardIrpSynchronous(DeviceObject, Irp);

    IoDetachDevice(deviceExtension->TargetDeviceObject);
    IoDeleteDevice(DeviceObject);
    lock_acquire(&deviceExtension->ExcludedLock);
    LIST_ENTRY* entry;
    while (&deviceExtension->ExcludedList != (entry = RemoveHeadList(&deviceExtension->ExcludedList))){
        PIO_EXCLUDE_RANGE range = CONTAINING_RECORD(entry, IO_EXCLUDE_RANGE, Bind);
        ExFreePool(range);
    }
    lock_release(&deviceExtension->ExcludedLock);

    //
    // Complete the Irp
    //
    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return status;
}

NTSTATUS
VcbtSendToNextDriver(
IN PDEVICE_OBJECT DeviceObject,
IN PIRP Irp
)

/*++

Routine Description:

This routine sends the Irp to the next driver in line
when the Irp is not processed by this driver.

Arguments:

DeviceObject
Irp

Return Value:

NTSTATUS

--*/

{
    PDEVICE_EXTENSION   deviceExtension;
    if (_ControlDeviceObject == DeviceObject){
        Irp->IoStatus.Status = STATUS_SUCCESS;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_SUCCESS;
    }
    IoSkipCurrentIrpStackLocation(Irp);
    deviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;

    return IoCallDriver(deviceExtension->TargetDeviceObject, Irp);

} // end VcbtSendToNextDriver()

NTSTATUS
VcbtDispatchPower(
IN PDEVICE_OBJECT DeviceObject,
IN PIRP Irp
)
{
    PDEVICE_EXTENSION deviceExtension;
    if (RtlIsNtDdiVersionAvailable(NTDDI_VISTA)){
        IoSkipCurrentIrpStackLocation(Irp);
        deviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
        return IoCallDriver(deviceExtension->TargetDeviceObject, Irp);
    }
    else{
        PoStartNextPowerIrp(Irp);
        IoSkipCurrentIrpStackLocation(Irp);
        deviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
        return PoCallDriver(deviceExtension->TargetDeviceObject, Irp);
    }
} // end VcbtDispatchPower

NTSTATUS
VcbtForwardIrpSynchronous(
IN PDEVICE_OBJECT DeviceObject,
IN PIRP Irp
)

/*++

Routine Description:

This routine sends the Irp to the next driver in line
when the Irp needs to be processed by the lower drivers
prior to being processed by this one.

Arguments:

DeviceObject
Irp

Return Value:

NTSTATUS

--*/

{
    PDEVICE_EXTENSION   deviceExtension;
    KEVENT event;
    NTSTATUS status;

    KeInitializeEvent(&event, NotificationEvent, FALSE);
    deviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;

    //
    // copy the irpstack for the next device
    //

    IoCopyCurrentIrpStackLocationToNext(Irp);

    //
    // set a completion routine
    //

    IoSetCompletionRoutine(Irp, VcbtIrpCompletion,
        &event, TRUE, TRUE, TRUE);

    //
    // call the next lower device
    //

    status = IoCallDriver(deviceExtension->TargetDeviceObject, Irp);

    //
    // wait for the actual completion
    //

    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = Irp->IoStatus.Status;
    }

    return status;

} // end VcbtForwardIrpSynchronous()


NTSTATUS
VcbtCreate(
IN PDEVICE_OBJECT DeviceObject,
IN PIRP Irp
)

/*++

Routine Description:

This routine services open commands. It establishes
the driver's existance by returning status success.

Arguments:

DeviceObject - Context for the activity.
Irp          - The device control argument block.

Return Value:

NT Status

--*/

{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(DeviceObject);

    Irp->IoStatus.Status = STATUS_SUCCESS;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;

} // end VcbtCreate()

NTSTATUS
VcbtReadWrite(
IN PDEVICE_OBJECT DeviceObject,
IN PIRP Irp
)

/*++

Routine Description:

This is the driver entry point for read and write requests
to disks to which the diskperf driver has attached.
This driver collects statistics and then sets a completion
routine so that it can collect additional information when
the request completes. Then it calls the next driver below
it.

Arguments:

DeviceObject
Irp

Return Value:

NTSTATUS

--*/

{

    PDEVICE_EXTENSION  deviceExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION currentIrpStack = IoGetCurrentIrpStackLocation(Irp);
    bool               passThrough = (!deviceExtension->Enabled) || (currentIrpStack->MajorFunction == IRP_MJ_READ) || ( PsGetCurrentThreadId() == deviceExtension->ThreadId.UniqueThread );

    if (_ControlDeviceObject == DeviceObject){
        Irp->IoStatus.Status = STATUS_SUCCESS;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_SUCCESS;
    }
    
    if (!passThrough) {       
        lock_acquire(&deviceExtension->ExcludedLock);
        LIST_ENTRY* entry;
        LIST_FOR_EACH(entry, &deviceExtension->ExcludedList){
            PIO_EXCLUDE_RANGE range = CONTAINING_RECORD(entry, IO_EXCLUDE_RANGE, Bind);
            LONGLONG End = currentIrpStack->Parameters.Write.ByteOffset.QuadPart + currentIrpStack->Parameters.Write.Length;
            if ( range->Start > currentIrpStack->Parameters.Write.ByteOffset.QuadPart &&
                 range->Start < End &&
                 range->End <= End ){
                DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtReadWrite: Write excluded I/O- start %I64u, length %d - DeviceObject %p, Irp %p",
                    currentIrpStack->Parameters.Write.ByteOffset.QuadPart, currentIrpStack->Parameters.Write.Length, DeviceObject, Irp);
                passThrough = true;
                break;
            }
        }
        lock_release(&deviceExtension->ExcludedLock);
    }

#if 0
    if (deviceExtension->Enabled && currentIrpStack->MajorFunction == IRP_MJ_WRITE){
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtReadWrite: Write - start %I64u, length %d - DeviceObject %p, Irp %p",
            currentIrpStack->Parameters.Write.ByteOffset.QuadPart, currentIrpStack->Parameters.Write.Length, DeviceObject, Irp);
    }
    //if (PsGetCurrentThreadId() == deviceExtension->ThreadId.UniqueThread)
    if (passThrough){
        if (currentIrpStack->MajorFunction == IRP_MJ_WRITE){
            DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtReadWrite: Write - start %I64u, length %d - DeviceObject %p, Irp %p",
                currentIrpStack->Parameters.Write.ByteOffset.QuadPart, currentIrpStack->Parameters.Write.Length, DeviceObject, Irp);
        }
        else{
            DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtReadWrite: Read - start %I64u, length %d - DeviceObject %p, Irp %p",
                currentIrpStack->Parameters.Read.ByteOffset.QuadPart, currentIrpStack->Parameters.Read.Length, DeviceObject, Irp);
        }
    }
#endif
    //
    // Device is not initialized properly. Blindly pass the irp along
    //
#if 1
    if (!passThrough){    
        IoMarkIrpPending(Irp);
        ExInterlockedInsertTailList(&deviceExtension->QueueList, &Irp->Tail.Overlay.ListEntry, &deviceExtension->QueueLock);
        KeSetEvent(&deviceExtension->QueueEvent, 0, FALSE);
        return STATUS_PENDING;
    }
    
    return VcbtSendToNextDriver(DeviceObject, Irp);
#else
    //
    // Copy current stack to next stack.
    //

    IoCopyCurrentIrpStackLocationToNext(Irp);

    //
    // Set completion routine callback.
    //

    IoSetCompletionRoutine(Irp,
        VcbtIoCompletion,
        DeviceObject,
        TRUE,
        TRUE,
        TRUE);

    //
    // Return the results of the call to the disk driver.
    //

    return IoCallDriver(deviceExtension->TargetDeviceObject,
        Irp);
#endif

} // end VcbtReadWrite()

NTSTATUS
VcbtIoCompletion(
IN PDEVICE_OBJECT DeviceObject,
IN PIRP           Irp,
IN PVOID          Context
)

/*++

Routine Description:

This routine will get control from the system at the completion of an IRP.
It will calculate the difference between the time the IRP was started
and the current time, and decrement the queue depth.

Arguments:

DeviceObject - for the IRP.
Irp          - The I/O request that just completed.
Context      - Not used.

Return Value:

The IRP status.

--*/

{
   // PDEVICE_EXTENSION  deviceExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);

    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Context);

    if (Irp->PendingReturned) {
        IoMarkIrpPending(Irp);
    }

    if (irpStack->MajorFunction == IRP_MJ_READ) {

       
    }
    else {

     
    }

    if (Irp->Flags & IRP_ASSOCIATED_IRP) {
    }

    return STATUS_SUCCESS;

} // VcbtIoCompletion

NTSTATUS VcbtGetRunTimeInfo(PDEVICE_OBJECT DeviceObject, PIRP Irp, PDEVICE_EXTENSION deviceExtension, ULONG outputBufferLength, PVOID ioBuffer, ULONG *resultLength){
    PVCBT_RUNTIME_COMMAND command            = (PVCBT_RUNTIME_COMMAND)ioBuffer;
    NTSTATUS              status             = STATUS_SUCCESS;
    bool                  need_free          = false;
    ULONG                 resultBufferLength = sizeof(VCBT_RUNTIME_RESULT);
    PRETRIEVAL_POINTERS_BUFFER  retrievalPointers = NULL;
    switch (command->Flag){
    case JOURNAL:
        retrievalPointers = deviceExtension->Journal->JournalRetrievalPointers;
        break;
    case JOURNAL_META:
        retrievalPointers = deviceExtension->Journal->JournalMetaDataRetrievalPointers;
        break;
    case UMAP:
        retrievalPointers = deviceExtension->Journal->UmapRetrievalPointers;
        break;
    case JOURNAL_FILE:
        status = get_file_retrieval_pointers(deviceExtension->Journal->JournalFileHandle, 0, &retrievalPointers);
        need_free = true;
        break;
    case JOURNAL_META_FILE:
        status = get_file_retrieval_pointers(deviceExtension->Journal->JournalMetaDataFileHandle, 0, &retrievalPointers);
        need_free = true;
        break;
    case UMAP_FILE:
        status = get_file_retrieval_pointers(deviceExtension->Journal->UmapFileHandle, 0, &retrievalPointers);
        need_free = true;
        break;
    }
    if (NULL != retrievalPointers){
        resultBufferLength += ((retrievalPointers->ExtentCount - 1) * sizeof(LARGE_INTEGER) * 2);
    }
    if (outputBufferLength >= resultBufferLength){
        PVCBT_RUNTIME_RESULT result = (PVCBT_RUNTIME_RESULT)ioBuffer;
        memset(result, 0, resultBufferLength);
        result->Ready = deviceExtension->Journal->Ready;
        result->Check = deviceExtension->Journal->Check;
        result->Initialized = deviceExtension->Journal->Initialized;
        result->FsClusterSize = deviceExtension->Journal->FsClusterSize;
        result->BytesPerSector = deviceExtension->Journal->BytesPerSector;
        result->Resolution = deviceExtension->Journal->Resolution;
        result->JournalViewSize = deviceExtension->Journal->JournalViewSize;
        result->JournalViewOffset = deviceExtension->Journal->JournalViewOffset;
        result->FileAreaOffset = deviceExtension->Journal->FileAreaOffset;
        memcpy(&result->JournalMetaData, &deviceExtension->Journal->JournalMetaData, sizeof(VCBT_JOURNAL_META_DATA));
        if (NULL != retrievalPointers){
            memcpy(&result->RetrievalPointers, retrievalPointers, sizeof(RETRIEVAL_POINTERS_BUFFER) + ((retrievalPointers->ExtentCount - 1) * sizeof(LARGE_INTEGER) * 2));
        }
        status = STATUS_SUCCESS;
    }
    else {
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_VCBT_RUNTIME(STATUS_BUFFER_TOO_SMALL), Size: %d - DeviceObject %p, Irp %p", resultBufferLength, DeviceObject, Irp);
        status = STATUS_BUFFER_TOO_SMALL;
    }
    if (need_free){
        __free(retrievalPointers);
    }
    *resultLength = resultBufferLength;
    return status;
}

NTSTATUS VcbtRunCommand(PDEVICE_OBJECT DeviceObject, PIRP Irp, ULONG IoControlCode, PDEVICE_EXTENSION deviceExtension, PVCBT_COMMAND command, PVCOMMAND_RESULT result){
    NTSTATUS              status = STATUS_SUCCESS;
    switch (IoControlCode) {
    case IOCTL_VCBT_ENABLE:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_VCBT_ENABLE - DeviceObject %p, Irp %p", DeviceObject, Irp);
        LONG sizeInMegaBytes = command->Detail.JournalSizeInMegaBytes;
        if (NULL == deviceExtension->Journal){
            if (NT_SUCCESS(status = EnableJournal(deviceExtension, &sizeInMegaBytes, &result->JournalId))){
                result->Detail.JournalSizeInMegaBytes = sizeInMegaBytes;
                if (NT_SUCCESS(status = StartThread(deviceExtension))){

                }
            }
            result->Status = status;
        }
        else{
            result->JournalId = deviceExtension->Journal->JournalMetaData.Block.j.JournalId;
            result->Detail.JournalSizeInMegaBytes = deviceExtension->Journal->JournalMetaData.Block.j.Size >> 20;
            result->Status = STATUS_SUCCESS;
        }
        break;
    case IOCTL_VCBT_DISABLE:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_VCBT_DISABLE - DeviceObject %p, Irp %p", DeviceObject, Irp);
        StopThread(deviceExtension);
        if (NT_SUCCESS(status = DisableJournal(deviceExtension))){
            result->Detail.Done = TRUE;
        }
        result->Status = status;
        break;
    case IOCTL_VCBT_SNAPSHOT:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_VCBT_SNAPSHOT - DeviceObject %p, Irp %p", DeviceObject, Irp);
        if (NT_SUCCESS(status = Snapshot(deviceExtension))){
            result->JournalId = deviceExtension->Journal->JournalMetaData.Block.j.JournalId;
            result->Detail.Done = TRUE;
        }
        result->Status = status;
        break;
    case IOCTL_VCBT_POST_SNAPSHOT:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_VCBT_POST_SNAPSHOT - DeviceObject %p, Irp %p", DeviceObject, Irp);
        if (NT_SUCCESS(status = PostSnapshot(deviceExtension))){
            result->JournalId = deviceExtension->Journal->JournalMetaData.Block.j.JournalId;
            result->Detail.Done = TRUE;
        }
        result->Status = status;
        break;
    case IOCTL_VCBT_UNDO_SNAPSHOT:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_VCBT_UNDO_SNAPSHOT - DeviceObject %p, Irp %p", DeviceObject, Irp);
        if (NT_SUCCESS(status = UndoSnapshot(deviceExtension))){
            result->JournalId = deviceExtension->Journal->JournalMetaData.Block.j.JournalId;
            result->Detail.Done = TRUE;
        }
        result->Status = status;
        break;
    case IOCTL_VCBT_IS_ENABLE:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_VCBT_IS_ENABLE - DeviceObject %p, Irp %p", DeviceObject, Irp);
        result->Detail.Enabled = deviceExtension->Enabled && (NULL != deviceExtension->Journal);
        if (TRUE == result->Detail.Enabled){
            result->JournalId = deviceExtension->Journal->JournalMetaData.Block.j.JournalId;
            result->Detail.JournalSizeInMegaBytes = deviceExtension->Journal->JournalMetaData.Block.j.Size >> 20;
        }
        break;
    }
    return status;
}

NTSTATUS
VcbtDeviceControl(
PDEVICE_OBJECT DeviceObject,
PIRP Irp
)

/*++

Routine Description:

This device control dispatcher handles only the disk performance
device control. All others are passed down to the disk drivers.
The disk performane device control returns a current snapshot of
the performance data.

Arguments:

DeviceObject - Context for the activity.
Irp          - The device control argument block.

Return Value:

Status is returned.

--*/

{
    PIO_STACK_LOCATION currentIrpStack = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS           status = STATUS_SUCCESS;

    if (_ControlDeviceObject == DeviceObject){
        PVOID				ioBuffer;
        ULONG				inputBufferLength, outputBufferLength;
        ioBuffer = Irp->AssociatedIrp.SystemBuffer;
        ULONG               resultBufferLength = 0;
        status = STATUS_NOT_SUPPORTED;
        inputBufferLength = currentIrpStack->Parameters.DeviceIoControl.InputBufferLength;
        outputBufferLength = currentIrpStack->Parameters.DeviceIoControl.OutputBufferLength;
        if (IOCTL_VCBT_RUNTIME == currentIrpStack->Parameters.DeviceIoControl.IoControlCode){          
            PVCBT_RUNTIME_COMMAND command = (PVCBT_RUNTIME_COMMAND)ioBuffer;
            if (inputBufferLength < sizeof(VCBT_RUNTIME_COMMAND)){
                status = STATUS_INVALID_PARAMETER;
            }
            else{
                PDEVICE_EXTENSION  deviceExtension = NULL;
                UNICODE_STRING     VolumeId;
                if (NT_SUCCESS(RtlStringFromGUID(&command->VolumeId, &VolumeId))){
                    PWSTR id = __malloc(VolumeId.MaximumLength);
                    if (id){
                        memset(id, 0, VolumeId.MaximumLength);
                        memcpy(id, VolumeId.Buffer, VolumeId.MaximumLength);
                        lock_acquire(&_ControlDeviceExtension->DeviceListLock);
                        LIST_ENTRY* entry;
                        LIST_FOR_EACH(entry, &_ControlDeviceExtension->DeviceList){
                            PDEVICE_EXTENSION device = CONTAINING_RECORD(entry, DEVICE_EXTENSION, Bind);
                            if (wcsstr(device->MountDeviceLink, id)){
                                deviceExtension = device;
                                break;
                            }
                        }
                        lock_release(&_ControlDeviceExtension->DeviceListLock);
                        __free(id);
                    }
                    RtlFreeUnicodeString(&VolumeId);
                }
                if (NULL == deviceExtension){
                    status = STATUS_NOT_FOUND;
                    DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_VCBT_RUNTIME(STATUS_NOT_FOUND) - DeviceObject %p, Irp %p", DeviceObject, Irp);
                }
                else if (!deviceExtension->Enabled){
                    status = STATUS_INVALID_DEVICE_REQUEST;
                    DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_VCBT_RUNTIME(STATUS_INVALID_DEVICE_REQUEST) - DeviceObject %p, Irp %p", DeviceObject, Irp);
                }
                else if (NULL != deviceExtension->Journal){
                    DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_VCBT_RUNTIME - DeviceObject %p, Irp %p", DeviceObject, Irp);
                    status = VcbtGetRunTimeInfo(DeviceObject, Irp, deviceExtension, outputBufferLength, ioBuffer, &resultBufferLength);
                }
            }
        }
        else{
            PVCBT_COMMAND_INPUT command = (PVCBT_COMMAND_INPUT)ioBuffer;
            if ((command->NumberOfCommands > 0) && (inputBufferLength >= (sizeof(VCBT_COMMAND_INPUT) + (sizeof(VCBT_COMMAND) * (command->NumberOfCommands - 1))))){
                resultBufferLength = sizeof(VCBT_COMMAND_RESULT) + (sizeof(VCOMMAND_RESULT) * (command->NumberOfCommands - 1));
                if (outputBufferLength == 0 || outputBufferLength >= resultBufferLength){
                    PVCBT_COMMAND_RESULT results = (PVCBT_COMMAND_RESULT)__malloc(resultBufferLength);
                    if (NULL == results){
                        status = STATUS_INSUFFICIENT_RESOURCES;
                    }
                    else{
                        memset(results, 0, resultBufferLength);
                        results->NumberOfResults = command->NumberOfCommands;
                        for (LONG i = 0; i < command->NumberOfCommands; i++){
                            memcpy(&results->Results[i].VolumeId, &command->Commands[i].VolumeId, sizeof(GUID));
                            PDEVICE_EXTENSION  deviceExtension = NULL;
                            UNICODE_STRING     VolumeId;
                            if (NT_SUCCESS(RtlStringFromGUID(&results->Results[i].VolumeId, &VolumeId))){
                                PWSTR id = __malloc(VolumeId.MaximumLength);
                                if (id){
                                    memset(id, 0, VolumeId.MaximumLength);
                                    memcpy(id, VolumeId.Buffer, VolumeId.MaximumLength);
                                    lock_acquire(&_ControlDeviceExtension->DeviceListLock);
                                    LIST_ENTRY* entry;
                                    LIST_FOR_EACH(entry, &_ControlDeviceExtension->DeviceList){
                                        PDEVICE_EXTENSION device = CONTAINING_RECORD(entry, DEVICE_EXTENSION, Bind);
                                        if (wcsstr(device->MountDeviceLink, id)){
                                            deviceExtension = device;
                                            break;
                                        }
                                    }
                                    lock_release(&_ControlDeviceExtension->DeviceListLock);
                                    __free(id);
                                }
                                RtlFreeUnicodeString(&VolumeId);
                            }
                            if (NULL == deviceExtension){
                                results->Results[i].Status = STATUS_NOT_FOUND;
                            }
                            else{
                                status = VcbtRunCommand(DeviceObject,
                                    Irp,
                                    currentIrpStack->Parameters.DeviceIoControl.IoControlCode,
                                    deviceExtension,
                                    &command->Commands[i],
                                    &results->Results[i]);
                            }
                        }
                        if (outputBufferLength != 0){
                            memcpy(ioBuffer, results, resultBufferLength);
                        }
                        __free(results);
                        status = STATUS_SUCCESS;
                    }
                }
                else{
                    status = STATUS_BUFFER_TOO_SMALL;
                }
            }
            else{
                status = STATUS_INVALID_PARAMETER;
            }
        }
        Irp->IoStatus.Information = resultBufferLength;
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;
    }
    
    PDEVICE_EXTENSION  deviceExtension = DeviceObject->DeviceExtension;

    switch (currentIrpStack->Parameters.DeviceIoControl.IoControlCode) {

    case IOCTL_VCBT_RUNTIME:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_VCBT_RUNTIME - DeviceObject %p, Irp %p", DeviceObject, Irp);
        if (!deviceExtension->Enabled){
            status = STATUS_INVALID_DEVICE_REQUEST;
            DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_VCBT_RUNTIME(STATUS_INVALID_DEVICE_REQUEST) - DeviceObject %p, Irp %p", DeviceObject, Irp);
        }
        else if (NULL != deviceExtension->Journal){
            DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_VCBT_RUNTIME - DeviceObject %p, Irp %p", DeviceObject, Irp);
            ULONG resultBufferLength = 0;
            status = VcbtGetRunTimeInfo(DeviceObject, 
                Irp, 
                deviceExtension, 
                currentIrpStack->Parameters.DeviceIoControl.OutputBufferLength, 
                Irp->AssociatedIrp.SystemBuffer, 
                &resultBufferLength);
            Irp->IoStatus.Information = resultBufferLength;
        }
        break;
    case IOCTL_VCBT_ENABLE:
    case IOCTL_VCBT_DISABLE:
    case IOCTL_VCBT_SNAPSHOT:
    case IOCTL_VCBT_POST_SNAPSHOT:
    case IOCTL_VCBT_UNDO_SNAPSHOT:
    case IOCTL_VCBT_IS_ENABLE:
        if (currentIrpStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(VCBT_COMMAND_INPUT)){
            status = STATUS_INVALID_PARAMETER;
        }
        else if (currentIrpStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(VCBT_COMMAND_RESULT)){
            status = STATUS_BUFFER_TOO_SMALL;
            Irp->IoStatus.Information = sizeof(VCBT_COMMAND_RESULT);
        }
        else{
            status = VcbtRunCommand(DeviceObject, 
                Irp, 
                currentIrpStack->Parameters.DeviceIoControl.IoControlCode, 
                deviceExtension, 
                Irp->AssociatedIrp.SystemBuffer, 
                Irp->AssociatedIrp.SystemBuffer);
        }
        break;
    case IOCTL_VOLUME_ONLINE: 
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_VOLUME_ONLINE - DeviceObject %p, Irp %p", DeviceObject, Irp);
        StartThread(deviceExtension);
        break;
    case IOCTL_VOLUME_OFFLINE:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_VOLUME_OFFLINE - DeviceObject %p, Irp %p", DeviceObject, Irp);
        StopThread(deviceExtension);
        break;
    case IOCTL_MOUNTDEV_LINK_CREATED:{          
            size_t length = 0;
            if (0 < currentIrpStack->Parameters.DeviceIoControl.InputBufferLength &&
                NT_SUCCESS(RtlStringCchLengthW(deviceExtension->MountDeviceLink, VCBT_MAXSTR, &length))){
                MOUNTDEV_NAME* dev = Irp->AssociatedIrp.SystemBuffer;
                if (dev->NameLength < sizeof(deviceExtension->MountDeviceLink)){
                    if (0 == length){
                        memcpy(deviceExtension->MountDeviceLink, dev->Name, dev->NameLength);
                    }
                    else{
                        if (!(deviceExtension->MountDeviceLink[0] == L'\\' &&
                            deviceExtension->MountDeviceLink[1] == L'?' &&
                            deviceExtension->MountDeviceLink[2] == L'?' &&
                            deviceExtension->MountDeviceLink[3] == L'\\' &&
                            deviceExtension->MountDeviceLink[length - 1] == L'}')){
                            memcpy(deviceExtension->MountDeviceLink, dev->Name, dev->NameLength);
                        }
                    }
                }
                DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_MOUNTDEV_LINK_CREATED (%ws) - DeviceObject %p, Irp %p",deviceExtension->MountDeviceLink, DeviceObject, Irp);
            }
        }
        break;
    case IOCTL_MOUNTDEV_LINK_DELETED:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_MOUNTDEV_LINK_DELETED - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_DISK_MEDIA_REMOVAL:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_DISK_MEDIA_REMOVAL - DeviceObject %p, Irp %p", DeviceObject, Irp);
        if ( NULL != deviceExtension->Journal && (FALSE == deviceExtension->Journal->Check) )
            CheckJournal(deviceExtension);
        break;
    case IOCTL_DISK_COPY_DATA:
    {
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_DISK_COPY_DATA - DeviceObject %p, Irp %p", DeviceObject, Irp);
        if (_ControlDeviceExtension->DiskCopyDataDisabled){
            status = STATUS_NOT_IMPLEMENTED;
            Irp->IoStatus.Status = status;
            Irp->IoStatus.Information = 0;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return (status);
        }
        else{
            IoMarkIrpPending(Irp);
            ExInterlockedInsertTailList(&deviceExtension->QueueList, &Irp->Tail.Overlay.ListEntry, &deviceExtension->QueueLock);
            KeSetEvent(&deviceExtension->QueueEvent, 0, FALSE);
            return STATUS_PENDING;
        }
    }
        break;
#if 0
    case IOCTL_VOLUME_POST_ONLINE:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_VOLUME_POST_ONLINE - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_VOLUME_IS_PARTITION:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_VOLUME_IS_PARTITION - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_VOLUME_PREPARE_FOR_CRITICAL_IO:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_VOLUME_PREPARE_FOR_CRITICAL_IO - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_DISK_GET_CLUSTER_INFO:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_DISK_GET_CLUSTER_INFO - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_VOLUME_QUERY_ALLOCATION_HINT:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_VOLUME_QUERY_ALLOCATION_HINT - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_VOLUME_QUERY_MINIMUM_SHRINK_SIZE:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_VOLUME_QUERY_MINIMUM_SHRINK_SIZE - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_VOLUME_IS_CSV:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_VOLUME_IS_CSV - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case 0x002d1480:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_EHSTOR_BANDMGMT_QUERY_CAPABILITIES - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case 0x002d148c:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_EHSTOR_BANDMGMT_ENUMERATE_BANDS - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_VOLUME_IS_OFFLINE:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_VOLUME_IS_OFFLINE - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_VOLUME_SUPPORTS_ONLINE_OFFLINE:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_VOLUME_SUPPORTS_ONLINE_OFFLINE - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_MOUNTDEV_QUERY_DEVICE_NAME:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_MOUNTDEV_QUERY_DEVICE_NAME - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_STORAGE_MANAGE_DATA_SET_ATTRIBUTES:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_STORAGE_MANAGE_DATA_SET_ATTRIBUTES - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_MOUNTMGR_QUERY_POINTS: 
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_MOUNTMGR_QUERY_POINTS - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_MOUNTDEV_QUERY_UNIQUE_ID:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_MOUNTDEV_QUERY_UNIQUE_ID - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_MOUNTDEV_QUERY_STABLE_GUID:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_MOUNTDEV_QUERY_STABLE_GUID - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_VOLUME_GET_GPT_ATTRIBUTES:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_VOLUME_GET_GPT_ATTRIBUTES - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_VOLUME_IS_DYNAMIC:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_VOLUME_IS_DYNAMIC - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_MOUNTDEV_QUERY_SUGGESTED_LINK_NAME:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_MOUNTDEV_QUERY_SUGGESTED_LINK_NAME - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case 0x704010: // Private dskpart ioctl command
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: 0x704010 - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case 0x2D518C: // Private VHD ioctl command
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: 0x2D518C - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_DISK_VERIFY:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_DISK_VERIFY - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_DISK_FORMAT_TRACKS:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_DISK_FORMAT_TRACKS - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_DISK_REASSIGN_BLOCKS:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_DISK_REASSIGN_BLOCKS - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_DISK_PERFORMANCE:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_DISK_PERFORMANCE - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_DISK_IS_WRITABLE:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_DISK_IS_WRITABLE - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_DISK_LOGGING:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_DISK_LOGGING - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_DISK_FORMAT_TRACKS_EX:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_DISK_FORMAT_TRACKS_EX - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_DISK_HISTOGRAM_STRUCTURE:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_DISK_HISTOGRAM_STRUCTURE - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_DISK_HISTOGRAM_DATA:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_DISK_HISTOGRAM_DATA - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_DISK_HISTOGRAM_RESET:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_DISK_HISTOGRAM_RESET - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_DISK_REQUEST_STRUCTURE:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_DISK_REQUEST_STRUCTURE - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_DISK_REQUEST_DATA:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_DISK_REQUEST_DATA - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_DISK_CONTROLLER_NUMBER:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_DISK_CONTROLLER_NUMBER - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_DISK_UPDATE_DRIVE_SIZE:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_DISK_UPDATE_DRIVE_SIZE - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_DISK_GROW_PARTITION:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_DISK_GROW_PARTITION - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_DISK_GET_CACHE_INFORMATION:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_DISK_GET_CACHE_INFORMATION - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_DISK_SET_CACHE_INFORMATION:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_DISK_SET_CACHE_INFORMATION - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_DISK_DELETE_DRIVE_LAYOUT:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_DISK_DELETE_DRIVE_LAYOUT - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_DISK_FORMAT_DRIVE:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_DISK_FORMAT_DRIVE - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_DISK_SENSE_DEVICE:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_DISK_SENSE_DEVICE - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_DISK_CHECK_VERIFY:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_DISK_CHECK_VERIFY - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_DISK_EJECT_MEDIA:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_DISK_EJECT_MEDIA - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_DISK_LOAD_MEDIA:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_DISK_LOAD_MEDIA - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_DISK_RESERVE:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_DISK_RESERVE - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_DISK_RELEASE:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_DISK_RELEASE - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_DISK_FIND_NEW_DEVICES:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_DISK_FIND_NEW_DEVICES - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_DISK_GET_MEDIA_TYPES:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_DISK_GET_MEDIA_TYPES - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_STORAGE_GET_DEVICE_NUMBER:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_STORAGE_GET_DEVICE_NUMBER - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_STORAGE_GET_MEDIA_TYPES:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_STORAGE_GET_MEDIA_TYPES - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_STORAGE_GET_MEDIA_TYPES_EX:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_STORAGE_GET_MEDIA_TYPES_EX - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_STORAGE_GET_MEDIA_SERIAL_NUMBER:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_STORAGE_GET_MEDIA_SERIAL_NUMBER - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_STORAGE_GET_HOTPLUG_INFO:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_STORAGE_GET_HOTPLUG_INFO - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_STORAGE_QUERY_PROPERTY:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_STORAGE_QUERY_PROPERTY - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_STORAGE_RESET_BUS:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_STORAGE_RESET_BUS - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_STORAGE_RESET_DEVICE:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_STORAGE_RESET_DEVICE - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_STORAGE_PREDICT_FAILURE:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_STORAGE_PREDICT_FAILURE - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_SCSI_PASS_THROUGH:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_SCSI_PASS_THROUGH - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_SCSI_MINIPORT:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_SCSI_MINIPORT - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_SCSI_GET_INQUIRY_DATA:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_SCSI_GET_INQUIRY_DATA - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_SCSI_GET_CAPABILITIES:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_SCSI_GET_CAPABILITIES - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_SCSI_PASS_THROUGH_DIRECT:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_SCSI_PASS_THROUGH_DIRECT - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_SCSI_GET_ADDRESS:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_SCSI_GET_ADDRESS - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_SCSI_RESCAN_BUS:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_SCSI_RESCAN_BUS - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_SCSI_GET_DUMP_POINTERS:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_SCSI_GET_DUMP_POINTERS - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_SCSI_FREE_DUMP_POINTERS:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_SCSI_FREE_DUMP_POINTERS - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_IDE_PASS_THROUGH:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_IDE_PASS_THROUGH - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_DISK_PERFORMANCE_OFF:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_DISK_PERFORMANCE_OFF - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_DISK_CREATE_DISK:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_DISK_CREATE_DISK - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_DISK_UPDATE_PROPERTIES:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_DISK_UPDATE_PROPERTIES - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case OBSOLETE_DISK_GET_WRITE_CACHE_STATE:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: OBSOLETE_DISK_GET_WRITE_CACHE_STATE - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_DISK_GET_CACHE_SETTING:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_DISK_GET_CACHE_SETTING - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_DISK_SET_CACHE_SETTING:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_DISK_SET_CACHE_SETTING - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_ATA_PASS_THROUGH:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_ATA_PASS_THROUGH - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_ATA_PASS_THROUGH_DIRECT:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_ATA_PASS_THROUGH_DIRECT - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_DISK_GET_DRIVE_GEOMETRY:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_DISK_GET_DRIVE_GEOMETRY - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_DISK_GET_PARTITION_INFO:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_DISK_GET_PARTITION_INFO - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_DISK_SET_PARTITION_INFO:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_DISK_SET_PARTITION_INFO - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_DISK_GET_DRIVE_LAYOUT:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_DISK_GET_DRIVE_LAYOUT - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_DISK_SET_DRIVE_LAYOUT:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_DISK_SET_DRIVE_LAYOUT - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_DISK_GET_PARTITION_INFO_EX:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_DISK_GET_PARTITION_INFO_EX - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_DISK_SET_PARTITION_INFO_EX:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_DISK_SET_PARTITION_INFO_EX - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_DISK_GET_DRIVE_LAYOUT_EX:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_DISK_GET_DRIVE_LAYOUT_EX - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_DISK_SET_DRIVE_LAYOUT_EX:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_DISK_SET_DRIVE_LAYOUT_EX - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_DISK_GET_LENGTH_INFO:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_DISK_GET_LENGTH_INFO - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_DISK_GET_DRIVE_GEOMETRY_EX:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_DISK_GET_DRIVE_GEOMETRY_EX - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_VOLUME_IS_CLUSTERED:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_VOLUME_IS_CLUSTERED - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_DISK_IS_CLUSTERED:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_DISK_IS_CLUSTERED - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_DISK_RESET_SNAPSHOT_INFO:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_DISK_RESET_SNAPSHOT_INFO - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_DISK_GET_PARTITION_ATTRIBUTES:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_DISK_GET_PARTITION_ATTRIBUTES - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_DISK_SET_PARTITION_ATTRIBUTES:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_DISK_SET_PARTITION_ATTRIBUTES - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_DISK_SET_SNAPSHOT_INFO:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_DISK_SET_SNAPSHOT_INFO - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_DISK_GET_SNAPSHOT_INFO:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_DISK_GET_SNAPSHOT_INFO - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_VOLUME_SET_GPT_ATTRIBUTES:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_VOLUME_SET_GPT_ATTRIBUTES - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_DISK_SET_DISK_ATTRIBUTES:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_DISK_SET_DISK_ATTRIBUTES - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    case IOCTL_DISK_GET_DISK_ATTRIBUTES:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "VcbtDeviceControl: IOCTL_DISK_GET_DISK_ATTRIBUTES - DeviceObject %p, Irp %p", DeviceObject, Irp);
        break;
    default:
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "Unknown IOCTL : 0x%08X - DeviceObject %p, Irp %p", currentIrpStack->Parameters.DeviceIoControl.IoControlCode, DeviceObject, Irp);
        break;
#endif
    }

    //
    // Pass device control requests
    // down to next driver layer.
    //
    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(deviceExtension->TargetDeviceObject, Irp);
} // end VcbtDeviceControl()

NTSTATUS
VcbtShutdownFlush(
IN PDEVICE_OBJECT DeviceObject,
IN PIRP Irp
)

/*++

Routine Description:

This routine is called for a shutdown and flush IRPs.  These are sent by the
system before it actually shuts down or when the file system does a flush.

Arguments:

DriverObject - Pointer to device object to being shutdown by system.
Irp          - IRP involved.

Return Value:

NT Status

--*/

{
    PDEVICE_EXTENSION  deviceExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION currentIrpStack = IoGetCurrentIrpStackLocation(Irp);

    if (currentIrpStack->MajorFunction == IRP_MJ_SHUTDOWN) {
        DoTraceMsg(2, "VcbtShutdownFlush: shutdown DeviceObject %p, Irp %p",
            DeviceObject, Irp);
        StopThread(deviceExtension);
    }
    //else{
    //    DoTraceMsg(2, "VcbtShutdownFlush: Flush DeviceObject %p, Irp %p",
    //        DeviceObject, Irp);
    //}
    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(deviceExtension->TargetDeviceObject, Irp);

} // end VcbtShutdownFlush()


VOID
VcbtUnload(
IN PDRIVER_OBJECT DriverObject
)

/*++

Routine Description:

Free all the allocated resources, etc.

Arguments:

DriverObject - pointer to a driver object.

Return Value:

VOID.

--*/
{
    PAGED_CODE();   
    UNREFERENCED_PARAMETER(DriverObject);
    WPP_CLEANUP(DriverObject);
    return;
}


NTSTATUS
VcbtRegisterDevice(
IN PDEVICE_OBJECT DeviceObject
)

/*++

Routine Description:

Routine to initialize a proper name for the device object, and
register it with WMI

Arguments:

DeviceObject - pointer to a device object to be initialized.

Return Value:

Status of the initialization. NOTE: If the registration fails,
the device name in the DeviceExtension will be left as empty.

--*/

{
    NTSTATUS                status = STATUS_SUCCESS;
    PDEVICE_EXTENSION       deviceExtension;

    PAGED_CODE();

    DoTraceMsg(2, "VcbtRegisterDevice: DeviceObject %p",
        DeviceObject);
    deviceExtension = DeviceObject->DeviceExtension;

    if (is_volume_device(deviceExtension->DeviceObject)){
        if (NT_SUCCESS(status = get_device_number(deviceExtension->TargetDeviceObject, &deviceExtension->Number))) {
        }
        else{
            deviceExtension->Number.DeviceNumber = (ULONG)-1;
            deviceExtension->Number.PartitionNumber = (ULONG)-1;
        }

        {

            // request for partition's information failed, try volume
            PMOUNTDEV_NAME  output;
            VOLUME_NUMBER   volumeNumber;
            ULONG           nameSize;

            if (!NT_SUCCESS(status = query_device_name(deviceExtension->DeviceObject, &output))) {
                VcbtLogError(
                    DeviceObject,
                    260,
                    STATUS_SUCCESS,
                    IO_ERR_CONFIGURATION_ERROR);
                return status;
            }

            //
            // Since we get the volume name instead of the disk number,
            // set it to a dummy value
            // Todo: Instead of passing the disk number back to the user app.
            // for tracing, pass the STORAGE_DEVICE_NUMBER structure instead.

            nameSize = min(output->NameLength, sizeof(deviceExtension->PhysicalDeviceNameBuffer) - sizeof(WCHAR));
            RtlStringCbCopyW(deviceExtension->PhysicalDeviceNameBuffer, nameSize, output->Name);
            RtlInitUnicodeString(&deviceExtension->PhysicalDeviceName, &deviceExtension->PhysicalDeviceNameBuffer[0]);
            ExFreePool(output);

            //
            // Now, get the VOLUME_NUMBER information
            //
            if (!NT_SUCCESS(status = query_volume_number(deviceExtension->DeviceObject, &volumeNumber)) ||
                volumeNumber.VolumeManagerName[0] == (WCHAR)UNICODE_NULL) {
                RtlCopyMemory(
                    &deviceExtension->StorageManagerName[0],
                    L"LogiDisk",
                    8 * sizeof(WCHAR));
                if (NT_SUCCESS(status))
                    deviceExtension->VolumeNumber = volumeNumber.VolumeNumber;
            }
            else {
                RtlCopyMemory(
                    &deviceExtension->StorageManagerName[0],
                    &volumeNumber.VolumeManagerName[0],
                    8 * sizeof(WCHAR));
                deviceExtension->VolumeNumber = volumeNumber.VolumeNumber;
            }

            RtlStringCchPrintfW(deviceExtension->PhysicalDeviceNameBuffer, VCBT_MAXSTR, L"%s%d (Disk %d:%d)",
                deviceExtension->PhysicalDeviceNameBuffer,
                deviceExtension->VolumeNumber,
                deviceExtension->Number.DeviceNumber,
                deviceExtension->Number.PartitionNumber);
            RtlInitUnicodeString(&deviceExtension->PhysicalDeviceName, &deviceExtension->PhysicalDeviceNameBuffer[0]);
            DoTraceMsg(3, "VcbtRegisterDevice: Device name %ws - DeviceObject %p",
                deviceExtension->PhysicalDeviceNameBuffer, DeviceObject);
        }
    }
    return status;
}

VOID
VcbtLogError(
IN PDEVICE_OBJECT DeviceObject,
IN ULONG UniqueId,
IN NTSTATUS ErrorCode,
IN NTSTATUS Status
)

/*++

Routine Description:

Routine to log an error with the Error Logger

Arguments:

DeviceObject - the device object responsible for the error
UniqueId     - an id for the error
Status       - the status of the error

Return Value:

None

--*/

{
    PIO_ERROR_LOG_PACKET errorLogEntry;

    UCHAR EntrySize = ERROR_LOG_MAXIMUM_SIZE; // sizeof(IO_ERROR_LOG_PACKET) + sizeof(DEVICE_OBJECT)

    errorLogEntry = (PIO_ERROR_LOG_PACKET)
        IoAllocateErrorLogEntry(
        (PVOID)DeviceObject,
        (UCHAR)(EntrySize)
        );

    if (errorLogEntry != NULL) {
        errorLogEntry->ErrorCode = ErrorCode;
        errorLogEntry->UniqueErrorValue = UniqueId;
        errorLogEntry->FinalStatus = Status;
        //
        // The following is necessary because DumpData is of type ULONG
        // and DeviceObject can be more than that
        //
        //RtlCopyMemory(
        //    &errorLogEntry->DumpData[0],
        //    DeviceObject,
        //    sizeof(DEVICE_OBJECT));
        //errorLogEntry->DumpDataSize = sizeof(DEVICE_OBJECT);
        IoWriteErrorLogEntry(errorLogEntry);
    }
}

NTSTATUS StartThread(PDEVICE_EXTENSION pdx){
    NTSTATUS status;
    HANDLE hthread;
    if (pdx->Thread != NULL){
        if (NULL == pdx->Journal){
            KeSetEvent(&pdx->KillEvent, 0, FALSE);
            KeWaitForSingleObject(pdx->Thread, Executive, KernelMode, FALSE, NULL);
            ObDereferenceObject(pdx->Thread);
            pdx->Thread = NULL;
        }
    }
    if (pdx->Thread == NULL){
        size_t len = 0;
        if ((NT_SUCCESS(RtlStringCbLengthW(pdx->MountDeviceLink, sizeof(pdx->MountDeviceLink), &len))) &&
            len > 0){
            DoTraceMsg(TRACE_LEVEL_VERBOSE, "StartThread, MountDeviceLink %ws - DeviceObject %p", pdx->MountDeviceLink, pdx->DeviceObject);
            pdx->Enabled = true;
            KeInitializeEvent(&pdx->KillEvent, NotificationEvent, FALSE);
            KeInitializeEvent(&pdx->QueueEvent, NotificationEvent, FALSE);
            status = PsCreateSystemThread(&hthread, THREAD_ALL_ACCESS,
                NULL, NULL, &pdx->ThreadId, (PKSTART_ROUTINE)ThreadProc, pdx);
            if (!NT_SUCCESS(status))
                return status;
            ObReferenceObjectByHandle(hthread, THREAD_ALL_ACCESS, NULL,
                KernelMode, (PVOID*)&pdx->Thread, NULL);
            ZwClose(hthread);
        }
    }
    return STATUS_SUCCESS;
}

VOID StopThread(PDEVICE_EXTENSION pdx){

    if (pdx->CheckThread != NULL){
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "StopThread, Stopping CheckJournalThreadProc : MountDeviceLink %ws - DeviceObject %p", pdx->MountDeviceLink, pdx->DeviceObject);
        if (NULL != pdx->Journal){
            pdx->Journal->Check = FALSE;
        }
        KeWaitForSingleObject(pdx->CheckThread, Executive, KernelMode, FALSE, NULL);
        ObDereferenceObject(pdx->CheckThread);
        pdx->CheckThread = NULL;
    }

    if (pdx->Thread != NULL){
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "StopThread, MountDeviceLink %ws - DeviceObject %p", pdx->MountDeviceLink, pdx->DeviceObject);
        if (NULL != pdx->Journal){
            _close_file(pdx->Journal->JournalMetaDataFileHandle);
            _close_file(pdx->Journal->JournalFileHandle);
            _close_file(pdx->Journal->UmapFileHandle);
        }
        KeSetEvent(&pdx->KillEvent, 0, FALSE);
        KeWaitForSingleObject(pdx->Thread, Executive, KernelMode, FALSE, NULL);
        ObDereferenceObject(pdx->Thread);
        pdx->Thread = NULL;
    }
    FreeJournal(pdx);
}

VOID     ThreadProc(PDEVICE_EXTENSION pdx){
    NTSTATUS status;
    DoTraceMsg(TRACE_LEVEL_VERBOSE, "ThreadProc, MountDeviceLink %ws - DeviceObject %p", pdx->MountDeviceLink, pdx->DeviceObject);
    KeSetPriorityThread(KeGetCurrentThread(), LOW_REALTIME_PRIORITY);
    InitialJournal(pdx);
    KeInitializeTimerEx(&pdx->Timer, SynchronizationTimer);   
    PVOID pollevents[3];
    pollevents[0] = (PVOID)&pdx->KillEvent;
    pollevents[1] = (PVOID)&pdx->QueueEvent;
    pollevents[2] = (PVOID)&pdx->Timer;
    LARGE_INTEGER duetime = { 0 };
#define POLLING_INTERVAL 5000
    duetime.QuadPart = -10;
    KeSetTimerEx(&pdx->Timer, duetime, POLLING_INTERVAL, NULL);
    bool loop = true;
    bool exit = false;
    while (loop){
        status = KeWaitForMultipleObjects(ARRAYSIZE(pollevents),
            pollevents, WaitAny, Executive, KernelMode, FALSE,
            NULL, NULL);     
        while (loop){
            NTSTATUS Status = STATUS_SUCCESS;
            PLIST_ENTRY entry = ExInterlockedRemoveHeadList(&pdx->QueueList, &pdx->QueueLock);
            if (NULL == entry){         
                /*if ((NULL != pdx->Journal) &&
                    (TRUE == pdx->Journal->Initialized) &&
                    (FALSE == pdx->Journal->Check) && 
                    (TRUE == pdx->Journal->Ready)){
                    lock_acquire(&pdx->Journal->Lock);
                    PLIST_ENTRY e;
                    NTSTATUS Status;
                    BOOLEAN  NeedUpdate = FALSE;
                    while (&pdx->Journal->WrittenList != (e = RemoveHeadList(&pdx->Journal->WrittenList))){
                        PIO_RANGE written = CONTAINING_RECORD(e, IO_RANGE, Bind);
                        if (TRUE == set_umap(pdx->Journal->UMAP, written->Start, written->Length, pdx->Journal->Resolution)){
                            NeedUpdate = TRUE;
                        }
                        Status = WriteJournalData(pdx, written->Start, written->Length);
                        if (!NT_SUCCESS(Status)){
                        }
                        ExFreePool(written);
                    }
                    lock_release(&pdx->Journal->Lock);
                    if (TRUE == NeedUpdate){
                        Status = FlushUmapData(pdx, 0, 0, TRUE);
                        if (!NT_SUCCESS(Status)){
                        }
                    }
                }*/
                loop =  (exit) ? false : pdx->Enabled;
                break;
            }
            else{
                PIRP Irp = CONTAINING_RECORD(entry, IRP, Tail.Overlay.ListEntry);
                PIO_STACK_LOCATION io_stack = IoGetCurrentIrpStackLocation(Irp);
                if (NULL != pdx->Journal){
                    if (TRUE == pdx->Journal->Initialized){
                        /*if (TRUE == pdx->Journal->Check){
                            PIO_RANGE written = (PIO_RANGE)ExAllocatePool(NonPagedPool, sizeof(IO_RANGE));
                            written->Start = io_stack->Parameters.Write.ByteOffset.QuadPart;
                            written->Length = io_stack->Parameters.Write.Length;
                            lock_acquire(&pdx->Journal->Lock);
                            InsertHeadList(&pdx->Journal->WrittenList, &written->Bind);
                            lock_release(&pdx->Journal->Lock);
                        }
                        else */if (FALSE == pdx->Journal->Ready){
                            FreeJournal(pdx);
                            DoTraceMsg(TRACE_LEVEL_VERBOSE, "ThreadProc: Free Journal - DeviceObject %p", pdx->DeviceObject);
                        }
                        else {
                            lock_acquire(&pdx->Journal->Lock);
                            if (pdx->Journal->NeedUpdateRetrievalPointers){
                                __free(pdx->Journal->UmapRetrievalPointers);
                                __free(pdx->Journal->JournalRetrievalPointers);
                                __free(pdx->Journal->JournalMetaDataRetrievalPointers);
                                pdx->Journal->UmapRetrievalPointers = pdx->Journal->NewUmapRetrievalPointers;
                                pdx->Journal->JournalRetrievalPointers = pdx->Journal->NewJournalRetrievalPointers;
                                pdx->Journal->JournalMetaDataRetrievalPointers = pdx->Journal->NewJournalMetaDataRetrievalPointers;
                                pdx->Journal->NewUmapRetrievalPointers = NULL;
                                pdx->Journal->NewJournalRetrievalPointers = NULL;
                                pdx->Journal->NewJournalMetaDataRetrievalPointers = NULL;
                                pdx->Journal->NeedUpdateRetrievalPointers = FALSE;
                            }                          
                            PLIST_ENTRY e;                     
                            BOOLEAN  NeedUpdate = FALSE;
                            BOOLEAN  FullUpdate = FALSE;
                            while (&pdx->Journal->WrittenList != (e = RemoveHeadList(&pdx->Journal->WrittenList))){
                                PIO_RANGE written = CONTAINING_RECORD(e, IO_RANGE, Bind);
                                if (TRUE == set_umap(pdx->Journal->UMAP, written->Start, written->Length, pdx->Journal->Resolution)){
                                    FullUpdate = TRUE;
                                    NeedUpdate = TRUE;
                                }
                                lock_release(&pdx->Journal->Lock);
                                Status = WriteJournalData(pdx, written->Start, written->Length);
                                lock_acquire(&pdx->Journal->Lock);
                                if (!NT_SUCCESS(Status)){
                                    InsertHeadList(&pdx->Journal->WrittenList, &written->Bind);
                                }
                                else{
                                    ExFreePool(written);
                                }
                           }
                            lock_release(&pdx->Journal->Lock);
                            if (io_stack->MajorFunction == IRP_MJ_WRITE){
                                if (TRUE == set_umap(pdx->Journal->UMAP, io_stack->Parameters.Write.ByteOffset.QuadPart, io_stack->Parameters.Write.Length, pdx->Journal->Resolution))
                                    NeedUpdate = TRUE;
                                if (TRUE == NeedUpdate){
                                    Status = FlushUmapData(pdx, io_stack->Parameters.Write.ByteOffset.QuadPart, io_stack->Parameters.Write.Length, FullUpdate);
                                    if (!NT_SUCCESS(Status)){
                                        DoTraceMsg(TRACE_LEVEL_ERROR, "ThreadProc: Failed to flush the umap data for I/O( start: %I64u, length %d ) - DeviceObject %p",
                                            io_stack->Parameters.Write.ByteOffset.QuadPart,
                                            io_stack->Parameters.Write.Length,
                                            pdx->DeviceObject);
                                    }
                                }
                                Status = WriteJournalData(pdx, io_stack->Parameters.Write.ByteOffset.QuadPart, io_stack->Parameters.Write.Length);
                                if (!NT_SUCCESS(Status)){
                                    DoTraceMsg(TRACE_LEVEL_ERROR, "ThreadProc: Failed to write the journal data for I/O( start: %I64u, length %d ) - DeviceObject %p",
                                        io_stack->Parameters.Write.ByteOffset.QuadPart,
                                        io_stack->Parameters.Write.Length,
                                        pdx->DeviceObject);
                                }
                            }
                            else if (io_stack->MajorFunction == IRP_MJ_DEVICE_CONTROL){
                                if (io_stack->Parameters.DeviceIoControl.IoControlCode == IOCTL_DISK_COPY_DATA){
                                    if (io_stack->Parameters.DeviceIoControl.InputBufferLength >= sizeof(DISK_COPY_DATA_PARAMETERS)){
                                        PDISK_COPY_DATA_PARAMETERS copy_data_parameters
                                            = (PDISK_COPY_DATA_PARAMETERS)Irp->AssociatedIrp.SystemBuffer;
                                        ULONGLONG start = copy_data_parameters->DestinationOffset.QuadPart;
                                        ULONGLONG length = copy_data_parameters->CopyLength.QuadPart;
                                        bool loop = true;
                                        while (loop){
                                            ULONG _length = 0;
                                            if (length > 4294963200){
                                                _length = 4294963200;
                                            }
                                            else{
                                                _length = (ULONG)length;
                                                loop = false;
                                            }
                                            if (TRUE == set_umap(pdx->Journal->UMAP, start, _length, pdx->Journal->Resolution))
                                                NeedUpdate = TRUE;
                                            if (TRUE == NeedUpdate){
                                                Status = FlushUmapData(pdx, start, _length, FullUpdate);
                                                if (!NT_SUCCESS(Status)){
                                                    DoTraceMsg(TRACE_LEVEL_ERROR, "ThreadProc: Failed to flush the umap data for I/O( start: %I64u, length %d ) - DeviceObject %p",
                                                        start,
                                                        _length,
                                                        pdx->DeviceObject);
                                                    break;
                                                }
                                            }
                                            Status = WriteJournalData(pdx, start, _length);
                                            if (!NT_SUCCESS(Status)){
                                                DoTraceMsg(TRACE_LEVEL_ERROR, "ThreadProc: Failed to write the journal data for I/O( start: %I64u, length %d ) - DeviceObject %p",
                                                    start,
                                                    _length,
                                                    pdx->DeviceObject);
                                                break;
                                            }
                                            if (loop){
                                                start += _length;
                                                length -= _length;
                                            }
                                            else{
                                                break;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                    else{
                        PIO_RANGE written = (PIO_RANGE)ExAllocatePool(NonPagedPool, sizeof(IO_RANGE));
                        written->Start = io_stack->Parameters.Write.ByteOffset.QuadPart;
                        written->Length = io_stack->Parameters.Write.Length;                             
                        lock_acquire(&pdx->Journal->Lock);
                        InsertTailList(&pdx->Journal->WrittenList, &written->Bind);
                        lock_release(&pdx->Journal->Lock);
                    }
                }
                if (!NT_SUCCESS(Status)){
                    Irp->IoStatus.Status = Status;
                    IoCompleteRequest(Irp, IO_NO_INCREMENT);
                }
                else{
                    IoSkipCurrentIrpStackLocation(Irp);
                    IoCallDriver(pdx->TargetDeviceObject, Irp);
                }
            }
        }
        if (!NT_SUCCESS(status))
            break;
        else if (status == STATUS_WAIT_0)
            exit = true;
        else if (status == STATUS_WAIT_1)
            KeClearEvent(&pdx->QueueEvent);
    }
    KeCancelTimer(&pdx->Timer);
    PsTerminateSystemThread(STATUS_SUCCESS);
    DoTraceMsg(TRACE_LEVEL_VERBOSE, "ThreadProc: Terminate Thread - DeviceObject %p", pdx->DeviceObject);
}

typedef struct _FILE_FS_ATTRIBUTE_INFORMATION {
    ULONG FileSystemAttributes;
    LONG  MaximumComponentNameLength;
    ULONG FileSystemNameLength;
    WCHAR FileSystemName[1];
} FILE_FS_ATTRIBUTE_INFORMATION, *PFILE_FS_ATTRIBUTE_INFORMATION;

VOID     JournalInitialThreadProc(PDEVICE_EXTENSION pdx){ 
    KIRQL  oldIrql = PASSIVE_LEVEL;
    if (PASSIVE_LEVEL != (oldIrql = KeGetCurrentIrql())){
        KeLowerIrql(PASSIVE_LEVEL);
    }
    size_t len = 0;
    NTSTATUS Status = STATUS_SUCCESS;
    if (NULL != pdx->Journal && 
        (NT_SUCCESS(Status = RtlStringCbLengthW(pdx->MountDeviceLink, sizeof(pdx->MountDeviceLink), &len))) &&
        len > 0){

        if (NT_SUCCESS(Status = OpenUmapFile(pdx)) && 
            NT_SUCCESS(Status = get_fs_cluster_size(pdx->Journal->UmapFileHandle, &pdx->Journal->FsClusterSize, &pdx->Journal->BytesPerSector))){
            LARGE_INTEGER umapFileSize;
            umapFileSize.QuadPart = 0;
            if (NT_SUCCESS(Status = get_file_size(pdx->Journal->UmapFileHandle, &umapFileSize))){
                if (umapFileSize.QuadPart >= RESOLUTION_UMAP_SIZE){
                    if (NT_SUCCESS(Status = read_file(pdx->Journal->UmapFileHandle, pdx->Journal->UMAP, sizeof(pdx->Journal->UMAP), 0))){
                    }   
                }
                else{
                    DoTraceMsg(TRACE_LEVEL_ERROR, "JournalInitialThreadProc: The U-map file size is invalid - DeviceObject %p", pdx->DeviceObject);
                    Status = STATUS_DATA_ERROR;
                }
            }
        }
        
        if (NT_SUCCESS(Status)){
            RtlStringCchPrintfW(pdx->Journal->JournalMetaDataFileNameBuffer, VCBT_MAX_PATH, L"%s%s%s", pdx->MountDeviceLink, VCBT_VOLUME_SYS_INFO, CBT_JOURNAL_METADATA);
            RtlInitUnicodeString(&pdx->Journal->JournalMetaDataFileName, &pdx->Journal->JournalMetaDataFileNameBuffer[0]);
            RtlStringCchPrintfW(pdx->Journal->JournalFileNameBuffer, VCBT_MAX_PATH, L"%s%s%s", pdx->MountDeviceLink, VCBT_VOLUME_SYS_INFO, CBT_JOURNAL_FILE1);
            RtlInitUnicodeString(&pdx->Journal->JournalFileName, &pdx->Journal->JournalFileNameBuffer[0]);
            if (FALSE == _ControlDeviceExtension->JournalDisabled){             
                    Status = open_file(&pdx->Journal->JournalMetaDataFileHandle, &pdx->Journal->JournalMetaDataFileName);
                if (NT_SUCCESS(Status) && NT_SUCCESS(Status = open_file(&pdx->Journal->JournalFileHandle, &pdx->Journal->JournalFileName))){
                    LARGE_INTEGER journalFileSize;
                    journalFileSize.QuadPart = 0;
                    if (NT_SUCCESS(Status = get_file_size(pdx->Journal->JournalFileHandle, &journalFileSize)) &&
                        NT_SUCCESS(Status = InitialJournalData(pdx))){
                        if (pdx->Journal->JournalMetaData.Block.j.Size <= journalFileSize.QuadPart){
                        }
                        else{
                            DoTraceMsg(TRACE_LEVEL_ERROR, "JournalInitialThreadProc: The Journal file size is invalid - DeviceObject %p", pdx->DeviceObject);
                            Status = STATUS_DATA_ERROR;
                        }
                    }
                }
            }
            else{
                delete_file_ex(&pdx->Journal->JournalFileName);
            }
        }

        if (NT_SUCCESS(Status)){
            Status = GetFilesRetrievalPointers(pdx);
        }
        
        if (NT_SUCCESS(Status)){
            ProtectJournalFilesClusters(pdx);
            pdx->Journal->Ready = TRUE;
            LogEventMessage(pdx->DeviceObject, VCBT_JOURNAL_ENABLED, 1, pdx->PhysicalDeviceNameBuffer);
        }
        else{
            LogEventMessage(pdx->DeviceObject, VCBT_JOURNAL_DISABLED, 1, pdx->PhysicalDeviceNameBuffer);
            if (pdx->Journal->UmapFileHandle != 0){
                delete_file(pdx->Journal->UmapFileHandle);
                _close_file(pdx->Journal->UmapFileHandle);
            }
            if (pdx->Journal->JournalMetaDataFileHandle != 0){
                delete_file(pdx->Journal->JournalMetaDataFileHandle);
                _close_file(pdx->Journal->JournalMetaDataFileHandle);
            }
            if (pdx->Journal->JournalFileHandle != 0){
                delete_file(pdx->Journal->JournalFileHandle);
                _close_file(pdx->Journal->JournalFileHandle);
            }
        }
        pdx->Journal->Initialized = TRUE;
    } 
    if (PASSIVE_LEVEL != oldIrql)
        KeRaiseIrql(oldIrql, &oldIrql);
    PsTerminateSystemThread(STATUS_SUCCESS);
}

NTSTATUS InitialJournalDataDirect(PDEVICE_EXTENSION pdx){
    NTSTATUS Status = STATUS_INVALID_PARAMETER;
    if (NULL != pdx->Journal){
        ULONGLONG offset = 0;
        bool found = false;
        pdx->Journal->JournalViewSize = pdx->Journal->FsClusterSize * 2;
        pdx->Journal->JournalView = (BYTE*)__malloc(pdx->Journal->JournalViewSize);
        if (NULL != pdx->Journal->JournalView){        
            if (NT_SUCCESS(Status = ReadJournalMetaData(pdx, (BYTE*)&pdx->Journal->JournalMetaData, 0, sizeof(pdx->Journal->JournalMetaData)))){
                if ((pdx->Journal->JournalMetaData.Block.j.CheckSum == (ULONG)-1) &&
                    (pdx->Journal->JournalMetaData.Block.j.Signature[0] == '1') &&
                    (pdx->Journal->JournalMetaData.Block.j.Signature[1] == 'c') &&
                    (pdx->Journal->JournalMetaData.Block.j.Signature[2] == 'b') &&
                    (pdx->Journal->JournalMetaData.Block.j.Signature[3] == 't')){
                    DoTraceMsg(TRACE_LEVEL_VERBOSE, "InitialJournalDataDirect : metadata file : Latest Key : %I64u, First Key : %I64u - DeviceObject %p", pdx->Journal->JournalMetaData.Block.r.Key, pdx->Journal->JournalMetaData.FirstKey, pdx->DeviceObject);
                    if (pdx->Journal->JournalMetaData.Block.r.Key >= pdx->Journal->JournalMetaData.FirstKey)
                        offset = (pdx->Journal->JournalMetaData.Block.r.Key - pdx->Journal->JournalMetaData.FirstKey) * sizeof(VCBT_RECORD);
                    else{
                        offset = (((ULONGLONG)-1) - pdx->Journal->JournalMetaData.FirstKey + pdx->Journal->JournalMetaData.Block.r.Key) * sizeof(VCBT_RECORD);
                    }
                    pdx->Journal->JournalViewOffset = (offset / pdx->Journal->FsClusterSize)* pdx->Journal->FsClusterSize;
                    if (pdx->Journal->JournalViewOffset > 0)
                        offset = sizeof(VCBT_RECORD) - (pdx->Journal->JournalViewOffset % sizeof(VCBT_RECORD));
                    else
                        offset = 0;
                }
                else{
                    Status = STATUS_DATA_ERROR;
                    DoTraceMsg(TRACE_LEVEL_ERROR, "InitialJournalDataDirect : The Journal metadata is invalid - DeviceObject %p", pdx->DeviceObject);
                }
            }

            if (NT_SUCCESS(Status)){
                UINT32 number_of_bytes_read = pdx->Journal->JournalViewSize;
                UINT32 number_of_bytes_to_read = pdx->Journal->JournalViewSize;
                pdx->Journal->JournalBlockPtr = (PVCBT_JOURNAL_BLOK)(pdx->Journal->JournalView + offset);
                offset = 0;
                while (NT_SUCCESS(Status = ReadFromJournalData(pdx, &pdx->Journal->JournalView[offset], pdx->Journal->JournalViewOffset + offset, number_of_bytes_to_read))){
                    LONG min_size = (LONG)min((LONG)number_of_bytes_read + (LONG)offset, (pdx->Journal->FsClusterSize + (LONG)sizeof(VCBT_RECORD)));
                    for (;
                        (BYTE*)pdx->Journal->JournalBlockPtr < (pdx->Journal->JournalView + min_size);
                        pdx->Journal->JournalBlockPtr = (PVCBT_JOURNAL_BLOK)(((BYTE*)pdx->Journal->JournalBlockPtr) + sizeof(VCBT_RECORD))){

                        if (0 == pdx->Journal->JournalViewOffset && pdx->Journal->JournalBlockPtr == (PVCBT_JOURNAL_BLOK)pdx->Journal->JournalView){                           
                            if (pdx->Journal->JournalMetaData.FirstKey != pdx->Journal->JournalBlockPtr->r.Key){
                                DoTraceMsg(TRACE_LEVEL_ERROR, "InitialJournalDataDirect : The Journal file first key is not equal to the meta data file recorded - DeviceObject %p", pdx->DeviceObject);
                                break;
                            }
                            DoTraceMsg(TRACE_LEVEL_VERBOSE, "InitialJournalDataDirect : The First record Key : %I64u - DeviceObject %p", pdx->Journal->JournalMetaData.FirstKey, pdx->DeviceObject);
                        }

                        if ((pdx->Journal->JournalBlockPtr->j.CheckSum == (ULONG)-1) &&
                            (pdx->Journal->JournalBlockPtr->j.Signature[0] == '1') &&
                            (pdx->Journal->JournalBlockPtr->j.Signature[1] == 'c') &&
                            (pdx->Journal->JournalBlockPtr->j.Signature[2] == 'b') &&
                            (pdx->Journal->JournalBlockPtr->j.Signature[3] == 't')){                            
                            if (pdx->Journal->JournalMetaData.Block.r.Key > pdx->Journal->JournalBlockPtr->r.Key){
                                Status = STATUS_DATA_ERROR;
                                DoTraceMsg(TRACE_LEVEL_ERROR, "InitialJournalDataDirect : The latest Journal record is not larger than or equal to the meta data file recorded - DeviceObject %p", pdx->DeviceObject);
                                break;
                            }
                            memcpy(&pdx->Journal->JournalMetaData.Block, pdx->Journal->JournalBlockPtr, sizeof(VCBT_JOURNAL_BLOK));
                            DoTraceMsg(TRACE_LEVEL_VERBOSE, "InitialJournalDataDirect : Found the latest Journal record (Key : %I64u) - DeviceObject %p", pdx->Journal->JournalMetaData.Block.r.Key, pdx->DeviceObject);
                            found = true;
                            break;
                        }
                    }
                    if (found)
                        break;
                    offset = number_of_bytes_to_read = number_of_bytes_read = pdx->Journal->FsClusterSize;
                    RtlMoveMemory(&pdx->Journal->JournalView[0], &pdx->Journal->JournalView[offset], (size_t)offset);
                    RtlZeroMemory(&pdx->Journal->JournalView[offset], (size_t)offset);
                    pdx->Journal->JournalBlockPtr = (PVCBT_JOURNAL_BLOK)(((BYTE*)pdx->Journal->JournalBlockPtr) - pdx->Journal->FsClusterSize);
                    pdx->Journal->JournalViewOffset += pdx->Journal->FsClusterSize;
                }
            }
        }
        if (!found){
            LogEventMessage(pdx->DeviceObject, VCBT_BAD_JOURNAL_DATA, 1, pdx->PhysicalDeviceNameBuffer);
            Status = STATUS_DATA_ERROR;
        }
    }
    return Status;
}

bool InitialJournalDirect(PDEVICE_EXTENSION pdx){
    bool result = false;
    PNTFS_VOLUME volume = get_ntfs_volume_info(pdx->TargetDeviceObject);
    if (volume){
        pdx->Journal->FsClusterSize = volume->cluster_size;
        pdx->Journal->BytesPerSector = volume->sector_size;
        PNTFS_FILE_RECORD root = get_file_record(volume, MFT_IDX_ROOT, MASK_INDEX_ROOT | MASK_INDEX_ALLOCATION);
        if (root){
            ULONGLONG fileRef = 0;
            if (find_sub_entry(volume, root, L"System Volume Information", &fileRef)){
                PNTFS_FILE_RECORD sys_volume_info = get_file_record(volume, fileRef, MASK_INDEX_ROOT | MASK_INDEX_ALLOCATION);
                if (sys_volume_info){
                    if (is_directory(sys_volume_info) && !is_deleted(sys_volume_info)){
                        LARGE_INTEGER    length;
                        NTSTATUS         status;
                        LARGE_INTEGER    liVcnPrev;
                        if (NT_SUCCESS(status = get_volume_length(pdx->DeviceObject, &length))){
                            int resolution = UMAP_RESOLUTION1;
                            int count = 1;
                            while ((length.QuadPart >> (resolution + 19)) > 0){
                                count++;
                                resolution++;
                            }
                            WCHAR name[VCBT_MAXSTR];
                            memset(name, 0, sizeof(name));
                            RtlStringCchPrintfW(name, VCBT_MAXSTR, UMAP_RESOLUTION_FILE_, count);
                            if (find_sub_entry(volume, sys_volume_info, name, &fileRef)){
                                PNTFS_FILE_RECORD umap = get_file_record(volume, fileRef, MASK_DATA | MASK_FILE_NAME);
                                if (umap){
                                    if (is_deleted(umap)){
                                        status = STATUS_DATA_ERROR;
                                        DoTraceMsg(TRACE_LEVEL_WARNING, "InitialJournalDirect: The U-map file was deleted - DeviceObject %p", pdx->DeviceObject);
                                    }
                                    else if (ntfs_get_file_clusters(umap) * volume->cluster_size >= RESOLUTION_UMAP_SIZE){
                                        RtlStringCchPrintfW(pdx->Journal->UmapFileNameBuffer, VCBT_MAX_PATH, L"%s%s"UMAP_RESOLUTION_FILE, pdx->MountDeviceLink, VCBT_VOLUME_SYS_INFO, count);
                                        RtlInitUnicodeString(&pdx->Journal->UmapFileName, &pdx->Journal->UmapFileNameBuffer[0]);
                                        pdx->Journal->Resolution = resolution;
                                        pdx->Journal->UmapRetrievalPointers = ntfs_get_file_retrieval_pointers(umap);
                                        if (pdx->Journal->UmapRetrievalPointers){
                                            liVcnPrev = pdx->Journal->UmapRetrievalPointers->StartingVcn;
                                            for (DWORD extent = 0; extent < pdx->Journal->UmapRetrievalPointers->ExtentCount; extent++){
                                                LONGLONG _Start = liVcnPrev.QuadPart * pdx->Journal->FsClusterSize;
                                                LONG     _Length = (ULONG)(pdx->Journal->UmapRetrievalPointers->Extents[extent].NextVcn.QuadPart - liVcnPrev.QuadPart) * pdx->Journal->FsClusterSize;
                                                LONGLONG _Lcn = pdx->Journal->UmapRetrievalPointers->Extents[extent].Lcn.QuadPart * pdx->Journal->FsClusterSize;
                                                DoTraceMsg(TRACE_LEVEL_VERBOSE, "InitialJournalDirect: Umap file location (start %I64u, lcn %I64u, length %d) - DeviceObject %p ", _Start, _Lcn, _Length, pdx->DeviceObject);
                                                liVcnPrev = pdx->Journal->UmapRetrievalPointers->Extents[extent].NextVcn;
                                            }
                                            status = ReadUmapData(pdx, pdx->Journal->UMAP, 0, RESOLUTION_UMAP_SIZE);
                                        }
                                        else{
                                            DoTraceMsg(TRACE_LEVEL_ERROR, "InitialJournalDirect: The U-map file retrieval pointers is invalid - DeviceObject %p", pdx->DeviceObject);
                                            status = STATUS_DATA_ERROR;
                                        }
                                    }
                                    else{
                                        DoTraceMsg(TRACE_LEVEL_ERROR, "InitialJournalDirect: The U-map file size is invalid - DeviceObject %p", pdx->DeviceObject);
                                        status = STATUS_DATA_ERROR;
                                    }
                                    free_file_record(&umap);
                                }
                                else{
                                    DoTraceMsg(TRACE_LEVEL_ERROR, "InitialJournalDirect: The U-map file is invalid - DeviceObject %p", pdx->DeviceObject);
                                    status = STATUS_DATA_ERROR;
                                }
                            }
                            else{
                                DoTraceMsg(TRACE_LEVEL_ERROR, "InitialJournalDirect: Can't find the U-map file (%ws) - DeviceObject %p", name, pdx->DeviceObject);
                                status = STATUS_DATA_ERROR;
                            }

                            if (NT_SUCCESS(status)){
                                if (FALSE == _ControlDeviceExtension->JournalDisabled){                                 
                                    if (find_sub_entry(volume, sys_volume_info, CBT_JOURNAL_METADATA_, &fileRef)){
                                        PNTFS_FILE_RECORD metadata = get_file_record(volume, fileRef, MASK_DATA | MASK_FILE_NAME);
                                        if (metadata){
                                            if (is_deleted(metadata)){
                                                status = STATUS_DATA_ERROR;
                                                DoTraceMsg(TRACE_LEVEL_ERROR, "InitialJournalDirect: The journal metadata file was deleted - DeviceObject %p", pdx->DeviceObject);
                                            }
                                            else{
                                                pdx->Journal->JournalMetaDataRetrievalPointers = ntfs_get_file_retrieval_pointers(metadata);
                                                if (!pdx->Journal->JournalMetaDataRetrievalPointers){
                                                    DoTraceMsg(TRACE_LEVEL_ERROR, "InitialJournalDirect: The journal metadata file retrieval pointers is invalid - DeviceObject %p", pdx->DeviceObject);
                                                    status = STATUS_DATA_ERROR;
                                                }
                                                else{
                                                    liVcnPrev = pdx->Journal->JournalMetaDataRetrievalPointers->StartingVcn;
                                                    for (DWORD extent = 0; extent < pdx->Journal->JournalMetaDataRetrievalPointers->ExtentCount; extent++){
                                                        LONGLONG _Start = liVcnPrev.QuadPart * pdx->Journal->FsClusterSize;
                                                        LONG     _Length = (ULONG)(pdx->Journal->JournalMetaDataRetrievalPointers->Extents[extent].NextVcn.QuadPart - liVcnPrev.QuadPart) * pdx->Journal->FsClusterSize;
                                                        LONGLONG _Lcn = pdx->Journal->JournalMetaDataRetrievalPointers->Extents[extent].Lcn.QuadPart * pdx->Journal->FsClusterSize;
                                                        DoTraceMsg(TRACE_LEVEL_VERBOSE, "InitialJournalDirect: Journal metadata file location (start %I64u, lcn %I64u, length %d) - DeviceObject %p ", _Start, _Lcn, _Length, pdx->DeviceObject);
                                                        liVcnPrev = pdx->Journal->JournalMetaDataRetrievalPointers->Extents[extent].NextVcn;
                                                    }
                                                }
                                            }
                                            free_file_record(&metadata);
                                        }
                                        else{
                                            DoTraceMsg(TRACE_LEVEL_ERROR, "InitialJournalDirect: Can't find the journal metadata file (%ws) - DeviceObject %p", CBT_JOURNAL_METADATA_, pdx->DeviceObject);
                                            status = STATUS_DATA_ERROR;
                                        }
                                    }
                                    else{
                                        DoTraceMsg(TRACE_LEVEL_ERROR, "InitialJournalDirect: Can't find the journal metadata file (%ws) - DeviceObject %p", CBT_JOURNAL_METADATA_, pdx->DeviceObject);
                                        status = STATUS_DATA_ERROR;
                                    }
                                    if (NT_SUCCESS(status)){
                                        if (find_sub_entry(volume, sys_volume_info, CBT_JOURNAL_FILE1_, &fileRef)){
                                            PNTFS_FILE_RECORD journal = get_file_record(volume, fileRef, MASK_DATA | MASK_FILE_NAME);
                                            if (journal){
                                                if (is_deleted(journal)){
                                                    status = STATUS_DATA_ERROR;
                                                    DoTraceMsg(TRACE_LEVEL_ERROR, "InitialJournalDirect: The journal file was deleted - DeviceObject %p", pdx->DeviceObject);
                                                }
                                                else{
                                                    pdx->Journal->JournalRetrievalPointers = ntfs_get_file_retrieval_pointers(journal);
                                                    if (!pdx->Journal->JournalRetrievalPointers){
                                                        DoTraceMsg(TRACE_LEVEL_ERROR, "InitialJournalDirect: The journal file retrieval pointers is invalid - DeviceObject %p", pdx->DeviceObject);
                                                        status = STATUS_DATA_ERROR;
                                                    }
                                                    else{

                                                        liVcnPrev = pdx->Journal->JournalRetrievalPointers->StartingVcn;
                                                        for (DWORD extent = 0; extent < pdx->Journal->JournalRetrievalPointers->ExtentCount; extent++){
                                                            LONGLONG _Start = liVcnPrev.QuadPart * pdx->Journal->FsClusterSize;
                                                            LONG     _Length = (ULONG)(pdx->Journal->JournalRetrievalPointers->Extents[extent].NextVcn.QuadPart - liVcnPrev.QuadPart) * pdx->Journal->FsClusterSize;
                                                            LONGLONG _Lcn = pdx->Journal->JournalRetrievalPointers->Extents[extent].Lcn.QuadPart * pdx->Journal->FsClusterSize;
                                                            DoTraceMsg(TRACE_LEVEL_VERBOSE, "InitialJournalDirect: Journal file location (start %I64u, lcn %I64u, length %d) - DeviceObject %p ", _Start, _Lcn, _Length, pdx->DeviceObject);
                                                            liVcnPrev = pdx->Journal->JournalRetrievalPointers->Extents[extent].NextVcn;
                                                        }

                                                        if (NT_SUCCESS(status = InitialJournalDataDirect(pdx))){
                                                            ULONGLONG journalFileSize = ntfs_get_file_clusters(journal)* volume->cluster_size;
                                                            if (!(journalFileSize > 0 && pdx->Journal->JournalMetaData.Block.j.Size <= journalFileSize)){
                                                                DoTraceMsg(TRACE_LEVEL_ERROR, "InitialJournalDirect: The Journal file size is invalid - DeviceObject %p", pdx->DeviceObject);
                                                                status = STATUS_DATA_ERROR;
                                                            }
                                                        }
                                                    }
                                                }
                                                free_file_record(&journal);
                                            }
                                            else{
                                                DoTraceMsg(TRACE_LEVEL_ERROR, "InitialJournalDirect: Can't find the journal file (%ws) - DeviceObject %p", CBT_JOURNAL_FILE1_, pdx->DeviceObject);
                                                status = STATUS_DATA_ERROR;
                                            }
                                        }
                                        else{
                                            DoTraceMsg(TRACE_LEVEL_ERROR, "InitialJournalDirect: Can't find the journal file (%ws) - DeviceObject %p", CBT_JOURNAL_FILE1_, pdx->DeviceObject);
                                            status = STATUS_DATA_ERROR;
                                        }
                                    }

                                    if (NT_SUCCESS(status)){
                                        pdx->Journal->Ready = TRUE;
                                        pdx->Journal->Initialized = TRUE;
                                        pdx->Journal->Check = TRUE;
                                        result = true;
                                        //LogEventMessage(pdx->DeviceObject, VCBT_JOURNAL_ENABLED, 1, pdx->PhysicalDeviceNameBuffer);
                                    }
                                    else{
                                        __free(pdx->Journal->UmapRetrievalPointers);
                                        __free(pdx->Journal->JournalRetrievalPointers);
                                        __free(pdx->Journal->JournalMetaDataRetrievalPointers);
                                        __free(pdx->Journal->JournalView);
                                    }
                                }
                            }
                        }
                    }
                    free_file_record(&sys_volume_info);
                }
            }
            free_file_record(&root);
        }
        free_ntfs_volume(&volume);
    }
    return result;
}

VOID     InitialJournalDirectPost(PDEVICE_EXTENSION pdx){
    KIRQL  oldIrql = PASSIVE_LEVEL;
    if (PASSIVE_LEVEL != (oldIrql = KeGetCurrentIrql())){
        KeLowerIrql(PASSIVE_LEVEL);
    }
    size_t len = 0;
    NTSTATUS Status = STATUS_SUCCESS;
    if (NULL != pdx->Journal &&
        (NT_SUCCESS(Status = RtlStringCbLengthW(pdx->MountDeviceLink, sizeof(pdx->MountDeviceLink), &len))) &&
        len > 0){

        if (NT_SUCCESS(Status = OpenUmapFile(pdx)) &&
            NT_SUCCESS(Status = get_fs_cluster_size(pdx->Journal->UmapFileHandle, &pdx->Journal->FsClusterSize, &pdx->Journal->BytesPerSector))){
        }

        if (NT_SUCCESS(Status)){
            RtlStringCchPrintfW(pdx->Journal->JournalMetaDataFileNameBuffer, VCBT_MAX_PATH, L"%s%s%s", pdx->MountDeviceLink, VCBT_VOLUME_SYS_INFO, CBT_JOURNAL_METADATA);
            RtlInitUnicodeString(&pdx->Journal->JournalMetaDataFileName, &pdx->Journal->JournalMetaDataFileNameBuffer[0]);
            RtlStringCchPrintfW(pdx->Journal->JournalFileNameBuffer, VCBT_MAX_PATH, L"%s%s%s", pdx->MountDeviceLink, VCBT_VOLUME_SYS_INFO, CBT_JOURNAL_FILE1);
            RtlInitUnicodeString(&pdx->Journal->JournalFileName, &pdx->Journal->JournalFileNameBuffer[0]);
            if (FALSE == _ControlDeviceExtension->JournalDisabled){
                
                Status = open_file(&pdx->Journal->JournalMetaDataFileHandle, &pdx->Journal->JournalMetaDataFileName);

                if (NT_SUCCESS(Status)){
                    Status = open_file(&pdx->Journal->JournalFileHandle, &pdx->Journal->JournalFileName);
                }
            }
            else{
                delete_file_ex(&pdx->Journal->JournalFileName);
            }
        }

        if (NT_SUCCESS(Status)){
            Status = GetFilesRetrievalPointers(pdx);
        }

        if (NT_SUCCESS(Status)){
            ProtectJournalFilesClusters(pdx);
            pdx->Journal->Ready = TRUE;
            LogEventMessage(pdx->DeviceObject, VCBT_JOURNAL_ENABLED, 1, pdx->PhysicalDeviceNameBuffer);
        }
        else{
            pdx->Journal->Ready = FALSE;
            LogEventMessage(pdx->DeviceObject, VCBT_JOURNAL_DISABLED, 1, pdx->PhysicalDeviceNameBuffer);
            if (pdx->Journal->UmapFileHandle != 0){
                delete_file(pdx->Journal->UmapFileHandle);
                _close_file(pdx->Journal->UmapFileHandle);
            }
            if (pdx->Journal->JournalMetaDataFileHandle != 0){
                delete_file(pdx->Journal->JournalMetaDataFileHandle);
                _close_file(pdx->Journal->JournalMetaDataFileHandle);
            }
            if (pdx->Journal->JournalFileHandle != 0){
                delete_file(pdx->Journal->JournalFileHandle);
                _close_file(pdx->Journal->JournalFileHandle);
            }
        }
        pdx->Journal->Check = FALSE;
        pdx->Journal->Initialized = TRUE;
    }
    if (PASSIVE_LEVEL != oldIrql)
        KeRaiseIrql(oldIrql, &oldIrql);
    PsTerminateSystemThread(STATUS_SUCCESS);
}

VOID     InitialJournal(PDEVICE_EXTENSION pdx){
    pdx->Journal = (PJOURNAL_DATA)ExAllocatePool(NonPagedPool, sizeof(JOURNAL_DATA));
    memset(pdx->Journal, 0, sizeof(JOURNAL_DATA));
    InitializeListHead(&pdx->Journal->WrittenList);
    lock_init(&pdx->Journal->Lock);
    HANDLE hthread;
    if (InitialJournalDirect(pdx)){
        PsCreateSystemThread(&hthread, THREAD_ALL_ACCESS,
            NULL, NULL, NULL, (PKSTART_ROUTINE)InitialJournalDirectPost, pdx);
    }
    else{
        PsCreateSystemThread(&hthread, THREAD_ALL_ACCESS,
            NULL, NULL, NULL, (PKSTART_ROUTINE)JournalInitialThreadProc, pdx);
    }
    ZwClose(hthread);
}

// from wdk wdm.h
#define SL_KEY_SPECIFIED                0x01
#define SL_OVERRIDE_VERIFY_VOLUME       0x02
#define SL_WRITE_THROUGH                0x04
#define SL_FT_SEQUENTIAL_WRITE          0x08
#define SL_FORCE_DIRECT_WRITE           0x10

NTSTATUS
FltReadWriteSectorsCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++
Routine Description:
A completion routine for use when calling the lower device objects to
which our filter deviceobject is attached.

Arguments:

DeviceObject - Pointer to deviceobject
Irp        - Pointer to a PnP Irp.
Context    - NULL or PKEVENT
Return Value:

NT Status is returned.

--*/
{
    PMDL    mdl;
    UNREFERENCED_PARAMETER(DeviceObject);

    // 
    // Free resources 
    // 

    if (Irp->AssociatedIrp.SystemBuffer && (Irp->Flags & IRP_DEALLOCATE_BUFFER)) {
        ExFreePool(Irp->AssociatedIrp.SystemBuffer);
    }

    if (Irp->IoStatus.Status){
        PIO_STACK_LOCATION currentIrpStack = IoGetCurrentIrpStackLocation(Irp);
        if (currentIrpStack->MajorFunction == IRP_MJ_READ)
            DoTraceMsg(TRACE_LEVEL_ERROR, "!!!!!!!!!!Read HD Error Code====0x%08x", Irp->IoStatus.Status);
        else
            DoTraceMsg(TRACE_LEVEL_ERROR, "!!!!!!!!!!Write HD Error Code====0x%08x", Irp->IoStatus.Status);
    }

    while (Irp->MdlAddress) {
        mdl = Irp->MdlAddress;
        Irp->MdlAddress = mdl->Next;
        MmUnlockPages(mdl);
        IoFreeMdl(mdl);
    }

    if (Irp->PendingReturned)
    {
        if (Irp->UserIosb && Context != NULL) 
        {
            *Irp->UserIosb = Irp->IoStatus;
            KeSetEvent((PKEVENT)Context, IO_DISK_INCREMENT, FALSE);
        }

        IoFreeIrp(Irp);
        // 
        // Don't touch irp any more 
        //
    }
    else
    {
        DoTraceMsg(TRACE_LEVEL_VERBOSE, "Irp->PendingReturned == FALSE");
    }
 
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS fastFsdRequest(
    IN PDEVICE_OBJECT DeviceObject,
    ULONG majorFunction,
    IN LONGLONG ByteOffset,
    IN OUT PVOID Buffer,
    IN ULONG Length,
    IN BOOLEAN Wait
    ){

    PIRP                irp;
    IO_STATUS_BLOCK     iosb;
    KEVENT              event;
    NTSTATUS            status;

    //
    irp = IoBuildAsynchronousFsdRequest(majorFunction, DeviceObject,
        Buffer, Length, (PLARGE_INTEGER)&ByteOffset, &iosb);
    if (!irp) {
        DoTraceMsg(TRACE_LEVEL_ERROR, "IoBuildAsynchronousFsdRequest 0x%x failed : STATUS_INSUFFICIENT_RESOURCES", majorFunction);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /*
    If the SL_FORCE_DIRECT_WRITE flag is set, kernel-mode drivers can write to volume areas that they
    normally cannot write to because of direct write blocking. Direct write blocking was implemented for
    security reasons in Windows Vista and later operating systems. This flag is checked both at the file
    system layer and storage stack layer. For more
    information about direct write blocking, see Blocking Direct Write Operations to Volumes and Disks.
    The SL_FORCE_DIRECT_WRITE flag is available in Windows Vista and later versions of Windows.
    http://msdn.microsoft.com/en-us/library/ms795960.aspx
    */
    
    if (IRP_MJ_WRITE == majorFunction){
        IoGetNextIrpStackLocation(irp)->Flags |= SL_FORCE_DIRECT_WRITE;
    }

    if (Wait) {        
        KeInitializeEvent(&event, NotificationEvent, FALSE);
        irp->Flags |= IRP_NOCACHE;
        IoSetCompletionRoutine(irp, FltReadWriteSectorsCompletion,
            &event, TRUE, TRUE, TRUE);
        if (STATUS_PENDING == IoCallDriver(DeviceObject, irp)){
            status = KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        }
        status = iosb.Status;
    }
    else {   
        IoSetCompletionRoutine(irp, FltReadWriteSectorsCompletion,
            NULL, TRUE, TRUE, TRUE);
        irp->UserIosb = NULL;
        status = IoCallDriver(DeviceObject, irp);
    }

    if (!NT_SUCCESS(status)){
        DoTraceMsg(TRACE_LEVEL_ERROR, "IoCallDriver 0x%x failed : 0x%08x", majorFunction, status);
    }

    return status;
}

NTSTATUS fastFsdRequest2(
    IN PDEVICE_OBJECT DeviceObject,
    ULONG majorFunction,
    IN LONGLONG ByteOffset,
    OUT PVOID Buffer,
    IN ULONG Length,
    IN BOOLEAN Wait){

    UNREFERENCED_PARAMETER(Wait);

    PIRP                irp;
    IO_STATUS_BLOCK     iosb;
    KEVENT              event;
    NTSTATUS            status;

    KeInitializeEvent(&event, NotificationEvent, FALSE);
    irp = IoBuildSynchronousFsdRequest(majorFunction, DeviceObject,
        Buffer, Length, (PLARGE_INTEGER)&ByteOffset, &event, &iosb);
    if (!irp) {
        DoTraceMsg(TRACE_LEVEL_ERROR, "IoBuildSynchronousFsdRequest 0x%x failed : STATUS_INSUFFICIENT_RESOURCES", majorFunction);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    if (IRP_MJ_WRITE == majorFunction){
        IoGetNextIrpStackLocation(irp)->Flags |= SL_FORCE_DIRECT_WRITE;
    }
    irp->Flags |= IRP_NOCACHE;
    if (STATUS_PENDING == IoCallDriver(DeviceObject, irp)) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
    }
    status = iosb.Status;
    if (!NT_SUCCESS(status)){
        if (majorFunction == IRP_MJ_READ)
            DoTraceMsg(TRACE_LEVEL_ERROR, "!!!!!!!!!!Read HD Error Code====0x%08x", status);
        else if (majorFunction == IRP_MJ_WRITE)
            DoTraceMsg(TRACE_LEVEL_ERROR, "!!!!!!!!!!Write HD Error Code====0x%08x", status);
    }
    return status;
}

VOID     FreeJournal(PDEVICE_EXTENSION pdx){
    pdx->Enabled = false;
    if ( NULL != pdx->Journal ){
        _close_file(pdx->Journal->JournalMetaDataFileHandle);
        _close_file(pdx->Journal->JournalFileHandle);
        _close_file(pdx->Journal->UmapFileHandle);
        lock_acquire(&pdx->Journal->Lock);
        PLIST_ENTRY e;
        while (&pdx->Journal->WrittenList != (e = RemoveHeadList(&pdx->Journal->WrittenList))){
            PIO_RANGE written = CONTAINING_RECORD(e, IO_RANGE, Bind);
            ExFreePool(written);
        }        
        __free(pdx->Journal->UmapRetrievalPointers);
        __free(pdx->Journal->JournalRetrievalPointers);
        __free(pdx->Journal->JournalMetaDataRetrievalPointers);
        __free(pdx->Journal->NewUmapRetrievalPointers);
        __free(pdx->Journal->NewJournalRetrievalPointers);
        __free(pdx->Journal->NewJournalMetaDataRetrievalPointers);
        __free(pdx->Journal->JournalView);
        lock_release(&pdx->Journal->Lock);
        __free(pdx->Journal);
    }
}
#define USE_PERF_CTR
#ifdef USE_PERF_CTR
#define _GetClock(a, b) (a) = KeQueryPerformanceCounter((b))
#else
#define _GetClock(a, b) KeQuerySystemTime(&(a))
#endif

NTSTATUS EnableJournal(PDEVICE_EXTENSION pdx, PLONG sizeInMegaBytes, LONGLONG* JournalId){
    KIRQL  oldIrql = PASSIVE_LEVEL;
    if (PASSIVE_LEVEL != (oldIrql = KeGetCurrentIrql())){
        KeLowerIrql(PASSIVE_LEVEL);
    }
    int umapcount = 1;
    NTSTATUS Status = STATUS_INVALID_PARAMETER;
    WCHAR buff[VCBT_MAX_PATH];
    memset(buff, 0, sizeof(buff));
    UNICODE_STRING  path;
    HANDLE          jhandle = 0, jmhandle = 0, mhandle = 0;
    RtlStringCchPrintfW(buff, VCBT_MAX_PATH, L"%s%s", pdx->MountDeviceLink, VCBT_VOLUME_SYS_INFO);
    RtlInitUnicodeString(&path, &buff[0]);
    LARGE_INTEGER length;
    LARGE_INTEGER pointer;
    FILE_BASIC_INFORMATION fbi;
    if (NT_SUCCESS(Status = get_volume_length(pdx->DeviceObject, &length))){
        int resolution = UMAP_RESOLUTION1;
        while ((length.QuadPart >> (resolution + 19)) > 0){
            umapcount++;
            resolution++;
        }
        if (NT_SUCCESS(Status = create_directory(&path))){
            memset(buff, 0, sizeof(buff));
            RtlStringCchPrintfW(buff, VCBT_MAX_PATH, L"%s%s%s", pdx->MountDeviceLink, VCBT_VOLUME_SYS_INFO, CBT_JOURNAL_FILE1);
            RtlInitUnicodeString(&path, &buff[0]);
            size_t temp_size = JOURNAL_MIN_SIZE_IN_BYTES;
            size_t count;
            if (NULL != sizeInMegaBytes && *sizeInMegaBytes != 0 ){
                count = *sizeInMegaBytes;
            }
            else{
                count = (size_t)(length.QuadPart >> 27);
                if (count > JOURNAL_MAX_SIZE_IN_MB)
                    count = JOURNAL_MAX_SIZE_IN_MB;
                else if (count < 1)
                    count = 1;
                *sizeInMegaBytes = (LONG)count;
            }
            BYTE* umap = ExAllocatePool(NonPagedPool, temp_size);
            if (NULL != umap){
                memset(umap, 0, temp_size);
                if ((FALSE == _ControlDeviceExtension->JournalDisabled) && NT_SUCCESS(Status = create_file_with_arrtibutes(&jhandle, &path, FILE_ATTRIBUTE_HIDDEN))){
                    if (NT_SUCCESS(get_file_base_info(jhandle, &fbi))){
                        fbi.FileAttributes |= FILE_ATTRIBUTE_NOT_CONTENT_INDEXED;
                        set_file_base_info(jhandle, &fbi);
                    }
                    pointer.QuadPart = count * 1024 * 1024;
                    set_end_of_file(jhandle, &pointer);
                    pointer.QuadPart = 0;
                    set_file_pointer(jhandle, &pointer);
                    VCBT_JOURNAL_META_DATA metadata;
                    LARGE_INTEGER timestamp;
                    _GetClock(timestamp, NULL);
                    memset(&metadata, 0, sizeof(VCBT_JOURNAL_META_DATA));
                    timestamp.HighPart = RtlRandomEx((PULONG)&timestamp.HighPart);
                    timestamp.LowPart  = RtlRandomEx((PULONG)&timestamp.LowPart);
                    metadata.Block.j.Size = (ULONG)(temp_size * count);
                    metadata.Block.j.JournalId = timestamp.QuadPart;
                    if (NULL != JournalId)
                        *JournalId = timestamp.QuadPart;
                    metadata.Block.j.Signature[0] = '1';
                    metadata.Block.j.Signature[1] = 'c';
                    metadata.Block.j.Signature[2] = 'b';
                    metadata.Block.j.Signature[3] = 't';
                    metadata.Block.j.CheckSum = (ULONG)-1;
                    size_t i = 0;
                    while (i < count){
                        memset(umap, 0, temp_size);
                        if (0 == i) {
                            memcpy(umap, &metadata.Block, sizeof(VCBT_JOURNAL_BLOK));
                        }
                        if (!NT_SUCCESS(Status = write_file(jhandle, umap, (UINT32)temp_size, i * temp_size)))
                            break;
                        i++;
                    }
                    _close_file(jhandle);
                    if (NT_SUCCESS(Status)){
                        DoTraceMsg(TRACE_LEVEL_VERBOSE, "EnableJournal: Create The Journal File(%wZ) Size: %d - DeviceObject %p", &path, metadata.Block.j.Size, pdx->DeviceObject);                    
                        RtlStringCchPrintfW(buff, VCBT_MAX_PATH, L"%s%s%s", pdx->MountDeviceLink, VCBT_VOLUME_SYS_INFO, CBT_JOURNAL_METADATA);
                        RtlInitUnicodeString(&path, &buff[0]);
                        if (NT_SUCCESS(Status = create_file_with_arrtibutes(&jmhandle, &path, FILE_ATTRIBUTE_HIDDEN))){
                            if (NT_SUCCESS(get_file_base_info(jmhandle, &fbi))){
                                fbi.FileAttributes |= FILE_ATTRIBUTE_NOT_CONTENT_INDEXED;
                                set_file_base_info(jmhandle, &fbi);
                            }
                            pointer.QuadPart = RESOLUTION_UMAP_SIZE;
                            set_end_of_file(jmhandle, &pointer);
                            pointer.QuadPart = 0;
                            set_file_pointer(jmhandle, &pointer);
                            DoTraceMsg(TRACE_LEVEL_VERBOSE, "EnableJournal: Create The Journal metadata File(%wZ) - DeviceObject %p", &path, pdx->DeviceObject);
                            memset(umap, 0, RESOLUTION_UMAP_SIZE);
                            memcpy(umap, &metadata, sizeof(VCBT_JOURNAL_META_DATA));
                            if (NT_SUCCESS(Status = write_file(jmhandle, umap, RESOLUTION_UMAP_SIZE, 0))){
                            }
                            _close_file(jmhandle);
                        }
                    }
                }
                if (NT_SUCCESS(Status)){
                    RtlStringCchPrintfW(buff, VCBT_MAX_PATH, L"%s%s"UMAP_RESOLUTION_FILE, pdx->MountDeviceLink, VCBT_VOLUME_SYS_INFO, umapcount);
                    RtlInitUnicodeString(&path, &buff[0]);
                    if (NT_SUCCESS(Status = create_file_with_arrtibutes(&mhandle, &path, FILE_ATTRIBUTE_HIDDEN))){
                        if (NT_SUCCESS(get_file_base_info(mhandle, &fbi))){
                            fbi.FileAttributes |= FILE_ATTRIBUTE_NOT_CONTENT_INDEXED;
                            set_file_base_info(mhandle, &fbi);
                        }
                        pointer.QuadPart = RESOLUTION_UMAP_SIZE;
                        set_end_of_file(mhandle, &pointer);
                        pointer.QuadPart = 0;
                        set_file_pointer(mhandle, &pointer);
                        DoTraceMsg(TRACE_LEVEL_VERBOSE, "EnableJournal: Create The Umap File(%wZ) - DeviceObject %p", &path, pdx->DeviceObject);
                        memset(umap, 0, RESOLUTION_UMAP_SIZE);
                        if (NT_SUCCESS(Status = write_file(mhandle, umap, RESOLUTION_UMAP_SIZE, 0))){
                        }
                        _close_file(mhandle);
                    }
                }
                
                if (!NT_SUCCESS(Status)){
                    RtlStringCchPrintfW(buff, VCBT_MAX_PATH, L"%s%s%s", pdx->MountDeviceLink, VCBT_VOLUME_SYS_INFO, CBT_JOURNAL_METADATA);
                    RtlInitUnicodeString(&path, &buff[0]);
                    delete_file_ex(&path);

                    RtlStringCchPrintfW(buff, VCBT_MAX_PATH, L"%s%s%s", pdx->MountDeviceLink, VCBT_VOLUME_SYS_INFO, CBT_JOURNAL_FILE1);
                    RtlInitUnicodeString(&path, &buff[0]);
                    delete_file_ex(&path);

                    RtlStringCchPrintfW(buff, VCBT_MAX_PATH, L"%s%s"UMAP_RESOLUTION_FILE, pdx->MountDeviceLink, VCBT_VOLUME_SYS_INFO, umapcount);
                    RtlInitUnicodeString(&path, &buff[0]);
                    delete_file_ex(&path);
                }
                ExFreePool(umap);
            }
            else{
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }
    }

    if (PASSIVE_LEVEL != oldIrql)
        KeRaiseIrql(oldIrql, &oldIrql);
    return Status;
}

NTSTATUS DisableJournal(PDEVICE_EXTENSION pdx){
    NTSTATUS Status = STATUS_INVALID_PARAMETER;
    size_t len = 0;
    NTSTATUS Statuc;
    if( (NT_SUCCESS(Statuc = RtlStringCbLengthW(pdx->MountDeviceLink, sizeof(pdx->MountDeviceLink), &len))) &&
        len > 0){
        KIRQL  oldIrql = PASSIVE_LEVEL;
        if (PASSIVE_LEVEL != (oldIrql = KeGetCurrentIrql())){
            KeLowerIrql(PASSIVE_LEVEL);
        }
        WCHAR buff[VCBT_MAX_PATH];
        memset(buff, 0, sizeof(buff));
        UNICODE_STRING  path;
        RtlStringCchPrintfW(buff, VCBT_MAX_PATH, L"%s%s%s", pdx->MountDeviceLink, VCBT_VOLUME_SYS_INFO, CBT_JOURNAL_FILE1);
        RtlInitUnicodeString(&path, &buff[0]);
        delete_file_ex(&path);

        RtlStringCchPrintfW(buff, VCBT_MAX_PATH, L"%s%s%s", pdx->MountDeviceLink, VCBT_VOLUME_SYS_INFO, CBT_JOURNAL_METADATA);
        RtlInitUnicodeString(&path, &buff[0]);
        delete_file_ex(&path);

        LARGE_INTEGER length;
        if (NT_SUCCESS(Status = get_volume_length(pdx->DeviceObject, &length))){           
            int resolution = UMAP_RESOLUTION1;
            int count = 1;
            while ((length.QuadPart >> (resolution + 19)) > 0){
                count++;
                resolution++;
            }
            memset(buff, 0, sizeof(buff));
            RtlStringCchPrintfW(buff, VCBT_MAX_PATH, L"%s%s"UMAP_RESOLUTION_FILE, pdx->MountDeviceLink, VCBT_VOLUME_SYS_INFO, count);
            RtlInitUnicodeString(&path, &buff[0]);
            delete_file_ex(&path);
        }
        else{
            for (int count = 1; count < 10; count++){
                memset(buff, 0, sizeof(buff));
                RtlStringCchPrintfW(buff, VCBT_MAX_PATH, L"%s%s"UMAP_RESOLUTION_FILE, pdx->MountDeviceLink, VCBT_VOLUME_SYS_INFO, count);
                RtlInitUnicodeString(&path, &buff[0]);
                delete_file_ex(&path);
            }
        }
        LogEventMessage(pdx->DeviceObject, VCBT_JOURNAL_DISABLED, 1, pdx->PhysicalDeviceNameBuffer);
        if (PASSIVE_LEVEL != oldIrql)
            KeRaiseIrql(oldIrql, &oldIrql);
    }
    Status = STATUS_SUCCESS;
    return Status;
}

void Sleep(unsigned ms)
{
    LARGE_INTEGER TimerMs;

    TimerMs.QuadPart = (__int64)(-1 * 1000 * 10) * (__int64)ms;

    if (KeGetCurrentIrql() <= APC_LEVEL)
    {
        KeDelayExecutionThread(KernelMode, FALSE, &TimerMs);
    }
    else
    {
        while (ms > 50)
        {
            KeStallExecutionProcessor(50);
            ms -= 50;
        }
        KeStallExecutionProcessor(ms);
    }
}

VOID     CheckJournalThreadProc(PDEVICE_EXTENSION pdx){
    KIRQL  oldIrql = PASSIVE_LEVEL;
    if (PASSIVE_LEVEL != (oldIrql = KeGetCurrentIrql())){
        KeLowerIrql(PASSIVE_LEVEL);
    }
    DoTraceMsg(TRACE_LEVEL_VERBOSE, "CheckJournalThreadProc: Entry - DeviceObject %p", pdx->DeviceObject);
    size_t len = 0;
    NTSTATUS Status = STATUS_SUCCESS;
    if (NULL != pdx->Journal &&
        (NT_SUCCESS(Status = RtlStringCbLengthW(pdx->MountDeviceLink, sizeof(pdx->MountDeviceLink), &len))) &&
        len > 0){
        while (NULL != pdx->Journal && pdx->Journal->Check == TRUE){
            _close_file(pdx->Journal->JournalMetaDataFileHandle);
            _close_file(pdx->Journal->JournalFileHandle);
            _close_file(pdx->Journal->UmapFileHandle);         
            if (FALSE == _ControlDeviceExtension->JournalDisabled){
                Status = open_file(&pdx->Journal->JournalFileHandle, &pdx->Journal->JournalFileName);
                if (NT_SUCCESS(Status)){
                    Status = open_file(&pdx->Journal->JournalMetaDataFileHandle, &pdx->Journal->JournalMetaDataFileName);
                    if (!NT_SUCCESS(Status)){
                        _close_file(pdx->Journal->JournalMetaDataFileHandle);
                    }
                }
            }
            if (NT_SUCCESS(Status)){
                if (!NT_SUCCESS(Status = open_file(&pdx->Journal->UmapFileHandle, &pdx->Journal->UmapFileName))){
                    _close_file(pdx->Journal->JournalFileHandle);
                    _close_file(pdx->Journal->JournalMetaDataFileHandle);
                }
                else{
                    Status = GetFilesRetrievalPointers(pdx);
                }
            }
            if (NT_SUCCESS(Status)){
                ProtectJournalFilesClusters(pdx);
                pdx->Journal->Check = FALSE;
                break;
            }
            else if (Status == STATUS_OBJECT_NAME_NOT_FOUND || Status == STATUS_OBJECT_PATH_NOT_FOUND){
                pdx->Journal->Ready = FALSE;
                pdx->Journal->Check = FALSE;
                break;
            }
            Sleep(500);
        }
    }
    DoTraceMsg(TRACE_LEVEL_VERBOSE, "CheckJournalThreadProc: Leave - DeviceObject %p", pdx->DeviceObject);
    if (PASSIVE_LEVEL != oldIrql)
        KeRaiseIrql(oldIrql, &oldIrql);
    PsTerminateSystemThread(STATUS_SUCCESS);
}

VOID     CheckJournal(PDEVICE_EXTENSION pdx){
    if (NULL != pdx->Journal){
        if ((TRUE == pdx->Journal->Initialized) &&
            (TRUE == pdx->Journal->Ready)){
            BYTE buff[1];
            NTSTATUS Status = STATUS_SUCCESS;
            if ((!NT_SUCCESS(Status = read_file(pdx->Journal->JournalFileHandle, buff, sizeof(buff), 0))) ||
                (!NT_SUCCESS(Status = read_file(pdx->Journal->UmapFileHandle, buff, sizeof(buff), 0)))){
                if (STATUS_VOLUME_DISMOUNTED == Status){
                    if (pdx->CheckThread != NULL){
                        ObDereferenceObject(pdx->CheckThread);
                        pdx->CheckThread = NULL;
                    }
                    pdx->Journal->Check = TRUE;
                    HANDLE hthread;
                    Status = PsCreateSystemThread(&hthread, THREAD_ALL_ACCESS,
                        NULL, NULL, &pdx->CheckThreadId, (PKSTART_ROUTINE)CheckJournalThreadProc, pdx);
                    if (!NT_SUCCESS(Status)){
                        pdx->Journal->Ready = FALSE;
                        pdx->Journal->Check = FALSE;
                        return;
                    }
                    ObReferenceObjectByHandle(hthread, THREAD_ALL_ACCESS, NULL,
                        KernelMode, (PVOID*)&pdx->CheckThread, NULL);
                    ZwClose(hthread);
                }
                else{            
                    pdx->Journal->Ready = FALSE;
                    pdx->Journal->Check = FALSE;
                }
            }
        }
    }
}

NTSTATUS GetFilesRetrievalPointers(PDEVICE_EXTENSION pdx){
    
    NTSTATUS Status = STATUS_SUCCESS;
    IO_STATUS_BLOCK             IoStatusBlock;
    LONG                        fsInfoSize = sizeof(FILE_FS_ATTRIBUTE_INFORMATION) + 64;
    PFILE_FS_ATTRIBUTE_INFORMATION fsInfo = (PFILE_FS_ATTRIBUTE_INFORMATION)ExAllocatePool(NonPagedPool, fsInfoSize);
    LARGE_INTEGER               liVcnPrev;

    if (NULL != fsInfo){
        memset(fsInfo, 0, fsInfoSize);
        if (NT_SUCCESS(ZwQueryVolumeInformationFile(pdx->Journal->UmapFileHandle,
            &IoStatusBlock,
            fsInfo,
            fsInfoSize,
            FileFsAttributeInformation))){
            ULONG nameSize = min(fsInfo->FileSystemNameLength, sizeof(pdx->FileSystemName) - sizeof(WCHAR));
            memcpy(pdx->FileSystemName, fsInfo->FileSystemName, nameSize);
            DoTraceMsg(TRACE_LEVEL_VERBOSE, "GetFilesRetrievalPointers: File System Name : %ws - DeviceObject %p", pdx->FileSystemName, pdx->DeviceObject);
        }
        ExFreePool(fsInfo);
    }

    if (NT_SUCCESS(Status = get_fs_cluster_size(pdx->Journal->UmapFileHandle, &pdx->Journal->FsClusterSize, &pdx->Journal->BytesPerSector))){   
        if (RtlIsNtDdiVersionAvailable(NTDDI_WIN7)){
            HANDLE volume;
            UNICODE_STRING path;
            RtlInitUnicodeString(&path, &pdx->MountDeviceLink[0]);
            if (NT_SUCCESS(open_file_read_only(&volume, &path))){
                get_retrieval_pointer_base(volume, pdx->Journal->BytesPerSector, &pdx->Journal->FileAreaOffset);
                close_file(volume);
            }
        }
        else
            get_fat_first_sector_offset(pdx->TargetDeviceObject, &pdx->Journal->FileAreaOffset);

        PRETRIEVAL_POINTERS_BUFFER       JournalMetaDataRetrievalPointers = NULL;
        PRETRIEVAL_POINTERS_BUFFER       JournalRetrievalPointers = NULL;
        PRETRIEVAL_POINTERS_BUFFER       UmapRetrievalPointers = NULL;

        if (FALSE == _ControlDeviceExtension->JournalDisabled &&
            NT_SUCCESS(Status = get_file_retrieval_pointers(pdx->Journal->JournalFileHandle, 0, &JournalRetrievalPointers))){
            liVcnPrev = JournalRetrievalPointers->StartingVcn;
            for (DWORD extent = 0; extent < JournalRetrievalPointers->ExtentCount; extent++){
                LONGLONG _Start = liVcnPrev.QuadPart * pdx->Journal->FsClusterSize;
                LONG     _Length = (ULONG)(JournalRetrievalPointers->Extents[extent].NextVcn.QuadPart - liVcnPrev.QuadPart) * pdx->Journal->FsClusterSize;
                LONGLONG _Lcn = JournalRetrievalPointers->Extents[extent].Lcn.QuadPart * pdx->Journal->FsClusterSize;
                DoTraceMsg(TRACE_LEVEL_VERBOSE, "GetFilesRetrievalPointers: Journal file location (start %I64u, lcn %I64u, length %d) - DeviceObject %p ", _Start, _Lcn, _Length, pdx->DeviceObject);
                liVcnPrev = JournalRetrievalPointers->Extents[extent].NextVcn;
            }
            if (NT_SUCCESS(Status = get_file_retrieval_pointers(pdx->Journal->JournalMetaDataFileHandle, 0, &JournalMetaDataRetrievalPointers))){
                liVcnPrev = JournalMetaDataRetrievalPointers->StartingVcn;
                for (DWORD extent = 0; extent < JournalMetaDataRetrievalPointers->ExtentCount; extent++){
                    LONGLONG _Start = liVcnPrev.QuadPart * pdx->Journal->FsClusterSize;
                    LONG     _Length = (ULONG)(JournalMetaDataRetrievalPointers->Extents[extent].NextVcn.QuadPart - liVcnPrev.QuadPart) * pdx->Journal->FsClusterSize;
                    LONGLONG _Lcn = JournalMetaDataRetrievalPointers->Extents[extent].Lcn.QuadPart * pdx->Journal->FsClusterSize;
                    DoTraceMsg(TRACE_LEVEL_VERBOSE, "GetFilesRetrievalPointers: Journal metadata file location (start %I64u, lcn %I64u, length %d) - DeviceObject %p ", _Start, _Lcn, _Length, pdx->DeviceObject);
                    liVcnPrev = JournalMetaDataRetrievalPointers->Extents[extent].NextVcn;
                }
            }
        }
        if (NT_SUCCESS(Status) &&
            NT_SUCCESS(Status = get_file_retrieval_pointers(pdx->Journal->UmapFileHandle, 0, &UmapRetrievalPointers))){
            liVcnPrev = UmapRetrievalPointers->StartingVcn;
            for (DWORD extent = 0; extent < UmapRetrievalPointers->ExtentCount; extent++){
                LONGLONG _Start = liVcnPrev.QuadPart * pdx->Journal->FsClusterSize;
                LONG     _Length = (ULONG)(UmapRetrievalPointers->Extents[extent].NextVcn.QuadPart - liVcnPrev.QuadPart) * pdx->Journal->FsClusterSize;
                LONGLONG _Lcn = UmapRetrievalPointers->Extents[extent].Lcn.QuadPart * pdx->Journal->FsClusterSize;
                DoTraceMsg(TRACE_LEVEL_VERBOSE, "GetFilesRetrievalPointers: Umap file location (start %I64u, lcn %I64u, length %d) - DeviceObject %p ", _Start, _Lcn, _Length, pdx->DeviceObject);
                liVcnPrev = UmapRetrievalPointers->Extents[extent].NextVcn;
            }
        }
        lock_acquire(&pdx->Journal->Lock);
        if (NT_SUCCESS(Status)){
            __free(pdx->Journal->NewUmapRetrievalPointers);
            __free(pdx->Journal->NewJournalRetrievalPointers);
            __free(pdx->Journal->NewJournalMetaDataRetrievalPointers);
            pdx->Journal->NewUmapRetrievalPointers = UmapRetrievalPointers;
            pdx->Journal->NewJournalRetrievalPointers = JournalRetrievalPointers;
            pdx->Journal->NewJournalMetaDataRetrievalPointers = JournalMetaDataRetrievalPointers;
        }
        else{
            __free(UmapRetrievalPointers);
            __free(JournalRetrievalPointers);
            __free(JournalMetaDataRetrievalPointers);
            __free(pdx->Journal->NewUmapRetrievalPointers);
            __free(pdx->Journal->NewJournalRetrievalPointers);
            __free(pdx->Journal->NewJournalMetaDataRetrievalPointers);
        }
        pdx->Journal->NeedUpdateRetrievalPointers = TRUE;
        lock_release(&pdx->Journal->Lock);
    }
    return Status;
}

NTSTATUS ReadUmapData(PDEVICE_EXTENSION pdx, BYTE* buff, LONGLONG offset, LONG length){
    NTSTATUS Status = STATUS_INVALID_PARAMETER;
    LARGE_INTEGER liVcnPrev = pdx->Journal->UmapRetrievalPointers->StartingVcn;
    LONGLONG _start = (offset) / pdx->Journal->FsClusterSize;
    LONG     _offset = 0;
    LONGLONG _end = ((offset + length - 1)) / pdx->Journal->FsClusterSize;
    for (DWORD extent = 0; extent < pdx->Journal->UmapRetrievalPointers->ExtentCount; extent++){
        LONGLONG _Start = liVcnPrev.QuadPart;
        LONG     _Length = (ULONG)(pdx->Journal->UmapRetrievalPointers->Extents[extent].NextVcn.QuadPart - liVcnPrev.QuadPart);
        LONGLONG _End = _Start + _Length - 1;
        if ((_Start <= _start) && (_start <= _End)){
            LONG _length = ((LONG)(((_end > _End) ? _End : _end) - _start + 1)) * pdx->Journal->FsClusterSize;
            LONGLONG _Lcn = (pdx->Journal->UmapRetrievalPointers->Extents[extent].Lcn.QuadPart + (_start - _Start))* pdx->Journal->FsClusterSize;
            _start = _start * (LONGLONG)pdx->Journal->FsClusterSize;
            _Lcn += pdx->Journal->FileAreaOffset;
            Status = fastFsdRequest(pdx->TargetDeviceObject, IRP_MJ_READ, _Lcn, &buff[_offset], _length, TRUE);
            if (!NT_SUCCESS(Status)){
                WCHAR start_[VCBT_MAX_NUM_STR];
                WCHAR length_[VCBT_MAX_NUM_STR];
                memset(start_, 0, sizeof(start_));
                memset(length_, 0, sizeof(length_));
                RtlStringCchPrintfW(start_, VCBT_MAX_NUM_STR, L"%I64u", _Lcn);
                RtlStringCchPrintfW(length_, VCBT_MAX_NUM_STR, L"%d", _length);
                LogEventMessage(pdx->DeviceObject, VCBT_FAILED_TO_READ_UMAP, 3, pdx->PhysicalDeviceNameBuffer, start_, length_);
                DoTraceMsg(TRACE_LEVEL_ERROR, "ReadUmapData : Failed to read umap data. (start %I64u, lcn %I64u, length %d) - (IO: offset %I64u, length %d) - DeviceObject %p ", _start, _Lcn, _length, offset, length, pdx->DeviceObject);
                break;
            }
            else{
                DoTraceMsg(TRACE_LEVEL_VERBOSE, "ReadUmapData : Succeeded to read umap data (start %I64u, lcn %I64u, length %d) - (IO: offset %I64u, length %d) - DeviceObject %p ", _start, _Lcn, _length, offset, length, pdx->DeviceObject);
            }
            if (_end <= _End)
                break;
            _offset += _length;
            _start = pdx->Journal->UmapRetrievalPointers->Extents[extent].NextVcn.QuadPart;
        }
        liVcnPrev = pdx->Journal->UmapRetrievalPointers->Extents[extent].NextVcn;
    }
    return Status;
}

NTSTATUS FlushUmapData(PDEVICE_EXTENSION pdx, LONGLONG offset, LONG length, BOOLEAN full){
    NTSTATUS Status = STATUS_INVALID_PARAMETER;
    if (NULL != pdx->Journal){
        if (FALSE == _ControlDeviceExtension->UmapFlushDisabled){
            if (NULL != pdx->Journal->UmapRetrievalPointers){
                if (full){
                    LARGE_INTEGER liVcnPrev = pdx->Journal->UmapRetrievalPointers->StartingVcn;
                    for (DWORD extent = 0; extent < pdx->Journal->UmapRetrievalPointers->ExtentCount; extent++){
                        LONGLONG _Start = liVcnPrev.QuadPart * pdx->Journal->FsClusterSize;
                        LONG     _Length = (ULONG)(pdx->Journal->UmapRetrievalPointers->Extents[extent].NextVcn.QuadPart - liVcnPrev.QuadPart) * pdx->Journal->FsClusterSize;
                        LONGLONG _Lcn = pdx->Journal->UmapRetrievalPointers->Extents[extent].Lcn.QuadPart * pdx->Journal->FsClusterSize;
                        _Lcn += pdx->Journal->FileAreaOffset;
                        Status = fastFsdRequest2(pdx->TargetDeviceObject, IRP_MJ_WRITE, _Lcn, &pdx->Journal->UMAP[_Start], _Length, TRUE);
                        if (!NT_SUCCESS(Status)){
                            WCHAR start_[VCBT_MAX_NUM_STR];
                            WCHAR length_[VCBT_MAX_NUM_STR];
                            memset(start_, 0, sizeof(start_));
                            memset(length_, 0, sizeof(length_));
                            RtlStringCchPrintfW(start_, VCBT_MAX_NUM_STR, L"%I64u", _Lcn);
                            RtlStringCchPrintfW(length_, VCBT_MAX_NUM_STR, L"%d", _Length);
                            LogEventMessage(pdx->DeviceObject, VCBT_FAILED_TO_FLUSH_UMAP, 3, pdx->PhysicalDeviceNameBuffer, start_, length_);
                            DoTraceMsg(TRACE_LEVEL_ERROR, "FlushUmapData : Failed to full flush umap data. (start %I64u, lcn %I64u, length %d) - DeviceObject %p ", _Start, _Lcn, _Length, pdx->DeviceObject);
                            break;
                        }
                        else{
                            DoTraceMsg(TRACE_LEVEL_VERBOSE, "FlushUmapData : Succeeded to full flush umap data(start %I64u, lcn %I64u, length %d) - DeviceObject %p ", _Start, _Lcn, _Length, pdx->DeviceObject);
                        }
                        liVcnPrev = pdx->Journal->UmapRetrievalPointers->Extents[extent].NextVcn;
                    }
                }
                else{
                    LARGE_INTEGER liVcnPrev = pdx->Journal->UmapRetrievalPointers->StartingVcn;
                    LONG resolution = pdx->Journal->Resolution + 3;
                    LONGLONG _start = ((offset >> resolution)) / pdx->Journal->FsClusterSize;
                    LONGLONG _end = ((offset + length - 1) >> resolution) / pdx->Journal->FsClusterSize;

                    for (DWORD extent = 0; extent < pdx->Journal->UmapRetrievalPointers->ExtentCount; extent++){
                        LONGLONG _Start = liVcnPrev.QuadPart;
                        LONG     _Length = (ULONG)(pdx->Journal->UmapRetrievalPointers->Extents[extent].NextVcn.QuadPart - liVcnPrev.QuadPart);
                        LONGLONG _End = _Start + _Length - 1;
                        if ((_Start <= _start) && (_start <= _End)){
                            LONG _length = ((LONG)(((_end > _End) ? _End : _end) - _start + 1)) * pdx->Journal->FsClusterSize;
                            LONGLONG _Lcn = (pdx->Journal->UmapRetrievalPointers->Extents[extent].Lcn.QuadPart + (_start - _Start))* pdx->Journal->FsClusterSize;
                            _start *= pdx->Journal->FsClusterSize;
                            _Lcn += pdx->Journal->FileAreaOffset;
                            Status = fastFsdRequest2(pdx->TargetDeviceObject, IRP_MJ_WRITE, _Lcn, &pdx->Journal->UMAP[_start], _length, TRUE);
                            if (!NT_SUCCESS(Status)){
                                WCHAR start_[VCBT_MAX_NUM_STR];
                                WCHAR length_[VCBT_MAX_NUM_STR];
                                memset(start_, 0, sizeof(start_));
                                memset(length_, 0, sizeof(length_));
                                RtlStringCchPrintfW(start_, VCBT_MAX_NUM_STR, L"%I64u", _Lcn);
                                RtlStringCchPrintfW(length_, VCBT_MAX_NUM_STR, L"%d", _Length);
                                LogEventMessage(pdx->DeviceObject, VCBT_FAILED_TO_FLUSH_UMAP, 3, pdx->PhysicalDeviceNameBuffer, start_, length_);
                                DoTraceMsg(TRACE_LEVEL_ERROR, "FlushUmapData : Failed to flush umap data. (UMAP: start %I64u, lcn %I64u, length %d) - (IO: offset %I64u, length %d) - DeviceObject %p ", _start, _Lcn, _length, offset, length, pdx->DeviceObject);
                                break;
                            }
                            else{
                                DoTraceMsg(TRACE_LEVEL_VERBOSE, "FlushUmapData : Succeeded to flush umap data(UMAP: start %I64u, lcn %I64u, length %d) - (IO: offset %I64u, length %d) - DeviceObject %p ", _start, _Lcn, _length, offset, length, pdx->DeviceObject);
                            }
                            if (_end <= _End)
                                break;
                            _start = pdx->Journal->UmapRetrievalPointers->Extents[extent].NextVcn.QuadPart;
                        }
                        liVcnPrev = pdx->Journal->UmapRetrievalPointers->Extents[extent].NextVcn;
                    }
                }
            }
        }
        else{
            Status = STATUS_SUCCESS;
        }
    }
    return Status;
}

NTSTATUS OpenUmapFile(PDEVICE_EXTENSION pdx){
    LARGE_INTEGER length;
    NTSTATUS      status;
    if (NT_SUCCESS(status = get_volume_length(pdx->DeviceObject, &length))){
        int resolution = UMAP_RESOLUTION1;
        int count = 1;
        while ((length.QuadPart >> ( resolution + 19 ) ) > 0){
            count++;
            resolution++;
        }
        RtlStringCchPrintfW(pdx->Journal->UmapFileNameBuffer, VCBT_MAX_PATH, L"%s%s"UMAP_RESOLUTION_FILE, pdx->MountDeviceLink, VCBT_VOLUME_SYS_INFO, count);
        RtlInitUnicodeString(&pdx->Journal->UmapFileName, &pdx->Journal->UmapFileNameBuffer[0]);
        pdx->Journal->Resolution = resolution;
        status = open_file(&pdx->Journal->UmapFileHandle, &pdx->Journal->UmapFileName);
    }
    return status;
}

NTSTATUS InitialJournalData(PDEVICE_EXTENSION pdx){
    NTSTATUS Status = STATUS_INVALID_PARAMETER;
    if (NULL != pdx->Journal){
        ULONGLONG offset = 0;
        bool found = false;
        pdx->Journal->JournalViewSize = pdx->Journal->FsClusterSize * 2;
        pdx->Journal->JournalView = (BYTE*)__malloc(pdx->Journal->JournalViewSize);
        if (NULL != pdx->Journal->JournalView){         
            if (NT_SUCCESS(Status = read_file(pdx->Journal->JournalMetaDataFileHandle, &pdx->Journal->JournalMetaData, sizeof(pdx->Journal->JournalMetaData), 0))){
                if ((pdx->Journal->JournalMetaData.Block.j.CheckSum == (ULONG)-1) &&
                    (pdx->Journal->JournalMetaData.Block.j.Signature[0] == '1') &&
                    (pdx->Journal->JournalMetaData.Block.j.Signature[1] == 'c') &&
                    (pdx->Journal->JournalMetaData.Block.j.Signature[2] == 'b') &&
                    (pdx->Journal->JournalMetaData.Block.j.Signature[3] == 't')){
                    DoTraceMsg(TRACE_LEVEL_VERBOSE, "InitialJournalData : metadata file : Latest Key : %I64u, First Key : %I64u - DeviceObject %p", pdx->Journal->JournalMetaData.Block.r.Key, pdx->Journal->JournalMetaData.FirstKey, pdx->DeviceObject);
                    if (pdx->Journal->JournalMetaData.Block.r.Key >= pdx->Journal->JournalMetaData.FirstKey)
                        offset = (pdx->Journal->JournalMetaData.Block.r.Key - pdx->Journal->JournalMetaData.FirstKey) * sizeof(VCBT_RECORD);
                    else{
                        offset = (((ULONGLONG)-1) - pdx->Journal->JournalMetaData.FirstKey + pdx->Journal->JournalMetaData.Block.r.Key) * sizeof(VCBT_RECORD);
                    }
                    pdx->Journal->JournalViewOffset = (offset / pdx->Journal->FsClusterSize)* pdx->Journal->FsClusterSize;
                    if (pdx->Journal->JournalViewOffset > 0)
                        offset = sizeof(VCBT_RECORD) - (pdx->Journal->JournalViewOffset % sizeof(VCBT_RECORD));
                    else
                        offset = 0;
                }
                else{
                    Status = STATUS_DATA_ERROR;
                    DoTraceMsg(TRACE_LEVEL_ERROR, "InitialJournalDataDirect : The Journal metadata is invalid - DeviceObject %p", pdx->DeviceObject);
                }
            }
            if (NT_SUCCESS(Status)){
                UINT32 number_of_bytes_read = 0;
                UINT32 number_of_bytes_to_read = pdx->Journal->JournalViewSize;
                pdx->Journal->JournalBlockPtr = (PVCBT_JOURNAL_BLOK)(pdx->Journal->JournalView + offset);
                offset = 0;
                while (NT_SUCCESS(Status = read_file_ex(pdx->Journal->JournalFileHandle, &pdx->Journal->JournalView[offset], number_of_bytes_to_read, pdx->Journal->JournalViewOffset + offset, &number_of_bytes_read))){
                    LONG min_size = (LONG)min((LONG)number_of_bytes_read + (LONG)offset, (pdx->Journal->FsClusterSize + (LONG)sizeof(VCBT_RECORD)));
                    for (;
                        (BYTE*)pdx->Journal->JournalBlockPtr < (pdx->Journal->JournalView + min_size);
                        pdx->Journal->JournalBlockPtr = (PVCBT_JOURNAL_BLOK)(((BYTE*)pdx->Journal->JournalBlockPtr) + sizeof(VCBT_RECORD))){
                        if (0 == pdx->Journal->JournalViewOffset && pdx->Journal->JournalBlockPtr == (PVCBT_JOURNAL_BLOK)pdx->Journal->JournalView){                        
                            if (pdx->Journal->JournalMetaData.FirstKey != pdx->Journal->JournalBlockPtr->r.Key){
                                DoTraceMsg(TRACE_LEVEL_ERROR, "InitialJournalData : The Journal file first key is not equal to the meta data file recorded - DeviceObject %p", pdx->DeviceObject);
                                break;
                            }
                            DoTraceMsg(TRACE_LEVEL_VERBOSE, "InitialJournalData : The First record Key : %I64u. - DeviceObject %p", pdx->Journal->JournalMetaData.FirstKey, pdx->DeviceObject);
                        }

                        if ((pdx->Journal->JournalBlockPtr->j.CheckSum == (ULONG)-1) &&
                            (pdx->Journal->JournalBlockPtr->j.Signature[0] == '1') &&
                            (pdx->Journal->JournalBlockPtr->j.Signature[1] == 'c') &&
                            (pdx->Journal->JournalBlockPtr->j.Signature[2] == 'b') &&
                            (pdx->Journal->JournalBlockPtr->j.Signature[3] == 't')){                           
                            if (pdx->Journal->JournalMetaData.Block.r.Key > pdx->Journal->JournalBlockPtr->r.Key){
                                Status = STATUS_DATA_ERROR;
                                DoTraceMsg(TRACE_LEVEL_ERROR, "InitialJournalData : The latest Journal record is not larger than or equal to the meta data file recorded - DeviceObject %p", pdx->DeviceObject);
                                break;
                            }
                            memcpy(&pdx->Journal->JournalMetaData.Block, pdx->Journal->JournalBlockPtr, sizeof(VCBT_JOURNAL_BLOK));
                            DoTraceMsg(TRACE_LEVEL_VERBOSE, "InitialJournalData : Found the latest Journal record (Key : %I64u) - DeviceObject %p", pdx->Journal->JournalMetaData.Block.r.Key, pdx->DeviceObject);
                            found = true;
                            break;
                        }
                    }
                    if (found)
                        break;
                    offset = number_of_bytes_to_read = pdx->Journal->FsClusterSize;
                    RtlMoveMemory(&pdx->Journal->JournalView[0], &pdx->Journal->JournalView[offset], (size_t)offset);
                    RtlZeroMemory(&pdx->Journal->JournalView[offset], (size_t)offset);
                    pdx->Journal->JournalBlockPtr = (PVCBT_JOURNAL_BLOK)(((BYTE*)pdx->Journal->JournalBlockPtr) - pdx->Journal->FsClusterSize);
                    pdx->Journal->JournalViewOffset += pdx->Journal->FsClusterSize;
                }
            }
        }
        if (!found){
            LogEventMessage(pdx->DeviceObject, VCBT_BAD_JOURNAL_DATA, 1, pdx->PhysicalDeviceNameBuffer);
            Status = STATUS_DATA_ERROR;
        }
    }
    return Status;
}

NTSTATUS ReadJournalMetaData(PDEVICE_EXTENSION pdx, BYTE* buff, LONGLONG offset, LONG length){
    NTSTATUS Status = STATUS_INVALID_PARAMETER;
    LARGE_INTEGER liVcnPrev = pdx->Journal->JournalMetaDataRetrievalPointers->StartingVcn;
    LONGLONG _start = (offset) / pdx->Journal->FsClusterSize;
    LONG     _offset = 0;
    LONGLONG _end = ((offset + length - 1)) / pdx->Journal->FsClusterSize;
    for (DWORD extent = 0; extent < pdx->Journal->JournalMetaDataRetrievalPointers->ExtentCount; extent++){
        LONGLONG _Start = liVcnPrev.QuadPart;
        LONG     _Length = (ULONG)(pdx->Journal->JournalMetaDataRetrievalPointers->Extents[extent].NextVcn.QuadPart - liVcnPrev.QuadPart);
        LONGLONG _End = _Start + _Length - 1;
        if ((_Start <= _start) && (_start <= _End)){
            LONG _length = ((LONG)(((_end > _End) ? _End : _end) - _start + 1)) * pdx->Journal->FsClusterSize;
            LONGLONG _Lcn = (pdx->Journal->JournalMetaDataRetrievalPointers->Extents[extent].Lcn.QuadPart + (_start - _Start))* pdx->Journal->FsClusterSize;
            _start = _start * (LONGLONG)pdx->Journal->FsClusterSize;
            _Lcn += pdx->Journal->FileAreaOffset;
            BYTE* temp = __malloc(_length);
            if (NULL != temp){
                memset(temp, 0, _length);
                Status = fastFsdRequest(pdx->TargetDeviceObject, IRP_MJ_READ, _Lcn, temp, _length, TRUE);
                if (!NT_SUCCESS(Status)){
                    WCHAR start_[VCBT_MAX_NUM_STR];
                    WCHAR length_[VCBT_MAX_NUM_STR];
                    memset(start_, 0, sizeof(start_));
                    memset(length_, 0, sizeof(length_));
                    RtlStringCchPrintfW(start_, VCBT_MAX_NUM_STR, L"%I64u", _Lcn);
                    RtlStringCchPrintfW(length_, VCBT_MAX_NUM_STR, L"%d", _length);
                    LogEventMessage(pdx->DeviceObject, VCBT_FAILED_TO_READ_JOURNAL_META, 3, pdx->PhysicalDeviceNameBuffer, start_, length_);
                    DoTraceMsg(TRACE_LEVEL_ERROR, "ReadJournalMetaData : Failed to read journal meta data. (start %I64u, lcn %I64u, length %d) - (IO: offset %I64u, length %d) - DeviceObject %p ", _start, _Lcn, _length, offset, length, pdx->DeviceObject);
                    break;
                }
                else{
                    memcpy(buff, temp, length);
                    DoTraceMsg(TRACE_LEVEL_VERBOSE, "ReadJournalMetaData : Succeeded to read journal meta data (start %I64u, lcn %I64u, length %d) - (IO: offset %I64u, length %d) - DeviceObject %p ", _start, _Lcn, length, offset, length, pdx->DeviceObject);
                }
                __free(temp);
            }
            if (_end <= _End)
                break;
            _offset += _length;
            _start = pdx->Journal->JournalMetaDataRetrievalPointers->Extents[extent].NextVcn.QuadPart;
        }
        liVcnPrev = pdx->Journal->JournalMetaDataRetrievalPointers->Extents[extent].NextVcn;
    }
    return Status;
}

NTSTATUS FlushJournalMetaData(PDEVICE_EXTENSION pdx, BYTE* buff, LONGLONG offset, LONG length){
    NTSTATUS Status = STATUS_INVALID_PARAMETER;
    LARGE_INTEGER liVcnPrev = pdx->Journal->JournalMetaDataRetrievalPointers->StartingVcn;
    LONGLONG _start = (offset) / pdx->Journal->FsClusterSize;
    LONG     _offset = 0;
    LONGLONG _end = ((offset + length - 1)) / pdx->Journal->FsClusterSize;
    for (DWORD extent = 0; extent < pdx->Journal->JournalMetaDataRetrievalPointers->ExtentCount; extent++){
        LONGLONG _Start = liVcnPrev.QuadPart;
        LONG     _Length = (ULONG)(pdx->Journal->JournalMetaDataRetrievalPointers->Extents[extent].NextVcn.QuadPart - liVcnPrev.QuadPart);
        LONGLONG _End = _Start + _Length - 1;
        if ((_Start <= _start) && (_start <= _End)){
            LONG _length = ((LONG)(((_end > _End) ? _End : _end) - _start + 1)) * pdx->Journal->FsClusterSize;
            LONGLONG _Lcn = (pdx->Journal->JournalMetaDataRetrievalPointers->Extents[extent].Lcn.QuadPart + (_start - _Start))* pdx->Journal->FsClusterSize;
            _start = _start * (LONGLONG)pdx->Journal->FsClusterSize;
            _Lcn += pdx->Journal->FileAreaOffset;
            Status = fastFsdRequest2(pdx->TargetDeviceObject, IRP_MJ_WRITE, _Lcn, &buff[_offset], _length, TRUE);
            if (!NT_SUCCESS(Status)){
                WCHAR start_[VCBT_MAX_NUM_STR];
                WCHAR length_[VCBT_MAX_NUM_STR];
                memset(start_, 0, sizeof(start_));
                memset(length_, 0, sizeof(length_));
                RtlStringCchPrintfW(start_, VCBT_MAX_NUM_STR, L"%I64u", _Lcn);
                RtlStringCchPrintfW(length_, VCBT_MAX_NUM_STR, L"%d", _length);
                LogEventMessage(pdx->DeviceObject, VCBT_FAILED_TO_FLUSH_JOURNAL_META_DATA, 3, pdx->PhysicalDeviceNameBuffer, start_, length_);
                DoTraceMsg(TRACE_LEVEL_ERROR, "FlushJournalMetaData : Failed to flush journal meta data. (start %I64u, lcn %I64u, length %d) - (IO: offset %I64u, length %d) - DeviceObject %p ", _start, _Lcn, _length, offset, length, pdx->DeviceObject);
                break;
            }
            else if (TRUE == _ControlDeviceExtension->VerboseDebug){
                DoTraceMsg(TRACE_LEVEL_VERBOSE, "FlushJournalData : Succeeded to flush journal meta data (start %I64u, lcn %I64u, length %d) - (IO: offset %I64u, length %d) - DeviceObject %p ", _start, _Lcn, _length, offset, length, pdx->DeviceObject);
            }
            if (_end <= _End)
                break;
            _offset += _length;
            _start = pdx->Journal->JournalMetaDataRetrievalPointers->Extents[extent].NextVcn.QuadPart;
        }
        liVcnPrev = pdx->Journal->JournalMetaDataRetrievalPointers->Extents[extent].NextVcn;
    }
    return Status;
}

NTSTATUS FlushJournalData(PDEVICE_EXTENSION pdx, BYTE* buff, LONGLONG offset, LONG length){
    NTSTATUS Status = STATUS_INVALID_PARAMETER;
    LARGE_INTEGER liVcnPrev = pdx->Journal->JournalRetrievalPointers->StartingVcn;
    LONGLONG _start = (offset) / pdx->Journal->FsClusterSize;
    LONG     _offset = 0;
    LONGLONG _end = ((offset + length - 1)) / pdx->Journal->FsClusterSize;
    for (DWORD extent = 0; extent < pdx->Journal->JournalRetrievalPointers->ExtentCount; extent++){
        LONGLONG _Start = liVcnPrev.QuadPart;
        LONG     _Length = (ULONG)(pdx->Journal->JournalRetrievalPointers->Extents[extent].NextVcn.QuadPart - liVcnPrev.QuadPart);
        LONGLONG _End = _Start + _Length - 1;
        if ((_Start <= _start) && (_start <= _End)){
            LONG _length = ((LONG)(((_end > _End) ? _End : _end) - _start + 1)) * pdx->Journal->FsClusterSize;
            LONGLONG _Lcn = (pdx->Journal->JournalRetrievalPointers->Extents[extent].Lcn.QuadPart + (_start - _Start))* pdx->Journal->FsClusterSize;
            _start = _start * (LONGLONG)pdx->Journal->FsClusterSize;
            _Lcn += pdx->Journal->FileAreaOffset;
            Status = fastFsdRequest2(pdx->TargetDeviceObject, IRP_MJ_WRITE, _Lcn, &buff[_offset], _length, TRUE);
            if (!NT_SUCCESS(Status)){
                WCHAR start_[VCBT_MAX_NUM_STR];
                WCHAR length_[VCBT_MAX_NUM_STR];
                memset(start_, 0, sizeof(start_));
                memset(length_, 0, sizeof(length_));
                RtlStringCchPrintfW(start_, VCBT_MAX_NUM_STR, L"%I64u", _Lcn);
                RtlStringCchPrintfW(length_, VCBT_MAX_NUM_STR, L"%d", _length);
                LogEventMessage(pdx->DeviceObject, VCBT_FAILED_TO_WRITE_JOURNAL, 3, pdx->PhysicalDeviceNameBuffer, start_, length_);
                DoTraceMsg(TRACE_LEVEL_ERROR, "FlushJournalData : Failed to flush journal data. (start %I64u, lcn %I64u, length %d) - (IO: offset %I64u, length %d) - DeviceObject %p ", _start, _Lcn, _length, offset, length, pdx->DeviceObject);
                break;
            }
            else if (TRUE == _ControlDeviceExtension->VerboseDebug){
                DoTraceMsg(TRACE_LEVEL_VERBOSE, "FlushJournalData : Succeeded to flush journal data (start %I64u, lcn %I64u, length %d) - (IO: offset %I64u, length %d) - DeviceObject %p ", _start, _Lcn, _length, offset, length, pdx->DeviceObject);
            }
            if (_end <= _End)
                break;
            _offset += _length;
            _start = pdx->Journal->JournalRetrievalPointers->Extents[extent].NextVcn.QuadPart;
        }
        liVcnPrev = pdx->Journal->JournalRetrievalPointers->Extents[extent].NextVcn;
    }
    return Status;
}

NTSTATUS ReadFromJournalData(PDEVICE_EXTENSION pdx, BYTE* buff, LONGLONG offset, LONG length){
    NTSTATUS Status = STATUS_INVALID_PARAMETER;
    LARGE_INTEGER liVcnPrev = pdx->Journal->JournalRetrievalPointers->StartingVcn;
    LONGLONG _start = (offset) / pdx->Journal->FsClusterSize;
    LONGLONG _end = ((offset + length - 1)) / pdx->Journal->FsClusterSize;
    LONG     _offset = 0;
    for (DWORD extent = 0; extent < pdx->Journal->JournalRetrievalPointers->ExtentCount; extent++){
        LONGLONG _Start = liVcnPrev.QuadPart;
        LONG     _Length = (ULONG)(pdx->Journal->JournalRetrievalPointers->Extents[extent].NextVcn.QuadPart - liVcnPrev.QuadPart);
        LONGLONG _End = _Start + _Length - 1;
        if ((_Start <= _start) && (_start <= _End)){
            LONG _length = ((LONG)(((_end > _End) ? _End : _end) - _start + 1)) * pdx->Journal->FsClusterSize;
            LONGLONG _Lcn = (pdx->Journal->JournalRetrievalPointers->Extents[extent].Lcn.QuadPart + (_start - _Start))* pdx->Journal->FsClusterSize;
            _start = _start * (LONGLONG)pdx->Journal->FsClusterSize;
            _Lcn += pdx->Journal->FileAreaOffset;
            Status = fastFsdRequest(pdx->TargetDeviceObject, IRP_MJ_READ, _Lcn, &buff[_offset], _length, TRUE);
            if (!NT_SUCCESS(Status)){
                WCHAR start_[VCBT_MAX_NUM_STR];
                WCHAR length_[VCBT_MAX_NUM_STR];
                memset(start_, 0, sizeof(start_));
                memset(length_, 0, sizeof(length_));
                RtlStringCchPrintfW(start_, VCBT_MAX_NUM_STR, L"%I64u", _Lcn);
                RtlStringCchPrintfW(length_, VCBT_MAX_NUM_STR, L"%d", _length);
                LogEventMessage(pdx->DeviceObject, VCBT_FAILED_TO_READ_JOURNAL, 3, pdx->PhysicalDeviceNameBuffer, start_, length_);
                DoTraceMsg(TRACE_LEVEL_ERROR, "ReadFromJournalData : Failed to Read journal data. (start %I64u, lcn %I64u, length %d) - (IO: offset %I64u, length %d) - DeviceObject %p ", _start, _Lcn, _length, offset, length, pdx->DeviceObject);
                break;
            }
            else if (TRUE == _ControlDeviceExtension->VerboseDebug){
                DoTraceMsg(TRACE_LEVEL_VERBOSE, "ReadFromJournalData : Succeeded to Read journal data (start %I64u, lcn %I64u, length %d) - (IO: offset %I64u, length %d) - DeviceObject %p ", _start, _Lcn, _length, offset, length, pdx->DeviceObject);
            }
            if (_end <= _End)
                break;
            _offset += _length;
            _start = pdx->Journal->JournalRetrievalPointers->Extents[extent].NextVcn.QuadPart;
        }
        liVcnPrev = pdx->Journal->JournalRetrievalPointers->Extents[extent].NextVcn;
    }
    return Status;
}

#define ULONG_MAX 	4294967295

NTSTATUS WriteJournalData(PDEVICE_EXTENSION pdx, ULONGLONG offset, ULONG length){
    NTSTATUS Status = STATUS_SUCCESS;
    bool needWrite = true;
    if (NULL != pdx->Journal && (FALSE == _ControlDeviceExtension->JournalDisabled)){
        if (NULL != pdx->Journal->JournalRetrievalPointers){
            if ((pdx->Journal->JournalMetaData.Block.r.Start != offset) || (pdx->Journal->JournalMetaData.Block.r.Length != length)){
                ULONGLONG End = pdx->Journal->JournalMetaData.Block.r.Start + pdx->Journal->JournalMetaData.Block.r.Length;
                if ((pdx->Journal->JournalMetaData.Block.r.Start <= offset) && (offset <= End)){
                    ULONGLONG end = offset + length;
                    if (End < end){
                        ULONGLONG newlen = end - pdx->Journal->JournalMetaData.Block.r.Start; 
                        if (newlen < ULONG_MAX && newlen > pdx->Journal->JournalMetaData.Block.r.Length){
                            pdx->Journal->JournalMetaData.Block.r.Length = (ULONG)newlen;
                        }
                        else{
                            pdx->Journal->JournalMetaData.Block.r.Start = offset;
                            pdx->Journal->JournalMetaData.Block.r.Length = length;
                            pdx->Journal->JournalMetaData.Block.r.Key++;
                            pdx->Journal->JournalBlockPtr = (PVCBT_JOURNAL_BLOK)(((BYTE*)pdx->Journal->JournalBlockPtr) + sizeof(VCBT_RECORD));
                        }
                    }
                    else{
                        DoTraceMsg(TRACE_LEVEL_WARNING, "WriteJournalData : Skip to write journal data. (IO: offset %I64u, length %d), Journal( Start: %I64u, Length: %d) - DeviceObject %p ",
                            offset,
                            length,
                            pdx->Journal->JournalMetaData.Block.r.Start,
                            pdx->Journal->JournalMetaData.Block.r.Length,
                            pdx->DeviceObject);
                        needWrite = false;
                    }
                }
                else{
                    pdx->Journal->JournalMetaData.Block.r.Start = offset;
                    pdx->Journal->JournalMetaData.Block.r.Length = length;
                    pdx->Journal->JournalMetaData.Block.r.Key++;
                    pdx->Journal->JournalBlockPtr = (PVCBT_JOURNAL_BLOK)(((BYTE*)pdx->Journal->JournalBlockPtr) + sizeof(VCBT_RECORD));
                }

                if (needWrite){
                    bool update_metadata = !_ControlDeviceExtension->BatchUpdateJournalMetaData;
                    LONG _offset = (LONG)(((BYTE*)pdx->Journal->JournalBlockPtr) - pdx->Journal->JournalView);
                    if (_offset < (pdx->Journal->FsClusterSize - (LONG)sizeof(VCBT_JOURNAL_BLOK))){
                        memcpy(pdx->Journal->JournalBlockPtr, &pdx->Journal->JournalMetaData.Block, sizeof(VCBT_JOURNAL_BLOK));
                        if (NT_SUCCESS(Status = FlushJournalData(pdx, pdx->Journal->JournalView, pdx->Journal->JournalViewOffset, pdx->Journal->FsClusterSize))){
                            if (TRUE == _ControlDeviceExtension->VerboseDebug){
                                DoTraceMsg(TRACE_LEVEL_VERBOSE, "WriteJournalData : Succeeded to write journal data. (IO: offset %I64u, length %d), Journal( Start: %I64u, Length: %d) - DeviceObject %p ",
                                    offset,
                                    length,
                                    pdx->Journal->JournalMetaData.Block.r.Start,
                                    pdx->Journal->JournalMetaData.Block.r.Length,
                                    pdx->DeviceObject);
                            }
                        }
                    }
                    else if (_offset < pdx->Journal->FsClusterSize){
                        memcpy(pdx->Journal->JournalBlockPtr, &pdx->Journal->JournalMetaData.Block, sizeof(VCBT_JOURNAL_BLOK));
                        if (NT_SUCCESS(Status = FlushJournalData(pdx, pdx->Journal->JournalView, pdx->Journal->JournalViewOffset, pdx->Journal->FsClusterSize * 2))){
                            if (TRUE == _ControlDeviceExtension->VerboseDebug){
                                DoTraceMsg(TRACE_LEVEL_VERBOSE, "WriteJournalData : Succeeded to write journal data. (IO: offset %I64u, length %d), Journal( Start: %I64u, Length: %d) - DeviceObject %p ",
                                    offset,
                                    length,
                                    pdx->Journal->JournalMetaData.Block.r.Start,
                                    pdx->Journal->JournalMetaData.Block.r.Length,
                                    pdx->DeviceObject);
                            }
                        }
                    }
                    else if (_offset < (pdx->Journal->JournalViewSize - (LONG)sizeof(VCBT_JOURNAL_BLOK))){
                        memcpy(pdx->Journal->JournalBlockPtr, &pdx->Journal->JournalMetaData.Block, sizeof(VCBT_JOURNAL_BLOK));
                        if (NT_SUCCESS(Status = FlushJournalData(pdx, pdx->Journal->JournalView + pdx->Journal->FsClusterSize, pdx->Journal->JournalViewOffset + pdx->Journal->FsClusterSize, pdx->Journal->FsClusterSize))){
                            if (TRUE == _ControlDeviceExtension->VerboseDebug){
                                DoTraceMsg(TRACE_LEVEL_VERBOSE, "WriteJournalData : Succeeded to write journal data. (IO: offset %I64u, length %d), Journal( Start: %I64u, Length: %d) - DeviceObject %p ",
                                    offset,
                                    length,
                                    pdx->Journal->JournalMetaData.Block.r.Start,
                                    pdx->Journal->JournalMetaData.Block.r.Length,
                                    pdx->DeviceObject);
                            }
                        }
                    }
                    else{
                        update_metadata = true;
                        int count = 2;
                        if (pdx->Journal->JournalViewOffset + pdx->Journal->JournalViewSize == pdx->Journal->JournalMetaData.Block.j.Size){
                            if (NT_SUCCESS(Status = ReadFromJournalData(pdx, pdx->Journal->JournalView, 0, pdx->Journal->JournalViewSize))){
                                pdx->Journal->JournalViewOffset = 0;
                                pdx->Journal->JournalBlockPtr = (PVCBT_JOURNAL_BLOK)pdx->Journal->JournalView;
                                pdx->Journal->JournalMetaData.FirstKey = pdx->Journal->JournalMetaData.Block.r.Key;
                                count = 1;
                                DoTraceMsg(TRACE_LEVEL_VERBOSE, "WriteJournalData : Succeeded to read journal data (start %I64u, length %d) - DeviceObject %p ", pdx->Journal->JournalViewOffset, pdx->Journal->JournalViewSize, pdx->DeviceObject);
                            }
                        }
                        else{
                            memmove(pdx->Journal->JournalView, pdx->Journal->JournalView + pdx->Journal->FsClusterSize, pdx->Journal->FsClusterSize);
                            pdx->Journal->JournalBlockPtr = (PVCBT_JOURNAL_BLOK)(((BYTE*)pdx->Journal->JournalBlockPtr) - pdx->Journal->FsClusterSize);
                            pdx->Journal->JournalViewOffset += pdx->Journal->FsClusterSize;
                            if (NT_SUCCESS(Status = ReadFromJournalData(pdx, pdx->Journal->JournalView + pdx->Journal->FsClusterSize, pdx->Journal->JournalViewOffset + pdx->Journal->FsClusterSize, pdx->Journal->FsClusterSize))){
                            }
                        }
                        if (NT_SUCCESS(Status)){
                            memcpy(pdx->Journal->JournalBlockPtr, &pdx->Journal->JournalMetaData.Block, sizeof(VCBT_JOURNAL_BLOK));
                            if (NT_SUCCESS(Status = FlushJournalData(pdx, pdx->Journal->JournalView, pdx->Journal->JournalViewOffset, pdx->Journal->FsClusterSize * count))){
                                if (TRUE == _ControlDeviceExtension->VerboseDebug){
                                    DoTraceMsg(TRACE_LEVEL_VERBOSE, "WriteJournalData : Succeeded to write journal data. (IO: offset %I64u, length %d), Journal( Start: %I64u, Length: %d) - DeviceObject %p ",
                                        offset,
                                        length,
                                        pdx->Journal->JournalMetaData.Block.r.Start,
                                        pdx->Journal->JournalMetaData.Block.r.Length,
                                        pdx->DeviceObject);
                                }
                            }
                        }
                    }
                    if ( update_metadata && NT_SUCCESS(Status)){
                        Status = FlushJournalMetaData(pdx,(BYTE*)&pdx->Journal->JournalMetaData, 0, pdx->Journal->BytesPerSector);
                    }
                    if (NT_SUCCESS(Status)){
                        memcpy(&pdx->Journal->BackupJournalMetaData, &pdx->Journal->JournalMetaData, sizeof(VCBT_JOURNAL_META_DATA));
                    }
                    else{
                        memcpy(&pdx->Journal->JournalMetaData, &pdx->Journal->BackupJournalMetaData, sizeof(VCBT_JOURNAL_META_DATA));
                    }
                }
            }
            else{
                DoTraceMsg(TRACE_LEVEL_WARNING, "WriteJournalData : Skip to write journal data. (IO: offset %I64u, length %d), Journal( Start: %I64u, Length: %d) - DeviceObject %p ", 
                    offset,
                    length, 
                    pdx->Journal->JournalMetaData.Block.r.Start,
                    pdx->Journal->JournalMetaData.Block.r.Length,
                    pdx->DeviceObject);
            }
        }
    }
    return Status;
}

NTSTATUS Snapshot(PDEVICE_EXTENSION pdx){
    NTSTATUS Status = STATUS_NOT_IMPLEMENTED;
    KIRQL  oldIrql = PASSIVE_LEVEL;
    if (PASSIVE_LEVEL != (oldIrql = KeGetCurrentIrql())){
        KeLowerIrql(PASSIVE_LEVEL);
    }
    if (NULL != pdx->Journal && pdx->Journal->UmapFileHandle != 0){
        WCHAR buff[VCBT_MAX_PATH];
        memset(buff, 0, sizeof(buff));
        UNICODE_STRING  path;
        HANDLE          mhandle;
        NTSTATUS      status;
        LARGE_INTEGER length;
        if (NT_SUCCESS(status = get_volume_length(pdx->DeviceObject, &length))){
            ULONG resolution = UMAP_RESOLUTION1;
            int count = 1;
            while ((length.QuadPart >> (resolution + 19)) > 0){
                count++;
                resolution++;
            }
            if (resolution != pdx->Journal->Resolution){
                lock_acquire(&pdx->Journal->Lock);
                pdx->Journal->Resolution = resolution;
                memset(pdx->Journal->UMAP, 0, sizeof(pdx->Journal->UMAP));
                lock_release(&pdx->Journal->Lock);
                RtlStringCchPrintfW(pdx->Journal->UmapFileNameBuffer, VCBT_MAX_PATH, L"%s%s"UMAP_RESOLUTION_FILE, pdx->MountDeviceLink, VCBT_VOLUME_SYS_INFO, count);
                RtlInitUnicodeString(&pdx->Journal->UmapFileName, &pdx->Journal->UmapFileNameBuffer[0]);
                rename_file(pdx->Journal->UmapFileHandle, &pdx->Journal->UmapFileName);
                FlushUmapData(pdx, 0, 0, TRUE);
                Status = STATUS_NOT_READ_FROM_COPY;
            }
            else{
                RtlStringCchPrintfW(buff, VCBT_MAX_PATH, L"%s.snap", pdx->Journal->UmapFileNameBuffer);
                RtlInitUnicodeString(&path, &buff[0]);
                if (NT_SUCCESS(Status = create_file(&mhandle, &path))){
                    BYTE* buf = (BYTE*)__malloc(RESOLUTION_UMAP_SIZE);
                    if (NULL != buf){
                        lock_acquire(&pdx->Journal->Lock);
                        memcpy(buf, pdx->Journal->UMAP, RESOLUTION_UMAP_SIZE);
                        memset(pdx->Journal->UMAP, 0, sizeof(pdx->Journal->UMAP));
                        lock_release(&pdx->Journal->Lock);
                        Status = write_file(mhandle, buf, RESOLUTION_UMAP_SIZE, 0);
                        if (NT_SUCCESS(Status)){
                            FlushUmapData(pdx, 0, 0, TRUE);
                        }
                        else{
                            lock_acquire(&pdx->Journal->Lock);
                            merge_umap(pdx->Journal->UMAP, buf, RESOLUTION_UMAP_SIZE);
                            lock_release(&pdx->Journal->Lock);
                            FlushUmapData(pdx, 0, 0, TRUE);
                        }
                        __free(buf);
                    }
                    close_file(mhandle);
                }
            }
        }
    }
    if (PASSIVE_LEVEL != oldIrql)
        KeRaiseIrql(oldIrql, &oldIrql);
    return Status;
}

NTSTATUS PostSnapshot(PDEVICE_EXTENSION pdx){
    NTSTATUS Status = STATUS_NOT_IMPLEMENTED;
    KIRQL  oldIrql = PASSIVE_LEVEL;
    if (PASSIVE_LEVEL != (oldIrql = KeGetCurrentIrql())){
        KeLowerIrql(PASSIVE_LEVEL);
    }
    if (NULL != pdx->Journal && pdx->Journal->UmapFileHandle != 0 ){
        WCHAR buff[VCBT_MAX_PATH];
        memset(buff, 0, sizeof(buff));
        UNICODE_STRING  path;
        RtlStringCchPrintfW(buff, VCBT_MAX_PATH, L"%s.snap", pdx->Journal->UmapFileNameBuffer);
        RtlInitUnicodeString(&path, &buff[0]);
        Status = delete_file_ex(&path);
    }
    if (PASSIVE_LEVEL != oldIrql)
        KeRaiseIrql(oldIrql, &oldIrql);
    return Status;
}

NTSTATUS UndoSnapshot(PDEVICE_EXTENSION pdx){
    NTSTATUS Status = STATUS_NOT_IMPLEMENTED;
    KIRQL  oldIrql = PASSIVE_LEVEL;
    if (PASSIVE_LEVEL != (oldIrql = KeGetCurrentIrql())){
        KeLowerIrql(PASSIVE_LEVEL);
    }
    if (NULL != pdx->Journal && pdx->Journal->UmapFileHandle != 0){
        WCHAR buff[VCBT_MAX_PATH];
        memset(buff, 0, sizeof(buff));
        UNICODE_STRING  path;
        HANDLE          mhandle;
        RtlStringCchPrintfW(buff, VCBT_MAX_PATH, L"%s.snap", pdx->Journal->UmapFileNameBuffer);
        RtlInitUnicodeString(&path, &buff[0]);
        if (NT_SUCCESS(Status = open_file(&mhandle, &path))){
            BYTE* buf = (BYTE*)__malloc(RESOLUTION_UMAP_SIZE);
            if (NULL != buf){
                Status = read_file(mhandle, buf, RESOLUTION_UMAP_SIZE, 0);
                if (NT_SUCCESS(Status)){
                    lock_acquire(&pdx->Journal->Lock);
                    merge_umap(pdx->Journal->UMAP, buf, RESOLUTION_UMAP_SIZE);              
                    lock_release(&pdx->Journal->Lock);
                    FlushUmapData(pdx, 0, 0, TRUE);
                    delete_file(mhandle);               
                }              
                __free(buf);
            }
            else{
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
            close_file(mhandle);
        }
    }
    if (PASSIVE_LEVEL != oldIrql)
        KeRaiseIrql(oldIrql, &oldIrql);
    return Status;
}

NTSTATUS ProtectJournalFilesClusters(PDEVICE_EXTENSION pdx){
    HANDLE              volumeHandle =0;
    NTSTATUS            Status = STATUS_SUCCESS;
    IO_STATUS_BLOCK     IoStatus;
    UNICODE_STRING path;
    WCHAR buff[VCBT_MAX_PATH];
    memset(buff, 0, sizeof(buff));
    RtlStringCchPrintfW(buff, VCBT_MAX_PATH, L"%s\\", pdx->MountDeviceLink);
    RtlInitUnicodeString(&path, &buff[0]);
    if (STATUS_SUCCESS == (Status = open_file(&volumeHandle, &path))){
        MARK_HANDLE_INFO mark_handle_info;
        mark_handle_info.UsnSourceInfo = USN_SOURCE_DATA_MANAGEMENT;
        mark_handle_info.VolumeHandle = volumeHandle;
        mark_handle_info.HandleInfo = MARK_HANDLE_PROTECT_CLUSTERS;
        if (STATUS_SUCCESS != (Status = ZwFsControlFile(pdx->Journal->UmapFileHandle, NULL, NULL, NULL, &IoStatus, FSCTL_MARK_HANDLE, &mark_handle_info, sizeof(MARK_HANDLE_INFO), NULL, 0))){
            DoTraceMsg(TRACE_LEVEL_ERROR, "ProtectJournalFilesClusters: Failed to protect the Umap File Clusters, status: 0x%08X - DeviceObject %p ", Status, pdx->DeviceObject);
        }
        else if (FALSE == _ControlDeviceExtension->JournalDisabled){
            if (STATUS_SUCCESS != (Status = ZwFsControlFile(pdx->Journal->JournalFileHandle, NULL, NULL, NULL, &IoStatus, FSCTL_MARK_HANDLE, &mark_handle_info, sizeof(MARK_HANDLE_INFO), NULL, 0))){
                DoTraceMsg(TRACE_LEVEL_ERROR, "ProtectJournalFilesClusters: Failed to protect the Journal File Clusters, status: 0x%08X - DeviceObject %p ", Status, pdx->DeviceObject);
            }
            else if (STATUS_SUCCESS != (Status = ZwFsControlFile(pdx->Journal->JournalMetaDataFileHandle, NULL, NULL, NULL, &IoStatus, FSCTL_MARK_HANDLE, &mark_handle_info, sizeof(MARK_HANDLE_INFO), NULL, 0))){
                DoTraceMsg(TRACE_LEVEL_ERROR, "ProtectJournalFilesClusters: Failed to protect the Journal MetaData File Clusters, status: 0x%08X - DeviceObject %p ", Status, pdx->DeviceObject);
            }
            else{
                DoTraceMsg(TRACE_LEVEL_INFORMATION, "ProtectJournalFilesClusters: Succeeded to protect the Journal Files' Clusters - DeviceObject %p ", pdx->DeviceObject);
            }
        }
        close_file(volumeHandle);
    }
    else{
        DoTraceMsg(TRACE_LEVEL_ERROR, "ProtectJournalFilesClusters: Failed to open the volume handle, status: 0x%08X - DeviceObject %p ", Status, pdx->DeviceObject);
    }
    return Status;
}