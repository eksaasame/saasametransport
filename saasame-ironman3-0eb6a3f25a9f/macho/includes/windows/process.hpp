// service.hpp ----------------------------------------------//
// -----------------------------------------------------------------------------

// Copyright 2013-2014 Albert Wei

// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt

// -----------------------------------------------------------------------------

// Revision History
// 04-29-2014 - Initial Release

// -----------------------------------------------------------------------------

#pragma once

#ifndef __MACHO_WINDOWS_PROCESS__
#define __MACHO_WINDOWS_PROCESS__

#include "..\config\config.hpp"
#include "..\common\tracelog.hpp"
#include "..\common\exception_base.hpp"
#include "boost\filesystem.hpp"
namespace macho{
namespace windows{

class process{
public:
    typedef boost::shared_ptr<process> ptr;
    typedef std::vector<ptr> vtr;
	static process::vtr     enumerate(stdstring name = _T(""));
	static process::ptr		find(boost::filesystem::path p);
    static bool             is_running(boost::filesystem::path p);
    static bool				exec_console_application_with_timeout(stdstring command, stdstring &ret, ULONGLONG timeout_seconds = INFINITE, bool is_hidden = true);
    static bool             exec_console_application_without_wait(std::wstring command, bool is_hidden = true);
    static bool             exec_console_application_with_retry(std::wstring command, std::wstring &ret = std::wstring(), ULONGLONG timeout_seconds = 60 * 3, int retry_count = 5);
    stdstring               name() const { return _name; }
    stdstring               options() const { return _command_options; }
    boost::filesystem::path path() const { return _path; }
    bool                    terminate();
private:
    static process::ptr get_process_info(DWORD pid);
    process(DWORD pid) : _pid(pid){}
    static stdstring get_base_name(HANDLE hProcess);
	static stdstring get_file_path(HANDLE  hProcess);
    DWORD     _pid;
    stdstring                _name;
    boost::filesystem::path  _path;
    stdstring                _command_options;
};

#ifndef MACHO_HEADER_ONLY

#include <Windows.h>
#include <Winternl.h>
#include <Psapi.h>
#pragma comment(lib, "Psapi.LIB")

typedef NTSTATUS(WINAPI*_NtQueryInformationProcess)(
    HANDLE ProcessHandle,
    DWORD ProcessInformationClass,
    PVOID ProcessInformation,
    DWORD ProcessInformationLength,
    PDWORD ReturnLength
    );

PVOID GetPebAddress(HANDLE ProcessHandle)
{
    _NtQueryInformationProcess NtQueryInformationProcess =
        (_NtQueryInformationProcess)GetProcAddress(
        GetModuleHandleA("ntdll.dll"), "NtQueryInformationProcess");
    PROCESS_BASIC_INFORMATION pbi;
    NtQueryInformationProcess(ProcessHandle, 0, &pbi, sizeof(pbi), NULL);
    return pbi.PebBaseAddress;
}

stdstring process::get_base_name(HANDLE  hProcess){
	HMODULE hModule;
	DWORD cbNeeded;
    TCHAR szProcessName[MAX_PATH] = TEXT("<unknown>");
	if (EnumProcessModules(hProcess, &hModule, sizeof(hModule),
		&cbNeeded)){
		if (GetModuleBaseName(hProcess, hModule, szProcessName,
			sizeof(szProcessName) / sizeof(TCHAR))){
		}
	}
    return szProcessName;
}

stdstring process::get_file_path(HANDLE  hProcess){
	std::auto_ptr<TCHAR> p_path;
	DWORD                size = MAX_PATH;
	do{
		p_path = std::auto_ptr<TCHAR>(new TCHAR[size]);
		memset(p_path.get(), 0, size * sizeof(TCHAR));
		size = GetModuleFileNameEx(hProcess, NULL, p_path.get(), size);
	} while (ERROR_INSUFFICIENT_BUFFER == GetLastError());

	if (size > 0)
		return p_path.get();
	return _T("");
}

process::ptr process::get_process_info(DWORD pid){
    HANDLE hProcess = NULL;
    PVOID pebAddress;
    PVOID rtlUserProcParamsAddress;
    UNICODE_STRING commandLine;
    WCHAR *commandLineContents;
    BOOL bResult = FALSE;
    process::ptr p = process::ptr(new process(pid));
    if ((hProcess = OpenProcess(
        PROCESS_QUERY_INFORMATION | /* required for NtQueryInformationProcess */
        PROCESS_VM_READ, /* required for ReadProcessMemory */
        FALSE, pid)) == 0){
        LOG(LOG_LEVEL_INFO, _T("Could not open process ( id = %d ) (0x%08X)"), pid, GetLastError());
    }
    else{
		p->_path = get_file_path(hProcess);
        p->_name = get_base_name(hProcess);
        pebAddress = GetPebAddress(hProcess);
        /* get the address of ProcessParameters */
        if (!ReadProcessMemory(hProcess, (PCHAR)pebAddress + 0x10,
            &rtlUserProcParamsAddress, sizeof(PVOID), NULL)){
            LOG(LOG_LEVEL_INFO, _T("Could not read the address of ProcessParameters (0x%08X)"), GetLastError());
        }
        else  if (!ReadProcessMemory(hProcess, (PCHAR)rtlUserProcParamsAddress + 0x40,
            &commandLine, sizeof(commandLine), NULL)){
            /* read the CommandLine UNICODE_STRING structure */
            LOG(LOG_LEVEL_INFO, _T("Could not read the command line (0x%08X)"), GetLastError());
        }
        else if (commandLine.Length) {
            /* allocate memory to hold the command line */
            commandLineContents = (WCHAR *)malloc(commandLine.Length);
            /* read the command line */
            if (commandLineContents){
                memset(commandLineContents, 0, commandLine.Length);
                if (!ReadProcessMemory(hProcess, commandLine.Buffer,
                    commandLineContents, commandLine.Length, NULL)){
                    LOG(LOG_LEVEL_INFO, _T("Could not read the command line string (0x%08X)"), GetLastError());
                }
                else{
                    p->_command_options = commandLineContents;
                }
                free(commandLineContents);
            }
        }
        CloseHandle(hProcess);
    }
    return p;
}

process::vtr process::enumerate(stdstring name){
    process::vtr processes;
    DWORD aProcesses[1024], cbNeeded, cProcesses;
    memset(&aProcesses, 0, sizeof(aProcesses));
    if (EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded)){
        cProcesses = cbNeeded / sizeof(DWORD);
        for (int i = 0; i < cProcesses; i++){
            if (aProcesses[i] != 0){
                process::ptr _p = get_process_info(aProcesses[i]);
				if (_p){
					if (name.empty()){
						processes.push_back(_p);
					}
					else if (!_p->_name.empty() && 0 ==_tcsicmp(name.c_str(), _p->_name.c_str())){
						processes.push_back(_p);
					}
				}
            }// if aProcesses[i]
        }// End for
    }// End EnumProcess
    return processes;
}

bool process::is_running(boost::filesystem::path p){
	if (!p.empty()){
		process::vtr processes;
		DWORD aProcesses[1024], cbNeeded, cProcesses;
		memset(&aProcesses, 0, sizeof(aProcesses));
		if (EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded)){
			cProcesses = cbNeeded / sizeof(DWORD);
			for (int i = 0; i < cProcesses; i++){
				if (aProcesses[i] != 0){
					process::ptr _p = get_process_info(aProcesses[i]);
					if (!_p->_path.empty() && _p->_path == p)
						return true;
				}// if aProcesses[i]
			}// End for
		}// End EnumProcess
	}
    return false;
}

bool process::terminate(){
    HANDLE hProcess = NULL;
    bool result = false;
    if ((hProcess = OpenProcess(
		PROCESS_TERMINATE , /* Required to terminate a process */
        FALSE, _pid)) == 0){
        LOG(LOG_LEVEL_ERROR, _T("Could not open process ( id = %d ) (0x%08X)"), _pid, GetLastError());
    }
    else{
        if (!(result = TRUE == TerminateProcess(hProcess, 1))){
            LOG(LOG_LEVEL_ERROR, _T("Could not terminate process ( id = %d ) (0x%08X)"), _pid, GetLastError());
        }
        CloseHandle(hProcess);
    }
    return result;
}

process::ptr process::find(boost::filesystem::path p){
	if (!p.empty()){
		process::vtr processes;
		DWORD aProcesses[1024], cbNeeded, cProcesses;
		memset(&aProcesses, 0, sizeof(aProcesses));
		if (EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded)){
			cProcesses = cbNeeded / sizeof(DWORD);
			for (int i = 0; i < cProcesses; i++){
				if (aProcesses[i] != 0){
					process::ptr _p = get_process_info(aProcesses[i]);
					if (!_p->_path.empty() && _p->_path == p)
						return _p;
				}// if aProcesses[i]
			}// End for
		}// End EnumProcess
	}
	return NULL;
}

bool process::exec_console_application_with_timeout(stdstring command, stdstring &ret, ULONGLONG timeout_seconds, bool is_hidden)
{
    HANDLE              hStdInRead, hStdInWrite, hStdInWriteTemp;
    HANDLE              hStdOutRead, hStdOutWrite, hStdOutReadTemp;
    SECURITY_ATTRIBUTES sa;
    STARTUPINFO         si;
    PROCESS_INFORMATION pi;
    HRESULT             hr = S_OK;

    if (!command.length()){
        SetLastError(ERROR_INVALID_PARAMETER);
        return false;
    }
    ret.clear();
	ZeroMemory(&sa, sizeof(SECURITY_ATTRIBUTES));
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle = TRUE;

	if (CreatePipe(&hStdOutRead, &hStdOutWrite, &sa, 0)){
		if (DuplicateHandle(GetCurrentProcess(), hStdOutRead, GetCurrentProcess(), &hStdOutReadTemp, 0, FALSE, DUPLICATE_SAME_ACCESS)){
			if (CreatePipe(&hStdInRead, &hStdInWrite, &sa, 0)){
				if (DuplicateHandle(GetCurrentProcess(), hStdInWrite, GetCurrentProcess(), &hStdInWriteTemp, 0, FALSE, DUPLICATE_SAME_ACCESS)){
					ZeroMemory(&si, sizeof(STARTUPINFO));
					si.cb = sizeof(STARTUPINFO);
					si.dwFlags = STARTF_USESTDHANDLES;
					si.hStdOutput = hStdOutWrite;
					si.hStdInput = hStdInRead;
					si.hStdError = GetStdHandle(STD_ERROR_HANDLE);

					ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

					LOG(LOG_LEVEL_INFO, TEXT("exec_console_application_with_timeout(%s) successfully."), command.c_str());
					if (CreateProcess(NULL,               /*ApplicationName*/
						(LPTSTR)command.c_str(),          /*CommandLine*/
						NULL,               /*ProcessAttributes*/
						NULL,               /*ThreadAttributes*/
						TRUE,               /*InheritHandles*/
						is_hidden ? CREATE_NO_WINDOW : 0,    /*CreationFlags*/
						NULL,               /*Environment*/
						NULL,               /*CurrentDirectory*/
						&si,                /*StartupInfo*/
						&pi))             /*ProcessInformation*/
					{
#define BUFFSIZE 512  
						CHAR    szBuffer[BUFFSIZE + 1];
						DWORD   procRetCode = 0;
						unsigned long exit = 0;  //process exit code
						unsigned long bread;   //bytes read
						unsigned long avail;   //bytes available
						SYSTEMTIME localTimeNow;
						FILETIME fileTimeCreate;
						memset(&localTimeNow, 0, sizeof(SYSTEMTIME));
						memset(&fileTimeCreate, 0, sizeof(FILETIME));
						GetSystemTime(&localTimeNow);
						SystemTimeToFileTime(&localTimeNow, &fileTimeCreate);
						bool terminated = false;
						while (TRUE) {
							PeekNamedPipe(hStdOutReadTemp, szBuffer, BUFFSIZE, &bread, &avail, NULL);
							if (bread == 0){
								FILETIME    ftCreate, ftExit, ftKernel, ftUser;
								memset(&ftExit, 0, sizeof(FILETIME));
								memset(&ftCreate, 0, sizeof(FILETIME));
								memset(&ftKernel, 0, sizeof(FILETIME));
								memset(&ftUser, 0, sizeof(FILETIME));
								GetExitCodeProcess(pi.hProcess, &exit);      //while the process is running
								if (exit != STILL_ACTIVE)
									break;
								else if (!GetProcessTimes(pi.hProcess, &ftCreate, &ftExit, &ftKernel, &ftUser))
									break;
								else if ((ftExit.dwHighDateTime != 0) || (ftExit.dwLowDateTime != 0))
									break;
								else{
                                    if (timeout_seconds != 0){
										FILETIME fileTimeNow;
										memset(&localTimeNow, 0, sizeof(SYSTEMTIME));
										memset(&fileTimeNow, 0, sizeof(FILETIME));
										GetSystemTime(&localTimeNow);
										if (SystemTimeToFileTime(&localTimeNow, &fileTimeNow)){
											ULARGE_INTEGER ulCreate, ulNow;
											ulCreate.HighPart = fileTimeCreate.dwHighDateTime;
											ulCreate.LowPart = fileTimeCreate.dwLowDateTime;
											ulNow.HighPart = fileTimeNow.dwHighDateTime;
											ulNow.LowPart = fileTimeNow.dwLowDateTime;
											ULONGLONG runningTime = (ulNow.QuadPart - ulCreate.QuadPart) / 10000000U;
                                            if (runningTime >= timeout_seconds){
                                                LOG(LOG_LEVEL_ERROR, TEXT("Terminate command process (%s) when it is running (%I64u) seconds which is over (%I64u) seconds."), command.c_str(), runningTime, timeout_seconds);
												TerminateProcess(pi.hProcess, ERROR_TIMEOUT);
												Sleep(15000);
												continue;
											}
											LOG(LOG_LEVEL_INFO, TEXT("The Command (%s) is still running. (%I64u seconds)"), command.c_str(), runningTime);
										}
										else{
											LOG(LOG_LEVEL_INFO, TEXT("The Command (%s) is still running."), command.c_str());
										}
									}
									else{
										LOG(LOG_LEVEL_INFO, TEXT("The Command (%s) is still running."), command.c_str());
									}
									Sleep(1000);
								}
							}
							else {
								ZeroMemory(szBuffer, sizeof(CHAR) * (BUFFSIZE + 1));
								if (avail > BUFFSIZE){
									while (bread >= BUFFSIZE){
										ReadFile(hStdOutReadTemp, szBuffer, BUFFSIZE, &bread, NULL);
#if _UNICODE
										ret.append(stringutils::convert_ansi_to_unicode(szBuffer));
#else
										ret.append(szBuffer);
#endif
										ZeroMemory(szBuffer, sizeof(CHAR) * (BUFFSIZE + 1));
									}
								}
								else {
									ReadFile(hStdOutReadTemp, szBuffer, BUFFSIZE, &bread, NULL);
#if _UNICODE
									ret.append(stringutils::convert_ansi_to_unicode(szBuffer));
#else
									ret.append(szBuffer);
#endif                             
								}
							}
							if (terminated)
								break;
							GetExitCodeProcess(pi.hProcess, &exit);      //while the process is running
							if (exit != STILL_ACTIVE)
								terminated = true;
						}

						if (GetExitCodeProcess(pi.hProcess, &procRetCode))
							hr = procRetCode;
						else
							hr = GetLastError();

						CloseHandle(pi.hProcess);
						CloseHandle(pi.hThread);
					}
					else
						hr = GetLastError();

					CloseHandle(hStdInWriteTemp);
				}
				else
					hr = GetLastError();

				CloseHandle(hStdInRead);
				CloseHandle(hStdInWrite);
			}
			else
				hr = GetLastError();

			CloseHandle(hStdOutReadTemp);
		}
		else
			hr = GetLastError();

		CloseHandle(hStdOutRead);
		CloseHandle(hStdOutWrite);
	}
	else{
		hr = GetLastError();
	}
	SetLastError(hr);
	return HRESULT_FROM_WIN32(hr) == 0;
}

bool  process::exec_console_application_without_wait(std::wstring command, bool is_hidden)
{
    STARTUPINFO				si;
    PROCESS_INFORMATION		pi;
    DWORD                   dwWait = 0;
    DWORD					procRetCode = 0;
    BOOL                    result = FALSE;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // Start the child process. 
    if (result = CreateProcess(NULL,            // No module name (use command line). 
        (LPTSTR)command.c_str(),        // Command line. 
        NULL,                           // Process handle not inheritable. 
        NULL,                           // Thread handle not inheritable. 
        FALSE,                          // Set handle inheritance to FALSE. 
        is_hidden ? CREATE_NO_WINDOW : 0,    /*CreationFlags*/
        NULL,                           // Use parent's environment block. 
        NULL,							// Use parent's starting directory. 
        &si,                            // Pointer to STARTUPINFO structure.
        &pi)                           // Pointer to PROCESS_INFORMATION structure.
        ){
        // Close process and thread handles. 
        if (pi.hProcess)
            CloseHandle(pi.hProcess);
        if (pi.hThread)
            CloseHandle(pi.hThread);
    }
    return result == TRUE;
}

bool process::exec_console_application_with_retry(std::wstring command, std::wstring &ret, ULONGLONG timeout_seconds, int retry_count)
{
    while (true)
    {
        std::wcout << L"" << command << std::endl;
        LOG(LOG_LEVEL_RECORD, TEXT("%s"), command.c_str());
        if (macho::windows::process::exec_console_application_with_timeout(command, ret, timeout_seconds)){
            std::wcout << L"Return : " << ret << std::endl;
            LOG(LOG_LEVEL_RECORD, TEXT("Return : %s"), ret.c_str());
            return true;
        }
        else if (GetLastError() != ERROR_TIMEOUT){
            std::wcout << L"Return : " << ret << std::endl;
            LOG(LOG_LEVEL_ERROR, TEXT("Return : %s"), ret.c_str());
            break;
        }
        if (retry_count == 0){
            std::wcout << L"(retry count == 0) : End of retry." << std::endl;
            LOG(LOG_LEVEL_ERROR, TEXT("(retry count == 0) : End of retry."));
            break;
        }
        Sleep(1000);
        LOG(LOG_LEVEL_RECORD, TEXT("Launch the command (%s) again for retry."), command.c_str());
        retry_count--;
    };
    return false;
}
#endif

};//namespace windows
};//namespace macho
#endif