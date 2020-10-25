
#include "vhdtool.h"
#include <windows.h>

#define htobe16(x) htons(x)
#define htole16(x) (x)
#define be16toh(x) ntohs(x)
#define le16toh(x) (x)
#define htobe32(x) htonl(x)
#define htole32(x) (x)
#define be32toh(x) ntohl(x)
#define le32toh(x) (x)
#define htobe64(x) htonll(x)
#define htole64(x) (x)
#define be64toh(x) ntohll(x)
#define le64toh(x) (x)

#define _FILE_OFFSET_BITS 64
#define BYTES_PER_SECTOR 512

#include <errno.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>

#define OPEN_RAW_OK (1 << 1)
#define OPEN_RW     (1 << 2)
#define OPEN_CREAT  (1 << 3)
#define COMPAT_SIZE (1 << 4)

#define COOKIE(x)           (*(uint64_t *) x)
#define COOKIE32(x)         (*(uint32_t *) x)
#define FOOTER_FEAT_RSVD    (2)
#define VHD_VERSION_1       (0x00010000UL)
#define VHD_VMAJ_MASK       (0xFFFF0000UL)
#define VHD_VMIN_MASK       (0x0000FFFFUL)
#define DYN_VERSION_1       (0x00010000UL)
#define DYN_VMAJ_MASK       (0xFFFF0000UL)
#define DYN_VMIN_MASK       (0x0000FFFFUL)
#define FOOTER_DOFF_FIXED   (0xFFFFFFFFFFFFFFFFULL)
#define DYN_DOFF_DYN        (0xFFFFFFFFFFFFFFFFULL)
#define SECONDS_OFFSET      946684800
#define FOOTER_TYPE_FIXED   (2)
#define FOOTER_TYPE_DYN     (3)
#define FOOTER_TYPE_DIFF    (4)
#define SEC_SHIFT           (9)
#define SEC_SZ              (1 << SEC_SHIFT)
#define SEC_MASK            (SEC_SZ - 1)
#define round_up(what, on) ((((what) + (on) - 1) / (on)) * (on))
#define DYN_BLOCK_SZ        0x200000
#define DYN_BLOCK_SZ_S      0x80000
#define BAT_ENTRY_EMPTY     0xFFFFFFFF

using namespace macho;

BYTE vhd_tool::_zero_buffer[ZERO_BLOCK_SZ] = { 0 };

vhd_tool::vhd_tool(uint64_t size) : 
_size(size), 
_offset(0), 
_flags(0), 
_type(FOOTER_TYPE_DYN), 
_block_size(DYN_BLOCK_SZ), 
_footer(NULL), 
_dyn(NULL), 
_header_size(0), 
_current_block(0),
_sectors_per_disk(0),
_sectors_per_block(0),
_bat_entries(0){
    _block_chunk_size = _block_size + BYTES_PER_SECTOR;
    _block = boost::shared_ptr<BYTE>(new BYTE[_block_chunk_size + sizeof(struct vhd_footer)]);
    memset(_block.get(), 0, _block_chunk_size + sizeof(struct vhd_footer));
    memset(_zero_buffer, 0, ZERO_BLOCK_SZ);
}

/* Returns the minimum, which cannot be zero. */
unsigned vhd_tool::_min_nz(unsigned a, unsigned b){
    if (a < b && a != 0) {
        return a;
    }
    else {
        return b;
    }
}

uint32_t vhd_tool::_vhd_checksum(uint8_t *data, size_t size){
    uint32_t csum = 0;
    while (size--) {
        csum += *data++;
    }
    return ~csum;
}

bool vhd_tool::_is_zero(LPBYTE buf, size_t length){
    for (size_t i = 0; i < length; i += ZERO_BLOCK_SZ){
        size_t s = length - i;
        if (memcmp(&buf[i], _zero_buffer, s > ZERO_BLOCK_SZ ? ZERO_BLOCK_SZ : s))
            return false;
    }
    return true;
}

bool vhd_tool::read(__in uint64_t start, __in uint32_t number_of_bytes_to_read, __inout void *buffer, __inout uint32_t& number_of_bytes_read){
    return false;
}

bool vhd_tool::sector_read(__in uint64_t start_sector, __in uint32_t number_of_sectors_to_read, __inout void *buffer, __inout uint32_t& number_of_sectors_read){
    return false;
}

bool vhd_tool::write(__in uint64_t start, __in const void *buffer, __in uint32_t number_of_bytes_to_write, __inout uint32_t& number_of_bytes_written){
    uint32_t number_of_sectors_written=0;
    if ( (start >> 9 << 9 != start) || (number_of_bytes_to_write >> 9 << 9 != number_of_bytes_to_write) )
        return false;
    bool result = sector_write(start / BYTES_PER_SECTOR, buffer, number_of_bytes_to_write / BYTES_PER_SECTOR, number_of_sectors_written);
    number_of_bytes_written = number_of_sectors_written * BYTES_PER_SECTOR;
    return result;
}

std::string vhd_tool::get_fixed_vhd_footer(uint64_t disk_size, UUID u){
    std::string result;
    vhd_tool* vhd = new vhd_tool(disk_size);
    if (vhd){
        if (vhd->_create(true, u)){
            result = std::string(reinterpret_cast<char const*>(vhd->_header.get()), vhd->_header_size);
        }
        delete vhd;
    }
    return result;
}

vhd_tool* vhd_tool::create(uint64_t disk_size, bool is_fixed, UUID u){
    vhd_tool* vhd = new vhd_tool(disk_size);
    if (vhd){
        if (vhd->_create(is_fixed, u))
            return vhd;
        delete vhd;
    }
    return NULL;
}

bool vhd_tool::_write(uint64_t offset, void *buf, size_t size){
    if (offset > _size ||(uint64_t)(offset + size) > _size) {
        LOG(LOG_LEVEL_ERROR, L"out-of-bound write");
        return false;
    }
    _offset = offset;
    return _vhd_write(buf, size);
}

bool vhd_tool::_vhd_write(void *buf, size_t size){
    _offset += size;
    return true;
}

bool vhd_tool::_vhd_footer(struct vhd_footer &footer, uint64_t data_offset, UUID u){
    if (_size >> 9 << 9 != _size) {
        LOG(LOG_LEVEL_ERROR, L"size must be in units of 512-byte sectors.");
        return false;
    }
    if (_vhd_chs(footer) == -1) {
        LOG(LOG_LEVEL_ERROR, L"size is too small");
        return false;
    }
    footer.cookie = COOKIE("conectix");
    footer.features = htobe32(FOOTER_FEAT_RSVD);
    if (FOOTER_TYPE_FIXED == _type)
        footer.data_offset = htobe64(-1LL);
    else
        footer.data_offset = htobe64(data_offset);
    footer.file_format_ver = htobe32(VHD_VERSION_1);
    footer.time_stamp = htobe32(time(NULL) + SECONDS_OFFSET);
    footer.creator_app = COOKIE32("win ");
    footer.creator_ver = htobe32(0x1);
    footer.creator_os = COOKIE32("Wi2k");
    footer.original_size = htobe64(_size);
    footer.current_size = htobe64(_size);
    footer.disk_type = htobe32(_type);
    memcpy(&footer.vhd_id, &u, sizeof(footer.vhd_id));
    footer.vhd_id.f1 = htobe32(footer.vhd_id.f1);
    footer.vhd_id.f2 = htobe16(footer.vhd_id.f2);
    footer.vhd_id.f3 = htobe16(footer.vhd_id.f3);
    footer.checksum = _vhd_checksum((uint8_t *)&footer, sizeof(struct vhd_footer));
    footer.checksum = htobe32(footer.checksum);
    return true;
}

bool vhd_tool::_vhd_dyn(struct vhd_dyn &dyn){
    dyn.cookie = COOKIE("cxsparse");
    dyn.data_offset = htobe64(DYN_DOFF_DYN);
    dyn.table_offset = htobe64((uint64_t)sizeof(struct vhd_footer) + sizeof(struct vhd_dyn));
    dyn.header_version = htobe32(DYN_VERSION_1);
    dyn.block_size = htobe32(_block_size);
    dyn.max_tab_entries = _bat_entries;
    if (!dyn.max_tab_entries) {
        LOG(LOG_LEVEL_ERROR, L"Block size can't be larger than the VHD");
        return false;
    }
    if ((uint64_t)dyn.max_tab_entries * _block_size != _size) {
        LOG(LOG_LEVEL_ERROR, L"VHD size not multiple of block size");
        return false;
    }
    dyn.max_tab_entries = htobe32(dyn.max_tab_entries);
    dyn.checksum = _vhd_checksum((uint8_t *)&dyn, sizeof(struct vhd_dyn));
    dyn.checksum = htobe32(dyn.checksum);
    return true;
}

int vhd_tool::_vhd_chs(struct vhd_footer &footer){
    uint64_t cyl_x_heads;
    struct vhd_chs chs;
    uint64_t new_sectors;
    uint64_t sectors;
    uint64_t original_size = _size;

again:
    sectors = _size >> 9;
    /*
    * All this logic is from the VHD specification.
    */

    if (sectors > 65535 * 16 * 255) {
        /* ~127GiB */
        sectors = 65535 * 16 * 255;
    }
    if (sectors >= 65535 * 16 * 63) {
        chs.s = 255;
        chs.h = 16;
        cyl_x_heads = sectors / chs.s;
    }
    else {
        chs.s = 17;
        cyl_x_heads = sectors / chs.s;
        chs.h = (cyl_x_heads + 1023) >> 10;
        if (chs.h < 4)
            chs.h = 4;
        if (cyl_x_heads >= (uint64_t)(chs.h << 10) ||
            chs.h > 16) {
            chs.s = 31;
            chs.h = 16;
            cyl_x_heads = sectors / chs.s;
        }

        if (cyl_x_heads >= (uint64_t)(chs.h << 10)) {
            chs.s = 63;
            chs.h = 16;
            cyl_x_heads = sectors / chs.s;
        }
    }
    chs.c = cyl_x_heads / chs.h;

    footer.disk_geometry.c = htobe16(chs.c);
    footer.disk_geometry.h = chs.h;
    footer.disk_geometry.s = chs.s;
    if (sectors < 65535 * 16 * 255) {
        /*
        * All of this nonsense matters only for disks < 127GB.
        */
        new_sectors = chs.c * chs.h * chs.s;
        if (new_sectors != sectors) {
            if (original_size == _size) {
                /* Only show warning once. */
                LOG(LOG_LEVEL_DEBUG, 
                    L"C(%u)H(%u)S(%u)-derived"
                    L" total sector count (%lu) does"
                    L" not match actual (%lu)%s.\n",
                    chs.c, chs.h, chs.s,
                    new_sectors, sectors,
                    _flags & COMPAT_SIZE ?
                    L" and will be recomputed" :
                    L"");
            }

            if (_flags & COMPAT_SIZE) {
                _size = round_up(_size + 1,
                    _min_nz(_min_nz(chs.c, chs.h),
                    chs.s) << 9);
                goto again;
            }

            LOG(LOG_LEVEL_DEBUG, L"You may have problems"
                L" with Hyper-V if converting raw disks"
                L" to VHD, or if moving VHDs from ATA to"
                L" SCSI.\n");
        }
    }

    if (original_size != _size) {
        LOG(LOG_LEVEL_WARNING, L"Increased VHD size from"
            L" %ju to %ju bytes\n", original_size,
            _size);
    }
    return 0;
}

bool vhd_tool::_create(bool is_fixed, UUID u){
    bool status = false;
    _sectors_per_disk = _size >> 9;
    _sectors_per_block = _block_size >> 9;
	if (is_fixed){
		_type = FOOTER_TYPE_FIXED;
		_header_size = round_up(sizeof(struct vhd_footer), 512);
		_header = boost::shared_ptr<BYTE>(new BYTE[_header_size]);
		memset(_header.get(), 0, _header_size);
		_footer = (struct vhd_footer *) _header.get();
		status = _vhd_footer(*_footer, sizeof(struct vhd_footer), u);
	}
	else{
		_bat_entries = ((_size - 1) / _block_size) + 1;
		_offset = sizeof(struct vhd_footer) + sizeof(struct vhd_dyn) + (_bat_entries * sizeof(vhd_batent));
		if (_offset < 1048576) _offset = 1048576;
		_header_size = _offset = round_up(_offset, 512);
		_header = boost::shared_ptr<BYTE>(new BYTE[_header_size]);
		memset(_header.get(), 0, _header_size);
		_footer = (struct vhd_footer *) _header.get();
		_dyn = (struct vhd_dyn *) (&_header.get()[sizeof(struct vhd_footer)]);
		_batents = (vhd_batent*)(&_header.get()[sizeof(struct vhd_footer) + sizeof(struct vhd_dyn)]);
        if (!(status = _vhd_footer(*_footer, sizeof(struct vhd_footer), u))) {}
		else if (!(status = _vhd_dyn(*_dyn))){}
		else{
			for (int i = 0; i < _bat_entries; ++i)
				_batents[i] = BAT_ENTRY_EMPTY;
		}
	}
    return status;
}

uint64_t vhd_tool::estimate_size(){
	uint64_t _estimate_size = 0;
	if (_type == FOOTER_TYPE_FIXED)
		_estimate_size = _size + sizeof(struct vhd_footer);
	else{		
		for (int i = 0; i < _bat_entries; ++i){
			if (_batents[i] != BAT_ENTRY_EMPTY){
				_estimate_size += _block_chunk_size;
			}
		}
		_estimate_size += (_header_size + sizeof(struct vhd_footer));
	}
	return _estimate_size;
}

void vhd_tool::reset(){
	if (_type != FOOTER_TYPE_FIXED){
		for (int i = 0; i < _bat_entries; ++i)
			_batents[i] = BAT_ENTRY_EMPTY;
		_offset = _header_size;
	}
}

bool vhd_tool::close(){
    bool result = false;
	if (_type == FOOTER_TYPE_FIXED)
		return _flush_data(_size, _size >> 9, _header.get(), _header_size);
    if (_is_cancel()){}
    if (!_flush_data(0, -1, _header.get(), _header_size)){}
    else if (_batents[_current_block] != BAT_ENTRY_EMPTY ){
        //_offset = be32toh(_batents[_current_block]) << 9;
        if (_block){
            memcpy(&_block.get()[_block_chunk_size], _header.get(), sizeof(struct vhd_footer));
            result = _flush_data(_offset, _current_block, _block.get(), _block_chunk_size + sizeof(struct vhd_footer));
        }
    }
    else{
        result = _flush_data(_offset, -1, _header.get(), sizeof(struct vhd_footer));
    }
    return result;
}

bool vhd_tool::sector_write(__in uint64_t start_sector, __in const void *buffer, __in uint32_t number_of_sectors_to_write, __inout uint32_t& number_of_sectors_written){ 
	bool result = false;
	if (_type == FOOTER_TYPE_FIXED){
		result = (NULL == buffer) || _flush_data(start_sector << 9, (uint32_t)start_sector, (LPBYTE)buffer, number_of_sectors_to_write << 9);
	}
	else{
		uint32_t current_sector, index = 0, block_number, sector_in_block, length, number_of_sectors;
		number_of_sectors = number_of_sectors_to_write;
		current_sector = start_sector;
		LPBYTE lpbuf = (LPBYTE)buffer;
		while (index < number_of_sectors_to_write){
			result = false;
			block_number = current_sector / _sectors_per_block;
			sector_in_block = current_sector % _sectors_per_block;
			// Verify the block number
			if (block_number > _bat_entries) break;
			if (_batents[block_number] != BAT_ENTRY_EMPTY){
				if (_current_block != block_number){
					//not support this scenario
					LOG(LOG_LEVEL_ERROR, L"Writing block(%d) is not currently block(%d).", block_number, _current_block);
					break;
				}
				// Calculate the length of writing buffer that is belong to this BAT block.
				length = _sectors_per_block - sector_in_block;
				if (length > number_of_sectors)
					length = number_of_sectors;

				// Set those bits to dirty.
				for (uint32_t ibit = sector_in_block; ibit < (sector_in_block + length); ibit++){
					set_umap_bit(_block.get(), ibit);
				}
				uint32_t data_length = length << 9;
				if (buffer){
					memcpy(&(_block.get()[(sector_in_block + 1) << 9]), lpbuf, data_length);
					lpbuf = &lpbuf[data_length];
				}
				else{
					memset(&(_block.get()[(sector_in_block + 1) << 9]), 1, data_length);
				}
				result = true;
				current_sector += length;
				index += length;
				_current_block = block_number;
				number_of_sectors -= length;
				number_of_sectors_written += length;
			}
			else{
				if (block_number){
					if (_is_cancel())
						break;
					else if (_is_zero(&_block.get()[BYTES_PER_SECTOR], _block_size)){
						_batents[_current_block] = BAT_ENTRY_EMPTY;
						result = true;
					}
					else if ((NULL == buffer) || _flush_data.empty()){
						result = true;
						_offset += _block_chunk_size;
					}
					else if (result = _flush_data(_offset, _current_block, _block.get(), _block_chunk_size)){
						_offset += _block_chunk_size;
					}
					else
						break;
				}
				memset(_block.get(), 0, _block_chunk_size + sizeof(struct vhd_footer));
				_batents[block_number] = htobe32((_offset >> 9));
				_current_block = block_number;
			}
		}
	}
    return result;
}

vhd_tool* vhd_tool::open(boost::filesystem::path vhd_file){
    vhd_tool* vhd = NULL;
    std::ifstream file(vhd_file.string(), std::ios::in | std::ios::binary);
    std::string buff = std::string();
    if (file.is_open()){
        int footer_size = sizeof(struct vhd_footer);
        buff.resize(footer_size);
        file.seekg(0 - footer_size, std::ios::end);
        file.read(&buff[0], buff.size());
        struct vhd_footer * footer = (struct vhd_footer *)&buff[0];
        uint64_t size = be64toh(footer->current_size);
        uint64_t data_offset = be64toh(footer->data_offset);
        int type = be32toh(footer->disk_type);
        if (FOOTER_TYPE_DYN == type || FOOTER_TYPE_DIFF == type){
            buff.resize(sizeof(struct vhd_dyn));
            file.seekg(data_offset, std::ios::beg);
            file.read(&buff[0], buff.size());
            struct vhd_dyn * dyn = (struct vhd_dyn *)&buff[0];
            vhd = new vhd_tool(size);
            vhd->_type = type;
            vhd->_bat_entries = be32toh(dyn->max_tab_entries);
            vhd->_block_size = be32toh(dyn->block_size);
            vhd->_block_chunk_size = vhd->_block_size + BYTES_PER_SECTOR;
            file.seekg(0, std::ios::beg);
            file.close();
        }
    }
    return vhd;
}

vhd_tool::dirty_range::vtr vhd_tool::get_dirty_ranges(boost::filesystem::path vhd_file, bool is_rough){
    dirty_range::vtr ranges;
    vhd_tool* vhd = NULL;
    std::ifstream file(vhd_file.string(), std::ios::in | std::ios::binary);
    std::string buff = std::string();
    if (file.is_open()){
        int footer_size = sizeof(struct vhd_footer);
        buff.resize(footer_size);
        file.seekg(0 - footer_size, std::ios::end);
        file.read(&buff[0], buff.size());
        struct vhd_footer * footer = (struct vhd_footer *)&buff[0];
        uint64_t size = be64toh(footer->current_size);
        uint64_t data_offset = be64toh(footer->data_offset);
        int type = be32toh(footer->disk_type);
        if (FOOTER_TYPE_DYN == type || FOOTER_TYPE_DIFF == type){
            buff.resize(sizeof(struct vhd_dyn));
            file.seekg(data_offset, std::ios::beg);
            file.read(&buff[0], buff.size());
            struct vhd_dyn * dyn = (struct vhd_dyn *)&buff[0];
            vhd = new vhd_tool(size);
            if (vhd){
                vhd->_type = type;
                uint32_t max_tab_entries = be32toh(dyn->max_tab_entries);
                vhd->_bat_entries = max_tab_entries;
                vhd->_block_size = be32toh(dyn->block_size);
                vhd->_block_chunk_size = vhd->_block_size + BYTES_PER_SECTOR;
                std::string batents = std::string();
                batents.resize(max_tab_entries *  sizeof(vhd_batent));
                data_offset = be64toh(dyn->table_offset);
                file.seekg(data_offset, std::ios::beg);
                file.read(&batents[0], batents.size());
                vhd->_batents = (vhd_batent*)&batents[0];
                for (int i = 0; i < max_tab_entries; ++i){
                    if (vhd->_batents[i] != BAT_ENTRY_EMPTY){
                        uint64_t offset = i * vhd->_block_size;
                        std::string bitmap = std::string();
                        bitmap.resize(BYTES_PER_SECTOR);
                        file.seekg((be32toh(vhd->_batents[i]) << 9), std::ios::beg);
                        file.read(&bitmap[0], bitmap.size());
                        if (is_rough){
                            uint64_t newBit, latest_dirty_bit = 0;
                            for (newBit = 0; newBit < (vhd->_block_size / BYTES_PER_SECTOR); newBit++){
                                if (bitmap[newBit >> 3] & (1 << (newBit & 7))){
                                    latest_dirty_bit = newBit;
                                }
                            }
                            ranges.push_back(dirty_range(offset, (latest_dirty_bit + 1) * BYTES_PER_SECTOR));
                        }
                        else{
                            uint64_t newBit, oldBit = 0;
                            bool     dirty = false;
                            for (newBit = 0; newBit < (vhd->_block_size / BYTES_PER_SECTOR); newBit++){
                                if (bitmap[newBit >> 3] & (1 << (newBit & 7))){
                                    if (!dirty){
                                        dirty = true;
                                        oldBit = newBit;
                                    }
                                }
                                else {
                                    if (dirty){
                                        uint32_t start = oldBit * BYTES_PER_SECTOR;
                                        uint32_t length = (newBit - oldBit) * BYTES_PER_SECTOR;
                                        ranges.push_back(dirty_range(offset + start, length));
                                        dirty = false;
                                    }
                                }
                            }
                            if (dirty){
                                uint32_t start = oldBit * BYTES_PER_SECTOR;
                                uint32_t length = (newBit - oldBit) * BYTES_PER_SECTOR;
                                ranges.push_back(dirty_range(offset + start, length));
                            }
                        }
                    }
                }
                delete vhd;
            }
            file.seekg(0, std::ios::beg);
            file.close();
        }
    }
    return ranges;
}