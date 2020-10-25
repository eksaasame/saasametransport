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
#include <thrift/stdcxx.h>
#include <event.h>
#include <unistd.h>

#include "../gen-cpp/physical_packer_service.h"
#include "../gen-cpp/saasame_constants.h"

/*tools include*/
#include "../tools/string_operation.h"
#include "../tools/sslHelper.h"
#include "../tools/system_tools.h"

using namespace std;
using namespace std::placeholders;
using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
using namespace apache::thrift::async;

using namespace saasame;
using namespace saasame::transport;

/*define*/
#define TEST



#ifdef TEST
#define remote_target "localhost"
#else
#define remote_target "192.168.31.62"
#endif


int main() {
	try {
		char buf[1024];
		GetExecutionFilePath_Linux(buf);
		string crtFileName("server.crt"), keyFileName("server.key"), working_path = system_tools::path_remove_last(string(buf));
		
		
		/*SSL socket here*/
		/*stdcxx::shared_ptr<TSSLSocketFactory> factory;
		factory = stdcxx::shared_ptr<TSSLSocketFactory>(new TSSLSocketFactory());
		factory->authenticate(false);
		factory->loadCertificate(strprintf().c_str());
		factory->loadPrivateKey((p / "server.key").string().c_str());
		factory->loadTrustedCertificates((p / "server.crt").string().c_str());
		boost::shared_ptr<AccessManager> accessManager(new MyAccessManager());
		factory->access(accessManager);
		_ssl_socket = boost::shared_ptr<TSSLSocket>(factory->createSocket(addr, saasame::transport::g_saasame_constants.PHYSICAL_PACKER_SERVICE_PORT));
		_ssl_socket->setConnTimeout(1000);
		_ssl_socket->setSendTimeout(10 * 60 * 1000);
		_ssl_socket->setRecvTimeout(10 * 60 * 1000);*/

		/*stdcxx::shared_ptr<TSSLSocketFactory> factory = getTSSLSocketFactoryHelper(false, false, path, crtFileName, keyFileName);
		stdcxx::shared_ptr<TSSLSocket> ssl_socket = ssl_helper(factory);*/

		stdcxx::shared_ptr<TSSLSocketFactory> factory;
		factory = stdcxx::shared_ptr<TSSLSocketFactory>(new TSSLSocketFactory());
		//factory->server(false);
		factory->authenticate(false);
		factory->loadCertificate((working_path + crtFileName).c_str());
		factory->loadPrivateKey((working_path + keyFileName).c_str());
		factory->loadTrustedCertificates((working_path + crtFileName).c_str());

		stdcxx::shared_ptr<TSSLSocket> outSslSocket;
		stdcxx::shared_ptr<AccessManager> accessManager(new MyAccessManager());
		factory->access(accessManager);
		outSslSocket = stdcxx::shared_ptr<TSSLSocket>(factory->createSocket(remote_target, g_saasame_constants.PHYSICAL_PACKER_SERVICE_PORT));
		outSslSocket->setConnTimeout(1000);
		outSslSocket->setSendTimeout(10 * 60 * 1000);
		outSslSocket->setRecvTimeout(10 * 60 * 1000);

		/*assert(0);
		printf("k\r\n");*/
		/*assert(1);
		printf("b\r\n");
		event_base* base = event_base_new();
		stdcxx::shared_ptr<TAsyncChannel> channel(new TEvhttpClientChannel("localhost", "/", "localhost", 9090, base));*/
		//stdcxx::shared_ptr<TTransport> socket(new TSocket(remote_target, g_saasame_constants.PHYSICAL_PACKER_SERVICE_PORT));
		stdcxx::shared_ptr<TTransport> transport(new TBufferedTransport(outSslSocket/*ssl_socket*//*socket*/));
		stdcxx::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
		stdcxx::shared_ptr<physical_packer_serviceClient> client(new physical_packer_serviceClient(protocol));
		//common_serviceClient client(protocol);
		//common_serviceClient CSclient = client;
		//function<void(common_servprintf("1\r\n");
		transport->open();
		//client.resume_job("0","0");
		/*ping test*/
		stdcxx::shared_ptr<common_serviceClient> common_Client = client;
		stdcxx::shared_ptr<service_info> forping(new service_info);
		common_Client->ping(*forping);
		printf("[ping]\r\n%s\r\n%s\r\n%s\r\n", forping->id.c_str(), forping->version.c_str(), forping->path.c_str());

		physical_machine_info machine_info;
		common_Client->get_host_detail(machine_info, "", machine_detail_filter::SIMPLE);
		/*if (machine_info.machine_id.length()) {
			//machine_id = machine_info.machine_id;
		}*/
		printf("machine_info.disk_infos.size() = %d\r\n", machine_info.disk_infos.size());
        printf("0000000sizeof(machine_info.disk_infos) = %d\r\n", machine_info.disk_infos.size());

        for (auto &d : machine_info.disk_infos)
        {
            printf("d.path = %s\r\n", d.path.c_str());
            printf("d.uri = %s\r\n", d.uri.c_str());
        }

        std::set<disk_info> dis1;
        common_Client->enumerate_disks(dis1, enumerate_disk_filter_style::type::ALL_DISK);

        printf("1111111sizeof(dis) = %d\r\n", dis1.size());
        for (auto & d : dis1)
        {
            printf("d.path = %s\r\n", d.path.c_str());
            printf("d.uri = %s\r\n", d.uri.c_str());
        }

        std::set<disk_info>  dis2;
        common_Client->enumerate_disks(dis2, enumerate_disk_filter_style::type::UNINITIALIZED_DISK);
        printf("2222222sizeof(dis) = %d\r\n", dis2.size());
        for (auto & d : dis2)
        {
            printf("d.path = %s\r\n", d.path.c_str());
            printf("d.uri = %s\r\n", d.uri.c_str());
        }



		std::set<string> disks;
		vector<snapshot> snapshotreturn;
        std::map<string,vector<snapshot>> allreturn;;

        for (auto & di : machine_info.disk_infos)
        {
            printf("di.uri = %s\r\n", di.uri.c_str());
            disks.insert(di.uri);
        }
        printf("go~\r\n");
        string session = string("5566");
		client->take_snapshots(snapshotreturn, session, disks);
        sleep(1);
        client->get_all_snapshots(allreturn, session);



        for (auto & sr : allreturn)
        {
            printf("mumi\r\n");
            printf("sr.first = %s\r\n", sr.first.c_str());

            for (auto & ssr : sr.second)
            {
                printf("get_all_snapshots %s : createion_time = %s\r\n", ssr.snapshot_id.c_str(), ssr.creation_time_stamp.c_str());
                
                delete_snapshot_result delete_result;
                client->delete_snapshot(delete_result, session, ssr.snapshot_id);
                if (delete_result.code == 0)
                {
                    printf("OKGG\r\n");
                }
                else
                {
                    printf("NOGG\r\n");
                }
            }
        }
        client->take_snapshots(snapshotreturn, session, disks);
        sleep(1);
        client->get_all_snapshots(allreturn, session);
        /*test here*/
        packer_job_detail job_detail;
        create_packer_job_detail create_job;

        /*test here*/
        /*printf("create_job_ex start\r\n");
        boost::uuids::random_generator gen;
        boost::uuids::uuid snuuid = gen();
        std::string job_id = boost::uuids::to_string(snuuid);
        create_job.__set_type(type);
        //std::string job_id = std::string("mumi");
        printf("before\r\n");
        client->create_job_ex(job_detail, session, job_id, create_job);
        printf("after\r\n");*/

        /*test here*/
        sleep(1);
        printf("mumi2\r\n");

        for (auto & sr : allreturn)
        {
            printf("sr.first = %s\r\n", sr.first.c_str());
            for (auto & ssr : sr.second)
            {
                printf("get_all_snapshots disk_path %s : createion_time = %s\r\n", ssr.snapshot_set_id.c_str(), ssr.creation_time_stamp.c_str());
                delete_snapshot_result delete_result;
                client->delete_snapshot_set(delete_result, session, ssr.snapshot_set_id);
                if (delete_result.code == 0)
                {
                    printf("OK\r\n");
                }
                else
                {
                    printf("NO\r\n");
                }
            }
        }
        std::string xray;
        common_Client->take_xray(xray);
        printf("xray,size = %llu \r\n", xray.size());

		outSslSocket->close();
		if(transport->isOpen())
			transport->close();
		return 0;
		/*get_host_detail test*/
		//client.get_host_detail();



		/*stdcxx::shared_ptr<TTransport> socket(new TSocket("127.0.0.1", 9090));
		stdcxx::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
		stdcxx::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
		physical_packer_serviceClient client(protocol);*/
  
    //transport->open();

		//client.ping();
		//cout << "ping()" << endl;
		/*event_base_dispatch(base);
		event_base_free(base);*/

	} catch (TException& tx) {
		cout << "ERROR: " << tx.what() << endl;
	}
}
