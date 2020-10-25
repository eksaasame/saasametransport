
#ifndef _IRM_DISK_H
#define _IRM_DISK_H
#define INITGUID
#include <guiddef.h>
#ifndef _WINIOCTL
#define _WINIOCTL
#include <winioctl.h>
#endif
#include <Windows.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GPT_PMBR_LBA 0
#define GPT_PMBR_SECTORS 1
#define GPT_PRIMARY_HEADER_LBA 1
#define GPT_HEADER_SECTORS 1
#define GPT_PRIMARY_PART_TABLE_LBA 2
#define MS_PARTITION_LDM   0x42                   // PARTITION_LDM

typedef struct _GPT_PARTITIONTABLE_HEADER
{
	unsigned __int64 Signature;
	unsigned __int32 Revision;
	unsigned __int32 HeaderSize;
	unsigned __int32 HeaderCRC32;
	unsigned __int32 Reserved1;
	unsigned __int64 MyLBA;
	unsigned __int64 AlternateLBA;
	unsigned __int64 FirstUsableLBA;
	unsigned __int64 LastUsableLBA;
	GUID             DiskGUID;
	unsigned __int64 PartitionEntryLBA;
	unsigned __int32 NumberOfPartitionEntries;
	unsigned __int32 SizeOfPartitionEntry;
	unsigned __int32 PartitionEntryArrayCRC32;

} GPT_PARTITIONTABLE_HEADER, *PGPT_PARTITIONTABLE_HEADER;

typedef struct _GPT_PARTITION_ENTRY
{
	GUID             PartitionTypeGuid;
	GUID             UniquePartitionGuid;
	unsigned __int64 StartingLBA;
	unsigned __int64 EndingLBA;
	unsigned __int64 Attributes;
	WCHAR PartitionName[36];

} GPT_PARTITION_ENTRY,*PGPT_PARTITION_ENTRY;

typedef struct _CHS_ADDRESS
{
    unsigned __int8 Head;
    unsigned __int8 Sector;
    unsigned __int32 Cylinder;

}CHS_ADDRESS,*PCHS_ADDRESS;

typedef struct _PARTITION_RECORD 
{
	/* (0x80 = bootable, 0x00 = non-bootable, other = invalid) */
	unsigned __int8 BootIndicator;

	/* Start of partition in CHS address, not used by EFI firmware. */
	unsigned __int8 StartCHS[3];  
    // [0] head
    // [1] sector is in bits 5-0; bits 9-8 of cylinder are in bits 7-6 
    // [2] bits 7-0 of cylinder
	unsigned __int8 PartitionType;
	/* End of partition in CHS address, not used by EFI firmware. */
	unsigned __int8 EndCHS[3]; 

    // [0] head
    // [1] sector is in bits 5-0; bits 9-8 of cylinder are in bits 7-6 
    // [2] bits 7-0 of cylinder

	/* Starting LBA address of the partition on the disk. Used by
	   EFI firmware to define the start of the partition. */
	unsigned __int32 StartingLBA;

	/* Size of partition in LBA. Used by EFI firmware to determine
	   the size of the partition. */
	unsigned __int32 SizeInLBA;

} PARTITION_RECORD, *PPARTITION_RECORD;


/* Protected Master Boot Record  & Legacy MBR share same structure */
/* Needs to be packed because the u16s force misalignment. */
#pragma pack(push)  /* push current alignment to stack */
#pragma pack(2)     /* set alignment to 2 byte boundary */

typedef struct _LEGACY_MBR 
{
	unsigned __int8 BootCode[440];
	unsigned __int32 UniqueMBRSignature;
	unsigned __int16 Unknown;
	PARTITION_RECORD PartitionRecord[4];
	unsigned __int16 Signature;

} LEGACY_MBR, *PLEGACY_MBR;

#pragma pack(pop)   /* restore original alignment from stack */

__inline void calculate_chs_address(ULONG LBA, PDISK_GEOMETRY geometry, PCHS_ADDRESS chs)
{
    ULONG max = (1024 * geometry->SectorsPerTrack * geometry->TracksPerCylinder) - 1;
    if (LBA > max){
        chs->Cylinder = 1023;
        chs->Head = geometry->TracksPerCylinder - 1;
        chs->Sector = geometry->SectorsPerTrack;
    }
    else{
        // LBA = ((cylinder * heads_per_cylinder + heads) * sectors_per_track) + sector - 1
        chs->Cylinder = LBA / (geometry->SectorsPerTrack * geometry->TracksPerCylinder);
        INT temp = LBA % (geometry->SectorsPerTrack * geometry->TracksPerCylinder);
        chs->Head = (__int8)(temp / geometry->SectorsPerTrack);
        chs->Sector = (temp % geometry->SectorsPerTrack) + 1;
	}
}

__inline void set_mbr_partition_chs_address( unsigned __int8 *p, PCHS_ADDRESS chs)
{
    p[0] = chs->Head;
    p[1] = ( unsigned __int8 ) ( ( ( chs->Cylinder & 0x0300 ) >> 2  ) | ( chs->Sector & 0x3f ) );
    p[2] = ( unsigned __int8 ) ( chs->Cylinder & 0xFF );
}

__inline void get_mbr_partition_chs_address( unsigned __int8 *p, PCHS_ADDRESS chs )
{
    chs->Head = p[0];
    chs->Sector = p[1] & 0x3f;
    chs->Cylinder = p[2] | ((p[1] << 2) & 0x0300);
}

static char DISK_MBRDATA[512] = 
{
	'\x33','\xC0','\x8E','\xD0','\xBC','\x00','\x7C','\x8E','\xC0','\x8E','\xD8','\xBE','\x00','\x7C','\xBF','\x00',
	'\x06','\xB9','\x00','\x02','\xFC','\xF3','\xA4','\x50','\x68','\x1C','\x06','\xCB','\xFB','\xB9','\x04','\x00',
	'\xBD','\xBE','\x07','\x80','\x7E','\x00','\x00','\x7C','\x0B','\x0F','\x85','\x0E','\x01','\x83','\xC5','\x10',
	'\xE2','\xF1','\xCD','\x18','\x88','\x56','\x00','\x55','\xC6','\x46','\x11','\x05','\xC6','\x46','\x10','\x00',
	'\xB4','\x41','\xBB','\xAA','\x55','\xCD','\x13','\x5D','\x72','\x0F','\x81','\xFB','\x55','\xAA','\x75','\x09',
	'\xF7','\xC1','\x01','\x00','\x74','\x03','\xFE','\x46','\x10','\x66','\x60','\x80','\x7E','\x10','\x00','\x74',
	'\x26','\x66','\x68','\x00','\x00','\x00','\x00','\x66','\xFF','\x76','\x08','\x68','\x00','\x00','\x68','\x00',
	'\x7C','\x68','\x01','\x00','\x68','\x10','\x00','\xB4','\x42','\x8A','\x56','\x00','\x8B','\xF4','\xCD','\x13',
	'\x9F','\x83','\xC4','\x10','\x9E','\xEB','\x14','\xB8','\x01','\x02','\xBB','\x00','\x7C','\x8A','\x56','\x00',
	'\x8A','\x76','\x01','\x8A','\x4E','\x02','\x8A','\x6E','\x03','\xCD','\x13','\x66','\x61','\x73','\x1C','\xFE',
	'\x4E','\x11','\x75','\x0C','\x80','\x7E','\x00','\x80','\x0F','\x84','\x8A','\x00','\xB2','\x80','\xEB','\x84',
	'\x55','\x32','\xE4','\x8A','\x56','\x00','\xCD','\x13','\x5D','\xEB','\x9E','\x81','\x3E','\xFE','\x7D','\x55',
	'\xAA','\x75','\x6E','\xFF','\x76','\x00','\xE8','\x8D','\x00','\x75','\x17','\xFA','\xB0','\xD1','\xE6','\x64',
	'\xE8','\x83','\x00','\xB0','\xDF','\xE6','\x60','\xE8','\x7C','\x00','\xB0','\xFF','\xE6','\x64','\xE8','\x75',
	'\x00','\xFB','\xB8','\x00','\xBB','\xCD','\x1A','\x66','\x23','\xC0','\x75','\x3B','\x66','\x81','\xFB','\x54',
	'\x43','\x50','\x41','\x75','\x32','\x81','\xF9','\x02','\x01','\x72','\x2C','\x66','\x68','\x07','\xBB','\x00',
	'\x00','\x66','\x68','\x00','\x02','\x00','\x00','\x66','\x68','\x08','\x00','\x00','\x00','\x66','\x53','\x66',
	'\x53','\x66','\x55','\x66','\x68','\x00','\x00','\x00','\x00','\x66','\x68','\x00','\x7C','\x00','\x00','\x66',
	'\x61','\x68','\x00','\x00','\x07','\xCD','\x1A','\x5A','\x32','\xF6','\xEA','\x00','\x7C','\x00','\x00','\xCD',
	'\x18','\xA0','\xB7','\x07','\xEB','\x08','\xA0','\xB6','\x07','\xEB','\x03','\xA0','\xB5','\x07','\x32','\xE4',
	'\x05','\x00','\x07','\x8B','\xF0','\xAC','\x3C','\x00','\x74','\x09','\xBB','\x07','\x00','\xB4','\x0E','\xCD',
	'\x10','\xEB','\xF2','\xF4','\xEB','\xFD','\x2B','\xC9','\xE4','\x64','\xEB','\x00','\x24','\x02','\xE0','\xF8',
	'\x24','\x02','\xC3','\x49','\x6E','\x76','\x61','\x6C','\x69','\x64','\x20','\x70','\x61','\x72','\x74','\x69',
	'\x74','\x69','\x6F','\x6E','\x20','\x74','\x61','\x62','\x6C','\x65','\x00','\x45','\x72','\x72','\x6F','\x72',
	'\x20','\x6C','\x6F','\x61','\x64','\x69','\x6E','\x67','\x20','\x6F','\x70','\x65','\x72','\x61','\x74','\x69',
	'\x6E','\x67','\x20','\x73','\x79','\x73','\x74','\x65','\x6D','\x00','\x4D','\x69','\x73','\x73','\x69','\x6E',
	'\x67','\x20','\x6F','\x70','\x65','\x72','\x61','\x74','\x69','\x6E','\x67','\x20','\x73','\x79','\x73','\x74',
	'\x65','\x6D','\x00','\x00','\x00','\x63','\x7B','\x9A','\xF4','\x8F','\xF6','\x9D','\x00','\x00','\x80','\x6B',
    '\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00',
    '\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00',
    '\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00',
    '\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x55','\xAA'
};

#ifdef __cplusplus
}
#endif

#include "..\vcbt\vcbt\ntfs.h"

#ifndef _IRM_DISK_GUID_DEF
#define _IRM_DISK_GUID_DEF

// GUID type:        {00000000-0000-0000-0000-000000000000}
//#define PARTITION_ENTRY_UNUSED_GUID { 0x00000000, 0x0000, 0x0000, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } } 
DEFINE_GUID(PARTITION_ENTRY_UNUSED_GUID, 0x00000000, 0x0000, 0x0000, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

// GUID type:        {c12a7328-f81f-11d2-ba4b-00a0c93ec93b}
// Partition name:   EFI system partition
DEFINE_GUID(PARTITION_SYSTEM_GUID, 0xC12A7328L, 0xF81F, 0x11D2, 0xBA, 0x4B, 0x00, 0xA0, 0xC9, 0x3E, 0xC9, 0x3B);

//GUID type:		{EBD0A0A2-B9E5-4433-87C0-68B6B72699C7}
//Partition name:   Basic data partition
DEFINE_GUID(PARTITION_BASIC_DATA_GUID, 0xEBD0A0A2L, 0xB9E5, 0x4433, 0x87, 0xC0, 0x68, 0xB6, 0xB7, 0x26, 0x99, 0xC7);

//GUID type:		{de94bba4-06d1-4d40-a16a-bfd50179d6ac}
//Partition name:   Microsoft Recovery partition
DEFINE_GUID(PARTITION_MSFT_RECOVERY_GUID, 0xde94bba4L, 0x06d1, 0x4d40, 0xa1, 0x6a, 0xbf, 0xd5, 0x01, 0x79, 0xd6, 0xac);

//GUID type:        {af9b60a0-1431-4f62-bc68-3311714a69ad}
//Partition name:   LDM data partition
DEFINE_GUID(PARTITION_LDM_DATA_GUID, 0xAF9B60A0L, 0x1431, 0x4F62, 0xBC, 0x68, 0x33, 0x11, 0x71, 0x4A, 0x69, 0xAD);

//GUID type:        {5808c8aa-7e8f-42e0-85d2-e1e90434cfb3}
//Partition name:   LDM metadata partition
DEFINE_GUID(PARTITION_LDM_METADATA_GUID, 0x5808C8AAL, 0x7E8F, 0x42E0, 0x85, 0xD2, 0xE1, 0xE9, 0x04, 0x34, 0xCF, 0xB3);

//GUID type:        {e3c9e316-0b5c-4db8-817d-f92df00215ae}
//Partition name:   Microsoft reserved partition
DEFINE_GUID(PARTITION_MSFT_RESERVED_GUID, 0xE3C9E316L, 0x0B5C, 0x4DB8, 0x81, 0x7D, 0xF9, 0x2D, 0xF0, 0x02, 0x15, 0xAE);

DEFINE_GUID(LEGACY_MBR_PARTITION_GUID, 0x024DEE41L, 0x33E7, 0x11d3, 0x9D, 0x69, 0x00, 0x08, 0xC7, 0x81, 0xF3, 0x9F);

#define __IsEqualGUID( rguid1, rguid2 )   !memcmp ( rguid1 , rguid2 , sizeof( GUID ) )

#endif

#endif  

