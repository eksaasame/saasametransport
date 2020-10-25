// WizardPacker.cpp : implementation file
//

#include "stdafx.h"
#include "irm_wizard.h"
#include "WizardPacker.h"
#include "afxdialogex.h"


// CWizardPacker dialog

IMPLEMENT_DYNAMIC(CWizardPacker, CPropertyPage)

CWizardPacker::CWizardPacker(register_info& info, SheetPos posPositionOnSheet)
: CWizardPage(posPositionOnSheet, CWizardPacker::IDD), _info(info)
{

}

CWizardPacker::~CWizardPacker()
{
}

void CWizardPacker::DoDataExchange(CDataExchange* pDX)
{
    CWizardPage::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_EDIT_PACKER, _packer);
    DDX_Control(pDX, IDC_PACKER_LIST, _packers);
}


BEGIN_MESSAGE_MAP(CWizardPacker, CWizardPage)
    ON_BN_CLICKED(IDC_BTN_REGISTER, &CWizardPacker::OnBnClickedBtnRegister)
END_MESSAGE_MAP()


// CWizardPacker message handlers


void CWizardPacker::OnBnClickedBtnRegister()
{
    BOOL Ret = FALSE;
    service_info Info;
    physical_machine_info MachineInfo;
    CString packer_addr;
    _packer.GetWindowTextW(packer_addr);
    if (packer_addr.IsEmpty())
        return;
    try{
        std::string packer = macho::stringutils::convert_unicode_to_utf8(packer_addr.GetString());
        if (GetPhysicalMachineInfo(packer, MachineInfo, Info)){
            std::string uri = saasame::transport::g_saasame_constants.MANAGEMENT_SERVICE_PATH;
            boost::shared_ptr<saasame::transport::management_serviceClient>    client;
            boost::shared_ptr<TTransport>                                      transport;
            if (_info.is_https){
                boost::shared_ptr<TCurlClient>           http = boost::shared_ptr<TCurlClient>(new TCurlClient(boost::str(boost::format("https://%s%s") % _info.addr %uri)));
                transport = boost::shared_ptr<TTransport>(new TBufferedTransport(http));
                boost::shared_ptr<TProtocol>             protocol = boost::shared_ptr<TProtocol>(new TBinaryProtocol(transport));
                client = boost::shared_ptr<saasame::transport::management_serviceClient>(new saasame::transport::management_serviceClient(protocol));
            }
            else{
                boost::shared_ptr<THttpClient>           http = boost::shared_ptr<THttpClient>(new THttpClient(_info.addr, 80, uri));
                transport = boost::shared_ptr<TTransport>(new TBufferedTransport(http));
                boost::shared_ptr<TProtocol>             protocol = boost::shared_ptr<TProtocol>(new TBinaryProtocol(transport));
                client = boost::shared_ptr<saasame::transport::management_serviceClient>(new saasame::transport::management_serviceClient(protocol));
            }
            try{
                transport->open();
                std::string packer_id;
                register_physical_packer_info packer_info;
                packer_info.mgmt_addr = _info.addr;
                packer_info.packer_addr = _info.machine_id;
                packer_info.username = _info.username;
                packer_info.password = _info.password;
                packer_info.path = Info.path;
                packer_info.version = Info.version;
                client->register_physical_packer(packer_id, _info.session, packer_info, MachineInfo);
                if (packer_id.length()){
                    AfxMessageBox(L"Registered!");
                    _packers.InsertString(0, packer_addr);
                }
                transport->close();
                
            }
            catch (saasame::transport::invalid_operation &ex){
                AfxMessageBox(macho::stringutils::convert_ansi_to_unicode(ex.why).c_str());
                LOG(LOG_LEVEL_ERROR, L"invalid_operation: %s", macho::stringutils::convert_ansi_to_unicode(ex.why).c_str());
                if (transport->isOpen())
                    transport->close();
            }
            catch (TException &tx){
                AfxMessageBox(macho::stringutils::convert_ansi_to_unicode(tx.what()).c_str());
                LOG(LOG_LEVEL_ERROR, L"TExceptioin: %s", macho::stringutils::convert_ansi_to_unicode(tx.what()).c_str());
                if (transport->isOpen())
                    transport->close();
            }
            curl_global_cleanup();
            SSL_COMP_free_compression_methods();
        }
    }
    catch (...){

    }
}

BOOL CWizardPacker::GetPhysicalMachineInfo(std::string host, physical_machine_info &MachineInfo, service_info &Info)
{
    macho::windows::registry reg;
    boost::filesystem::path p(macho::windows::environment::get_working_directory());
    boost::shared_ptr<TTransport> transport;
    if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
        if (reg[L"KeyPath"].exists() && reg[L"KeyPath"].is_string())
            p = reg[L"KeyPath"].wstring();
    }
    if (boost::filesystem::exists(p / "server.key") && boost::filesystem::exists(p / "server.crt"))
    {
        try
        {
            boost::shared_ptr<TSSLSocketFactory> factory;
            factory = boost::shared_ptr<TSSLSocketFactory>(new TSSLSocketFactory());
            factory->authenticate(false);
            factory->loadCertificate((p / "server.crt").string().c_str());
            factory->loadPrivateKey((p / "server.key").string().c_str());
            factory->loadTrustedCertificates((p / "server.crt").string().c_str());
            boost::shared_ptr<AccessManager> accessManager(new MyAccessManager());
            factory->access(accessManager);
            boost::shared_ptr<TSSLSocket> ssl_socket = boost::shared_ptr<TSSLSocket>(factory->createSocket(host, saasame::transport::g_saasame_constants.LOADER_SERVICE_PORT));
            ssl_socket->setConnTimeout(1000);
            transport = boost::shared_ptr<TTransport>(new TBufferedTransport(ssl_socket));
        }
        catch (TException& ex) {
            transport = nullptr;
            LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
        }
    }
    if (!transport){
        boost::shared_ptr<TSocket> socket(new TSocket(host, saasame::transport::g_saasame_constants.LOADER_SERVICE_PORT));
        socket->setConnTimeout(1000);
        transport = boost::shared_ptr<TTransport>(new TBufferedTransport(socket));
    }
    if (transport){
        boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
        common_serviceClient client(protocol);
        try {
            transport->open();
            client.ping(Info);
            client.get_host_detail(MachineInfo, "", machine_detail_filter::type::FULL);
            transport->close();
            return TRUE;
        }
        catch (TException& ex) {
            LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
        }
    }
    return FALSE;
}