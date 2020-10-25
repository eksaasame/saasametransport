// protected_data.hpp ----------------------------------------------//
// -----------------------------------------------------------------------------

// Copyright 2013-2014 Albert Wei

// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt

// -----------------------------------------------------------------------------

// Revision History
// 04-29-2014 - Initial Release

// -----------------------------------------------------------------------------

#pragma once

#ifndef __MACHO_WINDOWS_CERTIFICATE_H
#define __MACHO_WINDOWS_CERTIFICATE_H

#include <windows.h>
#include <Wincrypt.h>
#include <wintrust.h>
#include "boost\shared_ptr.hpp"
#include "common\exception_base.hpp"

#pragma comment(lib, "crypt32.lib")

namespace macho{
namespace windows{

class certificate_store;
class certificate{
public:
    typedef boost::shared_ptr<certificate> ptr;
    typedef std::vector<ptr> vtr;
    struct  exception : virtual public exception_base {};
    virtual ~certificate(){ CertFreeCertificateContext(_cert_context); }
    DWORD const version(){
        return _cert_context->pCertInfo->dwVersion;
    }
    std::wstring const serial_number();
    std::wstring const signature_algorithm();
    std::wstring const public_key_algorithm();

    std::wstring const name(){
        std::wstring name;
        _get_name_string(CERT_NAME_SIMPLE_DISPLAY_TYPE, name);
        return name;
    }

    std::wstring const subject_name(){
        std::wstring name;
        _get_name_string(CERT_NAME_SIMPLE_DISPLAY_TYPE, name);
        return name;
    }

    std::wstring const issuer_name(){
        std::wstring name;
        _get_name_string(CERT_NAME_SIMPLE_DISPLAY_TYPE, name, CERT_NAME_ISSUER_FLAG);
        return name;
    }

    std::wstring const friendly_name(){
        std::wstring name;
        _get_name_string(CERT_NAME_FRIENDLY_DISPLAY_TYPE, name);
        return name;
    }

    std::wstring const rdn_name(){
        std::wstring name;
        _get_name_string(CERT_NAME_RDN_TYPE, name);
        return name;
    }

    std::wstring const cn_name(){
        std::wstring name;
        _get_name_string(CERT_NAME_ATTR_TYPE, name, 0, _T("2.5.4.3"));
        return name;
    }

    std::wstring const url(){
        std::wstring name;
        _get_name_string(CERT_NAME_URL_TYPE, name);
        return name;
    }

    std::wstring const email(){
        std::wstring name;
        _get_name_string(CERT_NAME_EMAIL_TYPE, name);
        return name;
    }

    FILETIME const valid_from_time(SYSTEMTIME &localsystemtime = SYSTEMTIME(), FILETIME &localfiletime = FILETIME()){
        FileTimeToLocalFileTime(&(_cert_context->pCertInfo->NotBefore), &localfiletime);
        FileTimeToSystemTime(&localfiletime, &localsystemtime);
        return _cert_context->pCertInfo->NotBefore;
    }

    FILETIME const valid_to_time(SYSTEMTIME &localsystemtime = SYSTEMTIME(), FILETIME &localfiletime = FILETIME()){
        FileTimeToLocalFileTime(&(_cert_context->pCertInfo->NotAfter), &localfiletime);
        FileTimeToSystemTime(&localfiletime, &localsystemtime);
        return _cert_context->pCertInfo->NotAfter;
    }

    std::vector<std::wstring>	enhanced_key_usages();
    bool	                    is_support_email_protection();
    bool                        has_private_key();
    DWORD                       acquire_certificate_private_key(HCRYPTPROV* phCryptProv, DWORD* pKeySpec);
    certificate::vtr            certificate_chain();
    bool                        export_cert_der(bytes &data);
    bool                        export_cert_der_file(__in boost::filesystem::path cer_file_path, bool base64 = false);
    bool                        export_to_pfx_file(__in boost::filesystem::path  pfx_file_path, __in std::wstring password, __in bool exportchain = true);
    bool                        export_to_pfx_blob(CRYPT_DATA_BLOB &blob, __in std::wstring password, __in bool exportchain = true);
    PCCERT_CONTEXT const get() {
        return _cert_context;
    }
private:
    friend class certificate_store;
    certificate(PCCERT_CONTEXT pcert) : _cert_context(NULL){ _cert_context = CertDuplicateCertificateContext(pcert); }
    PCCERT_CONTEXT  _cert_context;
    SECURITY_STATUS _ncrypt_free_object(__in  NCRYPT_HANDLE object);
    int             _get_name_string(__in   DWORD type, __out std::wstring& name, __in_opt DWORD flags = 0, __in_opt void *pv_type_para = NULL);
    DWORD           _property(DWORD prop_id, bytes& data = bytes());
};

struct authenticode_signed_info{
    typedef boost::shared_ptr<authenticode_signed_info> ptr;
    authenticode_signed_info() {
        memset(&st, 0, sizeof(SYSTEMTIME));
    }
    std::wstring            program_name;
    std::wstring            publisher_link;
    std::wstring            more_info_link;
    SYSTEMTIME              st;
    certificate::ptr        signer_certificate;
    certificate::ptr        timestamp_certificate;
};

class certificate_store{
public:
    typedef boost::shared_ptr<certificate_store> ptr;
    static authenticode_signed_info::ptr get_authenticode_signed_info(const boost::filesystem::path file);

    static certificate_store::ptr open_system_store(std::wstring subsystem_protocol = L"MY" );

    static certificate_store::ptr open_store(DWORD dwFlags, std::wstring store_name){
        if (store_name.empty())
            return NULL;
        /*
        * Make sure CERT_STORE_OPEN_EXISTING_FLAG is not set. This causes Windows XP
        * to return ACCESS_DENIED when installing TrustedPublisher certificates via
        * CertAddCertificateContextToStore() if the TrustedPublisher store never has
        * been used (through certmgr.exe and friends) yet.
        *
        * According to MSDN, if neither CERT_STORE_OPEN_EXISTING_FLAG nor
        * CERT_STORE_CREATE_NEW_FLAG is set, the store will be either opened or
        * created accordingly.
        */
        dwFlags &= ~CERT_STORE_OPEN_EXISTING_FLAG;
        return _open(CERT_STORE_PROV_SYSTEM, PKCS_7_ASN_ENCODING | X509_ASN_ENCODING, NULL, dwFlags, store_name.c_str());
    }

    static certificate_store::ptr open_memory_store(){
        return _open(CERT_STORE_PROV_MEMORY, 0, NULL, 0, NULL);
    }
    virtual certificate::ptr find_certificate_in_store(__in DWORD dwCertEncodingType, __in DWORD dwFindFlags, __in DWORD dwFindType, __in_opt const void *pvFindPara, __in  PCCERT_CONTEXT pPrevCertContext = NULL);
    virtual certificate::vtr certificates();
    bool   import_pfx_file(__in boost::filesystem::path pfx_file_path, __in const std::wstring password, __in_opt DWORD flags = (CRYPT_EXPORTABLE | CRYPT_USER_PROTECTED));
    bool   import_pfx_blob(PCRYPT_DATA_BLOB pBlob, __in const std::wstring password, __in_opt DWORD flags = (CRYPT_EXPORTABLE | CRYPT_USER_PROTECTED));
    bool   add_certificate(certificate& cert, DWORD disposition = CERT_STORE_ADD_NEW);
    bool   remove_certificate(certificate& cert);
    virtual ~certificate_store(){
        close();
    }
private:
    static BOOL _get_prog_and_publisher_info(PCMSG_SIGNER_INFO pSignerInfo, std::wstring& program_name, std::wstring& publisher_link, std::wstring& more_info_link);
    static BOOL _get_date_of_time_stamp(PCMSG_SIGNER_INFO pSignerInfo, SYSTEMTIME *st);
    static BOOL _get_time_stamp_signer_info(PCMSG_SIGNER_INFO pSignerInfo, PCMSG_SIGNER_INFO *pCounterSignerInfo);

    virtual void close();
    static certificate_store::ptr _open(__in  LPCSTR lpszStoreProvider, __in  DWORD dwMsgAndCertEncodingType, __in  HCRYPTPROV_LEGACY hCryptProv, __in  DWORD dwFlags, __in  const void *pvPara);
    certificate_store() : _hCertStore(NULL){}
    HCERTSTORE _hCertStore;
};



#ifndef MACHO_HEADER_ONLY
#include <stdio.h>  
#include <tchar.h>
#include "..\common\stringutils.hpp"
#include "..\common\bytes.hpp"
#include "..\windows\environment.hpp"
#include "..\windows\auto_handle_base.hpp"

#define MY_ENCODING_TYPE  (PKCS_7_ASN_ENCODING | X509_ASN_ENCODING)

SECURITY_STATUS certificate::_ncrypt_free_object(__in  NCRYPT_HANDLE object){
    typedef SECURITY_STATUS(WINAPI *LPFN_NCryptFreeObject)(__in  NCRYPT_HANDLE hObject);
    LPFN_NCryptFreeObject fnNCryptFreeObject;
    fnNCryptFreeObject = (LPFN_NCryptFreeObject)GetProcAddress(
        GetModuleHandle(TEXT("Ncrypt.dll")), "NCryptFreeObject");
    if (NULL != fnNCryptFreeObject)
        return fnNCryptFreeObject(object);
    else
        return NTE_INVALID_HANDLE;
}

int certificate::_get_name_string(__in DWORD type, __out std::wstring& name, __in_opt DWORD flags, __in_opt void *pv_type_para){
    int rc = NO_ERROR;
    size_t size = 0;
    if ((size = CertGetNameStringW(_cert_context, type, flags, pv_type_para, NULL, 0)) >0){
        //OK
        std::auto_ptr<WCHAR> buffer = std::auto_ptr<WCHAR>(new WCHAR[size]);
        memset(buffer.get(), 0, sizeof(WCHAR) * size);
        if (CertGetNameStringW(_cert_context, type, flags, pv_type_para, buffer.get(), size) > 0) {
            name = buffer.get();
        }
        else {
            rc = GetLastError();
        }
    }
    else {
        rc = GetLastError();
    }
    return rc;
}

std::wstring const certificate::serial_number(){
    std::wstring serialnumber;
    size_t dwData = _cert_context->pCertInfo->SerialNumber.cbData;
    size_t bsize = dwData * 2 + 1;
    WCHAR* buffer = new WCHAR[bsize];
    memset(buffer, 0, bsize);
    for (int n = 0; n < (int)dwData; n++) {
        _swprintf(&buffer[n * 2], L"%02x", _cert_context->pCertInfo->SerialNumber.pbData[dwData - (n + 1)]);
    }
    serialnumber = buffer;
    delete[] buffer;
    return serialnumber;
}

std::wstring const certificate::signature_algorithm(){
    return stringutils::convert_ansi_to_unicode(_cert_context->pCertInfo->SignatureAlgorithm.pszObjId);
}

std::wstring const certificate::public_key_algorithm(){
    return stringutils::convert_ansi_to_unicode(_cert_context->pCertInfo->SubjectPublicKeyInfo.Algorithm.pszObjId);
}

DWORD                       certificate::_property(DWORD prop_id, bytes& data){
    int rc = NO_ERROR;
    DWORD cbData = 0;
    if (CertGetCertificateContextProperty(_cert_context, prop_id, NULL, &cbData)) {
        std::auto_ptr<BYTE> pvData = std::auto_ptr<BYTE>(new BYTE[cbData]);
        if (pvData.get() != NULL){
            memset(pvData.get(), 0, cbData);
            if (CertGetCertificateContextProperty(_cert_context, prop_id, pvData.get(), &cbData)) {
                //OK
                data.set(pvData.get(), cbData);
            }
            else {
                rc = GetLastError();
            }
        }
    }
    else {
        rc = GetLastError();
    }
    return rc;
}

std::vector<std::wstring>	certificate::enhanced_key_usages(){
    boost::shared_ptr<CERT_ENHKEY_USAGE> pusage;
    std::vector<std::wstring>       usages;
    DWORD			   cbUsage = 0;
    if (CertGetEnhancedKeyUsage(_cert_context, 0, pusage.get(), &cbUsage)){
        pusage = boost::shared_ptr<CERT_ENHKEY_USAGE>((PCERT_ENHKEY_USAGE) new BYTE[cbUsage]);
        if (pusage){
            if (CertGetEnhancedKeyUsage(_cert_context, 0, pusage.get(), &cbUsage)){
                for (int i = 0; i < (int)pusage->cUsageIdentifier; i++){
                    usages.push_back(stringutils::convert_ansi_to_unicode(pusage->rgpszUsageIdentifier[i]));
                }
            }
        }
    }
    return usages;
}

bool	                    certificate::is_support_email_protection(){
    std::vector<std::wstring> usages = enhanced_key_usages();
    std::wstring wsz_support_email_protection_id = stringutils::convert_ansi_to_unicode(szOID_PKIX_KP_EMAIL_PROTECTION);
    foreach(std::wstring& usage, usages){
        if (usage == wsz_support_email_protection_id)
            return true;
    }
    return false;
}

bool                        certificate::has_private_key(){
    DWORD dwRet = ERROR_SUCCESS;
    bool hasPrivateKey = false;
#if 1
    hasPrivateKey = (0 == _property(CERT_KEY_PROV_INFO_PROP_ID));
#else
    HCRYPTPROV hProv = NULL;
    DWORD dwKeySpec = 0;
    BOOL bCallerFreeProv = FALSE;
    if (CryptAcquireCertificatePrivateKey(_cert_context, 0, NULL, &hProv, &dwKeySpec, &bCallerFreeProv)){
        hasPrivateKey = true;
        if (bCallerFreeProv){
            if (CERT_NCRYPT_KEY_SPEC == dwKeySpec){
                _NCryptFreeObject(hProv);
            }
            else{
                CryptReleaseContext(hProv, 0);
            }
        }
    }
    else{
        dwRet = GetLastError();
    }
#endif
    return hasPrivateKey;
}

DWORD                       certificate::acquire_certificate_private_key(HCRYPTPROV* phCryptProv, DWORD* pKeySpec){
    int rc = NO_ERROR;
    HCRYPTPROV hProv = NULL;
    DWORD dwSpec = 0;
    if (!(CryptAcquireCertificatePrivateKey(_cert_context, 0, NULL, &hProv, &dwSpec, NULL))) {
        *phCryptProv = hProv;
        *pKeySpec = dwSpec;
    }
    else {
        rc = GetLastError();
    }
    return rc;
}

certificate::vtr            certificate::certificate_chain(){
    certificate::vtr		 tbCertificates;
    BOOL					 ret = TRUE;
    HCERTCHAINENGINE		 engine = NULL;
    CERT_CHAIN_ENGINE_CONFIG config;
    memset(&config, 0, sizeof(config));
    config.cbSize = sizeof(CERT_CHAIN_ENGINE_CONFIG);
    config.hRestrictedRoot = NULL;
    config.hRestrictedTrust = NULL;
    config.hRestrictedOther = NULL;
    config.cAdditionalStore = 0;
    config.rghAdditionalStore = NULL;
    config.dwFlags = CERT_CHAIN_CACHE_END_CERT;
    config.dwUrlRetrievalTimeout = 0;
    config.MaximumCachedCertificates = 0;
    config.CycleDetectionModulus = 0;
    if (!(ret = CertCreateCertificateChainEngine(&config, &engine))){
        LOG(LOG_LEVEL_ERROR, _T("Failed to CertCreateCertificateChainEngine. 0x%x"), GetLastError());
    }
    else{

        CERT_CHAIN_PARA chainPara;
        PCCERT_CHAIN_CONTEXT chain;
        memset(&chainPara, 0, sizeof(chainPara));
        chainPara.cbSize = sizeof(chainPara);
        if (ret = CertGetCertificateChain(engine, _cert_context, NULL, NULL, &chainPara, 0, NULL, &chain)){
            DWORD i, j;
            for (i = 0; ret && i < chain->cChain; i++){
                for (j = 0; ret && j < chain->rgpChain[i]->cElement; j++){
                    tbCertificates.push_back(certificate::ptr(new certificate(chain->rgpChain[i]->rgpElement[j]->pCertContext)));
                }
            }
            CertFreeCertificateChain(chain);
        }
    }
    if (engine)
        CertFreeCertificateChainEngine(engine);

    return tbCertificates;
}

bool                        certificate::export_cert_der(bytes &data){
    data.set(_cert_context->pbCertEncoded, _cert_context->cbCertEncoded);
    return true;
}

bool                        certificate::export_cert_der_file(__in boost::filesystem::path cer_file_path, bool base64){
    BOOL    ret = TRUE;
    auto_file_handle hFile = CreateFileW(cer_file_path.wstring().c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile.is_valid()) {
        DWORD	writtenBytes = 0;
        DWORD   size = 0;
        if (base64){
            if ((ret = CryptBinaryToStringA(_cert_context->pbCertEncoded, _cert_context->cbCertEncoded, CRYPT_STRING_BASE64, NULL, &size))){
                LPSTR buf = (LPSTR)HeapAlloc(GetProcessHeap(), 0, size);
                if (buf){
                    if (ret = CryptBinaryToStringA(_cert_context->pbCertEncoded, _cert_context->cbCertEncoded, CRYPT_STRING_BASE64, buf, &size)){
                        if (ret = WriteFile(hFile, buf, size, &size, NULL)){
                            LOG(LOG_LEVEL_INFO, _T("Written to a cert file(%s)."), cer_file_path.wstring().c_str());
                        }
                    }
                    HeapFree(GetProcessHeap(), 0, buf);
                }
                else{
                    SetLastError(ERROR_OUTOFMEMORY);
                    ret = FALSE;
                }
            }
        }
        else{
            if (WriteFile(hFile, _cert_context->pbCertEncoded, _cert_context->cbCertEncoded, &writtenBytes, NULL)) {
                LOG(LOG_LEVEL_INFO, _T("Written to a cert file(%s)."), cer_file_path.wstring().c_str());
            }
        }
    }
    else {
        LOG(LOG_LEVEL_ERROR, _T("Failed to open a file(%s) : (0x%x)"), cer_file_path.wstring().c_str(), GetLastError());
    }
    return ret == TRUE;
}

bool                        certificate::export_to_pfx_file(__in boost::filesystem::path  pfx_file_path, __in std::wstring password, __in bool exportchain){
    bool result = false;
    int		rc = NO_ERROR;
    CRYPT_DATA_BLOB data;
    memset(&data, 0, sizeof(data));
    data.pbData = NULL;
    if (result = export_to_pfx_blob(data, password, exportchain)){
        auto_file_handle hFile = CreateFileW(pfx_file_path.wstring().c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile.is_valid()) {
            DWORD writtenBytes = 0;
            if (WriteFile(hFile, data.pbData, data.cbData, &writtenBytes, NULL)) {
                LOG(LOG_LEVEL_INFO, _T("Written to a pfx file(%s)."), pfx_file_path.wstring().c_str());
            }
            else {
                rc = GetLastError();
                LOG(LOG_LEVEL_ERROR, _T("Failed to write a file(%s) : (0x%x)"), pfx_file_path.wstring().c_str(), rc);
            }
        }
        else {
            rc = GetLastError();
            LOG(LOG_LEVEL_ERROR, _T("Failed to open a file(%s) : (0x%x)"), pfx_file_path.wstring().c_str(), rc);
        }
        CryptMemFree(data.pbData);
        result = NO_ERROR == rc;
    }
    return result;
}

bool                        certificate::export_to_pfx_blob(CRYPT_DATA_BLOB &blob, __in std::wstring password, __in bool exportchain){
    int		rc = NO_ERROR;
    DWORD	cbData = 0;
    BOOL	ret = TRUE;
    DWORD	flags = EXPORT_PRIVATE_KEYS | REPORT_NOT_ABLE_TO_EXPORT_PRIVATE_KEY;
    HCERTSTORE hMemoryStore = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, NULL, 0, NULL);
    if (hMemoryStore == NULL) {
        return false;// GetLastError();
    }
    operating_system os = environment::get_os_version();;
    if (os.is_winvista_or_later()) {
        flags |= PKCS12_INCLUDE_EXTENDED_PROPERTIES;
    }

    if (exportchain){
        HCERTCHAINENGINE engine = NULL;
        CERT_CHAIN_ENGINE_CONFIG config;
        memset(&config, 0, sizeof(config));
        config.cbSize = sizeof(config);
        config.cAdditionalStore = 0;
        config.rghAdditionalStore = NULL;
        if (!(ret = CertCreateCertificateChainEngine(&config, &engine))){
            LOG(LOG_LEVEL_ERROR, _T("CertCreateCertificateChainEngine Failed."));
        }
        else{
            CERT_CHAIN_PARA chainPara;
            PCCERT_CHAIN_CONTEXT chain;
            memset(&chainPara, 0, sizeof(chainPara));
            chainPara.cbSize = sizeof(chainPara);
            if (ret = CertGetCertificateChain(engine, _cert_context, NULL, NULL, &chainPara, 0, NULL, &chain)){
                DWORD i, j;
                for (i = 0; ret && i < chain->cChain; i++){
                    for (j = 0; ret && j < chain->rgpChain[i]->cElement; j++){
                        ret = CertAddCertificateContextToStore(hMemoryStore, chain->rgpChain[i]->rgpElement[j]->pCertContext, CERT_STORE_ADD_ALWAYS, NULL);
                    }
                }
                CertFreeCertificateChain(chain);
            }
        }
        if (engine)
            CertFreeCertificateChainEngine(engine);
    }
    else{
        if (!(ret = CertAddCertificateContextToStore(hMemoryStore, _cert_context, CERT_STORE_ADD_ALWAYS, NULL))) {
            rc = GetLastError();
            LOG(LOG_LEVEL_ERROR, _T("CertAddCertificateContextToStore(hMemoryStore) Failed. (0x%x)"), rc);
        }
    }

    if (ret){
        if (blob.pbData){
            CryptMemFree(blob.pbData);
        }

        if (!PFXExportCertStoreEx(hMemoryStore, &blob, password.c_str(), NULL, flags)) {
            rc = GetLastError();
            LOG(LOG_LEVEL_ERROR, _T("PFXExportCertStoreEx(1) Failed: 0x%x"), rc);
        }

        blob.pbData = (LPBYTE)CryptMemAlloc(blob.cbData);
        if (!PFXExportCertStoreEx(hMemoryStore, &blob, password.c_str(), NULL, flags)){
            rc = GetLastError();
            LOG(LOG_LEVEL_ERROR, _T("PFXExportCertStoreEx(2) Failed: 0x%x"), rc);
            CryptMemFree(blob.pbData);
        }
    }
    CertCloseStore(hMemoryStore, CERT_CLOSE_STORE_CHECK_FLAG);
    return rc == NO_ERROR;
}

void certificate_store::close() {
    if (_hCertStore) {
        if (!CertCloseStore(_hCertStore, 0)){
            OutputDebugString(_T("Failed CertCloseStore\n"));
        }
        else{
            _hCertStore = NULL;
        }
    }
}

certificate_store::ptr certificate_store::_open(__in  LPCSTR lpszStoreProvider, __in  DWORD dwMsgAndCertEncodingType, __in  HCRYPTPROV_LEGACY hCryptProv, __in  DWORD dwFlags, __in  const void *pvPara){
    certificate_store::ptr cert(new certificate_store());
    if (cert->_hCertStore = CertOpenStore(lpszStoreProvider, dwMsgAndCertEncodingType, hCryptProv, dwFlags, pvPara)){
        return cert;
    }
    return NULL;
}

certificate::ptr certificate_store::find_certificate_in_store(__in DWORD dwCertEncodingType, __in DWORD dwFindFlags, __in DWORD dwFindType, __in_opt const void *pvFindPara, __in  PCCERT_CONTEXT pPrevCertContext){
    PCCERT_CONTEXT     pCertContext = NULL;
    if (pCertContext = CertFindCertificateInStore(_hCertStore, dwCertEncodingType, dwFindFlags, dwFindType, pvFindPara, pPrevCertContext)){
        return certificate::ptr(new certificate(pCertContext));
    }
    return NULL;
}

certificate::vtr certificate_store::certificates(){
    PCCERT_CONTEXT     pCertContext = NULL;
    certificate::vtr  tbCerts;
    while (pCertContext = CertEnumCertificatesInStore(
        _hCertStore,
        pCertContext))
    {
        tbCerts.push_back(certificate::ptr(new certificate(pCertContext)));
    }
    return tbCerts;
}

bool   certificate_store::import_pfx_file(__in boost::filesystem::path pfx_file_path, __in const std::wstring password, __in_opt DWORD flags){
    int rc = NO_ERROR;
    bool result = false;
    if ((!pfx_file_path.empty()) && boost::filesystem::exists(pfx_file_path)){
        CRYPT_DATA_BLOB data;
        memset(&data, 0, sizeof(data));
        auto_file_handle hFile = CreateFileW(pfx_file_path.wstring().c_str(),
            GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile.is_valid()) {
            data.cbData = GetFileSize(hFile, NULL);
            data.pbData = (LPBYTE)CryptMemAlloc(data.cbData);
            DWORD readBytes = 0;
            if (ReadFile(hFile, data.pbData, data.cbData, &readBytes, NULL)) {
                result = import_pfx_blob(&data, password, flags);
            }
            else{
                LOG(LOG_LEVEL_ERROR, _T("Failed to read the file(%s) : (0x%x)"), pfx_file_path.wstring().c_str(), GetLastError());
            }
            if (data.pbData) {
                CryptMemFree(data.pbData);
            }
        }
        else{
            LOG(LOG_LEVEL_ERROR, _T("Failed to open the file(%s) : (0x%x)"), pfx_file_path.wstring().c_str(), GetLastError());
        }
    }
    else{
        if( pfx_file_path.empty())
            LOG(LOG_LEVEL_ERROR, _T("pfx_file_path is empty."));
        else
            LOG(LOG_LEVEL_ERROR, _T("%s doesn't exist."), pfx_file_path.wstring().c_str());
    }
    return result;
}

bool   certificate_store::import_pfx_blob(PCRYPT_DATA_BLOB pBlob, __in const std::wstring password, __in_opt DWORD flags){
    int rc = NO_ERROR;
    if (!pBlob){
        LOG(LOG_LEVEL_ERROR, _T("pBlob is null."));
    }
    else if (!PFXIsPFXBlob(pBlob)) {
        LOG(LOG_LEVEL_ERROR, _T("PFXIsPFXBlob Failed: 0x%x"), GetLastError());
    }
    else if (!PFXVerifyPassword(pBlob, password.c_str(), 0)) {
        LOG(LOG_LEVEL_ERROR, _T("PFXVerifyPassword Failed: 0x%x"), GetLastError());
    }
    else {
        //Import a pfx blob and create a temporaray memory-based certstore
        HCERTSTORE  hMemoryStore = PFXImportCertStore(pBlob, password.c_str(), flags);
        if (hMemoryStore != NULL) {
            PCCERT_CONTEXT  pContext = NULL;
            while ((pContext = CertEnumCertificatesInStore(hMemoryStore, pContext))) {
                CertAddCertificateContextToStore(_hCertStore, pContext, CERT_STORE_ADD_ALWAYS, NULL);
            }
            CertCloseStore(hMemoryStore, 0);
            return true;
        }
        else {
            LOG(LOG_LEVEL_ERROR, _T("PFXImportCertStore Failed: 0x%x"), GetLastError());
        }
    }
    return false;
}

#define ENCODING (X509_ASN_ENCODING | PKCS_7_ASN_ENCODING)

BOOL certificate_store::_get_prog_and_publisher_info(PCMSG_SIGNER_INFO pSignerInfo, std::wstring& program_name, std::wstring& publisher_link, std::wstring& more_info_link){
    BOOL fReturn = FALSE;
    PSPC_SP_OPUS_INFO OpusInfo = NULL;
    DWORD dwData;
    BOOL fResult;
    // Loop through authenticated attributes and find
    // SPC_SP_OPUS_INFO_OBJID OID.
    for (DWORD n = 0; n < pSignerInfo->AuthAttrs.cAttr; n++)
    {
        if (lstrcmpA(SPC_SP_OPUS_INFO_OBJID,
            pSignerInfo->AuthAttrs.rgAttr[n].pszObjId) == 0)
        {
            // Get Size of SPC_SP_OPUS_INFO structure.
            fResult = CryptDecodeObject(ENCODING,
                SPC_SP_OPUS_INFO_OBJID,
                pSignerInfo->AuthAttrs.rgAttr[n].rgValue[0].pbData,
                pSignerInfo->AuthAttrs.rgAttr[n].rgValue[0].cbData,
                0,
                NULL,
                &dwData);
            if (!fResult)
            {
                LOG( LOG_LEVEL_ERROR, _T("CryptDecodeObject failed with %x"), GetLastError());
                break;
            }
            else{
                // Allocate memory for SPC_SP_OPUS_INFO structure.
                OpusInfo = (PSPC_SP_OPUS_INFO)LocalAlloc(LPTR, dwData);
                if (!OpusInfo)
                {
                    LOG(LOG_LEVEL_ERROR, _T("Unable to allocate memory for Publisher Info."));
                    break;
                }
                else{
                    // Decode and get SPC_SP_OPUS_INFO structure.
                    fResult = CryptDecodeObject(ENCODING,
                        SPC_SP_OPUS_INFO_OBJID,
                        pSignerInfo->AuthAttrs.rgAttr[n].rgValue[0].pbData,
                        pSignerInfo->AuthAttrs.rgAttr[n].rgValue[0].cbData,
                        0,
                        OpusInfo,
                        &dwData);
                    if (!fResult)
                    {
                        LOG(LOG_LEVEL_ERROR, _T("CryptDecodeObject failed with %x"), GetLastError());
                        break;
                    }
                    else{
                        // Fill in Program Name if present.
                        if (OpusInfo->pwszProgramName)
                        {
                            program_name = OpusInfo->pwszProgramName;
                        }

                        // Fill in Publisher Information if present.
                        if (OpusInfo->pPublisherInfo)
                        {

                            switch (OpusInfo->pPublisherInfo->dwLinkChoice)
                            {
                            case SPC_URL_LINK_CHOICE:
                                publisher_link = OpusInfo->pPublisherInfo->pwszUrl;
                                break;

                            case SPC_FILE_LINK_CHOICE:
                                publisher_link = OpusInfo->pPublisherInfo->pwszFile;
                                break;
                            default:
                                break;
                            }
                        }

                        // Fill in More Info if present.
                        if (OpusInfo->pMoreInfo)
                        {
                            switch (OpusInfo->pMoreInfo->dwLinkChoice)
                            {
                            case SPC_URL_LINK_CHOICE:
                                more_info_link = OpusInfo->pMoreInfo->pwszUrl;
                                break;

                            case SPC_FILE_LINK_CHOICE:
                                more_info_link = OpusInfo->pMoreInfo->pwszFile;
                                break;

                            default:
                                break;
                            }
                        }
                        fReturn = TRUE;
                        break; // Break from for loop.
                    }
                }
            }
        } // lstrcmp SPC_SP_OPUS_INFO_OBJID                 
    } // for 

    if (OpusInfo != NULL) LocalFree(OpusInfo);
    return fReturn;
}

BOOL certificate_store::_get_date_of_time_stamp(PCMSG_SIGNER_INFO pSignerInfo, SYSTEMTIME *st){
    BOOL fResult;
    FILETIME lft, ft;
    DWORD dwData;
    BOOL fReturn = FALSE;

    // Loop through authenticated attributes and find
    // szOID_RSA_signingTime OID.
    for (DWORD n = 0; n < pSignerInfo->AuthAttrs.cAttr; n++)
    {
        if (lstrcmpA(szOID_RSA_signingTime,
            pSignerInfo->AuthAttrs.rgAttr[n].pszObjId) == 0)
        {
            // Decode and get FILETIME structure.
            dwData = sizeof(ft);
            fResult = CryptDecodeObject(ENCODING,
                szOID_RSA_signingTime,
                pSignerInfo->AuthAttrs.rgAttr[n].rgValue[0].pbData,
                pSignerInfo->AuthAttrs.rgAttr[n].rgValue[0].cbData,
                0,
                (PVOID)&ft,
                &dwData);
            if (!fResult)
            {
                LOG(LOG_LEVEL_ERROR, _T("CryptDecodeObject failed with %x"), GetLastError());
                break;
            }

            // Convert to local time.
            FileTimeToLocalFileTime(&ft, &lft);
            FileTimeToSystemTime(&lft, st);

            fReturn = TRUE;

            break; // Break from for loop.

        } //lstrcmp szOID_RSA_signingTime
    } // for 

    return fReturn;
}

BOOL certificate_store::_get_time_stamp_signer_info(PCMSG_SIGNER_INFO pSignerInfo, PCMSG_SIGNER_INFO *pCounterSignerInfo){
    PCCERT_CONTEXT pCertContext = NULL;
    BOOL fReturn = FALSE;
    BOOL fResult;
    DWORD dwSize;

    *pCounterSignerInfo = NULL;

    // Loop through unathenticated attributes for
    // szOID_RSA_counterSign OID.
    for (DWORD n = 0; n < pSignerInfo->UnauthAttrs.cAttr; n++)
    {
        if (lstrcmpA(pSignerInfo->UnauthAttrs.rgAttr[n].pszObjId,
            szOID_RSA_counterSign) == 0)
        {
            // Get size of CMSG_SIGNER_INFO structure.
            fResult = CryptDecodeObject(ENCODING,
                PKCS7_SIGNER_INFO,
                pSignerInfo->UnauthAttrs.rgAttr[n].rgValue[0].pbData,
                pSignerInfo->UnauthAttrs.rgAttr[n].rgValue[0].cbData,
                0,
                NULL,
                &dwSize);
            if (!fResult)
            {
                LOG(LOG_LEVEL_ERROR, _T("CryptDecodeObject failed with %x"), GetLastError());
                break;
            }

            // Allocate memory for CMSG_SIGNER_INFO.
            *pCounterSignerInfo = (PCMSG_SIGNER_INFO)LocalAlloc(LPTR, dwSize);
            if (!*pCounterSignerInfo)
            {
                LOG(LOG_LEVEL_ERROR, _T("Unable to allocate memory for timestamp info."));
                break;
            }

            // Decode and get CMSG_SIGNER_INFO structure
            // for timestamp certificate.
            fResult = CryptDecodeObject(ENCODING,
                PKCS7_SIGNER_INFO,
                pSignerInfo->UnauthAttrs.rgAttr[n].rgValue[0].pbData,
                pSignerInfo->UnauthAttrs.rgAttr[n].rgValue[0].cbData,
                0,
                (PVOID)*pCounterSignerInfo,
                &dwSize);
            if (!fResult)
            {
                LOG(LOG_LEVEL_ERROR, _T("CryptDecodeObject failed with %x"), GetLastError());
                break;
            }

            fReturn = TRUE;

            break; // Break from for loop.
        }
    }
  
    // Clean up.
    if (pCertContext != NULL) CertFreeCertificateContext(pCertContext);
 
    return fReturn;
}

authenticode_signed_info::ptr certificate_store::get_authenticode_signed_info(const boost::filesystem::path file){
    BOOL fResult;
    DWORD dwEncoding, dwContentType, dwFormatType;
    DWORD dwSignerInfo;
    CERT_INFO CertInfo;
    HCRYPTMSG hMsg = NULL;
    certificate_store store;
    PCMSG_SIGNER_INFO pSignerInfo = NULL;
    PCMSG_SIGNER_INFO pCounterSignerInfo = NULL;
    authenticode_signed_info::ptr signed_executable(new authenticode_signed_info());
    // Get message handle and store handle from the signed file.
    fResult = CryptQueryObject(CERT_QUERY_OBJECT_FILE,
        file.wstring().c_str(),
        CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED_EMBED,
        CERT_QUERY_FORMAT_FLAG_BINARY,
        0,
        &dwEncoding,
        &dwContentType,
        &dwFormatType,
        &store._hCertStore,
        &hMsg,
        NULL);
    if (!fResult)
    {
        LOG(LOG_LEVEL_ERROR, _T("CryptQueryObject failed with %x\n"), GetLastError());
        return NULL;
    }
    // Get signer information size.
    fResult = CryptMsgGetParam(hMsg,
        CMSG_SIGNER_INFO_PARAM,
        0,
        NULL,
        &dwSignerInfo);
    if (!fResult)
    {
        LOG(LOG_LEVEL_ERROR, _T("CryptMsgGetParam failed with %x\n"), GetLastError());
        signed_executable = NULL;
    }
    else{
        // Allocate memory for signer information.
        pSignerInfo = (PCMSG_SIGNER_INFO)LocalAlloc(LPTR, dwSignerInfo);
        if (!pSignerInfo)
        {
            LOG(LOG_LEVEL_ERROR, _T("Unable to allocate memory for Signer Info.\n"));
            signed_executable = NULL;
        }
        else{

            // Get Signer Information.
            fResult = CryptMsgGetParam(hMsg,
                CMSG_SIGNER_INFO_PARAM,
                0,
                (PVOID)pSignerInfo,
                &dwSignerInfo);
            if (!fResult)
            {
                LOG(LOG_LEVEL_ERROR, _T("CryptMsgGetParam failed with %x\n"), GetLastError());
                signed_executable = NULL;
            }
            else{
                _get_prog_and_publisher_info(pSignerInfo, signed_executable->program_name, signed_executable->publisher_link, signed_executable->more_info_link);
                CertInfo.Issuer = pSignerInfo->Issuer;
                CertInfo.SerialNumber = pSignerInfo->SerialNumber;
                signed_executable->signer_certificate = store.find_certificate_in_store(ENCODING,
                    0,
                    CERT_FIND_SUBJECT_CERT,
                    (PVOID)&CertInfo,
                    NULL);
                if (_get_time_stamp_signer_info(pSignerInfo, &pCounterSignerInfo)){
                    CertInfo.Issuer = pCounterSignerInfo->Issuer;
                    CertInfo.SerialNumber = pCounterSignerInfo->SerialNumber;
                    signed_executable->timestamp_certificate = store.find_certificate_in_store(ENCODING,
                        0,
                        CERT_FIND_SUBJECT_CERT,
                        (PVOID)&CertInfo,
                        NULL);
                    _get_date_of_time_stamp(pCounterSignerInfo, &signed_executable->st);
                }
            }
        }
    }
    if (pSignerInfo != NULL) LocalFree(pSignerInfo);
    if (pCounterSignerInfo != NULL) LocalFree(pCounterSignerInfo);
    if (hMsg != NULL) CryptMsgClose(hMsg);

    return signed_executable;
}

bool   certificate_store::add_certificate(certificate& cert, DWORD disposition){
    return TRUE == CertAddCertificateContextToStore(_hCertStore, cert.get(), disposition, NULL);
}

bool   certificate_store::remove_certificate(certificate& cert){
    unsigned        cDeleted = 0;
    PCCERT_CONTEXT  pCurCtx = NULL;
    while ((pCurCtx = CertEnumCertificatesInStore(_hCertStore, pCurCtx)) != NULL)
    {
        if (CertCompareCertificate(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, pCurCtx->pCertInfo, cert.get()->pCertInfo))
        {
            PCCERT_CONTEXT pDeleteCtx = CertDuplicateCertificateContext(pCurCtx);
            if (pDeleteCtx)
            {
                if (CertDeleteCertificateFromStore(pDeleteCtx))
                    cDeleted++;
                else
                    LOG(LOG_LEVEL_ERROR, _T("CertDeleteFromStore (%s) failed: 0x%x"), cert.friendly_name().c_str(), GetLastError());
            }
            else
                LOG(LOG_LEVEL_ERROR, _T("CertDuplicateCertificateContext failed: 0x%x"), GetLastError());
        }
    }

    if (!cDeleted)
        LOG(LOG_LEVEL_INFO, _T("Found no matching certificates to remove."));
    return true;
}

#endif

};//namespace windows
};//namespace macho

#endif