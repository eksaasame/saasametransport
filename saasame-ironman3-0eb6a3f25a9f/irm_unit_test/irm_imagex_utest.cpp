#include "stdafx.h"
#include "CppUnitTest.h"
#include "assert.h"
#include <macho.h>
#include "..\gen-cpp\irm_imagex_op.h"
#include "..\irm_carrier\carrier_service_handler.h"
#include "..\irm_virtual_packer\virtual_packer_service_handler.h"
#include "..\gen-cpp\management_service.h"
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/THttpClient.h>
#include <boost/crc.hpp>
#include <openssl/md5.h>
#include <openssl/evp.h>
#include <boost/regex.hpp>
#include <boost/crc.hpp>
#include "..\gen-cpp\aws_s3.h"
#include "TCurlClient.h"
#include "scheduler_service.h"

using boost::shared_ptr;
using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace boost;
using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;

using namespace saasame::ironman::imagex;
//using namespace saasame::transport;

namespace irm_unit_test
{		
	TEST_CLASS(UnitTest1)
	{
	public:
		
		TEST_METHOD(irm_imagex_create_small)
		{
			// TODO: Your test code here
            boost::filesystem::path p("D:\\cloned_vmdks\\job\\test.ivd");
            uint64_t disk_size_bytes = 5368709120;
            std::wstring base_name = L"Test_base";

            irm_imagex_op::ptr op(new irm_imagex_local_op((std::wstring)p.parent_path().wstring()));
            irm_transport_image::create(base_name, (std::wstring)p.filename().wstring(), op, disk_size_bytes, irm_transport_image_block::BLOCK_32MB);
        }

        TEST_METHOD(irm_imagex_create_medium)
        {
            // TODO: Your test code here
            boost::filesystem::path p("D:\\cloned_vmdks\\job\\test.ivd");
            uint64_t disk_size_bytes = 42949672960;
            std::wstring base_name = L"Test_base";

            irm_imagex_op::ptr op(new irm_imagex_local_op((std::wstring)p.parent_path().wstring()));
            irm_transport_image::create(base_name, (std::wstring)p.filename().wstring(), op, disk_size_bytes, irm_transport_image_block::BLOCK_32MB, true, true);
        }

        TEST_METHOD(irm_imagex_create_medium_nocompress)
        {
            // TODO: Your test code here
            boost::filesystem::path p("D:\\cloned_vmdks\\job\\test.ivd");
            uint64_t disk_size_bytes = 42949672960;
            std::wstring base_name = L"Test_base";

            irm_imagex_op::ptr op(new irm_imagex_local_op((std::wstring)p.parent_path().wstring()));
            irm_transport_image::create(base_name, (std::wstring)p.filename().wstring(), op, disk_size_bytes, irm_transport_image_block::BLOCK_32MB, false, true);
        }

        TEST_METHOD(irm_imagex_open)
        {
            // TODO: Your test code here
            boost::filesystem::path p("D:\\cloned_vmdks\\job\\test.ivd");
            std::wstring base_name = L"Test_base";

            irm_imagex_op::ptr op(new irm_imagex_local_op((std::wstring)p.parent_path().wstring()));
            universal_disk_rw::ptr disk_rw = irm_transport_image::open(base_name, (std::wstring)p.filename().wstring(), op);

            if (disk_rw)
            {
                std::wstring path = disk_rw->path();
            }
        }

        TEST_METHOD(irm_imagex_write_uc1)
        {
            // TODO: Your test code here
            boost::filesystem::path p("D:\\cloned_vmdks\\job\\test.ivd");
            std::wstring base_name = L"Test_base";

            irm_imagex_op::ptr local_op(new irm_imagex_local_op((std::wstring)p.parent_path().wstring()));
            universal_disk_rw::ptr disk_rw = irm_transport_image::open(base_name, (std::wstring)p.filename().wstring(), local_op);

            if (disk_rw)
            {
                boost::shared_ptr<uint8_t> buf = NULL;
                uint32_t number_of_bytes_written = 0;

                std::wstring path = disk_rw->path();

                uint32_t len = 45056;
                buf = boost::shared_ptr<uint8_t>(new uint8_t[len]);
                memset(buf.get(), 'A', len);
                disk_rw->write(0, buf.get(), len, number_of_bytes_written);

                len = 45056;
                buf = boost::shared_ptr<uint8_t>(new uint8_t[len]);
                memset(buf.get(), 'C', len);
                disk_rw->write(0, buf.get(), len, number_of_bytes_written);

                len = 90849280;
                buf = boost::shared_ptr<uint8_t>(new uint8_t[len]);
                memset(buf.get(), 'B', len);
                disk_rw->write(6907904, buf.get(), len, number_of_bytes_written);

                irm_transport_image *image = dynamic_cast<irm_transport_image *>(disk_rw.get());
                image->close();
            }
        }

        TEST_METHOD(irm_imagex_write_uc2)
        {
            // TODO: Your test code here
            boost::filesystem::path p("D:\\cloned_vmdks\\job\\test.ivd");
            std::wstring base_name = L"Test_base";

            irm_imagex_op::ptr local_op(new irm_imagex_local_op((std::wstring)p.parent_path().wstring()));
            universal_disk_rw::ptr disk_rw = irm_transport_image::open(base_name, (std::wstring)p.filename().wstring(), local_op);

            if (disk_rw)
            {
                boost::shared_ptr<uint8_t> buf = NULL;
                uint32_t number_of_bytes_written = 0;

                std::wstring path = disk_rw->path();

                uint32_t len = 45056;
                buf = boost::shared_ptr<uint8_t>(new uint8_t[len]);
                memset(buf.get(), '8', len);
                disk_rw->write(512, buf.get(), len, number_of_bytes_written);
            }
        }

        TEST_METHOD(irm_imagex_write_uc3)
        {
            // TODO: Your test code here
            boost::filesystem::path p("D:\\cloned_vmdks\\job\\test.ivd");
            std::wstring base_name = L"Test_base";

            irm_imagex_op::ptr local_op(new irm_imagex_local_op((std::wstring)p.parent_path().wstring()));
            universal_disk_rw::ptr disk_rw = irm_transport_image::open(base_name, (std::wstring)p.filename().wstring(), local_op);

            if (disk_rw)
            {
                boost::shared_ptr<uint8_t> buf = NULL;
                uint32_t number_of_bytes_written = 0;

                std::wstring path = disk_rw->path();

                uint32_t len = 2097152;
                buf = boost::shared_ptr<uint8_t>(new uint8_t[len]);
                memset(buf.get(), '8', len);
                disk_rw->write(32481280, buf.get(), len, number_of_bytes_written);
                
                irm_transport_image *image = dynamic_cast<irm_transport_image *>(disk_rw.get());
                image->close();

                //disk_rw = NULL;
            }
        }

        TEST_METHOD(irm_imagex_write_uc4)
        {
            // TODO: Your test code here
            boost::filesystem::path p("D:\\cloned_vmdks\\job\\test.ivd");
            std::wstring base_name = L"Test_base";

            irm_imagex_op::ptr local_op(new irm_imagex_local_op((std::wstring)p.parent_path().wstring()));
            universal_disk_rw::ptr disk_rw = irm_transport_image::open(base_name, (std::wstring)p.filename().wstring(), local_op);

            if (disk_rw)
            {
                boost::shared_ptr<uint8_t> buf = NULL;
                uint32_t number_of_bytes_written = 0;

                std::wstring path = disk_rw->path();

                uint32_t len = 1024;
                buf = boost::shared_ptr<uint8_t>(new uint8_t[len]);
                memset(buf.get(), '2', len);
                disk_rw->write(0, buf.get(), len, number_of_bytes_written);
                
                len = 1024;
                buf = boost::shared_ptr<uint8_t>(new uint8_t[len]);
                memset(buf.get(), '4', len);
                disk_rw->write(1024, buf.get(), len, number_of_bytes_written);

                len = 1024;
                buf = boost::shared_ptr<uint8_t>(new uint8_t[len]);
                memset(buf.get(), '6', len);
                disk_rw->write(2048, buf.get(), len, number_of_bytes_written);

                len = 2048;
                buf = boost::shared_ptr<uint8_t>(new uint8_t[len]);
                memset(buf.get(), '6', len);
                disk_rw->write(33553408, buf.get(), len, number_of_bytes_written);

                len = 167936;
                buf = boost::shared_ptr<uint8_t>(new uint8_t[len]);
                memset(buf.get(), 'A', len);
                disk_rw->write(1734447104, buf.get(), len, number_of_bytes_written);

                len = 167936;
                buf = boost::shared_ptr<uint8_t>(new uint8_t[len]);
                memset(buf.get(), 'A', len);
                disk_rw->write(1734447104, buf.get(), len, number_of_bytes_written);

                len = 20480;
                buf = boost::shared_ptr<uint8_t>(new uint8_t[len]);
                memset(buf.get(), 'B', len);
                disk_rw->write(1734635520, buf.get(), 0, number_of_bytes_written);

                len = 18087936;
                buf = boost::shared_ptr<uint8_t>(new uint8_t[len]);
                memset(buf.get(), 'C', len);
                disk_rw->write(2102722560, buf.get(), len, number_of_bytes_written);

                len = 1048576;
                buf = boost::shared_ptr<uint8_t>(new uint8_t[len]);
                memset(buf.get(), 'C', len);
                disk_rw->write(42948624384, buf.get(), len, number_of_bytes_written);

                irm_transport_image *image = dynamic_cast<irm_transport_image *>(disk_rw.get());
                image->close();
            }
        }

        TEST_METHOD(irm_carrier_service_list_uc1)
        {
            std::set<service_info> _return;
            boost::shared_ptr<TSocket> socket(new TSocket("localhost", saasame::transport::g_saasame_constants.CARRIER_SERVICE_PORT));
            boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
            boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
            carrier_serviceClient client(protocol);
            
            try 
            {
                transport->open();
                client.get_service_list(_return, "");
                transport->close();

                std::string msg;
                foreach(service_info service, _return)
                {
                    msg = "Service: " + service.id;
                    Logger::WriteMessage(msg.c_str());
                    Logger::WriteMessage("\n");
                }
            }
            catch (TException& tx) {
            }
        }

        TEST_METHOD(irm_loader_enumerate_disks_uc1)
        {
            std::set<disk_info> _return;
            
            boost::shared_ptr<TSocket> socket(new TSocket("192.168.31.82", saasame::transport::g_saasame_constants.LOADER_SERVICE_PORT));
            boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
            boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
            common_serviceClient client(protocol);

            try
            {
                transport->open();
                client.enumerate_disks(_return, enumerate_disk_filter_style::UNINITIALIZED_DISK);
                transport->close();
            }
            catch (TException& tx) {
            }
        }

        TEST_METHOD(irm_loader_enumerate_disks_uc2)
        {
            std::set<disk_info> _return;

            boost::shared_ptr<TSocket> socket(new TSocket("192.168.31.82", saasame::transport::g_saasame_constants.LOADER_SERVICE_PORT));
            boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
            boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
            common_serviceClient client(protocol);

            try
            {
                transport->open();
                client.enumerate_disks(_return, enumerate_disk_filter_style::ALL_DISK);
                transport->close();
            }
            catch (TException& tx) {
            }
        }

        TEST_METHOD(irm_carrier_get_host_detail_uc1)
        {
            physical_machine_info _return;
            boost::shared_ptr<TSocket> socket(new TSocket("localhost", saasame::transport::g_saasame_constants.CARRIER_SERVICE_PORT));
            boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
            boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
            carrier_serviceClient client(protocol);

            try
            {
                transport->open();
                client.get_host_detail(_return, "", machine_detail_filter::FULL);
                transport->close();

                Logger::WriteMessage(_return.client_name.c_str());
            }
            catch (TException& tx) {
            }
        }

        TEST_METHOD(irm_carrier_add_connection_local_uc1)
        {
            connection conn;
            boost::shared_ptr<TSocket> socket(new TSocket("192.168.0.100", saasame::transport::g_saasame_constants.CARRIER_SERVICE_PORT));
            boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
            boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
            carrier_serviceClient client(protocol);

            try
            {
                conn.id = "bdf93479-0c2d-4fee-8c86-635d91bd1c38";
                conn.type = saasame::transport::connection_type::LOCAL_FOLDER;
                conn.options["folder"] = "E:\\imagex";
                conn.__set_options(conn.options);

                transport->open();
                client.add_connection(conn.id, conn);
                transport->close();
            }
            catch (TException& tx) {
            }
        }

        TEST_METHOD(macho_file_lock_uc1)
        {
            macho::windows::file_lock f_lock(macho::windows::environment::get_working_directory() +  L"\\aaaaaa.ivd");
            macho::windows::auto_lock lock(f_lock);

            Sleep(2000);
        }

        TEST_METHOD(irm_host_packer_get_host_detail_uc1)
        {
            physical_machine_info _return;
            boost::shared_ptr<TSocket> socket(new TSocket("192.168.31.112", saasame::transport::g_saasame_constants.SCHEDULER_SERVICE_PORT));
            boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
            boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
            scheduler_serviceClient client(protocol);

            try
            {
                transport->open();
                client.get_host_detail(_return, "", machine_detail_filter::FULL);
                bool r = client.verify_management("192.168.31.125", 80, false);
                transport->close();

                Logger::WriteMessage(_return.client_name.c_str());
            }
            catch (TException& tx) { 
                Logger::WriteMessage(tx.what());
            }
        }

        TEST_METHOD(irm_mgmt_get_replica_create_detail_uc1)
        {
            saasame::transport::replica_job_create_detail detail;
            bool result = true;
            boost::shared_ptr<THttpClient>           http = boost::shared_ptr<THttpClient>(new THttpClient("192.168.0.103", 80, "/mgmt/"));
            boost::shared_ptr<TTransport>            transport = boost::shared_ptr<TTransport>(new TBufferedTransport(http));
            boost::shared_ptr<TProtocol>             protocol = boost::shared_ptr<TProtocol>(new TBinaryProtocol(transport));
            boost::shared_ptr<saasame::transport::management_serviceClient>    client = boost::shared_ptr<saasame::transport::management_serviceClient>(new saasame::transport::management_serviceClient(protocol));
            try{
                transport->open();
                client->get_replica_job_create_detail(detail, "", "1234");
                result = true;
            }
            catch (TException & tx){
            }
            transport->close();
        }

        TEST_METHOD(irm_mgmt_check_snapshot_uc1)
        {
            saasame::transport::replica_job_create_detail detail;
            bool result = true;
            //boost::shared_ptr<THttpClient>           http = boost::shared_ptr<THttpClient>(new THttpClient("192.168.0.103", 80, "/mgmt/"));
            std::string mgmt_addr = "192.168.31.125";
            int port = 443;
            std::string uri = saasame::transport::g_saasame_constants.MANAGEMENT_SERVICE_PATH;
            boost::shared_ptr<TCurlClient>           http = boost::shared_ptr<TCurlClient>(new TCurlClient(boost::str(boost::format("https://%s%s") % mgmt_addr%uri), port));
            boost::shared_ptr<TTransport>            transport = boost::shared_ptr<TTransport>(new TBufferedTransport(http));
            boost::shared_ptr<TProtocol>             protocol = boost::shared_ptr<TProtocol>(new TBinaryProtocol(transport));
            boost::shared_ptr<saasame::transport::management_serviceClient>    client = boost::shared_ptr<saasame::transport::management_serviceClient>(new saasame::transport::management_serviceClient(protocol));
            try{
                transport->open();
                //client->check_snapshots("", macho::guid_(GUID_NULL));
                client->get_replica_job_create_detail(detail, "", "327d2b84-0552-451d-aaa9-881320954392");

                result = true;
            }
            catch (TException & tx){
                Logger::WriteMessage(tx.what());
            }
            catch (...){
                Logger::WriteMessage("Unknown exception.");
            }
            transport->close();
        }

        TEST_METHOD(irm_imagex_block_checksum_verify_uc1)
        {
            // TODO: Your test code here
            std::wstring parent = L"D:\\imagex";
            std::wstring base_name = L"Test_base";
            boost::filesystem::path p("6000C295-fca2-274a-248c-01a6b322b81b_full.ivd");

            irm_imagex_op::ptr local_op(new irm_imagex_local_op(parent));
            universal_disk_rw::ptr disk_rw = irm_transport_image::open(base_name, (std::wstring)p.filename().wstring(), local_op);
            irm_transport_image *image = dynamic_cast<irm_transport_image *>(disk_rw.get());

            if (image)
            {
                std::ifstream ifs;
                uint32_t block_size = 33554432;
                std::vector<std::string> files;

                files.push_back("D:\\imagex\\564d89ca-a86a-166e-c80f-922be03fd0fc_6000C295-fca2-274a-248c-01a6b322b81b_full-00002.ivd");
                files.push_back("D:\\imagex\\564d89ca-a86a-166e-c80f-922be03fd0fc_6000C295-fca2-274a-248c-01a6b322b81b_full-00000.ivd");

                foreach(std::string file, files)
                {
                    std::stringstream buf_crc(std::ios_base::binary | std::ios_base::out | std::ios_base::in);
                    std::stringstream buf_md5(std::ios_base::binary | std::ios_base::out | std::ios_base::in);

                    ifs.open(boost::filesystem::path(file).string(), std::ios::in | std::ios::binary, _SH_DENYWR);

                    if (ifs.is_open())
                    {
                        saasame::ironman::imagex::irm_transport_image_block::ptr p = saasame::ironman::imagex::irm_transport_image_block::ptr(new saasame::ironman::imagex::irm_transport_image_block(0, block_size));
                        boost::archive::binary_iarchive ar(ifs, boost::archive::no_header | boost::archive::no_codecvt | boost::archive::no_xml_tag_checking | boost::archive::no_tracking);
                        ar & (*p.get());
                        ifs.close();

                        if (p->compressed)
                        {
                            uint32_t decompressed_length = block_size;
                            boost::shared_ptr<uint8_t> decompressed_buf(new uint8_t[block_size]);
                            decompressed_length = LZ4_decompress_safe((const char *)&p->data[0], (char *)decompressed_buf.get(), p->data.size(), block_size);
                            p->data.resize(decompressed_length);
                            memcpy(&p->data[0], decompressed_buf.get(), decompressed_length);
                            p->length = decompressed_length;
                            decompressed_buf = NULL;
                            p->compressed = false;
                        }

                        boost::crc_32_type crc_processor;
                        uint32_t crc = p->crc;
                        uint8_t  md5[16] = { 0 };
                        memcpy(md5, p->md5, sizeof(md5));

                        //reset crc&md5 before calculate crc&md5
                        p->crc = 0;
                        memset(p->md5, 0, sizeof(p->md5));

                        //crc32
                        boost::archive::binary_oarchive oa_crc(buf_crc, boost::archive::no_header | boost::archive::no_codecvt | boost::archive::no_xml_tag_checking | boost::archive::no_tracking);
                        oa_crc & *p.get();

                        uint32_t new_crc = 0;
                        saasame::ironman::imagex::irm_transport_image_block::calc_crc32_val(buf_crc, new_crc);
                        p->crc = new_crc;

                        //md5
                        boost::archive::binary_oarchive oa_md5(buf_md5, boost::archive::no_header | boost::archive::no_codecvt | boost::archive::no_xml_tag_checking | boost::archive::no_tracking);
                        oa_md5 & *p.get();

                        std::string digest;
                        saasame::ironman::imagex::irm_transport_image_block::calc_md5_val(buf_md5, digest);
                        if (!digest.empty())
                            memcpy(p->md5, digest.c_str(), sizeof(p->md5));

                        buf_crc.clear();
                        buf_md5.clear();
                        p = NULL;
                    }
                }
            }
        }

        TEST_METHOD(irm_imagex_chunk_calc_verify)
        {
            struct _data_chunks
            {
                uint64_t start;
                uint64_t length;
            } data_chunks;

            data_chunks.start = 168820736;
            data_chunks.length = 662503424;

            uint64_t capacity = 0;
            uint64_t start_sector = 0;
            uint64_t vhdx_start_byte = 0;
            uint32_t num_sectors_to_read = 0;
            uint32_t num_sectors_read = 0;
            uint32_t num_bytes_written = 0;

            std::vector<struct _data_chunks> chunks;
            chunks.push_back(data_chunks);

            foreach(_data_chunks &chunk, chunks)
            {
                if(start_sector > chunk.start + chunk.length) //byte
                    continue;

                capacity = chunk.length / 512;
                if (capacity == 0)
                    continue;

                if (start_sector < chunk.start)
                    start_sector = chunk.start / 512;
                else
                {
                    start_sector /= 512;
                    capacity -= (start_sector - (chunk.start / 512));
                }

                if (capacity == 0)
                    continue;

                vhdx_start_byte = start_sector * 512;
                num_sectors_to_read = num_sectors_read = num_bytes_written = 0;

                do
                {
                    //if (capacity >= DATA_BUFFER_SIZE)
                    if (capacity >= irm_transport_image_block::BLOCK_32MB / 512)
                    {
                        num_sectors_to_read = irm_transport_image_block::BLOCK_32MB / 512;
                    }
                    else
                        num_sectors_to_read = capacity;

                    num_sectors_read = num_sectors_to_read;
                    num_bytes_written = num_sectors_read * 512;

                    start_sector += num_sectors_read;
                    vhdx_start_byte += num_bytes_written;
                    capacity -= num_sectors_read;
                } while (capacity > 0 );
            }
        }

        TEST_METHOD(loop_through_in_folder)
        {
            boost::filesystem::directory_iterator end_itr;
            boost::filesystem::path target_path = "d:\\imagex";
            boost::regex file_filter(std::string("564d89ca-a86a-166e-c80f-922be03fd0fc_6000C295-FCA2-274A-248C-01A6B322B81B_full_1454494169") + "\..*");

            for (boost::filesystem::directory_iterator i(target_path); i != end_itr; ++i)
            {
                // Skip if not a file
                if (!boost::filesystem::is_regular_file(i->status()))
                    continue;

                // Skip if no match
                if (!boost::regex_match(i->path().filename().string(), file_filter))
                    continue;

                // File matches, store it
                Logger::WriteMessage((i->path().filename().string() + "\n").c_str());
            }
        }

        TEST_METHOD(serialize_load_data_from_file)
        {
            std::ifstream ifs;

            ifs.exceptions(std::ifstream::failbit | std::ifstream::badbit);

            try
            {
                ifs.open("d:\\imagex\\564d878b-2d79-e518-359f-cb652c8bf104_6000C299-33A0-97F8-65DD-C0D10B76C55E_full_1456475200-00000.ivd", std::ios::in | std::ios::binary, _SH_DENYWR);
                if (ifs.is_open())
                {
                    irm_transport_image_block::ptr p = irm_transport_image_block::ptr(new irm_transport_image_block(0, irm_transport_image_block::BLOCK_32MB));
                    std::stringstream buffer(std::ios_base::binary | std::ios_base::out | std::ios_base::in);
                    buffer << ifs.rdbuf();
                    p->load(buffer);
                    ifs.close();
                    p = NULL;
                }
            }
            catch (...){}
        }

        TEST_METHOD(disk_full_error_handle)
        {
            std::ofstream ofs;
            char *buf = new char[irm_transport_image_block::BLOCK_64MB];

            memset(buf, '1', irm_transport_image_block::BLOCK_64MB);
            ofs.exceptions(std::ofstream::failbit | std::ofstream::badbit | std::ofstream::eofbit);

            try
            {
                for (int i = 1; i <= 1000; i++)
                {
                    std::string root = "g:\\";

                    ofs.open(root.append(std::to_string(i)).append(".bin"), std::ios::out | std::ios::binary, _SH_DENYWR);
                    if (ofs.is_open())
                    {
                        ofs << buf;
                        ofs.close();
                    }
                }
            }
            catch (std::exception& ex)
            {
                Logger::WriteMessage(ex.what());
            }
            catch (...)
            {
                Logger::WriteMessage("Unknown exception.");
            }
        }

        TEST_METHOD(irm_virtual_packer_get_virtual_host)
        {
            virtual_host _return;
            boost::shared_ptr<TSocket> socket(new TSocket("localhost", saasame::transport::g_saasame_constants.VIRTUAL_PACKER_SERVICE_PORT));
            boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
            boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
            virtual_packer_serviceClient client(protocol);

            try
            {
                transport->open();
                client.get_virtual_host_info(_return, "session_id", "192.168.31.199", "root", "abc@123");
                transport->close();
            }
            catch (TException& tx) {
            }
        }
	};
}