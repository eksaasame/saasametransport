#include "xfs.h"
#include <stdlib.h>

#define be16_to_cpu(x)  __builtin_bswap16((x))
#define be32_to_cpu(x)	__builtin_bswap32((x))
#define be64_to_cpu(x)	__builtin_bswap64((x))

using namespace linuxfs;

void xfs_volume::xfs_sb_quota_from_disk(struct xfs_sb *sbp){
    /*
    * older mkfs doesn't initialize quota inodes to NULLFSINO. This
    * leads to in-core values having two different values for a quota
    * inode to be invalid: 0 and NULLFSINO. Change it to a single value
    * NULLFSINO.
    *
    * Note that this change affect only the in-core values. These
    * values are not written back to disk unless any quota information
    * is written to the disk. Even in that case, sb_pquotino field is
    * not written to disk unless the superblock supports pquotino.
    */
    if (sbp->sb_uquotino == 0)
        sbp->sb_uquotino = NULLFSINO;
    if (sbp->sb_gquotino == 0)
        sbp->sb_gquotino = NULLFSINO;
    if (sbp->sb_pquotino == 0)
        sbp->sb_pquotino = NULLFSINO;

    /*
    * We need to do these manipilations only if we are working
    * with an older version of on-disk superblock.
    */
    if (xfs_sb_version_has_pquotino(sbp))
        return;

    if (sbp->sb_qflags & XFS_OQUOTA_ENFD)
        sbp->sb_qflags |= (sbp->sb_qflags & XFS_PQUOTA_ACCT) ?
    XFS_PQUOTA_ENFD : XFS_GQUOTA_ENFD;
    if (sbp->sb_qflags & XFS_OQUOTA_CHKD)
        sbp->sb_qflags |= (sbp->sb_qflags & XFS_PQUOTA_ACCT) ?
    XFS_PQUOTA_CHKD : XFS_GQUOTA_CHKD;
    sbp->sb_qflags &= ~(XFS_OQUOTA_ENFD | XFS_OQUOTA_CHKD);

    if (sbp->sb_qflags & XFS_PQUOTA_ACCT &&
        sbp->sb_gquotino != NULLFSINO)  {
        /*
        * In older version of superblock, on-disk superblock only
        * has sb_gquotino, and in-core superblock has both sb_gquotino
        * and sb_pquotino. But, only one of them is supported at any
        * point of time. So, if PQUOTA is set in disk superblock,
        * copy over sb_gquotino to sb_pquotino.  The NULLFSINO test
        * above is to make sure we don't do this twice and wipe them
        * both out!
        */
        sbp->sb_pquotino = sbp->sb_gquotino;
        sbp->sb_gquotino = NULLFSINO;
    }
}

void xfs_volume::xfs_sb_from_disk(struct xfs_sb	*to, xfs_dsb_t *from, bool convert_xquota){
    to->sb_magicnum = be32_to_cpu(from->sb_magicnum);
    LOG_TRACE("to->sb_magicnum = %u,from->sb_magicnum = %u", to->sb_magicnum, from->sb_magicnum);
    to->sb_blocksize = be32_to_cpu(from->sb_blocksize);
    LOG_TRACE("to->sb_blocksize = %u,from->sb_blocksize = %u", to->sb_blocksize, from->sb_blocksize);
    to->sb_dblocks = be64_to_cpu(from->sb_dblocks);
    LOG_TRACE("to->sb_dblocks = %llu,from->sb_dblocks = %llu", to->sb_dblocks, from->sb_dblocks);

    to->sb_rblocks = be64_to_cpu(from->sb_rblocks);
    to->sb_rextents = be64_to_cpu(from->sb_rextents);
    memcpy(&to->sb_uuid, &from->sb_uuid, sizeof(to->sb_uuid));
    to->sb_logstart = be64_to_cpu(from->sb_logstart);
    to->sb_rootino = be64_to_cpu(from->sb_rootino);
    to->sb_rbmino = be64_to_cpu(from->sb_rbmino);
    to->sb_rsumino = be64_to_cpu(from->sb_rsumino);
    to->sb_rextsize = be32_to_cpu(from->sb_rextsize);
    to->sb_agblocks = be32_to_cpu(from->sb_agblocks);
    LOG_TRACE("to->sb_agblocks = %u,from->sb_agblocks = %u", to->sb_agblocks, from->sb_agblocks);
    to->sb_agcount = be32_to_cpu(from->sb_agcount);
    LOG_TRACE("to->sb_agcount = %u,from->sb_agcount = %u", to->sb_agcount, from->sb_agcount);
    to->sb_rbmblocks = be32_to_cpu(from->sb_rbmblocks);
    LOG_TRACE("to->sb_rbmblocks = %u,from->sb_rbmblocks = %u", to->sb_rbmblocks, from->sb_rbmblocks);
    to->sb_logblocks = be32_to_cpu(from->sb_logblocks);
    LOG_TRACE("to->sb_logblocks = %u,from->sb_logblocks = %u", to->sb_logblocks, from->sb_logblocks);
    to->sb_versionnum = be16_to_cpu(from->sb_versionnum);
    LOG_TRACE("to->sb_versionnum = %hu,from->sb_versionnum = %hu", to->sb_versionnum, from->sb_versionnum);
    to->sb_sectsize = be16_to_cpu(from->sb_sectsize);
    LOG_TRACE("to->sb_sectsize = %hu,from->sb_sectsize = %hu", to->sb_sectsize, from->sb_sectsize);
    to->sb_inodesize = be16_to_cpu(from->sb_inodesize);
    to->sb_inopblock = be16_to_cpu(from->sb_inopblock);
    memcpy(&to->sb_fname, &from->sb_fname, sizeof(to->sb_fname));
    to->sb_blocklog = from->sb_blocklog;
    to->sb_sectlog = from->sb_sectlog;
    to->sb_inodelog = from->sb_inodelog;
    to->sb_inopblog = from->sb_inopblog;
    to->sb_agblklog = from->sb_agblklog;
    to->sb_rextslog = from->sb_rextslog;
    to->sb_inprogress = from->sb_inprogress;
    to->sb_imax_pct = from->sb_imax_pct;
    to->sb_icount = be64_to_cpu(from->sb_icount);
    to->sb_ifree = be64_to_cpu(from->sb_ifree);
    to->sb_fdblocks = be64_to_cpu(from->sb_fdblocks);
    to->sb_frextents = be64_to_cpu(from->sb_frextents);
    to->sb_uquotino = be64_to_cpu(from->sb_uquotino);
    to->sb_gquotino = be64_to_cpu(from->sb_gquotino);
    to->sb_qflags = be16_to_cpu(from->sb_qflags);
    to->sb_flags = from->sb_flags;
    to->sb_shared_vn = from->sb_shared_vn;
    to->sb_inoalignmt = be32_to_cpu(from->sb_inoalignmt);
    to->sb_unit = be32_to_cpu(from->sb_unit);
    to->sb_width = be32_to_cpu(from->sb_width);
    to->sb_dirblklog = from->sb_dirblklog;
    to->sb_logsectlog = from->sb_logsectlog;
    to->sb_logsectsize = be16_to_cpu(from->sb_logsectsize);
    to->sb_logsunit = be32_to_cpu(from->sb_logsunit);
    to->sb_features2 = be32_to_cpu(from->sb_features2);
    to->sb_bad_features2 = be32_to_cpu(from->sb_bad_features2);
    to->sb_features_compat = be32_to_cpu(from->sb_features_compat);
    to->sb_features_ro_compat = be32_to_cpu(from->sb_features_ro_compat);
    to->sb_features_incompat = be32_to_cpu(from->sb_features_incompat);
    to->sb_features_log_incompat =
        be32_to_cpu(from->sb_features_log_incompat);
    /* crc is only used on disk, not in memory; just init to 0 here. */
    to->sb_crc = 0;
    to->sb_spino_align = be32_to_cpu(from->sb_spino_align);
    to->sb_pquotino = be64_to_cpu(from->sb_pquotino);
    to->sb_lsn = be64_to_cpu(from->sb_lsn);
    /*
    * sb_meta_uuid is only on disk if it differs from sb_uuid and the
    * feature flag is set; if not set we keep it only in memory.
    */
    if (xfs_sb_version_hasmetauuid(to))
        memcpy(&to->sb_meta_uuid, &from->sb_meta_uuid, sizeof(to->sb_meta_uuid));
    else
        memcpy(&to->sb_meta_uuid, &from->sb_uuid, sizeof(to->sb_meta_uuid));

    /* Convert on-disk flags to in-memory flags? */
    if (convert_xquota)
        xfs_sb_quota_from_disk(to);
}

void xfs_volume::xfs_agf_from_disk(struct xfs_sb* sbp, xfs_agf_t* to, xfs_agf_t* from){
    memset(to, 0, sizeof(xfs_agf_t));
    to->agf_magicnum = be32_to_cpu(from->agf_magicnum);
    to->agf_versionnum = be32_to_cpu(from->agf_versionnum);
    to->agf_seqno = be32_to_cpu(from->agf_seqno);
    to->agf_length = be32_to_cpu(from->agf_length);
    to->agf_roots[0] = be32_to_cpu(from->agf_roots[0]);
    to->agf_roots[1] = be32_to_cpu(from->agf_roots[1]);
    to->agf_levels[0] = be32_to_cpu(from->agf_levels[0]);
    to->agf_levels[1] = be32_to_cpu(from->agf_levels[1]);
    to->agf_flfirst = be32_to_cpu(from->agf_flfirst);
    to->agf_fllast = be32_to_cpu(from->agf_fllast);
    to->agf_flcount = be32_to_cpu(from->agf_flcount);
    to->agf_freeblks = be32_to_cpu(from->agf_freeblks);
    to->agf_longest = be32_to_cpu(from->agf_longest);
    if (sbp->sb_features2 & XFS_SB_VERSION2_LAZYSBCOUNTBIT)
        to->agf_btreeblks = be32_to_cpu(from->agf_btreeblks);
}

void xfs_volume::xfs_agi_from_disk(xfs_agi_t* to, xfs_agi_t* from){
    memset(to, 0, sizeof(xfs_agi_t));
    to->agi_magicnum = be32_to_cpu(from->agi_magicnum);
    to->agi_versionnum = be32_to_cpu(from->agi_versionnum);
    to->agi_seqno = be32_to_cpu(from->agi_seqno);
    to->agi_length = be32_to_cpu(from->agi_length);
    to->agi_count = be32_to_cpu(from->agi_count);
    to->agi_root = be32_to_cpu(from->agi_root);
    to->agi_level = be32_to_cpu(from->agi_level);
    to->agi_freecount = be32_to_cpu(from->agi_freecount);
    to->agi_newino = be32_to_cpu(from->agi_newino);
    to->agi_dirino = be32_to_cpu(from->agi_dirino);
    for (int i = 0; i < XFS_AGI_UNLINKED_BUCKETS; i++)
        to->agi_unlinked[i] = be32_to_cpu(from->agi_unlinked[i]);
    to->agi_crc = be32_to_cpu(from->agi_crc);
    to->agi_pad32 = be32_to_cpu(from->agi_pad32);
    to->agi_lsn = be32_to_cpu(from->agi_lsn);
    to->agi_free_root = be32_to_cpu(from->agi_free_root);
    to->agi_free_level = be32_to_cpu(from->agi_free_level);
    memcpy(&to->agi_uuid, &from->agi_uuid, sizeof(uuid_t));
}

void xfs_volume::scanfunc(__uint32_t block, int level, xfs_agf_t *agf, linuxfs::io_range::vtr &excludes){
    int _level = level - 1;
    size_t buff_size = sbp.sb_blocksize;
    std::unique_ptr<BYTE> buff = std::unique_ptr<BYTE>(new BYTE[buff_size]);
    uint32_t number_of_bytes_read = 0;

    memset(buff.get(), 0, buff_size);
    uint64_t _offset = offset + ((uint64_t)agf->agf_seqno * sbp.sb_agblocks * sbp.sb_blocksize);
    if (rw->read(_offset + sbp.sb_blocksize * block, buff_size, buff.get(), number_of_bytes_read)){
        xfs_btree_block* pblock = (xfs_btree_block*)buff.get();
        uint32_t bb_magic = be32_to_cpu(pblock->bb_magic);
        if (bb_magic == XFS_ABTB_MAGIC || bb_magic == XFS_ABTB_CRC_MAGIC){
            if (0 == _level){
                xfs_alloc_rec_t *rp = XFS_ALLOC_REC_ADDR(this, pblock, 1);
                for (int i = 0; i < be16_to_cpu(pblock->bb_numrecs); i++){
                    __uint64_t startblock = (__uint64_t)be32_to_cpu(rp[i].ar_startblock);
                    __uint64_t blockcount = (__uint64_t)be32_to_cpu(rp[i].ar_blockcount);
                    excludes.push_back(linuxfs::io_range(_offset + (startblock * sbp.sb_blocksize), (blockcount * sbp.sb_blocksize)));
                }
            }
            else{
                int mxr = xfs_allocbt_maxrecs(this, sbp.sb_blocksize, 0);
                xfs_alloc_ptr_t	*pp = XFS_ALLOC_PTR_ADDR(this, pblock, 1, mxr);
                for (int i = 0; i < be16_to_cpu(pblock->bb_numrecs); i++)
                    scanfunc(be32_to_cpu(pp[i]), be16_to_cpu(pblock->bb_level), agf, excludes);
            }
        }
    }
}

int xfs_volume::get_block_size() {
    return sbp.sb_blocksize;
}

uint64_t xfs_volume::get_sb_agblocks() {
    return sbp.sb_agblocks;
}

io_range::vtr xfs_volume::file_system_ranges(){
    FUN_TRACE;
    io_range::vtr ranges;
    linuxfs::io_range::vtr excludes;
    size_t buff_size = sbp.sb_sectsize * 8;
    std::unique_ptr<BYTE> buff = std::unique_ptr<BYTE>(new BYTE[buff_size]);
    for (uint64_t index = 0; index < sbp.sb_agcount; index++){
        memset(buff.get(), 0, buff_size);
        uint32_t number_of_bytes_read = 0;
        uint64_t _offset = offset + (index * sbp.sb_agblocks * sbp.sb_blocksize);
        if (rw->read(_offset, buff_size, buff.get(), number_of_bytes_read)){
            LOG_TRACE("_offset = %llu, buff_size = %u, number_of_bytes_read = %u\r\n", _offset, buff_size, number_of_bytes_read);
            xfs_sb_t  _sbp;
            xfs_sb_from_disk(&_sbp, (xfs_dsb_t *)buff.get());
            LOG_TRACE("_sbp.sb_magicnum = %llu\r\n",_sbp.sb_magicnum);
            if (_sbp.sb_magicnum == XFS_SB_MAGIC){
                xfs_agf_t _agf;
                xfs_agf_from_disk(&sbp, &_agf, (xfs_agf_t *)&(buff.get()[sbp.sb_sectsize]));
                LOG_TRACE("_agf.agf_magicnum = %llu\r\n", _agf.agf_magicnum);
                if (_agf.agf_magicnum == XFS_AGF_MAGIC){      
                    scanfunc(_agf.agf_roots[0], _agf.agf_levels[0], &_agf, excludes);                    
                    if (_agf.agf_flcount){
                        __be32		*freelist;
                        xfs_agfl_t * _agfl = (xfs_agfl_t *)(&(buff.get()[sbp.sb_sectsize * 3]));
                        /* open coded XFS_BUF_TO_AGFL_BNO */
                        freelist = xfs_sb_version_hascrc(&(sbp)) ? &_agfl->agfl_bno[0] : (__be32 *)_agfl;
                        int	i = _agf.agf_flfirst;
                        for (;;) {
                            excludes.push_back(linuxfs::io_range(_offset + ((__uint64_t)be32_to_cpu(freelist[i]) * sbp.sb_blocksize), sbp.sb_blocksize));
                            if (i == _agf.agf_fllast)
                                break;
                            if (++i == XFS_AGFL_SIZE(this))
                                i = 0;
                        }
                    }
                }
               /* xfs_agi_t _agi;
                xfs_agi_from_disk(&_agi, (xfs_agi_t *)&(buff.get()[sbp.sb_sectsize*2]));
                if (_agi.agi_magicnum == XFS_AGI_MAGIC){
                    for (int i = 0; i < XFS_AGI_UNLINKED_BUCKETS; i++){
                        if (_agi.agi_unlinked[i] > 0){

                        }
                    }
                }*/
            }            
        }
    }
    uint64_t next_start = offset;
    std::sort(excludes.begin(), excludes.end(), linuxfs::io_range::compare());
    for(linuxfs::io_range &ex : excludes){
        if (ex.start > next_start)
        {
            LOG_TRACE("start = %llu,length=%llu,rw->path()=%s\r\n", next_start, ex.start - next_start, rw->path().c_str());
            ranges.push_back(linuxfs::io_range(next_start, ex.start - next_start, next_start, rw));
        }
        next_start = ex.start + ex.length;
    }
    if (next_start < total_size){
        LOG_TRACE("start = %llu,length=%llu,rw->path()=%s\r\n", next_start, total_size - next_start, rw->path().c_str());
        ranges.push_back(linuxfs::io_range(next_start, total_size - next_start, next_start,rw));
    }
    return ranges;
}

volume::ptr xfs_volume::get(universal_disk_rw::ptr& rw, ULONGLONG _offset, ULONGLONG _size){
    LOG_TRACE("rw->sector_size() = %llu\r\n", rw->sector_size());
    size_t buff_size = rw->sector_size() * 8;

    LOG_TRACE("buff_size = %llu\r\n", buff_size);

    std::unique_ptr<BYTE> buff = std::unique_ptr<BYTE>(new BYTE[buff_size]);
    memset(buff.get(), 0, buff_size);
    uint32_t number_of_sectors_read = 0;
    ULONGLONG relative_sectors = _offset / rw->sector_size();
    if (rw->sector_read(relative_sectors, 1, buff.get(), number_of_sectors_read)){
        LOG_TRACE("relative_sectors = %llu,_offset = %llu, rw->sector_size() = %d,number_of_sectors_read = %u\r\n", relative_sectors, _offset, rw->sector_size(), number_of_sectors_read);
        xfs_dsb_t *sblock = (xfs_dsb_t *)buff.get();
        xfs_volume * vol = new xfs_volume(rw, _offset, _size);
        volume::ptr v = volume::ptr(vol);
        if (v){
            xfs_sb_from_disk(&vol->sbp, sblock);
            LOG_TRACE("vol->sbp.sb_magicnum = %llu\r\n", vol->sbp.sb_magicnum);
            if (vol->sbp.sb_magicnum == XFS_SB_MAGIC){
                LOG_TRACE("vol->sbp.sb_magicnum == XFS_SB_MAGIC\r\n");
                return v;
            }
        }
    }
    return NULL;
}
