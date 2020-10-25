
#ifndef _TYPE_
#define _TYPE_

typedef enum {
    false = 0,
    true = 1
} bool;

#define MAX_GUID        40
#define MAX_ID          128
#define MAX_FILE_PATH   620
#define MAX_REG_PATH    620
typedef unsigned long       DWORD;
typedef unsigned char       BYTE;
typedef unsigned short      WORD;

#ifdef POOL_TAGGING
#ifdef ExAllocatePool
#undef ExAllocatePool
#endif
#define ExAllocatePool(a,b) ExAllocatePoolWithTag(a,b,'vcbt')
#endif

#define CW_TAG 'vcbt'

#define __malloc(size)   ( size > 0 ) ? ExAllocatePoolWithTag(NonPagedPool, size, CW_TAG) : NULL;
#define __free(p)        if(NULL != p) { ExFreePool(p); p = NULL; }

#endif