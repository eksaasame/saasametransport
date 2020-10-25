#include "reg_edit_ex.h"
#include "Strsafe.h"
using namespace macho::windows;
using namespace macho;

#if _DEBUG
#include <assert.h>
#define ASSERT(condition) assert(condition)
#else
#define ASSERT(condition)
#endif

reg_edit_ex::reg_edit_ex(macho::windows::reg_edit_base &reg) : _reg(reg){
    _reg_keys_map[_T("HKEY_CLASSES_ROOT")] = HKEY_CLASSES_ROOT;
    _reg_keys_map[_T("HKEY_CURRENT_CONFIG")] = HKEY_CURRENT_CONFIG;
    _reg_keys_map[_T("HKEY_CURRENT_USER")] = HKEY_CURRENT_USER;
    _reg_keys_map[_T("HKCU")] = HKEY_CURRENT_USER;
    _reg_keys_map[_T("HKEY_LOCAL_MACHINE")] = HKEY_LOCAL_MACHINE;
    _reg_keys_map[_T("HKLM")] = HKEY_LOCAL_MACHINE;
    _reg_keys_map[_T("HKEY_USERS")] = HKEY_USERS;
    _reg_keys_map[_T("HKU")] = HKEY_USERS;

    _reg_types_map[_T("hex(0)")] = REG_NONE;
    _reg_types_map[_T("hex(1)")] = REG_SZ;
    _reg_types_map[_T("hex(2)")] = REG_EXPAND_SZ;
    _reg_types_map[_T("hex(3)")] = REG_BINARY;
    _reg_types_map[_T("hex(4)")] = REG_DWORD;
    _reg_types_map[_T("hex(5)")] = REG_DWORD_BIG_ENDIAN;
    _reg_types_map[_T("hex(6)")] = REG_LINK;
    _reg_types_map[_T("hex(7)")] = REG_MULTI_SZ;
    _reg_types_map[_T("hex(8)")] = REG_RESOURCE_LIST;
    _reg_types_map[_T("hex(9)")] = REG_FULL_RESOURCE_DESCRIPTOR;
    _reg_types_map[_T("hex(10)")] = REG_RESOURCE_REQUIREMENTS_LIST;
    _reg_types_map[_T("hex(b)")] = REG_QWORD;
    _reg_types_map[_T("hex")] = REG_BINARY;
    _reg_types_map[_T("dword")] = REG_DWORD;
}

bool reg_edit_ex::reg_key_path_parser(const std::wstring& reg_path, HKEY& key, std::wstring& key_path){
    std::vector<std::wstring> keys = macho::stringutils::tokenize(reg_path, _T("\\"), 4);
    if (keys.size() > 0){
        key = _reg_keys_map[keys[0]];
        key_path.clear();
        for (size_t i = 1; i < keys.size(); ++i){
            if (i > 1)
                key_path.append(_T("\\"));
            key_path.append(keys[i]);
        }
        return true;
    }
    return false;
}

DWORD reg_edit_ex::reg_get_increase_key_path(HKEY hKeyRoot, std::wstring &original, std::wstring &replacement){
    LONG        lResult;
    DWORD       dwSize;
    TCHAR       szName[MAX_PATH];
    HKEY        hKey;
    size_t      found;
    std::wstring   ParentKey, SubKey;
    LONG        IncreaseNum = 0;
    TCHAR       szSubKey[MAX_PATH];
    TCHAR       szSubKeyFormat[20];
    INT         iCount = 0;

    SecureZeroMemory(szName, sizeof(szName));
    SecureZeroMemory(szSubKey, sizeof(szSubKey));
    SecureZeroMemory(szSubKeyFormat, sizeof(szSubKeyFormat));

    found = original.find_last_of(_T("\\"));
    ParentKey = original.substr(0, found);
    SubKey = original.substr(found + 1);

    lResult = _reg.open_key(hKeyRoot, ParentKey.c_str(), 0, KEY_READ, &hKey);

    if (lResult == ERROR_SUCCESS){
        // Enumerate the keys
        dwSize = MAX_PATH;
        while ((lResult = _reg.enumerate_key(hKey, iCount, szName, &dwSize, NULL, NULL, NULL, NULL)) == ERROR_SUCCESS){
            LONG num = _ttol(szName);
            if (IncreaseNum < num)
                IncreaseNum = num;

            dwSize = MAX_PATH;
            SecureZeroMemory(szName, sizeof(szName));
            iCount++;
        }
        _reg.close_key(hKey);
    }

    if (lResult == ERROR_NO_MORE_ITEMS){
        if (iCount > 0)
            ++IncreaseNum;
        _stprintf_s(szSubKeyFormat, sizeof(szSubKeyFormat) / sizeof(TCHAR), _T("%%0%ii"), SubKey.length());
        _stprintf_s(szSubKey, sizeof(szSubKey) / sizeof(TCHAR), szSubKeyFormat, IncreaseNum);
        replacement.append(ParentKey);
        replacement.append(_T("\\"));
        replacement.append(szSubKey);
        lResult = ERROR_SUCCESS;
    }
    return lResult;
}

int reg_edit_ex::hex_to_dec(const std::wstring& csHex){

    int tot = 0;
    TCHAR hex[17] = { _T('0'), _T('1'), _T('2'), _T('3'), _T('4'), _T('5'), _T('6'), _T('7'), _T('8'), _T('9'), _T('a'), _T('b'), _T('c'), _T('d'), _T('e'), _T('f'), _T('\0') };
    int num[16] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };

    int base = 1;
    int nCtr = (int)csHex.length() - 1;

    std::wstring csHexLocal;
    csHexLocal.resize(csHex.size());
    transform(csHex.begin(), csHex.end(), csHexLocal.begin(), tolower); // toupper/tolower

    for (int n = nCtr; n >= 0; n--){
        TCHAR lc = csHexLocal[n];
        int vl = 0;

        for (int n = 0; n < 16; n++){
            TCHAR cc = hex[n];
            if (lc == cc){
                vl = num[n];
                break;
            }
        }
        tot = tot + (base * vl);
        base = base * 16;
    }
    return tot;
}


LONG reg_edit_ex::reg_set_hex_value(HKEY hKey, DWORD dwType, std::wstring& KeyValueName, std::wstring& KeyValue){

    std::vector<std::wstring> tokens;
    DWORD        dwData = 0;
    LONG         status = ERROR_SUCCESS;
    std::wstring    delimiters = _T(",");
    BYTE         *lpData = NULL;
    DWORD        cbData = 0;

    switch (dwType){
    case REG_DWORD:
    case REG_DWORD_BIG_ENDIAN:{
        dwData = hex_to_dec(KeyValue);
        cbData = sizeof(DWORD);
        status = _reg.set_value(hKey, (LPCTSTR)KeyValueName.c_str(), NULL, dwType, (LPBYTE)&dwData, cbData);
    }
                              break;
    default:{
        tokens = macho::stringutils::tokenize(KeyValue, delimiters);
        lpData = (LPBYTE)malloc(sizeof(BYTE) * tokens.size());
        cbData = (DWORD)tokens.size();

        for (DWORD i = 0; i < cbData; i++){
            lpData[i] = hex_to_dec(tokens[i]);
        }
        status = _reg.set_value(hKey, (LPCTSTR)KeyValueName.c_str(), NULL, dwType, (LPBYTE)lpData, cbData);
    }
    }

    if (lpData)
        free(lpData);

    return status;
}

std::wstring  reg_edit_ex::reg_sz_key_value_parser(const std::wstring &txtRegValue){

    std::wstring::size_type  pos = 0;
    std::wstring txtResult;
    if ((pos = txtRegValue.find(_T("\\"))) != std::wstring::npos){
        txtResult = txtRegValue.substr(0, pos);
        switch (txtRegValue.at(pos + 1)){
        case _T('\\'):
            txtResult.append(_T("\\"));
            txtResult = txtResult + reg_sz_key_value_parser(txtRegValue.substr(pos + 2));
            break;
        default:
            txtResult = txtRegValue;
        }
    }
    else{
        txtResult = txtRegValue;
    }
    return txtResult;
}

bool reg_edit_ex::reg_key_value_parser( const std::wstring& txtRegString, DWORD& dwType, std::wstring& KeyName, std::wstring& KeyValue, bool& append, bool& hex){

    std::wstring::size_type  pos = 0;
    std::wstring sub_string, txtRegType;

    if ((pos = txtRegString.find(_T("="))) != std::wstring::npos){
        // Is default entry or not?
        if ((pos > 0) && (txtRegString[pos - 1] == _T('@'))) {
            KeyName = _T("");   // Set the key entry name as "" for the default key value.
        }
        else {
            KeyName = txtRegString.substr(1, pos - 2);  // Get the key entry name from the string line
        }
        dwType = REG_NONE;
        sub_string = txtRegString.substr(pos + 1, txtRegString.length() - (pos + 1));
        if ((sub_string[0] != _T('\"')) && ((pos = sub_string.find(_T(":"))) != std::wstring::npos)){
            txtRegType = sub_string.substr(0, pos);
            hex = true;
            dwType = _reg_types_map[txtRegType];
            if (sub_string[sub_string.length() - 1] == _T('\\')){
                // If the end of the string is "\", we need to append the next line string to the key entry value. 
                append = true;
                KeyValue = sub_string.substr(pos + 1, sub_string.length() - pos - 2);
            }
            else{
                KeyValue = sub_string.substr(pos + 1, sub_string.length() - pos - 1);
            }
        }
        else{
            hex = false;
            if (sub_string.compare(_T("-")) == 0){
                // if the key entry value is "-", we will delete this key entry.
                dwType = REG_DELETE;
                KeyValue = sub_string;
            }
            else {
                dwType = REG_SZ;
                KeyValue = reg_sz_key_value_parser(sub_string.substr(1, sub_string.length() - 2));
            }
        }
        return true;
    }
    return false;
}

bool reg_edit_ex::key_value_exists(HKEY hKey, const std::wstring& ValueName){
    DWORD dwRet = 0;
    DWORD cbData = 0;
    dwRet = _reg.query_value(hKey, ValueName.c_str(), NULL, NULL, NULL, &cbData);
    return ((dwRet == ERROR_FILE_NOT_FOUND) ? FALSE : TRUE);
}

LONG reg_edit_ex::reg_multi_sz_parser( HKEY hKey, const std::wstring& txtRegString){

    std::wstring::size_type  pos = 0;
    std::wstring sub_string, txtRegType;
    std::wstring ValueName;
    std::wstring KeyValue;
    LONG      status = ERROR_INVALID_DATA;

    if ((pos = txtRegString.find(_T("="))) != std::wstring::npos){
        // Is default entry or not?
        if ((pos > 0) && (txtRegString[pos - 1] == _T('@'))){
            ValueName = _T("");   // Set the key entry name as "" for the default key value.
        }
        else{
            ValueName = txtRegString.substr(1, pos - 2);  // Get the key entry name from the string line
        }

        sub_string = txtRegString.substr(pos + 1, txtRegString.length() - (pos + 1));

        if ((sub_string[0] != _T('\"')) && ((pos = sub_string.find(_T(":"))) != std::wstring::npos)){

            txtRegType = sub_string.substr(0, pos);
            //transform( txtRegType.begin(), txtRegType.end(), txtRegType.begin(), toupper ); // toupper/tolower

            if (_reg_types_map[txtRegType] == REG_MULTI_SZ){

                KeyValue = sub_string.substr(pos + 1, sub_string.length() - pos - 1);

                if (KeyValue[0] == _T('+')){
                    status = reg_prepend_sz_to_multi_sz( hKey, ValueName, reg_sz_key_value_parser(KeyValue.substr(2, KeyValue.length() - 3)));
                }
                else if (KeyValue[0] == _T('\"')){
                    status = reg_prepend_sz_to_multi_sz( hKey, ValueName, reg_sz_key_value_parser(KeyValue.substr(1, KeyValue.length() - 2)));
                }
                else if (KeyValue[0] == _T('-')){
                    status = reg_remove_sz_from_multi_sz( hKey, ValueName, reg_sz_key_value_parser(KeyValue.substr(2, KeyValue.length() - 3)));
                }
            }
            else
                status = ERROR_INVALID_DATA;
        }
    }
    return status;

}

LONG reg_edit_ex::import(std::vector<std::wstring> reg_lines, bool is_over_write){

    DWORD        status = ERROR_SUCCESS;
    std::wstring reg_key_path, reg_key_value_name, reg_key_value;
    HKEY         hRootKey;
    std::vector<std::wstring> lines;
    bool            append = false;
    bool            update = false;
    bool            hex = false;
    DWORD           key_type;
    HKEY            hKey = NULL;
    std::wstring    original, replacement;
    std::wstring    old_value, target_value;
    TCHAR		    default_control_set[15] = { 0 };

    for (size_t index = 0; index < reg_lines.size(); ++index){
        lines = macho::stringutils::tokenize2(reg_lines[index], _T("\r\n"));
        for (size_t i = 0; i < lines.size(); ++i){
            std::wstring current_line, sub_string;
            size_t    pos;
            current_line = lines[i];
            if (current_line.length()){
                if ((current_line[0] == _T('[')) && (current_line[current_line.length() - 1] == _T(']'))){
                    if (((current_line[1] == _T('H')) || (current_line[1] == _T('h')))
                        || ((current_line[1] == _T('+')) && ((current_line[2] == _T('H')) || (current_line[2] == _T('h'))))
                        || ((current_line[1] == _T('*')) && ((current_line[2] == _T('H')) || (current_line[2] == _T('h'))))
                        || ((current_line[1] == _T('@')) && ((current_line[2] == _T('H')) || (current_line[2] == _T('h'))))
                        ){
                        if ((current_line[1] == _T('H')) || (current_line[1] == _T('h')))
                            sub_string = current_line.substr(1, current_line.length() - 2);
                        else
                            sub_string = current_line.substr(2, current_line.length() - 3);

                        if (reg_key_path_parser(sub_string, hRootKey, reg_key_path)){
                            if (current_line[1] == _T('*')){
                                original = reg_key_path;
                                if (reg_get_increase_key_path( hRootKey, original, replacement) == ERROR_SUCCESS){
                                    reg_key_path = replacement;
                                    std::vector<std::wstring> tokens;
                                    std::wstring delimiters = _T("\\");
                                    tokens = macho::stringutils::tokenize(original, delimiters);
                                    if ((tokens[tokens.size() - 2][0] == _T('{')) && (tokens[tokens.size() - 2][tokens[tokens.size() - 2].length() - 1] == _T('}'))){
                                        old_value = tokens[tokens.size() - 2] + _T("\\") + tokens[tokens.size() - 1];
                                        tokens = macho::stringutils::tokenize(replacement, delimiters);
                                        target_value = tokens[tokens.size() - 2] + _T("\\") + tokens[tokens.size() - 1];
                                    }
                                }
                                else{
                                    original.clear();
                                }
                            }
                            else if (original.length() > 0){
                                if ((pos = reg_key_path.find(original)) != std::wstring::npos){
                                    reg_key_path.replace(pos, original.length(), replacement);
                                }
                            }
                            if (hKey != NULL){
                                if ((status = _reg.close_key(hKey)) == ERROR_SUCCESS){
                                    hKey = NULL;
                                }
                            }
                            if (current_line[1] == _T('@')){
                                status = _reg.open_key(hRootKey, (LPCTSTR)reg_key_path.c_str(), 0, KEY_ALL_ACCESS, &hKey);
                                if (status == ERROR_FILE_NOT_FOUND){
                                    status = ERROR_SUCCESS;
                                    hKey = NULL;
                                }
                            }
                            else {
                                status = _reg.create_key(hRootKey, (LPCTSTR)reg_key_path.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL);
                            }
                        }
                    }
                    else if ((current_line[1] == _T('-')) && (current_line[2] == _T('H'))){
                        sub_string = current_line.substr(2, current_line.length() - 3);
                        if (reg_key_path_parser(sub_string, hRootKey, reg_key_path)){
                            if (hKey != NULL){
                                if ((status = _reg.close_key(hKey)) == ERROR_SUCCESS){
                                    hKey = NULL;
                                }
                            }
                            status = _reg.delete_tree(hRootKey, reg_key_path.c_str());
                            if (ERROR_FILE_NOT_FOUND == status) 
                                status = ERROR_SUCCESS;
                        }
                    }
                }
                else if ((pos = current_line.find(_T("="))) != std::wstring::npos){
                    if (hKey != NULL) {
                        status = reg_multi_sz_parser(hKey, current_line);
                        if (status == ERROR_INVALID_DATA) {
                            status = ERROR_SUCCESS;
                        }
                        else {
                            if (status != ERROR_SUCCESS){
                                LOG(LOG_LEVEL_ERROR, _T("%s : Error ( 0x%08X )"), (LPCTSTR)current_line.c_str(), status);
                            }
                            continue;
                        }
                    }
                    reg_key_value_parser(current_line, key_type, reg_key_value_name, reg_key_value, append, hex);
                    if ((hKey != NULL) && (!append)){
                        update = (is_over_write) ? true : !key_value_exists( hKey, reg_key_value_name);
                        if (key_type == REG_DELETE){
                            if (key_value_exists( hKey, reg_key_value_name)){
                                status = _reg.delete_value(hKey, reg_key_value_name.c_str());
                            }
                        }
                        else if (hex) {
                            if (update)
                                status = reg_set_hex_value( hKey, key_type, reg_key_value_name, reg_key_value);
                        }
                        else {
                            if (update){
                                if ((old_value.length() > 0) && ((pos = reg_key_value.find(old_value)) != std::wstring::npos)){
                                    reg_key_value.replace(pos, old_value.length(), target_value);
                                }
                                status = _reg.set_value(hKey, reg_key_value_name.c_str(), NULL, key_type, (const BYTE *)(LPCTSTR)reg_key_value.c_str(), (DWORD)(reg_key_value.length() + 1) * sizeof(TCHAR));
                            }
                        }
                    }
                }
                else{
                    if ((hKey != NULL) && (append)){
                        if (current_line[current_line.length() - 1] == _T('\\')){
                            reg_key_value.append(current_line.begin(), current_line.end() - 1);
                        }
                        else{
                            reg_key_value.append(current_line);
                            append = false;
                            if (update)
                                status = reg_set_hex_value( hKey, key_type, reg_key_value_name, reg_key_value);
                        }
                    }
                }
                if (status != ERROR_SUCCESS){
                    LOG(LOG_LEVEL_ERROR, _T("%s : Error ( 0x%08X )"), (LPCTSTR)current_line.c_str(), status);
                }
            }
        }
    }
    if (hKey != NULL)
        status = _reg.close_key(hKey);

    return status;
}

/*
* returns the length (in characters) of the buffer required to hold this
* MultiSz, INCLUDING the trailing null.
*
* example: MultiSzLength("foo\0bar\0") returns 9
*
* note: since MultiSz cannot be null, a number >= 1 will always be returned
*
* parameters:
*   MultiSz - the MultiSz to get the length of
*/
size_t reg_edit_ex::multi_sz_length(__in IN LPTSTR MultiSz){

    size_t len = 0;
    size_t totalLen = 0;

    ASSERT(MultiSz != NULL);

    // search for trailing null character
    while (*MultiSz != _T('\0'))
    {
        len = _tcslen(MultiSz) + 1;
        MultiSz += len;
        totalLen += len;
    }

    // add one for the trailing null character
    return (totalLen + 1);
}

/*
* prepend the given string to a MultiSz
*
* returns true if successful, false if not (will only fail in memory
* allocation)
*
* note: This WILL allocate and free memory, so don't keep pointers to the
* MultiSz passed in.
*
* parameters:
*   SzToPrepend - string to prepend
*   MultiSz     - pointer to a MultiSz which will be prepended-to
*/
BOOLEAN reg_edit_ex::prepend_sz_to_multi_sz(__in LPTSTR SzToPrepend, __deref_inout LPTSTR *MultiSz){

    size_t szLen;
    size_t multiSzLen;
    LPTSTR newMultiSz = NULL;

    ASSERT(SzToPrepend != NULL);
    ASSERT(MultiSz != NULL);

    if (SzToPrepend == NULL || MultiSz == NULL) {
        return (FALSE);
    }

    // get the size, in bytes, of the two buffers
    szLen = (_tcslen(SzToPrepend) + 1)*sizeof(_TCHAR);
    multiSzLen = multi_sz_length(*MultiSz)*sizeof(_TCHAR);
    newMultiSz = (LPTSTR)malloc(szLen + multiSzLen);

    if (newMultiSz == NULL)
    {
        return (FALSE);
    }

    // recopy the old MultiSz into proper position into the new buffer.
    // the (char*) cast is necessary, because newMultiSz may be a wchar*, and
    // szLen is in bytes.

    memcpy(((char*)newMultiSz) + szLen, *MultiSz, multiSzLen);

    // copy in the new string
    StringCbCopy(newMultiSz, szLen, SzToPrepend);

    free(*MultiSz);
    *MultiSz = newMultiSz;

    return (TRUE);
}

/*
* Deletes all instances of a string from within a multi-sz.
*
* parameters:
*   FindThis        - the string to find and remove
*   FindWithin      - the string having the instances removed
*   NewStringLength - the new string length
*/
size_t reg_edit_ex::multi_sz_search_and_delete_case_insensitive(__in IN LPTSTR FindThis, __in IN LPTSTR FindWithin, OUT size_t *NewLength){

    LPTSTR search;
    size_t currentOffset;
    DWORD  instancesDeleted;
    size_t searchLen;

    ASSERT(FindThis != NULL);
    ASSERT(FindWithin != NULL);
    ASSERT(NewLength != NULL);

    currentOffset = 0;
    instancesDeleted = 0;
    search = FindWithin;

    *NewLength = multi_sz_length(FindWithin);

    // loop while the multisz null terminator is not found
    while (*search != _T('\0'))
    {
        // length of string + null char; used in more than a couple places
        searchLen = _tcslen(search) + 1;

        // if this string matches the current one in the multisz...
        if (_tcsicmp(search, FindThis) == 0)
        {
            // they match, shift the contents of the multisz, to overwrite the
            // string (and terminating null), and update the length
            instancesDeleted++;
            *NewLength -= searchLen;
            memmove(search,
                search + searchLen,
                (*NewLength - currentOffset) * sizeof(TCHAR));
        }
        else
        {
            // they don't mactch, so move pointers, increment counters
            currentOffset += searchLen;
            search += searchLen;
        }
    }

    return (instancesDeleted);
}

LONG reg_edit_ex::reg_prepend_sz_to_multi_sz(HKEY hKey, const std::wstring& ValueName, const std::wstring& Value){

    DWORD  dwRet = 0;
    DWORD  cbData = 0;
    DWORD  dwType = REG_NONE;
    LPTSTR lpData = NULL;
    size_t length = 0; // character length
    size_t size = 0; // buffer size

    dwRet = _reg.query_value(hKey, ValueName.c_str(), NULL, &dwType, NULL, &cbData);

    if (dwRet == ERROR_SUCCESS){

        if (dwType == REG_MULTI_SZ){

            lpData = (LPTSTR)malloc(cbData);

            if (lpData != NULL){

                memset(lpData, 0, cbData);
                dwRet = _reg.query_value(hKey, ValueName.c_str(), NULL, &dwType, (LPBYTE)lpData, &cbData);

                if (dwRet == ERROR_SUCCESS){

                    LPTSTR lpBuf = NULL;
                    // remove all instances of filter from driver list
                    multi_sz_search_and_delete_case_insensitive((LPTSTR)Value.c_str(), lpData, &length);

                    length = multi_sz_length(lpData) + Value.length() + 1;
                    size = length * sizeof(_TCHAR);
                    lpBuf = (LPTSTR)malloc(size);

                    if (lpBuf != NULL){

                        memset(lpBuf, 0, size);
                        // swap the buffers out
                        memcpy(lpBuf, lpData, multi_sz_length(lpData) * sizeof(_TCHAR));
                        free(lpData);
                        lpData = lpBuf;

                        // add the driver to the driver list
                        prepend_sz_to_multi_sz((LPTSTR)Value.c_str(), &lpData);
                    }
                    else{

                        free(lpData);
                        lpData = NULL;
                    }
                }
            }
        }
        else
            return ERROR_INVALID_DATATYPE; // RegType is mismatch.
    }
    else if (dwRet == ERROR_FILE_NOT_FOUND){
        // if there is no such value in the registry, and we can just put one there
        // make room for the string, string null terminator, and multisz null
        // terminator
        length = Value.length() + 1;
        size = (length + 1) * sizeof(_TCHAR);
        lpData = (LPTSTR)malloc(size);

        if (lpData != NULL){

            memset(lpData, 0, size);
            // copy the string into the new buffer
            memcpy(lpData, Value.c_str(), length * sizeof(_TCHAR));
        }
    }

    if (lpData != NULL){

        length = multi_sz_length(lpData);
        dwRet = _reg.set_value(hKey, ValueName.c_str(), NULL, REG_MULTI_SZ, (LPBYTE)lpData, (DWORD)(length * sizeof(TCHAR)));
        free(lpData);
    }
    else
        dwRet = STATUS_NO_MEMORY;

    return dwRet;
}

LONG reg_edit_ex::reg_remove_sz_from_multi_sz( HKEY hKey, const std::wstring& ValueName, const std::wstring& Value)
{
    DWORD  dwRet = 0;
    DWORD  cbData = 0;
    DWORD  dwType = REG_NONE;
    LPTSTR lpData = NULL;
    size_t length = 0; // character length

    dwRet = _reg.query_value(hKey, ValueName.c_str(), NULL, &dwType, NULL, &cbData);

    if ((dwRet != ERROR_FILE_NOT_FOUND) && (dwType == REG_MULTI_SZ)){

        lpData = (LPTSTR)malloc(cbData);

        if (lpData){

            dwRet = _reg.query_value(hKey, ValueName.c_str(), NULL, &dwType, (LPBYTE)lpData, &cbData);

            if (dwRet == ERROR_SUCCESS){

                multi_sz_search_and_delete_case_insensitive((LPTSTR)Value.c_str(), lpData, &length);

                length = multi_sz_length(lpData);

                ASSERT(length > 0);

                if (length == 1){
                    // if the length of the list is 1, the return value from
                    // MultiSzLength() was just accounting for the trailing '\0', so we can
                    // delete the registry key value.
                    dwRet = _reg.delete_value(hKey, ValueName.c_str());
                }
                else
                    dwRet = _reg.set_value(hKey, ValueName.c_str(), NULL, REG_MULTI_SZ, (LPBYTE)lpData, (DWORD)(length * sizeof(TCHAR)));
            }
            free(lpData);
        }
    }
    else
        dwRet = ERROR_SUCCESS;

    return dwRet;
}