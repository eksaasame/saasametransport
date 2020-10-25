// ssh_client.hpp ----------------------------------------------//
// -----------------------------------------------------------------------------

// Copyright 2013-2014 Albert Wei

// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt

// -----------------------------------------------------------------------------

// Revision History
// 04-29-2014 - Initial Release

// -----------------------------------------------------------------------------

#pragma once

#ifndef __MACHO_WINDOWS_SSH_CLIENT__
#define __MACHO_WINDOWS_SSH_CLIENT__

#include "..\config\config.hpp"
#include <sstream>
#include <iostream>
#include <fstream>
#include "boost\shared_ptr.hpp"
#include "common\exception_base.hpp"
#include "libssh2.h"

#pragma comment(lib, "zlib.lib")
#pragma comment(lib, "libssh2.lib")
#pragma comment(lib, "libeay32.lib")
#pragma comment(lib, "ssleay32.lib")
#pragma comment(lib, "ws2_32.lib") 

namespace macho{
namespace windows{

class ssh_client{
public:
    typedef boost::shared_ptr<ssh_client> ptr;
    virtual ~ssh_client();
    static ssh_client::ptr connect(std::string host, std::string username, std::string password, int port = 22);
    virtual bool run(std::string cmd, std::string& ret);
    virtual std::string run(std::string cmd){
        std::string ret;
        if (run(cmd, ret))
            return ret;
        return "";
    }
    virtual bool upload_file(std::string path, int mode, boost::filesystem::path data_file);
    virtual bool upload_file(std::string path, int mode, std::string data);
    virtual bool upload_file(std::string path, int mode, std::iostream& data);

private:
    bool wait_socket(int socket_fd);
    ssh_client(std::string host, int port);
    bool get_known_hosts();
    bool authenticate(std::string username, std::string password);
    static std::wstring get_error_message(DWORD error);
    void disconnect();
    SOCKET                      _sock;
    LIBSSH2_SESSION*            _session;
    std::string                 _host;
    int                         _port;
    critical_section            _cs;
};

#ifndef MACHO_HEADER_ONLY
#include "..\common\tracelog.hpp"

ssh_client::ssh_client(std::string host, int port)
    :_session(NULL),
    _host(host),
    _port(port){
    memset(&_sock, 0, sizeof(SOCKET));
}

ssh_client::~ssh_client(){
    disconnect();
}

void ssh_client::disconnect(){
    DWORD                       error;
    if (_session){
        if (LIBSSH2_ERROR_NONE != (error = libssh2_session_disconnect(_session, "Normal Shutdown, Thank you for playing"))){
            LOG(LOG_LEVEL_ERROR, L"libssh2_session_disconnect error (%d) %s.", error, get_error_message(error).c_str());
        }
        if (LIBSSH2_ERROR_NONE != (error = libssh2_session_free(_session))){
            LOG(LOG_LEVEL_ERROR, L"libssh2_session_free error (%d) %s.", error, get_error_message(error).c_str());
        }
        if (SOCKET_ERROR != closesocket(_sock)){
            libssh2_exit();
        }
        else{
            error = WSAGetLastError();
            LOG(LOG_LEVEL_ERROR, L"closesocket error: %s (%d).", environment::get_system_message(error).c_str(), error);
        }
    }
}

bool ssh_client::get_known_hosts(){
    bool                        result = false;
    DWORD                       error;
    const char *                finger_print;
    size_t                      len;
    int                         type;
    LIBSSH2_KNOWNHOSTS*         ssh_hnown_hosts;

    if (NULL != (ssh_hnown_hosts = libssh2_knownhost_init(_session))){
        /* read all hosts from here */
        libssh2_knownhost_readfile(ssh_hnown_hosts, "known_hosts", LIBSSH2_KNOWNHOST_FILE_OPENSSH);

        boost::filesystem::path dumpfile = environment::get_temp_path();
        if (!dumpfile.empty())
            dumpfile /= "dumpfile";
        else
            dumpfile = "dumpfile";

        /* store all known hosts to here */
        if (LIBSSH2_ERROR_NONE ==
            (error = libssh2_knownhost_writefile(ssh_hnown_hosts, dumpfile.string().c_str(), LIBSSH2_KNOWNHOST_FILE_OPENSSH))){
            if (NULL != (finger_print = libssh2_session_hostkey(_session, &len, &type))){
                struct libssh2_knownhost *host;
#if LIBSSH2_VERSION_NUM >= 0x010206
                /* introduced in 1.2.6 */
                int check = libssh2_knownhost_checkp(ssh_hnown_hosts, _host.c_str(), _port,
                    finger_print, len,
                    LIBSSH2_KNOWNHOST_TYPE_PLAIN | LIBSSH2_KNOWNHOST_KEYENC_RAW,
                    &host);
#else
                /* 1.2.5 or older */
                int check = libssh2_knownhost_check(sshKnownHosts, m_szHostName.c_str(),
                    szFingerprint, nLen,
                    LIBSSH2_KNOWNHOST_TYPE_PLAIN | LIBSSH2_KNOWNHOST_KEYENC_RAW,
                    &host);
#endif
                LOG(LOG_LEVEL_INFO, L"Host check: %d, key: %s", check,
                    (check <= LIBSSH2_KNOWNHOST_CHECK_MISMATCH) ? stringutils::convert_ansi_to_unicode(host->key).c_str() : L"<none>");
                result = true;
            }
            else{
                error = libssh2_session_last_errno(_session);
                LOG(LOG_LEVEL_ERROR, L"libssh2_session_hostkey error (%d) %s.", error, get_error_message(error).c_str());
            }
        }
        else{
            LOG(LOG_LEVEL_ERROR, L"libssh2_knownhost_writefile error (%d) %s.", error, get_error_message(error).c_str());
        }

        libssh2_knownhost_free(ssh_hnown_hosts);
    }
    else{
        error = libssh2_session_last_errno(_session);
        LOG(LOG_LEVEL_ERROR, L"libssh2_knownhost_init error (%d) %s.", error, get_error_message(error).c_str());
    }
    return result;
}

std::wstring ssh_client::get_error_message(DWORD error){
    switch (error){
    case LIBSSH2_ERROR_SOCKET_NONE: return L"LIBSSH2_ERROR_SOCKET_NONE";
    case LIBSSH2_ERROR_BANNER_RECV: return L"LIBSSH2_ERROR_BANNER_RECV";
    case LIBSSH2_ERROR_BANNER_SEND: return L"LIBSSH2_ERROR_BANNER_SEND";
    case LIBSSH2_ERROR_INVALID_MAC: return L"LIBSSH2_ERROR_INVALID_MAC";
    case LIBSSH2_ERROR_KEX_FAILURE: return L"LIBSSH2_ERROR_KEX_FAILURE";
    case LIBSSH2_ERROR_ALLOC: return L"LIBSSH2_ERROR_ALLOC";
    case LIBSSH2_ERROR_SOCKET_SEND: return L"LIBSSH2_ERROR_SOCKET_SEND";
    case LIBSSH2_ERROR_KEY_EXCHANGE_FAILURE: return L"LIBSSH2_ERROR_KEY_EXCHANGE_FAILURE";
    case LIBSSH2_ERROR_TIMEOUT: return L"LIBSSH2_ERROR_TIMEOUT";
    case LIBSSH2_ERROR_HOSTKEY_INIT: return L"LIBSSH2_ERROR_HOSTKEY_INIT";
    case LIBSSH2_ERROR_HOSTKEY_SIGN: return L"LIBSSH2_ERROR_HOSTKEY_SIGN";
    case LIBSSH2_ERROR_DECRYPT: return L"LIBSSH2_ERROR_DECRYPT";
    case LIBSSH2_ERROR_SOCKET_DISCONNECT: return L"LIBSSH2_ERROR_SOCKET_DISCONNECT";
    case LIBSSH2_ERROR_PROTO: return L"LIBSSH2_ERROR_PROTO";
    case LIBSSH2_ERROR_PASSWORD_EXPIRED: return L"LIBSSH2_ERROR_PASSWORD_EXPIRED";
    case LIBSSH2_ERROR_FILE: return L"LIBSSH2_ERROR_FILE";
    case LIBSSH2_ERROR_METHOD_NONE: return L"LIBSSH2_ERROR_METHOD_NONE";
    case LIBSSH2_ERROR_AUTHENTICATION_FAILED: return L"LIBSSH2_ERROR_AUTHENTICATION_FAILED";
    case LIBSSH2_ERROR_PUBLICKEY_UNVERIFIED: return L"LIBSSH2_ERROR_PUBLICKEY_UNVERIFIED";
    case LIBSSH2_ERROR_CHANNEL_OUTOFORDER: return L"LIBSSH2_ERROR_CHANNEL_OUTOFORDER";
    case LIBSSH2_ERROR_CHANNEL_FAILURE: return L"LIBSSH2_ERROR_CHANNEL_FAILURE";
    case LIBSSH2_ERROR_CHANNEL_REQUEST_DENIED: return L"LIBSSH2_ERROR_CHANNEL_REQUEST_DENIED";
    case LIBSSH2_ERROR_CHANNEL_UNKNOWN: return L"LIBSSH2_ERROR_CHANNEL_UNKNOWN";
    case LIBSSH2_ERROR_CHANNEL_WINDOW_EXCEEDED: return L"LIBSSH2_ERROR_CHANNEL_WINDOW_EXCEEDED";
    case LIBSSH2_ERROR_CHANNEL_PACKET_EXCEEDED: return L"LIBSSH2_ERROR_CHANNEL_PACKET_EXCEEDED";
    case LIBSSH2_ERROR_CHANNEL_CLOSED: return L"LIBSSH2_ERROR_CHANNEL_CLOSED";
    case LIBSSH2_ERROR_CHANNEL_EOF_SENT: return L"LIBSSH2_ERROR_CHANNEL_EOF_SENT";
    case LIBSSH2_ERROR_SCP_PROTOCOL: return L"LIBSSH2_ERROR_SCP_PROTOCOL";
    case LIBSSH2_ERROR_ZLIB: return L"LIBSSH2_ERROR_ZLIB";
    case LIBSSH2_ERROR_SOCKET_TIMEOUT: return L"LIBSSH2_ERROR_SOCKET_TIMEOUT";
    case LIBSSH2_ERROR_SFTP_PROTOCOL: return L"LIBSSH2_ERROR_SFTP_PROTOCOL";
    case LIBSSH2_ERROR_REQUEST_DENIED: return L"LIBSSH2_ERROR_REQUEST_DENIED";
    case LIBSSH2_ERROR_METHOD_NOT_SUPPORTED: return L"LIBSSH2_ERROR_METHOD_NOT_SUPPORTED";
    case LIBSSH2_ERROR_INVAL: return L"LIBSSH2_ERROR_INVAL";
    case LIBSSH2_ERROR_INVALID_POLL_TYPE: return L"LIBSSH2_ERROR_INVALID_POLL_TYPE";
    case LIBSSH2_ERROR_PUBLICKEY_PROTOCOL: return L"LIBSSH2_ERROR_PUBLICKEY_PROTOCOL";
    case LIBSSH2_ERROR_EAGAIN: return L"LIBSSH2_ERROR_EAGAIN";
    case LIBSSH2_ERROR_BUFFER_TOO_SMALL: return L"LIBSSH2_ERROR_BUFFER_TOO_SMALL";
    case LIBSSH2_ERROR_BAD_USE: return L"LIBSSH2_ERROR_BAD_USE";
    case LIBSSH2_ERROR_COMPRESS: return L"LIBSSH2_ERROR_COMPRESS";
    case LIBSSH2_ERROR_OUT_OF_BOUNDARY: return L"LIBSSH2_ERROR_OUT_OF_BOUNDARY";
    case LIBSSH2_ERROR_AGENT_PROTOCOL: return L"LIBSSH2_ERROR_AGENT_PROTOCOL";
    case LIBSSH2_ERROR_SOCKET_RECV: return L"LIBSSH2_ERROR_SOCKET_RECV";
    case LIBSSH2_ERROR_ENCRYPT: return L"LIBSSH2_ERROR_ENCRYPT";
    case LIBSSH2_ERROR_BAD_SOCKET: return L"LIBSSH2_ERROR_BAD_SOCKET";
    }
    return L"UNKNOWN";
}

bool ssh_client::authenticate(std::string username, std::string password){
    HRESULT                     hr = S_OK;
    DWORD                       error;
    if (password.length()){
        /* We could authenticate via password */
        while (LIBSSH2_ERROR_EAGAIN == (error = libssh2_userauth_password(_session, username.c_str(), password.c_str())))
            Sleep(100);
        if (LIBSSH2_ERROR_NONE != error){
            LOG(LOG_LEVEL_ERROR, L"libssh2_userauth_password error (%d) %s.", error, get_error_message(error).c_str());
            return false;
        }
    }
    else{
        /* Or by public key */
        while (LIBSSH2_ERROR_EAGAIN == (error = libssh2_userauth_publickey_fromfile(_session, username.c_str(),
            "/home/user/"
            ".ssh/id_rsa.pub",
            "/home/user/"
            ".ssh/id_rsa",
            password.c_str())))
            Sleep(100);
        if (LIBSSH2_ERROR_NONE != error){         
            LOG(LOG_LEVEL_ERROR, _T("libssh2_userauth_publickey_fromfile error (%d)."), error);
            return false;
        }
    }
    return true;
}

ssh_client::ptr ssh_client::connect(std::string host, std::string username, std::string password, int port){
    ssh_client::ptr p(new ssh_client(host, port));
    if (p){
        DWORD                       error =0 ;
        HRESULT                     hr = S_OK;
        ULONG                       hostAddr;
        std::wstring                wszMessage;
        struct sockaddr_in          sin;
        if (LIBSSH2_ERROR_NONE != (error = libssh2_init(0))){
            LOG(LOG_LEVEL_ERROR, L"libssh2_init error (%d) %s.", error, get_error_message(error).c_str());
        }
        else{
            hostAddr = inet_addr(host.c_str());
            /* Ultra basic "connect to port 22 on localhost"
            * Your code is responsible for creating the socket establishing the
            * connection
            */
            p->_sock = socket(AF_INET, SOCK_STREAM, 0);
            sin.sin_family = AF_INET;
            sin.sin_port = htons(port);
            sin.sin_addr.s_addr = hostAddr;
            if (SOCKET_ERROR != ::connect(p->_sock, (struct sockaddr *)(&sin), sizeof(struct sockaddr_in))){
                /* Create a session instance */
                if (NULL != (p->_session = libssh2_session_init())){
                    /* tell libssh2 we want it all done non-blocking */
                    libssh2_session_set_blocking(p->_session, 0);

                    /* ... start it up. This will trade welcome banners, exchange keys,
                    * and setup crypto, compression, and MAC layers
                    */
                    while (LIBSSH2_ERROR_EAGAIN == (error = libssh2_session_startup(p->_session, p->_sock)))
                        Sleep(100);
                    if (error)
                        LOG(LOG_LEVEL_ERROR, _T("libssh2_session_startup error (%d)."), error);
                    else if ( p->get_known_hosts() && p->authenticate(username, password))
                        return p;
                }
                else
                    LOG(LOG_LEVEL_ERROR, L"libssh2_session_init error.");
            }
            else{                 
                error = WSAGetLastError();
                LOG(LOG_LEVEL_ERROR, L"connect error: %s (%d).", environment::get_system_message(error).c_str(), error);
            }
        }
    }
    return NULL;
}

bool ssh_client::wait_socket(int socket_fd){
    bool                        result = false;
    DWORD                       error;
    struct timeval              timeout;
    fd_set                      fd;
    fd_set *                    writefd = NULL;
    fd_set *                    readfd = NULL;
    int                         dir;
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;
    FD_ZERO(&fd);
    FD_SET(socket_fd, &fd);

    /* now make sure we wait in the correct direction */
    dir = libssh2_session_block_directions(_session);
    if (dir & LIBSSH2_SESSION_BLOCK_INBOUND)
        readfd = &fd;

    if (dir & LIBSSH2_SESSION_BLOCK_OUTBOUND)
        writefd = &fd;

    if (SOCKET_ERROR != (error = select(socket_fd + 1, readfd, writefd, NULL, &timeout)))
        result = true;
    else{
        error = WSAGetLastError();
        LOG(LOG_LEVEL_ERROR, L"select error: %s (%d).", environment::get_system_message(error).c_str(), error);
    }
    return result;
}

bool ssh_client::run(std::string cmd, std::string& ret){
#define BUFFER_SIZE 0x4000
    DWORD                       error = 0;
    int                         exit_code = 0;
    int                         byte_count = 0;
    char *                      exit_signal = "none";
    LIBSSH2_CHANNEL*            ssh_channel = NULL;
    auto_lock                   lock(_cs);
    /* Exec non-blocking on the remove host */
    while (NULL == (ssh_channel = libssh2_channel_open_session(_session)) &&
        LIBSSH2_ERROR_EAGAIN == (error = libssh2_session_last_error(_session, NULL, NULL, 0))){
        wait_socket(_sock);
        Sleep(100);
    }

    if (ssh_channel){
        while (LIBSSH2_ERROR_EAGAIN == (error = libssh2_channel_exec(ssh_channel, cmd.c_str()))){
            wait_socket(_sock);
            Sleep(100);
        }
        if (LIBSSH2_ERROR_NONE == error){
            for (;;){
                /* loop until we block */
                int rc;
                do{
                    char buffer[BUFFER_SIZE];
                    memset(buffer, 0, BUFFER_SIZE);
                    rc = libssh2_channel_read(ssh_channel, buffer, sizeof(buffer) - 1);
                    if (rc > 0){
                        /*
                        int i;
                        bytecount += rc;
                        fprintf(stderr, "We read:\n");
                        for( i=0; i < rc; ++i )
                        fputc( buffer[i], stderr);
                        fprintf(stderr, "\n");
                        */
                       /* if (rc >= BUFFER_SIZE)
                            buffer[BUFFER_SIZE - 1] = NULL;
                        else
                            buffer[rc] = NULL;*/

                        if (strlen(buffer)){
                            ret.append(buffer);
                            LOG(LOG_LEVEL_DEBUG, L"libssh2_channel_read returned \"%s\"", stringutils::convert_ansi_to_unicode(buffer).c_str());
                        }
                    }
                    else{
                        LOG(LOG_LEVEL_TRACE, L"libssh2_channel_read returned %d", rc);
                    }
                } while (rc > 0);

                do{
                    char buffer[BUFFER_SIZE];
                    memset(buffer, 0, BUFFER_SIZE);
                    rc = libssh2_channel_read_ex(ssh_channel, SSH_EXTENDED_DATA_STDERR, buffer, sizeof(buffer) - 1);
                    if (rc > 0){
                        if (strlen(buffer)){
                            ret.append(buffer);
                            LOG(LOG_LEVEL_DEBUG, L"libssh2_channel_read returned \"%s\"", stringutils::convert_ansi_to_unicode(buffer).c_str());
                        }
                    }
                    else{
                        LOG(LOG_LEVEL_TRACE, L"libssh2_channel_read_ex returned %d", rc);
                    }
                } while (rc > 0);

                /* this is due to blocking that would occur otherwise so we loop on
                this condition */
                if (rc == LIBSSH2_ERROR_EAGAIN){
                    wait_socket(_sock);
                }
                else
                    break;
            }
            exit_code = 127;
            while (LIBSSH2_ERROR_EAGAIN == (error = libssh2_channel_close(ssh_channel)))
                wait_socket(_sock);

            if (LIBSSH2_ERROR_NONE == error){
                exit_code = libssh2_channel_get_exit_status(ssh_channel);
                libssh2_channel_get_exit_signal(ssh_channel, &exit_signal, NULL, NULL, NULL, NULL, NULL);
            }

            if (exit_signal)
                LOG(LOG_LEVEL_TRACE, L"Got signal: %s", stringutils::convert_ansi_to_unicode(exit_signal).c_str());
            else
                LOG(LOG_LEVEL_TRACE, L"EXIT: %d bytecount: %d", exit_code,byte_count);
            libssh2_channel_free(ssh_channel);
        }
        else{
            LOG(LOG_LEVEL_ERROR, L"libssh2_channel_exec('%s') error (%d).", stringutils::convert_ansi_to_unicode(cmd).c_str(), error);
        }
    }
    else{
        LOG(LOG_LEVEL_ERROR, L"libssh2_channel_open_session error (%d) %s.", error, get_error_message(error).c_str());
    }
    return (LIBSSH2_ERROR_NONE == error);
}

bool ssh_client::upload_file(std::string path, int mode, boost::filesystem::path data_file){
    return upload_file(path, mode, std::fstream(data_file.string(), std::ios::in | std::ios::binary));
}

bool ssh_client::upload_file(std::string path, int mode, std::string data){
    return upload_file(path, mode, std::stringstream(data));
}

bool ssh_client::upload_file(std::string path, int mode, std::iostream& data){
    int                         rc = 0;
    DWORD                       error = 0;
    LIBSSH2_CHANNEL*            ssh_channel = NULL;
    auto_lock                   lock(_cs);
    data.seekg((size_t)0, std::iostream::end);
    size_t data_size = data.tellg();
    data.seekg((size_t)0, std::iostream::beg);

    while (NULL == (ssh_channel = libssh2_scp_send64(_session, path.c_str(), mode & 0777, data_size, 0, 0)) &&
    LIBSSH2_ERROR_EAGAIN == (error = libssh2_session_last_error(_session, NULL, NULL, 0))){
        wait_socket(_sock);
        Sleep(100);
    }
    if (!ssh_channel){
        LOG(LOG_LEVEL_ERROR, L"libssh2_scp_send error (%d) %s.", error, get_error_message(error).c_str());
    }
    else{
        error = 0;
        char buf[4096];
        do {
            memset(buf, 0, sizeof(buf));
            size_t nread = (data_size - data.tellg()) > 4096 ? 4096 : (data_size - data.tellg());
            data.read((char *)buf, nread);
            /* write the same data over and over, until error or completion */
            while (LIBSSH2_ERROR_EAGAIN == (rc = libssh2_channel_write(ssh_channel, buf, nread))){
                wait_socket(_sock);
                Sleep(100);
            }
            if (rc < 0) {
                error = rc;
                LOG(LOG_LEVEL_ERROR, L"libssh2_channel_write error (%d).", rc);
                break;
            }
            if (0 == (size_t)data.tellg() - data_size)
                break;
        } while (true);
    }

    libssh2_channel_send_eof(ssh_channel);
    libssh2_channel_wait_eof(ssh_channel);
    libssh2_channel_wait_closed(ssh_channel);
    libssh2_channel_free(ssh_channel);
    ssh_channel = NULL;
    return (LIBSSH2_ERROR_NONE == error);
}

#endif
}
}
#endif