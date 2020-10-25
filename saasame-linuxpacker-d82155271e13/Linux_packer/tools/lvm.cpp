#include "linuxfs_parser.h"//this is dirty code, for void hested including
#include "lvm.h"
#include "log.h"
#include "snapshot.h"
#include "ext2fs.h"
#include <boost/regex.hpp>
#include <unistd.h>
//#include "sys/swap.h"
//#define SWAP_HEADER_SIZE (sizeof(union swap_header_v1_2))

#define remove_file_test 0
extern bool get_physicak_location_of_specify_file_by_command(std::vector<std::pair<uint64_t, uint64_t>> & inextents, const set<string> & filenames, const string & block_device, uint32_t block_size, bool is_xfs, linuxfs::volume::ptr v);

void volume_group::replace_all(std::string& str, const std::string& from, const std::string& to) {
    if (from.empty())
        return;
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
    }
}

physical_volume::ptr volume_group::find_physical_volume(std::string &_id) {
    std::string __id = _id;
    replace_all(__id, "-", "");
    auto it = std::find_if(pvolumes.begin(), pvolumes.end(), [&__id](const physical_volume::ptr& pv) { return pv->id == __id; });
    if (it != pvolumes.end())
        return *it;
    return NULL;
}

physical_volume::ptr volume_group::add_physical_volume(std::string &_name, std::string &_device, std::string &_id, uint64_t devsize, uint32_t start, uint32_t count) {
    std::string __id = _id;
    replace_all(__id, "-", "");
    auto it = std::find_if(pvolumes.begin(), pvolumes.end(), [&__id](const physical_volume::ptr& pv) { return pv->id == __id; });
    if (it != pvolumes.end()) {
        return *it;
    }
    else {
        physical_volume::ptr pv = physical_volume::ptr(new physical_volume());
        pv->id = __id;
        pv->device = _device;
        pv->dev_size = devsize;
        pv->pe_start = start;
        pv->pe_count = count;
        pv->name = _name;
        pvolumes.push_back(pv);
        return pv;
    }
}

logical_volume::ptr volume_group::find_logical_volume(std::string &_id) {
    std::string __id = _id;
    replace_all(__id, "-", "");
    auto it = std::find_if(lvolumes.begin(), lvolumes.end(), [&__id](const logical_volume::ptr& lv) { return lv->id == __id; });
    if (it != lvolumes.end())
        return *it;
    return NULL;
}

logical_volume::ptr volume_group::add_logical_volume(std::string &_id, int count, std::string &vname) {
    std::string __id = _id;
    replace_all(__id, "-", "");
    auto it = std::find_if(lvolumes.begin(), lvolumes.end(), [&__id](const logical_volume::ptr& lv) { return lv->id == __id; });
    if (it != lvolumes.end())
        return *it;
    else {
        logical_volume::ptr lv = logical_volume::ptr(new logical_volume(__id, count, vname, extent_size, this));
        lv->vg_volname = vgname + '-' + lv->volname;
        lvolumes.push_back(lv);
        return lv;
    }
}

volume_group::ptr logical_volume_manager::find_volgroup(std::string &id) {
    auto it = std::find_if(volume_groups.begin(), volume_groups.end(), [&id](const volume_group::ptr& vg) { return vg->id == id; });
    if (it != volume_groups.end())
        return *it;
    return NULL;
}
volume_group::ptr logical_volume_manager::add_volgroup(std::string &id, std::string &name, int seq, int size) {
    volume_group::ptr vg = find_volgroup(id);
    if (!vg) {
        vg = volume_group::ptr(new volume_group(id, name, seq, size));
        volume_groups.push_back(vg);
    }
    return vg;
}

bool logical_volume_manager::scan_pv(universal_disk_rw::ptr _rw, uint64_t _offset, bool b_full) {
    FUNC_TRACER;
    LOG_TRACE("rw.path = %s, offset = % llu", _rw->path().c_str(), _offset);
    lvm_metadata meta(_rw, _offset);
    PV_LABEL_HEADER *header;
    PV_LABEL *label;
    uint64_t offset;
    int length;
    int sector_size = _rw->sector_size();
    std::unique_ptr<char>  buffer = std::unique_ptr<char>(new char[sector_size]);
    std::unique_ptr<char>  metadata;
    uint32_t number_of_bytes_read = 0;
    for (int i = 0; i < 4; i++)
    {
        offset = _offset + (i * _rw->sector_size());
        if (!_rw->read(offset, sector_size, buffer.get(), number_of_bytes_read))
            break;
        header = (PV_LABEL_HEADER *)&buffer.get()[0];
        LOG_TRACE("header.pv_name = %s", header->pv_name);
        LOG_TRACE("header.pv_sector_xl = %llu", header->pv_sector_xl);
        LOG_TRACE("header.pv_crc = %u", header->pv_crc);
        LOG_TRACE("header.pv_offset_xl = %u", header->pv_offset_xl);
        LOG_TRACE("header.pv_vermagic = %s", header->pv_vermagic);
        LOG_TRACE("header.pv_uuid = %s", header->pv_uuid);
        LOG_TRACE("header.pv_size = %llu", header->pv_size);
        LOG_TRACE("header.pv_labeloffset = %llu", header->pv_labeloffset);
        LOG_TRACE("header.pv_labelsize = %llu", header->pv_labelsize);
        PDATA_AREA_DESCRIPTOR da = (PDATA_AREA_DESCRIPTOR)header->pv_unknown1;
        LOG_TRACE("da[0].offset = %llu", da[0].pv_data_area_offset);
        LOG_TRACE("da[0].pv_data_area_size = %llu", da[0].pv_data_area_size);
        LOG_TRACE("da[1].offset = %llu", da[1].pv_data_area_offset);
        LOG_TRACE("da[1].pv_data_area_size = %llu", da[1].pv_data_area_size);

        if (strncmp(header->pv_name, LVM_SIG, LVM_SIGLEN) != 0)
        {
            LOG_TRACE("(strncmp(header->pv_name, LVM_SIG, LVM_SIGLEN) != 0");
            continue;
            //LOG("Invalid label. The partition is not LVM2 volume\n");
            //return -1;
        }
        meta.pv_label_hdr_addr = offset;
        meta.pv_label_hdr_len = sector_size;
        meta.pv_id = std::string(reinterpret_cast<char const*>(header->pv_uuid), sizeof(header->pv_uuid));
        LOG_TRACE("header->pv_labeloffset = %llu", header->pv_labeloffset);
        if (header->pv_labeloffset > 0) {
            meta.pv_label_addr = offset = header->pv_labeloffset + _offset;
            meta.pv_label_len = sector_size;
            if (!_rw->read(offset, sector_size, buffer.get(), number_of_bytes_read))
            {
                LOG_TRACE("pv read error");
                break;
            }
            label = (PV_LABEL *)&buffer.get()[0];
            if (label->pv_length > 0) {
                LOG_TRACE("label->pv_length = %llu", label->pv_length);
                LOG_TRACE("label->unknown2 = %llu", label->unknown2);
                LOG_TRACE("label->pv_offset_low = %llu", label->pv_offset_low);
                LOG_TRACE("label->pv_offset_high = %llu", label->pv_offset_high);
                meta.vg_meta_addr = offset = _offset + ((label->pv_offset_low + label->pv_offset_high));
                meta.vg_meta_len = length = ((label->pv_length / sector_size) + 1) * sector_size;
                metadata = std::unique_ptr<char>(new char[length]);
                memset(metadata.get(), 0, length);
                if (_rw->read(offset, length, metadata.get(), number_of_bytes_read)) {
                    metadata.get()[label->pv_length] = 0;
                    meta.pv_metadata = std::string(reinterpret_cast<char const*>(metadata.get()), label->pv_length);
                }
                else {
                    break;
                }
            }
            else
            {
                LOG_ERROR("pv_length == 0,ERROR happen.");
                continue;
            }
        }
        metadatas.push_back(meta);
        if (meta.pv_metadata.empty())
        {
            LOG_TRACE("FALSE");
            return false;
        }
        else if(parse_metadata(meta.pv_metadata, b_full))
        {

            LOG_TRACE("TRUE");
            return true;
        }
        break;
    }
    LOG_TRACE("FALSE");
    return false;
}

bool logical_volume_manager::parse_metadata(std::string pv_metadata,bool b_full) {
    int				num = 0;
    int             vg_seq = 0;
    int             extent_size = 0;
    LOG_TRACE("pv_metadata = %s\r\n", pv_metadata.c_str());
    //std::regex	lvm_uuid("[a-zA-Z0-9]*-{1,}[a-zA-Z0-9]*", std::regex_constants::icase);
    //std::regex  lvm_pvid("pv[0-9\\s\\t]+\\{", std::regex_constants::icase);
    //std::regex  lvm_lv_name("[a-zA-Z_0-9\\s\\t]+\\{", std::regex_constants::icase);
    //std::regex  lvm_brace("[\\s\\t]+\\{", std::regex_constants::icase);
    //std::regex	lvm_pv_extent_size("extent_size");
    //std::regex	lvm_physical_volumes("physical_volumes");
    //std::regex	lvm_pv_dev_size("dev_size");
    //std::regex  lvm_pe_start("pe_start");
    //std::regex  lvm_device("device");
    //std::regex	lvm_pe_count("pe_count");
    //std::regex	lvm_logical_volumes("logical_volumes");
    //std::regex	lvm_stripe_id("pv[0-9]+"/*, std::regex_constants::basic*/);
    //std::regex  lvm_seg_count("segment_count");
    //std::regex  lvm_seg_id("segment[0-9]+"/*, std::regex_constants::basic*/);
    //std::regex  lvm_start_extent("start_extent");
    //std::regex  lvm_extent_count("extent_count");
    //std::regex  lvm_type("type");
    //std::regex  lvm_stripe_count("stripe_count");
    //std::regex  lvm_max_lv("max_lv");
    //std::regex  lvm_max_pv("max_pv");
    //std::regex  lvm_lf("\\n");
    //std::regex  lvm_value("[0-9]+"/*, std::regex_constants::basic*/);
    //std::regex  lvm_string("\".+\""/*, std::regex_constants::basic*/);

    boost::regex	lvm_uuid("[a-zA-Z0-9]*-{1,}[a-zA-Z0-9]*", boost::regex::icase);
    boost::regex  lvm_pvid("pv[0-9\\s\\t]+\\{", boost::regex::icase);
    boost::regex  lvm_lv_name("[a-zA-Z_0-9\\s\\t]+\\{", boost::regex::icase);
    boost::regex  lvm_brace("[\\s\\t]+\\{", boost::regex::icase);
    boost::regex	lvm_pv_extent_size("extent_size");
    boost::regex	lvm_physical_volumes("physical_volumes");
    boost::regex	lvm_pv_dev_size("dev_size");
    boost::regex  lvm_pe_start("pe_start");
    boost::regex  lvm_device("device");
    boost::regex	lvm_pe_count("pe_count");
    boost::regex	lvm_logical_volumes("logical_volumes");
    boost::regex	lvm_stripe_id("pv[0-9]+"/*, boost::regex::basic*/);
    boost::regex  lvm_seg_count("segment_count");
    boost::regex  lvm_seg_id("segment[0-9]+"/*, boost::regex::basic*/);
    boost::regex  lvm_start_extent("start_extent");
    boost::regex  lvm_extent_count("extent_count");
    boost::regex  lvm_type("type");
    boost::regex  lvm_stripe_count("stripe_count");
    boost::regex  lvm_max_lv("max_lv");
    boost::regex  lvm_max_pv("max_pv");
    boost::regex  lvm_lf("\\n");
    boost::regex  lvm_value("[0-9]+"/*, boost::regex::basic*/);
    boost::regex  lvm_string("\".+?\""/*, boost::regex::basic*/);

    std::string str = pv_metadata;
    std::string tmp;
    std::string vgVolname;
    std::string vguuid;
    std::string lvVolname;
    num = str.find("{");
    if (num > 0) {
        vgVolname = str.substr(0, num - 1);
    }
    //std::smatch what;
    boost::smatch what;
    tmp = str.substr(num, -1);
    if (/*std::regex_search(tmp, what, lvm_uuid)*/boost::regex_search(tmp, what, lvm_uuid)) {
        num = str.find(what[0]);
        vguuid = str.substr(num, LVM_META_UUID_LEN);
        tmp = str.substr(num + LVM_META_UUID_LEN, -1);
    }
    else {
        LOG_ERROR("error parsing lvm_uuid\n");
        return false;
    }

    if (/*std::regex_search(tmp, what, lvm_value)*/boost::regex_search(tmp, what, lvm_value)) {
        vg_seq = atol(what[0].str().c_str());
        tmp = tmp.substr(tmp.find(what[0]) + what[0].str().size(), -1);
    }
    else {
        LOG_ERROR("error parsing lvm_value\n");
        return false;
    }
    if (/*std::regex_search(tmp, what, lvm_pv_extent_size)*/boost::regex_search(tmp, what, lvm_pv_extent_size)) {
        tmp = tmp.substr(tmp.find(what[0]) + what[0].str().size(), -1);
        if (/*std::regex_search(tmp, what, lvm_value)*/boost::regex_search(tmp, what, lvm_value)) {
            extent_size = atol(what[0].str().c_str());
            tmp = tmp.substr(tmp.find(what[0]) + what[0].str().size(), -1);
        }
    }
    else {
        LOG_ERROR("error parsing lvm_pv_extent_size\n");
        return false;
    }

    volume_group::ptr vg = add_volgroup(vguuid, vgVolname, vg_seq, extent_size);
    vg->b_full = true;
    LOG_TRACE("vg->vgname = %s", vg->vgname.c_str());
    if (!vg) {
        LOG_ERROR("!vg\n");
        return false;
    }
    if (/*std::regex_search(tmp, what, lvm_max_lv)*/boost::regex_search(tmp, what, lvm_max_lv)) {
        tmp = tmp.substr(tmp.find(what[0]) + what[0].str().size(), -1);
        if (/*std::regex_search(tmp, what, lvm_value)*/boost::regex_search(tmp, what, lvm_value)) {
            vg->max_lv = atol(what[0].str().c_str());
            tmp = tmp.substr(tmp.find(what[0]) + what[0].str().size(), -1);
        }
    }

    if (/*std::regex_search(tmp, what, lvm_max_pv)*/boost::regex_search(tmp, what, lvm_max_pv)) {
        tmp = tmp.substr(tmp.find(what[0]) + what[0].str().size(), -1);
        if (/*std::regex_search(tmp, what, lvm_value)*/boost::regex_search(tmp, what, lvm_value)) {
            vg->max_pv = atol(what[0].str().c_str());
            tmp = tmp.substr(tmp.find(what[0]) + what[0].str().size(), -1);
        }
    }

    if (/*std::regex_search(tmp, what, lvm_physical_volumes)*/boost::regex_search(tmp, what, lvm_physical_volumes)) {
        tmp = tmp.substr(tmp.find(what[0]) + what[0].str().size(), -1);
        if (/*std::regex_search(tmp, what, lvm_brace)*/boost::regex_search(tmp, what, lvm_brace)) {
            tmp = tmp.substr(tmp.find(what[0]) + what[0].str().size(), -1);
        }
        while (/*std::regex_search(tmp, what, lvm_pvid)*/boost::regex_search(tmp, what, lvm_pvid)) {
            std::string pvname = what[0];
            pvname = string_op::remove_begining_whitespace(string_op::remove_trailing_whitespace(pvname.substr(0, pvname.find_first_of("{"))));
            tmp = tmp.substr(tmp.find(what[0]) + what[0].str().size(), -1);
            std::string pvid;
            std::string device;
            uint64_t devsize = 0;
            uint32_t start = 0;
            uint32_t count = 0;
            if (/*std::regex_search(tmp, what, lvm_uuid)*/boost::regex_search(tmp, what, lvm_uuid)) {
                num = tmp.find(what[0]);
                pvid = tmp.substr(num, LVM_META_UUID_LEN);
                tmp = tmp.substr(num + LVM_META_UUID_LEN, -1);
            }
            /*to get the device file path*/
            if (/*std::regex_search(tmp, what, lvm_device)*/boost::regex_search(tmp, what, lvm_device)) {
                tmp = tmp.substr(tmp.find(what[0]) + what[0].str().size(), -1);
                if (/*std::regex_search(tmp, what, lvm_string)*/boost::regex_search(tmp, what, lvm_string)) {
                    device = what[0].str();
                    tmp = tmp.substr(tmp.find(what[0]) + what[0].str().size(), -1);
                }
            }
            if (/*std::regex_search(tmp, what, lvm_pv_dev_size)*/boost::regex_search(tmp, what, lvm_pv_dev_size)) {
                tmp = tmp.substr(tmp.find(what[0]) + what[0].str().size(), -1);
                if (/*std::regex_search(tmp, what, lvm_value)*/boost::regex_search(tmp, what, lvm_value)) {
                    devsize = atoll(what[0].str().c_str());
                    tmp = tmp.substr(tmp.find(what[0]) + what[0].str().size(), -1);
                }
            }
            if (/*std::regex_search(tmp, what, lvm_pe_start)*/boost::regex_search(tmp, what, lvm_pe_start)) {
                tmp = tmp.substr(tmp.find(what[0]) + what[0].str().size(), -1);
                if (/*std::regex_search(tmp, what, lvm_value)*/boost::regex_search(tmp, what, lvm_value)) {
                    start = atol(what[0].str().c_str());
                    tmp = tmp.substr(tmp.find(what[0]) + what[0].str().size(), -1);
                }
            }
            if (/*std::regex_search(tmp, what, lvm_pe_count)*/boost::regex_search(tmp, what, lvm_pe_count)) {
                tmp = tmp.substr(tmp.find(what[0]) + what[0].str().size(), -1);
                if (/*std::regex_search(tmp, what, lvm_value)*/boost::regex_search(tmp, what, lvm_value)) {
                    count = atol(what[0].str().c_str());
                    tmp = tmp.substr(tmp.find(what[0]) + what[0].str().size(), -1);
                }
            }
            physical_volume::ptr pv = vg->add_physical_volume(pvname, device, pvid, devsize, start, count);
            LOG_TRACE("pv->name = %s", pv->name.c_str());
        }
    }
    else {
        LOG_ERROR("error parsing lvm_physical_volumes\n");
        return false;
    }

    if (/*std::regex_search(tmp, what, lvm_logical_volumes)*/boost::regex_search(tmp, what, lvm_logical_volumes)) {
        tmp = tmp.substr(tmp.find(what[0]) + what[0].str().size(), -1);
        if (/*std::regex_search(tmp, what, lvm_brace)*/boost::regex_search(tmp, what, lvm_brace)) {
            tmp = tmp.substr(tmp.find(what[0]) + what[0].str().size(), -1);
        }
        while (/*std::regex_search(tmp, what, lvm_lv_name)*/boost::regex_search(tmp, what, lvm_lv_name)) {
            std::string lvid;
            std::string lvname = what[0];
            lvname = string_op::remove_begining_whitespace(string_op::remove_trailing_whitespace(lvname.substr(0, lvname.find("{"))));
            LOG_TRACE("real lvname = %s\r\n", lvname.c_str());
            int nsegs = 0;
            tmp = tmp.substr(tmp.find(what[0]) + what[0].str().size(), -1);
            if (/*std::regex_search(tmp, what, lvm_uuid)*/boost::regex_search(tmp, what, lvm_uuid)) {
                num = tmp.find(what[0]);
                lvid = tmp.substr(num, LVM_META_UUID_LEN);
                tmp = tmp.substr(num + LVM_META_UUID_LEN, -1);
            }
            if (/*std::regex_search(tmp, what, lvm_seg_count)*/boost::regex_search(tmp, what, lvm_seg_count)) {
                tmp = tmp.substr(tmp.find(what[0]) + what[0].str().size(), -1);
                if (/*std::regex_search(tmp, what, lvm_value)*/boost::regex_search(tmp, what, lvm_value)) {
                    nsegs = atol(what[0].str().c_str());
                    tmp = tmp.substr(tmp.find(what[0]) + what[0].str().size(), -1);
                }
            }

            logical_volume::ptr lv = vg->add_logical_volume(lvid, nsegs, lvname);
            lv->b_full = b_full;
            LOG_TRACE("lv->vg_volname = %s", lv->vg_volname.c_str());
            for (int i = 0; i < nsegs; i++)
            {
                if (/*std::regex_search(tmp, what, lvm_seg_id)*/boost::regex_search(tmp, what, lvm_seg_id)) {
                    uint32_t start_extent = 0;
                    uint32_t extent_count = 0;
                    std::string type;
                    if (/*std::regex_search(tmp, what, lvm_start_extent)*/boost::regex_search(tmp, what, lvm_start_extent)) {
                        tmp = tmp.substr(tmp.find(what[0]) + what[0].str().size(), -1);
                        if (/*std::regex_search(tmp, what, lvm_value)*/boost::regex_search(tmp, what, lvm_value)) {
                            start_extent = atol(what[0].str().c_str());
                            tmp = tmp.substr(tmp.find(what[0]) + what[0].str().size(), -1);
                        }
                    }
                    if (/*std::regex_search(tmp, what, lvm_extent_count)*/boost::regex_search(tmp, what, lvm_extent_count)) {
                        tmp = tmp.substr(tmp.find(what[0]) + what[0].str().size(), -1);
                        if (/*std::regex_search(tmp, what, lvm_value)*/boost::regex_search(tmp, what, lvm_value)) {
                            extent_count = atol(what[0].str().c_str());
                            tmp = tmp.substr(tmp.find(what[0]) + what[0].str().size(), -1);
                        }
                    }

                    if (/*std::regex_search(tmp, what, lvm_type)*/boost::regex_search(tmp, what, lvm_type)) {
                        std::string t = tmp.substr(tmp.find(what[0]) + what[0].str().size(), -1);
                        std::vector<std::string> arr = string_op::tokenize2(t, "\n", 2, false);
                        if (arr.size()) {
                            std::vector<std::string> rr = string_op::tokenize2(arr[0], "=", 2);
                            if (rr.size()>0)
                                type = string_op::parser_double_quotation_mark(rr[1]);
                        }
                    }

                    lv_segment seg(start_extent, extent_count, type);
                    uint32_t stripe_count = 0;
                    if (/*std::regex_search(tmp, what, lvm_stripe_count)*/boost::regex_search(tmp, what, lvm_stripe_count)) {
                        tmp = tmp.substr(tmp.find(what[0]) + what[0].str().size(), -1);
                        if (/*std::regex_search(tmp, what, lvm_value)*/boost::regex_search(tmp, what, lvm_value)) {
                            stripe_count = atol(what[0].str().c_str());
                            tmp = tmp.substr(tmp.find(what[0]) + what[0].str().size(), -1);
                        }
                    }
                    for (int stripe = 0; stripe < stripe_count; stripe++) {
                        if (/*std::regex_search(tmp, what, lvm_stripe_id)*/boost::regex_search(tmp, what, lvm_stripe_id)) {
                            std::string pv_name = what[0];
                            tmp = tmp.substr(tmp.find(what[0]) + what[0].str().size(), -1);
                            if (/*std::regex_search(tmp, what, lvm_value)*/boost::regex_search(tmp, what, lvm_value)) {
                                seg.stripes.push_back(lv_stripe(pv_name, atol(what[0].str().c_str())));
                                tmp = tmp.substr(tmp.find(what[0]) + what[0].str().size(), -1);
                            }
                        }
                    }
                    bool found = false;
                    for (lv_segment& s : lv->segments) {
                        if (s.start_extent == seg.start_extent && s.extent_count == seg.extent_count) {
                            found = true;
                            break;
                        }
                    }
                    if (!found)
                        lv->segments.push_back(seg);
                }
            }
        }
    }
    else {
        LOG_ERROR("error parsing lvm_logical_volumes\n");
        return false;
    }
    return true;
}

logical_volume_manager::groups_luns_map logical_volume_manager::get_groups_luns_mapping() {
    logical_volume_manager::groups_luns_map mapping;
    for (volume_group::ptr &vg : volume_groups) {
        for (lvm_metadata& meta : metadatas) {
            std::string path = meta.rw->path();
            physical_volume::ptr pv = vg->find_physical_volume(meta.pv_id);
            if (pv) {
                mapping[vg->vgname].insert(path);
            }
        }
    }
    return mapping;
}

linuxfs::io_range::map logical_volume_manager::get_system_ranges(snapshot_manager::ptr sh, map<string, set<string>> & excluded_paths_map, map<string, set<string>> & resync_paths_map, bool b_full,bool b_filesystem_filter) {
    FUNC_TRACER;
    linuxfs::io_range::map ranges_map;
    logical_volume::vtr lvs;
    //if (b_full)
    //{
        for (volume_group::ptr &vg : volume_groups) {
            for (lvm_metadata& meta : metadatas) {
                std::string path = meta.rw->path();
                if (b_full || vg->b_full)
                {
                    if (meta.pv_label_hdr_len) {
                        LOG_TRACE("pv_label_hdr_len: path = %s\r\nmeta.pv_label_hdr_addr = %llu \r\nmeta.pv_label_hdr_len = %u\r\nmeta.pv_label_hdr_addr=%llu\r\n,meta.rw.path = %s\r\n", path.c_str(), meta.pv_label_hdr_addr, meta.pv_label_hdr_len, meta.pv_label_hdr_addr, meta.rw->path().c_str());
                        ranges_map[path].push_back(linuxfs::io_range(meta.pv_label_hdr_addr, meta.pv_label_hdr_len, meta.pv_label_hdr_addr, meta.rw));
                    }
                    if (meta.pv_label_len) {
                        LOG_TRACE("pv_label_len: path = %s\r\nmeta.pv_label_addr = %llu \r\nmeta.pv_label_len = %u\r\nmeta.pv_label_addr=%llu\r\n,meta.rw.path = %s\r\n", path.c_str(), meta.pv_label_addr, meta.pv_label_len, meta.pv_label_addr, meta.rw->path().c_str());

                        ranges_map[path].push_back(linuxfs::io_range(meta.pv_label_addr, meta.pv_label_len, meta.pv_label_addr, meta.rw));
                    }
                    if (meta.vg_meta_len) {
                        LOG_TRACE("vg_meta_len: path = %s\r\n", path.c_str());
                        LOG_TRACE("meta.vg_meta_addr = %llu \r\n", meta.vg_meta_addr);
                        LOG_TRACE("meta.vg_meta_len = = %u\r\n", meta.vg_meta_len);
                        LOG_TRACE("meta.rw.path = %s\r\n",meta.rw->path().c_str());
                        ranges_map[path].push_back(linuxfs::io_range(meta.vg_meta_addr, meta.vg_meta_len, meta.vg_meta_addr, meta.rw));
                    }
                }
                physical_volume::ptr pv = vg->find_physical_volume(meta.pv_id);
                if (pv) {
                    pv->rw = meta.rw;
                    pv->offset = meta.offset / pv->rw->sector_size();
                }
            }
        }
    //}
    logical_volume::vtr unmounts;
    for (volume_group::ptr &vg : volume_groups) {     
        logical_volume::vtr _lvs = vg->logical_mount(unmounts);
        lvs.insert(lvs.end(), _lvs.begin(), _lvs.end());
    }
    for (logical_volume::ptr lv : lvs) {
        LOG_TRACE("lv->vn = %s\r\n", lv->vg_volname.c_str());
        snapshot_instance::ptr sn = sh->get_snapshot_by_lvm_name(lv->vg_volname, lv->get_vg()->id, lv->id);
        //bool use_copy = true;
        if (sn != NULL)
        {
            lv->rw = sn->get_src_rw();
        }
        universal_disk_rw::ptr rw = logical_volume_rw::get(lv);
        if (rw) {
            linuxfs::io_range::vtr _ranges;
            linuxfs::io_range::vtr datto_ranges;
            linuxfs::io_range::vtr intersection_range;
            linuxfs::io_range::vtr complement_range;
            linuxfs::io_range::vtr union_ranges;
            linuxfs::io_range::vtr excluded, resync;
            linuxfs::volume::ptr v = linuxfs::volume::get(rw, 0, lv->size());
            if (v) {
                if (b_filesystem_filter)
                {
                    storage::ptr str = storage::get_storage();
                    partitionA::ptr matching_parts = str->get_lvm_instance_from_volname(lv->vg_volname);
                    _ranges = v->file_system_ranges(); //file system
                    //_ranges.push_back(linuxfs::io_range(0, matching_parts->blocks));
                    LOG_TRACE("\r\n\r\nrw->path() = %s\r\n", rw->path().c_str());
                    for (auto & io : _ranges)
                    {
                        LOG_TRACE("start = %llu , length = %llu\r\n", io.start, io.length);
                    }
                    LOG_TRACE("End");


                    //ext2_volume::ptr ext2v = boost::static_pointer_cast<ext2_volume>(v);
                    //std::vector<uint64_t> bbs = ext2v->get_all_excluded_bitmap_blocks();
                    //for (auto & bb : bbs)
                    //{
                    //    LOG_TRACE("bb = %llu\r\n", bb);
                    //}


                    //struct stat ss;
                    //stat(sn->get_abs_cow_path().c_str(), &ss);
                    //LOG_TRACE("ss.st_ino = %llu\r\n", ss.st_ino);
                    //ext2_file::ptr cow_file = ext2v->read_inode(ss.st_ino);
                    //std::vector<uint64_t> blocks = cow_file->get_data_blocks();
                    //sort(blocks.begin(), blocks.end());
                    //std::vector<std::pair<uint64_t, uint64_t>> blocks_ranges;
                    //uint64_t previous = 0;
                    //uint64_t start_b = 0;
                    //for (auto & b : blocks)
                    //{
                    //    //LOG_TRACE("b = %llu \r\n", b);
                    //    if (previous == 0 && start_b == 0)
                    //    {
                    //        start_b = b;
                    //        previous = b;
                    //    }
                    //    else
                    //    {
                    //        if (b != previous + 1)
                    //        {
                    //            blocks_ranges.push_back(std::make_pair(start_b, previous));
                    //            start_b = b;
                    //            previous = b;
                    //        }
                    //        else
                    //        {
                    //            previous = b;
                    //        }
                    //    }
                    //}
                    //LOG_TRACE("blocks_ranges.size() = %d\r\n", blocks_ranges.size());
                    //blocks_ranges.push_back(std::make_pair(start_b, previous));
                    //for (auto & br : blocks_ranges)
                    //{
                    //    LOG_TRACE("enter\r\n");
                    //    LOG_TRACE("br.first = %llu , br.second = %llu\r\n", br.first, br.second);
                    //    excluded.push_back(linuxfs::io_range((br.first * v->get_block_size()) , (1 + br.second - br.first) * v->get_block_size(), br.first * v->get_block_size(), rw));
                    //}
#if remove_file_test
                    std::vector<std::pair<uint64_t, uint64_t>> extents;
                    bool is_xfs = (v->get_type() == VOLUME_TYPE_XFS) ? true : false;
                    string cow_path = (!is_xfs) ? "/.snapshot" + std::to_string(sn->get_index()) : sn->get_abs_cow_path();
                    if (get_physicak_location_of_specify_file_by_command(extents, cow_path, sn->get_datto_device(), v->get_block_size(), is_xfs))
                    {
                        for (auto & ex : extents)
                        {
                            LOG_TRACE("start = %llu , length = %llu\r\n", ex.first, ex.second);
                            excluded.push_back(linuxfs::io_range(ex.first, ex.second, ex.first, rw/*r.start, r.length,r.start,rw*/));
                        }
                    }
                    string pre_cow_path = (!is_xfs) ? "/.snapshot" + std::to_string((sn->get_index() == 0) ? 9 : sn->get_index() - 1) : sn->get_previous_cow_path();
                    if (get_physicak_location_of_specify_file_by_command(extents, pre_cow_path, sn->get_datto_device(), v->get_block_size(), is_xfs))
                    {
                        for (auto & ex : extents)
                        {
                            LOG_TRACE("start = %llu , length = %llu\r\n", ex.first, ex.second);
                            excluded.push_back(linuxfs::io_range(ex.first, ex.second, ex.first, rw/*r.start, r.length,r.start,rw*/));
                        }
                    }
                    std::sort(excluded.begin(), excluded.end(), linuxfs::io_range::compare());
#endif
                    universal_disk_rw::ptr src = rw;
                    if (sn != NULL)
                    {
                        LOG_TRACE("sn != NULL\r\n");
#if REMOVE_DATTO_COW
                        std::vector<std::pair<uint64_t, uint64_t>> extents;
                        bool is_xfs = (v->get_type() == VOLUME_TYPE_XFS) ? true : false;
                        if (b_full || lv->b_full)
                        {
                            if (boost::filesystem::exists(boost::filesystem::path(sn->get_abs_cow_path())))
                            {
                                excluded_paths_map[sn->get_mounted_point()->get_mounted_on()].insert(sn->get_abs_cow_path());
                            }
                            if (boost::filesystem::exists(boost::filesystem::path(sn->get_previous_cow_path())))
                            {
                                excluded_paths_map[sn->get_mounted_point()->get_mounted_on()].insert(sn->get_previous_cow_path());
                            }

                            if (get_physicak_location_of_specify_file_by_command(extents, excluded_paths_map[sn->get_mounted_point()->get_mounted_on()], sn->get_datto_device(), v->get_block_size(), is_xfs, v))
                            {
                                for (auto & ex : extents)
                                {
                                    LOG_TRACE("start = %llu , length = %llu\r\n", ex.first, ex.second);

                                    excluded.push_back(linuxfs::io_range(ex.first, ex.second, ex.first, rw/*r.start, r.length,r.start,rw*/));
                                }
                            }
                            extents.clear();
                            std::sort(excluded.begin(), excluded.end(), linuxfs::io_range::compare());
                        }
                        if (sn->get_mounted_point() != NULL && resync_paths_map.count(sn->get_mounted_point()->get_mounted_on()) != 0)
                        {
                            if (get_physicak_location_of_specify_file_by_command(extents, resync_paths_map[sn->get_mounted_point()->get_mounted_on()], sn->get_datto_device(), v->get_block_size(), is_xfs, v))
                            {
                                for (auto & ex : extents)
                                {
                                    resync.push_back(linuxfs::io_range(ex.first, ex.second, ex.first, rw/*r.start, r.length,r.start,rw*/));
                                }
                            }
                        }
                        extents.clear();
                    }
                }
            }
            else {
                LOG_TRACE("unmounts.push_back(lv)");
                unmounts.push_back(lv);
            }
#endif
            if (sn != NULL)
            {
                //use_copy = false;
                universal_disk_rw::ptr src = sn->get_src_rw();
                //_ranges.clear();
                //printf("sn->get_block_device_size() = %llu\r\n", sn->get_block_device_size());
                if (b_full || lv->b_full)
                {
                    LOG_TRACE("b_full\r\n");
                    datto_ranges.push_back(linuxfs::io_range(0, sn->get_block_device_size(),0, src)); //because there are for lvm, no need offset 
                }
                else
                {
                    LOG_TRACE("b_full else\r\n");
                    changed_area::vtr tempca = sn->get_changed_area(b_full);
                    for (auto & ca : tempca)
                    {
                        LOG_TRACE("ca.src_start = %llu, ca.length = %llu, ca.src_start= %llu, src=%s", ca.src_start, ca.length, ca.src_start, src->path().c_str())
                        datto_ranges.push_back(linuxfs::io_range(ca.src_start, ca.length, ca.src_start, src));
                    }
                }
            }
            /*OK let intersrction now!*/
#if REMOVE_DATTO_COW
            if (b_full || lv->b_full)
            {
                if (datto_ranges.empty())
                {
                    LOG_TRACE("datto_ranges.empty()");
                    intersection_range = _ranges;
                }else if (_ranges.empty())
                {
                    LOG_TRACE("datto_ranges.empty()");
                    intersection_range = datto_ranges;
                }
                else
                {
                    LOG_TRACE("datto_ranges.empty() else");
                    system_tools::regions_vectors_intersection<linuxfs::io_range, linuxfs::io_range::vtr, linuxfs::io_range::vtr::iterator>(datto_ranges, _ranges, intersection_range);
                }
            }
            else
            {
                if (datto_ranges.empty())
                    intersection_range = _ranges;
                else
                    system_tools::regions_vectors_intersection<linuxfs::io_range, linuxfs::io_range::vtr, linuxfs::io_range::vtr::iterator>(datto_ranges, _ranges, intersection_range);
            }
            if (resync.empty())
            {
                union_ranges = intersection_range;
            }
            else
            {
                system_tools::regions_vectors_union<linuxfs::io_range, linuxfs::io_range::vtr, linuxfs::io_range::vtr::iterator>(intersection_range, resync, union_ranges);
            }

            if (excluded.empty())
            {
                LOG_TRACE("excluded.empty()");
                complement_range = union_ranges;
            }
            else
            {
                LOG_TRACE("excluded.empty() else");
                system_tools::regions_vectors_complement<linuxfs::io_range, linuxfs::io_range::vtr, linuxfs::io_range::vtr::iterator>(union_ranges, excluded, complement_range);
            }

#else
#if remove_file_test
            system_tools::regions_vectors_complement<linuxfs::io_range, linuxfs::io_range::vtr, linuxfs::io_range::vtr::iterator>(_ranges, excluded, complement_range);
            system_tools::regions_vectors_intersection<linuxfs::io_range, linuxfs::io_range::vtr, linuxfs::io_range::vtr::iterator>(datto_ranges, complement_range, intersection_range);
#else
            system_tools::regions_vectors_intersection<linuxfs::io_range, linuxfs::io_range::vtr, linuxfs::io_range::vtr::iterator>(datto_ranges, _ranges, intersection_range);
#endif
#endif
                /*we should get the change area and do and in these*/
                /*first just use the full*/
            if (/*_ranges*/complement_range.size()) {
                logical_volume_rw* lv_rw = dynamic_cast<logical_volume_rw*>(rw.get());
                if (lv_rw) {
                    linuxfs::io_range::map _map;
                    /*if(use_copy)
                        _map = lv_rw->get_ranges_mapper(complement_range, src);
                    else*/
                    _map = lv_rw->get_ranges_mapper(complement_range, sn->get_src_rw());
                    for (linuxfs::io_range::map::value_type &m : _map) {
                        /*LOG_TRACE("m.first.c_str() = %s\r\n", m.first.c_str());
                        for (auto & v : m.second)
                        {
                            LOG_TRACE("v.start = %llu, v.src_start = %llu, v.length = %llu, v._rw->path().c_str() = %s\r\n", v.start, v.src_start, v.length, v._rw->path().c_str());
                        }*/
                        ranges_map[m.first].insert(ranges_map[m.first].end(), m.second.begin(), m.second.end());
                    }
                }
            }
        }
    }

    /*std::regex  lvm_swap("swap_[0-9]+", std::regex_constants::icase);
    std::smatch what;*/
    boost::regex  lvm_swap("swap", boost::regex::icase);
    boost::smatch what;
    for (auto & u : unmounts)
    {
        LOG_TRACE("u->volname = %s", u->volname.c_str());
    }

    for (logical_volume::ptr lv : unmounts) {
        if (b_full || lv->b_full || sh->get_btrfs_snapshot_by_lvm_name(lv->vg_volname, lv->get_vg()->id, lv->id)!=NULL)
        {
            for (lv_segment &seg : lv->segments) {
                uint64_t seg_start_sector = (uint64_t)seg.start_extent * lv->extent_size;
                uint64_t seg_length = (uint64_t)(seg.extent_count) * lv->extent_size;
                for (lv_stripe &stripe : seg.stripes) {
                    physical_volume::ptr pv = stripe.pv;
                    uint32_t stripe_start = stripe.start_extent * lv->extent_size;
                    if (pv && pv->rw) {
                        uint64_t start = (pv->offset + pv->pe_start + stripe_start + seg_start_sector) * SECTOR_SIZE;
                        LOG_TRACE("umount start=%llu, seg_length = %llu", start, seg_length* SECTOR_SIZE);
                        if (/*std::regex_search(lv->volname, what, lvm_swap)*/boost::regex_search(lv->volname, what, lvm_swap)) {
                            LOG_TRACE("found swap!");
                            //continue;
                            ranges_map[pv->rw->path()].push_back(linuxfs::io_range(start, /*SECTOR_SIZE*/getpagesize(), start, pv->rw));
                        }
                        else
                        {
                            ranges_map[pv->rw->path()].push_back(linuxfs::io_range(start, seg_length* SECTOR_SIZE, start, pv->rw));
                        }
                    }
                }
            }
        }
    }
    //for(linuxfs::io_range::map::value_type& m : ranges_map){
    //    std::sort(m.second.begin(), m.second.end(), linuxfs::io_range::compare());
    //}
    return ranges_map;
}

logical_volume::vtr volume_group::logical_mount(logical_volume::vtr& unmounts) {
    FUNC_TRACER;
    bool result = true;
    logical_volume::vtr lvs;
    for (logical_volume::ptr &lv : lvolumes) {
        LOG_TRACE("MUMI-10 lv->volname = %s, lv->vg_volname = %s", lv->volname.c_str(), lv->vg_volname.c_str());
        bool valid = lv->segments.size() > 0;
        for (lv_segment &seg : lv->segments) {
            valid = seg.stripes.size() > 0;
            for (lv_stripe &stripe : seg.stripes) {
                auto it = std::find_if(pvolumes.begin(), pvolumes.end(), [&stripe](const physical_volume::ptr& pv) { return pv->name == stripe.pv_name; });
                if (it != pvolumes.end()) {
                    LOG_TRACE("MUMI 1");
                    stripe.pv = *it;
                }
            }
            valid = false;
            for (lv_stripe &stripe : seg.stripes) {
                if (stripe.pv && stripe.pv->rw) {
                    LOG_TRACE("MUMI 2");
                    valid = true;
                    break;
                }
            }
            if (!valid)
                break;
        }
        if (valid)
        {
            LOG_TRACE("MUMI0 lv->volname = %s, lv->vg_volname = %s", lv->volname.c_str(), lv->vg_volname.c_str());
            lvs.push_back(lv);
        }
        else
        {
            LOG_TRACE("MUMI2 lv->volname = %s, lv->vg_volname = %s", lv->volname.c_str(), lv->vg_volname.c_str());
            unmounts.push_back(lv);
        }
    }
    return lvs;
}

bool logical_volume_rw::read(__in uint64_t start, __in uint32_t number_of_bytes_to_read, __inout LPVOID buffer, __inout uint32_t& number_of_bytes_read) {
    bool result = false;
    if (0 == start % _sector_size) {
        uint32_t number_of_sector_read = 0;
        uint32_t number_of_sectors_to_read = number_of_bytes_to_read / _sector_size;
        if (number_of_bytes_to_read % _sector_size > 0) {
            number_of_sectors_to_read++;
            std::unique_ptr<BYTE> buff = std::unique_ptr<BYTE>(new BYTE[number_of_sectors_to_read*_sector_size]);
            if (NULL != buff.get()) {
                if (result = sector_read(start / _sector_size, number_of_sectors_to_read, buff.get(), number_of_sector_read)) {
                    memcpy(buffer, buff.get(), number_of_bytes_to_read);
                    number_of_bytes_read = number_of_bytes_to_read;
                }
            }
        }
        else {
            result = sector_read(start / _sector_size, number_of_bytes_to_read / _sector_size, buffer, number_of_sector_read);
            if (result)
                number_of_bytes_read = number_of_sector_read * _sector_size;
        }
    }
    return result;
}

bool logical_volume_rw::sector_read(__in uint64_t start_sector, __in uint32_t number_of_sectors_to_read, __inout void *buffer, __inout uint32_t& number_of_sectors_read) {
    bool result = false;
    number_of_sectors_read = 0;
    uint64_t end_sec = start_sector + number_of_sectors_to_read;
    uint64_t start_sec = start_sector;
    uint32_t sectors_to_read = number_of_sectors_to_read;
    uint32_t offset = 0;
    if (_lv->rw == NULL)
    {
        std::sort(_lv->segments.begin(), _lv->segments.end(), lv_segment::compare());
        for (lv_segment &seg : _lv->segments) {
            uint64_t seg_start_sector = (uint64_t)seg.start_extent * _lv->extent_size;
            uint64_t seg_end_sector = (uint64_t)((uint64_t)seg.start_extent + seg.extent_count) * _lv->extent_size;
            uint32_t sectors_read = 0;
            if (start_sec >= seg_start_sector && start_sec < seg_end_sector) {
                if (end_sec > seg_end_sector) {
                    for (lv_stripe &stripe : seg.stripes) {
                        physical_volume::ptr pv = stripe.pv;
                        uint32_t stripe_start = stripe.start_extent * _lv->extent_size;
                        if (pv && pv->rw) {
                            if (result = pv->rw->sector_read(pv->offset + pv->pe_start + stripe_start + start_sec - seg_start_sector, seg_end_sector - start_sec, &(((BYTE*)buffer)[offset]), sectors_read)) {
                                number_of_sectors_read += sectors_read;
                                offset += (sectors_read * _sector_size);
                                start_sec = seg_end_sector;
                            }
                            break;
                        }
                    }
                    if (!result)
                        break;
                }
                else {
                    for (lv_stripe &stripe : seg.stripes) {
                        physical_volume::ptr pv = stripe.pv;
                        uint32_t stripe_start = stripe.start_extent * _lv->extent_size;
                        if (pv && pv->rw) {
                            LOG_TRACE("pv->offset = %llu ,pv->pe_start = %u, stripe_start = %u, start_sec = %llu, end_sec = %llu\r\n", pv->offset, pv->pe_start, stripe_start, start_sec, end_sec);
                            if (result = pv->rw->sector_read(pv->offset + pv->pe_start + stripe_start + start_sec - seg_start_sector, end_sec - start_sec, &(((BYTE*)buffer)[offset]), sectors_read)) {
                                number_of_sectors_read += sectors_read;
                            }
                            break;
                        }
                    }
                    break;
                }
            }
            else if (seg_start_sector > end_sec)
                break;
        }
    }
    else
        result = _lv->rw->sector_read(start_sector, number_of_sectors_to_read, buffer, number_of_sectors_read);
    return result;
}

linuxfs::io_range::map logical_volume_rw::get_ranges_mapper(const linuxfs::io_range::vtr &ranges, universal_disk_rw::ptr _rw) {
    linuxfs::io_range::map ranges_map;
    for (const linuxfs::io_range &r : ranges) {
        linuxfs::io_range::map _map = mapper(r.start, r.length,_rw);
        for (linuxfs::io_range::map::value_type &m : _map) {
            ranges_map[m.first].insert(ranges_map[m.first].end(), m.second.begin(), m.second.end());
        }
    }
    return ranges_map;
}

/*linuxfs::io_range::map logical_volume_rw::get_ranges_mapper_reverse(const linuxfs::io_range::vtr &ranges) {
    linuxfs::io_range::map ranges_map_reverse;
    for (const linuxfs::io_range &r : ranges) {
        linuxfs::io_range::map _map = mapper(r.start, r.length);
        for (linuxfs::io_range::map::value_type &m : _map) {
            ranges_map_reverse[m.first].insert(ranges_map_reverse[m.first].end(), m.second.begin(), m.second.end());
        }
    }
    return ranges_map_reverse;
}*/


linuxfs::io_range::map logical_volume_rw::mapper(__in uint64_t start, __in uint64_t length, universal_disk_rw::ptr _rw) {
    linuxfs::io_range::map ranges_map;
    uint64_t _end = start + length;
    uint64_t _start = start;
    std::sort(_lv->segments.begin(), _lv->segments.end(), lv_segment::compare());
    for (lv_segment &seg : _lv->segments) {
        uint64_t seg_start = (uint64_t)seg.start_extent * _lv->extent_size * _sector_size;
        uint64_t seg_end = (uint64_t)((uint64_t)seg.start_extent + seg.extent_count) * _lv->extent_size * _sector_size;
        if (_start >= seg_start && _start < seg_end) {
            if (_end > seg_end) {
                for (lv_stripe &stripe : seg.stripes) {
                    physical_volume::ptr pv = stripe.pv;
                    uint32_t stripe_start = stripe.start_extent * _lv->extent_size;
                    if (pv && pv->rw) {
                        ranges_map[pv->rw->path()].push_back(linuxfs::io_range(((pv->offset + pv->pe_start + stripe_start) * _sector_size) + (_start - seg_start), seg_end - _start, _start, _rw));
                    }
                }
                _start = seg_end;
            }
            else {
                for (lv_stripe &stripe : seg.stripes) {
                    physical_volume::ptr pv = stripe.pv;
                    uint32_t stripe_start = stripe.start_extent * _lv->extent_size;
                    if (pv && pv->rw) {
                        ranges_map[pv->rw->path()].push_back(linuxfs::io_range(((pv->offset + pv->pe_start + stripe_start) * _sector_size) + (_start - seg_start), _end - _start, _start, _rw));
                    }
                }
                break;
            }
        }
        else if (seg_start > _end)
            break;
    }
    return ranges_map;
}
/*what's the reverse mapper doing*/
/*transfer the disk io range to lvm volume io range for the datto to use*/
/*first find the */
/*linuxfs::io_range::map logical_volume_rw::mapper_reverse(__in uint64_t start, __in uint64_t length) {
    linuxfs::io_range::map ranges_map;
    uint64_t _end = start + length;
    uint64_t _start = start;
    std::sort(_lv->segments.begin(), _lv->segments.end(), lv_segment::compare, ());
    for (lv_segment &seg : _lv->segments) {
        uint64_t seg_start = (uint64_t)seg.start_extent * _lv->extent_size * _sector_size;
        uint64_t seg_end = (uint64_t)((uint64_t)seg.start_extent + seg.extent_count) * _lv->extent_size * _sector_size;
        if (_start >= seg_start && _start < seg_end) {
            if (_end > seg_end) {
                for (lv_stripe &stripe : seg.stripes) {
                    physical_volume::ptr pv = stripe.pv;
                    uint32_t stripe_start = stripe.start_extent * _lv->extent_size;
                    if (pv && pv->rw) {
                        ranges_map[pv->rw->path()].push_back(linuxfs::io_range(((pv->offset + pv->pe_start + stripe_start) * _sector_size) + (_start - seg_start), seg_end - _start));
                    }
                }
                _start = seg_end;
            }
            else {
                for (lv_stripe &stripe : seg.stripes) {
                    physical_volume::ptr pv = stripe.pv;
                    uint32_t stripe_start = stripe.start_extent * _lv->extent_size;
                    if (pv && pv->rw) {
                        ranges_map[pv->rw->path()].push_back(linuxfs::io_range(((pv->offset + pv->pe_start + stripe_start) * _sector_size) + (_start - seg_start), _end - _start));
                    }
                }
                break;
            }
        }
        else if (seg_start > _end)
            break;
    }
    return ranges_map;
}*/