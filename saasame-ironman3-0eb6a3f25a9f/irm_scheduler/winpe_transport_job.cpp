#include "transport_job.h"
#include "common_service_handler.h"
#include "carrier_rw.h"
#include <codecvt>
#ifndef string_map
typedef std::map<std::string, std::string> string_map;
#endif

#ifndef string_set_map
typedef std::map<std::string, std::set<std::string>> string_set_map;
#endif

void winpe_transport_job::execute(){
    VALIDATE;
    FUN_TRACE;
    if (_is_canceled || _is_removing || _state & mgmt_job_state::job_state_finished){
        return;
    }
    bool is_get_replica_job_create_detail = false;
    if (!_is_initialized)
        is_get_replica_job_create_detail = _is_initialized = get_replica_job_create_detail(_replica_job_create_detail);
    else
        is_get_replica_job_create_detail = get_replica_job_create_detail(_replica_job_create_detail);
    save_config();
    save_status();
    if (!is_get_replica_job_create_detail){
        record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_E_FAIL, record_format("Cannot get the replica job detail."));
        save_status();
    }
    else if (!_replica_job_create_detail.is_paused){
        uint32_t concurrent_count = boost::thread::hardware_concurrency();
        registry reg;
        if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
            if (reg[L"ConcurrentSchedulerJobNumber"].exists() && (DWORD)reg[L"ConcurrentSchedulerJobNumber"] > 0){
                concurrent_count = ((DWORD)reg[L"ConcurrentSchedulerJobNumber"]);
            }
        }
        macho::windows::semaphore  scheduler(L"Global\\Transport", concurrent_count);
        _latest_enter_time = _latest_leave_time = boost::date_time::not_a_date_time;
        _latest_enter_time = boost::posix_time::microsec_clock::universal_time();
        _terminated = false;
        _thread = boost::thread(&transport_job::update, this, true);
        _state &= ~mgmt_job_state::job_state_error;
        physical_packer_job_op op(_replica_job_create_detail.addr, _create_job_detail.mgmt_addr);
        if (!op.initialize(_priority_packer_addr)){
            _state |= mgmt_job_state::job_state_error;
            record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST, record_format("Cannot connect to the physical host."));
        }
        else if (!op.can_connect_to_carriers(_replica_job_create_detail.carriers, _replica_job_create_detail.is_encrypted)){
            _state |= mgmt_job_state::job_state_error;
            record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_E_FAIL, record_format("The Packer cannot connect to the Carrier."));
        }
        else if (_will_be_resumed = !scheduler.trylock()){
            LOG(LOG_LEVEL_WARNING, L"Resource limited. Job %s will be resumed later.", _id.c_str());
            record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_E_QUEUE_FULL,
                record_format("Pending for available job resource."));
        }
        else{
            _is_interrupted = false;
            _is_source_ready = true;
            record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_S_OK, record_format("Carrier job started."));
            save_status();

            if (_state & mgmt_job_state::job_state_replicating){ // Error handling for the re-enter job
                //Checking snapshoting and job ....
                if (!is_packer_job_running(op)){
                    if (remove_packer_jobs(op, false)){
                        if (!(_state & mgmt_job_state::job_state_sche_completed)){
                            macho::windows::auto_lock lock(_cs);
                            _state = mgmt_job_state::job_state_initialed;
                        }
                    }
                }
            }

            if (_state & mgmt_job_state::job_state_sche_completed){
                if (_update(true)){
                    macho::windows::auto_lock lock(_cs);
                    _state = mgmt_job_state::job_state_finished;
                    return;
                }
                else{
                    record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_E_FAIL, record_format("Cannot connect to the management server."));
                }
            }

            if (_state & mgmt_job_state::job_state_none){
                
                //Initial checking 
                std::set<disk_info> disks;
                if (op.enumerate_disks(disks)){
                    bool found = false;
                    foreach(std::string uri, _replica_job_create_detail.disks){
                        disk_universal_identify disk_id(uri);
                        found = false;
                        foreach(disk_info d, disks){
                            if (found = (disk_id == disk_universal_identify(d.uri))){
                                physical_transport_job_progress::ptr progress;
                                if (!_progress.count(uri)){
                                    progress = physical_transport_job_progress::ptr(new physical_transport_job_progress());
                                    _progress[uri] = progress;
                                }
                                else{
                                    progress = _progress[uri];
                                }
                                progress->disk_uri = d.uri;
                                progress->total_size = d.size;
                                progress->number = d.number;
                                progress->friendly_name = _optimize_file_name(d.friendly_name);
                                break;
                            }
                        }
                        if (!found){
                            record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_E_PHYSICAL_CONFIG_FAILED, (record_format("Cannot find disks from host %1%.") % _replica_job_create_detail.host));
                            break;
                        }
                    }
                    if (found){
                        _state = mgmt_job_state::job_state_initialed;
                    }
                    save_status();
                }
            }

            if (_state & mgmt_job_state::job_state_initialed){
                // Create replication job.
                std::string update_time = _optimize_file_name(boost::posix_time::to_simple_string(boost::posix_time::second_clock::universal_time()));
                std::set<std::string> connections;

                physical_create_packer_job_detail detail;

                std::string job_id = macho::stringutils::convert_unicode_to_utf8(_id);
                foreach(string_map::value_type t, _replica_job_create_detail.targets)
                    connections.insert(t.first);

                foreach(physical_transport_job_progress::map::value_type p, _progress){
                    if (p.second->base.length())
                        detail.images[p.first].base = p.second->base;
                    else{
                        if (_replica_job_create_detail.disk_ids.count(p.first))
                            detail.images[p.first].base = p.second->base = _replica_job_create_detail.disk_ids[p.first];
                        else
                            detail.images[p.first].base = p.second->base = boost::str(boost::format("%s_%d") % p.second->friendly_name %p.second->number);
                    }

                    if (p.second->backup_size == 0){
                        detail.images[p.first].parent = p.second->parent = p.second->output;
                        detail.images[p.first].name = p.second->output = boost::str(boost::format("%s_%s") % p.second->base %update_time);
                    }
                    else{
                        detail.images[p.first].parent = p.second->parent;
                        detail.images[p.first].name = p.second->output;
                    }
                    if (p.second->backup_size != 0)
                        detail.backup_size[p.first] = p.second->backup_size;
                    if (p.second->backup_progress != 0)
                        detail.backup_progress[p.first] = p.second->backup_progress;
                    if (p.second->backup_image_offset != 0)
                        detail.backup_image_offset[p.first] = p.second->backup_image_offset;
                    if (p.second->completed_blocks.size() != 0)
                        detail.completed_blocks[p.first] = p.second->completed_blocks;
                }

                boost::posix_time::ptime created_time;
                std::vector<snapshot> snapshots;
                saasame::transport::snapshot snapshot;
                snapshot.snapshot_set_id = macho::guid_(_id);
                snapshot.snapshot_id = snapshot.snapshot_set_id;
                snapshot.creation_time_stamp = boost::posix_time::to_simple_string(boost::posix_time::microsec_clock::universal_time());
                snapshots.push_back(snapshot);
                detail.snapshots = snapshots;
                detail.previous_journals = _previous_journals;
                if (op.create_packer_job(connections, 
                    _replica_job_create_detail, 
                    detail,
                    _excluded_paths,
                    _resync_paths,
                    job_id, 
                    created_time, 
                    saasame::transport::job_type::winpe_packer_job_type)){
                    macho::windows::auto_lock lock(_cs);
                    foreach(string_map::value_type t, _replica_job_create_detail.targets){
                        _packer_jobs[t.first] = job_id;
                        _packer_time_diffs[job_id] = boost::posix_time::microsec_clock::universal_time() - created_time;
                        _jobs_state[job_id].state = job_state::type::job_state_none;
                    }
                    _snapshots.push_back(snapshot);
                    save_status();
                }
                else{
                    record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_E_JOB_CREATE_FAIL, (record_format("Cannot create a Packer job for host %1%.") % _replica_job_create_detail.host));
                    
                    _state |= mgmt_job_state::job_state_error;
                    save_status();
                }
            }

            if (!(_state & mgmt_job_state::job_state_error)){
                if ((_state & mgmt_job_state::job_state_initialed) || (_state & mgmt_job_state::job_state_replicating)){
                    //checking replication status
                    _state = mgmt_job_state::job_state_replicating;
                    save_status();
                    bool is_error = false;
                    while ((!is_error) && (!_is_interrupted) && (!_is_canceled)){
                        boost::this_thread::sleep(boost::posix_time::seconds(1));
                        {
                            macho::windows::auto_lock lock(_cs);
                            bool finished;
                            typedef std::map<std::string, running_job_state>  job_state_map;
                            foreach(job_state_map::value_type s, _jobs_state){
                                finished = false;
                                if (s.second.is_error){
                                    _state |= mgmt_job_state::job_state_error;
                                    is_error = true;
                                    break;
                                }
                                else if (s.second.disconnected){
                                    is_error = true;
                                    break;
                                }
                                else if (!(finished = (job_state::type::job_state_finished == (s.second.state & job_state::type::job_state_finished))))
                                    break;
                            }
                            if (finished && _jobs_state.size() == _packer_jobs.size()){
                                _state = mgmt_job_state::job_state_replicated;
                                save_status();
                                break;
                            }
                        }
                    }
                }
            }

            if ((_state & mgmt_job_state::job_state_replicated) || _is_canceled) {
                //Remove snapshots and job when replication was finished.                
                if (remove_packer_jobs(op, true)){
                    _state = mgmt_job_state::job_state_sche_completed;
                    if (_update(true)){
                        _state = mgmt_job_state::job_state_finished;
                    }
                    else{
                        record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_E_FAIL, record_format("Cannot connect to the management server."));
                    }
                }
                save_status();
            }
        }
        _latest_leave_time = boost::posix_time::microsec_clock::universal_time();
        save_status();
        _terminated = true;
        if (_thread.joinable())
            _thread.join();
    }
}