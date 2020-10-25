#include "macho.h"
#include "ext2fs.h"

using namespace linuxfs;
using namespace macho;

std::vector<uint64_t> ext2_file::get_data_blocks(){
    std::vector<uint64_t> data_blocks;
    //If the file is smaller than 60 bytes, then the data are stored inline in inode.i_block.
    if (is_reg() || is_dir() || file_size < EXT2_LINKLEN_IN_INODE){
    }
    else{
        uint64_t blocks, blkindex;
        blocks = file_size / volume->block_size;
        for (blkindex = 0; blkindex < blocks; blkindex++)
        {
            uint64_t block = 0;
            if (INODE_HAS_EXTENT(&inode))
                block = volume->extent_to_logical(&inode, blkindex);
            else
                block = volume->fileblock_to_logical(&inode, blkindex);

            if (block)
                data_blocks.push_back(block);
        }
        int extra = file_size % volume->block_size;
        if (extra)
        {
            uint64_t block = 0;
            if (INODE_HAS_EXTENT(&inode))
                block = volume->extent_to_logical(&inode, blkindex);
            else
                block = volume->fileblock_to_logical(&inode, blkindex);

            if (block)
                data_blocks.push_back(block);
        }
    }
    return data_blocks;
}

void  ext2_volume::get_filesystem_blocks(ext2_file::ptr dir, std::vector<uint64_t>& blocks){
    ext2_file::vtr files = enumerate_dir(dir);
    foreach(ext2_file::ptr &f, files){
        if (f->is_dir()){
            get_filesystem_blocks(f, blocks);
        }
        else{
            std::vector<uint64_t> file_blocks = f->get_data_blocks();
            blocks.insert(blocks.end(), file_blocks.begin(), file_blocks.end());
        }
    }
}

uint64_t ext2_volume::extent_binarysearch(EXT4_EXTENT_HEADER *header, uint64_t lbn, bool isallocated){

    EXT4_EXTENT *extent;
    EXT4_EXTENT_IDX *index;
    EXT4_EXTENT_HEADER *child;
    uint64_t physical_block = 0;
    uint64_t block;

    if (header->eh_magic != EXT4_EXT_MAGIC)
    {
        LOG(LOG_LEVEL_ERROR, L"Invalid magic in Extent Header: %X", header->eh_magic);
        return 0;
    }
    extent = EXT_FIRST_EXTENT(header);
    //    LOG("HEADER: magic %x Entries: %d depth %d\n", header->eh_magic, header->eh_entries, header->eh_depth);
    if (header->eh_depth == 0)
    {
        for (int i = 0; i < header->eh_entries; i++)
        {
            //          LOG("EXTENT: Block: %d Length: %d LBN: %d\n", extent->ee_block, extent->ee_len, lbn);
            if ((lbn >= extent->ee_block) &&
                (lbn < (extent->ee_block + extent->ee_len)))
            {
                physical_block = ext_to_block(extent) + lbn;
                physical_block = physical_block - (uint64_t)extent->ee_block;
                if (isallocated)
                    delete[] header;
                //                LOG("Physical Block: %d\n", physical_block);
                return physical_block;
            }
            extent++; // Pointer increment by size of Extent.
        }
        return 0;
    }

    index = EXT_FIRST_INDEX(header);
    for (int i = 0; i < header->eh_entries; i++)
    {
        //        LOG("INDEX: Block: %d Leaf: %d \n", index->ei_block, index->ei_leaf_lo);
        if ((i == (header->eh_entries - 1)) ||
            (lbn < (index + 1)->ei_block))
        {
            child = (EXT4_EXTENT_HEADER *) new char[block_size];
            block = idx_to_block(index);
            ext2_read_block(block, (void *)child);
            return extent_binarysearch(child, lbn, true);
        }
        index++;
    }

    // We reach here if we do not find the key
    if (isallocated)
        delete[] header;
    return physical_block;
}

uint32_t ext2_volume::fileblock_to_logical(EXT2_INODE *ino, uint32_t lbn){
    uint32_t block, indlast, dindlast;
    uint32_t tmpblk, sz;
    uint32_t *indbuffer;
    uint32_t *dindbuffer;
    uint32_t *tindbuffer;

    if (lbn < EXT2_NDIR_BLOCKS)
    {
        block = ino->i_block[lbn];
        return ino->i_block[lbn];
    }

    sz = block_size / sizeof(uint32_t);
    indlast = sz + EXT2_NDIR_BLOCKS;
    indbuffer = new uint32_t[sz];
    if ((lbn >= EXT2_NDIR_BLOCKS) && (lbn < indlast))
    {
        block = ino->i_block[EXT2_IND_BLOCK];
        ext2_read_block(block, indbuffer);
        lbn -= EXT2_NDIR_BLOCKS;
        block = indbuffer[lbn];
        delete[] indbuffer;
        return block;
    }

    dindlast = (sz * sz) + indlast;
    dindbuffer = new uint32_t[sz];
    if ((lbn >= indlast) && (lbn < dindlast))
    {
        block = ino->i_block[EXT2_DIND_BLOCK];
        ext2_read_block(block, dindbuffer);

        tmpblk = lbn - indlast;
        block = dindbuffer[tmpblk / sz];
        ext2_read_block(block, indbuffer);

        lbn = tmpblk % sz;
        block = indbuffer[lbn];

        delete[] dindbuffer;
        delete[] indbuffer;
        return block;
    }

    tindbuffer = new uint32_t[sz];
    if (lbn >= dindlast)
    {
        block = ino->i_block[EXT2_TIND_BLOCK];
        ext2_read_block(block, tindbuffer);

        tmpblk = lbn - dindlast;
        block = tindbuffer[tmpblk / (sz * sz)];
        ext2_read_block(block, dindbuffer);

        block = tmpblk / sz;
        lbn = tmpblk % sz;
        block = dindbuffer[block];
        ext2_read_block(block, indbuffer);
        block = indbuffer[lbn];

        delete[] tindbuffer;
        delete[] dindbuffer;
        delete[] indbuffer;
        return block;
    }
    // We should not reach here
    return 0;
}

int ext2_volume::read_data_block(EXT2_INODE *ino, uint64_t lbn, void *buf){

    uint64_t block;

    if (INODE_HAS_EXTENT(ino))
        block = extent_to_logical(ino, lbn);
    else
        block = fileblock_to_logical(ino, lbn);

    if (block == 0)
        return -1;
    return ext2_read_block(block, buf) ? 0 : -1;

}

ext2_file::vtr ext2_volume::enumerate_dir(ext2_file::ptr dir){
    ext2_file::vtr files;
    ext2_dirent::ptr dirent = open_dir(dir);
    if (dirent){
        ext2_file::ptr file;
        while (file = read_dir(dirent)){
            files.push_back(file);
        }
    }
    return files;
}

ext2_file::ptr ext2_volume::read_dir(ext2_dirent::ptr& dirent){
    std::string filename;
    ext2_file::ptr newEntry;
    char *pos;
    int ret;
    if (!dirent)
        return NULL;
    if (!dirent->dirbuf)
    {
        dirent->dirbuf = boost::shared_ptr<EXT2_DIR_ENTRY>((EXT2_DIR_ENTRY *) new char[block_size]);
        if (!dirent->dirbuf)
            return NULL;
        ret = read_data_block(&dirent->parent->inode, dirent->next_block, dirent->dirbuf.get());
        if (ret < 0)
            return NULL;

        dirent->next_block++;
    }

again:
    if (!dirent->next)
        dirent->next = dirent->dirbuf.get();
    else
    {
        pos = (char *)dirent->next;
        dirent->next = (EXT2_DIR_ENTRY *)(pos + dirent->next->rec_len);
        if (IS_BUFFER_END(dirent->next, dirent->dirbuf.get(), block_size))
        {
            dirent->next = NULL;
            if (dirent->read_bytes < dirent->parent->file_size)
            {
                //LOG("DIR: Reading next block %d parent %s\n", dirent->next_block, dirent->parent->file_name.c_str());
                ret = read_data_block(&dirent->parent->inode, dirent->next_block, dirent->dirbuf.get());
                if (ret < 0)
                    return NULL;

                dirent->next_block++;
                goto again;
            }
            return NULL;
        }
    }

    dirent->read_bytes += dirent->next->rec_len;
    filename.assign(dirent->next->name, dirent->next->name_len);
    if ((filename.compare(".") == 0) ||
        (filename.compare("..") == 0))
        goto again;


    newEntry = read_inode(dirent->next->inode);
    if (!newEntry)
    {
        LOG(LOG_LEVEL_ERROR, L"Error reading Inode %d parent inode %d.\n", dirent->next->inode, dirent->parent->inode_num);
        return NULL;
    }

    newEntry->file_type = dirent->next->filetype;
    newEntry->file_name = filename;

    return newEntry;

}

bool  ext2_volume::ext2_read_block(uint64_t blocknum, void *buffer){
    uint32_t number_of_bytes_read = 0;
    memset(buffer, 0, block_size);
    return rw->read(offset + blocknum *block_size, block_size, buffer, number_of_bytes_read);
}

ext2_file::ptr ext2_volume::read_inode(uint32_t inum)
{
    uint64_t group, index, blknum;
    int inode_index, ret = 0;
    ext2_file::ptr file;
    EXT2_INODE *src;

    if (inum == 0)
        return NULL;

    if (!inode_buffer)
    {
        inode_buffer = boost::shared_ptr<char>(new char[block_size]);
        if (!inode_buffer)
            return NULL;
    }

    group = (inum - 1) / inodes_per_group;

    if (group > total_groups)
    {
        LOG(LOG_LEVEL_ERROR, L"Error Reading Inode %X. Invalid Inode Number", inum);
        return NULL;
    }

    index = ((inum - 1) % inodes_per_group) * inode_size;
    inode_index = (index % block_size);
    blknum = get_inode_table_block(get_group_desc(group)) + (index / block_size);

    if (blknum != last_read_block) {
        if (!ext2_read_block(blknum, inode_buffer.get())) {
            LOG(LOG_LEVEL_ERROR, L"Disk read failed");
            return NULL;
        }
    }

    file = ext2_file::ptr(new ext2_file());
    if (!file)
    {
        LOG(LOG_LEVEL_ERROR, L"Allocation of File Failed.");
        return NULL;
    }
    src = (EXT2_INODE *)(inode_buffer.get() + inode_index);
    file->inode = *src;

    LOG(LOG_LEVEL_INFO, L"BLKNUM is %d, inode_index %d", file->inode.i_size, inode_index);
    file->inode_num = inum;
    file->file_size = (uint64_t)src->i_size | ((uint64_t)src->i_size_high << 32);
    if (file->file_size == 0)
    {
        LOG(LOG_LEVEL_ERROR, L"Inode %d with file size 0", inum);
    }
    file->volume = this;
    last_read_block = blknum;
    return file;
}

io_range::vtr ext2_volume::file_system_ranges(){
    io_range::vtr ranges;
    if (groups_desc){
        uint64_t umap_size = (total_blocks >> 3) + 1;
        //uint64_t inodes_table_size = inodes_per_group * inode_size;
        //uint32_t inodes_table_blocks = inodes_table_size / block_size;
        std::auto_ptr<unsigned char> umap = std::auto_ptr<unsigned char>(new unsigned char[umap_size]);
        std::auto_ptr<unsigned char> buff = std::auto_ptr<unsigned char>(new unsigned char[block_size]);
        //std::auto_ptr<unsigned char> inode_table_buff = std::auto_ptr<unsigned char>(new unsigned char[inodes_table_size]);
        if (NULL != buff.get() && NULL != umap.get() /*&& NULL != inode_table_buff.get()*/){
            memset(umap.get(), 0, umap_size);
            //memset(inode_table_buff.get(), 0, inodes_table_size);
            for (uint64_t index = 0; index < total_groups; index++){
                uint64_t blocks_in_this_group = blocks_per_group;
                if (((uint64_t)first_data_block + (((uint64_t)index + 1) * blocks_per_group)) > total_blocks)
                    blocks_in_this_group = (uint64_t)(total_blocks - first_data_block) - ((uint64_t)index * blocks_per_group);
                if (index == 0){
                    for (uint64_t i = 0; i < blocks_in_this_group; i++){
                        set_umap_bit(umap.get(), (uint64_t)first_data_block + ((uint64_t)blocks_per_group * index) + i);
                    }
                }
                PEXT4_GROUP_DESC gb = get_group_desc(index);
                if (get_block_bitmap(gb)){
                    memset(buff.get(), 0, block_size);
                    if (ext2_read_block(get_block_bitmap(gb), buff.get())){
                        if (get_free_blocks_count(gb) != blocks_in_this_group){
                            for (uint64_t i = 0; i < blocks_in_this_group; i++){
                                if (test_umap_bit(buff.get(), i)){
                                    set_umap_bit(umap.get(), (uint64_t)first_data_block + ((uint64_t)blocks_per_group * index) + i);
                                }
                            }
                        }
                    }
                }
           
                //if (get_inode_bitmap(gb)){
                //    if (get_free_inodes_count(gb) != inodes_per_group){
                //        uint64_t inode_table = get_inode_table_block(gb);
                //        for (uint64_t t = 0; t < inodes_table_blocks; t++){
                //            if (!test_umap_bit(umap.get(), inode_table + t)){
                //                set_umap_bit(umap.get(), inode_table + t);
                //            }
                //        }
                //        memset(buff.get(), 0, block_size);
                //        if (ext2_read_block(get_inode_bitmap(gb), buff.get()) &&
                //            ext2_read_inode_table_block(inode_table, inode_table_buff.get(), inodes_table_size)){
                //            for (uint32_t i = 0; i < inodes_per_group; i++){
                //                if (test_umap_bit(buff.get(), i)){
                //                    ext2_file::ptr file = ext2_file::ptr(new ext2_file());
                //                    EXT2_INODE * src = (EXT2_INODE *)(inode_table_buff.get() + (i *  inode_size));
                //                    file->inode = *src;
                //                    file->inode_num = (inodes_per_group * index) + i + 1;
                //                    file->file_size = (uint64_t)src->i_size | ((uint64_t)src->i_size_high << 32);
                //                    file->volume = this;
                //                    std::vector<uint64_t> file_blocks = file->get_data_blocks();
                //                    foreach(uint64_t b, file_blocks){
                //                        if (b < total_blocks && !test_umap_bit(umap.get(), b)){
                //                            set_umap_bit(umap.get(), b);
                //                        }
                //                    }
                //                }
                //            }
                //        }
                //    }
                //}
            }

           /* std::vector<uint64_t> blocks;
            std::vector<uint64_t> ublocks;

            get_filesystem_blocks(root, blocks);
            foreach(uint64_t b, blocks){
                if (b < total_blocks){
                    if (test_umap_bit(umap.get(), b)){

                    }
                    else{
                        ublocks.push_back(b);
                        set_umap_bit(umap.get(), b);
                    }
                }
            }*/

            uint64_t newBit, oldBit = 0;
            bool     dirty_range = false;
            for (newBit = 0; newBit < total_blocks; newBit++){
                if (umap.get()[newBit >> 3] & (1 << (newBit & 7))){
                    if (!dirty_range){
                        dirty_range = true;
                        oldBit = newBit;
                    }
                }
                else {
                    if (dirty_range){
                        ULONGLONG start = (oldBit * block_size) + offset;
                        ULONGLONG length = (newBit - oldBit) * block_size;
                        ranges.push_back(io_range(start, length));
                        dirty_range = false;
                    }
                }
            }
            if (dirty_range){
                ULONGLONG start = (oldBit * block_size) + offset;
                ULONGLONG length = (newBit - oldBit) * block_size;
                ranges.push_back(io_range(start, length));
            }
        }
    }
    return ranges;
}

volume::ptr ext2_volume::get(universal_disk_rw::ptr& rw, ULONGLONG _offset, ULONGLONG _size){
    EXT3_SUPER_BLOCK sblock;
    memset(&sblock, 0, sizeof(sblock));
    uint32_t number_of_sectors_read = 0;
    ULONGLONG relative_sectors = _offset / rw->sector_size();
    if (rw->sector_read(relative_sectors + 2, 2, &sblock, number_of_sectors_read)){
        if (sblock.s_magic == EXT2_SUPER_MAGIC){         
            if (ext2fs_has_feature_compression(&sblock)){
                LOG(LOG_LEVEL_WARNING, _T("File system compression is used which is not supported."));
            }
            else if (ext2fs_has_feature_bigalloc(&sblock)){
                LOG(LOG_LEVEL_WARNING, _T("File system bigalloc is used which is not supported."));
            }
            else{
                ext2_volume * vol = new ext2_volume(rw, _offset, _size);
                volume::ptr v = volume::ptr(vol);
                vol->is_64_bit = ext2fs_has_feature_64bit(&sblock);
                vol->is_big_alloc = ext2fs_has_feature_bigalloc(&sblock);
                vol->desc_size = EXT2_DESC_SIZE(&sblock) & ~7;
                vol->cluster_ratio_bits = sblock.s_obso_log_frag_size - sblock.s_log_block_size;
                vol->relative_sectors = relative_sectors;
                vol->block_size = EXT2_BLOCK_SIZE(&sblock);
                vol->inodes_per_group = EXT2_INODES_PER_GROUP(&sblock);
                vol->inode_size = EXT2_INODE_SIZE(&sblock);
                vol->volume_name = sblock.s_volume_name;
                vol->log_block_size = sblock.s_log_block_size;
                vol->log_cluster_size = sblock.s_obso_log_frag_size;
                vol->clusters_per_group = sblock.s_obso_frags_per_group;
                vol->total_blocks = ext2fs_blocks_count(&sblock);
                vol->total_groups = ext2fs_div64_ceil(
                    ext2fs_blocks_count(&sblock) - sblock.s_first_data_block,
                    EXT2_BLOCKS_PER_GROUP(&sblock));
                vol->blocks_per_group = EXT2_BLOCKS_PER_GROUP(&sblock);
                vol->first_data_block = sblock.s_first_data_block;
                GUID uuid;
                memcpy(&uuid, sblock.s_uuid, sizeof(GUID));
                vol->volume_uuid = uuid;
                LOG(LOG_LEVEL_INFO, _T("Block size %d, inp %d, inodesize %d"), vol->block_size, vol->inodes_per_group, vol->inode_size);

                int gSizes, gSizeb;		/* Size of total group desc in sectors */
                gSizeb = vol->desc_size * vol->total_groups;
                gSizes = (gSizeb / rw->sector_size()) + 1;
                vol->groups_desc = boost::shared_ptr<BYTE>(new BYTE[gSizes * rw->sector_size()]);
                memset(vol->groups_desc.get(), 0, gSizes * rw->sector_size());
                if (!rw->sector_read(relative_sectors + ((vol->block_size * (vol->first_data_block + 1)) / rw->sector_size()), gSizes, vol->groups_desc.get(), number_of_sectors_read)){
                    vol->groups_desc = NULL;
                }
                vol->root = vol->read_inode(EXT2_ROOT_INO);
                if (vol->root)
                    return v;
            }
        }
    }
    return NULL;
}