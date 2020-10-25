
#include "vmware.h"
#include "logging.h"
#include <sstream>
#include <boost/asio/ip/address.hpp>

using namespace macho;
using namespace boost;
using namespace mwdc::ironman::hypervisor;

#define BOOST_THROW_VMWARE_EXCEPTION( no, message ) BOOST_THROW_EXCEPTION_BASE( vmware_portal::exception, no, message )
#define BOOST_THROW_VMWARE_EXCEPTION_STRING(message) BOOST_THROW_EXCEPTION_BASE_STRING(vmware_portal::exception, message )
static bool xsd_true = 1;
static bool xsd_false = 0;
static const long DEFAULT_CAPACITY;
static const long DEFAULT_NUM_PORTS_IN_VSWITCH;
static const wchar_t* DEFAULT_NAME_OF_VSWITCH;

bool is_true_value(xsd__anyType* value){
    if (value &&
        SOAP_TYPE_xsd__string == value->soap_type() &&
        0 == _wcsicmp(L"true", reinterpret_cast< xsd__string* >(value)->__item.c_str()))
        return true;
    else
        return false;
}

bool net_is_address_format(TCHAR * ptcIPAddress){
    int            v1, v2, v3, v4, ret;
    ret = _stscanf(ptcIPAddress, L"%d.%d.%d.%d", &v1, &v2, &v3, &v4);
    if (ret == 4) {
        if (!(v1 >= 1 && v1 <= 255))
            return false;
        if (!(v2 >= 0 && v2 <= 255))
            return false;
        if (!(v3 >= 0 && v3 <= 255))
            return false;
        if (!(v4 >= 0 && v4 <= 255))
            return false;
        return true;
    }
    return false;
}

std::wstring get_host_name(const std::wstring& fqdn, std::wstring& hostname){
    size_t pos = std::wstring::npos;
    if (!net_is_address_format((TCHAR*)fqdn.c_str()))
        pos = fqdn.find(L'.');
    if (std::wstring::npos == pos)
        hostname = fqdn;
    else
        hostname = fqdn.substr(0, pos);
    return hostname;
}

class mwdc::ironman::hypervisor::journal_transaction{
public:
    journal_transaction(vmware_portal* portal, bool bInBoundOnly = false)
        : _portal(portal)
        , _start(0)
        , _level(macho::get_log_level()){
        if (TRACE_LOG_LEVEL::LOG_LEVEL_TRACE == _level){
            soap_set_logging_inbound(&_portal->_vim_binding, &_ss);
            soap_set_logging_outbound(&_portal->_vim_binding, bInBoundOnly ? NULL : &_ss);
        }
        time(&_start);
    }

    virtual ~journal_transaction(){
        time_t now = 0;
        time(&now);
        if (TRACE_LOG_LEVEL::LOG_LEVEL_TRACE == _level){
            soap_set_logging_inbound(&_portal->_vim_binding, NULL);
            soap_set_logging_outbound(&_portal->_vim_binding, NULL);
            LOG(LOG_LEVEL_TRACE, _ss.str().c_str());
        }
        LOG(LOG_LEVEL_INFO, L"Total operation time (%d) seconds.", now - _start);
    }

private:
    TRACE_LOG_LEVEL _level;
    vmware_portal* _portal;
    std::stringstream _ss;
    time_t _start;
};

service_connect::service_connect(vmware_portal *portal) : _portal(portal), _service_content(NULL){
    FUN_TRACE;
    _service_content = portal->get_service_content(portal->_uri);
    if (_service_content){
        if (!portal->login(_service_content, portal->_username, portal->_password)){
            _service_content = NULL;
        }
    }
}
service_connect::~service_connect(){ _portal->logout(_service_content); }
service_connect::operator Vim25Api1__ServiceContent*(){ return (Vim25Api1__ServiceContent *)_service_content; }

vmware_portal::ptr vmware_portal::connect(std::wstring uri, std::wstring username, std::wstring password){
    FUN_TRACE;
    try
    {
        return vmware_portal::ptr(new vmware_portal(uri, username, password));
    }
    catch (macho::exception_base& ex){
        LOG(LOG_LEVEL_ERROR, L"%s", macho::get_diagnostic_information(ex).c_str());
        return NULL;
    }
    catch (boost::exception &e)
    {
        std::string msg = boost::diagnostic_information(e);
        LOG(LOG_LEVEL_ERROR, L"%s.", macho::stringutils::convert_ansi_to_unicode(msg).c_str());
        return NULL;
    }
}

bool vmware_portal::login_verify(std::wstring uri, std::wstring username, std::wstring password){
    FUN_TRACE;
    try{
        vmware_portal p(uri, username, password);
    }
    catch(...){
        return false;
    }
    return true;
}

vmware_portal::vmware_portal(std::wstring uri, std::wstring username, std::wstring password) {
    FUN_TRACE;
    _uri = uri;
    _username = username;
    _password = password;
    soap_init(&_vim_binding);
    soap_register_plugin(&_vim_binding, logging);
    _connect = service_connect::ptr(new service_connect(this));
}

void vmware_portal::get_native_error(int &error, std::wstring &message){

    FUN_TRACE;
    error = VMWARE_ERROR_GENERALERROR;
    message = L"";

    if (_vim_binding.fault && _vim_binding.fault->detail){
        if (_vim_binding.fault->detail->__any){
            char* fault = _vim_binding.fault->detail->__any;
            if (strstr(fault, "InvalidLogin"))
                error = VMWARE_ERROR_InvalidLogin;
            else if (strstr(fault, "HostConnect") || strstr(fault, "connect failed"))
                error = VMWARE_ERROR_HostConnectFault;
        }
        message = stringutils::convert_ansi_to_unicode(*soap_faultstring(&_vim_binding));
    }
    else {
        if (strnlen(_vim_binding.buf, 1024) < 1024) {
            if (_vim_binding.fault)
                message = stringutils::convert_ansi_to_unicode(*soap_faultstring(&_vim_binding));
            else
                message = stringutils::convert_ansi_to_unicode(boost::str(boost::format("endpoint:(%1%), error num:(%2%).") % std::string(_vim_binding.endpoint) % std::to_string(_vim_binding.errnum)));
        }
    }
    _last_error_code = error;
    _last_error_message = message;
}

Vim25Api1__ServiceContent* vmware_portal::get_service_content(std::wstring uri){

    FUN_TRACE;
    int                                     nReturn;
    Vim25Api1__ServiceContent*                    pServiceContent = NULL;
    Vim25Api1__ManagedObjectReference             MOR;
    Vim25Api1__RetrieveServiceContentRequestType  service_request;
    _Vim25Api1__RetrieveServiceContentResponse    service_response;
    std::wstring type(L"ServiceInstance");

    MOR.__item = L"ServiceInstance";
    MOR.type = &type;
    service_request._USCOREthis = &MOR;

    if (_endpoint.empty()){
        _endpoint = stringutils::convert_unicode_to_ansi(uri);
        _vim_binding.soap_endpoint = _endpoint.c_str();
    }

    soap_ssl_init();
    int lastErrorCode;
    std::wstring lastErrorMessage;

    if (nReturn = soap_ssl_client_context(
        &_vim_binding,
        SOAP_SSL_NO_AUTHENTICATION,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL)) {
        get_native_error(lastErrorCode, lastErrorMessage);
        BOOST_THROW_VMWARE_EXCEPTION(lastErrorCode, lastErrorMessage);
    }
    else {
        journal_transaction trans(this);
        if (SOAP_OK != (nReturn = _vim_binding.RetrieveServiceContent(&service_request, service_response))){
            get_native_error(lastErrorCode, lastErrorMessage);
            if (VMWARE_ERROR_HostConnectFault == lastErrorCode){
                lastErrorCode = HV_ERROR_FAILED_TO_CONNECT;
            }
            else{
                lastErrorCode = HV_ERROR_LOGIN_FAILED;
            }
            BOOST_THROW_VMWARE_EXCEPTION(lastErrorCode, lastErrorMessage);
        }
        else{
            pServiceContent = service_response.returnval;
        }
    }
    return pServiceContent;
}

bool vmware_portal::login(LPVOID service_content, std::wstring username, std::wstring password){

    FUN_TRACE;
    HRESULT hr = S_OK;
    int                         nReturn = 0;
    Vim25Api1__LoginRequestType       login_request;
    _Vim25Api1__LoginResponse         login_response;
    int lastErrorCode;
    std::wstring lastErrorMessage;
    login_request._USCOREthis = ((Vim25Api1__ServiceContent *)service_content)->sessionManager;
    login_request.userName = username.c_str();
    login_request.password = password.c_str();
    journal_transaction trans(this, true);
    if (SOAP_OK != (nReturn = _vim_binding.Login(&login_request, login_response))) {
        get_native_error(lastErrorCode, lastErrorMessage);
        if (VMWARE_ERROR_InvalidLogin == lastErrorCode){
            lastErrorCode = hr = HV_ERROR_INVALID_CREDENTIALS;
        }
        else{
            lastErrorCode = hr = HV_ERROR_LOGIN_FAILED;
        }
        BOOST_THROW_VMWARE_EXCEPTION(lastErrorCode, lastErrorMessage);
    }
    return true;
}

bool vmware_portal::logout(LPVOID service_content){
    
    FUN_TRACE;
    int                         nReturn = 0;
    Vim25Api1__LogoutRequestType      logout_request;
    _Vim25Api1__LogoutResponse        logout_response;
    int lastErrorCode;
    std::wstring lastErrorMessage;
    logout_request._USCOREthis = ((Vim25Api1__ServiceContent *)service_content)->sessionManager;
    journal_transaction trans(this);
    if (SOAP_OK != (nReturn = _vim_binding.Logout(&logout_request, logout_response))){
        get_native_error(lastErrorCode, lastErrorMessage);
    }
    return true;
}

hv_connection_type vmware_portal::get_connection_type(vmware_virtual_center &virtual_center){
    hv_connection_type                      type;
    get_connection_info(*(Vim25Api1__ServiceContent*)*_connect.get(), type, virtual_center);
    return type;
}

float vmware_portal::get_connection_info(Vim25Api1__ServiceContent& service_content, hv_connection_type &type, vmware_virtual_center &virtual_center){

    FUN_TRACE;
    int                                     nReturn = 0;
    Vim25Api1__ManagedObjectReference             MOR;
    Vim25Api1__PropertySpec                       propertySpec;
    Vim25Api1__ObjectSpec                         objectSpec;
    Vim25Api1__PropertyFilterSpec                 propertyFilterSpec;
    Vim25Api1__DynamicProperty                    dynamicProperty;
    Vim25Api1__RetrievePropertiesRequestType      retrievePropertiesReq;
    _Vim25Api1__RetrievePropertiesResponse        retrievePropertiesRes;

    Vim25Api1__LogoutRequestType      logout_request;
    _Vim25Api1__LogoutResponse        logout_response;
    float                       version = 0;
    std::wstring                _type(_T("ServiceInstance"));

    MOR.__item = _T("ServiceInstance");
    MOR.type = &_type;

    propertySpec.type = L"ServiceInstance";
    propertySpec.all = &xsd_false;
    propertySpec.pathSet.push_back(L"content.about");

    objectSpec.obj = &MOR;
    objectSpec.skip = &xsd_false;

    propertyFilterSpec.propSet.push_back(&propertySpec);
    propertyFilterSpec.objectSet.push_back(&objectSpec);

    retrievePropertiesReq._USCOREthis = service_content.propertyCollector;
    retrievePropertiesReq.specSet.push_back(&propertyFilterSpec);

    {
        journal_transaction trans(this);
        nReturn = _vim_binding.RetrieveProperties(&retrievePropertiesReq, retrievePropertiesRes);
    }

    if (SOAP_OK == nReturn) {
        for (size_t i = 0; i < retrievePropertiesRes.returnval.size(); i++){
            Vim25Api1__ObjectContent* pObjectContent = retrievePropertiesRes.returnval[i];
            for (size_t j = 0; j < pObjectContent->propSet.size(); j++)
            {
                Vim25Api1__DynamicProperty* pDynamicProperty = pObjectContent->propSet[j];
                if (pDynamicProperty->name == L"capability"){
                    /*
                    Vim25Api1__Capability* pCapability    = ( Vim25Api1__Capability * ) pDynamicProperty->val;
                    m_bMultiHostSupported           = pCapability->multiHostSupported;
                    m_bProvisioningSupported        = pCapability->provisioningSupported;
                    m_bUserShellAccessSupported     = pCapability->userShellAccessSupported;
                    */
                }
                else if (pDynamicProperty->name == L"content.about"){
                    Vim25Api1__AboutInfo* pAboutInfo = (Vim25Api1__AboutInfo *)pDynamicProperty->val;
                    if (0 == pAboutInfo->apiType.compare(L"HostAgent")) {
                        type = HV_CONNECTION_TYPE_HOST;
                    }
                    else if (0 == pAboutInfo->apiType.compare(L"VirtualCenter")){
                        type = HV_CONNECTION_TYPE_VCENTER;
                        virtual_center.product_name = pAboutInfo->fullName;
                        virtual_center.version = pAboutInfo->version;
                    }
                    else{
                        type = HV_CONNECTION_TYPE_UNKNOWN;
                    }

                    std::wstring wszAPIVersion = pAboutInfo->apiVersion;
                    if (std::wstring::npos != wszAPIVersion.find(L"2.5")){
                        version = 2.5f;
                    }
                    else if (std::wstring::npos != wszAPIVersion.find(L"4.0")){
                        version = 4.0f;
                    }
                    else if (std::wstring::npos != wszAPIVersion.find(L"4.1")){
                        version = 4.1f;
                    }
                    else if (std::wstring::npos != wszAPIVersion.find(L"5.0")){
                        version = 5.0f;
                    }
                    else if (std::wstring::npos != wszAPIVersion.find(L"5.1")){
                        version = 5.1f;
                    }
                    else if (std::wstring::npos != wszAPIVersion.find(L"5.5")){
                        version = 5.5f;
                    }
                    else if (std::wstring::npos != wszAPIVersion.find(L"6.0")){
                        version = 6.0f;
                    }
                    else{
                        version = (float)_tcstod(wszAPIVersion.c_str(), NULL);
                    }
                    LOG(LOG_LEVEL_INFO, L"%s - API version %f.", wszAPIVersion.c_str(), version);

                }
                else if (pDynamicProperty->name == L"serverClock"){
                    /*
                    xsd__dateTime* pServerClock     = ( xsd__dateTime * ) pDynamicProperty->val;
                    m_tServerClock                  = pServerClock->__item;
                    */
                }
            }
        }
    }
    else{
        int lastErrorCode;
        std::wstring lastErrorMessage;
        get_native_error(lastErrorCode, lastErrorMessage);
        BOOST_THROW_VMWARE_EXCEPTION(lastErrorCode, lastErrorMessage);
    }

    return version;
}

vmware_host::vtr vmware_portal::get_hosts_internal(Vim25Api1__ServiceContent& service_content, std::wstring host_key){

    FUN_TRACE;
    int                                      nReturn;
    Vim25Api1__ManagedObjectReference*       pVmMOR = NULL;
    Vim25Api1__TraversalSpec                 dataCenterTraversalSpec,
                                             computeResourceTraversalSpec,
                                             clusterComputeResourceTraversalSpec,
                                             folderTraversalSpec;
    Vim25Api1__SelectionSpec                 dataCenterSelectionSpec,
                                             computeResourceSelectionSpec,
                                             clusterComputeResourceSelectionSpec,
                                             folderSelectionSpec;
    Vim25Api1__PropertySpec                  propertySpec;
    Vim25Api1__ObjectSpec                    objectSpec;
    Vim25Api1__PropertyFilterSpec            propertyFilterSpec;
    Vim25Api1__DynamicProperty               dynamicProperty;
    Vim25Api1__RetrievePropertiesRequestType retrievePropertiesReq;
    _Vim25Api1__RetrievePropertiesResponse   retrievePropertiesRes;
    LPVOID                                   pProperty = NULL;
    vmware_cluster::vtr                      clusters;
    key_name::map                            vm_more_key_name_map;
    vmware_host::vtr                         hosts;
    float                                    version = 0;
    hv_connection_type                       type;
    vmware_virtual_center                    virtual_center;
    license_map	                             features;
    version = get_connection_info(service_content, type, virtual_center);
    features = get_supported_features(service_content);
    bool bCheckHost = (!host_key.empty() && HV_CONNECTION_TYPE_VCENTER == type);
    bool bKeyIsIp = (bCheckHost && net_is_address_format((LPTSTR)host_key.c_str()));
    key_name::map mapDatastoreMorKeyName;
    mor_name_map mapVirtualNetworkMorName, mapDataCenterMorName;
    mor_map      ancestor_mor_cache;
    get_datastore_mor_key_name_map(service_content, mapDatastoreMorKeyName);
    get_virtual_network_mor_key_name_map(service_content, mapVirtualNetworkMorName);
    clusters = get_clusters_internal(service_content, L"", true );
    get_vm_mor_key_name_map(service_content, vm_more_key_name_map);

    dataCenterSelectionSpec.name = soap_new_std__wstring(&_vim_binding, -1);
    folderSelectionSpec.name = soap_new_std__wstring(&_vim_binding, -1);
    computeResourceSelectionSpec.name = soap_new_std__wstring(&_vim_binding, -1);
    clusterComputeResourceSelectionSpec.name = soap_new_std__wstring(&_vim_binding, -1);
    dataCenterTraversalSpec.name = soap_new_std__wstring(&_vim_binding, -1);
    folderTraversalSpec.name = soap_new_std__wstring(&_vim_binding, -1);
    computeResourceTraversalSpec.name = soap_new_std__wstring(&_vim_binding, -1);
    clusterComputeResourceTraversalSpec.name = soap_new_std__wstring(&_vim_binding, -1);

    dataCenterSelectionSpec.name->assign(L"DataCenterTraversalSpec");
    folderSelectionSpec.name->assign(L"FolderTraversalSpec");
    computeResourceSelectionSpec.name->assign(L"ComputeResourceTraversalSpec");
    clusterComputeResourceSelectionSpec.name->assign(L"ClusterComputeResourceTraversalSpec");

    dataCenterTraversalSpec.name->assign(L"DataCenterTraversalSpec");
    dataCenterTraversalSpec.type = L"Datacenter";
    dataCenterTraversalSpec.path = L"hostFolder";
    dataCenterTraversalSpec.skip = &xsd_true;

    folderTraversalSpec.name->assign(L"FolderTraversalSpec");
    folderTraversalSpec.type = L"Folder";
    folderTraversalSpec.path = L"childEntity";
    folderTraversalSpec.skip = &xsd_true;

    computeResourceTraversalSpec.name->assign(L"ComputeResourceTraversalSpec");
    computeResourceTraversalSpec.type = L"ComputeResource";
    computeResourceTraversalSpec.path = L"host";
    computeResourceTraversalSpec.skip = &xsd_false;

    clusterComputeResourceTraversalSpec.name->assign(L"ClusterComputeResourceTraversalSpec");
    clusterComputeResourceTraversalSpec.type = L"ClusterComputeResource";
    clusterComputeResourceTraversalSpec.path = L"host";
    clusterComputeResourceTraversalSpec.skip = &xsd_false;

    dataCenterTraversalSpec.selectSet.push_back(&folderSelectionSpec);
    folderTraversalSpec.selectSet.push_back(&dataCenterSelectionSpec);
    folderTraversalSpec.selectSet.push_back(&folderSelectionSpec);
    folderTraversalSpec.selectSet.push_back(&computeResourceSelectionSpec);
    folderTraversalSpec.selectSet.push_back(&clusterComputeResourceSelectionSpec);
    computeResourceTraversalSpec.selectSet.push_back(&folderSelectionSpec);
    clusterComputeResourceTraversalSpec.selectSet.push_back(&folderSelectionSpec);

    objectSpec.obj = service_content.rootFolder;
    objectSpec.skip = &xsd_true;
    objectSpec.selectSet.push_back(&dataCenterTraversalSpec);
    objectSpec.selectSet.push_back(&folderTraversalSpec);
    objectSpec.selectSet.push_back(&computeResourceTraversalSpec);
    objectSpec.selectSet.push_back(&clusterComputeResourceTraversalSpec);

    propertySpec.type = L"HostSystem";
    propertySpec.all = &xsd_false;

    propertySpec.pathSet.push_back(L"name");
    if (version < 5.5)
    {
        propertySpec.pathSet.push_back(L"config.network.dnsConfig.hostName");
        propertySpec.pathSet.push_back(L"config.network.dnsConfig.domainName");
    }
    else
        propertySpec.pathSet.push_back(L"config.network.netStackInstance");
    propertySpec.pathSet.push_back(L"config.network.vnic");
    propertySpec.pathSet.push_back(L"config.network.consoleVnic");
    propertySpec.pathSet.push_back(L"config.product.fullName");
    propertySpec.pathSet.push_back(L"config.product.version");
    propertySpec.pathSet.push_back(L"network");
    propertySpec.pathSet.push_back(L"datastore");
    propertySpec.pathSet.push_back(L"vm");
    propertySpec.pathSet.push_back(L"runtime.powerState");
    propertySpec.pathSet.push_back(L"runtime.inMaintenanceMode");
    propertySpec.pathSet.push_back(L"hardware.systemInfo.uuid");
    propertySpec.pathSet.push_back(L"hardware.cpuInfo");
    propertySpec.pathSet.push_back(L"hardware.memorySize");

    propertyFilterSpec.propSet.push_back(&propertySpec);
    propertyFilterSpec.objectSet.push_back(&objectSpec);

    retrievePropertiesReq._USCOREthis = service_content.propertyCollector;
    retrievePropertiesReq.specSet.push_back(&propertyFilterSpec);

    {
        journal_transaction trans(this);
        nReturn = _vim_binding.RetrieveProperties(&retrievePropertiesReq, retrievePropertiesRes);
    }

    if (SOAP_OK == nReturn){
        size_t returnSize = retrievePropertiesRes.returnval.size();
        LOG(LOG_LEVEL_DEBUG, L"Number of Hosts : %I64u", returnSize);
        for (size_t i = 0; i < returnSize; i++){
            vmware_host::ptr pHost = vmware_host::ptr(new vmware_host());
            pHost->connection_type = type;
            pHost->virtual_center = virtual_center;
            bool bSkip = false;
            bool bConsoleVnic = false;
            bool bVnic = false;

            size_t propSize = retrievePropertiesRes.returnval[i]->propSet.size();
            for (size_t j = 0; j < propSize; j++){
                pHost->name_ref = retrievePropertiesRes.returnval[i]->obj->__item;
                dynamicProperty = *retrievePropertiesRes.returnval[i]->propSet[j];
                if (dynamicProperty.name == L"config.network.dnsConfig.hostName"){
                    //pHost->key = pHost->name = ((xsd__string *)dynamicProperty.val)->__item.c_str();
                    LOG(LOG_LEVEL_DEBUG, L"Host Name : %s", ((xsd__string *)dynamicProperty.val)->__item.c_str());
                    pHost->name.push_back(((xsd__string *)dynamicProperty.val)->__item);
                    pHost->key.push_back(((xsd__string *)dynamicProperty.val)->__item);
                    if (bCheckHost && !bKeyIsIp && 0 != _wcsicmp(((xsd__string *)dynamicProperty.val)->__item.c_str(), host_key.c_str())) {
                        bSkip = true;
                        break;
                    }
                }
                else if (dynamicProperty.name == L"config.network.dnsConfig.domainName"){
                    //pHost->domain_name = ((xsd__string *)dynamicProperty.val)->__item.c_str();
                    pHost->domain_name.push_back(((xsd__string *)dynamicProperty.val)->__item);
                }
                else if (dynamicProperty.name == L"config.network.netStackInstance")
                {
                    Vim25Api1__ArrayOfHostNetStackInstance* pNetStacks = reinterpret_cast<Vim25Api1__ArrayOfHostNetStackInstance*>(dynamicProperty.val);
                    foreach(Vim25Api1__HostNetStackInstance* pNetStack, pNetStacks->HostNetStackInstance)
                    {
                        if (pNetStack->key->compare(L"defaultTcpipStack") == 0)
                        {
                            pHost->domain_name.push_back(pNetStack->dnsConfig->domainName);
                            pHost->name.push_back(pNetStack->dnsConfig->hostName);
                            pHost->key.push_back(pNetStack->dnsConfig->hostName);

                            if (bCheckHost && !bKeyIsIp && 0 != _wcsicmp(pNetStack->dnsConfig->hostName.c_str(), host_key.c_str()))
                            {
                                bSkip = true;
                                break;
                            }
                        }
                    }

                    if (bSkip)
                        break;
                }
                else if (dynamicProperty.name == L"config.network.vnic" && SOAP_TYPE_Vim25Api1__ArrayOfHostVirtualNic == dynamicProperty.val->soap_type()){
                    Vim25Api1__ArrayOfHostVirtualNic* pNics =
                        reinterpret_cast<Vim25Api1__ArrayOfHostVirtualNic*>(dynamicProperty.val);
                    for (size_t iNic = 0; iNic < pNics->HostVirtualNic.size(); iNic++){
                        Vim25Api1__HostVirtualNic* pNic = pNics->HostVirtualNic.at(iNic);
                        if (pNic && pNic->spec && pNic->spec->ip && pNic->spec->ip->ipAddress && !pNic->spec->ip->ipAddress->empty()){
                            pHost->ip_addresses.push_back(*pNic->spec->ip->ipAddress);
                            if (pHost->ip_addresses.empty()){
                                pHost->ip_address = *pNic->spec->ip->ipAddress;
                            }
                        }
                    }

                    bVnic = true;

                    if (bCheckHost && bKeyIsIp && bConsoleVnic && bVnic){
                        for (size_t iIp = 0; iIp < pHost->ip_addresses.size(); iIp++){
                            bSkip = true;
                            if (pHost->ip_addresses.at(iIp) == host_key){
                                bSkip = false;
                                break;
                            }
                        }

                        if (bSkip)
                            break;
                    }
                }
                else if (dynamicProperty.name == L"config.network.consoleVnic" && SOAP_TYPE_Vim25Api1__ArrayOfHostVirtualNic == dynamicProperty.val->soap_type()){
                    Vim25Api1__ArrayOfHostVirtualNic* pNics =
                        reinterpret_cast<Vim25Api1__ArrayOfHostVirtualNic*>(dynamicProperty.val);
                    for (size_t iNic = 0; iNic < pNics->HostVirtualNic.size(); iNic++){
                        Vim25Api1__HostVirtualNic* pNic = pNics->HostVirtualNic.at(iNic);
                        if (pNic && pNic->spec && pNic->spec->ip && pNic->spec->ip->ipAddress && !pNic->spec->ip->ipAddress->empty()){
                            pHost->ip_addresses.push_back(*pNic->spec->ip->ipAddress);
                            if (pHost->ip_addresses.empty()){
                                pHost->ip_address = *pNic->spec->ip->ipAddress;
                            }
                        }
                    }

                    bConsoleVnic = true;

                    if (bCheckHost && bKeyIsIp && bConsoleVnic && bVnic){
                        for (size_t iIp = 0; iIp < pHost->ip_addresses.size(); iIp++){
                            bSkip = true;
                            if (pHost->ip_addresses.at(iIp) == host_key){
                                bSkip = false;
                                break;
                            }
                        }

                        if (bSkip)
                            break;
                    }
                }
                else if (dynamicProperty.name == L"name"){
                    if (net_is_address_format((LPTSTR)((xsd__string *)dynamicProperty.val)->__item.c_str()))
                        pHost->ip_address = ((xsd__string *)dynamicProperty.val)->__item.c_str();
                }
                else if (dynamicProperty.name == L"hardware.systemInfo.uuid")
                    pHost->uuid = ((xsd__string *)dynamicProperty.val)->__item.c_str();
                else if (dynamicProperty.name == L"config.product.fullName")
                    pHost->product_name = ((xsd__string *)dynamicProperty.val)->__item.c_str();
                else if (dynamicProperty.name == L"config.product.version")
                    pHost->version = ((xsd__string *)dynamicProperty.val)->__item.c_str();
                else if (dynamicProperty.name == L"runtime.powerState"){
                    switch (((Vim25Api1__HostSystemPowerState_ *)dynamicProperty.val)->__item){
                    case Vim25Api1__HostSystemPowerState__poweredOn:
                        pHost->power_state = HV_HOSTPOWER_ON;
                        pHost->state       = L"poweredOn";
                        break;
                    case Vim25Api1__HostSystemPowerState__poweredOff:
                        pHost->power_state = HV_HOSTPOWER_OFF;
                        pHost->state       = L"poweredOff";
                        break;
                    case Vim25Api1__HostSystemPowerState__standBy:
                        pHost->power_state = HV_HOSTPOWER_STANDBY;
                        pHost->state       = L"standBy";
                        break;
                    case Vim25Api1__HostSystemPowerState__unknown:
                        pHost->power_state = HV_HOSTPOWER_UNKNOWN;
                        pHost->state       = L"unknown";
                        break;
                    default:
                        pHost->power_state = HV_HOSTPOWER_UNKNOWN;
                        break;
                    }
                }
                else if (dynamicProperty.name == L"runtime.inMaintenanceMode"){
                    pHost->in_maintenance_mode = ((xsd__boolean *)dynamicProperty.val)->__item;
                }
                else if (dynamicProperty.name == L"network"){
                    for (size_t i = 0; i < ((Vim25Api1__ArrayOfManagedObjectReference *)dynamicProperty.val)->ManagedObjectReference.size(); i++){
                        Vim25Api1__ManagedObjectReference *pNetworkMOR = ((Vim25Api1__ArrayOfManagedObjectReference *)dynamicProperty.val)->ManagedObjectReference[i];
                        mor_name_map::iterator iNetwork = mapVirtualNetworkMorName.find(pNetworkMOR->__item);
                        if (iNetwork != mapVirtualNetworkMorName.end()){
                            pHost->networks[iNetwork->second] = iNetwork->second;
                        }
                        else{
                            try	{
                                std::wstring network_name;
                                get_virtual_network_name_from_mor(service_content, *pNetworkMOR, network_name);
                                pHost->networks[network_name] = network_name;
                                mapVirtualNetworkMorName[pNetworkMOR->__item] = network_name;
                            }
                            catch (...){}
                        }
                    }
                }
                else if (dynamicProperty.name == L"datastore"){
                    for (size_t i = 0; i < ((Vim25Api1__ArrayOfManagedObjectReference *)dynamicProperty.val)->ManagedObjectReference.size(); i++){
                        Vim25Api1__ManagedObjectReference *pDatastoreMOR = ((Vim25Api1__ArrayOfManagedObjectReference *)dynamicProperty.val)->ManagedObjectReference[i];
                        key_name::map::iterator iDatastore = mapDatastoreMorKeyName.find(pDatastoreMOR->__item);
                        if (iDatastore != mapDatastoreMorKeyName.end()){
                            pHost->datastores[iDatastore->second.key] = iDatastore->second.name;
                        }
                        else{
                            vmware_datastore::ptr vmware_datastore_ptr = get_datastore_from_mor(service_content, *pDatastoreMOR);
                            if (vmware_datastore_ptr){
                                pHost->datastores[vmware_datastore_ptr->key] = vmware_datastore_ptr->name;
                            }
                        }
                    }
                }
                else if (dynamicProperty.name == L"vm"){
                    Vim25Api1__ArrayOfManagedObjectReference* pVmMorArray = (Vim25Api1__ArrayOfManagedObjectReference*)dynamicProperty.val;
                    for (size_t i = 0; i < pVmMorArray->ManagedObjectReference.size(); i++){
                        Vim25Api1__ManagedObjectReference* pVmMor = pVmMorArray->ManagedObjectReference.at(i);
                        key_name::map::iterator iVm = vm_more_key_name_map.find(pVmMor->__item);
                        if (iVm != vm_more_key_name_map.end()){
                            pHost->vms[iVm->second.key] = iVm->second.name;
                        }
                    }
                }
                else if (dynamicProperty.name == L"hardware.cpuInfo"){
                    Vim25Api1__HostCpuInfo* pCpuInfo = reinterpret_cast<Vim25Api1__HostCpuInfo*>(dynamicProperty.val);
                    if (pCpuInfo){
                        pHost->number_of_cpu_cores = pCpuInfo->numCpuCores;
                        pHost->number_of_cpu_packages = pCpuInfo->numCpuPackages;
                        pHost->number_of_cpu_threads = pCpuInfo->numCpuThreads;
                    }
                }
                else if (dynamicProperty.name == L"hardware.memorySize"){
                    pHost->size_of_memory = (((xsd__long *)dynamicProperty.val)->__item);
                }

            } // for j

            if (bSkip){
                // Host does not match the provided key
                continue;
            }

            //Get datacenter
            Vim25Api1__ManagedObjectReference *pDatacenterMOR = (Vim25Api1__ManagedObjectReference *)get_ancestor_mor(service_content, retrievePropertiesRes.returnval[i]->obj, L"Datacenter", ancestor_mor_cache);
            if (pDatacenterMOR){
                mor_name_map::iterator datacenterMorName = mapDataCenterMorName.find(pDatacenterMOR->__item);
                if (datacenterMorName != mapDataCenterMorName.end()){
                    pHost->datacenter_name = datacenterMorName->second;
                }
                else{
                    pProperty = get_property_from_mor(service_content, *pDatacenterMOR, L"name");
                    if (pProperty){
                        pHost->datacenter_name = ((xsd__string *)pProperty)->__item.c_str();
                        mapDataCenterMorName[pDatacenterMOR->__item] = pHost->datacenter_name;
                    }
                }
            }
            else{
                LOG(LOG_LEVEL_WARNING, L"No datacenter found for host (%s).", pHost->name.begin()->c_str()); //TODO, need verify under cluster env
            }

            //Clustering properties
            for (size_t iCluster = 0; iCluster < clusters.size(); iCluster++){
                vmware_cluster::ptr vmware_cluster_ptr(clusters.at(iCluster));
                key_map::iterator iHostName = vmware_cluster_ptr->hosts.find(pHost->name.begin()->c_str()); //TODO, need verify under cluster env
                if (iHostName != vmware_cluster_ptr->hosts.end()){
                    pHost->cluster_key = vmware_cluster_ptr->key;
                    break;
                }
            }

            if (pHost->cluster_key.empty()){
                LOG(LOG_LEVEL_WARNING, L"No cluster found for host (%s).", pHost->name.begin()->c_str()); //TODO, need verify under cluster env
            }
            pHost->features = features;
            hosts.push_back(pHost);
        } // for i

        if (!host_key.empty() && hosts.empty()){
            LOG(LOG_LEVEL_ERROR, L"Host(%s) is not found.", host_key.c_str());
        }
    }

   /* soap_delete_std__wstring(&_vim_binding, dataCenterSelectionSpec.name);
    soap_delete_std__wstring(&_vim_binding, folderSelectionSpec.name);
    soap_delete_std__wstring(&_vim_binding, computeResourceSelectionSpec.name);
    soap_delete_std__wstring(&_vim_binding, clusterComputeResourceSelectionSpec.name);
    soap_delete_std__wstring(&_vim_binding, dataCenterTraversalSpec.name);
    soap_delete_std__wstring(&_vim_binding, folderTraversalSpec.name);
    soap_delete_std__wstring(&_vim_binding, computeResourceTraversalSpec.name);
    soap_delete_std__wstring(&_vim_binding, clusterComputeResourceTraversalSpec.name);*/

    if (SOAP_OK != nReturn)
    {
        int lastErrorCode;
        std::wstring lastErrorMessage;

        get_native_error(lastErrorCode, lastErrorMessage);
        LOG(LOG_LEVEL_ERROR, L"__Vim25Api1__RetrieveProperties error: %s (%d)", lastErrorMessage.c_str(), nReturn);
        BOOST_THROW_VMWARE_EXCEPTION(lastErrorCode, lastErrorMessage);
    }

    return hosts;
}

vmware_host::vtr vmware_portal::get_hosts(const std::wstring& host_key){
    FUN_TRACE;
    return get_hosts_internal(*(Vim25Api1__ServiceContent*)*_connect.get(), host_key);
}

vmware_host::ptr vmware_portal::get_host(const std::wstring& host_key){
    FUN_TRACE;
    vmware_host::vtr hosts = get_hosts_internal(*(Vim25Api1__ServiceContent*)*_connect.get(), host_key);
    return hosts.empty() ? NULL : hosts.at(0);
}

vmware_cluster::vtr vmware_portal::get_clusters(const std::wstring& cluster_key){
    FUN_TRACE;    
    return get_clusters_internal(*(Vim25Api1__ServiceContent*)*_connect.get(), cluster_key);
}

vmware_cluster::ptr vmware_portal::get_cluster(const std::wstring& cluster_key){
    vmware_cluster::vtr clusters = get_clusters_internal(*(Vim25Api1__ServiceContent*)*_connect.get(), cluster_key);
    return clusters.empty() ? NULL : clusters.at(0);
}

vmware_virtual_machine::vtr vmware_portal::get_virtual_machines(const std::wstring& host_key, const std::wstring& machine_key){
    FUN_TRACE;
    return get_virtual_machines_internal(*(Vim25Api1__ServiceContent*)*_connect.get(), machine_key, host_key);
}

vmware_virtual_machine::ptr vmware_portal::get_virtual_machine(const std::wstring& machine_key, const std::wstring& host_key){
    FUN_TRACE;
    vmware_virtual_machine::ptr vm;
    if (host_key.empty()){
        Vim25Api1__ManagedObjectReference        vm_mor;
        key_name::map                            vm_more_key_name_map;
        vm_mor.__item = L"";
        get_vm_mor_key_name_map(*(Vim25Api1__ServiceContent*)*_connect.get(), vm_more_key_name_map);
        get_vm_mor_from_key_name_map(machine_key, vm_more_key_name_map, vm_mor);
        if (!vm_mor.__item.empty())
            vm = get_virtual_machine_from_mor(*(Vim25Api1__ServiceContent*)*_connect.get(), vm_mor);
    }
    else{
        vmware_virtual_machine::vtr vms = get_virtual_machines_internal(*(Vim25Api1__ServiceContent*)*_connect.get(), machine_key, host_key);
        vm = vms.empty() ? NULL : vms.at(0);
    }
    return vm;
}

vmware_vm_task_info::vtr vmware_portal::get_virtual_machine_tasks(const std::wstring &machine_key){
    FUN_TRACE;
    return get_virtual_machine_tasks_internal(*(Vim25Api1__ServiceContent*)*_connect.get(), machine_key);   
}

key_map vmware_portal::get_all_virtual_machines(){
    FUN_TRACE;
    key_map vms;
    key_name::map _vms = get_virtual_machines_key_name_map(*(Vim25Api1__ServiceContent*)*_connect.get());
    foreach(key_name::map::value_type& vm, _vms)
        vms[vm.second.key] = vm.second.name;
    return vms;
}

vmware_cluster::vtr vmware_portal::get_clusters_internal(Vim25Api1__ServiceContent& service_content, std::wstring cluster_key, bool host_only){
    FUN_TRACE;
    vmware_cluster::vtr clusters;
    // Get host mor name map
    mor_name_map mapHostMorName;
    float                                   version = 0;
    hv_connection_type                      type;

    version = get_connection_info(service_content, type);
    get_host_mor_name_map(service_content, mapHostMorName);
    // Get datastore mor key name map
    key_name::map mapDatastoreMorKeyName;
    if (!host_only)
        get_datastore_mor_key_name_map(service_content, mapDatastoreMorKeyName);

    Vim25Api1__PropertySpec propertySpec;
    if (version >= 2.5)
        propertySpec.type = L"ComputeResource";
    else
        propertySpec.type = L"ClusterComputeResource";
    propertySpec.all = &xsd_false;
    propertySpec.pathSet.push_back(L"name");
    if (version >= 2.5)
        propertySpec.pathSet.push_back(L"configurationEx");
    else
    {
        propertySpec.pathSet.push_back(L"configuration.dasConfig");
        propertySpec.pathSet.push_back(L"configuration.drsConfig");
    }
    propertySpec.pathSet.push_back(L"host");
    propertySpec.pathSet.push_back(L"summary");
    if (!host_only){
        propertySpec.pathSet.push_back(L"datastore");
    }
    Vim25Api1__SelectionSpec folderSelectionSpec;
    folderSelectionSpec.name = soap_new_std__wstring(&_vim_binding, -1);
    *folderSelectionSpec.name = L"Folder";

    Vim25Api1__SelectionSpec datacenterSelectionSpec;
    datacenterSelectionSpec.name = soap_new_std__wstring(&_vim_binding, -1);
    *datacenterSelectionSpec.name = L"Datacenter";

    Vim25Api1__TraversalSpec folderTraversalSpec;
    folderTraversalSpec.name = folderSelectionSpec.name;
    folderTraversalSpec.type = L"Folder";
    folderTraversalSpec.path = L"childEntity";
    folderTraversalSpec.skip = &xsd_false;
    folderTraversalSpec.selectSet.push_back(&folderSelectionSpec);
    folderTraversalSpec.selectSet.push_back(&datacenterSelectionSpec);

    Vim25Api1__TraversalSpec datacenterTraversalSpec;
    datacenterTraversalSpec.name = datacenterSelectionSpec.name;
    datacenterTraversalSpec.type = L"Datacenter";
    datacenterTraversalSpec.path = L"hostFolder";
    datacenterTraversalSpec.skip = &xsd_false;
    datacenterTraversalSpec.selectSet.push_back(&folderSelectionSpec);

    Vim25Api1__ObjectSpec objectSpec;
    objectSpec.obj = service_content.rootFolder;
    objectSpec.skip = &xsd_false;
    objectSpec.selectSet.push_back(&folderTraversalSpec);
    objectSpec.selectSet.push_back(&datacenterTraversalSpec);

    Vim25Api1__PropertyFilterSpec propertyFilterSpec;
    propertyFilterSpec.propSet.push_back(&propertySpec);
    propertyFilterSpec.objectSet.push_back(&objectSpec);

    Vim25Api1__RetrievePropertiesRequestType retrievePropertiesReq;
    retrievePropertiesReq._USCOREthis = service_content.propertyCollector;
    retrievePropertiesReq.specSet.push_back(&propertyFilterSpec);

    _Vim25Api1__RetrievePropertiesResponse retrievePropertiesRes;

    int nReturn = 0;

    {
        journal_transaction trans(this);
        nReturn = _vim_binding.RetrieveProperties(&retrievePropertiesReq, retrievePropertiesRes);
    }

    if (SOAP_OK != nReturn){
        int lastErrorCode;
        std::wstring lastErrorMessage;

        //soap_delete_std__wstring(&_vim_binding, datacenterSelectionSpec.name);
        //soap_delete_std__wstring(&_vim_binding, folderSelectionSpec.name);

        get_native_error(lastErrorCode, lastErrorMessage);
        LOG(LOG_LEVEL_ERROR, L"__Vim25Api1__RetrieveProperties error: %s (%d)", lastErrorMessage.c_str(), nReturn);
        BOOST_THROW_VMWARE_EXCEPTION(lastErrorCode, lastErrorMessage);
    }

    for (size_t iReturnval = 0; iReturnval < retrievePropertiesRes.returnval.size(); ++iReturnval){
        Vim25Api1__ObjectContent* objectContent = retrievePropertiesRes.returnval.at(iReturnval);

        vmware_cluster::ptr vmware_cluster_ptr(new vmware_cluster());
        std::vector<Vim25Api1__ManagedObjectReference*> hosts;

        for (size_t iPropSet = 0; iPropSet < objectContent->propSet.size(); ++iPropSet){
            Vim25Api1__DynamicProperty* dynamicProperty = objectContent->propSet.at(iPropSet);

            if (0 == dynamicProperty->name.compare(L"name")){
                xsd__string* name = (xsd__string*)dynamicProperty->val;
                vmware_cluster_ptr->key = vmware_cluster_ptr->name = name->__item;
                if (!cluster_key.empty() && 0 != _wcsicmp(cluster_key.c_str(), vmware_cluster_ptr->key.c_str())){
                    vmware_cluster_ptr = NULL;
                    break;
                }
            }
            else if (0 == dynamicProperty->name.compare(L"configuration.dasConfig")){
                Vim25Api1__ClusterDasConfigInfo* clusterDasConfigInfo = (Vim25Api1__ClusterDasConfigInfo*)dynamicProperty->val;
                if (clusterDasConfigInfo->enabled && *clusterDasConfigInfo->enabled){
                    vmware_cluster_ptr->is_ha = true;
                }
            }
            else if (0 == dynamicProperty->name.compare(L"configuration.drsConfig")){
                Vim25Api1__ClusterDrsConfigInfo* clusterDrsConfigInfo = (Vim25Api1__ClusterDrsConfigInfo*)dynamicProperty->val;
                if (clusterDrsConfigInfo->enabled && *clusterDrsConfigInfo->enabled){
                    vmware_cluster_ptr->is_drs = true;
                }
            }
            else if (0 == dynamicProperty->name.compare(L"configurationEx"))
            {
                if (dynamicProperty->val)
                {
                    Vim25Api1__ClusterConfigInfoEx* configEx = dynamic_cast<Vim25Api1__ClusterConfigInfoEx*>(dynamicProperty->val);
                    if (configEx)
                    {
                        if (configEx->dasConfig && configEx->dasConfig->enabled)
                            vmware_cluster_ptr->is_ha = true;

                        if (configEx->drsConfig && configEx->drsConfig->enabled)
                            vmware_cluster_ptr->is_drs = true;
                    }
                }
            }
            else if (0 == dynamicProperty->name.compare(L"host")){
                Vim25Api1__ArrayOfManagedObjectReference* morArray = (Vim25Api1__ArrayOfManagedObjectReference*)dynamicProperty->val;
                for (size_t iMor = 0; iMor < morArray->ManagedObjectReference.size(); ++iMor){
                    hosts.push_back(morArray->ManagedObjectReference.at(iMor));

                    std::wstring wszHostName;

                    mor_name_map::iterator iHost = mapHostMorName.find(morArray->ManagedObjectReference.at(iMor)->__item);
                    if (iHost != mapHostMorName.end()){
                        vmware_cluster_ptr->hosts[iHost->second] = iHost->second;
                    }
                }
            }
            else if (0 == dynamicProperty->name.compare(L"datastore")){
                Vim25Api1__ArrayOfManagedObjectReference* morArray = (Vim25Api1__ArrayOfManagedObjectReference*)dynamicProperty->val;
                for (size_t iMor = 0; iMor < morArray->ManagedObjectReference.size(); ++iMor){
                    key_name::map::iterator iDatastore = mapDatastoreMorKeyName.find(morArray->ManagedObjectReference.at(iMor)->__item);
                    if (iDatastore != mapDatastoreMorKeyName.end()){
                        vmware_cluster_ptr->datastores[iDatastore->second.key] = iDatastore->second.name;
                    }
                }
            }
            else if (0 == dynamicProperty->name.compare(L"summary")){
                Vim25Api1__ClusterComputeResourceSummary* summary = (Vim25Api1__ClusterComputeResourceSummary*)dynamicProperty->val;
                vmware_cluster_ptr->number_of_hosts = summary->numHosts;
                vmware_cluster_ptr->number_of_cpu_cores = summary->numCpuCores;
                vmware_cluster_ptr->total_cpu = summary->totalCpu;
                vmware_cluster_ptr->total_memory = summary->totalMemory;
                vmware_cluster_ptr->current_evc_mode_key = summary->currentEVCModeKey ? true : false;
            }
        }

        if (vmware_cluster_ptr){
            // Get datacenter name
            Vim25Api1__ManagedObjectReference* datacenter = reinterpret_cast<Vim25Api1__ManagedObjectReference*>(get_ancestor_mor(service_content, objectContent->obj, L"Datacenter"));
            if (datacenter){
                xsd__string* datacenterName =
                    reinterpret_cast<xsd__string*>(get_property_from_mor(service_content, *datacenter, L"name"));
                if (datacenterName)
                    vmware_cluster_ptr->datacenter = datacenterName->__item.c_str();
            }
            else{
                LOG(LOG_LEVEL_ERROR, L"No datacenter found for cluster (%s).", vmware_cluster_ptr->name.c_str());
            }
            if (!host_only){
                // Get networks
                vmware_virtual_network::vtr networks =
                    get_virtual_networks_internal(service_content, hosts, std::wstring());
                for (size_t iNetworks = 0; iNetworks < networks.size(); iNetworks++){
                    vmware_virtual_network::ptr network(networks.at(iNetworks));
                    vmware_cluster_ptr->networks[network->key] = network->name;
                }
            }

            clusters.push_back(vmware_cluster_ptr);
        }
    }

    //soap_delete_std__wstring(&_vim_binding, datacenterSelectionSpec.name);
    //soap_delete_std__wstring(&_vim_binding, folderSelectionSpec.name);

    return clusters;
}

vmware_virtual_machine::vtr vmware_portal::get_virtual_machines_internal(Vim25Api1__ServiceContent& service_content, std::wstring machine_key, std::wstring host_key, std::wstring cluster_key){
   
    FUN_TRACE;
    int                                 nReturn;
    Vim25Api1__ManagedObjectReference*        pVmMOR = NULL;
    Vim25Api1__TraversalSpec                  dataCenterTraversalSpec, folderTraversalSpec;
    Vim25Api1__SelectionSpec                  dataCenterSelectionSpec, folderSelectionSpec;
    Vim25Api1__PropertySpec                   propertySpec;
    Vim25Api1__ObjectSpec                     objectSpec;
    Vim25Api1__PropertyFilterSpec             propertyFilterSpec;
    Vim25Api1__RetrievePropertiesRequestType  retrievePropertiesReq;
    _Vim25Api1__RetrievePropertiesResponse    retrievePropertiesRes;
    mor_name_map                        mapHostMorName;
    mor_name_map                        mapNetworkMorName;
    key_name::map                       mapVmMorKeyName;
    mor_name_map                        mapHostMorClusterName;
    vmware_virtual_machine::vtr         vms;
    float                               version = 0;
    hv_connection_type                  type = HV_CONNECTION_TYPE_UNKNOWN;

    version = get_connection_info(service_content, type);

    get_host_mor_name_map(service_content, mapHostMorName);
    dataCenterSelectionSpec.name = soap_new_std__wstring(&_vim_binding, -1);
    folderSelectionSpec.name = soap_new_std__wstring(&_vim_binding, -1);
    dataCenterTraversalSpec.name = soap_new_std__wstring(&_vim_binding, -1);
    folderTraversalSpec.name = soap_new_std__wstring(&_vim_binding, -1);

    dataCenterSelectionSpec.name->assign(L"DataCenterTraversalSpec");
    folderSelectionSpec.name->assign(L"FolderTraversalSpec");

    dataCenterTraversalSpec.name->assign(L"DataCenterTraversalSpec");
    dataCenterTraversalSpec.type = L"Datacenter";
    dataCenterTraversalSpec.path = L"vmFolder";
    dataCenterTraversalSpec.skip = &xsd_true;

    folderTraversalSpec.name->assign(L"FolderTraversalSpec");
    folderTraversalSpec.type = L"Folder";
    folderTraversalSpec.path = L"childEntity";
    folderTraversalSpec.skip = &xsd_false;

    dataCenterTraversalSpec.selectSet.push_back(&folderSelectionSpec);
    folderTraversalSpec.selectSet.push_back(&dataCenterSelectionSpec);
    folderTraversalSpec.selectSet.push_back(&folderSelectionSpec);

    objectSpec.obj = service_content.rootFolder;
    objectSpec.skip = &xsd_true;
    objectSpec.selectSet.push_back(&dataCenterTraversalSpec);
    objectSpec.selectSet.push_back(&folderTraversalSpec);

    propertySpec.type = L"VirtualMachine";
    propertySpec.all = &xsd_false;
    propertySpec.pathSet.push_back(L"name");

    propertySpec.pathSet.push_back(L"overallStatus");
    propertySpec.pathSet.push_back(L"config.guestId");
    propertySpec.pathSet.push_back(L"config.guestFullName");
    if (version >= 4.0)
        propertySpec.pathSet.push_back(L"config.changeTrackingEnabled");
    propertySpec.pathSet.push_back(L"config.hardware.device");
    propertySpec.pathSet.push_back(L"config.hardware.memoryMB");
    propertySpec.pathSet.push_back(L"config.hardware.numCPU");
    if (version >= 4.0){
        propertySpec.pathSet.push_back(L"config.cpuHotAddEnabled");
        propertySpec.pathSet.push_back(L"config.cpuHotRemoveEnabled");  //New in API 4.0
    }
    propertySpec.pathSet.push_back(L"config.template");
    propertySpec.pathSet.push_back(L"config.uuid");
    propertySpec.pathSet.push_back(L"config.extraConfig");
    propertySpec.pathSet.push_back(L"config.version");

    if (version >= 5.0)
        propertySpec.pathSet.push_back(L"config.firmware");

    propertySpec.pathSet.push_back(L"guest.hostName");
    propertySpec.pathSet.push_back(L"guest.ipAddress");
    propertySpec.pathSet.push_back(L"guest.net");
    if (version >= 5.0)
        propertySpec.pathSet.push_back(L"guest.toolsVersionStatus2");
    else if (version == 4.0)
        propertySpec.pathSet.push_back(L"guest.toolsVersionStatus");
    else
        propertySpec.pathSet.push_back(L"guest.toolsStatus");
    propertySpec.pathSet.push_back(L"runtime.connectionState");
    propertySpec.pathSet.push_back(L"runtime.host");
    propertySpec.pathSet.push_back(L"runtime.powerState");
    propertySpec.pathSet.push_back(L"summary.config.annotation");
    propertySpec.pathSet.push_back(L"summary.config.vmPathName");
    propertySpec.pathSet.push_back(L"summary.vm");
    propertySpec.pathSet.push_back(L"datastore");
    propertySpec.pathSet.push_back(L"network");
    propertySpec.pathSet.push_back(L"snapshot");
    propertySpec.pathSet.push_back(L"resourceConfig.entity");

    propertyFilterSpec.propSet.push_back(&propertySpec);
    propertyFilterSpec.objectSet.push_back(&objectSpec);

    retrievePropertiesReq._USCOREthis = service_content.propertyCollector;
    retrievePropertiesReq.specSet.push_back(&propertyFilterSpec);

    {
        journal_transaction trans(this);
        nReturn = _vim_binding.RetrieveProperties(&retrievePropertiesReq, retrievePropertiesRes);
    }

    if (SOAP_OK == nReturn){
        size_t returnSize = retrievePropertiesRes.returnval.size();
        for (size_t i = 0; i < returnSize; i++){
            if (_is_interrupted)
            {
                LOG(LOG_LEVEL_WARNING, L"Interrupt captured.");
                vms.clear();
                break;
            }

            size_t propSize = retrievePropertiesRes.returnval[i]->propSet.size();

            if (!machine_key.empty())
            {
                bool found = false;
                for (size_t j = 0; j < propSize; j++)
                {
                    Vim25Api1__DynamicProperty& dynamicProperty = *retrievePropertiesRes.returnval[i]->propSet[j];
                    if (dynamicProperty.name == L"config.uuid")
                    {
                        if (0 == _wcsicmp(machine_key.c_str(), ((xsd__string *)dynamicProperty.val)->__item.c_str()))
                        {
                            found = true;
                        }
                        break;
                    }
                }

                if (!found)
                    continue;
            }

            vmware_virtual_machine::ptr pVirtualMachine = fetch_virtual_machine_info(service_content, 
                                                                                    *retrievePropertiesRes.returnval[i],
                                                                                    mapHostMorName, 
                                                                                    mapNetworkMorName,
                                                                                    mapVmMorKeyName,
                                                                                    mapHostMorClusterName,
                                                                                    type,
                                                                                    host_key,
                                                                                    cluster_key);
            if (pVirtualMachine){
                if (pVirtualMachine->key.empty()){
                    // Probably a messed up VM without a UUID
                    pVirtualMachine->key = pVirtualMachine->name;
                    if (!machine_key.empty()){
                        continue;
                    }
                }
                else{
                    key_name _key_name;
                    _key_name.key = pVirtualMachine->key;
                    _key_name.name = pVirtualMachine->name;
                    mapVmMorKeyName[retrievePropertiesRes.returnval[i]->obj->__item] = _key_name;
                }

                //vmx spec
                std::wstring vmx_spec = pVirtualMachine->config_path_file;
                std::wstring config_path = L"[" + pVirtualMachine->config_path + L"] ";
                replace_all(vmx_spec, config_path, L"");

                vmx_spec = vmx_spec + L"?dcPath=" + pVirtualMachine->datacenter_name + L"&dsName=" + pVirtualMachine->config_path;
                pVirtualMachine->vmxspec = vmx_spec;

                vms.push_back(pVirtualMachine);
                if (!machine_key.empty()){
                    break;
                }
            }
        } // for i
    }

    if (!vms.empty()){
        vmware_folder::vtr vm_folders = get_vm_folder_internal( service_content, std::wstring(), mapVmMorKeyName);
        foreach(vmware_virtual_machine::ptr vm, vms){
            foreach(vmware_folder::ptr f, vm_folders){
                f->get_vm_folder_path(vm->key, vm->folder_path);
            }
        }
    }

    //soap_delete_std__wstring(&_vim_binding, dataCenterSelectionSpec.name);
    //soap_delete_std__wstring(&_vim_binding, folderSelectionSpec.name);
    //soap_delete_std__wstring(&_vim_binding, dataCenterTraversalSpec.name);
    //soap_delete_std__wstring(&_vim_binding, folderTraversalSpec.name);

    if (SOAP_OK != nReturn)
    {
        int lastErrorCode;
        std::wstring lastErrorMessage;
        get_native_error(lastErrorCode, lastErrorMessage);
        LOG(LOG_LEVEL_ERROR, L"__Vim25Api1__RetrieveProperties error: %s (%d)", lastErrorMessage.c_str(), nReturn);
        BOOST_THROW_VMWARE_EXCEPTION(lastErrorCode, lastErrorMessage);
    }

    //{
    //    HRESULT                     hr2 = S_OK;
    //    SANHV_RESOURCEPOOL_TABLE    tbResourcePools;
    //    hr2 = ResourcePool_GetForHosts(*(Vim25Api1__ServiceContent *)pServiceContent, mapHostMorName, mapVmMorKeyName, tbResourcePools);
    //    if (FAILED(hr2))
    //    {
    //        // TODO, return error?
    //    }

    //    for (size_t iVm = 0; iVm < tbVmNics.size(); iVm++)
    //    {
    //        CSanHvVirtualMachinePtr pVirtualMachine(tbVmNics.at(iVm)->m_pVirtualMachine);
    //        for (size_t iRp = 0; iRp < tbResourcePools.size(); iRp++)
    //        {
    //            if (tbResourcePools.at(iRp)->GetResourcePoolPath(pVirtualMachine->m_wszKey, pVirtualMachine->m_wszResourcePoolPath))
    //                break;
    //        }
    //    }
    //}

#if 0
    if (!vms.empty())
        fetch_vm_vmdks_size(service_content, vms);
#endif

    return vms;
}

key_name::map vmware_portal::get_virtual_machines_key_name_map(Vim25Api1__ServiceContent& service_content){
    FUN_TRACE;
    int                                       nReturn;
    Vim25Api1__ManagedObjectReference*        pVmMOR = NULL;
    Vim25Api1__TraversalSpec                  dataCenterTraversalSpec, folderTraversalSpec;
    Vim25Api1__SelectionSpec                  dataCenterSelectionSpec, folderSelectionSpec;
    Vim25Api1__PropertySpec                   propertySpec;
    Vim25Api1__ObjectSpec                     objectSpec;
    Vim25Api1__PropertyFilterSpec             propertyFilterSpec;
    Vim25Api1__RetrievePropertiesRequestType  retrievePropertiesReq;
    _Vim25Api1__RetrievePropertiesResponse    retrievePropertiesRes;
    key_name::map                             mapVmMorKeyName;
    float                                     version = 0;
    hv_connection_type                        type = HV_CONNECTION_TYPE_UNKNOWN;

    version = get_connection_info(service_content, type);

    dataCenterSelectionSpec.name = soap_new_std__wstring(&_vim_binding, -1);
    folderSelectionSpec.name = soap_new_std__wstring(&_vim_binding, -1);
    dataCenterTraversalSpec.name = soap_new_std__wstring(&_vim_binding, -1);
    folderTraversalSpec.name = soap_new_std__wstring(&_vim_binding, -1);

    dataCenterSelectionSpec.name->assign(L"DataCenterTraversalSpec");
    folderSelectionSpec.name->assign(L"FolderTraversalSpec");

    dataCenterTraversalSpec.name->assign(L"DataCenterTraversalSpec");
    dataCenterTraversalSpec.type = L"Datacenter";
    dataCenterTraversalSpec.path = L"vmFolder";
    dataCenterTraversalSpec.skip = &xsd_true;

    folderTraversalSpec.name->assign(L"FolderTraversalSpec");
    folderTraversalSpec.type = L"Folder";
    folderTraversalSpec.path = L"childEntity";
    folderTraversalSpec.skip = &xsd_false;

    dataCenterTraversalSpec.selectSet.push_back(&folderSelectionSpec);
    folderTraversalSpec.selectSet.push_back(&dataCenterSelectionSpec);
    folderTraversalSpec.selectSet.push_back(&folderSelectionSpec);

    objectSpec.obj = service_content.rootFolder;
    objectSpec.skip = &xsd_true;
    objectSpec.selectSet.push_back(&dataCenterTraversalSpec);
    objectSpec.selectSet.push_back(&folderTraversalSpec);

    propertySpec.type = L"VirtualMachine";
    propertySpec.all = &xsd_false;
    propertySpec.pathSet.push_back(L"name");
    propertySpec.pathSet.push_back(L"config.uuid");

    propertyFilterSpec.propSet.push_back(&propertySpec);
    propertyFilterSpec.objectSet.push_back(&objectSpec);

    retrievePropertiesReq._USCOREthis = service_content.propertyCollector;
    retrievePropertiesReq.specSet.push_back(&propertyFilterSpec);

    {
        journal_transaction trans(this);
        nReturn = _vim_binding.RetrieveProperties(&retrievePropertiesReq, retrievePropertiesRes);
    }

    if (SOAP_OK == nReturn){
        size_t returnSize = retrievePropertiesRes.returnval.size();
        for (size_t i = 0; i < returnSize; i++){
            if (_is_interrupted){
                LOG(LOG_LEVEL_WARNING, L"Interrupt captured.");
                break;
            }
            key_name _key_name;
            size_t propSize = retrievePropertiesRes.returnval[i]->propSet.size();
            for (size_t j = 0; j < propSize; j++){
                Vim25Api1__DynamicProperty& dynamicProperty = *retrievePropertiesRes.returnval[i]->propSet[j];
                if (dynamicProperty.name == L"config.uuid"){
                    _key_name.key = ((xsd__string *)dynamicProperty.val)->__item.c_str();
                }
                else if (dynamicProperty.name == L"name"){
                    _key_name.name = ((xsd__string *)dynamicProperty.val)->__item.c_str();
                }
            }
            if (!_key_name.key.empty())
                mapVmMorKeyName[retrievePropertiesRes.returnval[i]->obj->__item] = _key_name;
        } // for i
    }

    if (SOAP_OK != nReturn)
    {
        int lastErrorCode;
        std::wstring lastErrorMessage;
        get_native_error(lastErrorCode, lastErrorMessage);
        LOG(LOG_LEVEL_ERROR, L"__Vim25Api1__RetrieveProperties error: %s (%d)", lastErrorMessage.c_str(), nReturn);
        BOOST_THROW_VMWARE_EXCEPTION(lastErrorCode, lastErrorMessage);
    }
    return mapVmMorKeyName;
}

vmware_vm_task_info::vtr vmware_portal::get_virtual_machine_tasks_internal(Vim25Api1__ServiceContent& service_content, std::wstring machine_key){
    FUN_TRACE;
    int nReturn = TASK_STATE::STATE_SUCCESS;
    vmware_vm_task_info::vtr tasks;
    Vim25Api1__PropertySpec taskPropertySpec;
    taskPropertySpec.type = L"Task";
    taskPropertySpec.all = &xsd_false;
    taskPropertySpec.pathSet.push_back(L"info.error");
    taskPropertySpec.pathSet.push_back(L"info.entity");
    taskPropertySpec.pathSet.push_back(L"info.entityName");
    taskPropertySpec.pathSet.push_back(L"info.name");
    taskPropertySpec.pathSet.push_back(L"info.state");
    taskPropertySpec.pathSet.push_back(L"info.progress");
    taskPropertySpec.pathSet.push_back(L"info.cancelable");
    taskPropertySpec.pathSet.push_back(L"info.cancelled");
    taskPropertySpec.pathSet.push_back(L"info.result");
    taskPropertySpec.pathSet.push_back(L"info.startTime");
    taskPropertySpec.pathSet.push_back(L"info.completeTime");
    
    Vim25Api1__ObjectSpec objectSpec;
    objectSpec.obj = service_content.taskManager;
    objectSpec.skip = &xsd_false;
    Vim25Api1__TraversalSpec tSpec;
    tSpec.type = L"TaskManager";
    tSpec.path = L"recentTask";
    tSpec.skip = &xsd_false;
    objectSpec.selectSet.push_back(&tSpec);

    Vim25Api1__PropertyFilterSpec propertyFilterSpec;
    propertyFilterSpec.propSet.push_back(&taskPropertySpec);
    propertyFilterSpec.objectSet.push_back(&objectSpec);
    Vim25Api1__RetrievePropertiesRequestType retrievePropertiesReq;
    retrievePropertiesReq._USCOREthis = service_content.propertyCollector;
    retrievePropertiesReq.specSet.push_back(&propertyFilterSpec);
    _Vim25Api1__RetrievePropertiesResponse retrievePropertiesRes;

    nReturn = _vim_binding.RetrieveProperties(&retrievePropertiesReq, retrievePropertiesRes);

    if (SOAP_OK != nReturn)
    {
        int lastErrorCode;
        std::wstring lastErrorMessage;
        get_native_error(lastErrorCode, lastErrorMessage);
        LOG(LOG_LEVEL_ERROR, L"error: %s (%d)",  lastErrorMessage.c_str(), nReturn);
        BOOST_THROW_VMWARE_EXCEPTION(lastErrorCode, lastErrorMessage);
    }

    for (size_t iReturnval = 0; iReturnval < retrievePropertiesRes.returnval.size(); ++iReturnval)
    {
        vmware_vm_task_info::ptr  task_info = vmware_vm_task_info::ptr(new vmware_vm_task_info());
        Vim25Api1__ObjectContent* objectContent = retrievePropertiesRes.returnval.at(iReturnval);
        _fetch_task_info(service_content, objectContent, *task_info.get());
        if (machine_key.empty() || 0 == _wcsicmp(machine_key.c_str(), task_info->vm_uuid.c_str()))
            tasks.push_back(task_info);
    }
    return tasks;
}

VOID vmware_portal::get_virtual_network_name_from_mor(Vim25Api1__ServiceContent& service_content, Vim25Api1__ManagedObjectReference& network, std::wstring& name){
    FUN_TRACE;
    xsd__string* pName = (xsd__string*)get_property_from_mor(service_content, network, L"name");
    if (!pName){
        std::wstring err = boost::str(boost::wformat(L"Failed to get name for network (%s).")%network.__item.c_str());
        LOG(LOG_LEVEL_ERROR, err.c_str());
        BOOST_THROW_VMWARE_EXCEPTION(HV_ERROR_FAILED_TO_GET_PROPERTY, err);
    }
    name = pName->__item.c_str();
}

VOID vmware_portal::get_cluster_name_from_mor(Vim25Api1__ServiceContent& service_content, Vim25Api1__ManagedObjectReference& mor, std::wstring& name){
    FUN_TRACE;
    xsd__anyType* pName = reinterpret_cast<xsd__anyType*>(get_property_from_mor(service_content, mor, L"name"));
    if (!pName || SOAP_TYPE_xsd__string != pName->soap_type()){
        std::wstring err = boost::str(boost::wformat(L"Failed to get name for cluster (%s).") % mor.__item.c_str());
        LOG(LOG_LEVEL_ERROR, err.c_str());
        BOOST_THROW_VMWARE_EXCEPTION(HV_ERROR_FAILED_TO_GET_PROPERTY, err);
    }
    name = reinterpret_cast<xsd__string*>(pName)->__item;
}

VOID vmware_portal::get_host_name_from_mor(Vim25Api1__ServiceContent& service_content, Vim25Api1__ManagedObjectReference& mor, std::wstring& name){
    FUN_TRACE;
    xsd__string* pHostName = (xsd__string*)get_property_from_mor(service_content, mor, L"config.network.dnsConfig.hostName");
    if (!pHostName){
        std::wstring err = boost::str(boost::wformat(L"Failed to get host name for host (%s).") % mor.__item.c_str());
        LOG(LOG_LEVEL_ERROR, err.c_str());
        BOOST_THROW_VMWARE_EXCEPTION(HV_ERROR_FAILED_TO_GET_PROPERTY, err);
    }
    name = pHostName->__item.c_str();
}

VOID vmware_portal::get_virtual_machine_uuid_from_mor(Vim25Api1__ServiceContent& service_content, Vim25Api1__ManagedObjectReference& mor, std::wstring& uuid){
    FUN_TRACE;
    xsd__string* pUuid = (xsd__string*)get_property_from_mor(service_content, mor, L"config.uuid");
    if (!pUuid){
        std::wstring err = boost::str(boost::wformat(L"Failed to get UUID for VM (%s).") % mor.__item.c_str());
        LOG(LOG_LEVEL_ERROR, err.c_str());
        BOOST_THROW_VMWARE_EXCEPTION(HV_ERROR_FAILED_TO_GET_PROPERTY, err);
    }
    uuid = pUuid->__item.c_str();
}

Vim25Api1__ManagedObjectReference*  vmware_portal::get_ancestor_mor(Vim25Api1__ServiceContent& service_content, Vim25Api1__ManagedObjectReference* mor, const std::wstring& ancestor_type, mor_map& ancestor_mor_cache){
    
    FUN_TRACE;
    Vim25Api1__ManagedObjectReference*    pParentMOR = NULL;
    int                             nReturn;
    Vim25Api1__PropertySpec               propertySpec;
    Vim25Api1__ObjectSpec                 objectSpec;
    Vim25Api1__PropertyFilterSpec         propertyFilterSpec;
    Vim25Api1__DynamicProperty            dynamicProperty;
    Vim25Api1__RetrievePropertiesRequestType  retrievePropertiesReq;
    _Vim25Api1__RetrievePropertiesResponse    retrievePropertiesRes;
   
    if (!mor){
        LOG(LOG_LEVEL_ERROR, L"mor is NULL.");
        return NULL;
    }
    propertySpec.type = mor->type->c_str();
    propertySpec.all = &xsd_false;
    propertySpec.pathSet.push_back(L"parent");

    objectSpec.obj = (Vim25Api1__ManagedObjectReference *)mor;
    objectSpec.skip = &xsd_false;

    propertyFilterSpec.propSet.push_back(&propertySpec);
    propertyFilterSpec.objectSet.push_back(&objectSpec);

    retrievePropertiesReq._USCOREthis = service_content.propertyCollector;
    retrievePropertiesReq.specSet.push_back(&propertyFilterSpec);

    {
        journal_transaction trans(this);
        nReturn = _vim_binding.RetrieveProperties(&retrievePropertiesReq, retrievePropertiesRes);
    }

    if (SOAP_OK == nReturn){
        if (retrievePropertiesRes.returnval.size() && retrievePropertiesRes.returnval[0]->propSet.size()){
            dynamicProperty = *retrievePropertiesRes.returnval[0]->propSet[0];
            pParentMOR = (Vim25Api1__ManagedObjectReference *)dynamicProperty.val;
            if (pParentMOR && ancestor_type.length() && (0 != pParentMOR->type->compare(ancestor_type))){
                mor_map::iterator _mor = ancestor_mor_cache.find(pParentMOR->__item);
                if (_mor != ancestor_mor_cache.end()){
                    pParentMOR = _mor->second;
                }
                else{
                    LOG(LOG_LEVEL_TRACE, L"Parent object (%s, %s) of object (%s, %s) is not type (%s), continue recursing.",
                        pParentMOR->__item.c_str(),
                        pParentMOR->type->c_str(),
                        objectSpec.obj->__item.c_str(),
                        objectSpec.obj->type->c_str(),
                        ancestor_type.c_str());
                    pParentMOR = (Vim25Api1__ManagedObjectReference *)get_ancestor_mor(service_content, pParentMOR, ancestor_type, ancestor_mor_cache);
                }
            }
        }
        if (!pParentMOR){
            LOG(LOG_LEVEL_ERROR, L"Ancestor not found for object (%s, %s) with type (%s).",
                objectSpec.obj->__item.c_str(),
                objectSpec.obj->type->c_str(),
                ancestor_type.c_str());
        }
        else{
            if (!ancestor_mor_cache.count(objectSpec.obj->__item))
                ancestor_mor_cache[objectSpec.obj->__item] = pParentMOR;
            LOG(LOG_LEVEL_INFO, L"Ancestor object (%s, %s) found for object (%s, %s) with type (%s).",
                pParentMOR->__item.c_str(),
                pParentMOR->type->c_str(),
                objectSpec.obj->__item.c_str(),
                objectSpec.obj->type->c_str(),
                ancestor_type.c_str());
        }
    }
    else{
        int lastErrorCode;
        std::wstring lastErrorMessage;
        get_native_error(lastErrorCode, lastErrorMessage);
        LOG(LOG_LEVEL_ERROR, L"__Vim25Api1__RetrieveProperties error: %s (%d)", lastErrorMessage.c_str(), nReturn);
        BOOST_THROW_VMWARE_EXCEPTION(lastErrorCode, lastErrorMessage);
    }
    return pParentMOR;
}

Vim25Api1__ManagedObjectReference* vmware_portal::get_ancestor_mor(Vim25Api1__ServiceContent& service_content, Vim25Api1__ManagedObjectReference* mor, const macho::string_array& types){

    FUN_TRACE;
    Vim25Api1__PropertySpec propertySpec;

    if (!mor){
        LOG(LOG_LEVEL_ERROR, L"mor is NULL.");
        return NULL;
    }

    propertySpec.type = mor->type ? *mor->type : L"";
    propertySpec.all = &xsd_false;
    propertySpec.pathSet.push_back(L"parent");

    Vim25Api1__ObjectSpec objectSpec;
    objectSpec.obj = mor;
    objectSpec.skip = &xsd_false;

    Vim25Api1__PropertyFilterSpec propertyFilterSpec;
    propertyFilterSpec.propSet.push_back(&propertySpec);
    propertyFilterSpec.objectSet.push_back(&objectSpec);

    Vim25Api1__RetrievePropertiesRequestType retrievePropertiesReq;
    retrievePropertiesReq._USCOREthis = service_content.propertyCollector;
    retrievePropertiesReq.specSet.push_back(&propertyFilterSpec);

    _Vim25Api1__RetrievePropertiesResponse retrievePropertiesRes;

    int nReturn = 0;

    {
        journal_transaction trans(this);
        nReturn = _vim_binding.RetrieveProperties(&retrievePropertiesReq, retrievePropertiesRes);
    }

    if (SOAP_OK != nReturn){
        int lastErrorCode;
        std::wstring lastErrorMessage;
        get_native_error(lastErrorCode, lastErrorMessage);
        LOG(LOG_LEVEL_ERROR, L"__Vim25Api1__RetrieveProperties error: %s (%d)", lastErrorMessage.c_str(), nReturn);
        BOOST_THROW_VMWARE_EXCEPTION(lastErrorCode, lastErrorMessage);
        return NULL;
    }

    if (retrievePropertiesRes.returnval.empty()){
        return NULL;
    }

    Vim25Api1__ObjectContent* objectContent = retrievePropertiesRes.returnval.at(0);
    if (!objectContent || objectContent->propSet.empty()){
        return NULL;
    }

    Vim25Api1__DynamicProperty* dynamicProperty = objectContent->propSet.at(0);
    if (!dynamicProperty || SOAP_TYPE_Vim25Api1__ManagedObjectReference != dynamicProperty->val->soap_type()){
        return NULL;
    }

    Vim25Api1__ManagedObjectReference* ancestor =
        reinterpret_cast<Vim25Api1__ManagedObjectReference*>(dynamicProperty->val);

    for (string_array::const_iterator i = types.begin(); i != types.end(); ++i){
        if (0 == _wcsicmp(i->c_str(), ancestor->type ? ancestor->type->c_str() : L"")){
            return ancestor;
        }
    }
    return get_ancestor_mor(service_content, ancestor, types);
}

LPVOID  vmware_portal::get_property_from_mor(Vim25Api1__ServiceContent& service_content, Vim25Api1__ManagedObjectReference& mor, const std::wstring& property_name){
    
    FUN_TRACE;
    LPVOID                          pProperty = NULL;
    int                             nReturn;
    Vim25Api1__PropertySpec               propertySpec;
    Vim25Api1__ObjectSpec                 objectSpec;
    Vim25Api1__PropertyFilterSpec         propertyFilterSpec;
    Vim25Api1__DynamicProperty            dynamicProperty;
    Vim25Api1__RetrievePropertiesRequestType  retrievePropertiesReq;
    _Vim25Api1__RetrievePropertiesResponse    retrievePropertiesRes;

    propertySpec.type = mor.type->c_str();
    propertySpec.all = &xsd_false;
    propertySpec.pathSet.push_back(property_name);

    objectSpec.obj = (Vim25Api1__ManagedObjectReference *)&mor;
    objectSpec.skip = &xsd_false;

    propertyFilterSpec.propSet.push_back(&propertySpec);
    propertyFilterSpec.objectSet.push_back(&objectSpec);

    retrievePropertiesReq._USCOREthis = service_content.propertyCollector;
    retrievePropertiesReq.specSet.push_back(&propertyFilterSpec);

    journal_transaction trans(this);

    if (SOAP_OK == (nReturn = _vim_binding.RetrieveProperties(&retrievePropertiesReq, retrievePropertiesRes))){
        if (retrievePropertiesRes.returnval.size()){
            if (retrievePropertiesRes.returnval[0]->propSet.size())
                dynamicProperty = *retrievePropertiesRes.returnval[0]->propSet[0];
            pProperty = (LPVOID)dynamicProperty.val;
        }
    }
    else{
        int lastErrorCode;
        std::wstring lastErrorMessage;
        get_native_error(lastErrorCode, lastErrorMessage);
        LOG(LOG_LEVEL_ERROR, L"__Vim25Api1__RetrieveProperties error: %s (%d)", lastErrorMessage.c_str(), nReturn);
        BOOST_THROW_VMWARE_EXCEPTION(lastErrorCode, lastErrorMessage);
    }
    return pProperty;
}

VOID vmware_portal::get_vm_mor_key_name_map(Vim25Api1__ServiceContent& service_content, key_name::map& more_key_name_map){

    FUN_TRACE;
    std::wstring dataCenterTraversalSpecName(L"DataCenterTraversalSpec");
    std::wstring folderTraversalSpecName(L"FolderTraversalSpec");

    Vim25Api1__SelectionSpec dataCenterSelectionSpec;
    dataCenterSelectionSpec.name = &dataCenterTraversalSpecName;

    Vim25Api1__SelectionSpec folderSelectionSpec;
    folderSelectionSpec.name = &folderTraversalSpecName;

    Vim25Api1__TraversalSpec dataCenterTraversalSpec;
    dataCenterTraversalSpec.name = &dataCenterTraversalSpecName;
    dataCenterTraversalSpec.type = L"Datacenter";
    dataCenterTraversalSpec.path = L"vmFolder";
    dataCenterTraversalSpec.skip = &xsd_true;
    dataCenterTraversalSpec.selectSet.push_back(&folderSelectionSpec);

    Vim25Api1__TraversalSpec folderTraversalSpec;
    folderTraversalSpec.name = &folderTraversalSpecName;
    folderTraversalSpec.type = L"Folder";
    folderTraversalSpec.path = L"childEntity";
    folderTraversalSpec.skip = &xsd_false;
    folderTraversalSpec.selectSet.push_back(&dataCenterSelectionSpec);
    folderTraversalSpec.selectSet.push_back(&folderSelectionSpec);

    Vim25Api1__ObjectSpec objectSpec;
    objectSpec.obj = service_content.rootFolder;
    objectSpec.skip = &xsd_true;
    objectSpec.selectSet.push_back(&dataCenterTraversalSpec);
    objectSpec.selectSet.push_back(&folderTraversalSpec);

    Vim25Api1__PropertySpec propertySpec;
    propertySpec.type = L"VirtualMachine";
    propertySpec.all = &xsd_false;
    propertySpec.pathSet.push_back(L"name");
    propertySpec.pathSet.push_back(L"config.template");
    propertySpec.pathSet.push_back(L"config.uuid");

    Vim25Api1__PropertyFilterSpec propertyFilterSpec;
    propertyFilterSpec.propSet.push_back(&propertySpec);
    propertyFilterSpec.objectSet.push_back(&objectSpec);

    Vim25Api1__RetrievePropertiesRequestType retrievePropertiesReq;
    retrievePropertiesReq._USCOREthis = service_content.propertyCollector;
    retrievePropertiesReq.specSet.push_back(&propertyFilterSpec);

    _Vim25Api1__RetrievePropertiesResponse retrievePropertiesRes;

    int nReturn = 0;

    {
        journal_transaction trans(this);
        nReturn = _vim_binding.RetrieveProperties(&retrievePropertiesReq, retrievePropertiesRes);
    }

    if (SOAP_OK != nReturn){
        int lastErrorCode;
        std::wstring lastErrorMessage;
        get_native_error(lastErrorCode, lastErrorMessage);
        LOG(LOG_LEVEL_ERROR, L"__Vim25Api1__RetrieveProperties error: %s (%d)", lastErrorMessage.c_str(), nReturn);
        BOOST_THROW_VMWARE_EXCEPTION(lastErrorCode, lastErrorMessage);
    }

    for (size_t iObj = 0; iObj < retrievePropertiesRes.returnval.size(); iObj++){
        Vim25Api1__ObjectContent* pObj = retrievePropertiesRes.returnval.at(iObj);
        if (!pObj || !pObj->obj){
            continue;
        }

        key_name _key_name;
        bool is_template;

        for (size_t iProp = 0; iProp < pObj->propSet.size(); iProp++){
            is_template = false;
            Vim25Api1__DynamicProperty* pProp = pObj->propSet.at(iProp);
            if (!pProp || !pProp->val){
                continue;
            }
            
            if (pProp->name == L"config.template")
            {
                if (((xsd__boolean *)pProp->val)->__item)
                {
                    is_template = true;
                    break;
                }
            }

            if (pProp->name == L"config.uuid"){
                _key_name.key = ((xsd__string *)pProp->val)->__item.c_str();
            }
            else if (pProp->name == L"name"){
                _key_name.name = ((xsd__string *)pProp->val)->__item.c_str();
            }
        }
        if (!is_template)
            more_key_name_map[pObj->obj->__item] = _key_name;
    }
}

VOID vmware_portal::get_datastore_mor_key_name_map(Vim25Api1__ServiceContent& service_content, key_name::map& more_key_name_map){

    FUN_TRACE;
    std::wstring dataCenterTraversalSpecName(L"DataCenterTraversalSpec");
    std::wstring folderTraversalSpecName(L"FolderTraversalSpec");

    Vim25Api1__SelectionSpec dataCenterSelectionSpec;
    dataCenterSelectionSpec.name = &dataCenterTraversalSpecName;

    Vim25Api1__SelectionSpec folderSelectionSpec;
    folderSelectionSpec.name = &folderTraversalSpecName;

    Vim25Api1__TraversalSpec dataCenterTraversalSpec;
    dataCenterTraversalSpec.name = &dataCenterTraversalSpecName;
    dataCenterTraversalSpec.type = L"Datacenter";
    dataCenterTraversalSpec.path = L"datastore";
    dataCenterTraversalSpec.selectSet.push_back(&folderSelectionSpec);

    Vim25Api1__TraversalSpec folderTraversalSpec;
    folderTraversalSpec.name = &folderTraversalSpecName;
    folderTraversalSpec.type = L"Folder";
    folderTraversalSpec.path = L"childEntity";
    folderTraversalSpec.selectSet.push_back(&dataCenterSelectionSpec);
    folderTraversalSpec.selectSet.push_back(&folderSelectionSpec);

    Vim25Api1__ObjectSpec objectSpec;
    objectSpec.obj = service_content.rootFolder;
    objectSpec.selectSet.push_back(&dataCenterTraversalSpec);
    objectSpec.selectSet.push_back(&folderTraversalSpec);

    Vim25Api1__PropertySpec dataStorePropertySpec;
    dataStorePropertySpec.type = L"Datastore";
    dataStorePropertySpec.all = &xsd_false;
    dataStorePropertySpec.pathSet.push_back(L"name");
    dataStorePropertySpec.pathSet.push_back(L"info");

    Vim25Api1__PropertyFilterSpec propertyFilterSpec;
    propertyFilterSpec.propSet.push_back(&dataStorePropertySpec);
    propertyFilterSpec.objectSet.push_back(&objectSpec);

    Vim25Api1__RetrievePropertiesRequestType retrievePropertiesReq;
    retrievePropertiesReq._USCOREthis = service_content.propertyCollector;
    retrievePropertiesReq.specSet.push_back(&propertyFilterSpec);

    _Vim25Api1__RetrievePropertiesResponse retrievePropertiesRes;

    int nReturn = 0;

    {
        journal_transaction trans(this);
        nReturn = _vim_binding.RetrieveProperties(&retrievePropertiesReq, retrievePropertiesRes);
    }

    if (SOAP_OK != nReturn){
        int lastErrorCode;
        std::wstring lastErrorMessage;
        get_native_error(lastErrorCode, lastErrorMessage);
        LOG(LOG_LEVEL_ERROR, L"__Vim25Api1__RetrieveProperties error: %s (%d)", lastErrorMessage.c_str(), nReturn);
        BOOST_THROW_VMWARE_EXCEPTION(lastErrorCode, lastErrorMessage);
    }

    for (size_t iObject = 0; iObject < retrievePropertiesRes.returnval.size(); iObject++){
        Vim25Api1__ObjectContent* pObjectContent = retrievePropertiesRes.returnval.at(iObject);

        key_name& keyName = more_key_name_map[pObjectContent->obj->__item];

        for (size_t iProperty = 0; iProperty < pObjectContent->propSet.size(); iProperty++){
            Vim25Api1__DynamicProperty* pDynamicProperty = pObjectContent->propSet.at(iProperty);

            if (pDynamicProperty->name == L"info"){
                if (SOAP_TYPE_Vim25Api1__VmfsDatastoreInfo == pDynamicProperty->val->soap_type()){
                    Vim25Api1__VmfsDatastoreInfo* pVmfsDatastoreInfo =
                        reinterpret_cast< Vim25Api1__VmfsDatastoreInfo* >(pDynamicProperty->val);

                    if (pVmfsDatastoreInfo->vmfs){
                        keyName.key = pVmfsDatastoreInfo->vmfs->uuid;
                    }
                }
                else if (SOAP_TYPE_Vim25Api1__NasDatastoreInfo == pDynamicProperty->val->soap_type()){
                    Vim25Api1__NasDatastoreInfo* pNasDatastoreInfo =
                        reinterpret_cast< Vim25Api1__NasDatastoreInfo* >(pDynamicProperty->val);

                    if (pNasDatastoreInfo->nas){
                        keyName.key = pNasDatastoreInfo->nas->remoteHost + L":" + pNasDatastoreInfo->nas->remotePath;
                    }
                }
            }
            else if (pDynamicProperty->name == L"name"){
                keyName.name =
                    reinterpret_cast< xsd__string* >(pDynamicProperty->val)->__item;
            }
        }
    }
}

VOID vmware_portal::get_virtual_network_mor_key_name_map(Vim25Api1__ServiceContent& service_content, mor_name_map& _mor_name_map){

    FUN_TRACE;
    std::wstring dataCenterTraversalSpecName(L"DataCenterTraversalSpec");
    std::wstring folderTraversalSpecName(L"FolderTraversalSpec");

    Vim25Api1__SelectionSpec dataCenterSelectionSpec;
    dataCenterSelectionSpec.name = &dataCenterTraversalSpecName;

    Vim25Api1__SelectionSpec folderSelectionSpec;
    folderSelectionSpec.name = &folderTraversalSpecName;

    Vim25Api1__TraversalSpec dataCenterTraversalSpec;
    dataCenterTraversalSpec.name = &dataCenterTraversalSpecName;
    dataCenterTraversalSpec.type = L"Datacenter";
    dataCenterTraversalSpec.path = L"network";
    dataCenterTraversalSpec.selectSet.push_back(&folderSelectionSpec);

    Vim25Api1__TraversalSpec folderTraversalSpec;
    folderTraversalSpec.name = &folderTraversalSpecName;
    folderTraversalSpec.type = L"Folder";
    folderTraversalSpec.path = L"childEntity";
    folderTraversalSpec.selectSet.push_back(&dataCenterSelectionSpec);
    folderTraversalSpec.selectSet.push_back(&folderSelectionSpec);

    Vim25Api1__ObjectSpec objectSpec;
    objectSpec.obj = service_content.rootFolder;
    objectSpec.selectSet.push_back(&dataCenterTraversalSpec);
    objectSpec.selectSet.push_back(&folderTraversalSpec);

    Vim25Api1__PropertySpec propertySpecNetwork;
    propertySpecNetwork.type = L"Network";
    propertySpecNetwork.all = &xsd_false;
    propertySpecNetwork.pathSet.push_back(L"name");

    Vim25Api1__PropertyFilterSpec propertyFilterSpec;
    propertyFilterSpec.propSet.push_back(&propertySpecNetwork);
    propertyFilterSpec.objectSet.push_back(&objectSpec);

    Vim25Api1__RetrievePropertiesRequestType retrievePropertiesReq;
    retrievePropertiesReq._USCOREthis = service_content.propertyCollector;
    retrievePropertiesReq.specSet.push_back(&propertyFilterSpec);

    _Vim25Api1__RetrievePropertiesResponse retrievePropertiesRes;

    int nReturn = 0;

    {
        journal_transaction trans(this);
        nReturn = _vim_binding.RetrieveProperties(&retrievePropertiesReq, retrievePropertiesRes);
    }

    if (SOAP_OK != nReturn){
        int lastErrorCode;
        std::wstring lastErrorMessage;
        get_native_error(lastErrorCode, lastErrorMessage);
        LOG(LOG_LEVEL_ERROR, L"__Vim25Api1__RetrieveProperties error: %s (%d)", lastErrorMessage.c_str(), nReturn);
        BOOST_THROW_VMWARE_EXCEPTION(lastErrorCode, lastErrorMessage);
    }

    for (size_t iObject = 0; iObject < retrievePropertiesRes.returnval.size(); iObject++){
        Vim25Api1__ObjectContent* pObjectContent = retrievePropertiesRes.returnval.at(iObject);
        for (size_t iProperty = 0; iProperty < pObjectContent->propSet.size(); iProperty++){
            Vim25Api1__DynamicProperty* pDynamicProperty = pObjectContent->propSet.at(iProperty);
            
            if (pDynamicProperty->name == L"name")
            {
                _mor_name_map[pObjectContent->obj->__item] = static_cast< xsd__string* >(pDynamicProperty->val)->__item.c_str();
            }
        }
    }
}

VOID vmware_portal::get_host_mor_name_map(Vim25Api1__ServiceContent& service_content, mor_name_map& _mor_name_map){
    
    FUN_TRACE;
    std::wstring dataCenterTraversalSpecName(L"DataCenterTraversalSpec");
    std::wstring folderTraversalSpecName(L"FolderTraversalSpec");
    std::wstring computeResourceTraversalSpecName(L"ComputeResourceTraversalSpec");
    std::wstring clusterComputeResourceTraversalSpecName(L"ClusterComputeResourceTraversalSpec");

    Vim25Api1__SelectionSpec dataCenterSelectionSpec;
    dataCenterSelectionSpec.name = &dataCenterTraversalSpecName;

    Vim25Api1__SelectionSpec folderSelectionSpec;
    folderSelectionSpec.name = &folderTraversalSpecName;

    Vim25Api1__SelectionSpec computeResourceSelectionSpec;
    computeResourceSelectionSpec.name = &computeResourceTraversalSpecName;

    Vim25Api1__SelectionSpec clusterComputeResourceSelectionSpec;
    clusterComputeResourceSelectionSpec.name = &clusterComputeResourceTraversalSpecName;

    Vim25Api1__TraversalSpec dataCenterTraversalSpec;
    dataCenterTraversalSpec.name = &dataCenterTraversalSpecName;
    dataCenterTraversalSpec.type = L"Datacenter";
    dataCenterTraversalSpec.path = L"hostFolder";
    dataCenterTraversalSpec.selectSet.push_back(&folderSelectionSpec);

    Vim25Api1__TraversalSpec folderTraversalSpec;
    folderTraversalSpec.name = &folderTraversalSpecName;
    folderTraversalSpec.type = L"Folder";
    folderTraversalSpec.path = L"childEntity";
    folderTraversalSpec.selectSet.push_back(&dataCenterSelectionSpec);
    folderTraversalSpec.selectSet.push_back(&folderSelectionSpec);
    folderTraversalSpec.selectSet.push_back(&computeResourceSelectionSpec);
    folderTraversalSpec.selectSet.push_back(&clusterComputeResourceSelectionSpec);

    Vim25Api1__TraversalSpec computeResourceTraversalSpec;
    computeResourceTraversalSpec.name = &computeResourceTraversalSpecName;
    computeResourceTraversalSpec.type = L"ComputeResource";
    computeResourceTraversalSpec.path = L"host";
    computeResourceTraversalSpec.selectSet.push_back(&folderSelectionSpec);

    Vim25Api1__TraversalSpec clusterComputeResourceTraversalSpec;
    clusterComputeResourceTraversalSpec.name = &clusterComputeResourceTraversalSpecName;
    clusterComputeResourceTraversalSpec.type = L"ClusterComputeResource";
    clusterComputeResourceTraversalSpec.path = L"host";
    clusterComputeResourceTraversalSpec.selectSet.push_back(&folderSelectionSpec);

    Vim25Api1__ObjectSpec objectSpec;
    objectSpec.obj = service_content.rootFolder;
    objectSpec.selectSet.push_back(&dataCenterTraversalSpec);
    objectSpec.selectSet.push_back(&folderTraversalSpec);
    objectSpec.selectSet.push_back(&computeResourceTraversalSpec);
    objectSpec.selectSet.push_back(&clusterComputeResourceTraversalSpec);

    Vim25Api1__PropertySpec propertySpec;
    propertySpec.type = L"HostSystem";
    propertySpec.all = &xsd_false;
    propertySpec.pathSet.push_back(L"config.network.dnsConfig.hostName");

    Vim25Api1__PropertyFilterSpec propertyFilterSpec;
    propertyFilterSpec.propSet.push_back(&propertySpec);
    propertyFilterSpec.objectSet.push_back(&objectSpec);

    Vim25Api1__RetrievePropertiesRequestType retrievePropertiesReq;
    retrievePropertiesReq._USCOREthis = service_content.propertyCollector;
    retrievePropertiesReq.specSet.push_back(&propertyFilterSpec);

    _Vim25Api1__RetrievePropertiesResponse retrievePropertiesRes;

    int nReturn = 0;

    {
        journal_transaction trans(this);
        nReturn = _vim_binding.RetrieveProperties(&retrievePropertiesReq, retrievePropertiesRes);
    }

    if (SOAP_OK != nReturn){
        int lastErrorCode;
        std::wstring lastErrorMessage;
        get_native_error(lastErrorCode, lastErrorMessage);
        LOG(LOG_LEVEL_ERROR, L"__Vim25Api1__RetrieveProperties error: %s (%d)", lastErrorMessage.c_str(), nReturn);
        BOOST_THROW_VMWARE_EXCEPTION(lastErrorCode, lastErrorMessage); 
    }

    for (size_t iObject = 0; iObject < retrievePropertiesRes.returnval.size(); iObject++){
        Vim25Api1__ObjectContent* pObjectContent = retrievePropertiesRes.returnval.at(iObject);

        for (size_t iProperty = 0; iProperty < pObjectContent->propSet.size(); iProperty++){
            Vim25Api1__DynamicProperty* pDynamicProperty = pObjectContent->propSet.at(iProperty);

            if (pDynamicProperty->name == L"config.network.dnsConfig.hostName"){
                _mor_name_map[pObjectContent->obj->__item] = static_cast< xsd__string* >(pDynamicProperty->val)->__item.c_str();
            }
        }
    }
}

vmware_virtual_network_adapter::ptr vmware_portal::get_virtual_network_adapter_from_object(Vim25Api1__ServiceContent& service_content, mor_name_map& network_mor_name_map, Vim25Api1__VirtualEthernetCard& virtual_ethernet_card){   
    FUN_TRACE;
    vmware_virtual_network_adapter::ptr adapter_ptr(new vmware_virtual_network_adapter());
    adapter_ptr->mac_address = virtual_ethernet_card.macAddress ? *virtual_ethernet_card.macAddress : L"";
    adapter_ptr->name = virtual_ethernet_card.deviceInfo ? virtual_ethernet_card.deviceInfo->label : L"";
    adapter_ptr->key = virtual_ethernet_card.key;
    adapter_ptr->address_type = virtual_ethernet_card.addressType ? *virtual_ethernet_card.addressType : L"";
    Vim25Api1__VirtualDeviceDeviceBackingInfo *device_backing_info = dynamic_cast< Vim25Api1__VirtualDeviceDeviceBackingInfo * >(virtual_ethernet_card.backing);
    if (device_backing_info){
        adapter_ptr->network = device_backing_info->deviceName;
    }
    else{
        Vim25Api1__VirtualEthernetCardDistributedVirtualPortBackingInfo* virtual_ethernet_card_distributed_virtual_port_backing_info = dynamic_cast< Vim25Api1__VirtualEthernetCardDistributedVirtualPortBackingInfo * >(virtual_ethernet_card.backing);
        if (virtual_ethernet_card_distributed_virtual_port_backing_info && virtual_ethernet_card_distributed_virtual_port_backing_info->port && virtual_ethernet_card_distributed_virtual_port_backing_info->port->portgroupKey){
            std::wstring network;
            mor_name_map::iterator iNetwork = network_mor_name_map.find(*virtual_ethernet_card_distributed_virtual_port_backing_info->port->portgroupKey);
            if (iNetwork != network_mor_name_map.end()){
                network = iNetwork->second;
            }
            else{
                Vim25Api1__ManagedObjectReference networkMor;
                networkMor.__item = *virtual_ethernet_card_distributed_virtual_port_backing_info->port->portgroupKey;
                std::wstring type = L"DistributedVirtualPortgroup";
                networkMor.type = &type;
                try
                {
                    get_virtual_network_name_from_mor(service_content, networkMor, network);
                }
                catch (...){}
                network_mor_name_map[*virtual_ethernet_card_distributed_virtual_port_backing_info->port->portgroupKey] = network;
            }

            adapter_ptr->network = network;
            adapter_ptr->port_group = *virtual_ethernet_card_distributed_virtual_port_backing_info->port->portgroupKey;
        }
    }

    if (virtual_ethernet_card.connectable){
        adapter_ptr->is_connected = virtual_ethernet_card.connectable->connected;
        adapter_ptr->is_start_connected = virtual_ethernet_card.connectable->startConnected;
        adapter_ptr->is_allow_guest_control = virtual_ethernet_card.connectable->allowGuestControl;
    }

    switch (virtual_ethernet_card.soap_type()){
    case SOAP_TYPE_Vim25Api1__VirtualE1000:
        adapter_ptr->type = L"E1000";
        break;
    case SOAP_TYPE_Vim25Api1__VirtualE1000e:
        adapter_ptr->type = L"E1000E";
        break;
    case SOAP_TYPE_Vim25Api1__VirtualVmxnet:
        adapter_ptr->type = L"Vmxnet";
        break;
    case SOAP_TYPE_Vim25Api1__VirtualVmxnet2:
        adapter_ptr->type = L"Vmxnet2";
        break;
    case SOAP_TYPE_Vim25Api1__VirtualVmxnet3:
        adapter_ptr->type = L"Vmxnet3";
        break;
    case SOAP_TYPE_Vim25Api1__VirtualPCNet32:
        //cSanHvVirtualNIC.m_wszType = L"PCNet32";
        adapter_ptr->type = L"Flexible";
        break;
        /*
        Should portal set default NIC type if it is unknown/unsupported?
        default:
        break;
        */
    }
    return adapter_ptr;
}

vmware_virtual_network::vtr vmware_portal::get_virtual_networks_internal(Vim25Api1__ServiceContent& service_content, std::vector<Vim25Api1__ManagedObjectReference*> hosts, const std::wstring& network_key){
    FUN_TRACE;
    vmware_virtual_network::vtr   vmware_virtual_network_vtr;
    std::vector<Vim25Api1__ManagedObjectReference*> networks;
    for (size_t iHost = 0; iHost < hosts.size(); iHost++){
        Vim25Api1__ManagedObjectReference& host = *hosts.at(iHost);

        Vim25Api1__ArrayOfManagedObjectReference* pNetworks = (Vim25Api1__ArrayOfManagedObjectReference*)get_property_from_mor(service_content, host, L"network");
        if (!pNetworks){
            std::wstring err = boost::str(boost::wformat(L"Failed to get networks for host (%s).") % host.__item.c_str());
            LOG(LOG_LEVEL_ERROR, err.c_str());
            BOOST_THROW_VMWARE_EXCEPTION(HV_ERROR_NETWORK_GET_FAILED, err);
        }

        std::vector<Vim25Api1__ManagedObjectReference*> findNetworks(networks);
        networks.clear();
        for (size_t iNetwork = 0; iNetwork < pNetworks->ManagedObjectReference.size(); iNetwork++){
            Vim25Api1__ManagedObjectReference* pNetwork = pNetworks->ManagedObjectReference.at(iNetwork);
            if (0 == iHost){
                networks.push_back(pNetwork);
            }
            else{
                for (size_t iFindNetwork = 0; iFindNetwork < findNetworks.size(); iFindNetwork++){
                    if (findNetworks.at(iFindNetwork)->__item == pNetwork->__item){
                        networks.push_back(pNetwork);
                        break;
                    }
                }
            }
        }
    }

    for (size_t i = 0; i < networks.size(); ++i){
        Vim25Api1__ManagedObjectReference* pNetwork = networks.at(i);
        if (!pNetwork){
            continue;
        }
        vmware_virtual_network::ptr vmware_virtual_network_ptr;
        try{
            vmware_virtual_network_ptr = get_virtual_network_from_mor( service_content, *pNetwork );
        }
        catch(...){
            continue;
        }

        if (!network_key.empty() && 0 != _wcsicmp(network_key.c_str(), vmware_virtual_network_ptr->key.c_str())){
            continue;
        }
        vmware_virtual_network_vtr.push_back(vmware_virtual_network_ptr);
    }
    return vmware_virtual_network_vtr;
}

vmware_virtual_network::ptr vmware_portal::get_virtual_network_from_mor(Vim25Api1__ServiceContent& service_content, Vim25Api1__ManagedObjectReference& network){
    FUN_TRACE;
    vmware_virtual_network::ptr     vmware_virtual_network_ptr;
    int                             nReturn;
    Vim25Api1__PropertySpec               propertySpecNetwork;
    Vim25Api1__PropertySpec               propertySpecDistributedPortgroup;
    Vim25Api1__ObjectSpec                 objectSpec;
    Vim25Api1__PropertyFilterSpec         propertyFilterSpec;
    Vim25Api1__DynamicProperty            dynamicProperty;
    Vim25Api1__RetrievePropertiesRequestType  retrievePropertiesReq;
    _Vim25Api1__RetrievePropertiesResponse    retrievePropertiesRes;

    objectSpec.obj = (Vim25Api1__ManagedObjectReference*)&network;
    objectSpec.skip = &xsd_false;

    propertySpecNetwork.type = L"Network";
    propertySpecNetwork.all = &xsd_false;
    propertySpecNetwork.pathSet.push_back(L"name");
    propertySpecNetwork.pathSet.push_back(L"host");
    propertySpecNetwork.pathSet.push_back(L"vm");

    propertySpecDistributedPortgroup.type = L"DistributedVirtualPortgroup";
    propertySpecDistributedPortgroup.all = &xsd_false;
    propertySpecDistributedPortgroup.pathSet.push_back(L"name");
    propertySpecDistributedPortgroup.pathSet.push_back(L"host");
    propertySpecDistributedPortgroup.pathSet.push_back(L"vm");
    propertySpecDistributedPortgroup.pathSet.push_back(L"key");
    propertySpecDistributedPortgroup.pathSet.push_back(L"config.distributedVirtualSwitch");

    propertyFilterSpec.propSet.push_back(&propertySpecNetwork);
    propertyFilterSpec.propSet.push_back(&propertySpecDistributedPortgroup);
    propertyFilterSpec.objectSet.push_back(&objectSpec);

    retrievePropertiesReq._USCOREthis = service_content.propertyCollector;
    retrievePropertiesReq.specSet.push_back(&propertyFilterSpec);

    {
        journal_transaction trans(this);
        nReturn = _vim_binding.RetrieveProperties(&retrievePropertiesReq, retrievePropertiesRes);
    }

    if (SOAP_OK == nReturn){
        vmware_virtual_network_ptr = vmware_virtual_network::ptr(new vmware_virtual_network());
        size_t returnSize = retrievePropertiesRes.returnval.size();
        for (size_t i = 0; i < returnSize; i++){
            Vim25Api1__ObjectContent* pObjectContent = retrievePropertiesRes.returnval[i];
            size_t propSize = pObjectContent->propSet.size();
            for (size_t j = 0; j < propSize; j++){
                dynamicProperty = *pObjectContent->propSet[j];
                if (dynamicProperty.name == L"name"){
                    vmware_virtual_network_ptr->key = vmware_virtual_network_ptr->name = ((xsd__string *)dynamicProperty.val)->__item.c_str();
                }
                else if (dynamicProperty.name == L"host"){
                    Vim25Api1__ArrayOfManagedObjectReference* pHosts = (Vim25Api1__ArrayOfManagedObjectReference*)dynamicProperty.val;
                    if (!pHosts){
                        continue;
                    }

                    for (size_t iHosts = 0; iHosts < pHosts->ManagedObjectReference.size(); ++iHosts){
                        Vim25Api1__ManagedObjectReference* pHost = pHosts->ManagedObjectReference.at(iHosts);
                        if (!pHost){
                            continue;
                        }

                        std::wstring host_key;
                        try
                        {
                            get_host_name_from_mor(service_content, *pHost, host_key);
                            vmware_virtual_network_ptr->hosts.push_back(host_key);
                        }
                        catch (...){
                        }
                    }
                }
                else if (dynamicProperty.name == L"vm"){
                    Vim25Api1__ArrayOfManagedObjectReference* pVms = (Vim25Api1__ArrayOfManagedObjectReference*)dynamicProperty.val;
                    if (!pVms){
                        continue;
                    }

                    for (size_t iVms = 0; iVms < pVms->ManagedObjectReference.size(); ++iVms) {
                        Vim25Api1__ManagedObjectReference* pVm = pVms->ManagedObjectReference.at(iVms);
                        if (!pVm){
                            continue;
                        }

                        std::wstring vm_uuid;
                        try
                        {
                            get_virtual_machine_uuid_from_mor(service_content, *pVm, vm_uuid);
                            vmware_virtual_network_ptr->vms.push_back(vm_uuid);
                        }
                        catch (...){
                        }
                    }
                }
                else if (dynamicProperty.name == L"key"){
                    vmware_virtual_network_ptr->distributed_port_group_key = reinterpret_cast< xsd__string* >(dynamicProperty.val)->__item;
                }
                else if (dynamicProperty.name == L"config.distributedVirtualSwitch"){
                    Vim25Api1__ManagedObjectReference* pDistributedSwitchMor = reinterpret_cast< Vim25Api1__ManagedObjectReference* >(dynamicProperty.val);

                    Vim25Api1__ArrayOfManagedObjectReference* pUplinkPortGroupMors = reinterpret_cast< Vim25Api1__ArrayOfManagedObjectReference* >(get_property_from_mor(service_content, *pDistributedSwitchMor, L"config.uplinkPortgroup"));
                    if (pUplinkPortGroupMors){
                        for (size_t i = 0; i < pUplinkPortGroupMors->ManagedObjectReference.size(); i++){
                            if (pObjectContent->obj->__item == pUplinkPortGroupMors->ManagedObjectReference.at(i)->__item){
                                std::wstring err = boost::str(boost::wformat(L"Distributed network (%s) is an uplink port group, skipping.") % pObjectContent->obj->__item.c_str());
                                LOG(LOG_LEVEL_ERROR, err.c_str());
                                BOOST_THROW_VMWARE_EXCEPTION(HV_ERROR_FAILED_TO_GET_PROPERTY, err);
                            }
                        }
                    }

                    xsd__string* pUuid = reinterpret_cast< xsd__string* >(get_property_from_mor(service_content, *pDistributedSwitchMor, L"uuid"));
                    if (pUuid){
                        vmware_virtual_network_ptr->distributed_switch_uuid = pUuid->__item;
                    }
                }
            }
        }
    }
    else{
        int lastErrorCode;
        std::wstring lastErrorMessage;
        get_native_error(lastErrorCode, lastErrorMessage);
        LOG(LOG_LEVEL_ERROR, L"__Vim25Api1__RetrieveProperties error: %s (%d)", lastErrorMessage.c_str(), nReturn);
        BOOST_THROW_VMWARE_EXCEPTION(lastErrorCode, lastErrorMessage);
    }
    return vmware_virtual_network_ptr;
}

vmware_datastore::ptr vmware_portal::get_datastore_from_mor(Vim25Api1__ServiceContent& service_content, Vim25Api1__ManagedObjectReference& datastore, const mor_name_map& hosts, const key_name::map& vms){
    FUN_TRACE;
    Vim25Api1__ObjectSpec objectSpec;
    objectSpec.obj = reinterpret_cast< Vim25Api1__ManagedObjectReference* >(&datastore);
    objectSpec.skip = &xsd_false;

    Vim25Api1__PropertySpec propertySpec;
    propertySpec.type = L"Datastore";
    propertySpec.all = &xsd_false;
    propertySpec.pathSet.push_back(L"info");
    propertySpec.pathSet.push_back(L"summary");
    if (hosts.size()){
        propertySpec.pathSet.push_back(L"host");
    }
    if (vms.size()){
        propertySpec.pathSet.push_back(L"vm");
    }

    Vim25Api1__PropertyFilterSpec propertyFilterSpec;
    propertyFilterSpec.propSet.push_back(&propertySpec);
    propertyFilterSpec.objectSet.push_back(&objectSpec);

    Vim25Api1__RetrievePropertiesRequestType retrievePropertiesReq;
    retrievePropertiesReq._USCOREthis = service_content.propertyCollector;
    retrievePropertiesReq.specSet.push_back(&propertyFilterSpec);

    int nReturn = SOAP_OK;
    _Vim25Api1__RetrievePropertiesResponse retrievePropertiesRes;

    {
        journal_transaction trans(this);
        nReturn = _vim_binding.RetrieveProperties(&retrievePropertiesReq, retrievePropertiesRes);
    }

    if (SOAP_OK != nReturn){
        int lastErrorCode;
        std::wstring lastErrorMessage;
        get_native_error(lastErrorCode, lastErrorMessage);
        LOG(LOG_LEVEL_ERROR, L"__Vim25Api1__RetrieveProperties error: %s (%d)", lastErrorMessage.c_str(), nReturn);
        BOOST_THROW_VMWARE_EXCEPTION(lastErrorCode, lastErrorMessage);
    }

    if (!retrievePropertiesRes.returnval.empty()){
        Vim25Api1__ObjectContent* pObjectContent = retrievePropertiesRes.returnval.at(0);
        if (pObjectContent){
            return get_datastore_internal(*pObjectContent, hosts, vms);
        }
    }
    return NULL;
}

vmware_datastore::ptr vmware_portal::get_datastore_internal(Vim25Api1__ObjectContent& object_content, const mor_name_map& hosts, const key_name::map& vms){
    FUN_TRACE;
    vmware_datastore::ptr datastore_ptr(new vmware_datastore());
    for (size_t iProperty = 0; iProperty < object_content.propSet.size(); iProperty++){
        Vim25Api1__DynamicProperty* pDynamicProperty = object_content.propSet.at(iProperty);
        if (pDynamicProperty->name == L"info"){
            Vim25Api1__DatastoreInfo* pDataStoreInfo = reinterpret_cast< Vim25Api1__DatastoreInfo* >(pDynamicProperty->val);		
            datastore_ptr->name = pDataStoreInfo->name;
            if (SOAP_TYPE_Vim25Api1__VmfsDatastoreInfo == pDataStoreInfo->soap_type()){
                Vim25Api1__VmfsDatastoreInfo* pVmfsDatastoreInfo = reinterpret_cast< Vim25Api1__VmfsDatastoreInfo* >(pDataStoreInfo);
                Vim25Api1__HostVmfsVolume* pVmfsVolume = pVmfsDatastoreInfo->vmfs;
                if (!pVmfsVolume || pVmfsVolume->extent.empty()){
                    LOG(LOG_LEVEL_ERROR, L"Datastore (%s) does not have any extents, skipping.", datastore_ptr->name.c_str());
                    return NULL;
                }
                datastore_ptr->type = HV_DSTYPE_VMFS;
                for (size_t partitionIndex = 0; partitionIndex < pVmfsVolume->extent.size(); partitionIndex++){
                    Vim25Api1__HostScsiDiskPartition *pScsiDiskPartition = pVmfsVolume->extent.at(partitionIndex);
                    datastore_ptr->lun_unique_names.push_back(pScsiDiskPartition->diskName); //disk canonical name
                }

                datastore_ptr->key = datastore_ptr->uuid = pVmfsVolume->uuid;

                int nMajor = 0;
                int nMinor = 0;
                if (2 == swscanf(pVmfsVolume->version.c_str(), L"%d.%d", &nMajor, &nMinor)){
                    datastore_ptr->version_major = nMajor;
                    datastore_ptr->version_minor = nMinor;
                }
            }
            else if (SOAP_TYPE_Vim25Api1__NasDatastoreInfo == pDataStoreInfo->soap_type()){
                Vim25Api1__NasDatastoreInfo* pNasDatastoreInfo = reinterpret_cast< Vim25Api1__NasDatastoreInfo* >(pDataStoreInfo);

                Vim25Api1__HostNasVolume* pNasVolume = pNasDatastoreInfo->nas;
                if (!pNasVolume || pNasVolume->remoteHost.empty() || pNasVolume->remotePath.empty()){
                    LOG(LOG_LEVEL_ERROR, L"Datastore (%s) does not have a remote host or path, skipping.", datastore_ptr->name.c_str());
                    return NULL;
                }

                datastore_ptr->type = HV_DSTYPE_NFS;
                datastore_ptr->key = datastore_ptr->uuid = pNasVolume->remoteHost + L":" + pNasVolume->remotePath;
            }
            else{
                LOG(LOG_LEVEL_ERROR, L"Datastore (%s) is not VMFS or NFS, skipping.", datastore_ptr->name.c_str());
                return NULL;
            }

        }
        else if (pDynamicProperty->name == L"summary" && SOAP_TYPE_Vim25Api1__DatastoreSummary == pDynamicProperty->val->soap_type()){
            Vim25Api1__DatastoreSummary* pDatastoreSummary = reinterpret_cast< Vim25Api1__DatastoreSummary* >(pDynamicProperty->val);
            datastore_ptr->is_accessible = pDatastoreSummary->accessible;
        }
        else if (pDynamicProperty->name == L"host" && hosts.size()){
            Vim25Api1__ArrayOfDatastoreHostMount* pDatastoreHostMounts =
                reinterpret_cast< Vim25Api1__ArrayOfDatastoreHostMount* >(pDynamicProperty->val);

            for (size_t iDatastoreHostMount = 0; iDatastoreHostMount < pDatastoreHostMounts->DatastoreHostMount.size(); iDatastoreHostMount++){
                mor_name_map::const_iterator iHostName = hosts.find(pDatastoreHostMounts->DatastoreHostMount.at(iDatastoreHostMount)->key->__item);
                if (iHostName != hosts.end()){
                    datastore_ptr->hosts[iHostName->second] = iHostName->second;
                }
            }
        }
        else if (pDynamicProperty->name == L"vm" && vms.size()){
            Vim25Api1__ArrayOfManagedObjectReference* pVmMors =
                reinterpret_cast< Vim25Api1__ArrayOfManagedObjectReference* >(pDynamicProperty->val);

            for (size_t iVmMor = 0; iVmMor < pVmMors->ManagedObjectReference.size(); iVmMor++){
                key_name::map::const_iterator iVmName = vms.find(pVmMors->ManagedObjectReference.at(iVmMor)->__item);
                if (iVmName != vms.end()){
                    datastore_ptr->vms[iVmName->second.key] = iVmName->second.name;
                }
            }
        }
    }
    return datastore_ptr;
}

vmware_folder::vtr vmware_portal::get_vm_folder_internal(Vim25Api1__ServiceContent& service_content, Vim25Api1__ManagedObjectReference& folder, const key_name::map& vm_mor_key_name_map){
    FUN_TRACE;
    vmware_folder::vtr folders;
    Vim25Api1__PropertySpec folderPropertySpec;
    folderPropertySpec.type = L"Folder";
    folderPropertySpec.all = &xsd_false;
    folderPropertySpec.pathSet.push_back(L"name");
    folderPropertySpec.pathSet.push_back(L"childEntity");

    Vim25Api1__ObjectSpec objectSpec;
    objectSpec.obj = &folder;
    objectSpec.skip = &xsd_false;

    Vim25Api1__PropertyFilterSpec propertyFilterSpec;
    propertyFilterSpec.propSet.push_back(&folderPropertySpec);
    propertyFilterSpec.objectSet.push_back(&objectSpec);

    Vim25Api1__RetrievePropertiesRequestType retrievePropertiesReq;
    retrievePropertiesReq._USCOREthis = service_content.propertyCollector;
    retrievePropertiesReq.specSet.push_back(&propertyFilterSpec);

    _Vim25Api1__RetrievePropertiesResponse retrievePropertiesRes;

    int nReturn = 0;

    {
        journal_transaction trans(this);
        nReturn = _vim_binding.RetrieveProperties(&retrievePropertiesReq, retrievePropertiesRes);
    }

    if (SOAP_OK != nReturn){
        int lastErrorCode;
        std::wstring lastErrorMessage;
        get_native_error(lastErrorCode, lastErrorMessage);
        LOG(LOG_LEVEL_ERROR, L"__Vim25Api1__RetrieveProperties error: %s (%d)", lastErrorMessage.c_str(), nReturn);
        BOOST_THROW_VMWARE_EXCEPTION(lastErrorCode, lastErrorMessage);
    }

    for (size_t iReturnval = 0; iReturnval < retrievePropertiesRes.returnval.size(); ++iReturnval){
        Vim25Api1__ObjectContent* objectContent = retrievePropertiesRes.returnval.at(iReturnval);

        vmware_folder::ptr f(new vmware_folder());

        for (size_t iPropSet = 0; iPropSet < objectContent->propSet.size(); ++iPropSet){
            Vim25Api1__DynamicProperty* dynamicProperty = objectContent->propSet.at(iPropSet);

            if (dynamicProperty->name == L"name"){
                xsd__string* name = reinterpret_cast<xsd__string*>(dynamicProperty->val);
                f->name = name ? name->__item : L"";
            }
            else if (dynamicProperty->name == L"childEntity"){
                Vim25Api1__ArrayOfManagedObjectReference* children =
                    reinterpret_cast<Vim25Api1__ArrayOfManagedObjectReference*>(dynamicProperty->val);

                for (size_t iChildren = 0; iChildren < children->ManagedObjectReference.size(); ++iChildren){
                    Vim25Api1__ManagedObjectReference* child =
                        children->ManagedObjectReference.at(iChildren);

                    if (!child || !child->type){
                        continue;
                    }

                    if (*child->type == L"Folder"){
                        vmware_folder::vtr folders = get_vm_folder_internal(service_content, *child, vm_mor_key_name_map);
                        foreach(vmware_folder::ptr p, folders){
                            p->parent = f;
                            f->folders.push_back(p);
                        }
                    }
                    else if (*child->type == L"VirtualMachine"){
                        key_name::map::const_iterator iVm = vm_mor_key_name_map.find(child->__item);
                        if (iVm != vm_mor_key_name_map.end()){
                            f->vms[iVm->second.key] = iVm->second.name;
                        }
                    }
                }
            }
        }
        folders.push_back(f);
    }
    return folders;
}

vmware_folder::vtr vmware_portal::get_vm_folder_internal(Vim25Api1__ServiceContent& service_content, const std::wstring& datacenter, const key_name::map& vm_mor_key_name_map){  
    FUN_TRACE;
    std::vector<Vim25Api1__ManagedObjectReference*> datacenters;
    if (!datacenter.empty()){
        Vim25Api1__ManagedObjectReference* data_center =
            get_datacenter_mor(service_content, datacenter);
        if (!data_center){
            std::wstring message = boost::str(boost::wformat(L"Could not find datacenter (%s).") % datacenter);
            LOG(LOG_LEVEL_ERROR, message.c_str());
            BOOST_THROW_VMWARE_EXCEPTION(HV_ERROR_DATACENTER_GET_FAILED, message);
        }
        datacenters.push_back(data_center);
    }
    else{
        datacenters = get_datacenters_mor(service_content);
    }
    vmware_folder::vtr folders;
    for (size_t iDatacenter = 0; iDatacenter < datacenters.size(); iDatacenter++){
        Vim25Api1__ManagedObjectReference* _datacenter =
            datacenters.at(iDatacenter);

        Vim25Api1__ManagedObjectReference* vmFolder =
            reinterpret_cast<Vim25Api1__ManagedObjectReference*>(get_property_from_mor(service_content, *_datacenter, L"vmFolder"));
        if (!vmFolder){
            std::wstring message = boost::str(boost::wformat(L"Could not get VM folder for datacenter (%s).") % _datacenter->__item);
            LOG(LOG_LEVEL_ERROR, message.c_str());
            BOOST_THROW_VMWARE_EXCEPTION(HV_ERROR_DATACENTER_GET_FAILED, message);
        }
        vmware_folder::vtr _folders = get_vm_folder_internal(service_content, *vmFolder, vm_mor_key_name_map);
        folders.insert(folders.end(), _folders.begin(), _folders.end());
    }
    return folders;
}

std::vector<Vim25Api1__ManagedObjectReference*> vmware_portal::get_datacenters_mor(Vim25Api1__ServiceContent& service_content){
    FUN_TRACE;
    std::vector<Vim25Api1__ManagedObjectReference*> datacenters;
    std::wstring folderTraversalSpecName(L"FolderTraversalSpec");
    Vim25Api1__SelectionSpec folderSelectionSpec;
    folderSelectionSpec.name = &folderTraversalSpecName;

    Vim25Api1__TraversalSpec folderTraversalSpec;
    folderTraversalSpec.name = &folderTraversalSpecName;
    folderTraversalSpec.type = L"Folder";
    folderTraversalSpec.path = L"childEntity";
    folderTraversalSpec.skip = &xsd_false;
    folderTraversalSpec.selectSet.push_back(&folderSelectionSpec);

    Vim25Api1__ObjectSpec objectSpec;
    objectSpec.obj = service_content.rootFolder;
    objectSpec.skip = &xsd_true;
    objectSpec.selectSet.push_back(&folderTraversalSpec);

    Vim25Api1__PropertySpec propertySpec;
    propertySpec.type = L"Datacenter";
    propertySpec.all = &xsd_false;
    propertySpec.pathSet.push_back(L"name");

    Vim25Api1__PropertyFilterSpec propertyFilterSpec;
    propertyFilterSpec.propSet.push_back(&propertySpec);
    propertyFilterSpec.objectSet.push_back(&objectSpec);

    Vim25Api1__RetrievePropertiesRequestType request;
    request._USCOREthis = service_content.propertyCollector;
    request.specSet.push_back(&propertyFilterSpec);

    int nReturn = 0;
    _Vim25Api1__RetrievePropertiesResponse response;
    {
        journal_transaction trans(this);
        nReturn = _vim_binding.RetrieveProperties(&request, response);
    }

    if (SOAP_OK != nReturn){
        int lastErrorCode;
        std::wstring lastErrorMessage;
        get_native_error(lastErrorCode, lastErrorMessage);
        LOG(LOG_LEVEL_ERROR, L"__Vim25Api1__RetrieveProperties error: %s (%d)", lastErrorMessage.c_str(), nReturn);
        BOOST_THROW_VMWARE_EXCEPTION(lastErrorCode, lastErrorMessage);
    }
    else{
        for (size_t i = 0; i < response.returnval.size(); i++){
            datacenters.push_back(response.returnval.at(i)->obj);
        }
    }
    return datacenters;
}

Vim25Api1__ManagedObjectReference* vmware_portal::get_datacenter_mor(Vim25Api1__ServiceContent& service_content, const std::wstring& datacenter){ 
    FUN_TRACE;
    Vim25Api1__PropertySpec datacenterPropertySpec;
    datacenterPropertySpec.type = L"Datacenter";
    datacenterPropertySpec.all = &xsd_false;
    datacenterPropertySpec.pathSet.push_back(L"name");

    Vim25Api1__SelectionSpec folderSelectionSpec;
    folderSelectionSpec.name = soap_new_std__wstring(&_vim_binding, -1);
    *folderSelectionSpec.name = L"Folder";

    Vim25Api1__TraversalSpec folderTraversalSpec;
    folderTraversalSpec.name = folderSelectionSpec.name;
    folderTraversalSpec.type = L"Folder";
    folderTraversalSpec.path = L"childEntity";
    folderTraversalSpec.skip = &xsd_false;
    folderTraversalSpec.selectSet.push_back(&folderSelectionSpec);

    Vim25Api1__ObjectSpec objectSpec;
    objectSpec.obj = service_content.rootFolder;
    objectSpec.skip = &xsd_false;
    objectSpec.selectSet.push_back(&folderTraversalSpec);

    Vim25Api1__PropertyFilterSpec propertyFilterSpec;
    propertyFilterSpec.propSet.push_back(&datacenterPropertySpec);
    propertyFilterSpec.objectSet.push_back(&objectSpec);

    Vim25Api1__RetrievePropertiesRequestType retrievePropertiesReq;
    retrievePropertiesReq._USCOREthis = service_content.propertyCollector;
    retrievePropertiesReq.specSet.push_back(&propertyFilterSpec);
    _Vim25Api1__RetrievePropertiesResponse retrievePropertiesRes;
    int nReturn = 0;
    {
        journal_transaction trans(this);
        nReturn = _vim_binding.RetrieveProperties(&retrievePropertiesReq, retrievePropertiesRes);
    }

    for (size_t iReturnval = 0; iReturnval < retrievePropertiesRes.returnval.size(); ++iReturnval){
        Vim25Api1__ObjectContent* objectContent = retrievePropertiesRes.returnval.at(iReturnval);
        for (size_t iPropSet = 0; iPropSet < objectContent->propSet.size(); ++iPropSet){
            Vim25Api1__DynamicProperty* dynamicProperty = objectContent->propSet.at(iPropSet);
            if (dynamicProperty->name == L"name"){
                xsd__string* name = reinterpret_cast<xsd__string*>(dynamicProperty->val);
                if (name && 0 == _wcsicmp(datacenter.c_str(), name->__item.c_str())){
                    return objectContent->obj;
                }
            }
        }
    }

    //soap_delete_std__wstring(&_vim_binding, folderSelectionSpec.name);

    if (SOAP_OK != nReturn){
        int lastErrorCode;
        std::wstring lastErrorMessage;
        get_native_error(lastErrorCode, lastErrorMessage);
        LOG(LOG_LEVEL_ERROR, L"__Vim25Api1__RetrieveProperties error: %s (%d)", lastErrorMessage.c_str(), nReturn);
        BOOST_THROW_VMWARE_EXCEPTION(lastErrorCode, lastErrorMessage);
    }

    return NULL;
}

Vim25Api1__ManagedObjectReference* vmware_portal::get_host_mor(Vim25Api1__ServiceContent& service_content, const std::wstring& host_key){
    FUN_TRACE;
    std::wstring dataCenterTraversalSpecName(L"DataCenterTraversalSpec");
    std::wstring folderTraversalSpecName(L"FolderTraversalSpec");
    std::wstring computeResourceTraversalSpecName(L"ComputeResourceTraversalSpec");
    std::wstring clusterComputeResourceTraversalSpecName(L"ClusterComputeResourceTraversalSpec");

    Vim25Api1__SelectionSpec dataCenterSelectionSpec;
    dataCenterSelectionSpec.name = &dataCenterTraversalSpecName;

    Vim25Api1__SelectionSpec folderSelectionSpec;
    folderSelectionSpec.name = &folderTraversalSpecName;

    Vim25Api1__SelectionSpec computeResourceSelectionSpec;
    computeResourceSelectionSpec.name = &computeResourceTraversalSpecName;

    Vim25Api1__SelectionSpec clusterComputeResourceSelectionSpec;
    clusterComputeResourceSelectionSpec.name = &clusterComputeResourceTraversalSpecName;

    Vim25Api1__TraversalSpec dataCenterTraversalSpec;
    dataCenterTraversalSpec.name = &dataCenterTraversalSpecName;
    dataCenterTraversalSpec.type = L"Datacenter";
    dataCenterTraversalSpec.path = L"hostFolder";
    dataCenterTraversalSpec.selectSet.push_back(&folderSelectionSpec);

    Vim25Api1__TraversalSpec folderTraversalSpec;
    folderTraversalSpec.name = &folderTraversalSpecName;
    folderTraversalSpec.type = L"Folder";
    folderTraversalSpec.path = L"childEntity";
    folderTraversalSpec.selectSet.push_back(&dataCenterSelectionSpec);
    folderTraversalSpec.selectSet.push_back(&folderSelectionSpec);
    folderTraversalSpec.selectSet.push_back(&computeResourceSelectionSpec);
    folderTraversalSpec.selectSet.push_back(&clusterComputeResourceSelectionSpec);

    Vim25Api1__TraversalSpec computeResourceTraversalSpec;
    computeResourceTraversalSpec.name = &computeResourceTraversalSpecName;
    computeResourceTraversalSpec.type = L"ComputeResource";
    computeResourceTraversalSpec.path = L"host";
    computeResourceTraversalSpec.selectSet.push_back(&folderSelectionSpec);

    Vim25Api1__TraversalSpec clusterComputeResourceTraversalSpec;
    clusterComputeResourceTraversalSpec.name = &clusterComputeResourceTraversalSpecName;
    clusterComputeResourceTraversalSpec.type = L"ClusterComputeResource";
    clusterComputeResourceTraversalSpec.path = L"host";
    clusterComputeResourceTraversalSpec.selectSet.push_back(&folderSelectionSpec);

    Vim25Api1__ObjectSpec objectSpec;
    objectSpec.obj = service_content.rootFolder;
    objectSpec.selectSet.push_back(&dataCenterTraversalSpec);
    objectSpec.selectSet.push_back(&folderTraversalSpec);
    objectSpec.selectSet.push_back(&computeResourceTraversalSpec);
    objectSpec.selectSet.push_back(&clusterComputeResourceTraversalSpec);

    Vim25Api1__PropertySpec propertySpec;
    propertySpec.type = L"HostSystem";
    propertySpec.all = &xsd_false;
    propertySpec.pathSet.push_back(L"config.network.dnsConfig.hostName");

    Vim25Api1__PropertyFilterSpec propertyFilterSpec;
    propertyFilterSpec.propSet.push_back(&propertySpec);
    propertyFilterSpec.objectSet.push_back(&objectSpec);

    Vim25Api1__RetrievePropertiesRequestType retrievePropertiesReq;
    retrievePropertiesReq._USCOREthis = service_content.propertyCollector;
    retrievePropertiesReq.specSet.push_back(&propertyFilterSpec);

    _Vim25Api1__RetrievePropertiesResponse retrievePropertiesRes;

    int nReturn = 0;

    {
        journal_transaction trans(this);
        nReturn = _vim_binding.RetrieveProperties(&retrievePropertiesReq, retrievePropertiesRes);
    }

    if (SOAP_OK != nReturn){
        int lastErrorCode;
        std::wstring lastErrorMessage;
        get_native_error(lastErrorCode, lastErrorMessage);
        LOG(LOG_LEVEL_ERROR, L"__Vim25Api1__RetrieveProperties error: %s (%d)", lastErrorMessage.c_str(), nReturn);
        BOOST_THROW_VMWARE_EXCEPTION(lastErrorCode, lastErrorMessage);
    }

    for (size_t iObject = 0; iObject < retrievePropertiesRes.returnval.size(); iObject++){
        Vim25Api1__ObjectContent* pObjectContent = retrievePropertiesRes.returnval.at(iObject);

        for (size_t iProperty = 0; iProperty < pObjectContent->propSet.size(); iProperty++){
            Vim25Api1__DynamicProperty* dynamicProperty = pObjectContent->propSet.at(iProperty);

            if (dynamicProperty->name == L"config.network.dnsConfig.hostName"){
                xsd__string* name = reinterpret_cast<xsd__string*>(dynamicProperty->val);
                if (name && 0 == _wcsicmp(host_key.c_str(), name->__item.c_str())){
                    return (Vim25Api1__ManagedObjectReference *)pObjectContent->obj;
                }
            }
        }
    }

    //soap_delete_std__wstring(&_vim_binding, folderSelectionSpec.name);

    if (SOAP_OK != nReturn){
        int lastErrorCode;
        std::wstring lastErrorMessage;
        get_native_error(lastErrorCode, lastErrorMessage);
        LOG(LOG_LEVEL_ERROR, L"__Vim25Api1__RetrieveProperties error: %s (%d)", lastErrorMessage.c_str(), nReturn);
        BOOST_THROW_VMWARE_EXCEPTION(lastErrorCode, lastErrorMessage);
    }

    return NULL;
}

vmware_virtual_machine_snapshots::ptr vmware_portal::get_virtual_machine_snapshot_from_object(Vim25Api1__VirtualMachineSnapshotTree& snapshot_tree){
    FUN_TRACE;
    vmware_virtual_machine_snapshots::ptr snapshots_ptr(new vmware_virtual_machine_snapshots());
    snapshots_ptr->snapshot_mor_item = snapshot_tree.snapshot->__item;
    snapshots_ptr->name = snapshot_tree.name;
    snapshots_ptr->description = snapshot_tree.description;
    snapshots_ptr->create_time = snapshot_tree.createTime;
    snapshots_ptr->quiesced = snapshot_tree.quiesced;
    snapshots_ptr->id = *snapshot_tree.id;
    snapshots_ptr->backup_manifest = snapshot_tree.backupManifest != NULL ? std::wstring(snapshot_tree.backupManifest->c_str()) : L"";
    if (snapshot_tree.replaySupported)
        snapshots_ptr->replay_supported = *snapshot_tree.replaySupported;

    std::vector<Vim25Api1__VirtualMachineSnapshotTree *> child_snap = snapshot_tree.childSnapshotList;

    for (std::vector<Vim25Api1__VirtualMachineSnapshotTree *>::iterator it = child_snap.begin(); it != child_snap.end(); ++it)
    {
        Vim25Api1__VirtualMachineSnapshotTree& obj = *(Vim25Api1__VirtualMachineSnapshotTree *)(*it);
        snapshots_ptr->child_snapshot_list.push_back(get_virtual_machine_snapshot_from_object(obj));
    }

    return snapshots_ptr;
}

int vmware_portal::create_virtual_machine_snapshot(const std::wstring &machine_key, vmware_vm_create_snapshot_parm& snapshot_request_parm, operation_progress::slot_type slot)
{
    FUN_TRACE;
    return create_virtual_machine_snapshot_internal(*(Vim25Api1__ServiceContent*)*_connect.get(), machine_key, snapshot_request_parm, slot);
}

int vmware_portal::remove_virtual_machine_snapshot(const std::wstring &machine_key, const vmware_vm_remove_snapshot_parm& snapshot_request_parm, operation_progress::slot_type slot)
{
    FUN_TRACE;
    return remove_virtual_machine_snapshot_internal(*(Vim25Api1__ServiceContent*)*_connect.get(), machine_key, snapshot_request_parm, slot);
}

int vmware_portal::revert_virtual_machine_snapshot(const std::wstring &machine_key, const vmware_vm_revert_to_snapshot_parm& snapshot_request_parm, operation_progress::slot_type slot)
{
    FUN_TRACE;
    return revert_virtual_machine_snapshot_internal(*(Vim25Api1__ServiceContent*)*_connect.get(), machine_key, snapshot_request_parm, slot);
}

VOID vmware_portal::get_vm_mor_from_key_name_map(const std::wstring& machine_key, key_name::map& more_key_name_map, Vim25Api1__ManagedObjectReference& mor)
{
    FUN_TRACE;
    for (key_name::map::iterator it = more_key_name_map.begin(); it != more_key_name_map.end(); ++it)
    {
        key_name keyname = it->second;
        if (!machine_key.empty() && 0 == _wcsicmp(keyname.key.c_str(), machine_key.c_str()))
        {
            mor.__item = it->first;
            mor.type = soap_new_std__wstring(&_vim_binding, -1);
            *mor.type = L"VirtualMachine";
            break;
        }
    }
}

int vmware_portal::create_virtual_machine_snapshot_internal(Vim25Api1__ServiceContent& service_content, const std::wstring &machine_key, vmware_vm_create_snapshot_parm& snapshot_request_parm, operation_progress::slot_type slot)
{
    FUN_TRACE;
    int nReturn = SOAP_OK;
    Vim25Api1__ManagedObjectReference vm_mor;
    Vim25Api1__CreateSnapshotRequestType snapshot_req;
    _Vim25Api1__CreateSnapshot_USCORETaskResponse snapshot_res;
    hv_connection_type type;
    key_name::map                           vm_more_key_name_map;

    vm_mor.__item = L"";
    get_connection_info(service_content, type);
    get_vm_mor_key_name_map(service_content, vm_more_key_name_map);

    get_vm_mor_from_key_name_map(machine_key, vm_more_key_name_map, vm_mor);

    if (!vm_mor.__item.empty())
    {
        snapshot_req.name = snapshot_request_parm.name;
        snapshot_req.description = soap_new_std__wstring(&_vim_binding, -1);
        *snapshot_req.description = snapshot_request_parm.description.c_str();
        snapshot_req.memory = snapshot_request_parm.include_memory;
        snapshot_req.quiesce = snapshot_request_parm.quiesce;
        snapshot_req._USCOREthis = &vm_mor;

        nReturn = _vim_binding.CreateSnapshot_USCORETask(&snapshot_req, snapshot_res);
        //soap_delete_std__wstring(&_vim_binding, snapshot_req.description);
        if (SOAP_OK != nReturn)
        {
            int lastErrorCode;
            std::wstring lastErrorMessage;
            get_native_error(lastErrorCode, lastErrorMessage);
            LOG(LOG_LEVEL_ERROR, L"CreateSnapshot_USCORETask error: %s (%d)", lastErrorMessage.c_str(), nReturn);
            BOOST_THROW_VMWARE_EXCEPTION(lastErrorCode, lastErrorMessage);
        }
        else
        {
            vmware_vm_task_info::ptr task_info(new vmware_vm_task_info());

            task_info->name = L"CreateSnapshot_USCORETask";

            try
            {
                nReturn = wait_task_completion_callback(service_content, *snapshot_res.returnval, *task_info, snapshot_req.name, slot);              
                snapshot_request_parm.snapshot_moref = (task_info->moref);
            }
            catch (boost::exception &e)
            {
                if (unsigned long const *error = boost::get_error_info<throw_errorno>(e))
                    nReturn = *error;
                else
                    nReturn = SOAP_ERR;
            }
        }
    }
    else
        nReturn = DEV_KEY::UNKNOWN_DEV;

    return nReturn;
}

VOID vmware_portal::get_snapshot_mor_from_snapshot_list(vmware_virtual_machine_snapshots::vtr& snapshot_list, const int& snapshot_id, Vim25Api1__ManagedObjectReference &snapshot_mor)
{
    FUN_TRACE;

    if (snapshot_list.size() > 0 && snapshot_id > 0)
    {
        for (vmware_virtual_machine_snapshots::vtr::iterator it = snapshot_list.begin(); it != snapshot_list.end(); ++it)
        {
            vmware_virtual_machine_snapshots::ptr obj = (vmware_virtual_machine_snapshots::ptr)(*it);
            
            if (obj->id == snapshot_id)
            {
                snapshot_mor.__item = obj->snapshot_mor_item;
                return;
            }

            if (obj->child_snapshot_list.size() > 0)
            {
                get_snapshot_mor_from_snapshot_list(obj->child_snapshot_list, snapshot_id, snapshot_mor);
            }
        }
    }
    else
    {
        std::wstring msg = L"Query snapshot managed object reference failure";
        LOG(LOG_LEVEL_ERROR, L"%s, error %d", msg.c_str(), ERROR_INVALID_PARAMETER);
        BOOST_THROW_VMWARE_EXCEPTION(ERROR_INVALID_PARAMETER, msg);
    }
}

int vmware_portal::remove_virtual_machine_snapshot_internal(Vim25Api1__ServiceContent& service_content, const std::wstring &machine_key, const vmware_vm_remove_snapshot_parm& snapshot_request_parm, operation_progress::slot_type slot)
{
    FUN_TRACE;
    int nReturn = SOAP_OK;
    Vim25Api1__ManagedObjectReference snapshot_mor;
    Vim25Api1__RemoveSnapshotRequestType snapshot_req;
    _Vim25Api1__RemoveSnapshot_USCORETaskResponse snapshot_res;
    key_name::map                           vm_more_key_name_map;
    float                               version = 0;
    hv_connection_type                  type = HV_CONNECTION_TYPE_UNKNOWN;

    version = get_connection_info(service_content, type);

    snapshot_mor.__item = L"";

    if (!machine_key.empty())
    {
        if (snapshot_request_parm.mor_ref.empty())
        {
            vmware_virtual_machine::ptr v = get_virtual_machine(machine_key); //bug => null ptr return if vmware session timeout, need fix
            get_snapshot_mor_from_snapshot_list(v->root_snapshot_list, snapshot_request_parm.id, snapshot_mor);
        }
        else
            snapshot_mor.__item = snapshot_request_parm.mor_ref;

        if (!snapshot_mor.__item.empty())
        {
            snapshot_mor.type = soap_new_std__wstring(&_vim_binding, -1);
            *snapshot_mor.type = L"VirtualMachineSnapshot";
            snapshot_req.removeChildren = snapshot_request_parm.remove_children;
            if (version >= 5.0)
                snapshot_req.consolidate = (bool *)&snapshot_request_parm.consolidate;
            snapshot_req._USCOREthis = &snapshot_mor;

            nReturn = _vim_binding.RemoveSnapshot_USCORETask(&snapshot_req, snapshot_res);

            if (SOAP_OK != nReturn)
            {
                int lastErrorCode;
                std::wstring lastErrorMessage;
                get_native_error(lastErrorCode, lastErrorMessage);
                LOG(LOG_LEVEL_ERROR, L"RemoveSnapshot_USCORETask error: %s (%d)", lastErrorMessage.c_str(), nReturn);
                BOOST_THROW_VMWARE_EXCEPTION(lastErrorCode, lastErrorMessage);
            }
            else
            {
                vmware_vm_task_info::ptr task_info(new vmware_vm_task_info());

                task_info->name = L"RemoveSnapshot_USCORETask";
                try
                {
                    nReturn = wait_task_completion_callback(service_content, *snapshot_res.returnval, *task_info, L"Remove snapshot id " + std::to_wstring(snapshot_request_parm.id) + L" of virtual machine " + machine_key, slot);
                }
                catch (boost::exception &e)
                {
                    if (unsigned long const *error = boost::get_error_info<throw_errorno>(e))
                        nReturn = *error;
                    else
                        nReturn = SOAP_ERR;
                }
            }
        }
        else
            nReturn = DEV_KEY::UNKNOWN_DEV;
    }
    else
        nReturn = DEV_KEY::UNKNOWN_DEV;

    return nReturn;
}

int vmware_portal::revert_virtual_machine_snapshot_internal(Vim25Api1__ServiceContent& service_content, const std::wstring &machine_key, const vmware_vm_revert_to_snapshot_parm& snapshot_request_parm, operation_progress::slot_type slot)
{
    FUN_TRACE;
    int nReturn = SOAP_OK;
    Vim25Api1__ManagedObjectReference snapshot_mor;
    Vim25Api1__RevertToSnapshotRequestType snapshot_req;
    _Vim25Api1__RevertToSnapshot_USCORETaskResponse snapshot_res;
    key_name::map                           vm_more_key_name_map;
    float                               version = 0;
    hv_connection_type                  type = HV_CONNECTION_TYPE_UNKNOWN;

    version = get_connection_info(service_content, type);

    snapshot_mor.__item = L"";

    if (!machine_key.empty())
    {
        if (snapshot_request_parm.mor_ref.empty())
        {
            vmware_virtual_machine::ptr v = get_virtual_machine(machine_key); //bug => null ptr return if vmware session timeout, need fix
            get_snapshot_mor_from_snapshot_list(v->root_snapshot_list, snapshot_request_parm.id, snapshot_mor);
        }
        else
            snapshot_mor.__item = snapshot_request_parm.mor_ref;

        if (!snapshot_mor.__item.empty())
        {
            snapshot_mor.type = soap_new_std__wstring(&_vim_binding, -1);
            *snapshot_mor.type = L"VirtualMachineSnapshot";
            snapshot_req.suppressPowerOn = (bool *)&snapshot_request_parm.suppress_power_on;
            snapshot_req._USCOREthis = &snapshot_mor;

            nReturn = _vim_binding.RevertToSnapshot_USCORETask(&snapshot_req, snapshot_res);

            if (SOAP_OK != nReturn)
            {
                int lastErrorCode;
                std::wstring lastErrorMessage;
                get_native_error(lastErrorCode, lastErrorMessage);
                LOG(LOG_LEVEL_ERROR, L"RevertToSnapshot_USCORETask error: %s (%d)", lastErrorMessage.c_str(), nReturn);
                BOOST_THROW_VMWARE_EXCEPTION(lastErrorCode, lastErrorMessage);
            }
            else
            {
                vmware_vm_task_info::ptr task_info(new vmware_vm_task_info());

                task_info->name = L"RevertToSnapshot_USCORETask";
                try
                {
                    nReturn = wait_task_completion_callback(service_content, *snapshot_res.returnval, *task_info, L"Revert snapshot id " + std::to_wstring(snapshot_request_parm.id) + L" of virtual machine " + machine_key, slot);
                }
                catch (boost::exception &e)
                {
                    if (unsigned long const *error = boost::get_error_info<throw_errorno>(e))
                        nReturn = *error;
                    else
                        nReturn = SOAP_ERR;
                }
            }
        }
        else
            nReturn = DEV_KEY::UNKNOWN_DEV;
    }
    else
        nReturn = DEV_KEY::UNKNOWN_DEV;

    return nReturn;
}

TASK_STATE vmware_portal::wait_task_completion_callback(Vim25Api1__ServiceContent& service_content, Vim25Api1__ManagedObjectReference& task_mor, vmware_vm_task_info& task_info, std::wstring task_display_name, operation_progress::slot_type slot, const DWORD wait_time)
{
    FUN_TRACE;

    int retry_count = 15;
    TASK_STATE state = TASK_STATE::STATE_SUCCESS;
    operation_progress progress;
    std::time_t start_time = std::time(nullptr);
    
    progress.connect(slot);

    while (!_is_interrupted)
    {
        journal_transaction trans(this);
        try
        {
            fetch_task_info(service_content, task_mor, task_info);
        }
        catch (vmware_portal::exception &e)
        {
            throw e;
        }

        state = task_info.state;

        if (state == TASK_STATE::STATE_RUNNING)
        {
            progress(task_display_name, task_info.progress);
            Sleep(2000);
        }
        else if (state == TASK_STATE::STATE_QUEUED)
        {
            Sleep(2000);
            if (--retry_count < 0)
            {
                LOG(LOG_LEVEL_WARNING, L"System busy => %s:%s, task is still in queue.", task_info.name.c_str(), task_display_name.c_str());
                break;
            }
        }
        else
        {
            if (state == TASK_STATE::STATE_SUCCESS)
                LOG(LOG_LEVEL_INFO, L"Task Completed => %s:%s, state (%d)", task_info.name.c_str(), task_display_name.c_str(), state);
            else{
                _last_error_code = state;
                _last_error_message = task_info.error;
                LOG(LOG_LEVEL_ERROR, L"Task Error => %s:%s, state (%d), %s", task_info.name.c_str(), task_display_name.c_str(), state, task_info.error.c_str());
            }
            break;
        }

        if (wait_time != INFINITE && (std::time(nullptr) - start_time > wait_time))
        {
            state = TASK_STATE::STATE_TIMEOUT;
            LOG(LOG_LEVEL_WARNING, L"Task Timeout => %s:%s", task_info.name.c_str(), task_display_name.c_str());
            break;
        }
    }

    progress(task_display_name, task_info.progress);
    //progress.disconnect(slot);
    if (_is_cancel)
    {
        LOG(LOG_LEVEL_WARNING, L"Task cancelling...");
        if (_isatty(_fileno(stdout)))
        {
            std::wcout << L"Cancel event captured, start cancelling procedure..." << std::endl;
            std::cout.flush();
        }

        switch (state)
        {
        case TASK_STATE::STATE_QUEUED:
        case TASK_STATE::STATE_RUNNING:
        case TASK_STATE::STATE_TIMEOUT:
                if (task_info.cancelable)
                {
                    Vim25Api1__CancelTaskRequestType cancel_task_req;
                    _Vim25Api1__CancelTaskResponse cancel_task_res;
                    cancel_task_req._USCOREthis = &task_mor;
                    if (_vim_binding.CancelTask(&cancel_task_req, cancel_task_res) == SOAP_OK)
                    {
                        state = TASK_STATE::STATE_CANCELLED;
                        LOG(LOG_LEVEL_WARNING, L"System busy => %s:%s, task cancelling.", task_info.name.c_str(), task_display_name.c_str());
                    }
                }

                break;

            default:
                ;
        }
    }

    return state;
}

TASK_STATE vmware_portal::wait_task_completion(Vim25Api1__ServiceContent& service_content, Vim25Api1__ManagedObjectReference& task_mor, vmware_vm_task_info& task_info, std::wstring task_display_name, const DWORD wait_time)
{
    FUN_TRACE;

    int retry_count = 15;
    TASK_STATE state = TASK_STATE::STATE_SUCCESS;
    std::time_t start_time = std::time(nullptr);

    while (!_is_interrupted)
    {
        journal_transaction trans(this);
        try
        {
            fetch_task_info(service_content, task_mor, task_info);
        }
        catch (vmware_portal::exception &e)
        {
            throw e;
        }

        state = task_info.state;

        if (state == TASK_STATE::STATE_RUNNING)
            Sleep(2000);
        else if (state == TASK_STATE::STATE_QUEUED)
        {
            Sleep(2000);
            if (--retry_count < 0)
            {
                LOG(LOG_LEVEL_WARNING, L"System busy => %s:%s, task queued too long.", task_info.name.c_str(), task_display_name.c_str());
                break;
            }
        }
        else
        {
            if (state == TASK_STATE::STATE_SUCCESS)
                LOG(LOG_LEVEL_INFO, L"Task Completed => %s:%s, state (%d)", task_info.name.c_str(), task_display_name.c_str(), state);
            else{
                _last_error_code = state;
                _last_error_message = task_info.error;
                LOG(LOG_LEVEL_ERROR, L"Task Error => %s:%s, state (%d), %s", task_info.name.c_str(), task_display_name.c_str(), state, task_info.error.c_str());
            }
            break;
        }

        if (wait_time != INFINITE && (std::time(nullptr) - start_time > wait_time))
        {
            state = TASK_STATE::STATE_TIMEOUT;
            LOG(LOG_LEVEL_ERROR, L"Task Timeout => %s:%s", task_info.name.c_str(), task_display_name.c_str());
            break;
        }
    }

    if (_is_cancel)
    {
        LOG(LOG_LEVEL_WARNING, L"Task cancelling...");
        if (_isatty(_fileno(stdout)))
        {
            std::wcout << L"Cancel event captured, start cancelling procedure..." << std::endl;
            std::cout.flush();
        }

        switch (state)
        {
        case TASK_STATE::STATE_QUEUED:
        case TASK_STATE::STATE_RUNNING:
        case TASK_STATE::STATE_TIMEOUT:
                if (task_info.cancelable)
                {
                    Vim25Api1__CancelTaskRequestType cancel_task_req;
                    _Vim25Api1__CancelTaskResponse cancel_task_res;
                    cancel_task_req._USCOREthis = &task_mor;
                    if (_vim_binding.CancelTask(&cancel_task_req, cancel_task_res) == SOAP_OK)
                    {
                        state = TASK_STATE::STATE_CANCELLED;
                        LOG(LOG_LEVEL_WARNING, L"System busy => %s:%s, task cancelling.", task_info.name.c_str(), task_display_name.c_str());
                    }
                }

                break;

            default:
                ;
        }
    }

    return state;
}

#if 0
VOID vmware_portal::fetch_vm_vmdks_size(Vim25Api1__ServiceContent& service_content, vmware_virtual_machine::vtr vms)
{
    FUN_TRACE;

    int nReturn;
    Vim25Api1__SearchDatastoreRequestType ds_req;
    _Vim25Api1__SearchDatastore_USCORETaskResponse ds_res;

    if (vms.empty())
    {
        LOG(LOG_LEVEL_ERROR, L"Invalid input parameter.");
        BOOST_THROW_VMWARE_EXCEPTION(HV_ERROR_INVALID_ARGUMENT, L"Invalid input parameter.");
    }

    Vim25Api1__VmDiskFileQuery disk_file_query;
    Vim25Api1__VmDiskFileQueryFilter disk_file_query_filter;
    disk_file_query_filter.diskType.push_back(L"VirtualDiskFlatVer2BackingInfo");
    disk_file_query_filter.diskType.push_back(L"VirtualDiskSparseVer2BackingInfo");
    disk_file_query_filter.diskType.push_back(L"VirtualDiskRawDiskMappingVer1BackingInfo");
    disk_file_query.filter = &disk_file_query_filter;

    Vim25Api1__VmDiskFileQueryFlags disk_file_query_flags;
    disk_file_query_flags.capacityKb = true;
    disk_file_query_flags.diskType = true;
    disk_file_query_flags.hardwareVersion = true;
    disk_file_query_flags.controllerType = &xsd_true;
    disk_file_query.details = &disk_file_query_flags;

    Vim25Api1__FolderFileQuery folder_file_query;

    Vim25Api1__FileQueryFlags file_query_flags;
    Vim25Api1__HostDatastoreBrowserSearchSpec search_spec;

    file_query_flags.fileType = true;
    file_query_flags.fileSize = true;
    file_query_flags.modification = true;
    file_query_flags.fileOwner = &xsd_true;

    search_spec.details = &file_query_flags;
    search_spec.matchPattern.push_back(L"*.vmdk");
    search_spec.searchCaseInsensitive = &xsd_true;
    search_spec.sortFoldersFirst = &xsd_true;
    search_spec.query.push_back(&disk_file_query);
    search_spec.query.push_back(&folder_file_query);

    ds_req.searchSpec = &search_spec;

    foreach(vmware_virtual_machine::ptr vm, vms)
    {
        Vim25Api1__ManagedObjectReference datastore_browser_mor;
        datastore_browser_mor.__item = vm->datastore_browser_mor_item;

        ds_req._USCOREthis = &datastore_browser_mor;

        for (key_map::iterator disk = vm->disks_map.begin(); disk != vm->disks_map.end(); disk++)
        {
            if (vm->disks_size.find((disk->first)) != vm->disks_size.end())
                continue;

            size_t pos = disk->second.find_last_of(L"/");
            std::wstring file_name;

            if (pos)
            {
                ds_req.datastorePath = disk->second.substr(0, pos);
                file_name = disk->second.substr(pos + 1);
                LOG(LOG_LEVEL_DEBUG, L"Search folder %s", ds_req.datastorePath.c_str());
            }
            else
                continue;

            if (_vim_binding.SearchDatastore_USCORETask(&ds_req, ds_res) == SOAP_OK)
            {
                vmware_vm_task_info::ptr task_info(new vmware_vm_task_info());

                task_info->name = L"SearchDatastore_USCORETask";

                Vim25Api1__ManagedObjectReference task_mor = *ds_res.returnval;

                try
                {
                    if ((nReturn = wait_task_completion(service_content, task_mor, *task_info, L"Search vmdk files of " + vm->name)) == TASK_STATE::STATE_SUCCESS)
                    {
                        Vim25Api1__PropertySpec taskPropertySpec;
                        taskPropertySpec.type = L"Task";
                        taskPropertySpec.all = &xsd_false;
                        taskPropertySpec.pathSet.push_back(L"info.result");

                        Vim25Api1__ObjectSpec objectSpec;
                        objectSpec.obj = &task_mor;
                        objectSpec.skip = &xsd_false;

                        Vim25Api1__PropertyFilterSpec propertyFilterSpec;
                        propertyFilterSpec.propSet.push_back(&taskPropertySpec);
                        propertyFilterSpec.objectSet.push_back(&objectSpec);

                        Vim25Api1__RetrievePropertiesRequestType retrievePropertiesReq;
                        retrievePropertiesReq._USCOREthis = service_content.propertyCollector;
                        retrievePropertiesReq.specSet.push_back(&propertyFilterSpec);
                        _Vim25Api1__RetrievePropertiesResponse retrievePropertiesRes;

                        if (_vim_binding.RetrieveProperties(&retrievePropertiesReq, retrievePropertiesRes) == SOAP_OK)
                        {
                            Vim25Api1__ObjectContent* oc = NULL;

                            for (int i = 0; i < retrievePropertiesRes.returnval.size(); i++)
                            {
                                oc = retrievePropertiesRes.returnval[i];

                                for (int j = 0; j < oc->propSet.size(); j++)
                                {
                                    Vim25Api1__DynamicProperty& dynamicProperty = *oc->propSet[j];
                                    if (dynamicProperty.name == L"info.result")
                                    {
                                        Vim25Api1__HostDatastoreBrowserSearchResults* pResult = reinterpret_cast<Vim25Api1__HostDatastoreBrowserSearchResults *>(dynamicProperty.val);

                                        for (std::vector<Vim25Api1__FileInfo *>::iterator p = pResult->file.begin(); p != pResult->file.end(); ++p)
                                        {
                                            Vim25Api1__FileInfo file_info = *(Vim25Api1__FileInfo *)(*p);

                                            if (file_info.path == file_name && vm->disks_size.find((disk->first)) == vm->disks_size.end())
                                            {
                                                vm->disks_size[disk->first] = std::to_wstring(*file_info.fileSize);
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                    else if (nReturn == TASK_STATE::CANCELLED)
                    {
                        LOG(LOG_LEVEL_WARNING, L"Stop to fetch image file size information.");
                        return;
                    }
                }
                catch (boost::exception &e)
                { }
            }
            else
            {
                int lastErrorCode;
                std::wstring lastErrorMessage;

                get_native_error(lastErrorCode, lastErrorMessage);
                LOG(LOG_LEVEL_ERROR, L"SearchDatastore_USCORETask error: %s (%d)", lastErrorMessage.c_str(), lastErrorCode);
            }
        }
    }
}
#endif

VOID vmware_portal::fetch_task_info(Vim25Api1__ServiceContent& service_content, Vim25Api1__ManagedObjectReference& task_mor, vmware_vm_task_info& task_info)
{
    int nReturn = SOAP_OK;
    FUN_TRACE;
    Vim25Api1__PropertySpec taskPropertySpec;
    taskPropertySpec.type = L"Task";
    taskPropertySpec.all = &xsd_false;
    taskPropertySpec.pathSet.push_back(L"info.error");
    taskPropertySpec.pathSet.push_back(L"info.entity");
    taskPropertySpec.pathSet.push_back(L"info.entityName");
    taskPropertySpec.pathSet.push_back(L"info.name");
    taskPropertySpec.pathSet.push_back(L"info.state");
    taskPropertySpec.pathSet.push_back(L"info.progress");
    taskPropertySpec.pathSet.push_back(L"info.cancelable");
    taskPropertySpec.pathSet.push_back(L"info.cancelled");
    taskPropertySpec.pathSet.push_back(L"info.result");
    taskPropertySpec.pathSet.push_back(L"info.startTime");
    taskPropertySpec.pathSet.push_back(L"info.completeTime");

    Vim25Api1__ObjectSpec objectSpec;
    objectSpec.obj = &task_mor;
    objectSpec.skip = &xsd_false;

    Vim25Api1__PropertyFilterSpec propertyFilterSpec;
    propertyFilterSpec.propSet.push_back(&taskPropertySpec);
    propertyFilterSpec.objectSet.push_back(&objectSpec);

    Vim25Api1__RetrievePropertiesRequestType retrievePropertiesReq;
    retrievePropertiesReq._USCOREthis = service_content.propertyCollector;
    retrievePropertiesReq.specSet.push_back(&propertyFilterSpec);
    _Vim25Api1__RetrievePropertiesResponse retrievePropertiesRes;

    nReturn = _vim_binding.RetrieveProperties(&retrievePropertiesReq, retrievePropertiesRes);

    if (SOAP_OK != nReturn)
    {
        int lastErrorCode;
        std::wstring lastErrorMessage;
        get_native_error(lastErrorCode, lastErrorMessage);
        LOG(LOG_LEVEL_ERROR, L"%s error: %s (%d)", task_info.name.c_str(), lastErrorMessage.c_str(), nReturn);
        BOOST_THROW_VMWARE_EXCEPTION(lastErrorCode, lastErrorMessage);
    }

    for (size_t iReturnval = 0; iReturnval < retrievePropertiesRes.returnval.size(); ++iReturnval)
    {
        Vim25Api1__ObjectContent* objectContent = retrievePropertiesRes.returnval.at(iReturnval);
        _fetch_task_info(service_content, objectContent, task_info);
    }
}

bool vmware_portal::enable_change_block_tracking(const std::wstring &machine_key)
{
    FUN_TRACE;

    vmware_virtual_machine::ptr vm = get_virtual_machine(machine_key);

    if (vm && vm->is_cbt_enabled == true)
        return true;
    else if (vm->version < 7)
        return false;
    else
    {
        return changeblocktracking_setting_internal(*(Vim25Api1__ServiceContent*)*_connect.get(), vm->vm_mor_item, true);
    }
}

bool vmware_portal::disable_change_block_tracking(const std::wstring &machine_key)
{
    FUN_TRACE;

    vmware_virtual_machine::ptr vm = get_virtual_machine(machine_key);

    if (vm && !vm->is_cbt_enabled)
        return true;
    else if (vm->version < 7)
        return false;
    else
    {
        return changeblocktracking_setting_internal(*(Vim25Api1__ServiceContent*)*_connect.get(), vm->vm_mor_item, false);
    }
}

bool vmware_portal::changeblocktracking_setting_internal(Vim25Api1__ServiceContent& service_content, const std::wstring& vm_mor_item, const bool& value)
{
    FUN_TRACE;

    Vim25Api1__ReconfigVMRequestType req;
    _Vim25Api1__ReconfigVM_USCORETaskResponse res;
    Vim25Api1__VirtualMachineConfigSpec spec;
    Vim25Api1__ManagedObjectReference mor;
    bool cbt_enable = value;
    bool result = true;
    int nReturn;
    mor.__item = vm_mor_item;
    mor.type = soap_new_std__wstring(&_vim_binding, -1);
    *mor.type = L"VirtualMachine";
    xsd__boolean* pcbt_enable = (xsd__boolean*)get_property_from_mor(service_content, mor, L"config.changeTrackingEnabled");
    if (pcbt_enable){
        if (cbt_enable == pcbt_enable->__item)
            return true;
    }
    spec.changeTrackingEnabled = &cbt_enable;
    req._USCOREthis = &mor;
    req.spec = &spec;
    if ((nReturn = _vim_binding.ReconfigVM_USCORETask(&req, res)) == SOAP_OK)
    {
        vmware_vm_task_info::ptr task_info(new vmware_vm_task_info());

        task_info->name = L"ReconfigVM_USCORETask";
        std::wstring task_display_name;
        if (cbt_enable)
            task_display_name = L"Enable CBT for " + vm_mor_item;
        else
            task_display_name = L"Disable CBT for " + vm_mor_item;

        Vim25Api1__ManagedObjectReference task_mor = *res.returnval;

        try
        {
            if ((nReturn = wait_task_completion(service_content, task_mor, *task_info, task_display_name)) != TASK_STATE::STATE_SUCCESS && nReturn != TASK_STATE::STATE_QUEUED)
                result = false;
        }
        catch (boost::exception &e)
        {
            result = false;
        }
    }
    else
    {
        int lastErrorCode;
        std::wstring lastErrorMessage;
        get_native_error(lastErrorCode, lastErrorMessage);
        LOG(LOG_LEVEL_ERROR, L"%s error: %s (%d)", L"ReconfigVM_USCORETask", lastErrorMessage.c_str(), nReturn);

        result = false;
    }

    return result;
}

bool vmware_portal::mount_vm_tools(const std::wstring &machine_key){
    FUN_TRACE;
    return mount_vm_tools_internal(*(Vim25Api1__ServiceContent*)*_connect.get(), machine_key);
}

vmware_snapshot_disk_info::vtr vmware_portal::_get_snapshot_info(Vim25Api1__ServiceContent& service_content, Vim25Api1__ManagedObjectReference& snapshot_mor)
{
    FUN_TRACE;
    vmware_snapshot_disk_info::vtr result;
    Vim25Api1__PropertySpec snapPropertySpec;
    snapPropertySpec.type = L"VirtualMachineSnapshot";
    snapPropertySpec.all = &xsd_false;
    snapPropertySpec.pathSet.push_back(L"config.hardware.device");

    Vim25Api1__ObjectSpec objectSpec;
    objectSpec.obj = &snapshot_mor;
    objectSpec.skip = &xsd_false;

    Vim25Api1__PropertyFilterSpec propertyFilterSpec;
    propertyFilterSpec.propSet.push_back(&snapPropertySpec);
    propertyFilterSpec.objectSet.push_back(&objectSpec);

    Vim25Api1__RetrievePropertiesRequestType retrievePropertiesReq;
    retrievePropertiesReq._USCOREthis = service_content.propertyCollector;
    retrievePropertiesReq.specSet.push_back(&propertyFilterSpec);
    _Vim25Api1__RetrievePropertiesResponse retrievePropertiesRes;

    int nReturn = _vim_binding.RetrieveProperties(&retrievePropertiesReq, retrievePropertiesRes);

    if (SOAP_OK == nReturn)
    {
        for (size_t iReturnval = 0; iReturnval < retrievePropertiesRes.returnval.size(); ++iReturnval)
        {
            Vim25Api1__ObjectContent* objectContent = retrievePropertiesRes.returnval.at(iReturnval);
            for (size_t iPropSet = 0; iPropSet < objectContent->propSet.size(); ++iPropSet)
            {
                Vim25Api1__DynamicProperty& dynamicProperty = *objectContent->propSet.at(iPropSet);

                if (dynamicProperty.name == L"config.hardware.device")
                {
                    for (size_t i = 0; i < ((Vim25Api1__ArrayOfVirtualDevice *)dynamicProperty.val)->VirtualDevice.size(); i++)
                    {
                        Vim25Api1__VirtualDisk* pVirtualDisk = dynamic_cast<Vim25Api1__VirtualDisk *>(((Vim25Api1__ArrayOfVirtualDevice *)dynamicProperty.val)->VirtualDevice[i]);

                        if (pVirtualDisk)
                        {
                            if ((pVirtualDisk->backing) && dynamic_cast<Vim25Api1__VirtualDeviceFileBackingInfo *>(pVirtualDisk->backing))
                            {
                                Vim25Api1__VirtualDeviceFileBackingInfo *pFileBackingInfo = dynamic_cast<Vim25Api1__VirtualDeviceFileBackingInfo *>(pVirtualDisk->backing);

                                if (dynamic_cast<Vim25Api1__VirtualDiskFlatVer2BackingInfo *>(pFileBackingInfo))
                                {
                                    Vim25Api1__VirtualDiskFlatVer2BackingInfo *pBackingInfo = dynamic_cast<Vim25Api1__VirtualDiskFlatVer2BackingInfo *>(pFileBackingInfo);
                                    vmware_snapshot_disk_info::ptr snapshot(new vmware_snapshot_disk_info());
                                    snapshot->name = std::wstring(pBackingInfo->fileName.c_str());
                                    snapshot->key = pVirtualDisk->key;
                                    if (pVirtualDisk->capacityInBytes)
                                        snapshot->size = *pVirtualDisk->capacityInBytes;
                                    else
                                        snapshot->size = pVirtualDisk->capacityInKB * 1024;
                                    snapshot->uuid = std::wstring(pBackingInfo->uuid->c_str());
                                    if (pBackingInfo->changeId)
                                        snapshot->change_id = std::wstring(pBackingInfo->changeId->c_str());
                                    result.push_back(snapshot);
                                }

                                if (dynamic_cast<Vim25Api1__VirtualDiskSparseVer2BackingInfo *>(pFileBackingInfo))
                                {
                                    Vim25Api1__VirtualDiskSparseVer2BackingInfo *pBackingInfo = dynamic_cast<Vim25Api1__VirtualDiskSparseVer2BackingInfo *>(pFileBackingInfo);                                                       
                                    vmware_snapshot_disk_info::ptr snapshot(new vmware_snapshot_disk_info());
                                    snapshot->name = std::wstring(pBackingInfo->fileName.c_str());
                                    snapshot->key = pVirtualDisk->key;
                                    if (pVirtualDisk->capacityInBytes)
                                        snapshot->size = *pVirtualDisk->capacityInBytes;
                                    else
                                        snapshot->size = pVirtualDisk->capacityInKB * 1024;
                                    snapshot->uuid = std::wstring(pBackingInfo->uuid->c_str());
                                    if (pBackingInfo->changeId)
                                        snapshot->change_id = std::wstring(pBackingInfo->changeId->c_str());
                                    result.push_back(snapshot);
                                }

                                if (dynamic_cast<Vim25Api1__VirtualDiskSeSparseBackingInfo *>(pFileBackingInfo))
                                {
                                    Vim25Api1__VirtualDiskSeSparseBackingInfo *pBackingInfo = dynamic_cast<Vim25Api1__VirtualDiskSeSparseBackingInfo *>(pFileBackingInfo);
                                    vmware_snapshot_disk_info::ptr snapshot(new vmware_snapshot_disk_info());
                                    snapshot->name = std::wstring(pBackingInfo->fileName.c_str());
                                    snapshot->key = pVirtualDisk->key;
                                    if (pVirtualDisk->capacityInBytes)
                                        snapshot->size = *pVirtualDisk->capacityInBytes;
                                    else
                                        snapshot->size = pVirtualDisk->capacityInKB * 1024;
                                    snapshot->uuid = std::wstring(pBackingInfo->uuid->c_str());
                                    if (pBackingInfo->changeId)
                                        snapshot->change_id = std::wstring(pBackingInfo->changeId->c_str());
                                    result.push_back(snapshot);
                                }
                            }
                            continue;
                        }
                    }
                }
            }
        }
    }
    else
    {
        int lastErrorCode;
        std::wstring lastErrorMessage;
        get_native_error(lastErrorCode, lastErrorMessage);
        LOG(LOG_LEVEL_ERROR, L"%s error: %s (%d)", L"Fetch VirtualMachineSnapshot config", lastErrorMessage.c_str(), nReturn);
    }
    return result;
}

vmdk_changed_areas vmware_portal::get_vmdk_changed_areas(const std::wstring &vm_mor_item, const std::wstring &snapshot_mor_item, const std::wstring &device_key, const std::wstring &changeid, const LONG64 start_offset, const LONG64 disk_size)
{
    FUN_TRACE;
    vmdk_changed_areas result;
    int nReturn = SOAP_OK;
    uint64_t next_start_offset = start_offset;

    while (next_start_offset < disk_size)
    {
        Vim25Api1__QueryChangedDiskAreasRequestType req;
        _Vim25Api1__QueryChangedDiskAreasResponse res;
        Vim25Api1__ManagedObjectReference vm_mor;
        Vim25Api1__ManagedObjectReference snapshot_mor;
        vm_mor.__item = vm_mor_item;
        vm_mor.type = soap_new_std__wstring(&_vim_binding, -1);
        *vm_mor.type = L"VirtualMachine";
        snapshot_mor.__item = snapshot_mor_item;
        snapshot_mor.type = soap_new_std__wstring(&_vim_binding, -1);
        *snapshot_mor.type = L"VirtualMachineSnapshot";

        req._USCOREthis = &vm_mor;
        req.changeId = changeid;
        req.deviceKey = std::stoi(device_key);
        req.snapshot = &snapshot_mor;
        req.startOffset = next_start_offset;

        VimBindingProxy *proxy = &_vim_binding;

        nReturn = proxy->QueryChangedDiskAreas(&req, res);
        if (nReturn == SOAP_OK)
        {
            Vim25Api1__DiskChangeInfo *disk_change_info = res.returnval;

            if (disk_change_info)
            {
                foreach(Vim25Api1__DiskChangeExtent *p, disk_change_info->changedArea)
                {
                    result.changed_list.push_back(changed_disk_extent(p->start, p->length));
                }
                next_start_offset += disk_change_info->length;
            }
        }
        else
        {
            get_native_error(result.last_error_code, result.error_description);         
            LOG(LOG_LEVEL_ERROR, L"%s (%d)", result.error_description.c_str(), result.last_error_code);
            break;
        }
    }
    return result;
}

bool vmware_portal::interrupt(bool is_cancel)
{
    FUN_TRACE;
    _is_interrupted = true;
    _is_cancel = is_cancel;
    return _is_interrupted;
}

vmware_snapshot_disk_info::map  vmware_portal::get_snapshot_info(const std::wstring& snapshot_mof, vmware_snapshot_disk_info::vtr& snapshot_disk_infos)
{
    FUN_TRACE;
    vmware_snapshot_disk_info::map result;
    boost::shared_ptr<Vim25Api1__ManagedObjectReference> snapshot_mor(new Vim25Api1__ManagedObjectReference());

    if (!snapshot_mof.empty())
    {
        snapshot_mor->__item = snapshot_mof;
        snapshot_mor->type = soap_new_std__wstring(&_vim_binding, -1);
        snapshot_mor->type->assign(L"VirtualMachineSnapshot");
        snapshot_disk_infos = _get_snapshot_info(*(Vim25Api1__ServiceContent*)*_connect.get(), *snapshot_mor.get());
        //soap_delete_std__wstring(&_vim_binding, snapshot_mor->type);
        foreach(vmware_snapshot_disk_info::ptr snapshot, snapshot_disk_infos){
            if (snapshot->uuid.empty())
                result[snapshot->wsz_key()] = snapshot;
            else
                result[snapshot->uuid] = snapshot;
        }
    }
    return result;
}

license_map vmware_portal::get_supported_features(Vim25Api1__ServiceContent& service_content)
{
    //Deprecated as of vSphere API 6.0 for vCenter Impl use cis.license.management.SystemManagementService 
    //For ESX it is still supported.
    FUN_TRACE;
    int lastErrorCode;
    std::wstring lastErrorMessage;
    Vim25Api1__RetrievePropertiesRequestType req;
    _Vim25Api1__RetrievePropertiesResponse res;
    Vim25Api1__PropertySpec licPropertySpec;
    Vim25Api1__PropertyFilterSpec propertyFilterSpec;
    Vim25Api1__ObjectSpec objectSpec;
    license_map features;

    licPropertySpec.type = L"LicenseManager";
    licPropertySpec.all = &xsd_false;
    licPropertySpec.pathSet.push_back(L"licenses");

    objectSpec.obj = service_content.licenseManager;
    objectSpec.skip = &xsd_false;

    propertyFilterSpec.propSet.push_back(&licPropertySpec);
    propertyFilterSpec.objectSet.push_back(&objectSpec);

    req._USCOREthis = service_content.propertyCollector;
    req.specSet.push_back(&propertyFilterSpec);

    if (_vim_binding.RetrieveProperties(&req, res) != SOAP_OK)
        get_native_error(lastErrorCode, lastErrorMessage);
    else
    {
        for (size_t iReturnval = 0; iReturnval < res.returnval.size(); ++iReturnval)
        {
            Vim25Api1__ObjectContent* objectContent = res.returnval.at(iReturnval);
            for (size_t iPropSet = 0; iPropSet < objectContent->propSet.size(); ++iPropSet)
            {
                Vim25Api1__DynamicProperty& dynamicProperty = *objectContent->propSet.at(iPropSet);

                if (dynamicProperty.name == L"licenses")
                {
                    Vim25Api1__ArrayOfLicenseManagerLicenseInfo *plicFeatures = ((Vim25Api1__ArrayOfLicenseManagerLicenseInfo *)dynamicProperty.val);
                   
                    foreach(Vim25Api1__LicenseManagerLicenseInfo * feature, plicFeatures->LicenseManagerLicenseInfo)
                    {
                        std::vector<std::wstring> props;

                        foreach(Vim25Api1__KeyAnyValue *prop, feature->properties)
                        {
                            if (prop->key == L"feature")
                            {
                                props.push_back(reinterpret_cast<Vim25Api1__KeyValue *>(prop->value)->value);
                            }
                        }
                        features[feature->name] = props;
                    }
                }
            }
        }
    }

    return features;
}

VOID  vmware_portal::_fetch_task_info(Vim25Api1__ServiceContent& service_content, Vim25Api1__ObjectContent* objectContent, vmware_vm_task_info& task_info)
{
    FUN_TRACE;
    for (size_t iPropSet = 0; iPropSet < objectContent->propSet.size(); ++iPropSet)
    {
        Vim25Api1__DynamicProperty& dynamicProperty = *objectContent->propSet.at(iPropSet);
        if (dynamicProperty.name == L"info.state")
        {
            switch (((Vim25Api1__TaskInfoState_ *)dynamicProperty.val)->__item)
            {
            case Vim25Api1__TaskInfoState__running:
                task_info.state_name = L"Running";
                task_info.state = TASK_STATE::STATE_RUNNING;
                break;

            case Vim25Api1__TaskInfoState__queued:
                task_info.state_name = L"Queued";
                task_info.state = TASK_STATE::STATE_QUEUED;
                break;

            case Vim25Api1__TaskInfoState__success:
                task_info.state_name = L"Success";
                task_info.state = TASK_STATE::STATE_SUCCESS;
                break;

            case Vim25Api1__TaskInfoState__error:
                task_info.state_name = L"Error";
                task_info.state = TASK_STATE::STATE_ERROR;
                break;
            }
        }
        else if (dynamicProperty.name == L"info.progress")
        {
            task_info.progress = ((xsd__int *)(dynamicProperty.val))->__item;
        }
        else if (dynamicProperty.name == L"info.cancelable")
        {
            task_info.cancelable = ((xsd__boolean *)(dynamicProperty.val))->__item;
        }
        else if (dynamicProperty.name == L"info.result")
        {
            if (task_info.name == L"CreateSnapshot_Task" || task_info.name == L"RegisterVM_Task"){
                Vim25Api1__ManagedObjectReference *pMOR = dynamic_cast<Vim25Api1__ManagedObjectReference *>(dynamicProperty.val);
                if (pMOR){
                    task_info.moref = pMOR->__item.c_str();
                    task_info.moref_type = pMOR->type->c_str();
                }
            }
        }
        else if (dynamicProperty.name == L"info.entity")
        {
            Vim25Api1__ManagedObjectReference *pMOR = dynamic_cast<Vim25Api1__ManagedObjectReference *>(dynamicProperty.val);
            if (pMOR && !_wcsicmp(pMOR->type->c_str(), L"VirtualMachine")){
                try{
                    get_virtual_machine_uuid_from_mor(service_content, *pMOR, task_info.vm_uuid);
                }
                catch (...){
                }
            }
        }
        else if (dynamicProperty.name == L"info.entityName")
        {
            xsd__string* name = reinterpret_cast<xsd__string*>(dynamicProperty.val);
            task_info.entity_name = name->__item.c_str();
        }
        else if (dynamicProperty.name == L"info.name")
        {
            xsd__string* name = reinterpret_cast<xsd__string*>(dynamicProperty.val);
            task_info.name = name->__item.c_str();
        }
        else if (dynamicProperty.name == L"info.cancelled")
        {
            task_info.cancelled = ((xsd__boolean *)(dynamicProperty.val))->__item;
        }
        else if (dynamicProperty.name == L"info.startTime")
        {
            xsd__dateTime* dateTime = reinterpret_cast<xsd__dateTime*>(dynamicProperty.val);
            task_info.start = boost::posix_time::from_time_t(dateTime->__item);
        }
        else if (dynamicProperty.name == L"info.completeTime")
        {
            xsd__dateTime* dateTime = reinterpret_cast<xsd__dateTime*>(dynamicProperty.val);
            task_info.complete = boost::posix_time::from_time_t(dateTime->__item);
        }
        else if (dynamicProperty.name == L"info.error")
        {
            Vim25Api1__LocalizedMethodFault* localized_method_fault = dynamic_cast<Vim25Api1__LocalizedMethodFault*>(dynamicProperty.val);
            if (localized_method_fault ){
                if (localized_method_fault->localizedMessage)
                    task_info.error = localized_method_fault->localizedMessage->c_str();
                else if ((task_info.name == L"CreateVM_USCORETask") && (SOAP_TYPE_Vim25Api1__DuplicateName == localized_method_fault->soap_type())){
                    task_info.error = L"Createvm_responsetask failed, a VM with the same name already exists.";
                }
            }    
        }
    }
}

vmware_virtual_machine::ptr vmware_portal::create_virtual_machine(const vmware_virtual_machine_config_spec &config_spec){
    FUN_TRACE;
    return create_virtual_machine_internal(*(Vim25Api1__ServiceContent*)*_connect.get(), config_spec);
}

vmware_virtual_machine::ptr vmware_portal::modify_virtual_machine(const std::wstring &machine_key, const vmware_virtual_machine_config_spec &config_spec){
    FUN_TRACE;
    return modify_virtual_machine_internal(*(Vim25Api1__ServiceContent*)*_connect.get(), machine_key, config_spec);
}

vmware_virtual_machine::ptr vmware_portal::clone_virtual_machine(const std::wstring &machine_key, const vmware_clone_virtual_machine_config_spec &clone_spec){
    FUN_TRACE;
    operation_progress slot;
    return clone_virtual_machine_internal(*(Vim25Api1__ServiceContent*)*_connect.get(), machine_key, clone_spec, slot);
}

vmware_virtual_machine::ptr vmware_portal::clone_virtual_machine(const std::wstring &machine_key, const vmware_clone_virtual_machine_config_spec &clone_spec, operation_progress::slot_type slot){
    FUN_TRACE;
    return clone_virtual_machine_internal(*(Vim25Api1__ServiceContent*)*_connect.get(), machine_key, clone_spec, slot);
}

bool vmware_portal::delete_virtual_machine(const std::wstring &machine_key, const std::wstring &host_key){
    FUN_TRACE;
    return delete_virtual_machine_internal(*(Vim25Api1__ServiceContent*)*_connect.get(), machine_key, host_key);
}

bool vmware_portal::power_on_virtual_machine(const std::wstring &machine_key, const std::wstring &host_key){
    FUN_TRACE;
    return power_on_virtual_machine_internal(*(Vim25Api1__ServiceContent*)*_connect.get(), machine_key, host_key);
}

bool vmware_portal::power_off_virtual_machine(const std::wstring &machine_key, const std::wstring &host_key){
    FUN_TRACE;
    return power_off_virtual_machine_internal(*(Vim25Api1__ServiceContent*)*_connect.get(), machine_key, host_key);
}

bool vmware_portal::unregister_virtual_machine(const std::wstring &machine_key, const std::wstring &host_key){
    FUN_TRACE;
    return unregister_virtual_machine_internal(*(Vim25Api1__ServiceContent*)*_connect.get(), machine_key, host_key);
}

vmware_virtual_machine::ptr vmware_portal::add_existing_virtual_machine(const vmware_add_existing_virtual_machine_spec &spec){
    FUN_TRACE;
    return add_existing_virtual_machine_internal(*(Vim25Api1__ServiceContent*)*_connect.get(), spec);
}

vmware_virtual_machine::ptr vmware_portal::create_virtual_machine_internal(Vim25Api1__ServiceContent& service_content, const vmware_virtual_machine_config_spec &config_spec){
    FUN_TRACE;
    bool result = false;
    vmware_virtual_machine::ptr vm;
    if (!(config_spec.name.empty() || config_spec.config_path.empty() || config_spec.host_key.empty())){
        float                               version = 0;
        hv_connection_type                  type = HV_CONNECTION_TYPE_UNKNOWN;
        version = get_connection_info(service_content, type);
        Vim25Api1__CreateVMRequestType            createvm_request;
        _Vim25Api1__CreateVM_USCORETaskResponse   createvm_responsetask;
        Vim25Api1__ManagedObjectReference*        pHostMOR = NULL;
        Vim25Api1__ManagedObjectReference*        pClusterOrComputeResourceMOR = NULL;
        Vim25Api1__ManagedObjectReference*        pResourcePoolMOR = NULL;
        Vim25Api1__ManagedObjectReference*        pDataCenterMOR = NULL;
        Vim25Api1__ManagedObjectReference*        pVmFolderMOR = NULL;
        Vim25Api1__VirtualMachineConfigSpec       virtualmachine_configspec;
        Vim25Api1__VirtualMachineFileInfo         virtualmachine_fileinfo;
        std::wstring                              datastore_volume_name;
        int                                       nReturn = SOAP_OK;
        Vim25Api1__OptionValue					  exConfigFirmware;
        if (version < 5.0 && hv_vm_firmware::HV_VM_FIRMWARE_EFI == config_spec.firmware){
            result = false;
            LOG(LOG_LEVEL_ERROR, L"The ESX version doesnot support UEFI boot.");
            return vm;
        }
        if (NULL == (pHostMOR = get_host_mor(service_content, config_spec.host_key))){
            LOG(LOG_LEVEL_ERROR, L"Cannnot get host mor.");
        }
        else{
            string_array type;
            type.push_back(L"ComputeResource");
            type.push_back(L"ClusterComputeResource");
            pClusterOrComputeResourceMOR = get_ancestor_mor(service_content, pHostMOR, type);
            pResourcePoolMOR = get_resource_pool_mor(service_content, *pClusterOrComputeResourceMOR, config_spec.resource_pool_path);
            if (!pResourcePoolMOR){
                if (!config_spec.resource_pool_path.empty()){
                    LOG(LOG_LEVEL_WARNING, L"Cannot use resource pool (%s), using default resource pool instead.", config_spec.resource_pool_path.c_str());
                }
                pResourcePoolMOR = (Vim25Api1__ManagedObjectReference*)get_property_from_mor(service_content, *pClusterOrComputeResourceMOR, L"resourcePool");
            }
            pDataCenterMOR = get_ancestor_mor(service_content, pClusterOrComputeResourceMOR, L"Datacenter");
            pVmFolderMOR = get_vm_folder_mor(service_content, *pDataCenterMOR, config_spec.folder_path, true);
            if (!pVmFolderMOR){
                if (!config_spec.folder_path.empty()){
                    LOG(LOG_LEVEL_WARNING, L"Cannot use folder (%s), using default folder instead.", config_spec.folder_path.c_str());
                }
                pVmFolderMOR = (Vim25Api1__ManagedObjectReference*)get_property_from_mor(service_content, *pDataCenterMOR, L"vmFolder");
            }
            datastore_volume_name = boost::str(boost::wformat(L"[%s]") % config_spec.config_path);
            virtualmachine_fileinfo.vmPathName = &datastore_volume_name;
            bool enable_cbt = true;
            std::wstring name(config_spec.name);
            std::wstring annotation(config_spec.annotation);
            std::wstring uuid(config_spec.uuid);
            std::wstring guestId(config_spec.guest_id);
            virtualmachine_configspec.files = &virtualmachine_fileinfo;
            virtualmachine_configspec.name = &name;
            virtualmachine_configspec.changeTrackingEnabled = &enable_cbt;
            if (!annotation.empty())
                virtualmachine_configspec.annotation = &annotation;
            int number_of_cpus = config_spec.number_of_cpu;
            if (number_of_cpus <= 0)                           //Default to 1 CPU
                number_of_cpus = 1;
            virtualmachine_configspec.numCPUs = &number_of_cpus;
            int64_t memoryMB = config_spec.memory_mb;
            if (memoryMB <= 0)                       //Default to 512 MB
                memoryMB = 64;
            if (uuid.empty())
                uuid = macho::guid_::create();
            virtualmachine_configspec.uuid = &uuid;
            virtualmachine_configspec.memoryMB = &memoryMB;
            if (version >= 5.0){
                exConfigFirmware.key = L"firmware";
                xsd__string* pValue = soap_new_xsd__string(&_vim_binding, -1);
                exConfigFirmware.value = pValue;
                if (hv_vm_firmware::HV_VM_FIRMWARE_EFI == config_spec.firmware && std::wstring::npos != guestId.find(L"64Guest"))
                    pValue->__item = L"efi";
                else
                    pValue->__item = L"bios";
                virtualmachine_configspec.extraConfig.push_back(&exConfigFirmware);
            }
            if (!guestId.empty()){
                virtualmachine_configspec.guestId = &guestId;
            }
            virtualmachine_configspec.deviceChange.push_back(create_floppy_spec(service_content));
            if (config_spec.has_cdrom)
                virtualmachine_configspec.deviceChange.push_back(create_cd_rom_spec(service_content, pHostMOR));
            if (config_spec.has_serial_port)
                virtualmachine_configspec.deviceChange.push_back(create_serial_port_spec(service_content));
            int i = 0;
            foreach(vmware_virtual_network_adapter::ptr n, config_spec.nics){
                n ->key = - (int)(DEV_KEY::NIC + i);
                i++;
                virtualmachine_configspec.deviceChange.push_back(create_nic_spec(service_content, pHostMOR, DEV_CONFIGSPEC_OP::OP_ADD, n));
            }

            Vim25Api1__CreateVMRequestType    createvm_request;
            _Vim25Api1__CreateVM_USCORETaskResponse   createvm_responsetask;
            createvm_request._USCOREthis = pVmFolderMOR;
            createvm_request.config = &virtualmachine_configspec;
            createvm_request.pool = pResourcePoolMOR;
            createvm_request.host = pHostMOR;

            {
                journal_transaction trans(this);
                nReturn = _vim_binding.CreateVM_USCORETask(&createvm_request, createvm_responsetask);
            }
            if (SOAP_OK != nReturn){
                int lastErrorCode;
                std::wstring lastErrorMessage;
                get_native_error(lastErrorCode, lastErrorMessage);
                LOG(LOG_LEVEL_ERROR, L"CreateVM_USCORETask error: %s (%d)", lastErrorMessage.c_str(), nReturn);
                result = false;
            }
            else{
                result = true;
                vmware_vm_task_info::ptr task_info(new vmware_vm_task_info());
                task_info->name = L"CreateVM_USCORETask";
                std::wstring task_display_name = L"Create virtual machine " + *virtualmachine_configspec.name;
                try{
                    if ((nReturn = wait_task_completion(service_content, *createvm_responsetask.returnval, *task_info, task_display_name)) != TASK_STATE::STATE_SUCCESS && nReturn != TASK_STATE::STATE_QUEUED){
                        result = false;
                    }
                }
                catch (boost::exception &e){
                    result = false;
                }
                if (result){
                    //find the guid of this VM
                    LPVOID pProperty = NULL;
                    pProperty = (Vim25Api1__ArrayOfManagedObjectReference *)get_property_from_mor(service_content, *pVmFolderMOR, L"childEntity");
                    if (pProperty){
                        for (size_t i = 0; i < ((Vim25Api1__ArrayOfManagedObjectReference *)pProperty)->ManagedObjectReference.size(); i++){
                            Vim25Api1__ManagedObjectReference *pVmMOR = ((Vim25Api1__ArrayOfManagedObjectReference *)pProperty)->ManagedObjectReference[i];                           
                            std::wstring wszVmName = L"", wszUUID = L"";
                            if (pVmMOR && *pVmMOR->type == L"VirtualMachine"){
                                LPVOID pPropertyName = NULL;
                                pPropertyName = get_property_from_mor(service_content, *pVmMOR, L"name"); 
                                if (pPropertyName)
                                    wszVmName = ((xsd__string *)pPropertyName)->__item.c_str();
                                pPropertyName = get_property_from_mor(service_content, *pVmMOR, L"config.uuid");
                                if (pPropertyName)
                                    wszUUID = ((xsd__string *)pPropertyName)->__item.c_str();
                                if (!uuid.empty()){
                                    if (0 == _wcsicmp(wszUUID.c_str(), uuid.c_str())){
                                        vm = add_virtual_disks_vmdk(service_content, *pVmMOR, config_spec.disks);
                                        break;
                                    }
                                }
                                else if (0 == wcscmp(wszVmName.c_str(), config_spec.name.c_str())){
                                    vm = add_virtual_disks_vmdk(service_content, *pVmMOR, config_spec.disks);
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return vm;
}

vmware_virtual_machine::ptr vmware_portal::modify_virtual_machine_internal(Vim25Api1__ServiceContent& service_content, const std::wstring &machine_key, const vmware_virtual_machine_config_spec &config_spec){
    FUN_TRACE;
    Vim25Api1__VirtualMachineConfigSpec spec;
    vmware_virtual_machine::ptr              vm;
    Vim25Api1__ManagedObjectReference        vm_mor;
    key_name::map                            vm_more_key_name_map;
    float                                    version = 0;
    hv_connection_type                       type = HV_CONNECTION_TYPE_UNKNOWN;
    version = get_connection_info(service_content, type);
    vm_mor.__item = L"";
    get_vm_mor_key_name_map(service_content, vm_more_key_name_map);
    get_vm_mor_from_key_name_map(machine_key, vm_more_key_name_map, vm_mor);

    vmware_virtual_machine::ptr _vm = get_virtual_machine_from_mor(*(Vim25Api1__ServiceContent*)*_connect.get(), vm_mor);
    if (_vm && !vm_mor.__item.empty()){
        Vim25Api1__ManagedObjectReference* host_mor = get_host_mor(service_content, _vm->host_key);
        std::wstring name(config_spec.name);
        std::wstring annotation(config_spec.annotation);
        std::wstring uuid(config_spec.uuid);
        Vim25Api1__OptionValue exConfigFirmware;
        int number_of_cpus = config_spec.number_of_cpu;
        int64_t memory_mb = config_spec.memory_mb;
        std::wstring guestId(config_spec.guest_id);
        Vim25Api1__VirtualMachineConfigSpec spec;
        if (_vm->name != name)
            spec.name = &name;
        if (0 != _wcsicmp(_vm->uuid.c_str(),uuid.c_str()))
            spec.uuid = &uuid;
        if (_vm->annotation != annotation)
            spec.annotation = &annotation;
        if (number_of_cpus > 0 && _vm->number_of_cpu != number_of_cpus)
            spec.numCPUs = &number_of_cpus;
        if (memory_mb > 0 && _vm->memory_mb != memory_mb)
            spec.memoryMB = &memory_mb;
        if (!guestId.empty() && _vm->guest_id != guestId)
            spec.guestId = &guestId;
        else if (guestId.empty())
            guestId = _vm->guest_id;
        
        if (version >= 5.0){
            exConfigFirmware.key = L"firmware";
            xsd__string* pValue = soap_new_xsd__string(&_vim_binding, -1);
            exConfigFirmware.value = pValue;
            if (hv_vm_firmware::HV_VM_FIRMWARE_EFI == config_spec.firmware && std::wstring::npos != guestId.find(L"64Guest"))
                pValue->__item = L"efi";
            else
                pValue->__item = L"bios";
            spec.extraConfig.push_back(&exConfigFirmware);
        }

        if (!_vm->has_cdrom && config_spec.has_cdrom)
            spec.deviceChange.push_back(create_cd_rom_spec(service_content, host_mor));

        if (!_vm->has_serial_port && config_spec.has_serial_port)
            spec.deviceChange.push_back(create_serial_port_spec(service_content));

        vmware_virtual_scsi_controller::vtr scsi_controllers;
        foreach(vmware_disk_info::ptr d, config_spec.disks){
            bool found = false;
            if (d->controller  && - 1 != d->controller->bus_number){
                foreach(vmware_virtual_scsi_controller::ptr &s, scsi_controllers){

                    if (d->controller->bus_number == s->bus_number){
                        found = true;
                        break;
                    }
                }
                if (!found)
                    scsi_controllers.push_back(d->controller);
            }
        }
        foreach(vmware_virtual_scsi_controller::ptr &s, _vm->scsi_controllers){
            foreach(vmware_virtual_scsi_controller::ptr &_s, scsi_controllers){
                if (_s->bus_number == s->bus_number && _s->type != s->type){
                    spec.deviceChange.push_back(create_scsi_ctrl_spec(service_content, *_s.get(), DEV_CONFIGSPEC_OP::OP_EDIT));
                }
            }
        }
        foreach(vmware_disk_info::ptr &d, _vm->disks){
            bool found = false;
            foreach(const vmware_disk_info::ptr &_d, config_spec.disks){
                if (_d->controller && d->key == _d->key){
                    if ((_d->name != d->name) || (_d->size > d->size) || (_d->unit_number != d->unit_number) ){
                        if (d->controller->slots[_d->unit_number]){
                            _d->unit_number = d->unit_number;
                        }
                        else{
                            d->controller->slots[d->unit_number] = false;
                            d->controller->slots[_d->unit_number] = true;
                        }
                        spec.deviceChange.push_back(create_virtual_disk_spec(service_content, _d, DEV_CONFIGSPEC_OP::OP_EDIT, DEV_CONFIGSPEC_FILEOP::FILEOP_UNSPECIFIED));
                    }
                    found = true;
                    break;
                }
            }

            if (!found && d->controller){
                vmware_disk_info::ptr _d(new vmware_disk_info(*d));
                d->controller->slots[d->unit_number] = false;
                spec.deviceChange.push_back(create_virtual_disk_spec(service_content, _d, DEV_CONFIGSPEC_OP::OP_REMOVE, DEV_CONFIGSPEC_FILEOP::FILEOP_DESTROY));
            }
        }
        vmware_disk_info::vtr new_disks;
        foreach(const vmware_disk_info::ptr &_d, config_spec.disks){
            bool found = false;
            foreach(vmware_disk_info::map::value_type &d, _vm->disks_map){
                if (d.second->key == _d->key){
                    found = true;
                    break;
                }
            }
            if (!found){
                new_disks.push_back(_d);
            }
        }
        if (!new_disks.empty()){
            std::wstring path = boost::filesystem::path(_vm->config_path_file).parent_path().wstring();
            foreach(vmware_disk_info::ptr d, new_disks){
                if (0 != d->name.find(L"[")){
                    d->name = boost::str(boost::wformat(L"%s/%s") % path % d->name);
                }
            }
            std::vector<Vim25Api1__VirtualDeviceConfigSpec*> controller_and_disks = create_vmdks_spec(service_content, _vm, new_disks);
            spec.deviceChange.insert(spec.deviceChange.end(), controller_and_disks.begin(), controller_and_disks.end());
        }
        int available_key = DEV_KEY::NIC;
        foreach(vmware_virtual_network_adapter::ptr &o, _vm->network_adapters){
            if (o->key >= available_key)
                available_key = o->key + 1;
            bool found = false;
            foreach(const vmware_virtual_network_adapter::ptr &n, config_spec.nics){
                if (o->key == n->key){
                    if ((o->address_type != n->address_type) || 
                        o->mac_address != n->mac_address || 
                        o->is_allow_guest_control != n->is_allow_guest_control || 
                        o->is_connected != n->is_connected ||
                        o->is_start_connected != n->is_start_connected ||
                        o->network != n->network ||
                        o->type != n->type ||
                        o->name != n->name || 
                        o->port_group != n->port_group){
                        spec.deviceChange.push_back(create_nic_spec(service_content, host_mor, DEV_CONFIGSPEC_OP::OP_EDIT, n));
                    }
                    found = true;
                    break;
                }
            }
            if (!found){
                spec.deviceChange.push_back(create_nic_spec(service_content, host_mor, DEV_CONFIGSPEC_OP::OP_REMOVE, o));
            }
        }
        foreach(const vmware_virtual_network_adapter::ptr &n, config_spec.nics){
            bool found = false;
            foreach(vmware_virtual_network_adapter::ptr &o, _vm->network_adapters){
                if (o->key == n->key){
                    found = true;
                    break;
                }
            }
            if (!found){
                n->key = -(int)(available_key++);
                spec.deviceChange.push_back(create_nic_spec(service_content, host_mor, DEV_CONFIGSPEC_OP::OP_ADD, n));
            }
        }
        if (reconfig_virtual_machine(service_content, vm_mor, spec)){
            vm = get_virtual_machine_from_mor(service_content, vm_mor);
        }
    }
    return vm;
}

bool                         vmware_portal::mount_vm_tools_internal(Vim25Api1__ServiceContent& service_content, const std::wstring &machine_key){
    FUN_TRACE;
    Vim25Api1__VirtualMachineConfigSpec      spec;
    vmware_virtual_machine::ptr              vm;
    Vim25Api1__ManagedObjectReference        vm_mor;
    key_name::map                            vm_more_key_name_map;
    float                                    version = 0;
    hv_connection_type                       type = HV_CONNECTION_TYPE_UNKNOWN;
    version = get_connection_info(service_content, type);
    vm_mor.__item = L"";
    get_vm_mor_key_name_map(service_content, vm_more_key_name_map);
    get_vm_mor_from_key_name_map(machine_key, vm_more_key_name_map, vm_mor);
    vmware_virtual_machine::ptr _vm = get_virtual_machine_from_mor(*(Vim25Api1__ServiceContent*)*_connect.get(), vm_mor);
    if (_vm && !vm_mor.__item.empty() && _vm->has_cdrom){
        Vim25Api1__VirtualDeviceBackingInfo* backing;
        Vim25Api1__VirtualDeviceConnectInfo* connectInfo = soap_new_Vim25Api1__VirtualDeviceConnectInfo(&_vim_binding, -1);
        connectInfo->startConnected = true;
        Vim25Api1__VirtualCdromIsoBackingInfo* isoBacking = soap_new_Vim25Api1__VirtualCdromIsoBackingInfo(&_vim_binding, -1);
        if (std::string::npos != _vm->guest_id.find(L"win")){
            if (std::string::npos != _vm->guest_id.find(L"winNet") ||
                std::string::npos != _vm->guest_id.find(L"winXP") ||
                std::string::npos != _vm->guest_id.find(L"win2000")){
                isoBacking->fileName = L"[]/vmimages/tools-isoimages/winPreVista.iso";
            }
            else{
                isoBacking->fileName = L"[]/vmimages/tools-isoimages/windows.iso";
            }
        }
        else{
            isoBacking->fileName = L"[]/vmimages/tools-isoimages/linux.iso";
        }	
        backing = isoBacking;
        xsd__int* controllerKey = soap_new_xsd__int(&_vim_binding, -1);
        controllerKey->__item = _vm->cdrom->controller_key;
        Vim25Api1__VirtualCdrom* cdRom = soap_new_Vim25Api1__VirtualCdrom(&_vim_binding, -1);
        cdRom->key = _vm->cdrom->key;
        cdRom->controllerKey = &controllerKey->__item;
        cdRom->backing = backing;
        cdRom->connectable = connectInfo;
        Vim25Api1__VirtualDeviceConfigSpecOperation_* operation = soap_new_Vim25Api1__VirtualDeviceConfigSpecOperation_(&_vim_binding, -1);
        operation->__item = Vim25Api1__VirtualDeviceConfigSpecOperation__edit;
        Vim25Api1__VirtualDeviceConfigSpec* pCdRomConfigSpec = soap_new_Vim25Api1__VirtualDeviceConfigSpec(&_vim_binding, -1);
        pCdRomConfigSpec->operation = &operation->__item;
        pCdRomConfigSpec->device = cdRom;
        spec.deviceChange.push_back(pCdRomConfigSpec);

        return reconfig_virtual_machine(service_content, vm_mor, spec);
    }
    return false;
}

vmware_virtual_machine::ptr vmware_portal::add_virtual_disks_vmdk(Vim25Api1__ServiceContent& service_content, Vim25Api1__ManagedObjectReference& vm_mor, vmware_disk_info::vtr disks){
    FUN_TRACE;
    vmware_virtual_machine::ptr vm = get_virtual_machine_from_mor(service_content, vm_mor);
    if (vm){
        std::wstring path = boost::filesystem::path(vm->config_path_file).parent_path().wstring();
        foreach(vmware_disk_info::ptr d, disks){
            if (0 != d->name.find(L"[")){
                d->name = boost::str(boost::wformat(L"%s/%s") % path % d->name);
            }
        }
        Vim25Api1__VirtualMachineConfigSpec spec;
        std::vector<Vim25Api1__VirtualDeviceConfigSpec*> controller_and_disks = create_vmdks_spec(service_content, vm, disks);
        spec.deviceChange.insert(spec.deviceChange.end(), controller_and_disks.begin(), controller_and_disks.end());
        if (reconfig_virtual_machine(service_content, vm_mor, spec)){
            vm = get_virtual_machine_from_mor(service_content, vm_mor);
        }
        else{
            if (delete_virtual_machine_internal(service_content, vm_mor))
                vm = NULL;
            else{
                LOG(LOG_LEVEL_ERROR, L"Can't delete unexpected virtual machine.");
            }
        }
    }
    return vm;
}

vmware_virtual_machine::ptr vmware_portal::clone_virtual_machine_internal(Vim25Api1__ServiceContent& service_content, const std::wstring &machine_key, const vmware_clone_virtual_machine_config_spec &clone_spec, operation_progress::slot_type slot){
    FUN_TRACE;
    bool result = false;
    int nReturn = SOAP_OK;
    vmware_virtual_machine::ptr              vm;
    Vim25Api1__ManagedObjectReference        vm_mor;
    Vim25Api1__CloneVMRequestType            cloneVMRequest;
    _Vim25Api1__CloneVM_USCORETaskResponse   cloneVMResponseTask;
    Vim25Api1__VirtualMachineCloneSpec       cloneVMSpec;
    Vim25Api1__VirtualMachineRelocateSpec    relocateSpec;
    Vim25Api1__VirtualMachineConfigSpec      vmSpec;
    hv_connection_type type;
    key_name::map                           vm_more_key_name_map;
    vm_mor.__item = L"";
    get_connection_info(service_content, type);
    if (hv_connection_type::HV_CONNECTION_TYPE_HOST == type){
        LOG(LOG_LEVEL_WARNING, L"The operation is not supported.");
        return vm;
    }
    get_vm_mor_key_name_map(service_content, vm_more_key_name_map);
    get_vm_mor_from_key_name_map(machine_key, vm_more_key_name_map, vm_mor);
    Vim25Api1__ManagedObjectReference snapshot_mor;
    Vim25Api1__ManagedObjectReference folder_mor;
    vmware_virtual_machine::ptr _vm = get_virtual_machine_from_mor(*(Vim25Api1__ServiceContent*)*_connect.get(), vm_mor);
    if (_vm && !vm_mor.__item.empty() && !clone_spec.snapshot_mor_item.empty()){
        snapshot_mor.__item = clone_spec.snapshot_mor_item;
        snapshot_mor.type = soap_new_std__wstring(&_vim_binding, -1);
        snapshot_mor.type->assign(L"VirtualMachineSnapshot");
        folder_mor.__item = _vm->parent_mor_item;
        folder_mor.type = soap_new_std__wstring(&_vim_binding, -1);
        folder_mor.type->assign(L"Folder");
        std::wstring uuid = clone_spec.uuid; 
        if (uuid.empty())
            uuid = macho::guid_::create();
        vmSpec.uuid = &uuid;
        cloneVMSpec.config = &vmSpec;
        cloneVMSpec.snapshot = &snapshot_mor;
        cloneVMSpec.powerOn = false;
        cloneVMSpec.template_ = false;
        cloneVMSpec.location = &relocateSpec;
        if (clone_spec.datastore.empty()){
            result = true;
        }
        else{
            key_name::map mapDatastoreMorKeyName;
            get_datastore_mor_key_name_map(service_content, mapDatastoreMorKeyName);
            foreach(key_name::map::value_type &d, mapDatastoreMorKeyName){
                if (0 == wcsicmp(d.second.name.c_str(), clone_spec.datastore.c_str())){
                    cloneVMSpec.location->datastore = soap_new_Vim25Api1__ManagedObjectReference(&_vim_binding, -1);
                    cloneVMSpec.location->datastore->__item.assign(d.first);
                    cloneVMSpec.location->datastore->type = soap_new_std__wstring(&_vim_binding, -1);
                    cloneVMSpec.location->datastore->type->assign(L"Datastore");
                    result = true;
                    break;
                }
            }
            if (!result){
                LOG(LOG_LEVEL_ERROR, L"Can't find the datastore : %s", clone_spec.datastore.c_str());
            }
        }

        if (result){
            result = false;
            cloneVMSpec.location->diskMoveType = soap_new_std__wstring(&_vim_binding, -1);
            switch (clone_spec.disk_move_type){
                case vmware_clone_virtual_machine_config_spec::createNewChildDiskBacking :
                    *cloneVMSpec.location->diskMoveType = L"createNewChildDiskBacking";
                    break;
                case vmware_clone_virtual_machine_config_spec::moveAllDiskBackingsAndAllowSharing:
                    *cloneVMSpec.location->diskMoveType = L"moveAllDiskBackingsAndAllowSharing";
                    break;
                case vmware_clone_virtual_machine_config_spec::moveAllDiskBackingsAndConsolidate:
                    *cloneVMSpec.location->diskMoveType = L"moveAllDiskBackingsAndConsolidate";
                    break;
                case vmware_clone_virtual_machine_config_spec::moveAllDiskBackingsAndDisallowSharing:
                    *cloneVMSpec.location->diskMoveType = L"moveAllDiskBackingsAndDisallowSharing";
                    break;
                case vmware_clone_virtual_machine_config_spec::moveChildMostDiskBacking:
                    *cloneVMSpec.location->diskMoveType = L"moveChildMostDiskBacking";
                    break;
            };

            cloneVMSpec.location->transform = soap_new_Vim25Api1__VirtualMachineRelocateTransformation(&_vim_binding, -1);		
            switch (clone_spec.transformation_type){
            case vmware_clone_virtual_machine_config_spec::Transformation__flat:
                *cloneVMSpec.location->transform = Vim25Api1__VirtualMachineRelocateTransformation__flat;
                break;
            case vmware_clone_virtual_machine_config_spec::Transformation__sparse:
                *cloneVMSpec.location->transform = Vim25Api1__VirtualMachineRelocateTransformation__sparse;
                break;
            }

            if (!clone_spec.folder_path.empty()){
                Vim25Api1__ManagedObjectReference*        pHostMOR = NULL;
                Vim25Api1__ManagedObjectReference*        pClusterOrComputeResourceMOR = NULL;
                Vim25Api1__ManagedObjectReference*        pDataCenterMOR = NULL;
                Vim25Api1__ManagedObjectReference*        pVmFolderMOR = NULL;
                if (pHostMOR = get_host_mor(service_content, _vm->host_key)){
                    string_array type;
                    type.push_back(L"ComputeResource");
                    type.push_back(L"ClusterComputeResource");
                    pClusterOrComputeResourceMOR = get_ancestor_mor(service_content, pHostMOR, type);
                    pDataCenterMOR = get_ancestor_mor(service_content, pClusterOrComputeResourceMOR, L"Datacenter");
                    pVmFolderMOR = get_vm_folder_mor(service_content, *pDataCenterMOR, clone_spec.folder_path, true);
                    if (!pVmFolderMOR){
                        if (!clone_spec.folder_path.empty()){
                            LOG(LOG_LEVEL_WARNING, L"Cannot use folder (%s), using default folder instead.", clone_spec.folder_path.c_str());
                        }
                        pVmFolderMOR = (Vim25Api1__ManagedObjectReference*)get_property_from_mor(service_content, *pDataCenterMOR, L"vmFolder");
                    }
                    cloneVMRequest.folder = pVmFolderMOR;
                }
            }
            
            if (NULL == cloneVMRequest.folder){
                cloneVMRequest.folder = &folder_mor;
            }

            cloneVMRequest.name = clone_spec.vm_name;
            cloneVMRequest.spec = &cloneVMSpec;
            cloneVMRequest._USCOREthis = &vm_mor;
            {
                journal_transaction trans(this);
                nReturn = _vim_binding.CloneVM_USCORETask(&cloneVMRequest, cloneVMResponseTask);
            }
            if (SOAP_OK != nReturn)
            {
                int lastErrorCode;
                std::wstring lastErrorMessage;
                get_native_error(lastErrorCode, lastErrorMessage);
                LOG(LOG_LEVEL_ERROR, L"Destroy_USCORETask error: %s (%d)", lastErrorMessage.c_str(), nReturn);
                result = false;
            }
            else
            {
                result = true;
                vmware_vm_task_info::ptr task_info(new vmware_vm_task_info());
                task_info->name = L"CloneVM_USCORETask";
                std::wstring task_display_name = L"Clone virtual machine " + machine_key;
                try
                {
                    if ((nReturn = wait_task_completion_callback(service_content, *cloneVMResponseTask.returnval, *task_info, task_display_name, slot)) != TASK_STATE::STATE_SUCCESS && nReturn != TASK_STATE::STATE_QUEUED)
                        result = false;
                }
                catch (boost::exception &e)
                {
                    result = false;
                }
                if (result){
                    //find the guid of this VM
                    LPVOID pProperty = NULL;
                    pProperty = (Vim25Api1__ArrayOfManagedObjectReference *)get_property_from_mor(service_content, *cloneVMRequest.folder, L"childEntity");
                    if (pProperty){
                        for (size_t i = 0; i < ((Vim25Api1__ArrayOfManagedObjectReference *)pProperty)->ManagedObjectReference.size(); i++){
                            Vim25Api1__ManagedObjectReference *pVmMOR = ((Vim25Api1__ArrayOfManagedObjectReference *)pProperty)->ManagedObjectReference[i];     
                            std::wstring wszUUID = L"";
                            if (pVmMOR && *pVmMOR->type == L"VirtualMachine"){
                                LPVOID pPropertyName = NULL;
                                pPropertyName = get_property_from_mor(service_content, *pVmMOR, L"config.uuid");
                                if (pPropertyName)
                                    wszUUID = ((xsd__string *)pPropertyName)->__item.c_str();
                                if (0 == _wcsicmp(wszUUID.c_str(), uuid.c_str())){
                                    vm = get_virtual_machine_from_mor(service_content, *pVmMOR);
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    else
    {
        result = false;
    }

    return vm;
}

bool                        vmware_portal::delete_virtual_machine_internal(Vim25Api1__ServiceContent& service_content, Vim25Api1__ManagedObjectReference& vm_mor){
    FUN_TRACE;
    bool result = true;
    int nReturn = SOAP_OK;
    Vim25Api1__DestroyRequestType            destroyRequest;
    _Vim25Api1__Destroy_USCORETaskResponse   desroyResponseTask;
    destroyRequest._USCOREthis = &vm_mor;
    {
        journal_transaction trans(this);
        nReturn = _vim_binding.Destroy_USCORETask(&destroyRequest, desroyResponseTask);
    }
    if (SOAP_OK != nReturn)
    {
        int lastErrorCode;
        std::wstring lastErrorMessage;
        get_native_error(lastErrorCode, lastErrorMessage);
        LOG(LOG_LEVEL_ERROR, L"Destroy_USCORETask error: %s (%d)", lastErrorMessage.c_str(), nReturn);
        result = false;
    }
    else
    {
        vmware_vm_task_info::ptr task_info(new vmware_vm_task_info());
        task_info->name = L"Destroy_USCORETask";
        std::wstring task_display_name = L"Delete virtual machine";
        try
        {
            if ((nReturn = wait_task_completion(service_content, *desroyResponseTask.returnval, *task_info, task_display_name)) != TASK_STATE::STATE_SUCCESS && nReturn != TASK_STATE::STATE_QUEUED)
                result = false;
        }
        catch (boost::exception &e)
        {
            result = false;
        }
    }
    return result;
}

bool                        vmware_portal::delete_virtual_machine_internal(Vim25Api1__ServiceContent& service_content, const std::wstring &machine_key, const std::wstring &host_key){
    FUN_TRACE;
    bool result = false;
    Vim25Api1__ManagedObjectReference vm_mor;
    key_name::map                           vm_more_key_name_map;
    vm_mor.__item = L"";
    get_vm_mor_key_name_map(service_content, vm_more_key_name_map);
    get_vm_mor_from_key_name_map(machine_key, vm_more_key_name_map, vm_mor);
    if (!vm_mor.__item.empty())
        result = delete_virtual_machine_internal(service_content, vm_mor);
    return result;
}

bool                        vmware_portal::power_on_virtual_machine_internal(Vim25Api1__ServiceContent& service_content, const std::wstring &machine_key, const std::wstring &host_key){
    FUN_TRACE;
    bool result = true;
    int nReturn = SOAP_OK;
    Vim25Api1__ManagedObjectReference vm_mor;
    Vim25Api1__PowerOnVMRequestType          poweron_request;
    _Vim25Api1__PowerOnVM_USCORETaskResponse poweron_responseTask;

    hv_connection_type type;
    key_name::map                           vm_more_key_name_map;

    vm_mor.__item = L"";
    get_connection_info(service_content, type);
    get_vm_mor_key_name_map(service_content, vm_more_key_name_map);

    get_vm_mor_from_key_name_map(machine_key, vm_more_key_name_map, vm_mor);

    if (!vm_mor.__item.empty())
    {
        vmware_virtual_machine::ptr _vm = get_virtual_machine_from_mor(service_content, vm_mor);
        if (_vm->power_state == hv_vm_power_state::HV_VMPOWER_ON){
            result = true;
            LOG(LOG_LEVEL_WARNING, L"The power state of virtual machine(%s) : power on.", _vm->name.c_str());
        }
        else{
            poweron_request._USCOREthis = &vm_mor;
            {
                journal_transaction trans(this);
                nReturn = _vim_binding.PowerOnVM_USCORETask(&poweron_request, poweron_responseTask);
            }
            if (SOAP_OK != nReturn)
            {
                int lastErrorCode;
                std::wstring lastErrorMessage;
                get_native_error(lastErrorCode, lastErrorMessage);
                LOG(LOG_LEVEL_ERROR, L"PowerOnVM_USCORETask error: %s (%d)", lastErrorMessage.c_str(), nReturn);
                result = false;
            }
            else
            {
                vmware_vm_task_info::ptr task_info(new vmware_vm_task_info());
                task_info->name = L"PowerOnVM_USCORETask";
                std::wstring task_display_name = L"Power On virtual machine " + machine_key;
                try
                {
                    if ((nReturn = wait_task_completion(service_content, *poweron_responseTask.returnval, *task_info, task_display_name)) != TASK_STATE::STATE_SUCCESS && nReturn != TASK_STATE::STATE_QUEUED)
                        result = false;
                }
                catch (boost::exception &e)
                {
                    result = false;
                }
            }
        }
    }
    else
    {
        result = false;
    }
    return result;
}

bool vmware_portal::power_off_virtual_machine_internal(Vim25Api1__ServiceContent& service_content, const std::wstring &machine_key, const std::wstring &host_key){
    FUN_TRACE;
    bool result = true;
    int nReturn = SOAP_OK;
    Vim25Api1__ManagedObjectReference vm_mor;
    Vim25Api1__PowerOffVMRequestType          poweroff_request;
    _Vim25Api1__PowerOffVM_USCORETaskResponse poweroff_responseTask;

    hv_connection_type type;
    key_name::map                           vm_more_key_name_map;

    vm_mor.__item = L"";
    get_connection_info(service_content, type);
    get_vm_mor_key_name_map(service_content, vm_more_key_name_map);

    get_vm_mor_from_key_name_map(machine_key, vm_more_key_name_map, vm_mor);

    if (!vm_mor.__item.empty())
    {
        vmware_virtual_machine::ptr _vm = get_virtual_machine_from_mor(service_content, vm_mor);
        if (_vm->power_state == hv_vm_power_state::HV_VMPOWER_OFF){
            result = true;
            LOG(LOG_LEVEL_WARNING, L"The power state of virtual machine(%s) : power off.", _vm->name.c_str());
        }
        else{
            poweroff_request._USCOREthis = &vm_mor;
            {
                journal_transaction trans(this);
                nReturn = _vim_binding.PowerOffVM_USCORETask(&poweroff_request, poweroff_responseTask);
            }
            if (SOAP_OK != nReturn)
            {
                int lastErrorCode;
                std::wstring lastErrorMessage;
                get_native_error(lastErrorCode, lastErrorMessage);
                LOG(LOG_LEVEL_ERROR, L"PowerOffVM_USCORETask error: %s (%d)", lastErrorMessage.c_str(), nReturn);
                result = false;
            }
            else
            {
                vmware_vm_task_info::ptr task_info(new vmware_vm_task_info());
                task_info->name = L"PowerOffVM_USCORETask";
                std::wstring task_display_name = L"Power Off virtual machine " + machine_key;
                try
                {
                    if ((nReturn = wait_task_completion(service_content, *poweroff_responseTask.returnval, *task_info, task_display_name)) != TASK_STATE::STATE_SUCCESS && nReturn != TASK_STATE::STATE_QUEUED)
                        result = false;
                }
                catch (boost::exception &e)
                {
                    result = false;
                }
            }
        }
    }
    else
    {
        result = false;
    }
    return result;
}

bool vmware_portal::unregister_virtual_machine_internal(Vim25Api1__ServiceContent& service_content, const std::wstring &machine_key, const std::wstring &host_key){
    FUN_TRACE;
    bool result = true;
    int nReturn = SOAP_OK;
    Vim25Api1__ManagedObjectReference     vm_mor;
    Vim25Api1__UnregisterVMRequestType    unregsiterRequest;
    _Vim25Api1__UnregisterVMResponse      unregsiterResponse;
    hv_connection_type type;
    key_name::map                         vm_more_key_name_map;

    vm_mor.__item = L"";
    get_connection_info(service_content, type);
    get_vm_mor_key_name_map(service_content, vm_more_key_name_map);
    get_vm_mor_from_key_name_map(machine_key, vm_more_key_name_map, vm_mor);

    if (!vm_mor.__item.empty()){
        unregsiterRequest._USCOREthis = &vm_mor;
        {
            journal_transaction trans(this);
            nReturn = _vim_binding.UnregisterVM(&unregsiterRequest, unregsiterResponse);
        }

        if (SOAP_OK != nReturn){
            int lastErrorCode;
            std::wstring lastErrorMessage;
            get_native_error(lastErrorCode, lastErrorMessage);
            LOG(LOG_LEVEL_ERROR, L"UnregisterVM error: %s (%d)", lastErrorMessage.c_str(), nReturn);
            result = false;
        }
    }
    return result;
}

vmware_virtual_machine::ptr vmware_portal::add_existing_virtual_machine_internal(Vim25Api1__ServiceContent& service_content, const vmware_add_existing_virtual_machine_spec &spec){
    FUN_TRACE;
    vmware_virtual_machine::ptr vm;
    bool result = true;
    int nReturn = SOAP_OK;
    Vim25Api1__ManagedObjectReference*        pHostMOR = NULL;
    Vim25Api1__ManagedObjectReference*        pClusterOrComputeResourceMOR = NULL;
    Vim25Api1__ManagedObjectReference*        pResourcePoolMOR = NULL;
    Vim25Api1__ManagedObjectReference*        pDataCenterMOR = NULL;
    Vim25Api1__ManagedObjectReference*        pVmFolderMOR = NULL;
    Vim25Api1__RegisterVMRequestType          register_vm_request;
    _Vim25Api1__RegisterVM_USCORETaskResponse register_vm_responseTask;
    hv_connection_type type;
    get_connection_info(service_content, type);

    if (NULL == (pHostMOR = get_host_mor(service_content, spec.host_key))){
        LOG(LOG_LEVEL_ERROR, L"Cannnot get host mor.");
    }
    else{
        string_array type;
        type.push_back(L"ComputeResource");
        type.push_back(L"ClusterComputeResource");
        pClusterOrComputeResourceMOR = get_ancestor_mor(service_content, pHostMOR, type);
        pResourcePoolMOR = get_resource_pool_mor(service_content, *pClusterOrComputeResourceMOR, spec.resource_pool_path);
        if (!pResourcePoolMOR){
            if (!spec.resource_pool_path.empty()){
                LOG(LOG_LEVEL_WARNING, L"Cannot use resource pool (%s), using default resource pool instead.", spec.resource_pool_path.c_str());
            }
            pResourcePoolMOR = (Vim25Api1__ManagedObjectReference*)get_property_from_mor(service_content, *pClusterOrComputeResourceMOR, L"resourcePool");
        }
        pDataCenterMOR = get_ancestor_mor(service_content, pClusterOrComputeResourceMOR, L"Datacenter");
        pVmFolderMOR = get_vm_folder_mor(service_content, *pDataCenterMOR, spec.folder_path, true);
        if (!pVmFolderMOR){
            if (!spec.folder_path.empty()){
                LOG(LOG_LEVEL_WARNING, L"Cannot use folder (%s), using default folder instead.", spec.folder_path.c_str());
            }
            pVmFolderMOR = (Vim25Api1__ManagedObjectReference*)get_property_from_mor(service_content, *pDataCenterMOR, L"vmFolder");
        }
        register_vm_request._USCOREthis = pVmFolderMOR;
        register_vm_request.path = spec.config_file_path;
        register_vm_request.asTemplate = false;
        register_vm_request.pool = pResourcePoolMOR;
        register_vm_request.host = pHostMOR;
        {
            journal_transaction trans(this);
            nReturn = _vim_binding.RegisterVM_USCORETask(&register_vm_request, register_vm_responseTask);
        }
        if (SOAP_OK != nReturn){
            int lastErrorCode;
            std::wstring lastErrorMessage;
            get_native_error(lastErrorCode, lastErrorMessage);
            LOG(LOG_LEVEL_ERROR, L"RegisterVM_USCORETask error: %s (%d)", lastErrorMessage.c_str(), nReturn);
            result = false;
        }
        else{
            result = true;
            vmware_vm_task_info::ptr task_info(new vmware_vm_task_info());
            task_info->name = L"RegisterVM_USCORETask";
            std::wstring task_display_name = L"Register virtual machine " + spec.config_file_path;
            try{
                if ((nReturn = wait_task_completion(service_content, *register_vm_responseTask.returnval, *task_info, task_display_name)) != TASK_STATE::STATE_SUCCESS && nReturn != TASK_STATE::STATE_QUEUED){
                    result = false;
                }
            }
            catch (boost::exception &e){
                result = false;
            }
            if (result){
                if (task_info->moref_type == L"VirtualMachine"){
                    Vim25Api1__ManagedObjectReference vm_mor;
                    vm_mor.type = &task_info->moref_type;
                    vm_mor.__item = task_info->moref;
                    if (spec.uuid.empty()){
                        vm = get_virtual_machine_from_mor(service_content, vm_mor);
                    }
                    else{
                        try{
                            macho::guid_ g(spec.uuid);
                            Vim25Api1__VirtualMachineConfigSpec _spec;
                            std::wstring uuid = spec.uuid;
                            _spec.uuid = &uuid;
                            reconfig_virtual_machine(service_content, vm_mor, _spec);
                            vm = get_virtual_machine_from_mor(service_content, vm_mor);
                        }
                        catch (...){
                            LOG(LOG_LEVEL_WARNING, L"Invalid UUID string");
                            vm = get_virtual_machine_from_mor(service_content, vm_mor);
                        }
                    }
                }
            }
        }
    }
    return vm;
}

Vim25Api1__ManagedObjectReference* vmware_portal::get_resource_pool_mor(Vim25Api1__ServiceContent& service_content, Vim25Api1__ManagedObjectReference& computeResource, const std::wstring& resource_pool_path){
    FUN_TRACE;
    string_array resource_pool_names = macho::stringutils::tokenize2(resource_pool_path, L"/");
    // at least default resouce pool
    if (resource_pool_names.empty() || _wcsicmp(L"Resources", resource_pool_names[0].c_str())){
        return NULL;
    }

    Vim25Api1__ManagedObjectReference* rp = reinterpret_cast<Vim25Api1__ManagedObjectReference*>(get_property_from_mor(service_content, computeResource, L"resourcePool"));
    if (!rp){
        return NULL;
    }

    for (size_t index = 1; index < resource_pool_names.size(); index++){
        const std::wstring& resource_pool_name = resource_pool_names[index];
        Vim25Api1__ArrayOfManagedObjectReference* children = reinterpret_cast<Vim25Api1__ArrayOfManagedObjectReference*>(get_property_from_mor(service_content, *rp, L"resourcePool"));
        rp = NULL;
        for (size_t children_index = 0; children_index < children->ManagedObjectReference.size(); ++children_index){
            Vim25Api1__ManagedObjectReference* child = children->ManagedObjectReference.at(children_index);
            if (!child || !child->type){
                continue;
            }
            xsd__string* name = reinterpret_cast<xsd__string*>(get_property_from_mor(service_content, *child, L"name"));
            if (name && 0 == _wcsicmp(resource_pool_name.c_str(), name->__item.c_str())){
                rp = child;
                break;
            }
        }
        if (!rp){
            return NULL;
        }
    }
    return rp;
}

Vim25Api1__ManagedObjectReference* vmware_portal::get_vm_folder_mor(Vim25Api1__ServiceContent& service_content, Vim25Api1__ManagedObjectReference& datacenter, const std::wstring& folder_path, bool can_be_create){
    
    FUN_TRACE;
    string_array folder_names = macho::stringutils::tokenize2(folder_path, L"/", 0, false);
    // at least default folder
    if (folder_names.empty() || (folder_names.size() == 1 && _wcsicmp(L"vm", folder_names[0].c_str()))){
        return NULL;
    }

    Vim25Api1__ManagedObjectReference* folder =
        reinterpret_cast<Vim25Api1__ManagedObjectReference*>(get_property_from_mor(service_content, datacenter, L"vmFolder"));
    if (!folder){
        return NULL;
    }

    for (size_t iFolderName = 1; iFolderName < folder_names.size(); iFolderName++){

        const std::wstring& folderName = folder_names[iFolderName];

        Vim25Api1__ArrayOfManagedObjectReference* children =
            reinterpret_cast<Vim25Api1__ArrayOfManagedObjectReference*>(get_property_from_mor(service_content, *folder, L"childEntity"));

        Vim25Api1__ManagedObjectReference* parentFolder = folder;
        folder = NULL;
        for (size_t iChildren = 0; iChildren < children->ManagedObjectReference.size(); ++iChildren)
        {
            Vim25Api1__ManagedObjectReference* child =
                children->ManagedObjectReference.at(iChildren);

            if (!child || !child->type)
            {
                continue;
            }

            if (*child->type == L"Folder")
            {
                xsd__string* name =
                    reinterpret_cast<xsd__string*>(get_property_from_mor(service_content, *child, L"name"));

                if (name && 0 == _wcsicmp(folderName.c_str(), name->__item.c_str()))
                {
                    folder = child;
                    break;
                }
            }
        }

        if (!folder && can_be_create){
            folder = create_vm_folder_internal(service_content, *parentFolder, folderName);
        }

        if (!folder){
            return NULL;
        }
    }

    return folder;
}

Vim25Api1__ManagedObjectReference* vmware_portal::create_vm_folder_internal(Vim25Api1__ServiceContent& service_content, Vim25Api1__ManagedObjectReference& parentFolder, const std::wstring& name){
    FUN_TRACE;
    Vim25Api1__CreateFolderRequestType request;
    request._USCOREthis = &parentFolder;
    request.name = name;

    _Vim25Api1__CreateFolderResponse response;
    int nReturn = 0;

    {
        journal_transaction trans(this);
        nReturn = _vim_binding.CreateFolder(&request, response);
    }

    if (SOAP_OK != nReturn){
        int lastErrorCode;
        std::wstring lastErrorMessage;
        get_native_error(lastErrorCode, lastErrorMessage);
        LOG(LOG_LEVEL_ERROR, L"CreateFolder error: %s (%d)", lastErrorMessage.c_str(), nReturn);
        return NULL;
    }

    return response.returnval;
}

Vim25Api1__VirtualDeviceConfigSpec* vmware_portal::create_serial_port_spec(Vim25Api1__ServiceContent& service_content){
    FUN_TRACE;
    Vim25Api1__VirtualDeviceConfigSpec                *serial_port_config_spec = soap_new_Vim25Api1__VirtualDeviceConfigSpec(&_vim_binding, -1);
    Vim25Api1__VirtualSerialPort                       *serialport = NULL;
    Vim25Api1__VirtualDeviceConfigSpecOperation_ *operation_ = soap_new_Vim25Api1__VirtualDeviceConfigSpecOperation_(&_vim_binding, -1);
    serial_port_config_spec->operation = &operation_->__item;
    *serial_port_config_spec->operation = Vim25Api1__VirtualDeviceConfigSpecOperation(Vim25Api1__VirtualDeviceConfigSpecOperation__add);
    serialport = soap_new_Vim25Api1__VirtualSerialPort(&_vim_binding, -1);
    Vim25Api1__VirtualSerialPortPipeBackingInfo *backing = soap_new_Vim25Api1__VirtualSerialPortPipeBackingInfo(&_vim_binding, -1);
    backing->endpoint = L"client";
	backing->pipeName = L"PIPE_NAME";
    serialport->backing = backing;
    serialport->key = -DEV_KEY::SERIAL_PORT;
    serialport->yieldOnPoll = true;
    serial_port_config_spec->device = serialport;
    return serial_port_config_spec;
}

Vim25Api1__VirtualDeviceConfigSpec* vmware_portal::create_floppy_spec(Vim25Api1__ServiceContent& service_content){
    FUN_TRACE;
    Vim25Api1__VirtualDeviceConfigSpec                *floppy_config_spec = soap_new_Vim25Api1__VirtualDeviceConfigSpec(&_vim_binding, -1);
    Vim25Api1__VirtualFloppy                          *floppy = NULL;
    Vim25Api1__VirtualDeviceConfigSpecOperation_ *operation_ = soap_new_Vim25Api1__VirtualDeviceConfigSpecOperation_(&_vim_binding, -1);
    floppy_config_spec->operation = &operation_->__item;
    *floppy_config_spec->operation = Vim25Api1__VirtualDeviceConfigSpecOperation(Vim25Api1__VirtualDeviceConfigSpecOperation__add);
    floppy = soap_new_Vim25Api1__VirtualFloppy(&_vim_binding, -1);
    Vim25Api1__VirtualFloppyRemoteDeviceBackingInfo   *pFloppyBacking = soap_new_Vim25Api1__VirtualFloppyRemoteDeviceBackingInfo(&_vim_binding, -1);
    pFloppyBacking->deviceName = L"";
    floppy->backing = pFloppyBacking;
    floppy->key = -DEV_KEY::FLOPPY;
    floppy_config_spec->device = floppy;
    return floppy_config_spec;
}

Vim25Api1__VirtualDeviceConfigSpec* vmware_portal::create_cd_rom_spec(Vim25Api1__ServiceContent& service_content, Vim25Api1__ManagedObjectReference* host_mor){
    FUN_TRACE;
    Vim25Api1__ManagedObjectReference*        cluster_or_compute_resource_mor = NULL;
    Vim25Api1__ManagedObjectReference *       envoronment_browser_mor = NULL;
    Vim25Api1__VirtualMachineConfigOption*    config_opt = NULL;
    Vim25Api1__VirtualIDEController*          ide_controller = NULL;
    int                                       nReturn;
    Vim25Api1__ArrayOfVirtualDevice           default_devices;

    Vim25Api1__VirtualDeviceConfigSpec*       cdrom_spec = NULL;
    Vim25Api1__VirtualCdrom*                  cdrom = NULL;
    Vim25Api1__VirtualCdromRemotePassthroughBackingInfo*  cdrom_device_backing = NULL;

    cluster_or_compute_resource_mor = get_ancestor_mor(service_content, host_mor, L"ComputeResource");
    if (!cluster_or_compute_resource_mor)
        cluster_or_compute_resource_mor = get_ancestor_mor(service_content, host_mor, L"ClusterComputeResource");

    if (cluster_or_compute_resource_mor){
        envoronment_browser_mor = (Vim25Api1__ManagedObjectReference *)get_property_from_mor(service_content, *cluster_or_compute_resource_mor, L"environmentBrowser");
        Vim25Api1__QueryConfigOptionRequestType   queryConfigOptionRequest;
        _Vim25Api1__QueryConfigOptionResponse     queryConfigOptionResponse;

        queryConfigOptionRequest._USCOREthis = envoronment_browser_mor;
        queryConfigOptionRequest.host = (Vim25Api1__ManagedObjectReference *)host_mor;

        {
            journal_transaction trans(this);
            nReturn = _vim_binding.QueryConfigOption(&queryConfigOptionRequest, queryConfigOptionResponse);
        }

        if (SOAP_OK != nReturn){
            int lastErrorCode;
            std::wstring lastErrorMessage;
            get_native_error(lastErrorCode, lastErrorMessage);
            LOG(LOG_LEVEL_ERROR, L"QueryConfigOption error: %s (%d)", lastErrorMessage.c_str(), nReturn);
            return NULL;
        }

        config_opt = queryConfigOptionResponse.returnval;

        if (!config_opt){
            LOG(LOG_LEVEL_ERROR, L" No VirtualHardwareInfo found in ComputeResource for CD-ROM.");
            return NULL;
        }
        else{
            default_devices.VirtualDevice = config_opt->defaultDevice;
        }
    }

    for (size_t i = 0; i < default_devices.VirtualDevice.size(); i++){
        ide_controller = dynamic_cast< Vim25Api1__VirtualIDEController * >(default_devices.VirtualDevice[i]);
        if (ide_controller)
            break;
    }

    if (ide_controller){
        cdrom_spec = soap_new_Vim25Api1__VirtualDeviceConfigSpec(&_vim_binding, -1);
        Vim25Api1__VirtualDeviceConfigSpecOperation_ *operation_ = soap_new_Vim25Api1__VirtualDeviceConfigSpecOperation_(&_vim_binding, -1);
        cdrom_spec->operation = &operation_->__item;
        *cdrom_spec->operation = Vim25Api1__VirtualDeviceConfigSpecOperation(Vim25Api1__VirtualDeviceConfigSpecOperation__add);
        cdrom = soap_new_Vim25Api1__VirtualCdrom(&_vim_binding, -1);
        cdrom_device_backing = soap_new_Vim25Api1__VirtualCdromRemotePassthroughBackingInfo(&_vim_binding, -1);
        cdrom_device_backing->deviceName = L"";
        cdrom_device_backing->exclusive = FALSE;
        cdrom->backing = cdrom_device_backing;
        cdrom->controllerKey = &ide_controller->key;
        cdrom->key = -DEV_KEY::CDROM;
        cdrom->unitNumber = soap_new_int(&_vim_binding, -1);
        *cdrom->unitNumber = 0;
        cdrom_spec->device = cdrom;
    }
    else{
        LOG(LOG_LEVEL_ERROR, L"Default IDE controller not found in environment browser.");
    }
    return cdrom_spec;
}

Vim25Api1__VirtualDeviceConfigSpec* vmware_portal::create_nic_spec(Vim25Api1__ServiceContent& service_content, Vim25Api1__ManagedObjectReference* host_mor, DEV_CONFIGSPEC_OP op, vmware_virtual_network_adapter::ptr nic){
    FUN_TRACE;
    if (!nic){
        LOG(LOG_LEVEL_ERROR, L"Invalid parameter.");
        return NULL;
    }
    Vim25Api1__VirtualDeviceConfigSpecOperation_* operation = soap_new_Vim25Api1__VirtualDeviceConfigSpecOperation_(&_vim_binding, -1);
    bool needBacking = true;
    switch (op){
    case DEV_CONFIGSPEC_OP::OP_ADD:
        operation->__item = Vim25Api1__VirtualDeviceConfigSpecOperation__add;
        break;
    case DEV_CONFIGSPEC_OP::OP_REMOVE:
        operation->__item = Vim25Api1__VirtualDeviceConfigSpecOperation__remove;
        needBacking = false;
        break;
    case DEV_CONFIGSPEC_OP::OP_EDIT:
        operation->__item = Vim25Api1__VirtualDeviceConfigSpecOperation__edit;
        break;
    default:
        operation->__item = Vim25Api1__VirtualDeviceConfigSpecOperation__add;
        break;
    }
    Vim25Api1__VirtualDeviceConnectInfo* connection_info = soap_new_Vim25Api1__VirtualDeviceConnectInfo(&_vim_binding, -1);
    connection_info->allowGuestControl = nic->is_allow_guest_control;
    connection_info->connected = nic->is_connected;
    connection_info->startConnected = nic->is_start_connected;
    Vim25Api1__VirtualDeviceBackingInfo* backing = NULL;

    if (needBacking && !nic->network.empty()){
        vmware_virtual_network::vtr networks = get_virtual_networks_internal(service_content, { host_mor }, nic->network);
        if (!networks.empty() && networks[0]->is_distributed()){
            vmware_virtual_network::ptr network = networks[0];
            Vim25Api1__VirtualEthernetCardDistributedVirtualPortBackingInfo* distributed_backing = soap_new_Vim25Api1__VirtualEthernetCardDistributedVirtualPortBackingInfo(&_vim_binding, -1);
            distributed_backing->port = soap_new_Vim25Api1__DistributedVirtualSwitchPortConnection(&_vim_binding, -1);
            distributed_backing->port->switchUuid = network->distributed_switch_uuid;
            distributed_backing->port->portgroupKey = soap_new_std__wstring(&_vim_binding, -1);
            *distributed_backing->port->portgroupKey = network->distributed_port_group_key;

            backing = distributed_backing;
        }
        else{
            Vim25Api1__VirtualEthernetCardNetworkBackingInfo* nic_backing = soap_new_Vim25Api1__VirtualEthernetCardNetworkBackingInfo(&_vim_binding, -1);
            nic_backing->deviceName = nic->network;
            backing = nic_backing;
        }
    }
    Vim25Api1__VirtualEthernetCard* _nic = NULL;
    std::wstring type = stringutils::toupper(nic->type);
    if (!type.compare(L"E1000"))
        _nic = soap_new_Vim25Api1__VirtualE1000(&_vim_binding, -1);
    else if (!type.compare(L"E1000E"))
        _nic = soap_new_Vim25Api1__VirtualE1000e(&_vim_binding, -1);
    else if (!type.compare(L"VMXNET"))
        _nic = soap_new_Vim25Api1__VirtualVmxnet(&_vim_binding, -1);
    else if (!type.compare(L"VMXNET2"))
        _nic = soap_new_Vim25Api1__VirtualVmxnet2(&_vim_binding, -1);
    else if (!type.compare(L"VMXNET3"))
        _nic = soap_new_Vim25Api1__VirtualVmxnet3(&_vim_binding, -1);
    else
        _nic = soap_new_Vim25Api1__VirtualPCNet32(&_vim_binding, -1);

    _nic->connectable = connection_info;
    _nic->backing = backing;
    _nic->key = nic->key;

    _nic->addressType = soap_new_std__wstring(&_vim_binding, -1);
    if (!nic->mac_address.empty()){
        if (!nic->address_type.empty()){
            _nic->addressType->assign(nic->address_type);
        }
        else if (0 == nic->mac_address.find(L"00:50:56:")){
            _nic->addressType->assign(L"manual");
        }
        else{
            _nic->addressType->assign(L"assigned");
        }
        _nic->macAddress = soap_new_std__wstring(&_vim_binding, -1);
        _nic->macAddress->assign(nic->mac_address);
    }
    else{
        _nic->addressType->assign(L"generated");
    }

    Vim25Api1__VirtualDeviceConfigSpec* nic_spec = soap_new_Vim25Api1__VirtualDeviceConfigSpec(&_vim_binding, -1);
    nic_spec->operation = &operation->__item;
    nic_spec->device = _nic;

    return nic_spec;
}

bool  vmware_portal::reconfig_virtual_machine(Vim25Api1__ServiceContent& service_content, Vim25Api1__ManagedObjectReference& vm_mor, Vim25Api1__VirtualMachineConfigSpec& spec){
    FUN_TRACE;
    bool                                        result = false;
    int                                         nReturn;
    Vim25Api1__ReconfigVMRequestType            reconfigvm_request;
    _Vim25Api1__ReconfigVM_USCORETaskResponse   reconfigvm_responsetask;

    reconfigvm_request._USCOREthis = (Vim25Api1__ManagedObjectReference *)&vm_mor;
    reconfigvm_request.spec = (Vim25Api1__VirtualMachineConfigSpec *)&spec;

    // Asynchronous Function
    {
        journal_transaction trans(this);
        nReturn = _vim_binding.ReconfigVM_USCORETask(&reconfigvm_request, reconfigvm_responsetask);
    }

    if (SOAP_OK != nReturn)
    {
        int lastErrorCode;
        std::wstring lastErrorMessage;
        get_native_error(lastErrorCode, lastErrorMessage);
        LOG(LOG_LEVEL_ERROR, L"ReconfigVM_USCORETask error: %s (%d)", lastErrorMessage.c_str(), nReturn);
        return NULL;
    }
    else
    {
        result = true;
        vmware_vm_task_info::ptr task_info(new vmware_vm_task_info());
        task_info->name = L"ReconfigVM_USCORETask";
        std::wstring task_display_name = L"modify virtual machine";
        try
        {
            if ((nReturn = wait_task_completion(service_content, *reconfigvm_responsetask.returnval, *task_info, task_display_name)) != TASK_STATE::STATE_SUCCESS && nReturn != TASK_STATE::STATE_QUEUED)
                result = false;
        }
        catch (boost::exception &e)
        {
            result = false;
        }
    }
    return result;
}

std::vector<Vim25Api1__VirtualDeviceConfigSpec*> vmware_portal::create_vmdks_spec(Vim25Api1__ServiceContent& service_content, vmware_virtual_machine::ptr vm, const vmware_disk_info::vtr disks){
    FUN_TRACE;
    std::vector<Vim25Api1__VirtualDeviceConfigSpec*> result;
    typedef std::map<int, vmware_virtual_scsi_controller::ptr> vmware_virtual_scsi_controller_map_type;
    vmware_virtual_scsi_controller_map_type controllers;
    int i = 0;
    int available_key = DEV_KEY::DISK;
    int available_ctrl_key = DEV_KEY::SCSICTRL;
    foreach(vmware_virtual_scsi_controller::ptr &c, vm->scsi_controllers){
        if (c->key >= available_ctrl_key)
            available_ctrl_key = c->key + 1;
        controllers[c->bus_number] = c;
    }
    foreach(const vmware_disk_info::ptr &d, vm->disks){
        if (d->key >= available_key)
            available_key = d->key + 1;
    }
    foreach(vmware_disk_info::ptr d, disks){
        if (-1 == d->controller->bus_number){
            d->controller->bus_number = 0;
            bool available = false;
            foreach(vmware_virtual_scsi_controller_map_type::value_type &c, controllers){
                if (c.second->type == d->controller->type && c.second->sharing == d->controller->sharing){
                    for (int i = 0; i < c.second->slots.size(); i++){
                        if (!c.second->slots[i]){
                            available = true;
                            d->controller = c.second;
                            break;
                        }
                    }
                    if (available)
                        break;
                }
            }
        }
        if (!controllers.count(d->controller->bus_number)){
            if (-1 == d->controller->key){
                d->controller->key = -(int)(available_ctrl_key++);
            }
            controllers[d->controller->bus_number] = d->controller;
            result.push_back(create_scsi_ctrl_spec(service_content, *d->controller));
        }
        else{
            d->controller = controllers[d->controller->bus_number];
        }
        if (-1 == d->unit_number){
            for (int i = 0; i < d->controller->slots.size(); i++){
                if (!d->controller->slots[i]){
                    d->controller->slots[i] = true;
                    d->unit_number = i;
                    break;
                }
            }
        }
        else{
            d->controller->slots[d->unit_number] = true;
        }
        d->key = -(int)(available_key + i);
        i++;
        result.push_back(create_virtual_disk_spec(service_content, d, DEV_CONFIGSPEC_OP::OP_ADD, DEV_CONFIGSPEC_FILEOP::FILEOP_CREATE));
    }
    return result;
}

Vim25Api1__VirtualDeviceConfigSpec* vmware_portal::create_scsi_ctrl_spec(Vim25Api1__ServiceContent& service_content, const vmware_virtual_scsi_controller& ctrl, DEV_CONFIGSPEC_OP op ){
    FUN_TRACE;
    Vim25Api1__VirtualSCSIController                  *scsi_ctrl = NULL;
    Vim25Api1__VirtualDeviceConfigSpec                *scsi_ctrl_config_spec = NULL;
    scsi_ctrl_config_spec = soap_new_Vim25Api1__VirtualDeviceConfigSpec(&_vim_binding, -1);
    Vim25Api1__VirtualDeviceConfigSpecOperation_ *operation_ = soap_new_Vim25Api1__VirtualDeviceConfigSpecOperation_(&_vim_binding, -1);
    scsi_ctrl_config_spec->operation = &operation_->__item;
    switch (op)
    {
    case DEV_CONFIGSPEC_OP::OP_ADD:
        *scsi_ctrl_config_spec->operation = Vim25Api1__VirtualDeviceConfigSpecOperation(Vim25Api1__VirtualDeviceConfigSpecOperation__add);
        break;
    case DEV_CONFIGSPEC_OP::OP_REMOVE:
        *scsi_ctrl_config_spec->operation = Vim25Api1__VirtualDeviceConfigSpecOperation(Vim25Api1__VirtualDeviceConfigSpecOperation__remove);
        break;
    case DEV_CONFIGSPEC_OP::OP_EDIT:
        *scsi_ctrl_config_spec->operation = Vim25Api1__VirtualDeviceConfigSpecOperation(Vim25Api1__VirtualDeviceConfigSpecOperation__edit);
        break;
    default:
        *scsi_ctrl_config_spec->operation = Vim25Api1__VirtualDeviceConfigSpecOperation(Vim25Api1__VirtualDeviceConfigSpecOperation__add);
        break;
    }
    if (hv_controller_type::HV_CTRL_LSI_LOGIC == ctrl.type){
        scsi_ctrl = soap_new_Vim25Api1__VirtualLsiLogicController(&_vim_binding, -1);
    }
    else if (hv_controller_type::HV_CTRL_PARA_VIRT_SCSI == ctrl.type){
        scsi_ctrl = soap_new_Vim25Api1__ParaVirtualSCSIController(&_vim_binding, -1);
    }
    else if (hv_controller_type::HV_CTRL_BUS_LOGIC == ctrl.type){
        scsi_ctrl = soap_new_Vim25Api1__VirtualBusLogicController(&_vim_binding, -1);
    }
    else if (hv_controller_type::HV_CTRL_LSI_LOGIC_SAS == ctrl.type){
        scsi_ctrl = soap_new_Vim25Api1__VirtualLsiLogicSASController(&_vim_binding, -1);
    }

    scsi_ctrl->key = ctrl.key;
    scsi_ctrl->busNumber = ctrl.bus_number;
    if (hv_bus_sharing::HV_BUS_SHARING_NONE == ctrl.sharing)
        scsi_ctrl->sharedBus = Vim25Api1__VirtualSCSISharing__noSharing;
    else if (hv_bus_sharing::HV_BUS_SHARING_PHYSICAL == ctrl.sharing)
        scsi_ctrl->sharedBus = Vim25Api1__VirtualSCSISharing__physicalSharing;
    else if (hv_bus_sharing::HV_BUS_SHARING_VIRTUAL == ctrl.sharing)
        scsi_ctrl->sharedBus = Vim25Api1__VirtualSCSISharing__virtualSharing;

    scsi_ctrl_config_spec->device = scsi_ctrl;

    return scsi_ctrl_config_spec;
}

Vim25Api1__VirtualDeviceConfigSpec* vmware_portal::create_virtual_disk_spec(Vim25Api1__ServiceContent& service_content, vmware_disk_info::ptr disk, DEV_CONFIGSPEC_OP op, DEV_CONFIGSPEC_FILEOP file_op){
    FUN_TRACE;
    Vim25Api1__VirtualDeviceConfigSpec*           disk_spec = NULL;
    Vim25Api1__VirtualDisk*                       virtual_disk = NULL;

    if (disk_spec = soap_new_Vim25Api1__VirtualDeviceConfigSpec(&_vim_binding, -1))
    {
        if (op != DEV_CONFIGSPEC_OP::OP_UNSPECIFIED)
        {
            Vim25Api1__VirtualDeviceConfigSpecOperation_ *operation_ = soap_new_Vim25Api1__VirtualDeviceConfigSpecOperation_(&_vim_binding, -1);
            disk_spec->operation = &operation_->__item;
            switch (op)
            {
            case DEV_CONFIGSPEC_OP::OP_ADD:
                *disk_spec->operation = Vim25Api1__VirtualDeviceConfigSpecOperation__add;
                break;
            case DEV_CONFIGSPEC_OP::OP_REMOVE:
                *disk_spec->operation = Vim25Api1__VirtualDeviceConfigSpecOperation__remove;
                break;
            case DEV_CONFIGSPEC_OP::OP_EDIT:
                *disk_spec->operation = Vim25Api1__VirtualDeviceConfigSpecOperation__edit;
                break;
            default:
                *disk_spec->operation = Vim25Api1__VirtualDeviceConfigSpecOperation__add;
                break;
            }
        }

        virtual_disk = soap_new_Vim25Api1__VirtualDisk(&_vim_binding, -1);
    
        //VMDK
        if (file_op != DEV_CONFIGSPEC_FILEOP::FILEOP_UNSPECIFIED)
        {
            Vim25Api1__VirtualDeviceConfigSpecFileOperation_ *fileOperation_ = soap_new_Vim25Api1__VirtualDeviceConfigSpecFileOperation_(&_vim_binding, -1);
            disk_spec->fileOperation = &fileOperation_->__item;
            switch (file_op)
            {
            case DEV_CONFIGSPEC_FILEOP::FILEOP_CREATE:
                *disk_spec->fileOperation = Vim25Api1__VirtualDeviceConfigSpecFileOperation__create;
                break;
            case DEV_CONFIGSPEC_FILEOP::FILEOP_DESTROY:
                *disk_spec->fileOperation = Vim25Api1__VirtualDeviceConfigSpecFileOperation__destroy;
                break;
            case DEV_CONFIGSPEC_FILEOP::FILEOP_REPLACE:
                *disk_spec->fileOperation = Vim25Api1__VirtualDeviceConfigSpecFileOperation__replace;
                break;
            default:
                *disk_spec->fileOperation = Vim25Api1__VirtualDeviceConfigSpecFileOperation__create;
                break;
            }
        }

        //Backing
        Vim25Api1__VirtualDiskFlatVer2BackingInfo *disk_backing = soap_new_Vim25Api1__VirtualDiskFlatVer2BackingInfo(&_vim_binding, -1);
        disk_backing->diskMode = L"independent_persistent";
        disk_backing->fileName = disk->name;
        if (disk->thin_provisioned && file_op == DEV_CONFIGSPEC_FILEOP::FILEOP_CREATE){
            disk_backing->thinProvisioned = soap_new_bool(&_vim_binding, -1);
            *disk_backing->thinProvisioned = disk->thin_provisioned;
        }
        if (!disk->uuid.empty()){
            disk_backing->uuid = soap_new_std__wstring(&_vim_binding, -1);
            *disk_backing->uuid = disk->uuid;
        }
        virtual_disk->backing = disk_backing;

        virtual_disk->capacityInKB = disk->size / 1024;

        virtual_disk->key = disk->key;

        xsd__int * controller_key = soap_new_xsd__int(&_vim_binding, -1);
        virtual_disk->controllerKey = &controller_key->__item;
        *virtual_disk->controllerKey = disk->controller->key;

        xsd__int * unit_number = soap_new_xsd__int(&_vim_binding, -1);
        virtual_disk->unitNumber = &unit_number->__item;
        *virtual_disk->unitNumber = disk->unit_number;

        disk_spec->device = virtual_disk;

        return disk_spec;
    }
    return(NULL);
}

vmware_virtual_machine::ptr vmware_portal::get_virtual_machine_from_mor(Vim25Api1__ServiceContent& service_content, Vim25Api1__ManagedObjectReference& vm_mor){
    FUN_TRACE;
    vmware_virtual_machine::ptr vm;
    int                             nReturn;
    Vim25Api1__PropertySpec               propertySpec;
    Vim25Api1__ObjectSpec                 objectSpec;
    Vim25Api1__PropertyFilterSpec         propertyFilterSpec;
    Vim25Api1__DynamicProperty            dynamicProperty;
    Vim25Api1__RetrievePropertiesRequestType  retrievePropertiesReq;
    _Vim25Api1__RetrievePropertiesResponse    retrievePropertiesRes;
    float                               version = 0;
    hv_connection_type                  type = HV_CONNECTION_TYPE_UNKNOWN;

    version = get_connection_info(service_content, type);

    objectSpec.obj = (Vim25Api1__ManagedObjectReference*)&vm_mor;
    objectSpec.skip = &xsd_false;

    propertySpec.type = L"VirtualMachine";
    propertySpec.all = &xsd_false;
    propertySpec.pathSet.push_back(L"name");

    propertySpec.pathSet.push_back(L"overallStatus");
    propertySpec.pathSet.push_back(L"config.guestId");
    propertySpec.pathSet.push_back(L"config.guestFullName");
    if (version >= 4.0)
        propertySpec.pathSet.push_back(L"config.changeTrackingEnabled");
    propertySpec.pathSet.push_back(L"config.hardware.device");
    propertySpec.pathSet.push_back(L"config.hardware.memoryMB");
    propertySpec.pathSet.push_back(L"config.hardware.numCPU");
    if (version >= 4.0){
        propertySpec.pathSet.push_back(L"config.cpuHotAddEnabled");
        propertySpec.pathSet.push_back(L"config.cpuHotRemoveEnabled");  //New in API 4.0
    }
    propertySpec.pathSet.push_back(L"config.template");
    propertySpec.pathSet.push_back(L"config.uuid");
    propertySpec.pathSet.push_back(L"config.extraConfig");
    propertySpec.pathSet.push_back(L"config.version");

    if (version >= 5.0)
        propertySpec.pathSet.push_back(L"config.firmware");

    propertySpec.pathSet.push_back(L"guest.hostName");
    propertySpec.pathSet.push_back(L"guest.ipAddress");
    propertySpec.pathSet.push_back(L"guest.net");
    if (version >= 5.0)
        propertySpec.pathSet.push_back(L"guest.toolsVersionStatus2");
    else if (version == 4.0)
        propertySpec.pathSet.push_back(L"guest.toolsVersionStatus");
    else
        propertySpec.pathSet.push_back(L"guest.toolsStatus");
    propertySpec.pathSet.push_back(L"runtime.connectionState");
    propertySpec.pathSet.push_back(L"runtime.host");
    propertySpec.pathSet.push_back(L"runtime.powerState");
    propertySpec.pathSet.push_back(L"summary.config.annotation");
    propertySpec.pathSet.push_back(L"summary.config.vmPathName");
    propertySpec.pathSet.push_back(L"summary.vm");
    propertySpec.pathSet.push_back(L"datastore");
    propertySpec.pathSet.push_back(L"network");
    propertySpec.pathSet.push_back(L"snapshot");
    propertySpec.pathSet.push_back(L"resourceConfig.entity");
    propertyFilterSpec.propSet.push_back(&propertySpec);
    propertyFilterSpec.objectSet.push_back(&objectSpec);

    retrievePropertiesReq._USCOREthis = service_content.propertyCollector;
    retrievePropertiesReq.specSet.push_back(&propertyFilterSpec);

    {
        journal_transaction trans(this);
        nReturn = _vim_binding.RetrieveProperties(&retrievePropertiesReq, retrievePropertiesRes);
    }

    if (SOAP_OK == nReturn)
    {
        mor_name_map                        mapHostMorName;
        mor_name_map                        mapNetworkMorName;
        key_name::map                       mapVmMorKeyName;
        mor_name_map                        mapHostMorClusterName;
        try{
            get_host_mor_name_map(service_content, mapHostMorName);
        }
        catch (...){
        }
        vm = fetch_virtual_machine_info(service_content,
            *retrievePropertiesRes.returnval[0],
            mapHostMorName,
            mapNetworkMorName,
            mapVmMorKeyName,
            mapHostMorClusterName,
            type);
        if (vm->key.empty()){
            // Probably a messed up VM without a UUID
            vm->key = vm->name;
        }
        else{
            key_name _key_name;
            _key_name.key = vm->key;
            _key_name.name = vm->name;
            mapVmMorKeyName[retrievePropertiesRes.returnval[0]->obj->__item] = _key_name;
        }
        //vmx spec
        std::wstring vmx_spec = vm->config_path_file;
        std::wstring config_path = L"[" + vm->config_path + L"] ";
        replace_all(vmx_spec, config_path, L"");
        vmx_spec = vmx_spec + L"?dcPath=" + vm->datacenter_name + L"&dsName=" + vm->config_path;
        vm->vmxspec = vmx_spec;
        vmware_folder::vtr vm_folders = get_vm_folder_internal(service_content, std::wstring(), mapVmMorKeyName);
        foreach(vmware_folder::ptr f, vm_folders){
            f->get_vm_folder_path(vm->key, vm->folder_path);
        }
    }
    return vm;
}

vmware_virtual_machine::ptr vmware_portal::fetch_virtual_machine_info(Vim25Api1__ServiceContent& service_content, Vim25Api1__ObjectContent& obj, mor_name_map &mapHostMorName, mor_name_map &mapNetworkMorName, key_name::map &mapVmMorKeyName, mor_name_map &mapHostMorClusterName, hv_connection_type &type, const std::wstring &host_key, const std::wstring &cluster_key){
    FUN_TRACE;
    vmware_virtual_machine::ptr pVirtualMachine(new vmware_virtual_machine());
    size_t propSize = obj.propSet.size();
    pVirtualMachine->connection_type = type;
    for (size_t j = 0; j < propSize; j++){
        Vim25Api1__DynamicProperty& dynamicProperty = *obj.propSet[j];
        if (dynamicProperty.name == L"config.uuid"){
            pVirtualMachine->key = pVirtualMachine->uuid = ((xsd__string *)dynamicProperty.val)->__item.c_str();
            /*if (!machine_key.empty() && 0 != _wcsicmp(machine_key.c_str(), pVirtualMachine->key.c_str())) {
            pVirtualMachine = NULL;
            break;
            }*/
        }
        else if (dynamicProperty.name == L"resourceConfig.entity"){
            Vim25Api1__ManagedObjectReference* pEntity =
                reinterpret_cast<Vim25Api1__ManagedObjectReference*>(dynamicProperty.val);
            if (pEntity){
                Vim25Api1__ManagedObjectReference* parent = (Vim25Api1__ManagedObjectReference*)get_property_from_mor(service_content, *pEntity, L"parent");
                if (parent){
                    pVirtualMachine->parent_mor_item = parent->__item;
                }
            }
        }
        else if (dynamicProperty.name == L"config.extraConfig"){
            Vim25Api1__ArrayOfOptionValue* pOptionValues =
                reinterpret_cast< Vim25Api1__ArrayOfOptionValue* >(dynamicProperty.val);

            for (size_t iOptionValue = 0; iOptionValue < pOptionValues->OptionValue.size(); iOptionValue++){
                Vim25Api1__OptionValue* pOptionValue = pOptionValues->OptionValue.at(iOptionValue);
                if (0 == _wcsicmp(pOptionValue->key.c_str(), L"disk.EnableUUID")){
                    pVirtualMachine->is_disk_uuid_enabled = is_true_value(pOptionValue->value);
                    break;
                }
            }
        }
        else if (dynamicProperty.name == L"config.version"){
            xsd__string* pVersion =
                reinterpret_cast< xsd__string* >(dynamicProperty.val);

            int nVersion = 0;
            if (1 == swscanf(pVersion->__item.c_str(), L"vmx-%d", &nVersion)){
                pVirtualMachine->version = nVersion;
            }
        }
        else if (dynamicProperty.name == L"name"){
            pVirtualMachine->name = ((xsd__string *)dynamicProperty.val)->__item.c_str();
        }
        else if (dynamicProperty.name == L"runtime.powerState"){
            switch (((Vim25Api1__VirtualMachinePowerState_ *)dynamicProperty.val)->__item)
            {
            case Vim25Api1__VirtualMachinePowerState__poweredOff:
                pVirtualMachine->power_state = HV_VMPOWER_OFF;
                break;
            case Vim25Api1__VirtualMachinePowerState__poweredOn:
                pVirtualMachine->power_state = HV_VMPOWER_ON;
                break;
            case Vim25Api1__VirtualMachinePowerState__suspended:
                pVirtualMachine->power_state = HV_VMPOWER_SUSPENDED;
                break;
            default:
                pVirtualMachine->power_state = HV_VMPOWER_UNKNOWN;
            }
        }
        else if (dynamicProperty.name == L"guest.toolsStatus"){
            if (dynamicProperty.val)
            {
                switch (((Vim25Api1__VirtualMachineToolsStatus_ *)dynamicProperty.val)->__item)
                {
                case Vim25Api1__VirtualMachineToolsStatus__toolsNotInstalled:
                    pVirtualMachine->tools_status = HV_VMTOOLS_NOTINSTALLED;
                    break;
                case Vim25Api1__VirtualMachineToolsStatus__toolsNotRunning:
                    pVirtualMachine->tools_status = HV_VMTOOLS_NOTRUNNING;
                    break;
                case Vim25Api1__VirtualMachineToolsStatus__toolsOld:
                    pVirtualMachine->tools_status = HV_VMTOOLS_OLD;
                    break;
                case Vim25Api1__VirtualMachineToolsStatus__toolsOk:
                    pVirtualMachine->tools_status = HV_VMTOOLS_OK;
                    break;
                default:
                    pVirtualMachine->tools_status = HV_VMTOOLS_UNKNOWN;
                    break;
                }
            }
        }
        else if (dynamicProperty.name == L"guest.toolsVersionStatus" || dynamicProperty.name == L"guest.toolsVersionStatus2")
        {
            if (dynamicProperty.val)
            {
                std::wstring toolStatus = ((xsd__string *)dynamicProperty.val)->__item;

                if (toolStatus == L"guestToolsNotInstalled")
                    pVirtualMachine->tools_status = HV_VMTOOLS_NOTINSTALLED;
                else if (toolStatus == L"guestToolsNeedUpgrade")
                    pVirtualMachine->tools_status = HV_VMTOOLS_NEEDUPGRADE;
                else if (toolStatus == L"guestToolsUnmanaged")
                    pVirtualMachine->tools_status = HV_VMTOOLS_UNMANAGED;
                else if (toolStatus == L"guestToolsTooOld")
                    pVirtualMachine->tools_status = HV_VMTOOLS_OLD;
                else if (toolStatus == L"guestToolsTooNew")
                    pVirtualMachine->tools_status = HV_VMTOOLS_NEW;
                else if (toolStatus == L"guestToolsBlacklisted")
                    pVirtualMachine->tools_status = HV_VMTOOLS_BLACKLISTED;
                else if (toolStatus == L"guestToolsCurrent" || toolStatus == L"guestToolsSupportedOld" || toolStatus == L"guestToolsSupportedNew")
                    pVirtualMachine->tools_status = HV_VMTOOLS_OK;
                else
                    pVirtualMachine->tools_status = HV_VMTOOLS_UNKNOWN;
            }
        }
        else if ((dynamicProperty.name == L"guest.ipAddress") && dynamicProperty.val)
            pVirtualMachine->guest_primary_ip = ((xsd__string *)dynamicProperty.val)->__item.c_str();
        else if ((dynamicProperty.name == L"guest.hostName") && dynamicProperty.val)
            pVirtualMachine->guest_host_name = ((xsd__string *)dynamicProperty.val)->__item.c_str();
        else if ((dynamicProperty.name == L"guest.net") && dynamicProperty.val)
        {
            for (size_t i = 0; i < ((Vim25Api1__ArrayOfGuestNicInfo *)dynamicProperty.val)->GuestNicInfo.size(); i++)
            {
                guest_nic_info nic_info;

                nic_info.mac_address = std::wstring((((Vim25Api1__ArrayOfGuestNicInfo *)dynamicProperty.val)->GuestNicInfo[i]->macAddress->c_str()));

                for (size_t j = 0; j < ((Vim25Api1__ArrayOfGuestNicInfo *)dynamicProperty.val)->GuestNicInfo[i]->ipAddress.size(); j++)
                {
                    std::wstring ip = std::wstring(((Vim25Api1__ArrayOfGuestNicInfo *)dynamicProperty.val)->GuestNicInfo[i]->ipAddress[j].c_str());
                    nic_info.ip_config.push_back(ip);
                }
                pVirtualMachine->guest_net.push_back(nic_info);
            }
        }
        else if (dynamicProperty.name == L"config.firmware"){
            std::wstring wszFirmware = ((xsd__string *)dynamicProperty.val)->__item.c_str();
            if (!_tcsicmp(wszFirmware.c_str(), _T("efi")))
                pVirtualMachine->firmware = HV_VM_FIRMWARE_EFI;
        }
        else if (dynamicProperty.name == L"config.changeTrackingEnabled"){
            pVirtualMachine->is_cbt_enabled = ((xsd__boolean *)dynamicProperty.val)->__item;
        }
        else if (dynamicProperty.name == L"config.guestFullName"){
            pVirtualMachine->guest_full_name = ((xsd__string *)dynamicProperty.val)->__item.c_str();
            if (_tcsstr(pVirtualMachine->guest_full_name.c_str(), L"Windows"))
                pVirtualMachine->guest_os_type = HV_OS_WINDOWS;
        }
        else if (dynamicProperty.name == L"config.guestId"){
            pVirtualMachine->guest_id = ((xsd__string *)dynamicProperty.val)->__item.c_str();
        }
        else if (dynamicProperty.name == L"runtime.host"){
            Vim25Api1__ManagedObjectReference *pHostMOR = (Vim25Api1__ManagedObjectReference *)dynamicProperty.val;
            if (!mapHostMorName.empty()){
                mor_name_map::iterator iHost = mapHostMorName.find(pHostMOR->__item);
                if (iHost == mapHostMorName.end() ||
                    (!host_key.empty() && 0 != _wcsicmp(host_key.c_str(), iHost->second.c_str()))){
                    pVirtualMachine = NULL;
                    break;
                }
                else{
                    pVirtualMachine->host_key = pVirtualMachine->host_name = iHost->second;
                    mor_name_map::const_iterator iCluster = mapHostMorClusterName.find(pHostMOR->__item);
                    if (iCluster != mapHostMorClusterName.end()){
                        pVirtualMachine->cluster_key = pVirtualMachine->cluster_name = iCluster->second;
                    }
                    else{
                        std::wstring wszClusterName;
                        string_array types;
                        types.push_back(L"ClusterComputeResource");
                        Vim25Api1__ManagedObjectReference* pClusterMOR = get_ancestor_mor(service_content, pHostMOR, types);
                        if (pClusterMOR){
                            try
                            {
                                get_cluster_name_from_mor(service_content, *pClusterMOR, wszClusterName);
                            }
                            catch (...) {}
                        }
                        mapHostMorClusterName[pHostMOR->__item] = wszClusterName;
                        pVirtualMachine->cluster_key = pVirtualMachine->cluster_name = wszClusterName;
                    }

                    if (!cluster_key.empty() && 0 != _wcsicmp(cluster_key.c_str(), pVirtualMachine->cluster_key.c_str())){
                        pVirtualMachine = NULL;
                        break;
                    }
                }
            }
        }
        else if ((dynamicProperty.name == L"summary.config.annotation") && dynamicProperty.val)
            pVirtualMachine->annotation = ((xsd__string *)dynamicProperty.val)->__item.c_str();
        else if ((dynamicProperty.name == L"config.cpuHotAddEnabled") && dynamicProperty.val)
            pVirtualMachine->is_cpu_hot_add = ((xsd__boolean *)dynamicProperty.val)->__item;
        else if ((dynamicProperty.name == L"config.cpuHotRemoveEnabled") && dynamicProperty.val)
            pVirtualMachine->is_cpu_hot_remove = ((xsd__boolean *)dynamicProperty.val)->__item;
        else if (dynamicProperty.name == L"config.hardware.memoryMB")
            pVirtualMachine->memory_mb = ((xsd__int *)dynamicProperty.val)->__item;
        else if (dynamicProperty.name == L"config.hardware.numCPU")
            pVirtualMachine->number_of_cpu = ((xsd__int *)dynamicProperty.val)->__item;
        else if (dynamicProperty.name == L"config.template")
            pVirtualMachine->is_template = ((xsd__boolean *)dynamicProperty.val)->__item;
        else if (dynamicProperty.name == L"runtime.connectionState"){
            switch (((Vim25Api1__VirtualMachineConnectionState_ *)dynamicProperty.val)->__item){
            case Vim25Api1__VirtualMachineConnectionState__connected:
                pVirtualMachine->connection_state = HV_VMCONNECT_CONNECTED;
                break;
            case Vim25Api1__VirtualMachineConnectionState__disconnected:
                pVirtualMachine->connection_state = HV_VMCONNECT_DISCONNECTED;
                break;
            case Vim25Api1__VirtualMachineConnectionState__orphaned:
                pVirtualMachine->connection_state = HV_VMCONNECT_ORPHANED;
                break;
            case Vim25Api1__VirtualMachineConnectionState__inaccessible:
                pVirtualMachine->connection_state = HV_VMCONNECT_INACCESSIBLE;
                break;
            case Vim25Api1__VirtualMachineConnectionState__invalid:
                pVirtualMachine->connection_state = HV_VMCONNECT_INVALID;
                break;
            default:
                pVirtualMachine->connection_state = HV_VMCONNECT_UNKNOWN;
                break;
            }
        }
        else if (dynamicProperty.name == L"network"){
            for (size_t i = 0; i < ((Vim25Api1__ArrayOfManagedObjectReference *)dynamicProperty.val)->ManagedObjectReference.size(); i++){
                Vim25Api1__ManagedObjectReference *pNetworkMOR = ((Vim25Api1__ArrayOfManagedObjectReference *)dynamicProperty.val)->ManagedObjectReference[i];
                if (!pNetworkMOR){
                    continue;
                }

                std::wstring wszNetwork;
                mor_name_map::iterator iNetwork = mapNetworkMorName.find(pNetworkMOR->__item);
                if (iNetwork != mapNetworkMorName.end()){
                    wszNetwork = iNetwork->second;
                }
                else{
                    try
                    {
                        get_virtual_network_name_from_mor(service_content, *pNetworkMOR, wszNetwork);
                    }
                    catch (...){}
                    mapNetworkMorName[pNetworkMOR->__item] = wszNetwork;
                }

                if (!wszNetwork.empty()){
                    //Key and name are same
                    pVirtualMachine->networks[wszNetwork] = wszNetwork;
                }
            }
        }
        else if (dynamicProperty.name == L"config.hardware.device"){
            for (size_t i = 0; i < ((Vim25Api1__ArrayOfVirtualDevice *)dynamicProperty.val)->VirtualDevice.size(); i++){

                Vim25Api1__VirtualIDEController* pVirtualIDEController = dynamic_cast< Vim25Api1__VirtualIDEController * >(((Vim25Api1__ArrayOfVirtualDevice *)dynamicProperty.val)->VirtualDevice[i]);
                if (pVirtualIDEController)
                    continue;
                Vim25Api1__VirtualLsiLogicSASController* pVirtualLsiLogicSASController = dynamic_cast< Vim25Api1__VirtualLsiLogicSASController * >(((Vim25Api1__ArrayOfVirtualDevice *)dynamicProperty.val)->VirtualDevice[i]);
                if (pVirtualLsiLogicSASController){
                    vmware_virtual_scsi_controller::ptr controller(new vmware_virtual_scsi_controller());
                    controller->type = HV_CTRL_LSI_LOGIC_SAS;
                    controller->key = pVirtualLsiLogicSASController->key;
                    controller->bus_number = pVirtualLsiLogicSASController->busNumber;
                    controller->sharing = (hv_bus_sharing)(int)pVirtualLsiLogicSASController->sharedBus;
                    pVirtualMachine->scsi_controllers.push_back(controller);
                    continue;
                }
                Vim25Api1__VirtualBusLogicController* pVirtualBusLogicController = dynamic_cast< Vim25Api1__VirtualBusLogicController * >(((Vim25Api1__ArrayOfVirtualDevice *)dynamicProperty.val)->VirtualDevice[i]);
                if (pVirtualBusLogicController){
                    vmware_virtual_scsi_controller::ptr controller(new vmware_virtual_scsi_controller());
                    controller->type = HV_CTRL_BUS_LOGIC;
                    controller->key = pVirtualBusLogicController->key;
                    controller->bus_number = pVirtualBusLogicController->busNumber;
                    controller->sharing = (hv_bus_sharing)(int)pVirtualBusLogicController->sharedBus;
                    pVirtualMachine->scsi_controllers.push_back(controller);
                    continue;
                }
                Vim25Api1__VirtualLsiLogicController* pVirtualLsiLogicController = dynamic_cast< Vim25Api1__VirtualLsiLogicController * >(((Vim25Api1__ArrayOfVirtualDevice *)dynamicProperty.val)->VirtualDevice[i]);
                if (pVirtualLsiLogicController){
                    vmware_virtual_scsi_controller::ptr controller(new vmware_virtual_scsi_controller());
                    controller->type = HV_CTRL_LSI_LOGIC;
                    controller->key = pVirtualLsiLogicController->key;
                    controller->bus_number = pVirtualLsiLogicController->busNumber;
                    controller->sharing = (hv_bus_sharing)(int)pVirtualLsiLogicController->sharedBus;
                    pVirtualMachine->scsi_controllers.push_back(controller);
                    continue;
                }
                Vim25Api1__ParaVirtualSCSIController* pParaVirtualSCSIController = dynamic_cast< Vim25Api1__ParaVirtualSCSIController * >(((Vim25Api1__ArrayOfVirtualDevice *)dynamicProperty.val)->VirtualDevice[i]);
                if (pParaVirtualSCSIController){
                    vmware_virtual_scsi_controller::ptr controller(new vmware_virtual_scsi_controller());
                    controller->type = HV_CTRL_PARA_VIRT_SCSI;
                    controller->key = pParaVirtualSCSIController->key;
                    controller->bus_number = pParaVirtualSCSIController->busNumber;
                    controller->sharing = (hv_bus_sharing)(int)pParaVirtualSCSIController->sharedBus;
                    pVirtualMachine->scsi_controllers.push_back(controller);
                    continue;
                }
                Vim25Api1__VirtualPS2Controller* pVirtualPS2Controller = dynamic_cast< Vim25Api1__VirtualPS2Controller * >(((Vim25Api1__ArrayOfVirtualDevice *)dynamicProperty.val)->VirtualDevice[i]);
                if (pVirtualPS2Controller)
                    continue;
                Vim25Api1__VirtualPCIController* pVirtualPCIController = dynamic_cast< Vim25Api1__VirtualPCIController * >(((Vim25Api1__ArrayOfVirtualDevice *)dynamicProperty.val)->VirtualDevice[i]);
                if (pVirtualPCIController)
                    continue;
                Vim25Api1__VirtualSIOController* pVirtualSIOController = dynamic_cast< Vim25Api1__VirtualSIOController * >(((Vim25Api1__ArrayOfVirtualDevice *)dynamicProperty.val)->VirtualDevice[i]);
                if (pVirtualSIOController)
                    continue;
                Vim25Api1__VirtualController* pVirtualController = dynamic_cast< Vim25Api1__VirtualController * >(((Vim25Api1__ArrayOfVirtualDevice *)dynamicProperty.val)->VirtualDevice[i]);
                if (pVirtualController)
                    continue;
                Vim25Api1__VirtualCdrom* pVirtualCdrom = dynamic_cast< Vim25Api1__VirtualCdrom * >(((Vim25Api1__ArrayOfVirtualDevice *)dynamicProperty.val)->VirtualDevice[i]);
                if (pVirtualCdrom){
                    if (pVirtualCdrom->controllerKey){
                        pVirtualMachine->has_cdrom = true;
                        pVirtualMachine->cdrom = vmware_device::ptr(new vmware_device());
                        pVirtualMachine->cdrom->controller_key = *pVirtualCdrom->controllerKey;
                        pVirtualMachine->cdrom->key = pVirtualCdrom->key;
                    }
                    continue;
                }

                Vim25Api1__VirtualSerialPort* pVirtualSerialPort = dynamic_cast< Vim25Api1__VirtualSerialPort * >(((Vim25Api1__ArrayOfVirtualDevice *)dynamicProperty.val)->VirtualDevice[i]);
                if (pVirtualSerialPort){
                    pVirtualMachine->has_serial_port = true;
                    continue;
                }

                Vim25Api1__VirtualDisk* pVirtualDisk = dynamic_cast< Vim25Api1__VirtualDisk * >(((Vim25Api1__ArrayOfVirtualDevice *)dynamicProperty.val)->VirtualDevice[i]);
                if (pVirtualDisk){
                    if ((pVirtualDisk->backing) && dynamic_cast< Vim25Api1__VirtualDeviceFileBackingInfo * >(pVirtualDisk->backing)){
                        Vim25Api1__VirtualDeviceFileBackingInfo *pFileBackingInfo = dynamic_cast< Vim25Api1__VirtualDeviceFileBackingInfo * >(pVirtualDisk->backing);
                        vmware_disk_info::ptr disk(new vmware_disk_info());
                        disk->key = pVirtualDisk->key;
                        disk->name = pFileBackingInfo->fileName;
                        if (pVirtualDisk->controllerKey){
                            foreach(vmware_virtual_scsi_controller::ptr& ctrl, pVirtualMachine->scsi_controllers){
                                if (ctrl->key == *pVirtualDisk->controllerKey){
                                    disk->controller = ctrl;
                                    break;
                                }
                            }
                        }
                        if (pVirtualDisk->unitNumber){
                            disk->unit_number = *pVirtualDisk->unitNumber;
                            if (disk->controller)
                                disk->controller->slots[disk->unit_number] = true;
                        }
                        if (pVirtualDisk->capacityInBytes)
                            disk->size = *pVirtualDisk->capacityInBytes;
                        else
                            disk->size = pVirtualDisk->capacityInKB * 1024;

                        if (dynamic_cast<Vim25Api1__VirtualDiskFlatVer2BackingInfo *>(pFileBackingInfo))
                        {
                            Vim25Api1__VirtualDiskFlatVer2BackingInfo *pBackingInfo = dynamic_cast<Vim25Api1__VirtualDiskFlatVer2BackingInfo *>(pFileBackingInfo);
                            if (pBackingInfo)
                            {
                                if (pBackingInfo->uuid)
                                    disk->uuid = std::wstring(pBackingInfo->uuid->c_str());
                                if (pBackingInfo->thinProvisioned)
                                    disk->thin_provisioned = *pBackingInfo->thinProvisioned;
                            }
                        }

                        if (dynamic_cast<Vim25Api1__VirtualDiskSparseVer2BackingInfo *>(pFileBackingInfo))
                        {
                            Vim25Api1__VirtualDiskSparseVer2BackingInfo *pBackingInfo = dynamic_cast<Vim25Api1__VirtualDiskSparseVer2BackingInfo *>(pFileBackingInfo);
                            if (pBackingInfo)
                            {
                                if (pBackingInfo->uuid)
                                    disk->uuid = std::wstring(pBackingInfo->uuid->c_str());
                            }
                        }

                        if (dynamic_cast<Vim25Api1__VirtualDiskSeSparseBackingInfo *>(pFileBackingInfo))
                        {
                            Vim25Api1__VirtualDiskSeSparseBackingInfo *pBackingInfo = dynamic_cast<Vim25Api1__VirtualDiskSeSparseBackingInfo *>(pFileBackingInfo);
                            if (pBackingInfo)
                            {
                                if (pBackingInfo->uuid)
                                    disk->uuid = std::wstring(pBackingInfo->uuid->c_str());
                            }
                        }
                        pVirtualMachine->disks.push_back(disk);
                        if (disk->uuid.empty()){
                            pVirtualMachine->disks_map[disk->wsz_key()] = disk;
                        }
                        else
                            pVirtualMachine->disks_map[disk->uuid] = disk;
                    }
                    continue;
                }
                Vim25Api1__VirtualEthernetCard* pVirtualEthernetCard = dynamic_cast< Vim25Api1__VirtualEthernetCard * >(((Vim25Api1__ArrayOfVirtualDevice *)dynamicProperty.val)->VirtualDevice[i]);
                if (pVirtualEthernetCard){
                    vmware_virtual_network_adapter::ptr pVirtualNic =
                        get_virtual_network_adapter_from_object(service_content, mapNetworkMorName, *pVirtualEthernetCard);
                    pVirtualMachine->network_adapters.push_back(pVirtualNic);
                    continue;
                }
            }
        }
        else if (dynamicProperty.name == L"summary.config.vmPathName"){
            // parse out the datastore
            pVirtualMachine->config_path_file = ((xsd__string *)dynamicProperty.val)->__item.c_str();
            std::vector< std::wstring > output;
            output = macho::stringutils::tokenize(pVirtualMachine->config_path_file, L"[]", 0, false);
            if (!output.empty()){
                pVirtualMachine->config_path = output.at(0);
            }
        }
        else if (dynamicProperty.name == L"snapshot")
        {
            Vim25Api1__VirtualMachineSnapshotInfo *root_snap = reinterpret_cast< Vim25Api1__VirtualMachineSnapshotInfo* >(dynamicProperty.val);

            if (root_snap)
            {
                if (root_snap->currentSnapshot)
                {
                    pVirtualMachine->current_snapshot_mor_item = root_snap->currentSnapshot->__item;
                }

                for (std::vector<Vim25Api1__VirtualMachineSnapshotTree *>::iterator it = root_snap->rootSnapshotList.begin(); it != root_snap->rootSnapshotList.end(); ++it)
                {
                    Vim25Api1__VirtualMachineSnapshotTree& obj = *(Vim25Api1__VirtualMachineSnapshotTree *)(*it);
                    pVirtualMachine->root_snapshot_list.push_back(get_virtual_machine_snapshot_from_object(obj));
                }
            }
        }
        else if (dynamicProperty.name == L"summary.vm")
        {
            Vim25Api1__ManagedObjectReference *pVmMOR = dynamic_cast<Vim25Api1__ManagedObjectReference *>(dynamicProperty.val);
            pVirtualMachine->vm_mor_item = pVmMOR->__item;

            //Get datacenter
            //Vim25Api1__ManagedObjectReference *pVmMOR = dynamic_cast<Vim25Api1__ManagedObjectReference *>(dynamicProperty.val);
            Vim25Api1__ManagedObjectReference *pDatacenterMOR = (Vim25Api1__ManagedObjectReference *)get_ancestor_mor(service_content, pVmMOR, L"Datacenter");
            if (pDatacenterMOR){
                LPVOID pProperty = get_property_from_mor(service_content, *pDatacenterMOR, L"name");
                if (pProperty)
                    pVirtualMachine->datacenter_name = ((xsd__string *)pProperty)->__item;
            }
        }
        else if (dynamicProperty.name == L"datastore")
        {
            for (size_t i = 0; i < ((Vim25Api1__ArrayOfManagedObjectReference *)dynamicProperty.val)->ManagedObjectReference.size(); i++)
            {
                Vim25Api1__ManagedObjectReference *pDatastoreMOR = ((Vim25Api1__ArrayOfManagedObjectReference *)dynamicProperty.val)->ManagedObjectReference[i];

                Vim25Api1__ObjectSpec objectSpec;
                objectSpec.obj = pDatastoreMOR;
                objectSpec.skip = &xsd_false;

                Vim25Api1__PropertySpec propertySpec;
                propertySpec.type = L"Datastore";
                propertySpec.pathSet.push_back(L"browser");
                propertySpec.all = &xsd_false;

                Vim25Api1__PropertyFilterSpec propertyFilterSpec;
                propertyFilterSpec.propSet.push_back(&propertySpec);
                propertyFilterSpec.objectSet.push_back(&objectSpec);

                Vim25Api1__DynamicProperty* dynamicProperty;
                Vim25Api1__RetrievePropertiesRequestType retrievePropertiesReq;
                _Vim25Api1__RetrievePropertiesResponse retrievePropertiesRes;
                retrievePropertiesReq._USCOREthis = service_content.propertyCollector;
                retrievePropertiesReq.specSet.push_back(&propertyFilterSpec);

                if (_vim_binding.RetrieveProperties(&retrievePropertiesReq, retrievePropertiesRes) == SOAP_OK)
                {
                    Vim25Api1__ObjectContent* oc = NULL;
                    for (int i = 0; i < retrievePropertiesRes.returnval.size(); i++)
                    {
                        oc = retrievePropertiesRes.returnval[i];

                        for (int j = 0; j < oc->propSet.size(); j++)
                        {
                            dynamicProperty = oc->propSet[j];
                            if (dynamicProperty->name == L"browser")
                            {
                                Vim25Api1__ManagedObjectReference pBrowserMOR = *((Vim25Api1__ManagedObjectReference *)dynamicProperty->val);

                                if (pVirtualMachine->datastore_browser_mor_item.empty())
                                {
                                    pVirtualMachine->datastore_browser_mor_item = pBrowserMOR.__item;
                                    LOG(LOG_LEVEL_DEBUG, L"Datastore browser item value %s", pBrowserMOR.__item.c_str());
                                }
                            }
                        }
                    }
                }
            }
        }
    } // for j
    return pVirtualMachine;
}

vmware_datacenter::vtr vmware_portal::get_datacenters(const std::wstring &name){
    return get_datacenters_internal(*(Vim25Api1__ServiceContent*)*_connect.get(), name);
}

vmware_datacenter::ptr vmware_portal::get_datacenter(const std::wstring &name){
    if (name.empty())
        return NULL;
    vmware_datacenter::vtr datacenters = get_datacenters(name);
    return datacenters.empty() ? NULL : datacenters[0];
}

vmware_datacenter::vtr vmware_portal::get_datacenters_internal(Vim25Api1__ServiceContent& service_content, const std::wstring &datacenter){
    FUN_TRACE;
    vmware_datacenter::vtr result;
    key_name::map vm_mor_key_name_map = get_virtual_machines_key_name_map(service_content);
    Vim25Api1__PropertySpec datacenterPropertySpec;
    datacenterPropertySpec.type = L"Datacenter";
    datacenterPropertySpec.all = &xsd_false;
    datacenterPropertySpec.pathSet.push_back(L"name");

    Vim25Api1__SelectionSpec folderSelectionSpec;
    folderSelectionSpec.name = soap_new_std__wstring(&_vim_binding, -1);
    *folderSelectionSpec.name = L"Folder";

    Vim25Api1__TraversalSpec folderTraversalSpec;
    folderTraversalSpec.name = folderSelectionSpec.name;
    folderTraversalSpec.type = L"Folder";
    folderTraversalSpec.path = L"childEntity";
    folderTraversalSpec.skip = &xsd_false;
    folderTraversalSpec.selectSet.push_back(&folderSelectionSpec);

    Vim25Api1__ObjectSpec objectSpec;
    objectSpec.obj = service_content.rootFolder;
    objectSpec.skip = &xsd_false;
    objectSpec.selectSet.push_back(&folderTraversalSpec);

    Vim25Api1__PropertyFilterSpec propertyFilterSpec;
    propertyFilterSpec.propSet.push_back(&datacenterPropertySpec);
    propertyFilterSpec.objectSet.push_back(&objectSpec);

    Vim25Api1__RetrievePropertiesRequestType retrievePropertiesReq;
    retrievePropertiesReq._USCOREthis = service_content.propertyCollector;
    retrievePropertiesReq.specSet.push_back(&propertyFilterSpec);
    _Vim25Api1__RetrievePropertiesResponse retrievePropertiesRes;
    int nReturn = 0;
    {
        journal_transaction trans(this);
        nReturn = _vim_binding.RetrieveProperties(&retrievePropertiesReq, retrievePropertiesRes);
    }
    
    for (size_t iReturnval = 0; iReturnval < retrievePropertiesRes.returnval.size(); ++iReturnval){
        Vim25Api1__ObjectContent* objectContent = retrievePropertiesRes.returnval.at(iReturnval);
        for (size_t iPropSet = 0; iPropSet < objectContent->propSet.size(); ++iPropSet){
            Vim25Api1__DynamicProperty* dynamicProperty = objectContent->propSet.at(iPropSet);
            if (dynamicProperty->name == L"name"){
                xsd__string* name = reinterpret_cast<xsd__string*>(dynamicProperty->val);			
                if (name){
                    vmware_datacenter::ptr _datacenter(new vmware_datacenter());
                    if (datacenter.empty()){
                        _datacenter->name = name->__item;
                        _datacenter->folders = get_vm_folder_internal(service_content, _datacenter->name, vm_mor_key_name_map);
                        result.push_back(_datacenter);
                    }
                    else if (0 == _wcsicmp(datacenter.c_str(), name->__item.c_str())){
                        _datacenter->name = name->__item;
                        _datacenter->folders = get_vm_folder_internal(service_content, _datacenter->name, vm_mor_key_name_map);
                        result.push_back(_datacenter);
                        break;
                    }
                }
            }
        }
    }

    //soap_delete_std__wstring(&_vim_binding, folderSelectionSpec.name);

    if (SOAP_OK != nReturn){
        int lastErrorCode;
        std::wstring lastErrorMessage;
        get_native_error(lastErrorCode, lastErrorMessage);
        LOG(LOG_LEVEL_ERROR, L"__Vim25Api1__RetrieveProperties error: %s (%d)", lastErrorMessage.c_str(), nReturn);
        BOOST_THROW_VMWARE_EXCEPTION(lastErrorCode, lastErrorMessage);
    }

    return result;
}