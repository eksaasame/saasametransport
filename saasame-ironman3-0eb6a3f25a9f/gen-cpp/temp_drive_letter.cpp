#include "temp_drive_letter.h"
#ifndef _WINIOCTL
#define _WINIOCTL
#include <winioctl.h>
#endif

using namespace macho::windows;
using namespace macho;

DWORD temp_drive_letter::get_net_used_drives(){
    DWORD ret = 0;
    NET_API_STATUS status;
    USE_INFO_0 *pInfo;
    DWORD i, EntriesRead, TotalEntries;
    status = NetUseEnum(
        NULL,   //UncServerName
        0,      //Level
        (LPBYTE*)&pInfo,
        MAX_PREFERRED_LENGTH, //PreferedMaximumSize
        &EntriesRead,
        &TotalEntries,
        NULL    //ResumeHandle
        );

    if (status == NERR_Success){
        for (i = 0; i < EntriesRead; i++){
            DWORD mask = 1 << (pInfo[i].ui0_local[0] - (TCHAR)'A');
            ret |= mask;
        }

        NetApiBufferFree(pInfo);
    }
    return ret;
}

std::wstring temp_drive_letter::find_next_available_drive(){
    TCHAR drive[4];
    UINT	nIndex = 0;
    macho::windows::mutex m(L"find_next_available_drive");
    macho::windows::auto_lock lock(m);
    DWORD   drives = GetLogicalDrives() | get_net_used_drives();
    if ((nIndex = find_next_available_drive(drives)) != 0){
        drive[0] = (TCHAR)'A' + (TCHAR)nIndex;
        drive[1] = (TCHAR)':';
        drive[2] = (TCHAR)'\\';
        drive[3] = (TCHAR)'\0';
        return drive;
    }
    return _T("");
}

UINT temp_drive_letter::find_next_available_drive(DWORD drives){
    UINT i;
    DWORD mask = 4; //from C:

    for (i = 2; i < 26; i++){
        if (!(drives & mask)){
            return i;
        }
        mask <<= 1;
    }
    return 0;
}

temp_drive_letter::ptr temp_drive_letter::assign(std::wstring device_name, bool chkdsk){

    temp_drive_letter::ptr p;
    TCHAR szDevice[MAX_PATH];
    TCHAR szDriveLetter[3];
    BOOL  fResult;
	macho::windows::mutex m(L"mount");
	macho::windows::auto_lock lock(m);
    std::wstring drive = find_next_available_drive();

    if (drive.length()){
        /*
        Make sure the drive letter isn't already in use.  If not in use,
        create the symbolic link to establish the temporary drive letter.

        pszDriveLetter could be in the format X: or X:\; QueryDosDevice and
        DefineDosDevice need X:
        */
        szDriveLetter[0] = drive[0];
        szDriveLetter[1] = _T(':');
        szDriveLetter[2] = _T('\0');

        if (!QueryDosDevice(szDriveLetter, szDevice, MAX_PATH)){
            /*
            If we can create the symbolic link, verify that it points to a real
            device.  If not, remove the link and return an error.  CreateFile sets
            the last error code to ERROR_FILE_NOT_FOUND.
            */

            fResult = DefineDosDevice(DDD_RAW_TARGET_PATH, szDriveLetter, device_name.c_str());

            if (fResult){
                HANDLE hDevice;
                TCHAR  szDriveName[7]; // holds \\.\X: plus NULL.

                _tcsncpy_s(szDriveName, _countof(szDriveName), _T("\\\\.\\"), _TRUNCATE);
                _tcsncat_s(szDriveName, _countof(szDriveName), szDriveLetter, _TRUNCATE);

                hDevice = CreateFile(szDriveName, GENERIC_READ,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL, OPEN_EXISTING, 0, NULL);

                if (INVALID_HANDLE_VALUE != hDevice){
                    CloseHandle(hDevice);
                    p = temp_drive_letter::ptr(new temp_drive_letter(drive));
                    if (chkdsk){
                        std::wstring result;
                        std::wstring cmd = boost::str(boost::wformat(L"%1%\\chkdsk.exe /f %2%:") % macho::windows::environment::get_system_directory() % drive[0]);
                        process::exec_console_application_with_timeout(cmd, result, -1, true);
                        LOG(LOG_LEVEL_RECORD, _T("Run Command '%s' - result: \n%s"), cmd.c_str(), result.c_str());
                    }
                }
                else{
                    DefineDosDevice(DDD_RAW_TARGET_PATH | DDD_REMOVE_DEFINITION |
                        DDD_EXACT_MATCH_ON_REMOVE, szDriveLetter,
                        device_name.c_str());
                }
            }
        }
    }
    return p;
}

bool temp_drive_letter::remove(){
    BOOL fResult;
    TCHAR szDriveLetter[3];
    TCHAR szDeviceName[MAX_PATH];

    temp_drive_letter::flush_and_dismount_fs(_drive_letter);
    /*
    pszDriveLetter could be in the format X: or X:\; DefineDosDevice
    needs X:
    */
    szDriveLetter[0] = _drive_letter[0];
    szDriveLetter[1] = _T(':');
    szDriveLetter[2] = _T('\0');

    fResult = QueryDosDevice(szDriveLetter, szDeviceName, MAX_PATH);
    if (fResult){
        fResult = DefineDosDevice(DDD_RAW_TARGET_PATH | DDD_REMOVE_DEFINITION |
            DDD_EXACT_MATCH_ON_REMOVE, szDriveLetter,
            szDeviceName);
    }
    return fResult ? true : false;
}

bool  temp_drive_letter::is_drive_letter(std::wstring volume_path){
    /*
    format must be: X:<null> or X:\<NULL> where X is a letter.
    */
    if ((1 < volume_path.length()) &&
        (IsCharAlphaW(volume_path[0])) &&
        (L':' == volume_path[1]) &&
        ((L'\0' == volume_path[2]) ||
        ((2 < volume_path.length()) && (L'\\' == volume_path[2]) &&
        ((4 > volume_path.length()) || (L'\0' == volume_path[3]))))){
        return true;
    }
    else{
        return false;
    }
}

bool  temp_drive_letter::is_drive_letter(std::string volume_path){
    /*
    format must be: X:<null> or X:\<NULL> where X is a letter.
    */
    if ((1 < volume_path.length()) &&
        (IsCharAlphaA(volume_path[0])) &&
        (':' == volume_path[1]) &&
        (('\0' == volume_path[2]) ||
        ((2 < volume_path.length()) && ('\\' == volume_path[2]) &&
        ((4 > volume_path.length()) || ('\0' == volume_path[3]))))){
        return true;
    }
    else{
        return false;
    }
}

bool  temp_drive_letter::flush_and_dismount_fs(std::wstring volume_path){

    BOOL            bRet = FALSE;
    std::wstring    szVolumeName;
    DWORD           cbReturned;

    if (volume_path.length()){
        if (is_drive_letter(volume_path))
            szVolumeName = boost::str(boost::wformat(L"\\\\.\\%c:") % volume_path[0]);
        else{
            if (volume_path[0] == TEXT('\\') ||
                volume_path[1] == TEXT('\\') ||
                volume_path[2] == TEXT('?') ||
                volume_path[3] == TEXT('\\')){
                szVolumeName = volume_path;
                if (szVolumeName[szVolumeName.length() - 1] == TEXT('\\'))
                    szVolumeName.erase(szVolumeName.length()-1);
            }
            else{
                std::wstring szVolumeMountPoint = volume_path;
                TCHAR     szGuid[MAX_PATH] = _T("");
                if (szVolumeMountPoint[szVolumeMountPoint.length() - 1] != TEXT('\\'))
                    szVolumeMountPoint.append(_T("\\"));

                if (GetVolumeNameForVolumeMountPoint(szVolumeMountPoint.c_str(), szGuid, MAX_PATH)){
                    szGuid[_tcsclen(szGuid) - 1] = _T('\0');
                    szVolumeName = szGuid;
                }
            }
        }
    }

    if (szVolumeName.length()){
        macho::windows::auto_file_handle handle = CreateFile(
            szVolumeName.c_str(),
            GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS,
            NULL
            );

        if (handle.is_invalid()){
            LOG(LOG_LEVEL_ERROR, _T("Failed to open volume (%s) handle : error (%d)."), volume_path.c_str(), GetLastError());
            return false;
        }

        if (bRet = DeviceIoControl(handle, FSCTL_IS_VOLUME_MOUNTED, NULL, 0, NULL, 0, &cbReturned, NULL)){
            bRet = FlushFileBuffers(handle);
            if (!bRet){
                LOG(LOG_LEVEL_ERROR, _T("Flush Volume(%s) file buffers failed. error: (%d)."), volume_path.c_str(), GetLastError());
            }
            Sleep(1000 * 1);
            if (bRet = DeviceIoControl(handle, FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0, &cbReturned, NULL)){
                bRet = DeviceIoControl(handle, FSCTL_DISMOUNT_VOLUME, NULL, 0, NULL, 0, &cbReturned, NULL);
                DeviceIoControl(handle, FSCTL_UNLOCK_VOLUME, NULL, 0, NULL, 0, &cbReturned, NULL);
            }
            else{
                LOG(LOG_LEVEL_ERROR, _T("Lock volume(%s) failed. error: (%d)."), volume_path.c_str(), GetLastError());
                bRet = DeviceIoControl(handle, FSCTL_DISMOUNT_VOLUME, NULL, 0, NULL, 0, &cbReturned, NULL);
            }

            if (!bRet){
                LOG(LOG_LEVEL_ERROR, _T("Dismount volume(%s) failed. error: (%d)."), volume_path.c_str(), GetLastError());
            }
            else{
                LOG(LOG_LEVEL_INFO, _T("Volume(%s) filesystem dismounted."), volume_path.c_str());
            }
        }
        else{
            LOG(LOG_LEVEL_INFO, _T("Volume(%s) filesystem already dismounted."), volume_path.c_str());
        }
    }
    return bRet ? true : false;
}
