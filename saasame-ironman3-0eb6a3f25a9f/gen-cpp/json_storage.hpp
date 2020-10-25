#pragma once
#ifndef __json_storage__
#define __json_storage__

#include "macho.h"
#include "json_spirit.h"
#include <codecvt>
#pragma comment(lib, "json_spirit_lib.lib")

namespace macho{ namespace windows{

class json_storage : virtual public storage {

protected:
    static const json_spirit::mValue& find_value(const json_spirit::mObject& obj, const std::string& name){
        json_spirit::mObject::const_iterator i = obj.find(name);
        assert(i != obj.end());
        assert(i->first == name);
        return i->second;
    }
    static const std::string find_value_string(const json_spirit::mObject& obj, const std::string& name){
        json_spirit::mObject::const_iterator i = obj.find(name);
        if (i != obj.end()){
            return i->second.get_str();
        }
        return "";
    }

    static const int find_value_int32(const json_spirit::mObject& obj, const std::string& name, int default_value = 0){
        json_spirit::mObject::const_iterator i = obj.find(name);
        if (i != obj.end()){
            return i->second.get_int();
        }
        return default_value;
    }

    class json_disk : virtual public disk{
    public:
        json_disk(json_spirit::mObject &obj, storage::ptr storage) : _obj(obj), _storage(storage){}
        virtual bool online(){ return false; }
        virtual bool offline(){ return false; }
        virtual bool clear_read_only_flag(){ return false; }
        virtual bool set_read_only_flag(){ return false; }
        virtual bool initialize(ST_PARTITION_STYLE partition_style = ST_PST_GPT) { return false; }
        virtual partition::ptr create_partition(uint64_t size = 0,
            bool use_maximum_size = true,
            uint64_t offset = 0,
            uint32_t alignment = 0,
            std::wstring drive_letter = L"",
            bool assign_drive_letter = true,
            ST_MBR_PARTITION_TYPE mbr_type = ST_IFS,
            std::wstring  gpt_type = L"{ebd0a0a2-b9e5-4433-87c0-68b6b72699c7}",
            bool is_hidden = false,
            bool is_active = false
            ){
            return false;
        }
        virtual std::wstring        unique_id(){ return macho::stringutils::convert_utf8_to_unicode(find_value_string(_obj, "unique_id")); }
        virtual uint16_t            unique_id_format(){ return find_value_int32(_obj, "unique_id_format"); }
        virtual std::wstring        path() { return macho::stringutils::convert_utf8_to_unicode(find_value(_obj, "path").get_str()); }
        virtual std::wstring        location() { return macho::stringutils::convert_utf8_to_unicode(find_value(_obj, "location").get_str()); }
        virtual std::wstring        friendly_name() { return macho::stringutils::convert_utf8_to_unicode(find_value(_obj, "friendly_name").get_str()); }
        virtual uint32_t            number() { return find_value(_obj, "number").get_int(); }
        virtual std::wstring        serial_number() { return macho::stringutils::convert_utf8_to_unicode(find_value(_obj, "serial_number").get_str()); }
        virtual std::wstring        firmware_version() { return macho::stringutils::convert_utf8_to_unicode(find_value(_obj, "firmware_version").get_str()); }
        virtual std::wstring        manufacturer() { return macho::stringutils::convert_utf8_to_unicode(find_value(_obj, "manufacturer").get_str()); }
        virtual std::wstring        model() { return macho::stringutils::convert_utf8_to_unicode(find_value(_obj, "model").get_str()); }
        virtual uint64_t            size() { return find_value(_obj, "size").get_uint64(); }
        virtual uint64_t            allocated_size() { return find_value(_obj, "allocated_size").get_uint64(); }
        virtual uint32_t            logical_sector_size() { return find_value(_obj, "logical_sector_size").get_int(); }
        virtual uint32_t            physical_sector_size() { return find_value(_obj, "physical_sector_size").get_int(); }
        virtual uint64_t            largest_free_extent() { return find_value(_obj, "largest_free_extent").get_uint64(); }
        virtual uint32_t            number_of_partitions() { return find_value(_obj, "number_of_partitions").get_int(); }
        virtual uint16_t            health_status() { return find_value(_obj, "health_status").get_int(); }
        virtual uint16_t            bus_type() { return find_value(_obj, "bus_type").get_int(); }
        virtual ST_PARTITION_STYLE  partition_style() { return (ST_PARTITION_STYLE)find_value(_obj, "partition_style").get_int(); }
        virtual uint32_t            signature() { return find_value(_obj, "signature").get_int(); }
        virtual std::wstring        guid() { return macho::stringutils::convert_utf8_to_unicode(find_value(_obj, "guid").get_str()); }
        virtual bool                is_offline() { return find_value(_obj, "is_offline").get_bool(); }
        virtual uint16_t            offline_reason() { return find_value(_obj, "offline_reason").get_int(); }
        virtual bool                is_read_only() { return find_value(_obj, "is_read_only").get_bool(); }
        virtual bool                is_system() { return find_value(_obj, "is_system").get_bool(); }
        virtual bool                is_clustered() { return find_value(_obj, "is_clustered").get_bool(); }
        virtual bool                is_boot() { return find_value(_obj, "is_boot").get_bool(); }
        virtual bool                boot_from_disk() { return find_value(_obj, "boot_from_disk").get_bool(); }
        virtual uint32_t            sectors_per_track() { return find_value(_obj, "sectors_per_track").get_int(); }
        virtual uint32_t            tracks_per_cylinder() { return find_value(_obj, "tracks_per_cylinder").get_int(); }
        virtual uint64_t            total_cylinders() { return find_value(_obj, "total_cylinders").get_uint64(); }
        virtual uint32_t            scsi_bus() { return find_value(_obj, "scsi_bus").get_int(); }
        virtual uint16_t            scsi_logical_unit() { return find_value(_obj, "scsi_logical_unit").get_int(); }
        virtual uint16_t            scsi_port() { return find_value(_obj, "scsi_port").get_int(); }
        virtual uint16_t            scsi_target_id() { return find_value(_obj, "scsi_target_id").get_int(); }
        virtual volume::vtr         get_volumes(){ return _storage->get_volumes(number()); }
        virtual partition::vtr      get_partitions() { return _storage->get_partitions(number()); }
    private:
        json_spirit::mObject _obj;
        storage::ptr         _storage;
    };

    class json_partition : virtual public partition{
    public:
        json_partition(json_spirit::mObject &obj) : _obj(obj){}
        virtual uint32_t       disk_number()  { return find_value(_obj, "disk_number").get_int(); }
        virtual uint32_t       partition_number()  { return find_value(_obj, "partition_number").get_int(); }
        virtual std::wstring   drive_letter(){ return macho::stringutils::convert_utf8_to_unicode(find_value(_obj, "drive_letter").get_str()); }
        virtual string_array_w access_paths() {
            string_array_w paths;
            json_spirit::mArray  access_paths = find_value(_obj, "access_paths").get_array();
            foreach(json_spirit::mValue p, access_paths){
                paths.push_back(macho::stringutils::convert_utf8_to_unicode(p.get_str()));
            }
            return paths;
        }
        virtual uint64_t     offset()  { return find_value(_obj, "offset").get_uint64(); }
        virtual uint64_t     size()  { return find_value(_obj, "size").get_uint64(); }
        virtual uint16_t     mbr_type()  { return find_value(_obj, "mbr_type").get_int(); }
        virtual std::wstring gpt_type(){ return macho::stringutils::convert_utf8_to_unicode(find_value(_obj, "gpt_type").get_str()); }
        virtual std::wstring guid() { return macho::stringutils::convert_utf8_to_unicode(find_value(_obj, "guid").get_str()); }
        virtual bool         is_read_only() { return find_value(_obj, "is_read_only").get_bool(); }
        virtual bool         is_offline() { return find_value(_obj, "is_offline").get_bool(); }
        virtual bool         is_system() { return find_value(_obj, "is_system").get_bool(); }
        virtual bool         is_boot() { return find_value(_obj, "is_boot").get_bool(); }
        virtual bool         is_active() { return find_value(_obj, "is_active").get_bool(); }
        virtual bool         is_hidden() { return find_value(_obj, "is_hidden").get_bool(); }
        virtual bool         is_shadow_copy() { return find_value(_obj, "is_shadow_copy").get_bool(); }
        virtual bool         no_default_drive_letter() { return find_value(_obj, "no_default_drive_letter").get_bool(); }
        virtual uint32_t     set_attributes(bool _is_read_only, bool _no_default_drive_letter, bool _is_active, bool _is_hidden) { return 1;}
    private:
        json_spirit::mObject _obj;
    };

    class json_volume : virtual public volume{
    public:
        json_volume(json_spirit::mObject &obj) : _obj(obj){}
        virtual std::wstring   id() { return macho::stringutils::convert_utf8_to_unicode(find_value(_obj, "id").get_str()); }
        virtual ST_VOLUME_TYPE type() { return ST_VOLUME_TYPE(find_value_int32(_obj, "type", 1)); }
        virtual std::wstring   drive_letter() { return macho::stringutils::convert_utf8_to_unicode(find_value(_obj, "drive_letter").get_str()); }
        virtual std::wstring   path() { return macho::stringutils::convert_utf8_to_unicode(find_value(_obj, "path").get_str()); }
        virtual uint16_t       health_status() { return find_value(_obj, "health_status").get_int(); }
        virtual std::wstring   file_system(){ return macho::stringutils::convert_utf8_to_unicode(find_value(_obj, "file_system").get_str()); }
        virtual std::wstring   file_system_label() { return macho::stringutils::convert_utf8_to_unicode(find_value(_obj, "file_system_label").get_str()); }
        virtual uint64_t       size() { return find_value(_obj, "size").get_uint64(); }
        virtual uint64_t       size_remaining() { return find_value(_obj, "size_remaining").get_uint64(); }
        virtual ST_DRIVE_TYPE  drive_type() { return (ST_DRIVE_TYPE)find_value(_obj, "drive_type").get_int(); }
        virtual string_array_w access_paths(){
            string_array_w paths;
            json_spirit::mArray  access_paths = find_value(_obj, "access_paths").get_array();
            foreach(json_spirit::mValue p, access_paths){
                paths.push_back(macho::stringutils::convert_utf8_to_unicode(p.get_str()));
            }
            return paths;
        }
        virtual bool           mount() { return false; }
        virtual bool           dismount(bool force = false, bool permanent = false) { return false; }
        virtual bool           format(
            std::wstring file_system,
            std::wstring file_system_label,
            uint32_t allocation_unit_size,
            bool full = false,
            bool force = false,
            bool compress = false,
            bool short_file_name_support = false,
            bool set_integrity_streams = false,
            bool use_large_frs = false,
            bool disable_heat_gathering = false) {
            return false;
        }
    private:
        json_spirit::mObject _obj;
    };

public:
    json_storage(disk::vtr disks, partition::vtr partitions, volume::vtr volumes) : _disks(disks), _partitions(partitions), _volumes(volumes){}
    json_storage(){}
    static storage::ptr get(boost::filesystem::path file){
        storage::ptr stg;
        if (boost::filesystem::exists(file)){
            stg = storage::ptr(new json_storage());
            disk::vtr      mdisks;
            partition::vtr mpartitions;
            volume::vtr    mvolumes;
            std::ifstream is(file.string());
            json_spirit::mValue storage;
            json_spirit::read(is, storage);
            json_spirit::mArray disks = find_value(storage.get_obj(), "disks").get_array();
            json_spirit::mArray partitions = find_value(storage.get_obj(), "partitions").get_array();
            json_spirit::mArray volumes = find_value(storage.get_obj(), "volumes").get_array();
            foreach(json_spirit::mValue &d, disks){
                mdisks.push_back(disk::ptr(new json_disk(d.get_obj(), stg)));
            }
            foreach(json_spirit::mValue &p, partitions){
                mpartitions.push_back(partition::ptr(new json_partition(p.get_obj())));
            }
            foreach(json_spirit::mValue &v, volumes){
                mvolumes.push_back(volume::ptr(new json_volume(v.get_obj())));
            }
            json_storage * s = dynamic_cast<json_storage*>(stg.get());
            s->_disks = mdisks;
            s->_partitions = mpartitions;
            s->_volumes = mvolumes;
        }
        return stg;
    }

    virtual bool save(boost::filesystem::path file){
        try{
            json_spirit::mObject storage;  
            json_spirit::mArray disks;
            foreach(disk::ptr &d, _disks){
                json_spirit::mObject o;
                o["unique_id"] = macho::stringutils::convert_unicode_to_utf8(d->unique_id());
                o["unique_id_format"] = d->unique_id_format();
                o["path"] = macho::stringutils::convert_unicode_to_utf8(d->path());
                o["location"] = macho::stringutils::convert_unicode_to_utf8(d->location());
                o["friendly_name"] = macho::stringutils::convert_unicode_to_utf8(d->friendly_name());
                o["number"] = (int)d->number();
                o["serial_number"] = macho::stringutils::convert_unicode_to_utf8(d->serial_number());
                o["firmware_version"] = macho::stringutils::convert_unicode_to_utf8(d->firmware_version());
                o["manufacturer"] = macho::stringutils::convert_unicode_to_utf8(d->manufacturer());
                o["model"] = macho::stringutils::convert_unicode_to_utf8(d->model());
                o["size"] = d->size();
                o["allocated_size"] = d->allocated_size();
                o["logical_sector_size"] = (int)d->logical_sector_size();
                o["physical_sector_size"] = (int)d->physical_sector_size();
                o["largest_free_extent"] = d->largest_free_extent();
                o["number_of_partitions"] = (int)d->number_of_partitions();
                o["health_status"] = d->health_status();
                o["bus_type"] = d->bus_type();
                o["partition_style"] = d->partition_style();
                o["signature"] = (int)d->signature();
                o["guid"] = macho::stringutils::convert_unicode_to_utf8(d->guid());
                o["is_offline"] = d->is_offline();
                o["offline_reason"] = d->offline_reason();
                o["is_read_only"] = d->is_read_only();
                o["is_system"] = d->is_system();
                o["is_clustered"] = d->is_clustered();
                o["is_boot"] = d->is_boot();
                o["boot_from_disk"] = d->boot_from_disk();
                o["sectors_per_track"] = (int)d->sectors_per_track();
                o["tracks_per_cylinder"] = (int)d->tracks_per_cylinder();
                o["total_cylinders"] = d->total_cylinders();
                o["scsi_bus"] = (int)d->scsi_bus();
                o["scsi_logical_unit"] = d->scsi_logical_unit();
                o["scsi_port"] = d->scsi_port();
                o["scsi_target_id"] = d->scsi_target_id();
                disks.push_back(o);
            }
            storage["disks"] = disks;
            json_spirit::mArray partitions;
            foreach(partition::ptr &p, _partitions){
                json_spirit::mObject o;
                o["disk_number"] = (int)p->disk_number();
                o["partition_number"] = (int)p->partition_number();
                o["path"] = macho::stringutils::convert_unicode_to_utf8(p->drive_letter());
                string_array_w _access_paths = p->access_paths();
                string_array_a _paths;
                foreach(std::wstring s, _access_paths){
                    _paths.push_back(macho::stringutils::convert_unicode_to_utf8(s));
                }
                json_spirit::mArray  access_paths(_paths.begin(), _paths.end());
                o["access_paths"] = access_paths;
                o["offset"] = p->offset();
                o["size"] = p->size();
                o["mbr_type"] = p->mbr_type();
                o["gpt_type"] = macho::stringutils::convert_unicode_to_utf8(p->gpt_type());
                o["guid"] = macho::stringutils::convert_unicode_to_utf8(p->guid());
                o["is_read_only"] = p->is_read_only();
                o["is_offline"] = p->is_offline();
                o["is_system"] = p->is_system();
                o["is_boot"] = p->is_boot();
                o["is_active"] = p->is_active();
                o["is_hidden"] = p->is_hidden();
                o["is_shadow_copy"] = p->is_shadow_copy();
                o["no_default_drive_letter"] = p->no_default_drive_letter();
                partitions.push_back(o);
            }
            storage["partitions"] = partitions;
            json_spirit::mArray volumes;

            foreach(volume::ptr &v, _volumes){
                json_spirit::mObject o;
                o["id"] = macho::stringutils::convert_unicode_to_utf8(v->id());
                o["type"] = v->type();
                o["drive_letter"] = macho::stringutils::convert_unicode_to_utf8(v->drive_letter());
                o["path"] = macho::stringutils::convert_unicode_to_utf8(v->path());
                o["health_status"] = v->health_status();
                o["file_system"] = macho::stringutils::convert_unicode_to_utf8(v->file_system());
                o["file_system_label"] = macho::stringutils::convert_unicode_to_utf8(v->file_system_label());
                o["size"] = v->size();
                o["size_remaining"] = v->size_remaining();
                o["drive_type"] = v->drive_type();
                string_array_w _access_paths = v->access_paths();
                string_array_a _paths;
                foreach(std::wstring s, _access_paths){
                    _paths.push_back(macho::stringutils::convert_unicode_to_utf8(s));
                }
                json_spirit::mArray  access_paths(_paths.begin(), _paths.end());
                o["access_paths"] = access_paths;
                volumes.push_back(o);
            }
            storage["volumes"] = volumes;
            std::ofstream output(file.string(), std::ios::out | std::ios::trunc);
            std::locale loc(std::locale::classic(), new std::codecvt_utf8<wchar_t>);
            output.imbue(loc);
            json_spirit::write(storage, output, json_spirit::pretty_print | json_spirit::raw_utf8);
            return true;
        }
        catch (boost::exception& ex){
            LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_ansi_to_unicode(boost::exception_detail::get_diagnostic_information(ex, "can't output storage info.")).c_str());
        }
        catch (...){
        }
        return false;
    }

    virtual ~json_storage(){}
    virtual void rescan(){}
    virtual disk::vtr get_disks() { return _disks; }
    virtual volume::vtr get_volumes() { return _volumes; }
    virtual partition::vtr get_partitions() { return _partitions; }
    virtual disk::ptr get_disk(uint32_t disk_number) {
        foreach(disk::ptr &d, _disks){
            if (d->number() == disk_number)
                return d;
        }
        return disk::ptr();
    }
    
    virtual volume::vtr get_volumes(uint32_t disk_number) {
        volume::vtr volumes;
        foreach(volume::ptr &v, _volumes){
            bool found = false;
            foreach(partition::ptr &p, _partitions){
                foreach(std::wstring s, p->access_paths()){
                    if (std::wstring::npos != s.find(v->path())){
                        if (p->disk_number() == disk_number){
                            volumes.push_back(v);
                            found = true;
                            break;
                        }
                    }
                }
                if (found)
                    break;
            }
        }
        return volumes;
    }

    virtual partition::vtr get_partitions(uint32_t disk_number){
        partition::vtr partitions;
        foreach(partition::ptr &p, _partitions){
            if (p->disk_number() == disk_number){
                partitions.push_back(p);
            }
        }
        return partitions;
    }
private:
    disk::vtr       _disks;
    partition::vtr  _partitions;
    volume::vtr     _volumes;
};

};};

#endif