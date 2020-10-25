#ifndef _UMAP_
#define _UMAP_

#define RESOLUTION_UMAP_SIZE        65536   // UMAP file is 64KB by default  2^16 
#define RESOLUTION_UMAP_BIT_SIZE    524288  // 2^19

#define UMAP_RESOLUTION1            22      // UMAP buffer resoulution is 2^22 =    4MB/bit    2TB (2^41)
#define UMAP_RESOLUTION2            23      // UMAP buffer resoulution is 2^23 =    8MB/bit    4TB (2^42)
#define UMAP_RESOLUTION3            24      // UMAP buffer resoulution is 2^24 =   16MB/bit    8TB (2^43)
#define UMAP_RESOLUTION4            25      // UMAP buffer resoulution is 2^25 =   32MB/bit   16TB (2^44)
#define UMAP_RESOLUTION5            26      // UMAP buffer resoulution is 2^26 =   64MB/bit   32TB (2^45) 
#define UMAP_RESOLUTION6            27      // UMAP buffer resoulution is 2^27 =  128MB/bit   64TB (2^46)
#define UMAP_RESOLUTION7            28      // UMAP buffer resoulution is 2^28 =  256MB/bit  128TB (2^47)
#define UMAP_RESOLUTION8            29      // UMAP buffer resoulution is 2^29 =  512MB/bit  256TB (2^48)
#define UMAP_RESOLUTION9            30      // UMAP buffer resoulution is 2^30 = 1024MB/bit  512TB (2^49)

#define UMAP_RESOLUTION_FILE_       L"v%d.umap"
#define UMAP_RESOLUTION_FILE        L"\\v%d.umap"
#define UMAP_RESOLUTION1_FILE       L"\\v1.umap"
#define UMAP_RESOLUTION2_FILE       L"\\v2.umap"
#define UMAP_RESOLUTION3_FILE       L"\\v3.umap"
#define UMAP_RESOLUTION4_FILE       L"\\v4.umap"
#define UMAP_RESOLUTION5_FILE       L"\\v5.umap"
#define UMAP_RESOLUTION6_FILE       L"\\v6.umap"
#define UMAP_RESOLUTION7_FILE       L"\\v7.umap"
#define UMAP_RESOLUTION8_FILE       L"\\v8.umap"
#define UMAP_RESOLUTION9_FILE       L"\\v9.umap"

int set_umap(unsigned char* buffer, ULONGLONG start, ULONG length, ULONG resolution);
void merge_umap(unsigned char* buffer, unsigned char* backup, ULONG size_in_byte);
void merge_umap_ex(unsigned char* buffer, ULONG resolution, unsigned char* old_buffer, ULONG old_resolution, ULONG size_in_bytes);

int	test_umap_bit(unsigned char* buffer, ULONGLONG bit);
int set_umap_bit(unsigned char* buffer, ULONGLONG bit);
int clear_umap_bit(unsigned char* buffer, ULONGLONG bit);

#endif