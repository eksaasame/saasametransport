#ifndef _FAT_OP_
#define _FAT_OP_

#include "type.h"
#include <ntddk.h>
#include "stdio.h"

NTSTATUS get_fat_first_sector_offset(PDEVICE_OBJECT target, PULONGLONG file_area_offset);

#endif