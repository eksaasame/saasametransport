/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <iostream>

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TSSLServerSocket.h>
#include <thrift/transport/TTransportUtils.h>
#include <thrift/async/TEvhttpClientChannel.h>
#include <thrift/async/TAsyncChannel.h>
#include <thrift/transport/THttpClient.h>
#include <thrift/stdcxx.h>
#include <event.h>
#include <unistd.h>
#include <curl/curl.h>
#include <openssl/ssl.h>
#include <unistd.h>


#include "../gen-cpp/physical_packer_service.h"
#include "../gen-cpp/management_service.h"
#include "../gen-cpp/saasame_constants.h"
#include "../gen-cpp/saasame_types.h"

/*tools include*/
#include "../tools/string_operation.h"
#include "../tools/sslHelper.h"
#include "../tools/system_tools.h"
#include "../tools/string_operation.h"
#include "../tools/log.h"
#include "../tools/TCurlClient.h"
#include "../tools/service_status.h"
using namespace std;
using namespace std::placeholders;
using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
using namespace apache::thrift::async;
using namespace boost;
using namespace saasame;
using namespace saasame::transport;

/*define*/
#define TEST



#ifdef TEST
#define remote_target "localhost"
#else
#define remote_target "192.168.31.62"
#endif

#define CONFIG_FILE_PATH "register_config"

struct register_info {
    register_info() : is_https(true) {}
    std::string session;
    std::string machine_id;
    std::string addr;
    std::string username;
    std::string password;
    bool        is_https;
};

bool GetPhysicalMachineInfo(physical_machine_info &MachineInfo, std::set<service_info> &service_infos, int port)
{
    FUNC_TRACER;
    std::string host = "localhost";
    boost::filesystem::path p = boost::filesystem::path(SERVICE_CONFIG_FOLDER_PATH);
    std::shared_ptr<TTransport> transport;
    if (boost::filesystem::exists(p / "server.key") && boost::filesystem::exists(p / "server.crt"))
    {
        try
        {
            std::shared_ptr<TSSLSocketFactory> factory;
            factory = std::shared_ptr<TSSLSocketFactory>(new TSSLSocketFactory());
            factory->authenticate(false);
            factory->loadCertificate((p / "server.crt").string().c_str());
            factory->loadPrivateKey((p / "server.key").string().c_str());
            factory->loadTrustedCertificates((p / "server.crt").string().c_str());
            std::shared_ptr<AccessManager> accessManager(new MyAccessManager());
            factory->access(accessManager);
            std::shared_ptr<TSSLSocket> ssl_socket = std::shared_ptr<TSSLSocket>(factory->createSocket(host, port));
            ssl_socket->setConnTimeout(1000);
            transport = std::shared_ptr<TTransport>(new TBufferedTransport(ssl_socket));
        }
        catch (TException& ex) {
            transport = nullptr;
            LOG(LOG_LEVEL_ERROR, "%s", ex.what());
        }
    }
    if (!transport) {
        std::shared_ptr<TSocket> socket(new TSocket(host, port));
        socket->setConnTimeout(1000);
        transport = std::shared_ptr<TTransport>(new TBufferedTransport(socket));
    }
    if (transport) {
        std::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
        common_serviceClient client(protocol);
        try {
            transport->open();
            client.get_service_list(service_infos, "");
            client.get_host_detail(MachineInfo, "", machine_detail_filter::type::FULL);
            transport->close();
            return true;
        }
        catch (TException& ex) {
            LOG(LOG_LEVEL_ERROR, "%s", ex.what());
        }
    }
    return false;
}

void parsing_config_file(std::string & mgmt_addr,std::string & username, std::string & passward, bool & _is_https, bool & _is_allow_multiple)
{
    FUNC_TRACER;
    fstream file;
    boost::filesystem::path config_file = boost::filesystem::path(system_tools::get_execution_file_path_linux()).parent_path() / CONFIG_FILE_PATH;

    file.open(config_file.string(),ios::in);
    std::string read_buffer;
    while (std::getline(file, read_buffer,'\n'))
    {
        std::string tag, value;
        size_t delimit_location = read_buffer.find(":");
        tag = string_op::remove_begining_whitespace(string_op::remove_trailing_whitespace(read_buffer.substr(0, delimit_location)));
        value = string_op::remove_begining_whitespace(string_op::remove_trailing_whitespace(read_buffer.substr(delimit_location + 1, read_buffer.size())));
        if (tag == "manager_address")
        {
            mgmt_addr = value;
        }
        else if (tag == "username")
        {
            username = value;
        }
        else if (tag == "password")
        {
            passward = value;
        }
        else if (tag == "is_https")
        {
            if (value == "1")
                _is_https = true;
            else
                _is_https = false;
        }
        else if (tag == "is_allow_multiple")
        {
            if (value == "1")
                _is_allow_multiple = true;
            else
                _is_allow_multiple = false;
        }
    }
    file.close();
}

bool RegisterServices(std::string & mgmt_addr, std::string & username, std::string & password,bool _is_https, bool _is_allow_multiple)
{
    FUNC_TRACER;
    register_info _info;
    bool Ret = false;
    std::set<service_info> service_infos;
    physical_machine_info MachineInfo;
    register_return _return;
    vector<string> strVec;
    //std::string mgmt_addr;
    //_mgmt_addr.GetWindowTextW(mgmt_addr);
    string ip_address;
    if (mgmt_addr.empty()) {
        service_status status;
        status.setAllowMultiple(0);
        status.setMgmtAddr("");
        status.setSessionId("");
        status.write_service_config_file();
        /*try {
            macho::windows::service irm_launcher = macho::windows::service::get_service(L"irm_launcher");
            irm_launcher.stop();
            irm_launcher.start();
        }
        catch (...) {*/
        service_control_helper sc("linux_packer");
        sc.stop();
        sc.start();
        //}
        return true;
    }
    else
    {
        boost::split(strVec, mgmt_addr, is_any_of(":"));
        ip_address = system_tools::analysis_ip_address(strVec[0]);
        if (strVec.size() == 2)
        {
            ip_address += ":" + strVec[1];
        }
    }
    _info.addr = ip_address;
    printf("_info.addr = %s\r\n", _info.addr.c_str());
    try {
        if (GetPhysicalMachineInfo(MachineInfo, service_infos,
            /*is_transport() ? saasame::transport::g_saasame_constants.LAUNCHER_SERVICE_PORT :*/ saasame::transport::g_saasame_constants.PHYSICAL_PACKER_SERVICE_PORT)) {
            _info.machine_id = MachineInfo.machine_id;
            printf("_info.machine_id = %s\r\n", _info.machine_id.c_str());
            std::string uri = saasame::transport::g_saasame_constants.MANAGEMENT_SERVICE_PATH;
            printf("uri = %s\r\n", uri.c_str());
            std::shared_ptr<saasame::transport::management_serviceClient>    client;
            std::shared_ptr<TTransport>                                      transport;
            if (_info.is_https != _is_https) { //reverse?
                std::shared_ptr<TCurlClient>           http = std::shared_ptr<TCurlClient>(new TCurlClient(boost::str(boost::format("https://%s%s") % _info.addr%uri)));
                transport = std::shared_ptr<TTransport>(new TBufferedTransport(http));
                std::shared_ptr<TProtocol>             protocol = std::shared_ptr<TProtocol>(new TBinaryProtocol(transport));
                client = std::shared_ptr<saasame::transport::management_serviceClient>(new saasame::transport::management_serviceClient(protocol));
            }
            else {
                std::shared_ptr<THttpClient>           http = std::shared_ptr<THttpClient>(new THttpClient(_info.addr, 80, uri));
                transport = std::shared_ptr<TTransport>(new TBufferedTransport(http));
                std::shared_ptr<TProtocol>             protocol = std::shared_ptr<TProtocol>(new TBinaryProtocol(transport));
                client = std::shared_ptr<saasame::transport::management_serviceClient>(new saasame::transport::management_serviceClient(protocol));
            }
            try {
                transport->open();
                printf("open finish\r\n");
                std::string session_id;
                register_service_info reg_info;
                reg_info.mgmt_addr = _info.addr;
                reg_info.username = username;
                reg_info.password = password;
               
                
                for(service_info info : service_infos) {
                    if (info.id == saasame::transport::g_saasame_constants.PHYSICAL_PACKER_SERVICE) {
                        reg_info.version = info.version;
                        printf("reg_info.version = %s\r\n", reg_info.version.c_str());
                        reg_info.path = info.path;
                        printf("reg_info.path = %s\r\n", reg_info.path.c_str());
                    }
                    reg_info.service_types.insert(info.id);
                }
                register_physical_packer_info packer_info;
                packer_info.mgmt_addr = _info.addr;
                packer_info.packer_addr = _info.machine_id;
                packer_info.username = username;
                //packer_info.password = password;
                packer_info.path = reg_info.path;
                packer_info.version = reg_info.version;
                printf("session_id = %s\r\n", session_id.c_str());
                printf("_info = %s\r\n", _info.session.c_str());
                printf("packer_info.mgmt_addr = %s\r\n", packer_info.mgmt_addr.c_str());
                printf("packer_info.packer_addr = %s\r\n", packer_info.packer_addr.c_str());
                printf("packer_info.username = %s\r\n", packer_info.username.c_str());
                printf("packer_info.packer_addr = %s\r\n", packer_info.packer_addr.c_str());
                printf("packer_info.password = %s\r\n", packer_info.password.c_str());
                printf("packer_info.version = %s\r\n", packer_info.version.c_str());

                for (auto & mi : MachineInfo.network_infos)
                {
                    for (auto & i : mi.ip_addresses)
                    {
                        printf("ip_addresses = %s\r\n", i.c_str());
                    }
                    for (auto & gt : mi.gateways)
                    {
                        printf("gateways = %s\r\n", gt.c_str());
                    }

                    for (auto & dns : mi.dnss)
                    {
                        printf("dns = %s\r\n", dns.c_str());
                    }
                }
                client->register_physical_packer(_return ,session_id, packer_info, MachineInfo);
                transport->close();
                printf("session = %s\r\n", _return.session.c_str());
                printf("message = %s\r\n", _return.message.c_str());

                if (_return.session.length()) {
                    _info.session = _return.session;
                    _info.username = reg_info.username;
                    _info.password = reg_info.password;
                    Ret = true;
                    service_status status;
                    status.setMgmtAddr(_info.addr);
                    status.setSessionId(_return.session);
                    if (_is_allow_multiple)
                        status.setAllowMultiple(1);
                    else
                        status.setAllowMultiple(0);
                    status.write_service_config_file();                        
                    service_control_helper sc("linux_packer");
                    printf("Registered!");
                    sc.stop();
                    printf("Registered!");
                    sc.start();
                    printf("Registered!");
                }
            }
            catch (saasame::transport::invalid_operation &ex) {
                printf("invalid_operation: %s\r\n", ex.why.c_str());
                if (transport->isOpen())
                    transport->close();
            }
            catch (TException &tx) {
                printf("TExceptioin: %s\r\n",tx.what());
                if (transport->isOpen())
                    transport->close();
            }
            curl_global_cleanup();
            SSL_COMP_free_compression_methods();
        }
    }
    catch (...) {

    }
    return Ret;
}


int main(int argc, char **argv) {

    global_verbose = 0;
    string ma, u, p;
    int cmd_opt = 0;
    bool is_https = false, is_allow_multiple = false;
    while (1) {
        cmd_opt = getopt(argc, argv, "i:s:hmtrv");
        if (cmd_opt == -1)
            break;
        switch (cmd_opt)
        {
        case 'i':
        {
            ma = string(optarg);
            break;
        }
        case 's':
        {
            u = string(optarg);
            break;
        }
        case 'h':
        {
            printf("-i :Mamagement Address\r\n-s :Security Code\r\n-m :allow_multiple\r\n-t :is_https\r\n-h :Help\r\n");
            break;
        }
        case 'm':
        {
            is_allow_multiple = true;
            break;
        }
        case 't':
        {
            is_https = true;
            break;
        }
        case 'r':   //unregister
        {
            service_status::unregister();
            service_control_helper sc("linux_packer");
            sc.stop();
            sleep(5);
            sc.start();
            break;
        }
        case 'v':   //unregister
        {
            service_status::ptr s_s = service_status::create();
            if (s_s)
            {
                printf("you are registered on %s\r\n", s_s->getMgmtAddr().c_str());
            }
            else
            {
                printf("you are not registered.\r\n");
            }
            break;
        }
        default:
        {
            printf("error option");
            break;
        }
        }
    }
    if (ma.size() == 0 || u.size() == 0)
    {
        //printf("option error, read config...\r\n");
        //parsing_config_file(ma, u, p, is_https, is_allow_multiple);
    }
    else
    {
        p = string("admin");
        printf("ma = %s , u = %s, p = %s\r\n", ma.c_str(), u.c_str(), p.c_str());
        RegisterServices(ma, u, p, is_https, is_allow_multiple);
    }

    /*service_status status;
    status.setMgmtAddr("9.9.9.9");
    status.setAllowMultiple("yes");
    status.setSessionId("mumi...");
    status.print();
    status.write_service_config_file();*/
    //RegisterServices(ma, u, p);
	//try {
	//	char buf[1024];
	//	GetExecutionFilePath_Linux(buf);
	//	string crtFileName("server.crt"), keyFileName("server.key"), working_path = system_tools::path_remove_last(string(buf));
	//	
	//	
	//	/*SSL socket here*/
	//	/*stdcxx::shared_ptr<TSSLSocketFactory> factory;
	//	factory = stdcxx::shared_ptr<TSSLSocketFactory>(new TSSLSocketFactory());
	//	factory->authenticate(false);
	//	factory->loadCertificate(strprintf().c_str());
	//	factory->loadPrivateKey((p / "server.key").string().c_str());
	//	factory->loadTrustedCertificates((p / "server.crt").string().c_str());
	//	boost::shared_ptr<AccessManager> accessManager(new MyAccessManager());
	//	factory->access(accessManager);
	//	_ssl_socket = boost::shared_ptr<TSSLSocket>(factory->createSocket(addr, saasame::transport::g_saasame_constants.PHYSICAL_PACKER_SERVICE_PORT));
	//	_ssl_socket->setConnTimeout(1000);
	//	_ssl_socket->setSendTimeout(10 * 60 * 1000);
	//	_ssl_socket->setRecvTimeout(10 * 60 * 1000);*/

	//	/*stdcxx::shared_ptr<TSSLSocketFactory> factory = getTSSLSocketFactoryHelper(false, false, path, crtFileName, keyFileName);
	//	stdcxx::shared_ptr<TSSLSocket> ssl_socket = ssl_helper(factory);*/

	//	stdcxx::shared_ptr<TSSLSocketFactory> factory;
	//	factory = stdcxx::shared_ptr<TSSLSocketFactory>(new TSSLSocketFactory());
	//	//factory->server(false);
	//	factory->authenticate(false);
	//	factory->loadCertificate((working_path + crtFileName).c_str());
	//	factory->loadPrivateKey((working_path + keyFileName).c_str());
	//	factory->loadTrustedCertificates((working_path + crtFileName).c_str());

	//	stdcxx::shared_ptr<TSSLSocket> outSslSocket;
	//	stdcxx::shared_ptr<AccessManager> accessManager(new MyAccessManager());
	//	factory->access(accessManager);
	//	outSslSocket = stdcxx::shared_ptr<TSSLSocket>(factory->createSocket(remote_target, g_saasame_constants.PHYSICAL_PACKER_SERVICE_PORT));
	//	outSslSocket->setConnTimeout(1000);
	//	outSslSocket->setSendTimeout(10 * 60 * 1000);
	//	outSslSocket->setRecvTimeout(10 * 60 * 1000);

	//	/*assert(0);
	//	printf("k\r\n");*/
	//	/*assert(1);
	//	printf("b\r\n");
	//	event_base* base = event_base_new();
	//	stdcxx::shared_ptr<TAsyncChannel> channel(new TEvhttpClientChannel("localhost", "/", "localhost", 9090, base));*/
	//	//stdcxx::shared_ptr<TTransport> socket(new TSocket(remote_target, g_saasame_constants.PHYSICAL_PACKER_SERVICE_PORT));
	//	stdcxx::shared_ptr<TTransport> transport(new TBufferedTransport(outSslSocket/*ssl_socket*//*socket*/));
	//	stdcxx::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
	//	stdcxx::shared_ptr<physical_packer_serviceClient> client(new physical_packer_serviceClient(protocol));
	//	//common_serviceClient client(protocol);
	//	//common_serviceClient CSclient = client;
	//	//function<void(common_servprintf("1\r\n");
	//	transport->open();
	//	//client.resume_job("0","0");
	//	/*ping test*/
	//	stdcxx::shared_ptr<common_serviceClient> common_Client = client;
	//	stdcxx::shared_ptr<service_info> forping(new service_info);
	//	common_Client->ping(*forping);
	//	printf("[ping]\r\n%s\r\n%s\r\n%s\r\n", forping->id.c_str(), forping->version.c_str(), forping->path.c_str());

	//	physical_machine_info machine_info;
	//	common_Client->get_host_detail(machine_info, "", machine_detail_filter::SIMPLE);
	//	/*if (machine_info.machine_id.length()) {
	//		//machine_id = machine_info.machine_id;
	//	}*/
	//	printf("machine_info.disk_infos.size() = %d\r\n", machine_info.disk_infos.size());
 //       printf("0000000sizeof(machine_info.disk_infos) = %d\r\n", machine_info.disk_infos.size());

 //       for (auto &d : machine_info.disk_infos)
 //       {
 //           printf("d.path = %s\r\n", d.path.c_str());
 //           printf("d.uri = %s\r\n", d.uri.c_str());
 //       }

 //       std::set<disk_info> dis1;
 //       common_Client->enumerate_disks(dis1, enumerate_disk_filter_style::type::ALL_DISK);

 //       printf("1111111sizeof(dis) = %d\r\n", dis1.size());
 //       for (auto & d : dis1)
 //       {
 //           printf("d.path = %s\r\n", d.path.c_str());
 //           printf("d.uri = %s\r\n", d.uri.c_str());
 //       }

 //       std::set<disk_info>  dis2;
 //       common_Client->enumerate_disks(dis2, enumerate_disk_filter_style::type::UNINITIALIZED_DISK);
 //       printf("2222222sizeof(dis) = %d\r\n", dis2.size());
 //       for (auto & d : dis2)
 //       {
 //           printf("d.path = %s\r\n", d.path.c_str());
 //           printf("d.uri = %s\r\n", d.uri.c_str());
 //       }



	//	std::set<string> disks;
	//	vector<snapshot> snapshotreturn;
 //       std::map<string,vector<snapshot>> allreturn;;

 //       for (auto & di : machine_info.disk_infos)
 //       {
 //           printf("di.uri = %s\r\n", di.uri.c_str());
 //           disks.insert(di.uri);
 //       }
 //       printf("go~\r\n");
 //       string session = string("5566");
	//	client->take_snapshots(snapshotreturn, session, disks);
 //       sleep(1);
 //       client->get_all_snapshots(allreturn, session);



 //       for (auto & sr : allreturn)
 //       {
 //           printf("mumi\r\n");
 //           printf("sr.first = %s\r\n", sr.first.c_str());

 //           for (auto & ssr : sr.second)
 //           {
 //               printf("get_all_snapshots %s : createion_time = %s\r\n", ssr.snapshot_id.c_str(), ssr.creation_time_stamp.c_str());
 //               
 //               delete_snapshot_result delete_result;
 //               client->delete_snapshot(delete_result, session, ssr.snapshot_id);
 //               if (delete_result.code == 0)
 //               {
 //                   printf("OKGG\r\n");
 //               }
 //               else
 //               {
 //                   printf("NOGG\r\n");
 //               }
 //           }
 //       }
 //       client->take_snapshots(snapshotreturn, session, disks);
 //       sleep(1);
 //       client->get_all_snapshots(allreturn, session);
 //       /*test here*/
 //       packer_job_detail job_detail;
 //       create_packer_job_detail create_job;

 //       /*test here*/
 //       /*printf("create_job_ex start\r\n");
 //       boost::uuids::random_generator gen;
 //       boost::uuids::uuid snuuid = gen();
 //       std::string job_id = boost::uuids::to_string(snuuid);
 //       create_job.__set_type(type);
 //       //std::string job_id = std::string("mumi");
 //       printf("before\r\n");
 //       client->create_job_ex(job_detail, session, job_id, create_job);
 //       printf("after\r\n");*/

 //       /*test here*/
 //       sleep(1);
 //       printf("mumi2\r\n");

 //       for (auto & sr : allreturn)
 //       {
 //           printf("sr.first = %s\r\n", sr.first.c_str());
 //           for (auto & ssr : sr.second)
 //           {
 //               printf("get_all_snapshots disk_path %s : createion_time = %s\r\n", ssr.snapshot_set_id.c_str(), ssr.creation_time_stamp.c_str());
 //               delete_snapshot_result delete_result;
 //               client->delete_snapshot_set(delete_result, session, ssr.snapshot_set_id);
 //               if (delete_result.code == 0)
 //               {
 //                   printf("OK\r\n");
 //               }
 //               else
 //               {
 //                   printf("NO\r\n");
 //               }
 //           }
 //       }
 //       std::string xray;
 //       common_Client->take_xray(xray);
 //       printf("xray,size = %llu \r\n", xray.size());

	//	outSslSocket->close();
	//	if(transport->isOpen())
	//		transport->close();
	//	return 0;
	//	/*get_host_detail test*/
	//	//client.get_host_detail();



	//	/*stdcxx::shared_ptr<TTransport> socket(new TSocket("localhost", 9090));
	//	stdcxx::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
	//	stdcxx::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
	//	physical_packer_serviceClient client(protocol);*/
 // 
 //   //transport->open();

	//	//client.ping();
	//	//cout << "ping()" << endl;
	//	/*event_base_dispatch(base);
	//	event_base_free(base);*/

	//} catch (TException& tx) {
	//	cout << "ERROR: " << tx.what() << endl;
	//}
}
