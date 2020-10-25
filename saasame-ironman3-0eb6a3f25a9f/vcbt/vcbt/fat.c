#include "fat.h"
#include "fatlbr.h"
#include "type.h"
#include "vcbt.h"
#include "trace.h"
#include "ioctl.h"
#include "fat.tmh"

#define BUFSIZE 4096
#define MAX_PATH 256
static BYTE chBuffer[BUFSIZE];

NTSTATUS get_fat_first_sector_offset(PDEVICE_OBJECT target, PULONGLONG file_area_offset){
    NTSTATUS	status;
    FAT_LBR		fatLBR = { 0 };
    LARGE_INTEGER	pos;
    pos.QuadPart = 0;
    if (!file_area_offset){
        return STATUS_NOT_FOUND;
    }
    memset(chBuffer, 0, BUFSIZE);
    status = fastFsdRequest(target, IRP_MJ_READ, 0, chBuffer, BUFSIZE, TRUE);  
    if (NT_SUCCESS(status)){

        memcpy(&fatLBR, chBuffer, sizeof(fatLBR));
        DWORD dwRootDirSectors = 0;
        DWORD dwFATSz = 0;

        // Validate jump instruction to boot code. This field has two
        // allowed forms: 
        // jmpBoot[0] = 0xEB, jmpBoot[1] = 0x??, jmpBoot[2] = 0x90 
        // and
        // jmpBoot[0] = 0xE9, jmpBoot[1] = 0x??, jmpBoot[2] = 0x??
        // 0x?? indicates that any 8-bit value is allowed in that byte.
        // JmpBoot[0] = 0xEB is the more frequently used format.

        if ((fatLBR.wTrailSig != 0xAA55) ||
            ((fatLBR.pbyJmpBoot[0] != 0xEB ||
            fatLBR.pbyJmpBoot[2] != 0x90) &&
            (fatLBR.pbyJmpBoot[0] != 0xE9))){
            status = STATUS_NOT_FOUND;
            goto __faild;
        }

        // Compute first sector offset for the FAT volumes:		


        // First, we determine the count of sectors occupied by the
        // root directory. Note that on a FAT32 volume the BPB_RootEntCnt
        // value is always 0, so on a FAT32 volume dwRootDirSectors is
        // always 0. The 32 in the above is the size of one FAT directory
        // entry in bytes. Note also that this computation rounds up.

        dwRootDirSectors =
            (((fatLBR.bpb.wRootEntCnt * 32) +
            (fatLBR.bpb.wBytsPerSec - 1)) /
            fatLBR.bpb.wBytsPerSec);

        // The start of the data region, the first sector of cluster 2,
        // is computed as follows:

        dwFATSz = fatLBR.bpb.wFATSz16;
        if (!dwFATSz)
            dwFATSz = fatLBR.ebpb.ebpb32.dwFATSz32;


        if (!dwFATSz){
            status = STATUS_NOT_FOUND;
            goto __faild;
        }

        // 
        *file_area_offset =
            (fatLBR.bpb.wRsvdSecCnt +
            (fatLBR.bpb.byNumFATs * dwFATSz) +
            dwRootDirSectors) * fatLBR.bpb.wBytsPerSec;
        DoTraceMsg(TRACE_LEVEL_INFORMATION, "get_fat_first_sector_offset: FileAreaOffset: %I64d", *file_area_offset);
    }
    status = STATUS_SUCCESS;
__faild:
    return status;
}