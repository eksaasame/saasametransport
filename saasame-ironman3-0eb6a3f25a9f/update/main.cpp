// Setup.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "macho.h"
#include <boost/program_options.hpp>
#include <iostream>     // std::cout
#include <fstream>
#include <string>
#include "miniz.c"

using namespace macho;
using namespace macho::windows;
namespace po = boost::program_options;

void extract_binary_resource( stdstring custom_resource_name, int resource_id, stdstring output_file_path, bool append ){
	HGLOBAL hResourceLoaded;		// handle to loaded resource 
	HRSRC hRes;						// handle/ptr. to res. info. 
	char *lpResLock;				// pointer to resource data 
	DWORD dwSizeRes;
	// find location of the resource and get handle to it
	hRes = FindResource( NULL, MAKEINTRESOURCE(resource_id), custom_resource_name.c_str() );
	// loads the specified resource into global memory. 
	hResourceLoaded = LoadResource( NULL, hRes ); 
	// get a pointer to the loaded resource!
	lpResLock = (char*)LockResource( hResourceLoaded ); 
	// determine the size of the resource, so we know how much to write out to file!  
	dwSizeRes = SizeofResource( NULL, hRes );
	std::ofstream outputFile(output_file_path.c_str(), append ? std::ios::binary | std::ios::app : std::ios::binary );
	outputFile.write((const char*)lpResLock, dwSizeRes);
	outputFile.close();
}

bool exec_console_application( stdstring command, stdstring &ret, bool is_hidden ){

    HANDLE              hStdInRead, hStdInWrite, hStdInWriteTemp;
    HANDLE              hStdOutRead, hStdOutWrite, hStdOutReadTemp;
    SECURITY_ATTRIBUTES sa;
    STARTUPINFO         si;
    PROCESS_INFORMATION pi;
    HRESULT             hr = S_OK;

    if ( !command.length() )
		return false;
    ZeroMemory( &sa, sizeof( SECURITY_ATTRIBUTES ) );
    sa.nLength = sizeof( SECURITY_ATTRIBUTES );
    sa.bInheritHandle = TRUE;
    ret.empty();
    if ( CreatePipe( &hStdOutRead, &hStdOutWrite, &sa, 0 ) ){
        if ( DuplicateHandle( GetCurrentProcess(), hStdOutRead, GetCurrentProcess(), &hStdOutReadTemp, 0, FALSE, DUPLICATE_SAME_ACCESS ) ){
            if ( CreatePipe( &hStdInRead, &hStdInWrite, &sa, 0 ) ){
                if ( DuplicateHandle( GetCurrentProcess(), hStdInWrite, GetCurrentProcess(), &hStdInWriteTemp, 0, FALSE, DUPLICATE_SAME_ACCESS ) ){
                    ZeroMemory( &si, sizeof( STARTUPINFO ) );
                    si.cb           = sizeof( STARTUPINFO );
                    si.dwFlags      = STARTF_USESTDHANDLES;
                    si.hStdOutput   = hStdOutWrite;
                    si.hStdInput    = hStdInRead;
                    si.hStdError    = GetStdHandle( STD_ERROR_HANDLE );

                    ZeroMemory( &pi, sizeof( PROCESS_INFORMATION ) );

                    if ( CreateProcess( NULL,               /*ApplicationName*/
                                        (LPTSTR) command.c_str(),          /*CommandLine*/
                                        NULL,               /*ProcessAttributes*/
                                        NULL,               /*ThreadAttributes*/ 
                                        TRUE,               /*InheritHandles*/
                                        is_hidden ? CREATE_NO_WINDOW : 0,   /*CreationFlags*/ 
                                        NULL,               /*Environment*/
                                        NULL,               /*CurrentDirectory*/
                                        &si,                /*StartupInfo*/
                                        &pi ) )  {           /*ProcessInformation*/
#define BUFFSIZE 512                        
                        CHAR    szBuffer[ BUFFSIZE + 1 ];
                        DWORD   procRetCode = 0;
                        unsigned long exit=0;  //process exit code
                        unsigned long bread;   //bytes read
                        unsigned long avail;   //bytes available
                        while ( TRUE ){
                            GetExitCodeProcess(pi.hProcess,&exit);      //while the process is running
                            if (exit != STILL_ACTIVE)
                              break;
                            
                            PeekNamedPipe( hStdOutReadTemp, szBuffer, BUFFSIZE, &bread, &avail, NULL);
                            if ( bread == 0 ){
 								FILETIME    ftCreate, ftExit, ftKernel, ftUser;
								memset(&ftExit, 0 , sizeof( FILETIME ) );                           
								
								if( !GetProcessTimes( pi.hProcess, &ftCreate, &ftExit, &ftKernel, &ftUser ) )
									break;
								else if ( ( ftExit.dwHighDateTime != 0 ) || ( ftExit.dwLowDateTime != 0 ) )
									break;
								else{
									//SYSTEMTIME localTimeNow;
									//FILETIME fileTimeNow;
									//GetLocalTime(&localTimeNow);
									//SystemTimeToFileTime(&localTimeNow, &fileTimeNow);
									//ULARGE_INTEGER ulCreate, ulNow;
									//ulCreate.HighPart = ftCreate.dwHighDateTime;
									//ulCreate.LowPart  = ftCreate.dwLowDateTime;
									//ulNow.HighPart	  = fileTimeNow.dwHighDateTime;
									//ulNow.LowPart     = fileTimeNow.dwLowDateTime;
								}
								Sleep(500);
							}
							else{
                                ZeroMemory( szBuffer, sizeof( CHAR ) * ( BUFFSIZE + 1 ) );
                                if ( avail > BUFFSIZE ){
                                    while ( bread >= BUFFSIZE ){
                                        ReadFile( hStdOutReadTemp, szBuffer, BUFFSIZE, &bread, NULL );
#if _UNICODE
                                        ret.append( macho::stringutils::convert_ansi_to_unicode(szBuffer) );
#else
                                        ret.append(szBuffer);
#endif
                                        ZeroMemory( szBuffer, sizeof( CHAR ) * ( BUFFSIZE + 1 ) );
                                    }
                                }
                                else {
                                    ReadFile( hStdOutReadTemp, szBuffer, BUFFSIZE, &bread, NULL );
#if _UNICODE
                                        ret.append( macho::stringutils::convert_ansi_to_unicode(szBuffer) );
#else
                                        ret.append(szBuffer);
#endif                             
                                }
                            }
                        }

                        if ( GetExitCodeProcess( pi.hProcess, &procRetCode ) )            
                            hr = procRetCode;
                        else
                            hr = GetLastError();

                        CloseHandle( pi.hProcess );
                        CloseHandle( pi.hThread );
                    }
                    else
                        hr = GetLastError();
                    CloseHandle( hStdInWriteTemp );
                }            
                else
                    hr = GetLastError();
                CloseHandle( hStdInRead );
                CloseHandle( hStdInWrite );
            }
            else
                hr = GetLastError();
            CloseHandle( hStdOutReadTemp );
        }    
        else
            hr = GetLastError();
        CloseHandle( hStdOutRead );
        CloseHandle( hStdOutWrite );
    }
    else
        hr = GetLastError();
    SetLastError( hr );
    return HRESULT_FROM_WIN32(hr) == 0;
}

bool decompress_signal_file(const char* data, std::size_t size, const std::string& filename, const std::string& output ){
    mz_zip_archive zip_archive;
    memset(&zip_archive, 0, sizeof(zip_archive));
    if (!mz_zip_reader_init_mem(&zip_archive, data, size, 0)) {
        std::cout << "mz_zip_reader_init_mem() failed!\n";
        LOG(LOG_LEVEL_ERROR, _T("mz_zip_reader_init_mem() failed!"));
        return false;
    }
    boost::filesystem::path filepath = boost::filesystem::path(output);
    boost::filesystem::path parent_path = filepath.parent_path();
    if (!boost::filesystem::exists(parent_path)){
        if (!boost::filesystem::create_directories(filepath.parent_path())) {
            std::cout << "DecompressArchive could not create directory.\n";
            LOG(LOG_LEVEL_ERROR, _T("DecompressArchive could not create directory."));
            return false;
        }
    }
    boost::filesystem::remove(output);
    if (!mz_zip_reader_extract_file_to_file(&zip_archive, filename.c_str(), output.c_str(), 0)) {
        std::cout << "mz_zip_reader_extract_file_to_file() failed!\n";
        LOG(LOG_LEVEL_ERROR, _T("mz_zip_reader_extract_file_to_file() failed!"));
        mz_zip_reader_end(&zip_archive);
        return false;
    }

    if (!mz_zip_reader_end(&zip_archive)) {
        std::cout << "mz_zip_reader_end() failed!\n";
        LOG(LOG_LEVEL_ERROR, _T("mz_zip_reader_end() failed!"));
        return false;
    }
    return true;
}

bool decompress_archive( const char* data, std::size_t size, const std::string& directory, std::vector<std::string>& extracted_files ){

	mz_zip_archive zip_archive;
	memset(&zip_archive, 0, sizeof(zip_archive));

	if( !mz_zip_reader_init_mem( &zip_archive, data, size, 0 ) ) {
	    std::cout << "mz_zip_reader_init_mem() failed!\n";
        LOG(LOG_LEVEL_ERROR, _T("mz_zip_reader_init_mem() failed!"));
	    return false;
	}

    for( unsigned int i = 0; i < mz_zip_reader_get_num_files( &zip_archive ); i++ ) {
        mz_zip_archive_file_stat file_stat;
        if( !mz_zip_reader_file_stat( &zip_archive, i, &file_stat ) ) {
            std::cout << "mz_zip_reader_file_stat() failed!\n";
            LOG(LOG_LEVEL_ERROR, _T("mz_zip_reader_file_stat() failed!"));
            mz_zip_reader_end( &zip_archive );
            return false;
        }

        if( !mz_zip_reader_is_file_a_directory( &zip_archive, i ) ) {
            auto filename = directory + ( directory.empty() ? "" : "\\" ) + file_stat.m_filename;

			boost::filesystem::path filepath = boost::filesystem::path(filename);
			boost::filesystem::path parent_path = filepath.parent_path();
			if (!boost::filesystem::exists(parent_path)){
				//if ( std::find(extracted_files.begin(), extracted_files.end(), parent_path.generic_string()) == extracted_files.end())
				//extracted_files.push_back(parent_path.generic_string());
				if( !boost::filesystem::create_directories( filepath.parent_path() ) ) {
					std::cout << "DecompressArchive could not create directory.\n";
                    LOG(LOG_LEVEL_ERROR, _T("DecompressArchive could not create directory."));
					return false;
				}
            }
			extracted_files.push_back(filename);
			boost::filesystem::remove(filename);
            if( !mz_zip_reader_extract_to_file( &zip_archive, i, filename.c_str(), 0 ) ) {
                std::cout << "mz_zip_reader_extract_to_file() failed!\n";
                LOG(LOG_LEVEL_ERROR, _T("mz_zip_reader_extract_to_file() failed!"));
                mz_zip_reader_end( &zip_archive );
                return false;
            }
        }
    }

    if( !mz_zip_reader_end( &zip_archive ) ) {
        std::cout << "mz_zip_reader_end() failed!\n";
        LOG(LOG_LEVEL_ERROR, _T("mz_zip_reader_end() failed!"));
        return false;
    }

    return true;
}

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPTSTR    lpCmdLine,
                     _In_ int       nCmdShow){

	macho::windows::com_init com;
	DWORD dwRet = 0;
	std::wstring exepath = macho::windows::environment::get_execution_full_path();
	std::wstring working_dir = macho::windows::environment::get_working_directory();
    std::wstring post_cmd;
    std::wstring service_name;
    boost::filesystem::path logfile = macho::windows::environment::get_execution_full_path() + L".log";
    boost::filesystem::create_directories(logfile.parent_path());
    macho::set_log_file(logfile.wstring());
    macho::set_log_level(LOG_LEVEL_WARNING);

	//https://github.com/binary1248/SFMLInstaller/blob/master/src/Decompress.cpp
	std::ifstream inputFile( exepath.c_str(), std::ios::binary|std::ios::in );
	char sig[4];
	std::vector<BYTE> buff;
	while( inputFile && inputFile.read(sig,4)){
		if ( MZ_ZIP_LOCAL_DIR_HEADER_SIG == MZ_READ_LE32(sig) ){
			buff.push_back(sig[0]);
			buff.push_back(sig[1]);
			buff.push_back(sig[2]);
			buff.push_back(sig[3]);
			char ch;
			while( inputFile.get(ch)) buff.push_back(ch);
			break;
		}
	}
	inputFile.close();
	
    if (0 == buff.size()){
        LOG(LOG_LEVEL_ERROR, _T("Can't find archived data!"));
        return 1;
    }

    boost::filesystem::path ini_path	   = boost::str(boost::wformat(L"%s\\update.ini") %working_dir) ; 
    if (!decompress_signal_file((const char*)&buff[0], buff.size(), "update.ini", ini_path.string())){
        LOG(LOG_LEVEL_ERROR, _T("Can't extract update.ini file!"));
    }
    else{
        po::variables_map   vm;
        std::locale::global(std::locale(""));
        po::options_description config_file("Options");
        config_file.add_options()
            ("Service", po::wvalue<std::wstring>()->default_value(L"", ""))
            ("PostCmd", po::wvalue<std::wstring>()->default_value(L"", ""))
            ;
        try{
            std::ifstream ifs(ini_path.wstring());
            if (ifs.is_open()){
                po::store(po::parse_config_file(ifs, config_file, true), vm);
                service_name = macho::stringutils::parser_double_quotation_mark(vm["Service"].as<std::wstring>());
                post_cmd = macho::stringutils::parser_double_quotation_mark(vm["PostCmd"].as<std::wstring>());
            }
        }
        catch (...){}
        
        std::wstring ret;

        try{
            if (!service_name.empty()){
                macho::windows::service sc = macho::windows::service::get_service(service_name);
                sc.stop();
            }
        }
        catch(macho::windows::service_exception &ex){
            LOG(LOG_LEVEL_ERROR, _T("%s"), get_diagnostic_information(ex).c_str());
        }
        catch (...) { 
            LOG(LOG_LEVEL_ERROR, _T("Failed to stop the service (%s)!"), service_name.c_str());
        }

        std::vector<std::string> extracted_files;
        if (!decompress_archive((const char*)&buff[0], buff.size(), macho::stringutils::convert_unicode_to_ansi(working_dir), extracted_files)){
            LOG(LOG_LEVEL_ERROR, _T("Failed to extract files!"));
        }
        buff.clear();

        try{
            if (!service_name.empty()){
                macho::windows::service sc = macho::windows::service::get_service(service_name);
                sc.start();
            }
        }
        catch (macho::windows::service_exception &ex){
            LOG(LOG_LEVEL_ERROR, _T("%s"), get_diagnostic_information(ex).c_str());
        }
        catch (...) {
            LOG(LOG_LEVEL_ERROR, _T("Failed to start the service (%s)!"), service_name.c_str());
        }

        if (post_cmd.length()){
            std::wstring batch_cmd = boost::str(boost::wformat(L"\"%s\\%s\"") % working_dir%post_cmd);
            LOG(LOG_LEVEL_INFO, _T("Execute the post command file (%s)!"), batch_cmd.c_str());
            exec_console_application(batch_cmd, ret, true);
        }
    }
	return dwRet;
}

