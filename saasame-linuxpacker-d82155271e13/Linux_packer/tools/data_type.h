#pragma once
#ifndef data_type_H
#define data_type_H
#include <uuid/uuid.h>
#include <string>
#include <vector>
#include <map>
#include "string_operation.h"
#include "universal_disk_rw.h"

typedef uint8_t BYTE;
typedef uint16_t WORD;
typedef uint32_t DWORD;
typedef unsigned long ULONG, ULONG_PTR, *PULONG_PTR, DWORD_PTR, *PDWORD_PTR; ;
typedef unsigned long long ULONGLONG;
typedef unsigned long long LONGLONG;
#define UUID uuid_t

#define MAKEWORD(a, b)      ((WORD)(((BYTE)(((DWORD_PTR)(a)) & 0xff)) | ((WORD)((BYTE)(((DWORD_PTR)(b)) & 0xff))) << 8))
#define MAKELONG(a, b)      ((LONG)(((WORD)(((DWORD_PTR)(a)) & 0xffff)) | ((DWORD)((WORD)(((DWORD_PTR)(b)) & 0xffff))) << 16))
#define LOWORD(l)           ((WORD)(((DWORD_PTR)(l)) & 0xffff))
#define HIWORD(l)           ((WORD)((((DWORD_PTR)(l)) >> 16) & 0xffff))
#define LOBYTE(w)           ((BYTE)(((DWORD_PTR)(w)) & 0xff))
#define HIBYTE(w)           ((BYTE)((((DWORD_PTR)(w)) >> 8) & 0xff))


#pragma pack(push)
#pragma pack(1)
using namespace std;
#define GUID_size_with_dash 36
typedef struct _GUID {          // size is 16
    DWORD Data1;
    WORD   Data2;
    WORD   Data3;
    BYTE  Data4[8];
    _GUID() :Data1(0), Data2(0), Data3(0) { memset(Data4, 0, sizeof(Data4)); }
    static bool get_GUID_from_string(string & input,_GUID & result_GUID)
    {
        vector<string> strVec;
        if (sscanf(input.c_str(), "%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX",
            &result_GUID.Data1, &result_GUID.Data2, &result_GUID.Data3,
            &result_GUID.Data4[0], &result_GUID.Data4[1], &result_GUID.Data4[2], &result_GUID.Data4[3],
            &result_GUID.Data4[4], &result_GUID.Data4[5], &result_GUID.Data4[6], &result_GUID.Data4[7]) != 11)
        {
            return false;
        }
        return true;
    }
    string to_string() {
        return string_op::strprintf("%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX",
            Data1, Data2, Data3,
            Data4[0], Data4[1], Data4[2], Data4[3],
            Data4[4], Data4[5], Data4[6], Data4[7]);
    }
} GUID;

struct changed_area {
    typedef std::vector<changed_area> vtr;
    typedef std::map<std::string, vtr> map;
    changed_area() : start(0), length(0), src_start(0), _rw(NULL){}
    changed_area(uint64_t _start, uint64_t _length, uint64_t _src_start = 0, universal_disk_rw::ptr rw_ = NULL) : start(_start), length(_length), src_start(_src_start), _rw(rw_) { /*printf("_start = %llu\r\n, _length = %llu\r\n", _start, _length);*/ }
    struct compare {
        bool operator() (changed_area const & lhs, changed_area const & rhs) const {
            return lhs.start < rhs.start;
        }
    };
    void clear()
    {
        start = 0;
        length = 0;
        src_start = 0;
        _rw = NULL;
    }
    bool isEmpty(){
        return (start == 0) && (length == 0) && (src_start == 0) && (_rw == NULL);
    }
    uint64_t start;
    uint64_t length;
    uint64_t src_start;
    universal_disk_rw::ptr _rw;
};
#pragma pack(pop)
#endif