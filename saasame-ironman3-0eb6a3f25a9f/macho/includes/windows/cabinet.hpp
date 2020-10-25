// cabinet.hpp ----------------------------------------------//
// -----------------------------------------------------------------------------

// Copyright 2013-2014 Albert Wei

// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt

// -----------------------------------------------------------------------------

// Revision History
// 04-29-2014 - Initial Release

// -----------------------------------------------------------------------------

#pragma once

#ifndef __MACHO_WINDOWS_CABINET__
#define __MACHO_WINDOWS_CABINET__

#include <FCI.h>
#include <FDI.h>
#include "boost\shared_ptr.hpp"
#include "boost\filesystem.hpp"
#include "..\config\config.hpp"

#pragma comment(lib, "cabinet.lib")

namespace macho{

namespace windows{

class cabinet{
public:
class compress{
public:
    typedef boost::shared_ptr<compress> ptr;
    virtual ~compress(){ 
        close(); 
    }
    static compress::ptr create(boost::filesystem::path cabfile){
        compress::ptr cab = compress::ptr(new compress());
        if (cab->_create(cabfile))
            return cab;
        return NULL;
    }
    bool add( boost::filesystem::path path, boost::filesystem::path name_in_cab = "" );
private:
    bool close();
    bool _create(boost::filesystem::path cabfile);
    compress();

    typedef struct _CURSTATUS{
        ULONG u32_TotCompressedSize;
        ULONG u32_TotUncompressedSize;
        ULONG cb1;
        ULONG cb2;
        ULONG FolderPercent;
        char  szCabName[CB_MAX_CABINET_NAME];
    }CURSTATUS,*PCURSTATUS;
    static void* fci_alloc(UINT size);
    static void  fci_free(void* memblock);
    static int   fci_open(char* pszFile, int oflag, int pmode, int *err, void *pv);
    static UINT  fci_read(int fd, void *memory, UINT count, int *err, void *pv);
    static UINT  fci_write(int fd, void *memory, UINT count, int *err, void *pv);
    static int   fci_close(int fd, int *err, void *pv);
    static long  fci_seek(int fd, long offset, int seektype, int *err, void *pv);
    static int   fci_delete(char *pszFile, int *err, void *pv);
    static BOOL  fci_get_temp_file(char *pszTempName, int cbTempName, void *pv);
    static int   fci_get_attribs_and_date(char *pszName, USHORT *pdate, USHORT *ptime, USHORT *pattribs, int *err, void *pv);
    static int   fci_file_placed(PCCAB pccab, char *sz_File, int cbFile, BOOL fContinuation, void *pv);
    static BOOL  fci_get_next_cabinet(PCCAB pccab, ULONG cbPrevCab, void* pv);
    static long  fci_update_status(UINT typeStatus, ULONG cb1, ULONG cb2, void *pv);

    HFCI                 _hFci;   
    ERF                  _erf;   
    CCAB                 _params;  
    CURSTATUS            _status;
};

class uncompress{
public:

    static bool extract_all_files(boost::filesystem::path cab, boost::filesystem::path target);
    static bool extract_file(boost::filesystem::path cab, boost::filesystem::path file, boost::filesystem::path target);
    static bool is_cabinet(boost::filesystem::path cabfile);

    virtual ~uncompress(){
    }

private:
    uncompress();
    static void* fdi_alloc(UINT size);
    static void  fdi_free(void* memblock);
    static int   fdi_open(char* pszFile, int oflag, int pmode);
    static UINT  fdi_read(int fd, void *memory, UINT count);
    static UINT  fdi_write(int fd, void *memory, UINT count);
    static int   fdi_close(int fd);
    static long  fdi_seek(int fd, long offset, int seektype);
    static void  fdi_set_attribs_and_date(char* szFile, USHORT uDate, USHORT uTime, USHORT uAttribs);
    static int   fdi_callback(FDINOTIFICATIONTYPE fdint, PFDINOTIFICATION pfdin);
    static std::wstring fdi_error_to_string(FDIERROR err);
};

private:
    static bool  safe_copy_path(char* buffer, UINT buf_size, char* path, bool is_append);
    static bool  safe_format_path(char* buffer, UINT buf_size, char* format, ...);

};

#ifndef MACHO_HEADER_ONLY
#include <windows.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>
#include <stdio.h>
#include <errno.h>
#include "..\windows\environment.hpp"
#include "..\common\stringutils.hpp"
using namespace macho;
using namespace macho::windows;

// Stores in the file attributes that the file was compressed using UTC time
#define   FILE_ATTR_UTC_TIME     0x100
#define   MEDIA_SIZE             655360000   
#define   FOLDER_THRESHOLD       900000  

typedef struct _CAB_EXTRACT_PARAMETER
{
    char sz_target_dir[CB_MAX_CAB_PATH];
    char sz_extract_fileName[CB_MAX_CAB_PATH];
    char sz_last_cab_name[CB_MAX_CAB_PATH];
    BOOL is_extract_file_existing;
} CAB_EXTRACT_PARAMETER, *PCAB_EXTRACT_PARAMETER;

bool cabinet::safe_copy_path(char* buffer, UINT buf_size, char* path, bool is_append ){
    if (!is_append) buffer[0] = 0;
    if (strlen(buffer) + strlen(path) >= buf_size -1) // + backslash and zero
        return false;
    strcat_s(buffer, buf_size, path);
    return true;        
}

bool cabinet::safe_format_path(char* buffer, UINT buf_size, char* format, ...){
    bool     status    =   true;
    va_list  args;
    va_start(args, format);
    if (_vsnprintf_s(buffer, buf_size, buf_size, format, args) < 0){
        buffer[0] = 0;
        status = false;
    }
    va_end( args );
    return status;                
}

void* cabinet::compress::fci_alloc(UINT size){ 
   return malloc(size); 
}

void cabinet::compress::fci_free(void* memblock){ 
   free(memblock);  
}

int cabinet::compress::fci_open(char* pszFile, int oflag, int pmode, int *err, void *pv){ 
    int fd;
#if _DEBUG
    SYSTEMTIME SystemTime;
    GetLocalTime(&SystemTime);
    OutputDebugStringA( boost::str(boost::format("%d-%d-%d %d:%d:%d.%d : fci_open : %s \r\n") 
                        %SystemTime.wYear
                        %SystemTime.wMonth
                        %SystemTime.wDay 
                        %SystemTime.wHour
                        %SystemTime.wMinute 
                        %SystemTime.wSecond
                        %SystemTime.wMilliseconds
                        %pszFile).c_str() );
#endif
    errno_t err_t = _sopen_s( &fd, pszFile, oflag, _SH_DENYRW, pmode );
    if(fd == -1) *err = err_t;
    return fd; 
}

UINT cabinet::compress::fci_read(int fd, void *memory, UINT count, int *err, void *pv){ 
    int s32_Read = _read(fd, memory, count); 
    if  (s32_Read != count) *err = errno;
    return s32_Read; 
}

UINT cabinet::compress::fci_write(int fd, void *memory, UINT count, int *err, void *pv){ 
    UINT u32_Written = (UINT) _write(fd, memory, count);
    if  (u32_Written != count) *err = errno;
    return u32_Written;
}

int cabinet::compress::fci_close(int fd, int *err, void *pv){ 
    int result = _close(fd);
    if (result != 0) *err = errno;
    return result;
}

long cabinet::compress::fci_seek(int fd, long offset, int seektype, int *err, void *pv){
    // Move the file pointer to the new position
    long Pos = _lseek(fd, offset, seektype);
    if  (Pos == -1) *err = errno;
    return Pos; 
}

int cabinet::compress::fci_delete(char *pszFile, int *err, void *pv){ 
    int result = remove(pszFile);
    if (result != 0) *err = errno;
    return result;
}

BOOL cabinet::compress::fci_get_temp_file(char *pszTempName, int cbTempName, void *pv){ 
    stdstring tempfile = environment::create_temp_file(_T("cab"));
    if ( tempfile.length() ){
        DeleteFile( tempfile.c_str() );
        safe_format_path( pszTempName, cbTempName, "%s", stringutils::convert_unicode_to_ansi(tempfile).c_str());
        return TRUE;
    }
    else{
        SYSTEMTIME SystemTime;
        GetLocalTime(&SystemTime);
        OutputDebugStringA( boost::str(boost::format("%d-%d-%d %d:%d:%d.%d : fci_get_temp_file : Faild to create temp file. \r\n") 
                            %SystemTime.wYear
                            %SystemTime.wMonth
                            %SystemTime.wDay 
                            %SystemTime.wHour
                            %SystemTime.wMinute 
                            %SystemTime.wSecond
                            %SystemTime.wMilliseconds ).c_str() );     
    }
    return FALSE;
}

int cabinet::compress::fci_get_attribs_and_date(char *pszName, USHORT *pdate, USHORT *ptime, USHORT *pattribs, int *err, void *pv){ 
    HANDLE h_File;
    BY_HANDLE_FILE_INFORMATION k_FileInfo;
    FILETIME k_CabTime;

    h_File = CreateFileA(pszName, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);

    if (h_File == INVALID_HANDLE_VALUE){
        *err = GetLastError();
        return -1;
    }

    if (!GetFileInformationByHandle(h_File, &k_FileInfo)){
        *err = GetLastError();
        CloseHandle(h_File);
        return -1;
    }

    CloseHandle(h_File);

    //// The Windows filesystem stores UTC times
    //k_CabTime = k_FileInfo.ftLastWriteTime;
    FileTimeToLocalFileTime(&k_FileInfo.ftLastWriteTime, &k_CabTime);
    FileTimeToDosDateTime(&k_CabTime, pdate, ptime);

    // Mask out all other bits except these four, since other bits are used 
    // by the cabinet format to indicate a special meaning.
    *pattribs = (int)(k_FileInfo.dwFileAttributes & (FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_ARCHIVE));

    // Now return a handle using _open()
    return fci_open(pszName, _O_RDONLY | _O_BINARY, _S_IREAD, err, pv);
}

int cabinet::compress::fci_file_placed(PCCAB pccab, char *sz_File, int cbFile, BOOL fContinuation, void *pv){ 
#if _DEBUG
    SYSTEMTIME SystemTime;
    GetLocalTime(&SystemTime);
    OutputDebugStringA( boost::str(boost::format("%d-%d-%d %d:%d:%d.%d : fci_file_placed : %s \r\n") 
                        %SystemTime.wYear
                        %SystemTime.wMonth
                        %SystemTime.wDay 
                        %SystemTime.wHour
                        %SystemTime.wMinute 
                        %SystemTime.wSecond
                        %SystemTime.wMilliseconds
                        %sz_File).c_str() );
#endif
    return 0; 
}

BOOL cabinet::compress::fci_get_next_cabinet(PCCAB pccab, ULONG cbPrevCab, void* pv){ 
    PCURSTATUS pCurStatus = (PCURSTATUS ) pv;
    if (!safe_format_path(pccab->szCab,  sizeof(pccab->szCab), "%s_%d.cab", pCurStatus->szCabName, pccab->iCab ) )
        return FALSE;
    if (!safe_format_path(pccab->szDisk, sizeof(pccab->szDisk), "Disk %d", pccab->iDisk ++))
        return FALSE;
    return TRUE;
}

long cabinet::compress::fci_update_status(UINT typeStatus, ULONG cb1, ULONG cb2, void *pv){

    PCURSTATUS pCurStatus = (PCURSTATUS ) pv;
    pCurStatus->cb1           = cb1;
    pCurStatus->cb2           = cb2;
    pCurStatus->FolderPercent = 0;

    if (typeStatus == statusFile){
        // Calculate how many bytes have been compressed totally yet
        pCurStatus->u32_TotCompressedSize   += cb1;
        pCurStatus->u32_TotUncompressedSize += cb2;
    }
    else if (typeStatus == statusFolder){
        // Calculate percentage of folder compression
        while (cb1 > 10000000){
            cb1 >>= 3;
            cb2 >>= 3;
        }

        if (cb2 != 0 && cb1<=cb2) pCurStatus->FolderPercent = ((cb1*100)/cb2);
    }
     return 0;
}

cabinet::compress::compress(): _hFci(NULL){ 
    memset(&_params,0,sizeof(CCAB));  
    memset(&_status,0,sizeof(CURSTATUS));  
}

bool cabinet::compress::_create(boost::filesystem::path cabfile){
    if ( close() && _hFci == NULL ){
        _params.cb               =   MEDIA_SIZE;   
        _params.cbFolderThresh   =   FOLDER_THRESHOLD;   
        _params.setID            =   12345;
        _params.iCab             =   0;
        _params.iDisk            =   0; 
        std::string file = cabfile.filename().string();
        std::string extension = cabfile.filename().extension().string();
        strcpy_s(_params.szCabPath, CB_MAX_CAB_PATH, (cabfile.parent_path()/"\\").string().c_str());   
        strcpy_s(_params.szCab, CB_MAX_CABINET_NAME, file.c_str());   
        strcpy_s(_status.szCabName, CB_MAX_CABINET_NAME, file.substr( 0, file.length() - extension.length() ).c_str()); 
        _hFci = FCICreate(&_erf, 
                        (PFNFCIFILEPLACED) (fci_file_placed),
                        (PFNFCIALLOC)      (fci_alloc),
                        (PFNFCIFREE)       (fci_free),
                        (PFNFCIOPEN)       (fci_open),
                        (PFNFCIREAD)       (fci_read),
                        (PFNFCIWRITE)      (fci_write),
                        (PFNFCICLOSE)      (fci_close),
                        (PFNFCISEEK)       (fci_seek),
                        (PFNFCIDELETE)     (fci_delete),
                        (PFNFCIGETTEMPFILE)(fci_get_temp_file),
                        &_params,
                        &_status);
        if ( _hFci != NULL ){
            return true;
        }
    }
    return false;
}

bool cabinet::compress::close(){
    if ( _hFci != NULL ){
        if (!FCIFlushCabinet(_hFci,FALSE,fci_get_next_cabinet,fci_update_status) )
            OutputDebugStringA( "Call FCIFlushCabinet failed.\r\n" );
        if ( FCIDestroy(_hFci) )
            _hFci = NULL;   
        else{
            OutputDebugStringA( "Call FCIDestroy failed.\r\n" );
            return false;
        }
    }
    return true;
}

bool cabinet::compress::add( boost::filesystem::path path, boost::filesystem::path name_in_cab ){
    boost::filesystem::directory_iterator end_iter;
    if ( NULL == _hFci )
        return false;
    else if ( !boost::filesystem::exists( path ) ){
        //the path is not exist;
        return false;
    }
    else if ( boost::filesystem::is_directory( path ) ){
        for( boost::filesystem::directory_iterator dir_iter( path ) ; dir_iter != end_iter ; ++dir_iter){
            if ( !add( dir_iter->path(), name_in_cab.empty() ? _T(".") : name_in_cab/path.filename() ) )
                return false;
        }
    }
    else if ( boost::filesystem::is_regular_file( path ) ){
        if ( name_in_cab.empty() )
            name_in_cab = path.filename();
        else
            name_in_cab/= path.filename();
        if (!FCIAddFile( _hFci,   
            (LPSTR)path.string().c_str(),     /*   file   to   add   */   
            (LPSTR)name_in_cab.string().c_str(),   /*   file   name   in   cabinet   file   */   
            FALSE,   
            (PFNFCIGETNEXTCABINET) fci_get_next_cabinet,
            (PFNFCISTATUS)         fci_update_status,
            (PFNFCIGETOPENINFO)    fci_get_attribs_and_date, 
            tcompTYPE_MSZIP ))
            return false;
    }
    return true;
}

cabinet::uncompress::uncompress(){

}

void* cabinet::uncompress::fdi_alloc(UINT size)
{
    return malloc(size);
}

void cabinet::uncompress::fdi_free(void* memblock)
{
    free(memblock);
}

int cabinet::uncompress::fdi_open(char* pszFile, int oflag, int pmode)
{
    SetFileAttributesA(pszFile, FILE_ATTRIBUTE_NORMAL);
    int fd;
#if _DEBUG
    SYSTEMTIME SystemTime;
    GetLocalTime(&SystemTime);
    OutputDebugStringA(boost::str(boost::format("%d-%d-%d %d:%d:%d.%d : fdi_open : %s \r\n")
        % SystemTime.wYear
        %SystemTime.wMonth
        %SystemTime.wDay
        %SystemTime.wHour
        %SystemTime.wMinute
        %SystemTime.wSecond
        %SystemTime.wMilliseconds
        %pszFile).c_str());
#endif
    errno_t err_t = _sopen_s(&fd, pszFile, oflag, _SH_DENYNO, pmode);
    return fd;
}

UINT cabinet::uncompress::fdi_read(int fd, void *memory, UINT count)
{
    int s32_Read = _read(fd, memory, count);
    return s32_Read;
}

UINT cabinet::uncompress::fdi_write(int fd, void *memory, UINT count)
{
    UINT u32_Written = (UINT)_write(fd, memory, count);
    return u32_Written;
}

int cabinet::uncompress::fdi_close(int fd)
{
    int result = _close(fd);
    return result;
}

long cabinet::uncompress::fdi_seek(int fd, long offset, int seektype)
{ 		// Move the file pointer to the new position
    long Pos = _lseek(fd, offset, seektype);
    return Pos;
}

// Sets the date and attributes for the specified file.
void cabinet::uncompress::fdi_set_attribs_and_date(char* szFile, USHORT uDate, USHORT uTime, USHORT uAttribs)
{
    HANDLE h_File;
    UINT u32_Attribs;
    FILETIME k_CabTime;
    FILETIME k_FileTime;

    h_File = CreateFileA(szFile, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

    if (h_File == INVALID_HANDLE_VALUE)
        return;

    if (DosDateTimeToFileTime(uDate, uTime, &k_CabTime))
    {
        // The Windows filesystem stores UTC times
        k_FileTime = k_CabTime;

        if ((uAttribs & FILE_ATTR_UTC_TIME) == 0)
            LocalFileTimeToFileTime(&k_CabTime, &k_FileTime); // Local time --> UTC

        SetFileTime(h_File, &k_FileTime, NULL, &k_FileTime);
    }

    CloseHandle(h_File);

    u32_Attribs = uAttribs & (FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_ARCHIVE);

    SetFileAttributesA(szFile, u32_Attribs);
}

int cabinet::uncompress::fdi_callback(FDINOTIFICATIONTYPE fdint, PFDINOTIFICATION pfdin)
{
    int nRet = 0; // Allow unsupported notifications to continue
    PCAB_EXTRACT_PARAMETER pCabExtractParam = NULL;

    char szFile[CB_MAX_CAB_PATH] = "";
    char szSubFolder[CB_MAX_CAB_PATH] = "";
    char szPath[CB_MAX_CAB_PATH] = "";
    char* szSlash = NULL;

    if (pfdin->pv)
        pCabExtractParam = (PCAB_EXTRACT_PARAMETER)pfdin->pv;
    else
        return 0;

    switch (fdint)
    {
    case fdintCABINET_INFO:

        break;

    case fdintNEXT_CABINET:
    {
        if (pfdin->fdie) // error occurred
        {
            // avoid error "User Aborted" if next cabinet file could not be found
            return -1;
        }
        // this is required to unpack all files from a spanned archive
        if (!safe_copy_path(pCabExtractParam->sz_last_cab_name, sizeof(pCabExtractParam->sz_last_cab_name), pfdin->psz1, FALSE))
            return -1;
    }
    break;

    // return  0 -> skip this file
    // return -1 -> abort extraction
    case fdintCOPY_FILE:
    {
        if (!safe_copy_path(szFile, sizeof(szFile), pfdin->psz1, FALSE)) return 0;
        if (!safe_copy_path(szSubFolder, sizeof(szSubFolder), pfdin->psz1, FALSE)) return 0;

        szSlash = strrchr(szSubFolder, '\\');
        if (szSlash)
        {
            if (!safe_copy_path(szFile, sizeof(szFile), szSlash + 1, FALSE)) return 0;
            szSlash[1] = 0;
        }
        else szSubFolder[0] = 0;

        if (strlen(pCabExtractParam->sz_extract_fileName) && _stricmp(pCabExtractParam->sz_extract_fileName, szFile))
            return 0;
        else
            pCabExtractParam->is_extract_file_existing = TRUE;

        boost::filesystem::path output = boost::filesystem::path(pCabExtractParam->sz_target_dir) / szSubFolder / szFile;
        boost::filesystem::create_directories(output.parent_path());

        // IMPORTANT:
        // If _O_TRUNC is not set, this class will create corrupt files on disk
        // if the file to be written already exists and is bigger than the one in the CAB
        nRet = fdi_open((char*)output.string().c_str(), _O_TRUNC | _O_BINARY | _O_CREAT | _O_WRONLY | _O_SEQUENTIAL, _S_IREAD | _S_IWRITE);
    }

    break;

    case fdintCLOSE_FILE_INFO:
    {
        // Close the file
        fdi_close((int)pfdin->hf);
        boost::filesystem::path fullpath = boost::filesystem::path(pCabExtractParam->sz_target_dir) / pfdin->psz1;
        fdi_set_attribs_and_date((char*)fullpath.string().c_str(), pfdin->date, pfdin->time, pfdin->attribs);

        nRet = TRUE;

    }
    break;
    }
    return nRet;
}

bool cabinet::uncompress::extract_all_files(boost::filesystem::path cab, boost::filesystem::path target){
    return extract_file(cab, L"", target);
}

bool cabinet::uncompress::extract_file(boost::filesystem::path cab, boost::filesystem::path file, boost::filesystem::path target){
    ERF                         _erf;
    HFDI                        _hFdi;
    int                         _handle = 0;
    BOOL                        _result = FALSE;
    if (boost::filesystem::exists(cab) && boost::filesystem::exists(target)){
        CAB_EXTRACT_PARAMETER ExtractParam;
        char szFile[CB_MAX_CAB_PATH] = "";
        char szPath[CB_MAX_CAB_PATH] = "";
        memset(&ExtractParam, 0, sizeof(CAB_EXTRACT_PARAMETER));
        if (!safe_copy_path(ExtractParam.sz_target_dir, sizeof(ExtractParam.sz_target_dir), (char*)target.string().c_str(), FALSE)){
            LOG(LOG_LEVEL_ERROR, L"Can't copy target dir (%s)", target.wstring().c_str());
            return false;
        }
        if (!safe_copy_path(szFile, sizeof(szFile), (char*)cab.filename().string().c_str(), FALSE)){
            LOG(LOG_LEVEL_ERROR, L"Can't copy cab file name (%s)", cab.filename().wstring().c_str());
            return false;
        }
        if (!safe_copy_path(szPath, sizeof(szPath), (char*)cab.parent_path().string().c_str(), FALSE)){
            LOG(LOG_LEVEL_ERROR, L"Can't copy cab parent path (%s)", cab.parent_path().wstring().c_str());
            return false;
        }
        safe_copy_path(szPath, sizeof(szPath), "\\", TRUE);
        if (!safe_copy_path(ExtractParam.sz_last_cab_name, sizeof(ExtractParam.sz_last_cab_name), szFile, FALSE)){
            return false;
        }
        if (!file.empty()){
            if (!safe_copy_path(ExtractParam.sz_extract_fileName, sizeof(ExtractParam.sz_extract_fileName), (char*)file.string().c_str(), FALSE)){
                LOG(LOG_LEVEL_ERROR, L"Can't copy extract file name (%s)", file.wstring().c_str());
                return false;
            }
        }
        _hFdi = FDICreate((PFNALLOC)fdi_alloc, //function to allocate memory
            (PFNFREE)fdi_free,   //function to free memory
            (PFNOPEN)fdi_open,  //function to open a file
            (PFNREAD)fdi_read,  //function to read data from a file
            (PFNWRITE)fdi_write, //function to write data to a file
            (PFNCLOSE)fdi_close, //function to close a file
            (PFNSEEK)fdi_seek,  //function to move the file pointer
            cpuUNKNOWN,  //only used by the 16-bit version of FDI
            &_erf);       //pointer the FDI error structure
        if (_hFdi){
            while (true){
                if (!FDICopy(_hFdi, szFile, szPath, 0, (PFNFDINOTIFY)(fdi_callback), NULL, &ExtractParam)){
                    LOG(LOG_LEVEL_ERROR, L"Failed to extract cab file. Error (%s)", fdi_error_to_string((FDIERROR)_erf.erfOper).c_str());
                    return false;
                }
                if (_stricmp(szFile, ExtractParam.sz_last_cab_name) == 0)
                    break;
                if (!safe_copy_path(szFile, sizeof(szFile), ExtractParam.sz_last_cab_name, FALSE))
                    return false;
            }
            _result = ExtractParam.is_extract_file_existing;
            FDIDestroy(_hFdi);
        }
    }
    return _result ? true : false;
}

bool cabinet::uncompress::is_cabinet(boost::filesystem::path cabfile){
    ERF                         _erf;
    HFDI                        _hFdi;
    int                         _handle = 0;
    FDICABINETINFO              _fdici;
    BOOL                        _result = FALSE;
    _hFdi = FDICreate((PFNALLOC)fdi_alloc, //function to allocate memory
        (PFNFREE)fdi_free,   //function to free memory
        (PFNOPEN)fdi_open,  //function to open a file
        (PFNREAD)fdi_read,  //function to read data from a file
        (PFNWRITE)fdi_write, //function to write data to a file
        (PFNCLOSE)fdi_close, //function to close a file
        (PFNSEEK)fdi_seek,  //function to move the file pointer
        cpuUNKNOWN,  //only used by the 16-bit version of FDI
        &_erf);       //pointer the FDI error structure
    if (_hFdi){
        _handle = fdi_open((char *)cabfile.string().c_str(), _O_BINARY | _O_RDONLY | _O_SEQUENTIAL, 0);
        if (_handle){
            _result = FDIIsCabinet(_hFdi, _handle, &_fdici);
            fdi_close(_handle);
        }
        FDIDestroy(_hFdi);
    }
    return _result ? true : false;
}

std::wstring cabinet::uncompress::fdi_error_to_string(FDIERROR err){

    switch (err)
    {
    case FDIERROR_NONE:
        return L"No error";

    case FDIERROR_CABINET_NOT_FOUND:
        return L"Cabinet not found";

    case FDIERROR_NOT_A_CABINET:
        return L"Not a cabinet";

    case FDIERROR_UNKNOWN_CABINET_VERSION:
        return L"Unknown cabinet version";

    case FDIERROR_CORRUPT_CABINET:
        return L"Corrupt cabinet";

    case FDIERROR_ALLOC_FAIL:
        return L"Memory allocation failed";

    case FDIERROR_BAD_COMPR_TYPE:
        return L"Unknown compression type";

    case FDIERROR_MDI_FAIL:
        return L"Failure decompressing data";

    case FDIERROR_TARGET_FILE:
        return L"Failure writing to target file";

    case FDIERROR_RESERVE_MISMATCH:
        return L"Cabinets in set have different RESERVE sizes";

    case FDIERROR_WRONG_CABINET:
        return L"Cabinet returned on fdintNEXT_CABINET is incorrect";

    case FDIERROR_USER_ABORT:
        return L"Application aborted";

    default:
        return L"Unknown error";
    }
}

#endif

};

};

#endif