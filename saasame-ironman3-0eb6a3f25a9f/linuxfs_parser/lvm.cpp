#include "lvm.h"
#include <regex>

#define LVM_LABEL_HDR_UUID_LEN  32
#define LVM_META_UUID_LEN       38
#define LVM_SIG                 "LABELONE"
#define LVM_VERMAGIC            "LVM2 001"
#define LVM_LABEL_SIG           "LVM2"

bool logical_volume_manager::scan_pv(universal_disk_rw::ptr _rw, uint64_t _offset){
    lvm_metadata meta(_rw, _offset);
    PV_LABEL_HEADER *header;
    PV_LABEL *label;
    uint64_t offset;
    int length;
    int sector_size = _rw->sector_size();
    std::auto_ptr<char>  buffer = std::auto_ptr<char>(new char[sector_size]);
    std::auto_ptr<char>  metadata;
    uint32_t number_of_bytes_read = 0;
    for (int i = 0; i < 4; i++)
    {
        offset = _offset + (i * _rw->sector_size());
        if (!_rw->read(offset, sector_size, buffer.get(), number_of_bytes_read))
            break;
        header = (PV_LABEL_HEADER *)&buffer.get()[0];
        if (strncmp(header->pv_name, LVM_SIG, LVM_SIGLEN) != 0)
        {
            continue;
            //LOG("Invalid label. The partition is not LVM2 volume\n");
            //return -1;
        }
        meta.pv_label_hdr_addr = offset;
        meta.pv_label_hdr_len = sector_size;
        meta.pv_id = std::string(reinterpret_cast<char const*>(header->pv_uuid), sizeof(header->pv_uuid));
        if (header->pv_labeloffset > 0){
            meta.pv_label_addr = offset = header->pv_labeloffset + _offset;
            meta.pv_label_len = sector_size;
            if (!_rw->read(offset, sector_size, buffer.get(), number_of_bytes_read))
                break;
            label = (PV_LABEL *)&buffer.get()[0];
            if (label->pv_length > 0){
                meta.vg_meta_addr = offset = _offset + ((label->pv_offset_low + label->pv_offset_high));
                meta.vg_meta_len = length = ((label->pv_length / sector_size) + 1) * sector_size;
                metadata = std::auto_ptr<char>(new char[length]);
                memset(metadata.get(), 0, length);
                if (_rw->read(offset, length, metadata.get(), number_of_bytes_read)){
                    metadata.get()[label->pv_length] = 0;
                    meta.pv_metadata = std::string(reinterpret_cast<char const*>(metadata.get()), label->pv_length);
                }
                else{
                    break;
                }
            }
        }
        metadatas.push_back(meta);
        if (!meta.pv_metadata.empty() && parse_metadata(meta.pv_metadata))
            return true;
        break;
    }
    return false;
}

bool  logical_volume_manager::parse_metadata(std::string pv_metadata){
    int				num = 0;
    int             vg_seq = 0;
    int             extent_size = 0;

    std::regex	lvm_uuid("[a-zA-Z0-9]*-{1,}[a-zA-Z0-9]*", std::regex_constants::icase);
    std::regex  lvm_pvid("pv[0-9\\s\\t]+\\{", std::regex_constants::icase);
    std::regex  lvm_lv_name("[a-zA-Z_0-9\\s\\t]+\\{", std::regex_constants::icase);
    std::regex  lvm_brace("[\\s\\t]+\\{", std::regex_constants::icase);
    std::regex  lvm_value("[0-9]+");
    std::regex	lvm_pv_extent_size("extent_size");
    std::regex	lvm_physical_volumes("physical_volumes");
    std::regex	lvm_pv_dev_size("dev_size");
    std::regex  lvm_pe_start("pe_start");
    std::regex	lvm_pe_count("pe_count");
    std::regex	lvm_logical_volumes("logical_volumes");
    std::regex	lvm_stripe_id("pv[0-9]+");
    std::regex  lvm_seg_count("segment_count");
    std::regex  lvm_seg_id("segment[0-9]+");    
    std::regex  lvm_start_extent("start_extent");
    std::regex  lvm_extent_count("extent_count");
    std::regex  lvm_type("type");
    std::regex  lvm_stripe_count("stripe_count");
    std::regex  lvm_max_lv("max_lv");
    std::regex  lvm_max_pv("max_pv");
    std::regex  lvm_lf("\\n");
    std::string str = pv_metadata;
    std::string tmp;
    std::string vgVolname;
    std::string vguuid;
    std::string lvVolname;
    num = str.find("{");
    if (num > 0){
        vgVolname = str.substr( 0, num - 1);
    }
    std::smatch what;
    tmp = str.substr(num, -1);
    if (std::regex_search(tmp, what, lvm_uuid)){
        num = str.find(what[0]);
        vguuid = str.substr(num, LVM_META_UUID_LEN);
        tmp = str.substr(num + LVM_META_UUID_LEN, -1);
    }
    else{
        return false;
    }
    if (std::regex_search(tmp, what, lvm_value)){
        vg_seq = atol(what[0].str().c_str());
        tmp = tmp.substr(tmp.find(what[0]) + what[0].str().size(), -1);
    }
    else{
        return false;
    }
    if (std::regex_search(tmp, what, lvm_pv_extent_size)){
        tmp = tmp.substr(tmp.find(what[0]) + what[0].str().size(), -1);
        if (std::regex_search(tmp, what, lvm_value)){
            extent_size = atol(what[0].str().c_str());
            tmp = tmp.substr(tmp.find(what[0]) + what[0].str().size(), -1);
        }
    }
    else{
        return false;
    }
   
    volume_group::ptr vg = add_volgroup(vguuid, vgVolname, vg_seq, extent_size);
    if (!vg){
        return false;
    }
    if (std::regex_search(tmp, what, lvm_max_lv)){
        tmp = tmp.substr(tmp.find(what[0]) + what[0].str().size(), -1);
        if (std::regex_search(tmp, what, lvm_value)){
            vg->max_lv = atol(what[0].str().c_str());
            tmp = tmp.substr(tmp.find(what[0]) + what[0].str().size(), -1);
        }
    }

    if (std::regex_search(tmp, what, lvm_max_pv)){
        tmp = tmp.substr(tmp.find(what[0]) + what[0].str().size(), -1);
        if (std::regex_search(tmp, what, lvm_value)){
            vg->max_pv = atol(what[0].str().c_str());
            tmp = tmp.substr(tmp.find(what[0]) + what[0].str().size(), -1);
        }
    }
   
    if (std::regex_search(tmp, what, lvm_physical_volumes)){
        tmp = tmp.substr(tmp.find(what[0]) + what[0].str().size(), -1);
        if (std::regex_search(tmp, what, lvm_brace)){
            tmp = tmp.substr(tmp.find(what[0]) + what[0].str().size(), -1);
        }
        while (std::regex_search(tmp, what, lvm_pvid)){
            std::string pvname = what[0];
            pvname = macho::stringutils::remove_begining_whitespaces(macho::stringutils::erase_trailing_whitespaces(pvname.substr(0, pvname.find_first_of("{"))));
            tmp = tmp.substr(tmp.find(what[0]) + what[0].str().size(), -1);
            std::string pvid;
            uint64_t devsize = 0;
            uint32_t start = 0;
            uint32_t count = 0 ;
            if (std::regex_search(tmp, what, lvm_uuid)){
                num = tmp.find(what[0]);
                pvid = tmp.substr(num, LVM_META_UUID_LEN);
                tmp = tmp.substr(num + LVM_META_UUID_LEN, -1);
            }
            if (std::regex_search(tmp, what, lvm_pv_dev_size)){
                tmp = tmp.substr(tmp.find(what[0]) + what[0].str().size(), -1);
                if (std::regex_search(tmp, what, lvm_value)){
                    devsize = atoll(what[0].str().c_str());
                    tmp = tmp.substr(tmp.find(what[0]) + what[0].str().size(), -1);
                }
            }
            if (std::regex_search(tmp, what, lvm_pe_start)){
                tmp = tmp.substr(tmp.find(what[0]) + what[0].str().size(), -1);
                if (std::regex_search(tmp, what, lvm_value)){
                    start = atol(what[0].str().c_str());
                    tmp = tmp.substr(tmp.find(what[0]) + what[0].str().size(), -1);
                }
            }
            if (std::regex_search(tmp, what, lvm_pe_count)){
                tmp = tmp.substr(tmp.find(what[0]) + what[0].str().size(), -1);
                if (std::regex_search(tmp, what, lvm_value)){
                    count = atol(what[0].str().c_str());
                    tmp = tmp.substr(tmp.find(what[0]) + what[0].str().size(), -1);
                }
            }
            physical_volume::ptr pv = vg->add_physical_volume(pvname, pvid, devsize, start, count);
        }
    }
    else{
        return false;
    }

    if (std::regex_search(tmp, what, lvm_logical_volumes)){
        tmp = tmp.substr(tmp.find(what[0]) + what[0].str().size(), -1);
        if (std::regex_search(tmp, what, lvm_brace)){
            tmp = tmp.substr(tmp.find(what[0]) + what[0].str().size(), -1);
        }
        while (std::regex_search(tmp, what, lvm_lv_name)){
            std::string lvid;
            std::string lvname = what[0];
            lvname = macho::stringutils::remove_begining_whitespaces(macho::stringutils::erase_trailing_whitespaces(lvname.substr(0, lvname.find("{"))));
            int nsegs = 0;
            tmp = tmp.substr(tmp.find(what[0]) + what[0].str().size(), -1);
            if (std::regex_search(tmp, what, lvm_uuid)){
                num = tmp.find(what[0]);
                lvid = tmp.substr(num, LVM_META_UUID_LEN);
                tmp = tmp.substr(num + LVM_META_UUID_LEN, -1);
            }
            if (std::regex_search(tmp, what, lvm_seg_count)){
                tmp = tmp.substr(tmp.find(what[0]) + what[0].str().size(), -1);
                if (std::regex_search(tmp, what, lvm_value)){
                    nsegs = atol(what[0].str().c_str());
                    tmp = tmp.substr(tmp.find(what[0]) + what[0].str().size(), -1);
                }
            }
  
            logical_volume::ptr lv = vg->add_logical_volume(lvid, nsegs, lvname);

            for (int i = 0; i < nsegs; i++)
            {
                if (std::regex_search(tmp, what, lvm_seg_id)){
                    uint32_t start_extent = 0;
                    uint32_t extent_count = 0;
                    std::string type;
                    if (std::regex_search(tmp, what, lvm_start_extent)){
                        tmp = tmp.substr(tmp.find(what[0]) + what[0].str().size(), -1);
                        if (std::regex_search(tmp, what, lvm_value)){
                            start_extent = atol(what[0].str().c_str());
                            tmp = tmp.substr(tmp.find(what[0]) + what[0].str().size(), -1);
                        }
                    }
                    if (std::regex_search(tmp, what, lvm_extent_count)){
                        tmp = tmp.substr(tmp.find(what[0]) + what[0].str().size(), -1);
                        if (std::regex_search(tmp, what, lvm_value)){
                            extent_count = atol(what[0].str().c_str());
                            tmp = tmp.substr(tmp.find(what[0]) + what[0].str().size(), -1);
                        }
                    }

                    if (std::regex_search(tmp, what, lvm_type)){
                        std::string t = tmp.substr(tmp.find(what[0]) + what[0].str().size(), -1);
                        std::vector<std::string> arr = macho::stringutils::tokenize2(t, "\n", 2, false);
                        if (arr.size()){
                            std::vector<std::string> rr = macho::stringutils::tokenize2(arr[0], "=", 2);
                            if (rr.size()>0)
                                type = macho::stringutils::parser_double_quotation_mark(rr[1]);
                        }
                    }

                    lv_segment seg(start_extent, extent_count, type);
                    uint32_t stripe_count = 0;
                    if (std::regex_search(tmp, what, lvm_stripe_count)){
                        tmp = tmp.substr(tmp.find(what[0]) + what[0].str().size(), -1);
                        if (std::regex_search(tmp, what, lvm_value)){
                            stripe_count = atol(what[0].str().c_str());
                            tmp = tmp.substr(tmp.find(what[0]) + what[0].str().size(), -1);
                        }
                    }
                    for (int stripe = 0; stripe < stripe_count; stripe++){
                        if (std::regex_search(tmp, what, lvm_stripe_id)){
                            std::string pv_name = what[0];
                            tmp = tmp.substr(tmp.find(what[0]) + what[0].str().size(), -1);
                            if (std::regex_search(tmp, what, lvm_value)){
                                seg.stripes.push_back(lv_stripe(pv_name, atol(what[0].str().c_str())));
                                tmp = tmp.substr(tmp.find(what[0]) + what[0].str().size(), -1);
                            }
                        }
                    }
                    bool found = false;
                    foreach(lv_segment& s, lv->segments){
                        if (s.start_extent == seg.start_extent && s.extent_count == seg.extent_count){
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
    else{
        return false;
    }
    return true;
}

logical_volume_manager::groups_luns_map logical_volume_manager::get_groups_luns_mapping(){
    logical_volume_manager::groups_luns_map mapping;
    foreach(volume_group::ptr &vg, volume_groups){
        foreach(lvm_metadata& meta, metadatas){
            std::wstring path = meta.rw->path();
            physical_volume::ptr pv = vg->find_physical_volume(meta.pv_id);
            if (pv){
                mapping[vg->vgname].insert(path);
            }
        }
    }
    return mapping;
}

io_range::map logical_volume_manager::get_system_ranges(){
    io_range::map ranges_map;
    logical_volume::vtr lvs;
    foreach(volume_group::ptr &vg, volume_groups){
        foreach(lvm_metadata& meta, metadatas){
            std::wstring path = meta.rw->path();
            if (meta.pv_label_hdr_len)
                ranges_map[path].push_back(io_range(meta.pv_label_hdr_addr, meta.pv_label_hdr_len));
            if (meta.pv_label_len)
                ranges_map[path].push_back(io_range(meta.pv_label_addr, meta.pv_label_len));
            if (meta.vg_meta_len)
                ranges_map[path].push_back(io_range(meta.vg_meta_addr, meta.vg_meta_len));

            physical_volume::ptr pv = vg->find_physical_volume(meta.pv_id);
            if (pv){
                pv->rw = meta.rw;
                pv->offset = meta.offset / pv->rw->sector_size();
            }
        }
    }
    logical_volume::vtr unmounts;
    foreach(volume_group::ptr &vg, volume_groups){
        logical_volume::vtr _lvs = vg->logical_mount(unmounts);
        lvs.insert(lvs.end(), _lvs.begin(), _lvs.end());
    }

    foreach(logical_volume::ptr lv, lvs){
        universal_disk_rw::ptr rw = logical_volume_rw::get(lv);
        if (rw){
            linuxfs::volume::ptr v = linuxfs::volume::get(rw, 0, lv->size());
            if (v){
                io_range::vtr _ranges = v->file_system_ranges();
                if (_ranges.size()){
                    logical_volume_rw* lv_rw = dynamic_cast<logical_volume_rw*>(rw.get());
                    if (lv_rw){                
                        io_range::map _map = lv_rw->get_ranges_mapper(_ranges);
                        foreach(io_range::map::value_type &m, _map){
                            ranges_map[m.first].insert(ranges_map[m.first].end(), m.second.begin(), m.second.end());
                        }
                    }
                }
            }
            else{
                unmounts.push_back(lv);
            }
        }
    }

    std::regex  lvm_swap("swap_[0-9]+");
    std::smatch what;

    foreach(logical_volume::ptr lv, unmounts){
        if (std::regex_search(lv->volname, what, lvm_swap)){
            continue;
        }
        foreach(lv_segment &seg, lv->segments){
            uint64_t seg_start_sector = (uint64_t)seg.start_extent * lv->extent_size;
            uint64_t seg_length = (uint64_t)(seg.extent_count) * lv->extent_size;
            foreach(lv_stripe &stripe, seg.stripes){
                physical_volume::ptr pv = stripe.pv;
                uint32_t stripe_start = stripe.start_extent * lv->extent_size;
                if (pv && pv->rw){
                    ranges_map[pv->rw->path()].push_back(io_range((pv->offset + pv->pe_start + stripe_start + seg_start_sector) * SECTOR_SIZE, seg_length* SECTOR_SIZE));
                }
            }
        }
    }
    //foreach(linuxfs::io_range::map::value_type& m, ranges_map){
    //    std::sort(m.second.begin(), m.second.end(), linuxfs::io_range::compare());
    //}
    return ranges_map;
}

logical_volume::vtr volume_group::logical_mount(logical_volume::vtr& unmounts){
    bool result = true;
    logical_volume::vtr lvs;
    foreach(logical_volume::ptr &lv, lvolumes){
        bool valid = lv->segments.size() > 0;
        foreach(lv_segment &seg, lv->segments){
            valid = seg.stripes.size() > 0;
            foreach(lv_stripe &stripe, seg.stripes){
                auto it = std::find_if(pvolumes.begin(), pvolumes.end(), [&stripe](const physical_volume::ptr& pv){ return pv->name == stripe.pv_name; });
                if (it != pvolumes.end()){
                    stripe.pv = *it;
                }
            }
            valid = false;
            foreach(lv_stripe &stripe, seg.stripes){
                if (stripe.pv && stripe.pv->rw){
                    valid = true;
                    break;
                }
            }
            if (!valid)
                break;
        }
        if (valid)
            lvs.push_back(lv);
        else
            unmounts.push_back(lv);
    }
    return lvs;
}

bool logical_volume_rw::read(__in uint64_t start, __in uint32_t number_of_bytes_to_read, __inout LPVOID buffer, __inout uint32_t& number_of_bytes_read){
    bool result = false;
    if (0 == start % _sector_size){
        uint32_t number_of_sector_read = 0;
        uint32_t number_of_sectors_to_read = number_of_bytes_to_read / _sector_size ;
        if (number_of_bytes_to_read % _sector_size > 0){
            number_of_sectors_to_read++;
            std::auto_ptr<BYTE> buff = std::auto_ptr<BYTE>(new BYTE[number_of_sectors_to_read*_sector_size]);
            if (NULL != buff.get()){
                if (result = sector_read(start / _sector_size, number_of_sectors_to_read, buff.get(), number_of_sector_read)){
                    memcpy(buffer, buff.get(), number_of_bytes_to_read);
                    number_of_bytes_read = number_of_bytes_to_read;
                }    
            }
        }
        else{
            result = sector_read(start / _sector_size, number_of_bytes_to_read / _sector_size, buffer, number_of_sector_read);
            if (result)
                number_of_bytes_read = number_of_sector_read * _sector_size;
        }
    }
    return result;
}

bool logical_volume_rw::sector_read(__in uint64_t start_sector, __in uint32_t number_of_sectors_to_read, __inout void *buffer, __inout uint32_t& number_of_sectors_read){
    bool result = false;
    number_of_sectors_read = 0;
    uint64_t end_sec = start_sector + number_of_sectors_to_read;
    uint64_t start_sec = start_sector;
    uint32_t sectors_to_read = number_of_sectors_to_read;
    uint32_t offset = 0;
    std::sort(_lv->segments.begin(), _lv->segments.end(), lv_segment::compare());
    foreach(lv_segment &seg, _lv->segments){
        uint64_t seg_start_sector = (uint64_t)seg.start_extent * _lv->extent_size;
        uint64_t seg_end_sector = (uint64_t)((uint64_t)seg.start_extent + seg.extent_count) * _lv->extent_size;
        uint32_t sectors_read = 0;
        if (start_sec >= seg_start_sector && start_sec < seg_end_sector){     
            if (end_sec > seg_end_sector){
                foreach(lv_stripe &stripe, seg.stripes){
                    physical_volume::ptr pv = stripe.pv;
                    uint32_t stripe_start = stripe.start_extent * _lv->extent_size;
                    if (pv && pv->rw){
                        if (result = pv->rw->sector_read(pv->offset + pv->pe_start + stripe_start + (start_sec - seg_start_sector), seg_end_sector - start_sec, &(((BYTE*)buffer)[offset]), sectors_read)){
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
            else{
                foreach(lv_stripe &stripe, seg.stripes){
                    physical_volume::ptr pv = stripe.pv;
                    uint32_t stripe_start = stripe.start_extent * _lv->extent_size;
                    if (pv && pv->rw){
                        if (result = pv->rw->sector_read(pv->offset + pv->pe_start + stripe_start + (start_sec - seg_start_sector), end_sec - start_sec, &(((BYTE*)buffer)[offset]), sectors_read)){
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
    return result;
}

io_range::map logical_volume_rw::get_ranges_mapper(const io_range::vtr &ranges){
    io_range::map ranges_map;
    foreach(const io_range &r, ranges){
        io_range::map _map = mapper(r.start, r.length);
        foreach(io_range::map::value_type &m, _map){
            ranges_map[m.first].insert(ranges_map[m.first].end(), m.second.begin(), m.second.end());
        }
    }
    return ranges_map;
}

io_range::map logical_volume_rw::mapper(__in uint64_t start, __in uint64_t length){
    io_range::map ranges_map;
    uint64_t _end = start + length;
    uint64_t _start = start;
    std::sort(_lv->segments.begin(), _lv->segments.end(), lv_segment::compare());
    foreach(lv_segment &seg, _lv->segments){
        uint64_t seg_start = (uint64_t)seg.start_extent * _lv->extent_size * _sector_size;
        uint64_t seg_end = (uint64_t)((uint64_t)seg.start_extent + seg.extent_count) * _lv->extent_size * _sector_size;
        if (_start >= seg_start && _start < seg_end){
            if (_end > seg_end){
                foreach(lv_stripe &stripe, seg.stripes){
                    physical_volume::ptr pv = stripe.pv;
                    uint32_t stripe_start = stripe.start_extent * _lv->extent_size;
                    if (pv && pv->rw){
                        ranges_map[pv->rw->path()].push_back(io_range(((pv->offset + pv->pe_start + stripe_start) * _sector_size) + (_start - seg_start), seg_end - _start));
                    }
                }
                _start = seg_end;
            }
            else{
                foreach(lv_stripe &stripe, seg.stripes){
                    physical_volume::ptr pv = stripe.pv;
                    uint32_t stripe_start = stripe.start_extent * _lv->extent_size;
                    if (pv && pv->rw){
                        ranges_map[pv->rw->path()].push_back(io_range(((pv->offset + pv->pe_start + stripe_start) * _sector_size) + (_start - seg_start), _end - _start));
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