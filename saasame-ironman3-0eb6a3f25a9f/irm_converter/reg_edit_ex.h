#pragma once
#ifndef __REG_EDIT_EX__
#define __REG_EDIT_EX__

#include <macho.h>

/* ===================================================
* reg_edit_ex::import(std::vector<std::wstring> reg_lines, bool is_over_write)
* Important Params:
* 	reg_lines	           : format string for registry import usage
* 	is_over_write           : overwrite the registry value
*
* Notes: following are the rules for the format string.
*
* [HK...   Import the Reg Key
* [+HK...  Import the Reg Key
* [*HK...  Add the Reg Key By the increase number
* [@HK...  Import the Reg Key if the key is existed.
* [-HK...  Delete the Reg Key
*
* =REG_MULTI_SZ:+""  Prepend the string to the multi string value
* =REG_MULTI_SZ:-""  Remove the string from the multi string value
* =-                 Delete Registry Key Value
*
* @= (Default)
* ="" REG_SZ
* =hex(2): REG_EXPAND_SZ
* =hex: REG_BINARY
* =dword: REG_DWORD
* =dword: REG_DWORD_BIG_ENDIAN
* =hex(6): REG_LINK
* =hex(7): REG_MULTI_SZ
* =hex(8): REG_RESOURCE_LIST
* =hex(9): REG_FULL_RESOURCE_DESCRIPTOR
* =hex(10): REG_RESOURCE_REQUIREMENTS_LIST
* =hex(b): REG_QWORD
*/

class reg_edit_ex{
public:
    reg_edit_ex(macho::windows::reg_edit_base &reg);
    LONG          import(std::vector<std::wstring> reg_lines, bool is_over_write = false);
    inline LONG   import(std::wstring regs, bool is_over_write = false){
        return import(macho::stringutils::tokenize(regs, L"\n"), is_over_write);
    }
private:
    bool          reg_key_path_parser(const std::wstring& reg_path, HKEY& key, std::wstring& key_path);
    DWORD         reg_get_increase_key_path(HKEY hKeyRoot, std::wstring &txtOriginal, std::wstring &txtReplacement);
    LONG          reg_set_hex_value(HKEY hKey, DWORD dwType, std::wstring& KeyValueName, std::wstring& KeyValue);
    int           hex_to_dec(const std::wstring& csHex);
    std::wstring  reg_sz_key_value_parser(const std::wstring &txtRegValue);
    bool          reg_key_value_parser( const std::wstring& txtRegString, DWORD& dwType, std::wstring& KeyName, std::wstring& KeyValue, bool& IsAppend, bool& IsHex);
    bool          key_value_exists(HKEY hKey, const std::wstring& ValueName);
    LONG          reg_multi_sz_parser( HKEY hKey, const std::wstring& txtRegString);
    LONG          reg_prepend_sz_to_multi_sz(HKEY hKey, const std::wstring& ValueName, const std::wstring& Value);
    LONG          reg_remove_sz_from_multi_sz(HKEY hKey, const std::wstring& ValueName, const std::wstring& Value);
    size_t        multi_sz_search_and_delete_case_insensitive(__in IN LPTSTR FindThis, __in IN LPTSTR FindWithin, OUT size_t *NewLength);
    size_t        multi_sz_length(__in IN LPTSTR MultiSz);
    BOOLEAN       prepend_sz_to_multi_sz(__in LPTSTR SzToPrepend, __deref_inout LPTSTR *MultiSz);

    std::map<std::wstring, HKEY, macho::stringutils::no_case_string_less>  _reg_keys_map;
    std::map<std::wstring, DWORD, macho::stringutils::no_case_string_less> _reg_types_map;
    macho::windows::reg_edit_base &_reg;
};

#endif