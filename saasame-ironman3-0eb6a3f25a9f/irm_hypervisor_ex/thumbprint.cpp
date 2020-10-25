#include "vmware_ex.h"
#include <boost/asio.hpp>
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>

using namespace macho;
using namespace boost;
using namespace mwdc::ironman::hypervisor_ex;

#define SHA1LEN 20

DWORD vmware_portal_ex::get_ssl_thumbprint(const std::wstring& host, const int port, std::string& thumbprint)
{
    FUN_TRACE;

    int     sockfd = 0;
    DWORD   dwReturn = S_FALSE;
    BIO     *certbio = NULL;
    X509    *cert = NULL;
    SSL_CTX *ctx = NULL;
    SSL     *ssl = NULL;
    const SSL_METHOD *method;
    struct hostent *hostent;
    struct sockaddr_in dest_addr = { 0 };
    std::string hostname("");

    int size = WideCharToMultiByte(CP_UTF8, 0, host.c_str(), -1, NULL, 0, NULL, NULL);

    if (size > 0)
    {
        char *buffer = new char[size + 1];
        WideCharToMultiByte(CP_UTF8, 0, host.c_str(), -1, buffer, size, NULL, NULL);

        hostname = buffer;
        delete[]buffer;
    }
    else
    {
        LOG(LOG_LEVEL_ERROR, L"Failed to allocate memory buffer, error = %d", GetLastError());
        goto _exit;
    }

    dest_addr.sin_addr.S_un.S_addr = inet_addr(hostname.c_str());
    if (dest_addr.sin_addr.S_un.S_addr == INADDR_NONE)
    {
        dest_addr.sin_addr.S_un.S_addr = 0;
        if ((hostent = gethostbyname(hostname.c_str())) != NULL)
        {
            dest_addr.sin_addr.s_addr = *(long*)(hostent->h_addr);
        }
        else
        {
            LOG(LOG_LEVEL_ERROR, L"Invalid host format, %s", host.c_str());
            goto _exit;
        }
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(port);

    if (::connect(sockfd, (struct sockaddr *) &dest_addr, sizeof(struct sockaddr)) == -1) 
    {
        DWORD err_code = GetLastError();
        LOG(LOG_LEVEL_ERROR, L"Error: Cannot connect to host %s on port %d, error = %d", host.c_str(), port, err_code);
        goto _exit;
    }

    OpenSSL_add_all_algorithms();
    ERR_load_BIO_strings();
    ERR_load_crypto_strings();
    SSL_load_error_strings();

    certbio = BIO_new(BIO_s_mem());

    if (SSL_library_init() < 0)
    {
        LOG(LOG_LEVEL_ERROR, L"Could not initialize the OpenSSL library!");
        goto _exit;
    }

    method = SSLv23_client_method();

    if ((ctx = SSL_CTX_new(method)) == NULL)
    {
        LOG(LOG_LEVEL_ERROR, L"Unable to create a new SSL context structure.");
        goto _exit;
    }

    SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv2);

    ssl = SSL_new(ctx);

    SSL_set_fd(ssl, sockfd);

    if (SSL_connect(ssl) != 1)
    {
        LOG(LOG_LEVEL_ERROR, L"Error: Could not build a SSL session to: %s and port: %d.", host.c_str(), port);
        goto _exit;
    }

    cert = SSL_get_peer_certificate(ssl);
    if (cert == NULL)
    {
        LOG(LOG_LEVEL_ERROR, L"Error: Could not get a certificate from: %s.", host.c_str());
        goto _exit;
    }

    unsigned char sha1_buf[SHA1LEN] = { 0 };

    const EVP_MD *digest = EVP_sha1();
    unsigned len;
    char thubmprint_buf[3 * SHA1LEN + 1] = { 0 };

    int rc = X509_digest(cert, digest, (unsigned char*)sha1_buf, &len);
    if (rc == 0 || len != SHA1LEN)
    {
        LOG(LOG_LEVEL_ERROR, L"Failed to get the server's digest from: %s.", host.c_str());
        goto _exit;
    }
    else
    {
        char *l = (char*)(((intptr_t)thubmprint_buf));

        for (size_t i = 0; i < SHA1LEN; i++)
        {
            sprintf(l, "%02x", sha1_buf[i]);
            l += 2;

            if (i < SHA1LEN - 1)
            {
                sprintf(l, "%c", ':');
                l += 1;
            }
        }
        LOG(LOG_LEVEL_INFO, L"%s", thubmprint_buf);
    }

    dwReturn = S_OK;

_exit:
    if (ssl != NULL)
        SSL_free(ssl);
    
    if (sockfd > 0)
        closesocket(sockfd);
    
    if (cert != NULL)
        X509_free(cert);

    if (ctx != NULL)
        SSL_CTX_free(ctx);

    if (dwReturn == S_OK && strlen(thubmprint_buf) > 0)
        thumbprint = thubmprint_buf;
    
    return dwReturn;
}