#include <thrift/concurrency/ThreadManager.h>
#include <thrift/concurrency/PlatformThreadFactory.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/server/TThreadPoolServer.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TSSLSocket.h>
#include <thrift/transport/TTransportUtils.h>
#include <thrift/TToString.h>
#include <thrift/stdcxx.h>
#include <mcheck.h>

#include "physical_packer_service_handler.h"
#include "../tools/archive.h"
//#include "physical_packer_job.h"


#include <vector>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <sstream>
#include <signal.h>
#include <uuid/uuid.h>

#include "../gen-cpp/physical_packer_service.h"

#include <boost/format.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/random_generator.hpp>
//#include <boost/exception/to_string.hpp>
#include "../tools/log.h"
#include "../tools/string_operation.h"
#include "../tools/sslHelper.h"
#include "../tools/clone_disk.h"
#include "../tools/service_status.h"
#include "../tools/thrift_helper.h"
/*linux system include*/
#include <stdlib.h> 


/*for test*/
#include <time.h>

#define JOB 0
//#define carrier_address "192.168.31.153"


using namespace std;
using namespace apache::thrift;
using namespace apache::thrift::concurrency;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
using namespace apache::thrift::server;

using namespace saasame;
using namespace saasame::transport;

using namespace boost::algorithm;

#define INITCTL_RELOAD "initctl reload-configuration"
#define SYSTEMCTL_RELOAD "systemctl daemon-reload"
#define BASH_HEAD "#!/bin/bash"


/*tool function*/
/*void getExecutionFilePath_Linux(char *buf)
{
	readlink("/proc/self/exe", buf, sizeof(buf));
}*/
/*get the config info to the _hc structure*/
/*read and write file*/

/*class test_parent
{
public:
    virtual void VF() = 0;
    virtual ~test_parent() {}
};

class test_child : virtual public test_parent
{
public:
    test_child() {}
    void VF();
};*/


void print_all__create_job_detail(const _create_packer_job_detail & _create_job)
{
    LOG_TRACE("***print_all__create_job_detail***\r\n");
    for (auto & d : _create_job.p.disks)
    {
        LOG_TRACE("_create_job.disks.d = %s\r\n", d.c_str());
    }
    for (auto & sn : _create_job.p.snapshots)
    {
        LOG_TRACE("_create_job.snapshots.sn.snapshot_set_id = %s\r\n", sn.snapshot_set_id.c_str());
        LOG_TRACE("_create_job.snapshots.sn.snapshot_id = %s\r\n", sn.snapshot_id.c_str());
        LOG_TRACE("_create_job.snapshots.sn.original_volume_name = %s\r\n", sn.original_volume_name.c_str());
        LOG_TRACE("_create_job.snapshots.sn.snapshot_device_object = %s\r\n", sn.snapshot_device_object.c_str());
        LOG_TRACE("_create_job.snapshots.sn.creation_time_stamp = %s\r\n", sn.creation_time_stamp.c_str());
        LOG_TRACE("_create_job.snapshots.sn.snapshots_count = %d\r\n", sn.snapshots_count);
    }
    for (auto & pj : _create_job.p.previous_journals)
    {
        LOG_TRACE("_create_job.previous_journals.pj.first = %llu\r\n", pj.first);
        LOG_TRACE("_create_job.previous_journals.pj.second.id = %llu\r\n", pj.second.id);
        LOG_TRACE("_create_job.previous_journals.pj.second.first_key = %llu\r\n", pj.second.first_key);
        LOG_TRACE("_create_job.previous_journals.pj.second.latest_key = %llu\r\n", pj.second.latest_key);
        LOG_TRACE("_create_job.previous_journals.pj.second.lowest_valid_key = %llu\r\n", pj.second.lowest_valid_key);
    }
    for (auto & im : _create_job.p.images)
    {
        LOG_TRACE("_create_job.images.im.first = %s\r\n", im.first.c_str());
        LOG_TRACE("_create_job.images.im.second.name = %s\r\n", im.second.name.c_str());
        LOG_TRACE("_create_job.images.im.second.parent = %s\r\n", im.second.parent.c_str());
        LOG_TRACE("_create_job.images.im.second.base = %s\r\n", im.second.base.c_str());
    }
    for (auto & bs : _create_job.p.backup_size)
    {
        LOG_TRACE("_create_job.backup_size.bs.first = %s\r\n", bs.first.c_str());
        LOG_TRACE("_create_job.backup_size.bs.second = %s\r\n", bs.second);
    }
    for (auto & bp : _create_job.p.backup_progress)
    {
        LOG_TRACE("_create_job.backup_progress.bp.first = %s\r\n", bp.first.c_str());
        LOG_TRACE("_create_job.backup_progress.bp.second = %s\r\n", bp.second);
    }
    for (auto & bi : _create_job.p.backup_image_offset)
    {
        LOG_TRACE("_create_job.backup_image_offset.bi.first = %s\r\n", bi.first.c_str());
        LOG_TRACE("_create_job.backup_image_offset.bi.second = %s\r\n", bi.second);
    }
}
void print_all_create_job_detail(const create_packer_job_detail& create_job)
{
    LOG_TRACE("***print_all_create_job_detail***\r\n");
    for (auto & id : create_job.connection_ids)
    {
        LOG_TRACE("create_job.connection_ids.id = %s\r\n", id.c_str());
    }
    for (auto & cr : create_job.carriers)
    {
        LOG_TRACE("create_job.carriers.cr.first = %s\r\n", cr.first.c_str());
        for (auto & crs : cr.second)
        {
            LOG_TRACE("create_job.carriers.cr.second.crs = %s\r\n", crs.c_str());
        }
    }
    print_all__create_job_detail(create_job.detail);
}


 void test_copy_snap_shot(snapshot_instance::ptr & si)
 {
     int ret = 0;
     std::string file_path;
     si->copy(true); // full image
     si->transition_incremental(); // trance to inc
     printf("si.get_abs_cow_path().c_str() = %s\r\n", si->get_abs_cow_path().c_str());
     /*FILE *cow = fopen(si.get_abs_cow_path().c_str(), "r");
     si.verify_files(cow);
     fclose(cow);*/
     file_path = si->get_mounted_point()->get_mounted_on() + "/muni"; //create a file name mumi
     FILE * fp = fopen(file_path.c_str(), "w");
     fprintf(fp, "mini\r\n");
     fclose(fp);
     sleep(1);
     ret = si->transition_snapshot(); // trans to snap
     if (ret != 0)
         printf("transition_snapshot error\r\n");
     si->copy(false);  // inc image, update
     remove(file_path.c_str()); // remove the old file 
     file_path = si->get_mounted_point()->get_mounted_on() + "/muni2"; //creat a mew file mumi2
     fp = fopen(file_path.c_str(), "w");
     fprintf(fp, "muni2\r\n");
     fclose(fp);
     /*so the test way is that use the update image , and the final result "mumi" would remain at last*/
 }

 void test_copy_snap_shot_by_change_area(snapshot_instance::ptr & si)
 {
     int ret = 0;
     std::string file_path;
     si->copy_by_changed_area(true); // full image
     si->transition_incremental(); // trance to inc
     printf("si.get_abs_cow_path().c_str() = %s\r\n", si->get_abs_cow_path().c_str());
     /*FILE *cow = fopen(si.get_abs_cow_path().c_str(), "r");
     si.verify_files(cow);
     fclose(cow);*/
     file_path = si->get_mounted_point()->get_mounted_on() + "/muni"; //create a file name mumi
     FILE * fp = fopen(file_path.c_str(), "w");
     fprintf(fp, "mini\r\n");
     fclose(fp);
     sleep(1);
     ret = si->transition_snapshot(); // trans to snap
     if (ret != 0)
         printf("transition_snapshot error\r\n");
     si->copy_by_changed_area(false);  // inc image, update
     remove(file_path.c_str()); // remove the old file 
     file_path = si->get_mounted_point()->get_mounted_on() + "/muni2"; //creat a mew file mumi2
     fp = fopen(file_path.c_str(), "w");
     fprintf(fp, "muni2\r\n");
     fclose(fp);
     /*so the test way is that use the update image , and the final result "mumi" would remain at last*/
 }

void physical_packer_service_handler::ping(service_info& _return) 
{
	//FUNC_TRACER;
	_return.__set_id(g_saasame_constants.PHYSICAL_PACKER_SERVICE);
	_return.__set_version(boost::str(boost::format("%d.%d.%d.0") % PRODUCT_MAJOR %PRODUCT_MINOR %PRODUCT_BUILD));
	char buf[1024];
	GetExecutionFilePath_Linux(buf);
    system_tools::path_remove_last(string(buf));
	_return.__set_path(buf);
}

void physical_packer_service_handler::_fill_snapshot_info(snapshot &sn, snapshot_instance::ptr &si)
{
    sn.__set_snapshot_set_id(si->get_set_uuid());
    sn.__set_snapshot_id(string(si->get_uuid()));
    LOG_TRACE("sn.snapshot_set_id = %s\r\n", sn.snapshot_set_id.c_str());
    LOG_TRACE("sn.snapshot_id = %s\r\n", sn.snapshot_id.c_str());
    sn.__set_creation_time_stamp(boost::posix_time::to_simple_string(si->get_creation_time()));
}
/*thrift api start*/
void physical_packer_service_handler::take_snapshots(std::vector<snapshot> & _return, const std::string& session_id, const std::set<std::string> & disks) 
{
	FUNC_TRACER;
    unsigned char result = 0;
	/*first find the disk's partition*/
	//1. got the disk info saved in dcs
    if (dcs.size())
        dcs.clear();
    dcs = disk::get_disk_info();
	//2. find the match (mounted) partition's
    for (auto & d : disks)
    {
        LOG_TRACE("%s\r\n", d.c_str());
    }
    storage::ptr str_new = storage::get_storage();
    disk::vtr old_disks = str->get_all_disk();
    disk::vtr new_disks = str_new->get_all_disk();
    set<string> error_disk_uris;
    modify_disks.clear();
    for (auto & dskN : new_disks)
    {
        bool dsk_same = false;
        for (auto & dskO : old_disks)
        {
            if (disk::compare(dskN, dskO))
            {
                dsk_same = true;
                break;
            }
        }
        if (!dsk_same)
            modify_disks[dskN->ab_path] = dskN;
    }

    for (auto & dskO : old_disks)
    {
        bool dsk_same = false;
        for (auto & dskN : new_disks)
        {
            if (disk::compare(dskO, dskN))
            {
                dsk_same = true;
                break;
            }
        }
        if (!dsk_same && !modify_disks.count(dskO->ab_path))
            modify_disks[dskO->ab_path] = dskO;
    }

    //rescan the disk and clear the unmount snapshot and mount new partition or snapshots
    sh->clear_umount_snapshots();
    /*flush the datto*/
    system_tools::execute_command("sync; echo 3 > /proc/sys/vm/drop_caches");
    /*this is for the modify disk structure between snapshot.*/
    sh->take_snapshot_by_disk_uri(disks, &result);
    if (result)
        return;
    /*this is for the modify disk structure between snapshot.*/
    int dr = 1; //datto result

    /**/

    snapshot_instance::vtr snapshots =  sh->change_snapshot_mode_by_uri(disks,false, dr, error_disk_uris);
    for (auto & uri : error_disk_uris)
    {
        string disk_ab_path = str_new->get_disk_ab_path_from_uri(uri);
        if (modify_disks.count(disk_ab_path) == 0)
        {
            linux_storage::disk_baseA::ptr dbA = str_new->get_instance_from_ab_path(disk_ab_path);
            modify_disks[disk_ab_path] = boost::static_pointer_cast<linux_storage::disk>(dbA);
        }
    }
    btrfs_snapshot_instance::vtr bsic = sh->take_btrfs_snapshot_by_disk_uri(disks);
    if (!dr || !bsic.empty())
    {
        //clear all snapshot in snset
        snset.clear();

        snset.uuid = system_tools::gen_random_uuid();
        snset.disks = disks;
        for (snapshot_instance::ptr & si : snapshots)
        {
            si->set_set_uuid(snset.uuid);
            LOG_TRACE("set_uuid = %s\r\n", si->get_set_uuid().c_str());
            snset.sets.push_back(si);
            snapshot sn;
            _fill_snapshot_info(sn, si);
            _return.push_back(sn);
        }
        for (auto & sp : sh->snapshot_map)
        {
            auto &si = sp.second;
            LOG_TRACE("si->get_uuid() = %s\r\n", si->get_uuid().c_str());
            LOG_TRACE("si->get_set_uuid() = %s\r\n", si->get_set_uuid().c_str());
        }
        for (btrfs_snapshot_instance::ptr & psi : bsic)
        {
            LOG_TRACE("psi->get_uuid() = %s\r\n", psi->get_uuid().c_str());
            psi->set_set_uuid(snset.uuid);
            snset.btrfs_sets.push_back(psi);
        }
        for (auto & sp : sh->btrfs_snapshot_vtr)
        {
            LOG_TRACE("sp->get_uuid() = %s\r\n", sp->get_uuid().c_str());
            LOG_TRACE("sp->get_set_uuid() = %s\r\n", sp->get_set_uuid().c_str());
        }

        sh->write_snapshot_config_file();
    }
   /*write a file here*/
}

void physical_packer_service_handler::take_snapshots_ex(std::vector<snapshot> & _return, const std::string& session_id, const std::set<std::string> & disks, const std::string& pre_script, const std::string& post_script)
{
    FUNC_TRACER;
    boost::filesystem::path appfile = boost::filesystem::path(system_tools::get_execution_file_path_linux());
    std::ofstream out;
    if (!pre_script.empty())
    {
        boost::filesystem::path pre_script_out = appfile.parent_path() / "pre_script.sh";
        out.open(pre_script_out.string().c_str());
        out << BASH_HEAD << std::endl << pre_script;
        out.close();
        chmod(pre_script_out.string().c_str(), S_IRWXU);
        std::string pre_result = system_tools::execute_command(pre_script_out.string().c_str());
        boost::filesystem::path pre_result_out = appfile.parent_path() / "pre_result";
        out.open(pre_result_out.string().c_str());
        out << pre_result;
        out.close();
        remove(pre_script_out.string().c_str());
    }

    take_snapshots(_return, session_id, disks);

    if (!post_script.empty())
    {
        boost::filesystem::path post_script_out = appfile.parent_path() / "post_script.sh";
        out.open(post_script_out.string().c_str());
        out << BASH_HEAD << std::endl << post_script;
        out.close();
        chmod(post_script_out.string().c_str(), S_IRWXU);
        std::string post_result = system_tools::execute_command(post_script_out.string().c_str());
        boost::filesystem::path post_result_out = appfile.parent_path() / "post_result";
        out.open(post_result_out.string().c_str());
        out << post_result;
        out.close();
        remove(post_script_out.string().c_str());
    }
}
void physical_packer_service_handler::take_snapshots2(std::vector<snapshot> & _return, const std::string& session_id, const take_snapshots_parameters& parameters)
{
    FUNC_TRACER;
    set<string> created_file_list;
    if (parameters.excluded_paths.size())
    {
        map<string, set<string>> excluded_paths_map;
        for (auto &a : parameters.excluded_paths)
        {
            string mp = system_tools::get_files_mount_point_by_command(a);
            int mpindex = a.find(mp);
            string remove_mounted = "/" + a.substr(mpindex + mp.size(), a.size() - mp.size());
            excluded_paths_map[mp].insert(remove_mounted);
        }
        for (auto &a : excluded_paths_map)
        {
            string excluded_file_name = a.first + ".excluded";
            fstream fs;
            fs.open(excluded_file_name.c_str(), ios::out);
            created_file_list.insert(excluded_file_name);
            /*UTF-8*/
            unsigned char smarker[3];
            smarker[0] = 0xEF;
            smarker[1] = 0xBB;
            smarker[2] = 0xBF;
            fs << smarker;
            /*UTF-8*/
            for (auto & b : a.second)
            {
                fs << b << std::endl;
            }
            fs.close();
        }
    }
    take_snapshots_ex(_return, session_id, parameters.disks, parameters.pre_script, parameters.post_script);
    for (auto & a : created_file_list)
    {
        remove(a.c_str());
    }
}
void physical_packer_service_handler::delete_snapshot(delete_snapshot_result& _return, const std::string& session_id, const std::string& snapshot_id) {
	FUNC_TRACER;
    _delete_snapshot(_return, session_id, snapshot_id, false);
}

void physical_packer_service_handler::delete_snapshot_set(delete_snapshot_result& _return, const std::string& session_id, const std::string& snapshot_set_id) {
	FUNC_TRACER;
    _delete_snapshot(_return, session_id, snapshot_set_id, true);
}

void physical_packer_service_handler::_delete_snapshot(delete_snapshot_result& _return, const std::string& session_id, const std::string& id, bool is_set) {
    FUNC_TRACER;
    int ret;
    int count = 0;
    LOG_TRACE("deleted_node id = %s\r\n", id.c_str());
    LOG_TRACE("is_set = %d", is_set);
    for (auto deleted_node = sh->snapshot_map.begin(); deleted_node != sh->snapshot_map.end(); ++deleted_node)
    {
        LOG_TRACE("size of sh->snapshot_map = %d\r\n", sh->snapshot_map.size());
        string uuid = deleted_node->second->get_uuid();
        string set_uuid = deleted_node->second->get_set_uuid();
        string compare_target = (is_set)? set_uuid :uuid;
        LOG_TRACE("compare_target = %s", compare_target.c_str());

        /*printf("deleted_node->second = %s\r\n", deleted_node->second->get_block_device_path().c_str());
        printf("uuid = %s\r\n", uuid.c_str());
        printf("set_uuid = %s\r\n", set_uuid.c_str());
        printf("compare_target = %s\r\n", compare_target.c_str());*/

        if (compare_target == id) {
            //printf("deleted_node->second = %s\r\n", deleted_node->second->get_block_device_path().c_str());
            ret = deleted_node->second->transition_incremental();
            if (ret)
            {
                _return.__set_non_deleted_snapshot_id(deleted_node->second->get_block_device_path());
                continue;//i think it sould be continue;
            }
            else
            {
                deleted_node->second->set_set_uuid(""); //clear the set uuid
            }
            //sh->snapshot_map.erase(deleted_node++);
            count++;
        }
    }
    LOG_TRACE("size of sh->btrfs_snapshot_vtr = %d\r\n", sh->btrfs_snapshot_vtr.size());
    btrfs_snapshot_instance::vtr::iterator deleted_node;
    for (deleted_node = sh->btrfs_snapshot_vtr.begin(); deleted_node != sh->btrfs_snapshot_vtr.end(); )
    {
        LOG_TRACE("is_set = %d", is_set);
        string uuid = (*deleted_node)->get_uuid();
        LOG_TRACE("uuid = %s", uuid.c_str());
        string set_uuid = (*deleted_node)->get_set_uuid();
        LOG_TRACE("set_uuid = %s", set_uuid.c_str());
        string compare_target = (is_set) ? set_uuid : uuid;
        LOG_TRACE("compare_target = %s", compare_target.c_str());

        /*printf("deleted_node->second = %s\r\n", deleted_node->second->get_block_device_path().c_str());
        printf("uuid = %s\r\n", uuid.c_str());
        printf("set_uuid = %s\r\n", set_uuid.c_str());
        printf("compare_target = %s\r\n", compare_target.c_str());*/
        LOG_TRACE("compare_target = %s", compare_target.c_str());
        LOG_TRACE("id = %s", id.c_str());
        if (compare_target == id) {
            //printf("deleted_node->second = %s\r\n", deleted_node->second->get_block_device_path().c_str());
            if ((*deleted_node)->delete_snapshot())
            {
                _return.__set_non_deleted_snapshot_id((*deleted_node)->get_disk_path());
                if(deleted_node!= sh->btrfs_snapshot_vtr.end())
                    ++deleted_node;
                LOG_ERROR("delete btrfs snapshot error");
                continue;//i think it sould be continue;
            }
            else
            {
                count++;
                (*deleted_node)->set_set_uuid(""); //clear the set uuid
                deleted_node = sh->btrfs_snapshot_vtr.erase(deleted_node);
                if (deleted_node == sh->btrfs_snapshot_vtr.end())
                    break;
                else
                    continue;
            }
        }
        ++deleted_node;
    }

    if (is_set)
    {
        snset.clear();
    }
    else
    {
        snapshot_instance::vtr::iterator b;
        for (b = snset.sets.begin(); b != snset.sets.end();) {
            if (!(*b)->get_set_uuid().size()) {
                b = snset.sets.erase(b);
            }
            else
                b++;
        }
        btrfs_snapshot_instance::vtr::iterator btrfs_b;
        for (btrfs_b = snset.btrfs_sets.begin(); btrfs_b != snset.btrfs_sets.end();) {
            if (!(*btrfs_b)->get_set_uuid().size()) {
                btrfs_b = snset.btrfs_sets.erase(btrfs_b);
            }
            else
                btrfs_b++;
        }

        if (!snset.sets.size() && snset.btrfs_sets.size())
        {
            snset.uuid.clear();
            snset.disks.clear();
        }
    }
    _return.__set_code(ret);
    _return.__set_deleted_snapshots(count);
    sh->write_snapshot_config_file();

}


void physical_packer_service_handler::get_all_snapshots(std::map<std::string, std::vector<snapshot> > & _return, const std::string& session_id) {
	FUNC_TRACER;
    LOG_TRACE("snapshot_map.size = %d\r\n", sh->snapshot_map.size());
    for (auto & sp : sh->snapshot_map)
    {
        auto &si = sp.second;
        /*refreash the datto info*/
        si->get_info();
        if (si->get_dattobd_info().error != 0)
        {
            LOG_ERROR("snapshot on %s has error %d.", si->get_block_device_path().c_str(), si->get_dattobd_info().error);
            need_reset_snapshot = true;
            if (cg_retry_count>0) //if retry count != 0, clear the list, retry next time
            {
                _return.clear();
                break;
            }
            else
                continue;
        }
        snapshot sn;
        _fill_snapshot_info(sn, si);
        LOG_TRACE("si->get_uuid() = %s\r\n", si->get_uuid().c_str());
        LOG_TRACE("si->get_set_uuid() = %s\r\n", si->get_set_uuid().c_str());
        _return[si->get_set_uuid()].push_back(sn);
    }
}
/*should change to snapshot mode at first and change back to inc mode*/
void physical_packer_service_handler::create_job_ex(packer_job_detail& _return, const std::string& session_id, const std::string& job_id, const create_packer_job_detail& create_job) {
	
    FUNC_TRACER;
    boost::unique_lock<boost::mutex> lock(_cs);
    linux_packer_job::ptr new_job;
    //print_all_create_job_detail(create_job);
    /*before job created check the snapshot is status 3, if not, change it to 3*/
    if (create_job.detail.p.snapshots.size())
    {
        for (auto &s : create_job.detail.p.snapshots)
        {
            for (auto & si : sh->snapshot_map)
            {
                if (si.second->get_uuid() == s.snapshot_id && si.second->get_dattobd_info().state == 2)
                {
                    if (si.second->transition_snapshot())
                    {
                        LOG_ERROR("trans to snapshot fail. return");
                        return;
                    }
                }
            }
        }
    }
    try {
        LOG_TRACE("job_id = %s\r\n", job_id.c_str());
        new_job = linux_packer_job::create(job_id, create_job,sh,this,b_force_full, is_skip_read_error);
        if (!new_job) {
            LOG_ERROR("new_job create fail\r\n");
        }

        b_force_full = false;
    }
    catch (boost::exception &e) {
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_JOB_CREATE_FAIL;
        error.why = "Create job failure (Invalid job id).";
        throw error;
    }
    try
    {
        if (!new_job) {
            saasame::transport::invalid_operation error;
            error.what_op = saasame::transport::error_codes::SAASAME_E_JOB_CREATE_FAIL;
            error.why = "Create job failure";
            throw error;
        }
        else {
            if (!_scheduled.count(new_job->id())) {
                /*read the new file here*/
                /*boost::filesystem::path p = boost::filesystem::path(system_tools::get_execution_file_path_linux()).parent_path();
                p = p / ".exclude";
                fstream fs;
                fstream fs2;
                if (boost::filesystem::exists(p))
                {
                    fs.open(p.string().c_str(), ios::in);
                    string input;
                    while (getline(fs, input))
                    {
                        boost::filesystem::path tempp = boost::filesystem::path(input);
                        if(boost::filesystem::exists(tempp))
                            new_job->get_create_job_detail()->detail.p.excluded_paths.insert(input);
                    }
                    fs.close();
                }
                

                boost::filesystem::path p2 = boost::filesystem::path(system_tools::get_execution_file_path_linux()).parent_path();
                p2 = p2 / ".resync";
                if (boost::filesystem::exists(p2))
                {
                    fs2.open(p2.string().c_str(), ios::in);
                    string input;
                    while (getline(fs2, input))
                    {
                        boost::filesystem::path tempp = boost::filesystem::path(input);
                        if (boost::filesystem::exists(tempp))
                            new_job->get_create_job_detail()->detail.p.resync_paths.insert(input);
                    }
                    fs2.close();
                }*/


                new_job->register_job_to_be_executed_callback_function(boost::bind(&physical_packer_service_handler::job_to_be_executed_callback, this, _1, _2));
                new_job->register_job_was_executed_callback_function(boost::bind(&physical_packer_service_handler::job_was_executed_callback, this, _1, _2, _3));
                run_once_trigger rot;
                _scheduler.schedule_job(new_job, rot); //push the job to scheduler
                _scheduled[new_job->id()] = new_job;  //and add the job into the _scheduled
                new_job->save_config();
                _return.id = new_job->id();
                _return = new_job->get_job_detail();
            }
            else {
                saasame::transport::invalid_operation error;
                error.what_op = saasame::transport::error_codes::SAASAME_E_JOB_ID_DUPLICATED;
                error.why = boost::str(boost::format("Job '%1%' is duplicated") % new_job->id());
                throw error;
            }
        }
    }
    catch (...)
    {
        LOG_ERROR("there are exception\r\rn");
    }
}

void physical_packer_service_handler::create_job(packer_job_detail& _return, const std::string& session_id, const create_packer_job_detail& create_job) {
    FUNC_TRACER;
}

void physical_packer_service_handler::get_job(packer_job_detail& _return, const std::string& session_id, const std::string& job_id, const std::string& previous_updated_time) {
	//FUNC_TRACER;
    boost::unique_lock<boost::mutex> lock(_cs);
    if (!_scheduled.count(job_id)) {
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_JOB_NOTFOUND;
        error.why = boost::str(boost::format("Job '%1%' is not found.") % job_id);
        throw error;
    }
    _return = _scheduled[job_id]->get_job_detail(boost::posix_time::time_from_string(previous_updated_time));
}

bool physical_packer_service_handler::interrupt_job(const std::string& session_id, const std::string& job_id) { 
    boost::unique_lock<boost::mutex> lock(_cs);
    FUN_TRACE;
    if (!_scheduled.count(job_id)) {
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_JOB_NOTFOUND;
        error.why = boost::str(boost::format("Job '%1%' is not found.") % job_id);
        throw error;
    }
    _scheduler.interrupt_job(job_id);
    _scheduled[job_id]->cancel();
    return true;
}

bool physical_packer_service_handler::resume_job(const std::string& session_id, const std::string& job_id) {
    boost::unique_lock<boost::mutex> lock(_cs);
    FUN_TRACE;
    if (!_scheduled.count(job_id)) {
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_JOB_NOTFOUND;
        error.why = boost::str(boost::format("Job '%1%' is not found.") % job_id);
        throw error;
    }
    if (!_scheduler.is_scheduled(job_id)) {
        _scheduled[job_id]->resume();
        run_once_trigger rot;
        _scheduler.schedule_job(_scheduled[job_id],rot);
    }
    return true;
}

void physical_packer_service_handler::compress_log_file()
{
    /*zip the log, i think it's should be larger than imagenation, compress and remove ori file*/
    boost::filesystem::path working_dir = boost::filesystem::path(system_tools::get_execution_file_path_linux()).parent_path();
    string filename = "linux_packer_log_" + to_string(0) + ".zip";
    /*rename the logs zip*/
    int result;
    for (int i = PK_MAX_LOGS_COUNT; i >= 0;--i)
    {
        string old_filename = "linux_packer_log_" + to_string(i) + ".zip";
        string new_filename = "linux_packer_log_" + to_string(i+1) + ".zip";
        boost::filesystem::path old_rename_logzip = working_dir / "logs" / old_filename;
        boost::filesystem::path new_rename_logzip = working_dir / "logs" / new_filename;
        if (boost::filesystem::exists(old_rename_logzip))
        {
            if(i+1<=PK_MAX_LOGS_COUNT)
                result = rename(old_rename_logzip.string().c_str(), new_rename_logzip.string().c_str());
            else
                result = remove(old_rename_logzip.string().c_str());
        }
    }
    boost::filesystem::path logzip = working_dir / "logs" / filename;
    linux_tools::archive::zip::ptr zip_ptr = linux_tools::archive::zip::open(logzip);
    if (zip_ptr) {
        boost::filesystem::path appfile = boost::filesystem::path(system_tools::get_execution_file_path_linux());
        string logfilename = appfile.filename().string() + ".log";
        boost::filesystem::path abs_logfilename = working_dir / ("logs") / (logfilename);
        restore_stdout();
        zip_ptr->add(abs_logfilename, MZ_DEFAULT_LEVEL);
        zip_ptr->close();
        remove(abs_logfilename.string().c_str());
        redirect_stdout(abs_logfilename.string().c_str());
    }
}

bool physical_packer_service_handler::remove_job(const std::string& session_id, const std::string& job_id) { 
    boost::unique_lock<boost::mutex> lock(_cs);
    FUN_TRACE;
    if (_scheduled.count(job_id)) {
        _scheduler.interrupt_job(job_id);
        _scheduled[job_id]->cancel();
        _scheduled[job_id]->remove();
        _scheduler.remove_job(job_id);
        _scheduled.erase(job_id);
    }
    if (need_reset_snapshot)
    {
        need_reset_snapshot = false;
        /*clear all and reset*/
        if (cg_retry_count && !sh->reset_all_snapshot())
        {
            sh->destroy_all_snapshot();
            snapshot_init(sh);
        }
        cg_retry_count--;  //if there are error happened , just reduce the retry count
    }
    else
        cg_retry_count = 2; //if there are finished, reset the retry count to 2
    compress_log_file();
    /*check the log */
    return true;
}

void physical_packer_service_handler::list_jobs(std::vector<packer_job_detail> & _return, const std::string& session_id) {
    boost::unique_lock<boost::mutex> lock(_cs);
    FUNC_TRACER;
    for(linux_packer_job::map::value_type& j: _scheduled) {
        _return.push_back(j.second->get_job_detail());
    }
}

void physical_packer_service_handler::terminate(const std::string& session_id) {
    boost::unique_lock<boost::mutex> lock(_cs);
    FUNC_TRACER;
}

bool physical_packer_service_handler::running_job(const std::string& session_id, const std::string& job_id) { 
    boost::unique_lock<boost::mutex> lock(_cs);
    FUN_TRACE;
    if (!_scheduled.count(job_id))
    {
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_JOB_NOTFOUND;
        error.why = boost::str(boost::format("Job '%1%' is not found.") % job_id);
        throw error;
    }
    else
    {
        return _scheduler.is_running(job_id);
    }
}
/*thrift api end*/

struct thread_input
{
    universal_disk_rw::ptr rw;
    uint64_t size;
};

void random_datto_read(thread_input * in)
{
    //int size = si->get_block_device_size();
    //universal_disk_rw::ptr rw = si->get_src_rw();
    std::unique_ptr<BYTE> buf(new BYTE[8 * 1024 * 1024]); //max 8M
    srand((unsigned int)time(NULL));
    while (1)
    {
        uint32_t data_count = 0;
        uint64_t start = rand() % in->size;
        uint64_t max_length = ((in->size - start) > (8 * 1024 * 1024)) ? (8 * 1024 * 1024) : (in->size - start);
        uint64_t legth = rand() % max_length;
        in->rw->read(start, max_length, buf.get(), data_count);
        printf("start = %llu, legth = %llu\r\n", start, legth);
    }
}

void physical_packer_service_handler::random_mutli_threads_datto_read(std::string input)
{
    printf("\r\nrandom_mutli_threads_datto_read\r\n\r\n");
    int result = 0;
    snapshot_instance::vtr sis = sh->take_snapshot_by_disk_ab_path(input, result);

    //std::string str_dst = output;

    //universal_disk_rw::ptr dst = general_io_rw::open_rw(str_dst, false);

    universal_disk_rw::vtr rws;
    /*change to snapshot mode*/
    std::unique_ptr<pthread_t> thread_id_buf(new pthread_t[4*sis.size()]); //max 8M
    std::unique_ptr<thread_input> ti(new thread_input[4 * sis.size()]);
    int index = 0;
    for (auto & si : sis)
    {
        si->transition_snapshot();
        for (int i = 0; i < 4; ++i)
        {
            thread_input * _ti = &ti.get()[index * 4 + i];
            _ti->rw = si->get_src_rw();
            _ti->size = si->get_block_device_size();
            int ret = pthread_create(&(thread_id_buf.get()[index*4+i]), NULL, random_datto_read, (void *)_ti);
        }
        ++index;
    }

    for (int i = 0; i < 4 * sis.size(); ++i)
    {
        int ret = 0;
        pthread_join(thread_id_buf.get()[i], NULL);
    }

    /*for (auto & si : sis)
        si->destroy();*/
}


void physical_packer_service_handler::job_to_be_executed_callback(const trigger::ptr& job, const job_execution_context& ec) {
    boost::unique_lock<boost::mutex> lock(_cs);

}
void physical_packer_service_handler::job_was_executed_callback(const trigger::ptr& job, const job_execution_context& ec, const job::exception& ex) {
    boost::unique_lock<boost::mutex> lock(_cs);

}

bool physical_packer_service_handler::create_mutex(const std::string& session, const int16_t timeout) {
    FUNC_TRACER;
    //_tcs.try_lock_for(seconds(timeout));
    return false;
}
bool physical_packer_service_handler::delete_mutex(const std::string& session) {
    FUNC_TRACER;
    //_tcs.unlock();
    return false;

}

void physical_packer_service_handler::snapshot_init(snapshot_manager::ptr sh)
{
    FUNC_TRACER;
    linux_storage::storage::ptr str = linux_storage::storage::get_storage();
    disk::vtr dsks = str->get_all_disk();
    std::set<std::string> uri_set;
    for(auto & dsk : dsks)
    {
        LOG_TRACE("dsk->string_uri = %s\r\n", dsk->string_uri.c_str());
        uri_set.insert(dsk->string_uri);
    }
    unsigned char result = 0;
    snapshot_instance::vtr sis = sh->take_snapshot_by_disk_uri(uri_set, &result, str);
    LOG_TRACE("take_snapshot_by_disk_uri create %d snapshots successes\r\n", sis.size());
    if (!sis.empty())
        b_force_full = true;
    /*force all change to transition_incremental*/
    /*for (auto &si : sh->snapshot_map)
    {
        si.second->get_info();
        if (si.second->get_dattobd_info().state == 3)
            si.second->transition_incremental();
    }*/
    for (auto & a : sh->snapshot_map_by_uri)
    {
        LOG_TRACE("a.first = %s", a.first.c_str());
    }
    if(snset.disks.size())
    {
        for (auto & uri : snset.disks)
        {
            if (sh->snapshot_map_by_uri.count(uri))
            {
                for (auto & si : sh->snapshot_map_by_uri[uri])
                {
                    snset.sets.push_back(si);
                    si->set_set_uuid(snset.uuid);
                }
            }
        }
    }
    for (auto & a : sh->snapshot_map_by_uri)
    {
        LOG_TRACE("~a.first = %s", a.first.c_str());
    }

    sh->write_snapshot_config_file();
    /*over*/
}
void* kill_thread()
{
    sleep(10);
    service_status::unregister();
    exit(1);
}
bool physical_packer_service_handler::unregister(const std::string& session)
{
    pthread_t p;
    pthread_create(&p,NULL, kill_thread,NULL);
    return true;
}
/*
  CalculatorIfFactory is code generated.
  CalculatorCloneFactory is useful for getting access to the server side of the
  transport.  It is also useful for making per-connection state.  Without this
  CloneFactory, all connections will end up sharing the same handler instance.
*/
extern void create_config_file();
extern void read_config_file(std::string * version, ::string * uuid, int * verbose, bool * is_skip_read_error, std::map<std::string, int> * snapshots_cow_space_user_config, uint64_t * merge_size);

stdcxx::shared_ptr<TThreadedServer>  g_physical_packer_server;
stdcxx::shared_ptr<TThreadedServer>  g_proxy_server;


bool b_polling_mode = false;
int main() {
    //int i = mcheck(NULL);
    boost::filesystem::path appfile = boost::filesystem::path(system_tools::get_execution_file_path_linux());
    string logfilename = appfile.filename().string() + ".log";
    boost::filesystem::path logfile = appfile.parent_path() / "logs" / logfilename;
    boost::filesystem::create_directories(logfile.parent_path());
    std::string machine_id;
    std::string computer_name,arch;
    int major, minor;
    system_tools::get_os_pretty_name_and_info(computer_name, arch, major,minor);
    create_config_file();
    string version;
    uint64_t merge_size = 0;
    read_config_file(&version,&machine_id, NULL, NULL,NULL, &merge_size);
    //test_child rc();
    //SET_LOG_OUTPUT(logfile.string());
    redirect_stdout(logfile.string().c_str());
    //fflush(stdout);
    //int newstdout = open(logfile.string().c_str(), O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    //LOG_TRACE("newstdout = %d", newstdout);
    //dup2(newstdout, fileno(stdout));
    //close(newstdout);
    service_status ss;
    LOG_TRACE("ss.getSessionId() = %s", ss.getSessionId().c_str());
    LOG_TRACE("ss.getMgmtAddr() = %s", ss.getMgmtAddr().c_str());
    if (ss.getSessionId().length() && ss.getMgmtAddr().length())
    {
        LOG_TRACE("b_polling_mode = true");
        b_polling_mode = true;
    }
    else
    {
        LOG_TRACE("b_polling_mode = false");
    }

    /*test in main*/
    /*string gg = string("tpe2.saasame.com");
    string ip_address = system_tools::analysis_ip_address(gg);
    LOG_TRACE("path to tpe2.saasame.com = %s", ip_address.c_str());
    gg = string("MUMI");
    ip_address = system_tools::analysis_ip_address(gg);
    LOG_TRACE("path to MUMI = %s", ip_address.c_str());
    gg = string("192.168.31.53");
    ip_address = system_tools::analysis_ip_address(gg);
    LOG_TRACE("path to 192.168.31.53 = %s", ip_address.c_str());*/

    vector<string> ip_addr = system_tools::get_ip_address();
    for (auto & i : ip_addr)
    {
        LOG_TRACE("ip_addr = %s\r\n", i.c_str());
    }

    /*std::string result_string;
    result_string = system_tools::execute_command(SYSTEMCTL_RELOAD);
    LOG_TRACE("result_string = %s\r\n", result_string.c_str());
    result_string = system_tools::execute_command(INITCTL_RELOAD);
    LOG_TRACE("result_string = %s\r\n", result_string.c_str());*/
  /*TThreadedServer server(
    stdcxx::make_shared<CalculatorProcessorFactory>(stdcxx::make_shared<CalculatorCloneFactory>()),
    stdcxx::make_shared<TServerSocket>(9090), //port
    stdcxx::make_shared<TBufferedTransportFactory>(),
    stdcxx::make_shared<TBinaryProtocolFactory>());*/

  
  // if you don't need per-connection state, do the following instead
	
	/*just test the simple function*/


	/*this is SSL part, got the path*/
	char buf[1024];
	GetExecutionFilePath_Linux(buf);
	string crtFileName("server.crt"), keyFileName("server.key"), working_path = system_tools::path_remove_last(string(buf));

	/*disable SIGPIPE*/
	struct sigaction sa;
	sa.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &sa, 0);

	//stdcxx::shared_ptr<TSSLSocketFactory> factory = getTSSLSocketFactoryHelper(true, true, working_path, crtFileName, keyFileName);

	stdcxx::shared_ptr<TSSLSocketFactory> factory;
	factory = stdcxx::shared_ptr<TSSLSocketFactory>(new TSSLSocketFactory());
    //g_factory = stdcxx::shared_ptr<TSSLSocketFactory>(new TSSLSocketFactory());
    //g_factory = stdcxx::shared_ptr<TSSLSocketFactory>(new TSSLSocketFactory());
    //g_accessManager = std::shared_ptr<AccessManager>(new MyAccessManager());

    factory->server(true);
    factory->authenticate(true);
    factory->loadCertificate((working_path + crtFileName).c_str());
    factory->loadPrivateKey((working_path + keyFileName).c_str());
    factory->loadTrustedCertificates((working_path + crtFileName).c_str());
    stdcxx::shared_ptr<TServerTransport> ssl_serverTransport;
    if (b_polling_mode)
    {
        ssl_serverTransport = stdcxx::shared_ptr<TServerTransport>(new TSSLServerSocket("127.0.0.1",g_saasame_constants.PHYSICAL_PACKER_SERVICE_PORT, factory));
        LOG_TRACE( "Disable listen port from \'*\'.");
    }
    else
    {
        ssl_serverTransport = stdcxx::shared_ptr<TServerTransport>(new TSSLServerSocket(g_saasame_constants.PHYSICAL_PACKER_SERVICE_PORT, factory));
    }
    stdcxx::shared_ptr<physical_packer_service_handler> ppsh(new physical_packer_service_handler());
	//ppsh->init_config();

    //ppsh->test_clone_disk("/","/image_sda1");
    //ppsh->random_mutli_threads_datto_read("/dev/sda1");
    stdcxx::shared_ptr<TProcessor> physical_packer_processor(new physical_packer_serviceProcessor(ppsh));
    stdcxx::shared_ptr<TTransportFactory> transportFactory(new TBufferedTransportFactory());
    stdcxx::shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());
  
    g_physical_packer_server = stdcxx::shared_ptr<TThreadedServer>(new TThreadedServer(physical_packer_processor,
        ssl_serverTransport,
        transportFactory,
        protocolFactory));
    /*if there is polling mode, it should be create a proxy server*/
    if (b_polling_mode)
    {
        stdcxx::shared_ptr<TServerTransport> proxyTransport(new TServerReverseSocket(ss.getMgmtAddr(), ss.getSessionId(), machine_id, computer_name));
        g_proxy_server = stdcxx::shared_ptr<TThreadedServer>(new TThreadedServer(physical_packer_processor,
            proxyTransport,
            transportFactory,
            protocolFactory));
    }
  /**
   * Here are some alternate server types...

  // This server only allows one connection at a time, but spawns no threads
  TSimpleServer server(
    stdcxx::make_shared<CalculatorProcessor>(stdcxx::make_shared<CalculatorHandler>()),
    stdcxx::make_shared<TServerSocket>(9090),
    stdcxx::make_shared<TBufferedTransportFactory>(),
    stdcxx::make_shared<TBinaryProtocolFactory>());

  const int workerCount = 4;

  stdcxx::shared_ptr<ThreadManager> threadManager =
    ThreadManager::newSimpleThreadManager(workerCount);
  threadManager->threadFactory(
    stdcxx::make_shared<PlatformThreadFactory>());
  threadManager->start();

  // This server allows "workerCount" connection at a time, and reuses threads
  TThreadPoolServer server(
    stdcxx::make_shared<CalculatorProcessorFactory>(stdcxx::make_shared<CalculatorCloneFactory>()),
    stdcxx::make_shared<TServerSocket>(9090),
    stdcxx::make_shared<TBufferedTransportFactory>(),
    stdcxx::make_shared<TBinaryProtocolFactory>(),
    threadManager);
  */

  cout << "Starting the server..." << endl;
  if (g_physical_packer_server)
  {
      boost::thread_group tg;
      tg.create_thread(boost::bind(&TThreadedServer::serve, g_physical_packer_server));
      if (g_proxy_server)
      {
          b_polling_mode = true;
          tg.create_thread(boost::bind(&TThreadedServer::serve, g_proxy_server));
      }
      else
      {
          b_polling_mode = false;
      }
      tg.join_all();
  }

  //server.serve();// change to create a thread;

  cout << "Done." << endl;
  return 0;
}
