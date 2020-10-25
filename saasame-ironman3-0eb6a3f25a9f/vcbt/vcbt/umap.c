
#include "type.h"
#include <ntddk.h>
#include "umap.h"
#include "trace.h"
#include "umap.tmh"

int	test_umap_bit(unsigned char* buffer, ULONGLONG bit){
    return buffer[bit >> 3] & (1 << (bit & 7));
}

int set_umap_bit(unsigned char* buffer, ULONGLONG bit){
    return buffer[bit >> 3] |= (1 << (bit & 7));
}

int clear_umap_bit(unsigned char* buffer, ULONGLONG bit){
    return buffer[bit >> 3] &= ~(1 << (bit & 7));
}

void merge_umap(unsigned char* buffer, unsigned char* source, ULONG size_in_bytes){
    for (ULONG index = 0; index < size_in_bytes; index++)
        buffer[index] |= source[index];
}

void merge_umap_ex(unsigned char* buffer, ULONG resolution, unsigned char* old_buffer, ULONG old_resolution, ULONG size_in_bytes){
    ULONG current_bit = 0;
    ULONG diff = 0;
    ULONG index = 0;
    ULONG count = 0;
    bool  dirty = false;
    if (old_resolution > resolution){
        diff = 1 << (old_resolution - resolution);
        for (index = 0; index < (size_in_bytes * 8); index++){
            if (test_umap_bit(old_buffer, index)){
                for (count = 0; count < diff; count++)
                    set_umap_bit(buffer, current_bit + count);
            }
            current_bit += diff;
        }
    }
    else if (old_resolution == resolution){
        merge_umap(buffer, old_buffer, size_in_bytes);
    }
    else{
        diff = 1 << (resolution - old_resolution);
        for (index = 0; index < ((size_in_bytes * 8 + diff - 1) / diff); index++){
            for (count = 0; count < diff; count++){
                if ((index * diff + count) / diff >= size_in_bytes)
                    break;
                else if (test_umap_bit(old_buffer, index * diff + count)){
                    dirty = true;
                    break;
                }
            }

            if (dirty)
                set_umap_bit(buffer, current_bit);
            dirty = false;
            current_bit++;
        }
    }
}

int set_umap(unsigned char* buffer, ULONGLONG start, ULONG length, ULONG resolution){
    int ret = 0;
    for (ULONGLONG curbit = (start >> resolution); curbit <= (start + length - 1) >> resolution; curbit++){
#if 1
        if (curbit >= RESOLUTION_UMAP_BIT_SIZE){
            DoTraceMsg(TRACE_LEVEL_ERROR, "The bit %I64u is out of scope. (Start: %I64u, Length: %d, Resolution: %d)", curbit, start, length, resolution);
            break;
        }
#endif   
        if (0 == test_umap_bit(buffer, curbit)){
            ret = set_umap_bit(buffer, curbit);
        }
    }
    return ret;
}