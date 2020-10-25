#pragma once
#ifndef __temp_drive_letter_H__
#define __temp_drive_letter_H__

#include "macho.h"

class temp_drive_letter{
public:
    typedef boost::shared_ptr<temp_drive_letter> ptr;
    static temp_drive_letter::ptr assign(std::wstring device_name, bool chkdsk = true );
    static temp_drive_letter::ptr assign(int disk_number, int partition_number, bool chkdsk = true){
        return assign(boost::str(boost::wformat(L"\\Device\\Harddisk%d\\Partition%d") % disk_number% partition_number), chkdsk);
    }
    std::wstring drive_letter() const { return _drive_letter; }
    virtual ~temp_drive_letter(){ remove(); }
    static std::wstring find_next_available_drive();
    
    static bool flush_and_dismount_fs(std::wstring volume_path);
    static bool is_drive_letter(std::wstring volume_path);
    static bool is_drive_letter(std::string volume_path);
private:
    bool remove();
    static UINT find_next_available_drive(DWORD drives);
    static DWORD get_net_used_drives();
    temp_drive_letter(std::wstring drive_letter) :_drive_letter(drive_letter){}
    std::wstring _drive_letter;
};

#endif