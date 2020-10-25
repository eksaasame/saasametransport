#pragma once
#ifndef xfs_H
#define xfs_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef signed __int8  __s8;
typedef signed __int16 __s16;
typedef signed __int32 __s32;
typedef signed __int64 __s64;

typedef unsigned __int8  __u8;
typedef unsigned __int16 __u16;
typedef unsigned __int32 __u32;
typedef unsigned __int64 __u64;

#ifdef __CHECKER__
#define __bitwise__ __attribute__((bitwise))
#else
#define __bitwise__
#endif
#define __bitwise __bitwise__

typedef __u16 __bitwise __le16;
typedef __u16 __bitwise __be16;
typedef __u32 __bitwise __le32;
typedef __u32 __bitwise __be32;
typedef __u64 __bitwise __le64;
typedef __u64 __bitwise __be64;
typedef __s64         xfs_off_t;      /* <file offset> type */
typedef __u64         xfs_ino_t;      /* <inode> type */
typedef __s64         xfs_daddr_t;    /* <disk address> type */
typedef char *        xfs_caddr_t;    /* <core address> type */
typedef __u32         xfs_dev_t;
 
typedef uint64_t      __uint64_t;
typedef uint32_t      __uint32_t;
typedef uint16_t      __uint16_t;
typedef uint8_t       __uint8_t;

typedef uint32_t      xfs_agblock_t;  /* blockno in alloc. group */
typedef uint32_t      xfs_extlen_t;   /* extent length in blocks */
typedef uint32_t      xfs_agnumber_t; /* allocation group number */
typedef int32_t       xfs_extnum_t;   /* # of extents in a file */
typedef int16_t       xfs_aextnum_t;  /* # extents in an attribute fork */
typedef int64_t       xfs_fsize_t;    /* bytes in a file */
typedef uint64_t      xfs_ufsize_t;   /* unsigned bytes in a file */
 
typedef int32_t       xfs_suminfo_t;  /* type of bitmap summary info */
typedef int32_t       xfs_rtword_t;   /* word type for bitmap manipulations */
 
typedef int64_t       xfs_lsn_t;      /* log sequence number */
typedef int32_t       xfs_tid_t;      /* transaction identifier */
 
typedef uint32_t      xfs_dablk_t;    /* dir/attr block number (in file) */
typedef uint32_t      xfs_dahash_t;   /* dir/attr hash value */
 
typedef uint16_t      xfs_prid_t;     /* prid_t truncated to 16bits in XFS */ 
/*
* These types are 64 bits on disk but are either 32 or 64 bits in memory.
* Disk based types:
*/
typedef uint64_t      xfs_dfsbno_t;   /* blockno in filesystem (agno|agbno) */
typedef uint64_t      xfs_drfsbno_t;  /* blockno in filesystem (raw) */
typedef uint64_t      xfs_drtbno_t;   /* extent (block) in realtime area */
typedef uint64_t      xfs_dfiloff_t;  /* block number in a file */
typedef uint64_t      xfs_dfilblks_t; /* number of blocks in a file */
 
/*
* Memory based types are conditional.
*/
#if XFS_BIG_BLKNOS
typedef uint64_t      xfs_fsblock_t;  /* blockno in filesystem (agno|agbno) */
typedef uint64_t      xfs_rfsblock_t; /* blockno in filesystem (raw) */
typedef uint64_t      xfs_rtblock_t;  /* extent (block) in realtime area */
typedef int64_t       xfs_srtblock_t; /* signed version of xfs_rtblock_t */
#else
typedef uint32_t      xfs_fsblock_t;  /* blockno in filesystem (agno|agbno) */
typedef uint32_t      xfs_rfsblock_t; /* blockno in filesystem (raw) */
typedef uint32_t      xfs_rtblock_t;  /* extent (block) in realtime area */
typedef int32_t       xfs_srtblock_t; /* signed version of xfs_rtblock_t */
#endif
typedef uint64_t      xfs_fileoff_t;  /* block number in a file */
typedef int64_t       xfs_sfiloff_t;  /* signed block number in a file */
typedef uint64_t      xfs_filblks_t;  /* number of blocks in a file */

typedef uint8_t       xfs_arch_t;     /* architecture of an xfs fs */ 

/*
* Null values for the types.
*/
#define NULLDFSBNO      ((xfs_dfsbno_t)-1)
#define NULLDRFSBNO     ((xfs_drfsbno_t)-1)
#define NULLDRTBNO      ((xfs_drtbno_t)-1)
#define NULLDFILOFF     ((xfs_dfiloff_t)-1)

#define NULLFSBLOCK     ((xfs_fsblock_t)-1)
#define NULLRFSBLOCK    ((xfs_rfsblock_t)-1)
#define NULLRTBLOCK     ((xfs_rtblock_t)-1)
#define NULLFILEOFF     ((xfs_fileoff_t)-1)
 
#define NULLAGBLOCK     ((xfs_agblock_t)-1)
#define NULLAGNUMBER    ((xfs_agnumber_t)-1)
#define NULLEXTNUM      ((xfs_extnum_t)-1) 
#define NULLCOMMITLSN   ((xfs_lsn_t)-1)

/*
* Max values for extlen, extnum, aextnum.
*/
#define MAXEXTLEN       ((xfs_extlen_t)0x001fffff)      /* 21 bits */
#define MAXEXTNUM       ((xfs_extnum_t)0x7fffffff)      /* signed int */
#define MAXAEXTNUM      ((xfs_aextnum_t)0x7fff)         /* signed short */

/*
* MAXNAMELEN is the length (including the terminating null) of
* the longest permissible file (component) name.
*/
#define MAXNAMELEN      256
 
typedef struct xfs_dirent {             /* data from readdir() */
         xfs_ino_t       d_ino;          /* inode number of entry */
         xfs_off_t       d_off;          /* offset of disk directory entry */
         unsigned short  d_reclen;       /* length of this record */
         char            d_name[1];      /* name of file */
} xfs_dirent_t;
 
#define DIRENTBASESIZE          (((xfs_dirent_t *)0)->d_name - (char *)0)
#define DIRENTSIZE(namelen)     \
        ((DIRENTBASESIZE + (namelen) + \
                sizeof(xfs_off_t)) & ~(sizeof(xfs_off_t) - 1))

/*
* MAXNAMELEN is the length (including the terminating null) of
* the longest permissible file (component) name.
*/
#define MAXNAMELEN	256

typedef enum {
    XFS_LOOKUP_EQi, XFS_LOOKUP_LEi, XFS_LOOKUP_GEi
} xfs_lookup_t;

typedef enum {
    XFS_BTNUM_BNOi, XFS_BTNUM_CNTi, XFS_BTNUM_RMAPi, XFS_BTNUM_BMAPi,
    XFS_BTNUM_INOi, XFS_BTNUM_FINOi, XFS_BTNUM_REFCi, XFS_BTNUM_MAX
} xfs_btnum_t;

/*
* Superblock - on disk version.  Must match the in core version above.
* Must be padded to 64 bit alignment.
*/
typedef struct xfs_dsb {
    __be32		sb_magicnum;	/* magic number == XFS_SB_MAGIC */
    __be32		sb_blocksize;	/* logical block size, bytes */
    __be64		sb_dblocks;	/* number of data blocks */
    __be64		sb_rblocks;	/* number of realtime blocks */
    __be64		sb_rextents;	/* number of realtime extents */
    uuid_t		sb_uuid;	/* user-visible file system unique id */
    __be64		sb_logstart;	/* starting block of log if internal */
    __be64		sb_rootino;	/* root inode number */
    __be64		sb_rbmino;	/* bitmap inode for realtime extents */
    __be64		sb_rsumino;	/* summary inode for rt bitmap */
    __be32		sb_rextsize;	/* realtime extent size, blocks */
    __be32		sb_agblocks;	/* size of an allocation group */
    __be32		sb_agcount;	/* number of allocation groups */
    __be32		sb_rbmblocks;	/* number of rt bitmap blocks */
    __be32		sb_logblocks;	/* number of log blocks */
    __be16		sb_versionnum;	/* header version == XFS_SB_VERSION */
    __be16		sb_sectsize;	/* volume sector size, bytes */
    __be16		sb_inodesize;	/* inode size, bytes */
    __be16		sb_inopblock;	/* inodes per block */
    char		sb_fname[12];	/* file system name */
    __u8		sb_blocklog;	/* log2 of sb_blocksize */
    __u8		sb_sectlog;	/* log2 of sb_sectsize */
    __u8		sb_inodelog;	/* log2 of sb_inodesize */
    __u8		sb_inopblog;	/* log2 of sb_inopblock */
    __u8		sb_agblklog;	/* log2 of sb_agblocks (rounded up) */
    __u8		sb_rextslog;	/* log2 of sb_rextents */
    __u8		sb_inprogress;	/* mkfs is in progress, don't mount */
    __u8		sb_imax_pct;	/* max % of fs for inode space */
    /* statistics */
    /*
    * These fields must remain contiguous.  If you really
    * want to change their layout, make sure you fix the
    * code in xfs_trans_apply_sb_deltas().
    */
    __be64		sb_icount;	/* allocated inodes */
    __be64		sb_ifree;	/* free inodes */
    __be64		sb_fdblocks;	/* free data blocks */
    __be64		sb_frextents;	/* free realtime extents */
    /*
    * End contiguous fields.
    */
    __be64		sb_uquotino;	/* user quota inode */
    __be64		sb_gquotino;	/* group quota inode */
    __be16		sb_qflags;	/* quota flags */
    __u8		sb_flags;	/* misc. flags */
    __u8		sb_shared_vn;	/* shared version number */
    __be32		sb_inoalignmt;	/* inode chunk alignment, fsblocks */
    __be32		sb_unit;	/* stripe or raid unit */
    __be32		sb_width;	/* stripe or raid width */
    __u8		sb_dirblklog;	/* log2 of dir block size (fsbs) */
    __u8		sb_logsectlog;	/* log2 of the log sector size */
    __be16		sb_logsectsize;	/* sector size for the log, bytes */
    __be32		sb_logsunit;	/* stripe unit size for the log */
    __be32		sb_features2;	/* additional feature bits */
    /*
    * bad features2 field as a result of failing to pad the sb
    * structure to 64 bits. Some machines will be using this field
    * for features2 bits. Easiest just to mark it bad and not use
    * it for anything else.
    */
    __be32		sb_bad_features2;

    /* version 5 superblock fields start here */

    /* feature masks */
    __be32		sb_features_compat;
    __be32		sb_features_ro_compat;
    __be32		sb_features_incompat;
    __be32		sb_features_log_incompat;

    __le32		sb_crc;		/* superblock crc */
    __be32		sb_spino_align;	/* sparse inode chunk alignment */

    __be64		sb_pquotino;	/* project quota inode */
    __be64		sb_lsn;		/* last write sequence */
    uuid_t		sb_meta_uuid;	/* metadata file system unique id */

    /* must be padded to 64 bit alignment */
} xfs_dsb_t;

/*
* Superblock - in core version.  Must match the ondisk version below.
* Must be padded to 64 bit alignment.
*/
typedef struct xfs_sb {
    __uint32_t	    sb_magicnum;	/* magic number == XFS_SB_MAGIC */
    __uint32_t	    sb_blocksize;	/* logical block size, bytes */
    xfs_rfsblock_t	sb_dblocks;	/* number of data blocks */
    xfs_rfsblock_t	sb_rblocks;	/* number of realtime blocks */
    xfs_rtblock_t	sb_rextents;	/* number of realtime extents */
    uuid_t		    sb_uuid;	/* user-visible file system unique id */
    xfs_fsblock_t	sb_logstart;	/* starting block of log if internal */
    xfs_ino_t	    sb_rootino;	/* root inode number */
    xfs_ino_t       sb_rbmino;	/* bitmap inode for realtime extents */
    xfs_ino_t       sb_rsumino;	/* summary inode for rt bitmap */
    xfs_agblock_t	sb_rextsize;	/* realtime extent size, blocks */
    xfs_agblock_t	sb_agblocks;	/* size of an allocation group */
    xfs_agnumber_t	sb_agcount;	/* number of allocation groups */
    xfs_extlen_t	sb_rbmblocks;	/* number of rt bitmap blocks */
    xfs_extlen_t	sb_logblocks;	/* number of log blocks */
    __uint16_t	sb_versionnum;	/* header version == XFS_SB_VERSION */
    __uint16_t	sb_sectsize;	/* volume sector size, bytes */
    __uint16_t	sb_inodesize;	/* inode size, bytes */
    __uint16_t	sb_inopblock;	/* inodes per block */
    char		sb_fname[12];	/* file system name */
    __uint8_t	sb_blocklog;	/* log2 of sb_blocksize */
    __uint8_t	sb_sectlog;	/* log2 of sb_sectsize */
    __uint8_t	sb_inodelog;	/* log2 of sb_inodesize */
    __uint8_t	sb_inopblog;	/* log2 of sb_inopblock */
    __uint8_t	sb_agblklog;	/* log2 of sb_agblocks (rounded up) */
    __uint8_t	sb_rextslog;	/* log2 of sb_rextents */
    __uint8_t	sb_inprogress;	/* mkfs is in progress, don't mount */
    __uint8_t	sb_imax_pct;	/* max % of fs for inode space */
    /* statistics */
    /*
    * These fields must remain contiguous.  If you really
    * want to change their layout, make sure you fix the
    * code in xfs_trans_apply_sb_deltas().
    */
    __uint64_t	sb_icount;	/* allocated inodes */
    __uint64_t	sb_ifree;	/* free inodes */
    __uint64_t	sb_fdblocks;	/* free data blocks */
    __uint64_t	sb_frextents;	/* free realtime extents */
    /*
    * End contiguous fields.
    */
    xfs_ino_t	sb_uquotino;	/* user quota inode */
    xfs_ino_t	sb_gquotino;	/* group quota inode */
    __uint16_t	sb_qflags;	/* quota flags */
    __uint8_t	sb_flags;	/* misc. flags */
    __uint8_t	sb_shared_vn;	/* shared version number */
    xfs_extlen_t	sb_inoalignmt;	/* inode chunk alignment, fsblocks */
    __uint32_t	sb_unit;	/* stripe or raid unit */
    __uint32_t	sb_width;	/* stripe or raid width */
    __uint8_t	sb_dirblklog;	/* log2 of dir block size (fsbs) */
    __uint8_t	sb_logsectlog;	/* log2 of the log sector size */
    __uint16_t	sb_logsectsize;	/* sector size for the log, bytes */
    __uint32_t	sb_logsunit;	/* stripe unit size for the log */
    __uint32_t	sb_features2;	/* additional feature bits */

    /*
    * bad features2 field as a result of failing to pad the sb structure to
    * 64 bits. Some machines will be using this field for features2 bits.
    * Easiest just to mark it bad and not use it for anything else.
    *
    * This is not kept up to date in memory; it is always overwritten by
    * the value in sb_features2 when formatting the incore superblock to
    * the disk buffer.
    */
    __uint32_t	sb_bad_features2;

    /* version 5 superblock fields start here */

    /* feature masks */
    __uint32_t	sb_features_compat;
    __uint32_t	sb_features_ro_compat;
    __uint32_t	sb_features_incompat;
    __uint32_t	sb_features_log_incompat;

    __uint32_t	sb_crc;		/* superblock crc */
    xfs_extlen_t	sb_spino_align;	/* sparse inode chunk alignment */

    xfs_ino_t	sb_pquotino;	/* project quota inode */
    xfs_lsn_t	sb_lsn;		/* last write sequence */
    uuid_t		sb_meta_uuid;	/* metadata file system unique id */

    /* must be padded to 64 bit alignment */
} xfs_sb_t;

#define	XFS_SB_VERSION_NUM(sbp)	((sbp)->sb_versionnum & XFS_SB_VERSION_NUMBITS)
/*
* Super block
* Fits into a sector-sized buffer at address 0 of each allocation group.
* Only the first of these is ever updated except during growfs.
*/
#define	XFS_SB_MAGIC		0x58465342	/* 'XFSB' */
#define	XFS_SB_VERSION_1	1		/* 5.3, 6.0.1, 6.1 */
#define	XFS_SB_VERSION_2	2		/* 6.2 - attributes */
#define	XFS_SB_VERSION_3	3		/* 6.2 - new inode version */
#define	XFS_SB_VERSION_4	4		/* 6.2+ - bitmask version */
#define	XFS_SB_VERSION_5	5		/* CRC enabled filesystem */
#define	XFS_SB_VERSION_NUMBITS		0x000f
#define	XFS_SB_VERSION_ALLFBITS		0xfff0
#define	XFS_SB_VERSION_ATTRBIT		0x0010
#define	XFS_SB_VERSION_NLINKBIT		0x0020
#define	XFS_SB_VERSION_QUOTABIT		0x0040
#define	XFS_SB_VERSION_ALIGNBIT		0x0080
#define	XFS_SB_VERSION_DALIGNBIT	0x0100
#define	XFS_SB_VERSION_SHAREDBIT	0x0200
#define XFS_SB_VERSION_LOGV2BIT		0x0400
#define XFS_SB_VERSION_SECTORBIT	0x0800
#define	XFS_SB_VERSION_EXTFLGBIT	0x1000
#define	XFS_SB_VERSION_DIRV2BIT		0x2000
#define	XFS_SB_VERSION_BORGBIT		0x4000	/* ASCII only case-insens. */
#define	XFS_SB_VERSION_MOREBITSBIT	0x8000

/*
* The size of a single extended attribute on disk is limited by
* the size of index values within the attribute entries themselves.
* These are be16 fields, so we can only support attribute data
* sizes up to 2^16 bytes in length.
*/
#define XFS_XATTR_SIZE_MAX (1 << 16)

/*
* Supported feature bit list is just all bits in the versionnum field because
* we've used them all up and understand them all. Except, of course, for the
* shared superblock bit, which nobody knows what it does and so is unsupported.
*/
#define	XFS_SB_VERSION_OKBITS		\
	((XFS_SB_VERSION_NUMBITS | XFS_SB_VERSION_ALLFBITS) & \
		~XFS_SB_VERSION_SHAREDBIT)

/*
* There are two words to hold XFS "feature" bits: the original
* word, sb_versionnum, and sb_features2.  Whenever a bit is set in
* sb_features2, the feature bit XFS_SB_VERSION_MOREBITSBIT must be set.
*
* These defines represent bits in sb_features2.
*/
#define XFS_SB_VERSION2_RESERVED1BIT	0x00000001
#define XFS_SB_VERSION2_LAZYSBCOUNTBIT	0x00000002	/* Superblk counters */
#define XFS_SB_VERSION2_RESERVED4BIT	0x00000004
#define XFS_SB_VERSION2_ATTR2BIT	0x00000008	/* Inline attr rework */
#define XFS_SB_VERSION2_PARENTBIT	0x00000010	/* parent pointers */
#define XFS_SB_VERSION2_PROJID32BIT	0x00000080	/* 32 bit project id */
#define XFS_SB_VERSION2_CRCBIT		0x00000100	/* metadata CRCs */
#define XFS_SB_VERSION2_FTYPE		0x00000200	/* inode type in dir */

#define	XFS_SB_VERSION2_OKBITS		\
	(XFS_SB_VERSION2_LAZYSBCOUNTBIT	| \
	 XFS_SB_VERSION2_ATTR2BIT	| \
	 XFS_SB_VERSION2_PROJID32BIT	| \
	 XFS_SB_VERSION2_FTYPE)

#define XFS_SB_FEAT_INCOMPAT_FTYPE	(1 << 0)	/* filetype in dirent */
#define XFS_SB_FEAT_INCOMPAT_SPINODES	(1 << 1)	/* sparse inode chunks */
#define XFS_SB_FEAT_INCOMPAT_META_UUID	(1 << 2)	/* metadata UUID */
#define XFS_SB_FEAT_INCOMPAT_ALL \
		(XFS_SB_FEAT_INCOMPAT_FTYPE|	\
		 XFS_SB_FEAT_INCOMPAT_SPINODES|	\
		 XFS_SB_FEAT_INCOMPAT_META_UUID)

#define XFS_SB_FEAT_INCOMPAT_UNKNOWN	~XFS_SB_FEAT_INCOMPAT_ALL
/*
* Null values for the types.
*/
#define	NULLFSBLOCK	((xfs_fsblock_t)-1)
#define	NULLRFSBLOCK	((xfs_rfsblock_t)-1)
#define	NULLRTBLOCK	((xfs_rtblock_t)-1)
#define	NULLFILEOFF	((xfs_fileoff_t)-1)

#define	NULLAGBLOCK	((xfs_agblock_t)-1)
#define	NULLAGNUMBER	((xfs_agnumber_t)-1)

#define NULLCOMMITLSN	((xfs_lsn_t)-1)

#define	NULLFSINO	((xfs_ino_t)-1)
#define	NULLAGINO	((xfs_agino_t)-1)

/*
* Max values for extlen, extnum, aextnum.
*/
#define	MAXEXTLEN	((xfs_extlen_t)0x001fffff)	/* 21 bits */
#define	MAXEXTNUM	((xfs_extnum_t)0x7fffffff)	/* signed int */
#define	MAXAEXTNUM	((xfs_aextnum_t)0x7fff)		/* signed short */

/*
* Minimum and maximum blocksize and sectorsize.
* The blocksize upper limit is pretty much arbitrary.
* The sectorsize upper limit is due to sizeof(sb_sectsize).
* CRC enable filesystems use 512 byte inodes, meaning 512 byte block sizes
* cannot be used.
*/
#define XFS_MIN_BLOCKSIZE_LOG	9	/* i.e. 512 bytes */
#define XFS_MAX_BLOCKSIZE_LOG	16	/* i.e. 65536 bytes */
#define XFS_MIN_BLOCKSIZE	(1 << XFS_MIN_BLOCKSIZE_LOG)
#define XFS_MAX_BLOCKSIZE	(1 << XFS_MAX_BLOCKSIZE_LOG)
#define XFS_MIN_CRC_BLOCKSIZE	(1 << (XFS_MIN_BLOCKSIZE_LOG + 1))
#define XFS_MIN_SECTORSIZE_LOG	9	/* i.e. 512 bytes */
#define XFS_MAX_SECTORSIZE_LOG	15	/* i.e. 32768 bytes */
#define XFS_MIN_SECTORSIZE	(1 << XFS_MIN_SECTORSIZE_LOG)
#define XFS_MAX_SECTORSIZE	(1 << XFS_MAX_SECTORSIZE_LOG)

/*
* Inode fork identifiers.
*/
#define	XFS_DATA_FORK	0
#define	XFS_ATTR_FORK	1
#define	XFS_COW_FORK	2

/*
* Min numbers of data/attr fork btree root pointers.
*/
#define MINDBTPTRS	3
#define MINABTPTRS	2

/*
* MAXNAMELEN is the length (including the terminating null) of
* the longest permissible file (component) name.
*/
#define MAXNAMELEN	256

/*
* Disk quotas status in m_qflags, and also sb_qflags. 16 bits.
*/
#define XFS_UQUOTA_ACCT	0x0001  /* user quota accounting ON */
#define XFS_UQUOTA_ENFD	0x0002  /* user quota limits enforced */
#define XFS_UQUOTA_CHKD	0x0004  /* quotacheck run on usr quotas */
#define XFS_PQUOTA_ACCT	0x0008  /* project quota accounting ON */
#define XFS_OQUOTA_ENFD	0x0010  /* other (grp/prj) quota limits enforced */
#define XFS_OQUOTA_CHKD	0x0020  /* quotacheck run on other (grp/prj) quotas */
#define XFS_GQUOTA_ACCT	0x0040  /* group quota accounting ON */

/*
* Conversion to and from the combined OQUOTA flag (if necessary)
* is done only in xfs_sb_qflags_to_disk() and xfs_sb_qflags_from_disk()
*/
#define XFS_GQUOTA_ENFD	0x0080  /* group quota limits enforced */
#define XFS_GQUOTA_CHKD	0x0100  /* quotacheck run on group quotas */
#define XFS_PQUOTA_ENFD	0x0200  /* project quota limits enforced */
#define XFS_PQUOTA_CHKD	0x0400  /* quotacheck run on project quotas */

#define XFS_ALL_QUOTA_ACCT	\
		(XFS_UQUOTA_ACCT | XFS_GQUOTA_ACCT | XFS_PQUOTA_ACCT)
#define XFS_ALL_QUOTA_ENFD	\
		(XFS_UQUOTA_ENFD | XFS_GQUOTA_ENFD | XFS_PQUOTA_ENFD)
#define XFS_ALL_QUOTA_CHKD	\
		(XFS_UQUOTA_CHKD | XFS_GQUOTA_CHKD | XFS_PQUOTA_CHKD)

#define XFS_MOUNT_QUOTA_ALL	(XFS_UQUOTA_ACCT|XFS_UQUOTA_ENFD|\
				 XFS_UQUOTA_CHKD|XFS_GQUOTA_ACCT|\
				 XFS_GQUOTA_ENFD|XFS_GQUOTA_CHKD|\
				 XFS_PQUOTA_ACCT|XFS_PQUOTA_ENFD|\
				 XFS_PQUOTA_CHKD)

/*
* Allocation group header
*
* This is divided into three structures, placed in sequential 512-byte
* buffers after a copy of the superblock (also in a 512-byte buffer).
*/
#define	XFS_AGF_MAGIC	0x58414746	/* 'XAGF' */
#define	XFS_AGI_MAGIC	0x58414749	/* 'XAGI' */
#define	XFS_AGFL_MAGIC	0x5841464c	/* 'XAFL' */
#define	XFS_AGF_VERSION	1
#define	XFS_AGI_VERSION	1

/*
* Btree number 0 is bno, 1 is cnt, 2 is rmap. This value gives the size of the
* arrays below.
*/
#define	XFS_BTNUM_AGF	((int)XFS_BTNUM_RMAPi + 1)

/*
* The second word of agf_levels in the first a.g. overlaps the EFS
* superblock's magic number.  Since the magic numbers valid for EFS
* are > 64k, our value cannot be confused for an EFS superblock's.
*/

typedef struct xfs_agf {
    /*
    * Common allocation group header information
    */
    __be32		agf_magicnum;	/* magic number == XFS_AGF_MAGIC */
    __be32		agf_versionnum;	/* header version == XFS_AGF_VERSION */
    __be32		agf_seqno;	/* sequence # starting from 0 */
    __be32		agf_length;	/* size in blocks of a.g. */
    /*
    * Freespace and rmap information
    */
    __be32		agf_roots[XFS_BTNUM_AGF];	/* root blocks */
    __be32		agf_levels[XFS_BTNUM_AGF];	/* btree levels */

    __be32		agf_flfirst;	/* first freelist block's index */
    __be32		agf_fllast;	/* last freelist block's index */
    __be32		agf_flcount;	/* count of blocks in freelist */
    __be32		agf_freeblks;	/* total free blocks */

    __be32		agf_longest;	/* longest free space */
    __be32		agf_btreeblks;	/* # of blocks held in AGF btrees */
    uuid_t		agf_uuid;	/* uuid of filesystem */

    __be32		agf_rmap_blocks;	/* rmapbt blocks used */
    __be32		agf_refcount_blocks;	/* refcountbt blocks used */

    __be32		agf_refcount_root;	/* refcount tree root block */
    __be32		agf_refcount_level;	/* refcount btree levels */

    /*
    * reserve some contiguous space for future logged fields before we add
    * the unlogged fields. This makes the range logging via flags and
    * structure offsets much simpler.
    */
    __be64		agf_spare64[14];

    /* unlogged fields, written during buffer writeback. */
    __be64		agf_lsn;	/* last write sequence */
    __be32		agf_crc;	/* crc of agf sector */
    __be32		agf_spare2;

    /* structure must be padded to 64 bit alignment */
} xfs_agf_t;

/*
* Size of the unlinked inode hash table in the agi.
*/
#define	XFS_AGI_UNLINKED_BUCKETS	64

typedef struct xfs_agi {
    /*
    * Common allocation group header information
    */
    __be32		agi_magicnum;	/* magic number == XFS_AGI_MAGIC */
    __be32		agi_versionnum;	/* header version == XFS_AGI_VERSION */
    __be32		agi_seqno;	/* sequence # starting from 0 */
    __be32		agi_length;	/* size in blocks of a.g. */
    /*
    * Inode information
    * Inodes are mapped by interpreting the inode number, so no
    * mapping data is needed here.
    */
    __be32		agi_count;	/* count of allocated inodes */
    __be32		agi_root;	/* root of inode btree */
    __be32		agi_level;	/* levels in inode btree */
    __be32		agi_freecount;	/* number of free inodes */

    __be32		agi_newino;	/* new inode just allocated */
    __be32		agi_dirino;	/* last directory inode chunk */
    /*
    * Hash table of inodes which have been unlinked but are
    * still being referenced.
    */
    __be32		agi_unlinked[XFS_AGI_UNLINKED_BUCKETS];
    /*
    * This marks the end of logging region 1 and start of logging region 2.
    */
    uuid_t		agi_uuid;	/* uuid of filesystem */
    __be32		agi_crc;	/* crc of agi sector */
    __be32		agi_pad32;
    __be64		agi_lsn;	/* last write sequence */

    __be32		agi_free_root; /* root of the free inode btree */
    __be32		agi_free_level;/* levels in free inode btree */

    /* structure must be padded to 64 bit alignment */
} xfs_agi_t;

/*
* Allocation Btree format definitions
*
* There are two on-disk btrees, one sorted by blockno and one sorted
* by blockcount and blockno.  All blocks look the same to make the code
* simpler; if we have time later, we'll make the optimizations.
*/
#define	XFS_ABTB_MAGIC		0x41425442	/* 'ABTB' for bno tree */
#define	XFS_ABTB_CRC_MAGIC	0x41423342	/* 'AB3B' */
#define	XFS_ABTC_MAGIC		0x41425443	/* 'ABTC' for cnt tree */
#define	XFS_ABTC_CRC_MAGIC	0x41423343	/* 'AB3C' */

/*
* Data record/key structure
*/
typedef struct xfs_alloc_rec {
    __be32		ar_startblock;	/* starting block number */
    __be32		ar_blockcount;	/* count of free blocks */
} xfs_alloc_rec_t, xfs_alloc_key_t;

typedef struct xfs_alloc_rec_incore {
    xfs_agblock_t	ar_startblock;	/* starting block number */
    xfs_extlen_t	ar_blockcount;	/* count of free blocks */
} xfs_alloc_rec_incore_t;

/* btree pointer type */
typedef __be32 xfs_alloc_ptr_t;

/*
* Generic Btree block format definitions
*
* This is a combination of the actual format used on disk for short and long
* format btrees.  The first three fields are shared by both format, but the
* pointers are different and should be used with care.
*
* To get the size of the actual short or long form headers please use the size
* macros below.  Never use sizeof(xfs_btree_block).
*
* The blkno, crc, lsn, owner and uuid fields are only available in filesystems
* with the crc feature bit, and all accesses to them must be conditional on
* that flag.
*/
/* short form block header */
struct xfs_btree_block_shdr {
    __be32		bb_leftsib;
    __be32		bb_rightsib;

    __be64		bb_blkno;
    __be64		bb_lsn;
    uuid_t		bb_uuid;
    __be32		bb_owner;
    __le32		bb_crc;
};

/* long form block header */
struct xfs_btree_block_lhdr {
    __be64		bb_leftsib;
    __be64		bb_rightsib;

    __be64		bb_blkno;
    __be64		bb_lsn;
    uuid_t		bb_uuid;
    __be64		bb_owner;
    __le32		bb_crc;
    __be32		bb_pad; /* padding for alignment */
};

struct xfs_btree_block {
    __be32		bb_magic;	/* magic number for block type */
    __be16		bb_level;	/* 0 is a leaf */
    __be16		bb_numrecs;	/* current # of data records */
    union {
        struct xfs_btree_block_shdr s;
        struct xfs_btree_block_lhdr l;
    } bb_u;				/* rest */
};

/* size of a short form block */
#define XFS_BTREE_SBLOCK_LEN \
	(offsetof(struct xfs_btree_block, bb_u) + \
	 offsetof(struct xfs_btree_block_shdr, bb_blkno))
/* size of a long form block */
#define XFS_BTREE_LBLOCK_LEN \
	(offsetof(struct xfs_btree_block, bb_u) + \
	 offsetof(struct xfs_btree_block_lhdr, bb_blkno))

/* sizes of CRC enabled btree blocks */
#define XFS_BTREE_SBLOCK_CRC_LEN \
	(offsetof(struct xfs_btree_block, bb_u) + \
	 sizeof(struct xfs_btree_block_shdr))
#define XFS_BTREE_LBLOCK_CRC_LEN \
	(offsetof(struct xfs_btree_block, bb_u) + \
	 sizeof(struct xfs_btree_block_lhdr))

#define XFS_BTREE_SBLOCK_CRC_OFF \
	offsetof(struct xfs_btree_block, bb_u.s.bb_crc)
#define XFS_BTREE_LBLOCK_CRC_OFF \
	offsetof(struct xfs_btree_block, bb_u.l.bb_crc)

/*
* Btree block header size depends on a superblock flag.
*/
#define XFS_ALLOC_BLOCK_LEN(mp) \
	(xfs_sb_version_hascrc(&((mp)->sbp)) ? \
		XFS_BTREE_SBLOCK_CRC_LEN : XFS_BTREE_SBLOCK_LEN)

/*
* Record, key, and pointer address macros for btree blocks.
*
* (note that some of these may appear unused, but they are used in userspace)
*/
#define XFS_ALLOC_REC_ADDR(mp, block, index) \
	((xfs_alloc_rec_t *) \
		((char *)(block) + \
		 XFS_ALLOC_BLOCK_LEN(mp) + \
		 (((index) - 1) * sizeof(xfs_alloc_rec_t))))

#define XFS_ALLOC_KEY_ADDR(mp, block, index) \
	((xfs_alloc_key_t *) \
		((char *)(block) + \
		 XFS_ALLOC_BLOCK_LEN(mp) + \
		 ((index) - 1) * sizeof(xfs_alloc_key_t)))

#define XFS_ALLOC_PTR_ADDR(mp, block, index, maxrecs) \
	((xfs_alloc_ptr_t *) \
		((char *)(block) + \
		 XFS_ALLOC_BLOCK_LEN(mp) + \
		 (maxrecs) * sizeof(xfs_alloc_key_t) + \
		 ((index) - 1) * sizeof(xfs_alloc_ptr_t)))

#define	XFS_BTREE_MAXLEVELS	8	/* max of all btrees */
#define	BBSHIFT		9
#define	BBSIZE		(1<<BBSHIFT)
/*
* Size of the AGFL.  For CRC-enabled filesystes we steal a couple of
* slots in the beginning of the block for a proper header with the
* location information and CRC.
*/
#define XFS_AGFL_SIZE(mp) \
	(((mp)->sbp.sb_sectsize - \
	 (xfs_sb_version_hascrc(&((mp)->sbp)) ? \
		sizeof(struct xfs_agfl) : 0)) / \
	  sizeof(xfs_agblock_t))

typedef struct xfs_agfl {
    __be32		agfl_magicnum;
    __be32		agfl_seqno;
    uuid_t		agfl_uuid;
    __be64		agfl_lsn;
    __be32		agfl_crc;
    __be32		agfl_bno[1];	/* actually XFS_AGFL_SIZE(mp) */
}xfs_agfl_t;

#ifdef __cplusplus
}
#endif

#include "linuxfs_parser.h"

class xfs_volume : public linuxfs::volume{
public:
    static linuxfs::volume::ptr get(universal_disk_rw::ptr& rw, ULONGLONG _offset, ULONGLONG _size);
    virtual linuxfs::io_range::vtr      file_system_ranges();
private:
    xfs_volume(universal_disk_rw::ptr &_rw, ULONGLONG _offset, ULONGLONG _size) : volume(_rw, _offset, _size){
        memset(&sbp, 0, sizeof(xfs_sb));
    }
    static void xfs_sb_from_disk(struct xfs_sb	*to, xfs_dsb_t *from, bool convert_xquota = true);
    static void xfs_sb_quota_from_disk(struct xfs_sb *sbp);
    static void xfs_agf_from_disk(struct xfs_sb	*sbp, xfs_agf_t* to, xfs_agf_t* from);
    static void xfs_agi_from_disk(xfs_agi_t* to, xfs_agi_t* from);

    void scanfunc(__uint32_t block, int level, xfs_agf_t *agf, linuxfs::io_range::vtr &excludes);

    /*
    * XFS_SB_FEAT_INCOMPAT_META_UUID indicates that the metadata UUID
    * is stored separately from the user-visible UUID; this allows the
    * user-visible UUID to be changed on V5 filesystems which have a
    * filesystem UUID stamped into every piece of metadata.
    */
    static inline int xfs_sb_version_hascrc(struct xfs_sb *sbp)
    {
        return XFS_SB_VERSION_NUM(sbp) == XFS_SB_VERSION_5;
    }

    static inline bool xfs_sb_version_hasmetauuid(struct xfs_sb *sbp)
    {
        return (XFS_SB_VERSION_NUM(sbp) == XFS_SB_VERSION_5) &&
            (sbp->sb_features_incompat & XFS_SB_FEAT_INCOMPAT_META_UUID);
    }

    static inline int xfs_sb_version_has_pquotino(struct xfs_sb *sbp)
    {
        return XFS_SB_VERSION_NUM(sbp) == XFS_SB_VERSION_5;
    }
    /*
    * Calculate number of records in an alloc btree block.
    */
    static inline int xfs_allocbt_maxrecs(
        xfs_volume	*mp,
        int			blocklen,
        int			leaf)
    {
        blocklen -= XFS_ALLOC_BLOCK_LEN(mp);
        if (leaf)
            return blocklen / sizeof(xfs_alloc_rec_t);
        return blocklen / (sizeof(xfs_alloc_key_t) + sizeof(xfs_alloc_ptr_t));
    }
    xfs_sb_t  sbp;
};

#endif