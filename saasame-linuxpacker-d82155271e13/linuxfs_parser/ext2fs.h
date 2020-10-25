#pragma once
#ifndef ext2fs_H
#define ext2fs_H

#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#define EXT2_DEFAULT_PREALLOC_BLOCKS	8

/* Special Inode Numbers  */
#define EXT2_BAD_INO			1
#define EXT2_ROOT_INO			2
#define EXT2_ACL_IDX_INO		3
#define EXT2_ACL_DATA_INO		4
#define EXT2_BOOT_LOADER_INO	5
#define EXT2_UNDEL_DIR_INO		6

#define EXT2_GOOD_OLD_FIRST_INO	11

#define EXT2_SUPER_MAGIC		0xEF53	/* EXT2 Fs Magic Number */

#define EXT2_LINK_MAX			32000	/* Max count of links to the file */

#define EXT2_LINKLEN_IN_INODE   60

/* Block Size Management */
#define EXT2_MIN_BLOCK_SIZE		1024
#define EXT2_MAX_BLOCK_SIZE		4096
#define EXT2_MIN_BLOCK_LOG_SIZE 10

/* EXT2 Fragment Sizes */
#define EXT2_MIN_FRAG_SIZE		1024
#define EXT2_MAX_FRAG_SIZE		4096
#define EXT2_MIN_FRAG_LOG_SIZE	10

#define EXT2_FRAG_SIZE(s)		(EXT2_MIN_FRAG_SIZE << (s)->s_log_frag_size)
#define EXT2_FRAGS_PER_BLOCK(s)	(EXT2_BLOCK_SIZE(s) / EXT2_FRAG_SIZE(s))

/* Constants relative to the data blocks  */
#define	EXT2_NDIR_BLOCKS		12
#define	EXT2_IND_BLOCK			EXT2_NDIR_BLOCKS
#define	EXT2_DIND_BLOCK			(EXT2_IND_BLOCK + 1)
#define	EXT2_TIND_BLOCK			(EXT2_DIND_BLOCK + 1)
#define	EXT2_N_BLOCKS			(EXT2_TIND_BLOCK + 1)

/* Superblock Flags */
#define EXT2_FEATURE_INCOMPAT_COMPRESSION 0x0001    /* disk/file compression is used */
/*Inode flags  */
#define	EXT2_SECRM_FL			0x00000001 /* Secure deletion */
#define	EXT2_UNRM_FL		    0x00000002 /* Undelete */
#define	EXT2_COMPR_FL			0x00000004 /* Compress file */
#define	EXT2_SYNC_FL			0x00000008 /* Synchronous updates */
#define	EXT2_IMMUTABLE_FL		0x00000010 /* Immutable file */
#define EXT2_APPEND_FL			0x00000020 /* writes to file may only append */
#define EXT2_NODUMP_FL			0x00000040 /* do not dump file */
#define EXT2_NOATIME_FL			0x00000080 /* do not update atime */

/* Reserved for compression usage... */
#define EXT2_DIRTY_FL				0x00000100
#define EXT2_COMPRBLK_FL			0x00000200 /* One or more compressed clusters */
#define EXT2_NOCOMP_FL				0x00000400 /* Don't compress */
#define EXT2_ECOMPR_FL				0x00000800 /* Compression error */

/* End compression flags --- maybe not all used */
#define EXT2_BTREE_FL				0x00001000 /* btree format dir */
#define EXT2_IMAGIC_FL              0x00002000 /* AFS directory */
#define EXT2_JOURNAL_DATA_FL        0x00004000 /* file data should be journaled */
#define EXT2_NOTAIL_FL              0x00008000 /* file tail should not be merged */
#define EXT2_DIRSYNC_FL             0x00010000 /* dirsync behaviour (directories only) */
#define EXT2_TOPDIR_FL              0x00020000 /* Top of directory hierarchies*/
#define EXT2_HUGE_FILE_FL           0x00040000 /* Set to each huge file */
#define EXT2_EXTENTS_FL             0x00080000 /* Inode uses extents */
#define EXT2_RESERVED_FL		    0x80000000 /* reserved for ext2 lib */

#define EXT2_FL_USER_VISIBLE		0x00001FFF /* User visible flags */
#define EXT2_FL_USER_MODIFIABLE		0x000000FF /* User modifiable flags */

/* Codes for operating systems */
#define EXT2_OS_LINUX		0
#define EXT2_OS_HURD		1
#define EXT2_OS_MASIX		2
#define EXT2_OS_FREEBSD		3
#define EXT2_OS_LITES		4

/* Revision levels  */
#define EXT2_GOOD_OLD_REV	0	/* The good old (original) format */
#define EXT2_DYNAMIC_REV	1 	/* V2 format w/ dynamic inode sizes */

#define EXT2_CURRENT_REV	EXT2_GOOD_OLD_REV
#define EXT2_MAX_SUPP_REV	EXT2_DYNAMIC_REV

#define EXT2_GOOD_OLD_INODE_SIZE 128

/* Default values for user and/or group using reserved blocks */
#define	EXT2_DEF_RESUID		0
#define	EXT2_DEF_RESGID		0

/* Structure of a directory entry */
#define EXT2_NAME_LEN           255

#define  EXT2_BLOCK_SIZE(s)			(EXT2_MIN_BLOCK_SIZE << (s)->s_log_block_size)
#define  EXT2_ACLE_PER_BLOCK(s)		(EXT2_BLOCK_SIZE(s) / sizeof (struct ext2_acl_entry))
#define  EXT2_ADDR_PER_BLOCK(s)		(EXT2_BLOCK_SIZE(s) / sizeof (uint16_t))

#define EXT2_INODE_SIZE(s)	(((s)->s_rev_level == EXT2_GOOD_OLD_REV) ? \
				 EXT2_GOOD_OLD_INODE_SIZE : (s)->s_inode_size)

#define EXT2_FIRST_INO(s)	(((s)->s_rev_level == EXT2_GOOD_OLD_REV) ? \
				 EXT2_GOOD_OLD_FIRST_INO :  (s)->s_first_ino)

#define EXT2_BLOCK_SIZE_BITS(s)	((s)->s_log_block_size + 10)

/* Block Group Macros */
#define EXT2_BLOCKS_PER_GROUP(s)	((s)->s_blocks_per_group)
#define EXT2_INODES_PER_GROUP(s)	((s)->s_inodes_per_group)
#define EXT2_CLUSTERS_PER_GROUP(s)	((s)->s_frags_per_group)
#define EXT2_INODES_PER_BLOCK(s)	(EXT2_BLOCK_SIZE(s)/EXT2_INODE_SIZE(s))


/* In Linux disk is divided into Blocks. These Blocks are divided into Groups. This   */
/* Structure shows different types of groups but it is not implemented. Not Necessary */
/*
typedef struct tagBLOCK_GROUP
{
1. The SuperBlock
2. The Group Descriptors
3. The block Bitmap
4. The Inode Bitmap
5. The Inode Table
6. Data Blocks and Fragments

}BLOCK_GROUP;

*/

/* The Super Block comes first in the block group */
typedef struct _EXT2_SUPER_BLOCK
{
    uint32_t	s_inodes_count;   	/* total no of inodes */
    uint32_t	s_blocks_count;   	/* total no of blocks */
    uint32_t	s_r_blocks_count; 	/* total no of blocks reserved for exclusive use  of superuser */
    uint32_t	s_free_blocks_count;	/* total no of free blocks */
    uint32_t	s_free_inodes_count;	/* total no of free inodes */
    uint32_t	s_first_data_block;	/* position of the first data block */
    uint32_t	s_log_block_size;	/* used to compute logical block size in bytes */
    uint32_t	s_log_frag_size;		/* used to compute logical fragment size  */
    uint32_t	s_blocks_per_group;	/* total number of blocks contained in the group  */
    uint32_t	s_frags_per_group;	/* total number of fragments in a group */
    uint32_t	s_inodes_per_group;	/* number of inodes in a group  */
    uint32_t	s_mtime;			/* time at which the last mount was performed */
    uint32_t	s_wtime;			/* time at which the last write was performed */
    uint16_t	s_mnt_count;		/* number of time the fs system has been mounted in r/w mode without having checked */
    uint16_t	s_max_mnt_count;	/* the max no of times the fs can be mounted in r/w mode before a check must be done */
    uint16_t	s_magic;			/* a number that identifies the fs (eg. 0xef53 for ext2) */
    uint16_t	s_state;			/* gives the state of fs (eg. 0x001 is Unmounted cleanly) */
    uint16_t	s_pad;				/* unused */
    uint16_t    s_minor_rev_level;	/*	*/
    uint32_t	s_lastcheck;		/* the time of last check performed */
    uint32_t	s_checkinterval;		/* the max possible time between checks on the fs */
    uint32_t	s_creator_os;		/* os */
    uint32_t	s_rev_level;			/* Revision level */
    uint16_t	s_def_resuid;		/* default uid for reserved blocks */
    uint16_t	s_def_regid;		/* default gid for reserved blocks */

    /* for EXT2_DYNAMIC_REV superblocks only */
    uint32_t	s_first_ino; 		/* First non-reserved inode */
    uint16_t    s_inode_size; 		/* size of inode structure */
    uint16_t	s_block_group_nr; 	/* block group # of this superblock */
    uint32_t	s_feature_compat; 	/* compatible feature set */
    uint32_t	s_feature_incompat; 	/* incompatible feature set */
    uint32_t	s_feature_ro_compat; 	/* readonly-compatible feature set */
    uint8_t	    s_uuid[16];		/* 128-bit uuid for volume */
    char	    s_volume_name[16]; 		/* volume name */
    char	    s_last_mounted[64]; 		/* directory where last mounted */
    uint32_t	s_algorithm_usage_bitmap; /* For compression */
    uint8_t	    s_prealloc_blocks;	/* Nr of blocks to try to preallocate*/
    uint8_t	    s_prealloc_dir_blocks;	/* Nr to preallocate for dirs */
    uint16_t	s_padding1;
    uint32_t	s_reserved[204];		/* unused */
} EXT2_SUPER_BLOCK, *PEXT2_SUPER_BLOCK;

/* The Group Descriptors follow the Super Block. */
typedef struct _EXT2_GROUP_DESC
{
    uint32_t	bg_block_bitmap;	/* points to the blocks bitmap for the group */
    uint32_t	bg_inode_bitmap;	/* points to the inodes bitmap for the group */
    uint32_t	bg_inode_table;		/* points to the inode table first block     */
    uint16_t	bg_free_blocks_count;	/* number of free blocks in the group 	     */
    uint16_t	bg_free_inodes_count;	/* number of free inodes in the		     */
    uint16_t	bg_used_dirs_count;	/* number of inodes allocated to directories */
    uint16_t	bg_pad;			/* padding */
    uint32_t	bg_reserved[3];		/* reserved */
}EXT2_GROUP_DESC, *PEXT2_GROUP_DESC;

/* Structure of an inode on the disk  */
typedef struct _EXT2_INODE
{
    uint16_t	i_mode;		/* File mode */
    uint16_t	i_uid;		/* Low 16 bits of Owner Uid */
    uint32_t	i_size;		/* Size in bytes */
    uint32_t	i_atime;		/* Access time */
    uint32_t	i_ctime;		/* Creation time */
    uint32_t	i_mtime;		/* Modification time */
    uint32_t	i_dtime;		/* Deletion Time */
    uint16_t	i_gid;		/* Low 16 bits of Group Id */
    uint16_t	i_links_count;	/* Links count */
    uint32_t	i_blocks;		/* Blocks count */
    uint32_t	i_flags;		/* File flags */
    union {
        struct {
            uint32_t  l_i_reserved1;
        } linux1;
        struct {
            uint32_t  h_i_translator;
        } hurd1;
        struct {
            uint32_t  m_i_reserved1;
        } masix1;
    } osd1;				/* OS dependent 1 */
    uint32_t	i_block[EXT2_N_BLOCKS];/* Pointers to blocks */
    uint32_t	i_generation;	/* File version (for NFS) */
    uint32_t	i_file_acl;		/* File ACL */
    //    uint32_t	i_dir_acl;		/* Directory ACL */
    uint32_t    i_size_high;            /* This is used store the high 32 bit of file size in large files */
    uint32_t	i_faddr;		/* Fragment address */
    union {
        struct {
            uint8_t	    l_i_frag;	/* Fragment number */
            uint8_t	    l_i_fsize;	/* Fragment size */
            uint16_t	i_pad1;
            uint16_t	l_i_uid_high;	/* these 2 fields    */
            uint16_t	l_i_gid_high;	/* were reserved2[0] */
            uint32_t	l_i_reserved2;
        } linux2;
        struct {
            uint8_t	    h_i_frag;	/* Fragment number */
            uint8_t	    h_i_fsize;	/* Fragment size */
            uint16_t	h_i_mode_high;
            uint16_t	h_i_uid_high;
            uint16_t	h_i_gid_high;
            uint16_t	h_i_author;
        } hurd2;
        struct {
            uint8_t	    m_i_frag;	/* Fragment number */
            uint8_t	    m_i_fsize;	/* Fragment size */
            uint16_t	m_pad1;
            uint32_t	m_i_reserved2[2];
        } masix2;
    } osd2;					/* OS dependent 2 */
} EXT2_INODE, *PEXT2_INODE;

/* EXT2 directory structure */
typedef struct _EXT2_DIR_ENTRY {
    uint32_t	inode;			/* Inode number */
    uint16_t	rec_len;		/* Directory entry length */
    uint8_t 	name_len;		/* Name length */
    uint8_t	    filetype;		/* File type */
    char	    name[EXT2_NAME_LEN];	/* File name */
} EXT2_DIR_ENTRY, *PEXT2_DIR_ENTRY;

typedef struct _EXT3_SUPER_BLOCK{
    /*00*/
    uint32_t	s_inodes_count;		    /* Inodes count */
    uint32_t	s_blocks_count;	        /* Blocks count */
    uint32_t	s_r_blocks_count;	    /* Reserved blocks count */
    uint32_t	s_free_blocks_count;	/* Free blocks count */
    /*10*/
    uint32_t	s_free_inodes_count;	/* Free inodes count */
    uint32_t	s_first_data_block;	    /* First Data Block */
    uint32_t	s_log_block_size;	    /* Block size */
    uint32_t	s_obso_log_frag_size;	/* Obsoleted fragment size */
    /*20*/
    uint32_t	s_blocks_per_group;	    /* # Blocks per group */
    uint32_t	s_obso_frags_per_group;	/* Obsoleted fragments per group */
    uint32_t	s_inodes_per_group;	    /* # Inodes per group */
    uint32_t	s_mtime;		        /* Mount time */
    /*30*/
    uint32_t	s_wtime;		        /* Write time */
    uint16_t	s_mnt_count;		    /* Mount count */
    uint16_t	s_max_mnt_count;	    /* Maximal mount count */
    uint16_t	s_magic;		        /* Magic signature */
    uint16_t	s_state;		        /* File system state */
    uint16_t	s_errors;		        /* Behaviour when detecting errors */
    uint16_t	s_minor_rev_level;	/* minor revision level */
    /*40*/
    uint32_t	s_lastcheck;		/* time of last check */
    uint32_t	s_checkinterval;	/* max. time between checks */
    uint32_t	s_creator_os;		/* OS */
    uint32_t	s_rev_level;		/* Revision level */
    /*50*/
    uint16_t	s_def_resuid;		/* Default uid for reserved blocks */
    uint16_t	s_def_resgid;		/* Default gid for reserved blocks */
    /*
    * These fields are for EXT4_DYNAMIC_REV superblocks only.
    *
    * Note: the difference between the compatible feature set and
    * the incompatible feature set is that if there is a bit set
    * in the incompatible feature set that the kernel doesn't
    * know about, it should refuse to mount the filesystem.
    *
    * e2fsck's requirements are more strict; if it doesn't know
    * about a feature in either the compatible or incompatible
    * feature set, it must abort and not try to meddle with
    * things it doesn't understand...
    */
    uint32_t	s_first_ino;		/* First non-reserved inode */
    uint16_t    s_inode_size;		/* size of inode structure */
    uint16_t	s_block_group_nr;	/* block group # of this superblock */
    uint32_t	s_feature_compat;	/* compatible feature set */
    /*60*/
    uint32_t	s_feature_incompat;	/* incompatible feature set */
    uint32_t	s_feature_ro_compat;	/* readonly-compatible feature set */
    /*68*/
    uint8_t	    s_uuid[16];		/* 128-bit uuid for volume */
    /*78*/
    char	    s_volume_name[16];	/* volume name */
    /*88*/
    char	    s_last_mounted[64];	/* directory where last mounted */
    /*C8*/
    uint32_t	s_algorithm_usage_bitmap; /* For compression */
    /*
    * Performance hints.  Directory preallocation should only
    * happen if the EXT4_FEATURE_COMPAT_DIR_PREALLOC flag is on.
    */
    uint8_t	    s_prealloc_blocks;	/* Nr of blocks to try to preallocate*/
    uint8_t	    s_prealloc_dir_blocks;	/* Nr to preallocate for dirs */
    uint16_t	s_reserved_gdt_blocks;	/* Per group desc for online growth */
    /*
    * Journaling support valid if EXT4_FEATURE_COMPAT_HAS_JOURNAL set.
    */
    /*D0*/
    uint8_t	    s_journal_uuid[16];	/* uuid of journal superblock */
    /*E0*/
    uint32_t	s_journal_inum;		/* inode number of journal file */
    uint32_t	s_journal_dev;		/* device number of journal file */
    uint32_t	s_last_orphan;		/* start of list of inodes to delete */
    uint32_t	s_hash_seed[4];		/* HTREE hash seed */
    uint8_t	    s_def_hash_version;	/* Default hash version to use */
    uint8_t	    s_reserved_char_pad;
    uint16_t    s_desc_size;		/* size of group descriptor */
    /*100*/
    uint32_t	s_default_mount_opts;
    uint32_t	s_first_meta_bg;	/* First metablock block group */
    uint32_t	s_mkfs_time;		/* When the filesystem was created */
    uint32_t	s_jnl_blocks[17];	/* Backup of the journal inode */
    /* 64bit support valid if EXT4_FEATURE_COMPAT_64BIT */
    /*150*/
    uint32_t	s_blocks_count_hi;	/* Blocks count */
    uint32_t	s_r_blocks_count_hi;	/* Reserved blocks count */
    uint32_t	s_free_blocks_count_hi;	/* Free blocks count */
    uint16_t	s_min_extra_isize;	/* All inodes have at least # bytes */
    uint16_t	s_want_extra_isize; 	/* New inodes should reserve # bytes */
    uint32_t	s_flags;		/* Miscellaneous flags */
    uint16_t    s_raid_stride;		/* RAID stride */
    uint16_t    s_mmp_interval;         /* # seconds to wait in MMP checking */
    uint64_t    s_mmp_block;            /* Block for multi-mount protection */
    uint32_t    s_raid_stripe_width;    /* blocks on all data disks (N*stride)*/
    uint8_t	    s_log_groups_per_flex;  /* FLEX_BG group size */
    uint8_t     s_reserved_char_pad2;
    uint16_t    s_reserved_pad;
    uint32_t    s_reserved[162];        /* Padding to the end of the block */
}EXT3_SUPER_BLOCK, *PEXT3_SUPER_BLOCK;

/*
* Structure of a blocks group descriptor
*/
typedef struct _EXT4_GROUP_DESC
{
    uint32_t	bg_block_bitmap;	    /* Blocks bitmap block */
    uint32_t	bg_inode_bitmap;	    /* Inodes bitmap block */
    uint32_t	bg_inode_table;	        /* Inodes table block */
    uint16_t	bg_free_blocks_count;   /* Free blocks count */
    uint16_t	bg_free_inodes_count;   /* Free inodes count */
    uint16_t	bg_used_dirs_count;	    /* Directories count */
    uint16_t	bg_flags;		        /* EXT4_BG_flags (INODE_UNINIT, etc) */
    uint32_t	bg_reserved[2];		    /* Likely block/inode bitmap checksum */
    uint16_t    bg_itable_unused;	    /* Unused inodes count */
    uint16_t    bg_checksum;		    /* crc16(sb_uuid+group+desc) */
    uint32_t	bg_block_bitmap_hi;	    /* Blocks bitmap block MSB */
    uint32_t	bg_inode_bitmap_hi;	    /* Inodes bitmap block MSB */
    uint32_t	bg_inode_table_hi;	    /* Inodes table block MSB */
    uint16_t	bg_free_blocks_count_hi;/* Free blocks count MSB */
    uint16_t	bg_free_inodes_count_hi;/* Free inodes count MSB */
    uint16_t	bg_used_dirs_count_hi;	/* Directories count MSB */
    uint16_t    bg_itable_unused_hi;    /* Unused inodes count MSB */
    uint32_t	bg_reserved2[3];
}EXT4_GROUP_DESC, *PEXT4_GROUP_DESC;

/*
* This is the extent on-disk structure.
* It's used at the bottom of the tree.
*/
typedef struct _EXT4_EXTENT {
    uint32_t ee_block; /* first logical block extent covers */
    uint16_t ee_len; /* number of blocks covered by extent */
    uint16_t ee_start_hi; /* high 16 bits of physical block */
    uint32_t ee_start_lo; /* low 32 bits of physical block */
} EXT4_EXTENT, *PEXT4_EXTENT;

/*
* This is index on-disk structure.
* It's used at all the levels except the bottom.
*/
typedef struct _EXT4_EXTENT_IDX {
    uint32_t  ei_block;       /* index covers logical blocks from 'block' */
    uint32_t  ei_leaf_lo;     /* pointer to the physical block of the next *
                              * level. leaf or next index could be there */
    uint16_t  ei_leaf_hi;     /* high 16 bits of physical block */
    uint16_t   ei_unused;
}EXT4_EXTENT_IDX, *PEXT4_EXTENT_IDX;

/*
* Each block (leaves and indexes), even inode-stored has header.
*/
typedef struct _EXT4_EXTENT_HEADER {
    uint16_t  eh_magic;       /* probably will support different formats */
    uint16_t  eh_entries;     /* number of valid entries */
    uint16_t  eh_max;         /* capacity of store in entries */
    uint16_t  eh_depth;       /* has tree real underlying blocks? */
    uint32_t  eh_generation;  /* generation of the tree */
} EXT4_EXTENT_HEADER, *PEXT4_EXTENT_HEADER;

#define EXT2_HAS_COMPAT_FEATURE(sb,mask)			\
	( EXT2_SB(sb)->s_feature_compat & (mask) )
#define EXT2_HAS_RO_COMPAT_FEATURE(sb,mask)			\
	( EXT2_SB(sb)->s_feature_ro_compat & (mask) )
#define EXT2_HAS_INCOMPAT_FEATURE(sb,mask)			\
	( EXT2_SB(sb)->s_feature_incompat & (mask) )

#define EXT2_FEATURE_COMPAT_DIR_PREALLOC	0x0001
#define EXT2_FEATURE_COMPAT_IMAGIC_INODES	0x0002
#define EXT3_FEATURE_COMPAT_HAS_JOURNAL		0x0004
#define EXT2_FEATURE_COMPAT_EXT_ATTR		0x0008
#define EXT2_FEATURE_COMPAT_RESIZE_INODE	0x0010
#define EXT2_FEATURE_COMPAT_DIR_INDEX		0x0020
#define EXT2_FEATURE_COMPAT_LAZY_BG		0x0040
/* #define EXT2_FEATURE_COMPAT_EXCLUDE_INODE	0x0080 not used, legacy */
#define EXT2_FEATURE_COMPAT_EXCLUDE_BITMAP	0x0100
#define EXT4_FEATURE_COMPAT_SPARSE_SUPER2	0x0200


#define EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER	0x0001
#define EXT2_FEATURE_RO_COMPAT_LARGE_FILE	0x0002
/* #define EXT2_FEATURE_RO_COMPAT_BTREE_DIR	0x0004 not used */
#define EXT4_FEATURE_RO_COMPAT_HUGE_FILE	0x0008
#define EXT4_FEATURE_RO_COMPAT_GDT_CSUM		0x0010
#define EXT4_FEATURE_RO_COMPAT_DIR_NLINK	0x0020
#define EXT4_FEATURE_RO_COMPAT_EXTRA_ISIZE	0x0040
#define EXT4_FEATURE_RO_COMPAT_HAS_SNAPSHOT	0x0080
#define EXT4_FEATURE_RO_COMPAT_QUOTA		0x0100
#define EXT4_FEATURE_RO_COMPAT_BIGALLOC		0x0200
/*
* METADATA_CSUM implies GDT_CSUM.  When METADATA_CSUM is set, group
* descriptor checksums use the same algorithm as all other data
* structures' checksums.
*/
#define EXT4_FEATURE_RO_COMPAT_METADATA_CSUM	0x0400
#define EXT4_FEATURE_RO_COMPAT_REPLICA		0x0800
#define EXT4_FEATURE_RO_COMPAT_READONLY		0x1000
#define EXT4_FEATURE_RO_COMPAT_PROJECT		0x2000 /* Project quota */


#define EXT2_FEATURE_INCOMPAT_COMPRESSION	0x0001
#define EXT2_FEATURE_INCOMPAT_FILETYPE		0x0002
#define EXT3_FEATURE_INCOMPAT_RECOVER		0x0004 /* Needs recovery */
#define EXT3_FEATURE_INCOMPAT_JOURNAL_DEV	0x0008 /* Journal device */
#define EXT2_FEATURE_INCOMPAT_META_BG		0x0010
#define EXT3_FEATURE_INCOMPAT_EXTENTS		0x0040
#define EXT4_FEATURE_INCOMPAT_64BIT		0x0080
#define EXT4_FEATURE_INCOMPAT_MMP		0x0100
#define EXT4_FEATURE_INCOMPAT_FLEX_BG		0x0200
#define EXT4_FEATURE_INCOMPAT_EA_INODE		0x0400
#define EXT4_FEATURE_INCOMPAT_DIRDATA		0x1000
#define EXT4_FEATURE_INCOMPAT_CSUM_SEED		0x2000
#define EXT4_FEATURE_INCOMPAT_LARGEDIR		0x4000 /* >2GB or 3-lvl htree */
#define EXT4_FEATURE_INCOMPAT_INLINE_DATA	0x8000 /* data in inode */
#define EXT4_FEATURE_INCOMPAT_ENCRYPT		0x10000

#define EXT2_SB(sb)	(sb)

#define EXT2_MIN_DESC_SIZE             32
#define EXT2_MIN_DESC_SIZE_64BIT       64
#define EXT2_MAX_DESC_SIZE             EXT2_MIN_BLOCK_SIZE
#define EXT2_DESC_SIZE(s)                                                \
       ((EXT2_SB(s)->s_feature_incompat & EXT4_FEATURE_INCOMPAT_64BIT) ? \
	(s)->s_desc_size : EXT2_MIN_DESC_SIZE)
#define EXT2_DESC_PER_BLOCK(s)		(EXT2_BLOCK_SIZE(s) / EXT2_DESC_SIZE(s))

#define EXT4_FEATURE_COMPAT_FUNCS(name, ver, flagname) \
static inline int ext2fs_has_feature_##name(EXT3_SUPER_BLOCK *sb) \
{ \
	return ((EXT2_SB(sb)->s_feature_compat & \
		 EXT##ver##_FEATURE_COMPAT_##flagname) != 0); \
}

#define EXT4_FEATURE_RO_COMPAT_FUNCS(name, ver, flagname) \
static inline int ext2fs_has_feature_##name(EXT3_SUPER_BLOCK *sb) \
{ \
	return ((EXT2_SB(sb)->s_feature_ro_compat & \
		 EXT##ver##_FEATURE_RO_COMPAT_##flagname) != 0); \
}

#define EXT4_FEATURE_INCOMPAT_FUNCS(name, ver, flagname) \
static inline int ext2fs_has_feature_##name(EXT3_SUPER_BLOCK *sb) \
{ \
	return ((EXT2_SB(sb)->s_feature_incompat & \
		 EXT##ver##_FEATURE_INCOMPAT_##flagname) != 0); \
}

EXT4_FEATURE_COMPAT_FUNCS(dir_prealloc, 2, DIR_PREALLOC)
EXT4_FEATURE_COMPAT_FUNCS(imagic_inodes, 2, IMAGIC_INODES)
EXT4_FEATURE_COMPAT_FUNCS(journal, 3, HAS_JOURNAL)
EXT4_FEATURE_COMPAT_FUNCS(xattr, 2, EXT_ATTR)
EXT4_FEATURE_COMPAT_FUNCS(resize_inode, 2, RESIZE_INODE)
EXT4_FEATURE_COMPAT_FUNCS(dir_index, 2, DIR_INDEX)
EXT4_FEATURE_COMPAT_FUNCS(lazy_bg, 2, LAZY_BG)
EXT4_FEATURE_COMPAT_FUNCS(exclude_bitmap, 2, EXCLUDE_BITMAP)
EXT4_FEATURE_COMPAT_FUNCS(sparse_super2, 4, SPARSE_SUPER2)

EXT4_FEATURE_RO_COMPAT_FUNCS(sparse_super, 2, SPARSE_SUPER)
EXT4_FEATURE_RO_COMPAT_FUNCS(large_file, 2, LARGE_FILE)
EXT4_FEATURE_RO_COMPAT_FUNCS(huge_file, 4, HUGE_FILE)
EXT4_FEATURE_RO_COMPAT_FUNCS(gdt_csum, 4, GDT_CSUM)
EXT4_FEATURE_RO_COMPAT_FUNCS(dir_nlink, 4, DIR_NLINK)
EXT4_FEATURE_RO_COMPAT_FUNCS(extra_isize, 4, EXTRA_ISIZE)
EXT4_FEATURE_RO_COMPAT_FUNCS(has_snapshot, 4, HAS_SNAPSHOT)
EXT4_FEATURE_RO_COMPAT_FUNCS(quota, 4, QUOTA)
EXT4_FEATURE_RO_COMPAT_FUNCS(bigalloc, 4, BIGALLOC)
EXT4_FEATURE_RO_COMPAT_FUNCS(metadata_csum, 4, METADATA_CSUM)
EXT4_FEATURE_RO_COMPAT_FUNCS(replica, 4, REPLICA)
EXT4_FEATURE_RO_COMPAT_FUNCS(readonly, 4, READONLY)
EXT4_FEATURE_RO_COMPAT_FUNCS(project, 4, PROJECT)

EXT4_FEATURE_INCOMPAT_FUNCS(compression, 2, COMPRESSION)
EXT4_FEATURE_INCOMPAT_FUNCS(filetype, 2, FILETYPE)
EXT4_FEATURE_INCOMPAT_FUNCS(journal_needs_recovery, 3, RECOVER)
EXT4_FEATURE_INCOMPAT_FUNCS(journal_dev, 3, JOURNAL_DEV)
EXT4_FEATURE_INCOMPAT_FUNCS(meta_bg, 2, META_BG)
EXT4_FEATURE_INCOMPAT_FUNCS(extents, 3, EXTENTS)
EXT4_FEATURE_INCOMPAT_FUNCS(64bit, 4, 64BIT)
EXT4_FEATURE_INCOMPAT_FUNCS(mmp, 4, MMP)
EXT4_FEATURE_INCOMPAT_FUNCS(flex_bg, 4, FLEX_BG)
EXT4_FEATURE_INCOMPAT_FUNCS(ea_inode, 4, EA_INODE)
EXT4_FEATURE_INCOMPAT_FUNCS(dirdata, 4, DIRDATA)
EXT4_FEATURE_INCOMPAT_FUNCS(csum_seed, 4, CSUM_SEED)
EXT4_FEATURE_INCOMPAT_FUNCS(largedir, 4, LARGEDIR)
EXT4_FEATURE_INCOMPAT_FUNCS(inline_data, 4, INLINE_DATA)
EXT4_FEATURE_INCOMPAT_FUNCS(encrypt, 4, ENCRYPT)

#define EXT2_FEATURE_COMPAT_SUPP	0
#define EXT2_FEATURE_INCOMPAT_SUPP    (EXT2_FEATURE_INCOMPAT_FILETYPE| \
				       EXT4_FEATURE_INCOMPAT_MMP)
#define EXT2_FEATURE_RO_COMPAT_SUPP	(EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER| \
					 EXT2_FEATURE_RO_COMPAT_LARGE_FILE| \
					 EXT4_FEATURE_RO_COMPAT_DIR_NLINK| \
					 EXT2_FEATURE_RO_COMPAT_BTREE_DIR)

#define EXT4_EXT_MAGIC          0xf30a
#define get_ext4_header(i)      ((struct _EXT4_EXTENT_HEADER *) (i)->i_block)

#define EXT_FIRST_EXTENT(__hdr__) \
((struct _EXT4_EXTENT *) (((char *) (__hdr__)) +         \
                         sizeof(struct _EXT4_EXTENT_HEADER)))

#define EXT_FIRST_INDEX(__hdr__) \
        ((struct _EXT4_EXTENT_IDX *) (((char *) (__hdr__)) +     \
                                     sizeof(struct _EXT4_EXTENT_HEADER)))

#define INODE_HAS_EXTENT(i) ((i)->i_flags & EXT2_EXTENTS_FL)

static inline uint64_t ext_to_block(EXT4_EXTENT *extent)
{
    uint64_t block;

    block = (uint64_t)extent->ee_start_lo;
    block |= ((uint64_t)extent->ee_start_hi << 31) << 1;

    return block;
}

static inline uint64_t idx_to_block(EXT4_EXTENT_IDX *idx)
{
    uint64_t block;

    block = (uint64_t)idx->ei_leaf_lo;
    block |= ((uint64_t)idx->ei_leaf_hi << 31) << 1;

    return block;
}

#ifdef __cplusplus
}
#endif

/* Identifies the type of file by using i_mode of inode */
/* structure as input.									*/
#define EXT2_S_ISDIR(mode)	((mode & 0x0f000) == 0x04000)
#define EXT2_S_ISLINK(mode)	((mode & 0x0f000) == 0x0a000)
#define EXT2_S_ISBLK(mode)	((mode & 0x0f000) == 0x06000)
#define EXT2_S_ISSOCK(mode)	((mode & 0x0f000) == 0x0c000)
#define EXT2_S_ISREG(mode)	((mode & 0x0f000) == 0x08000)
#define EXT2_S_ISCHAR(mode)	((mode & 0x0f000) == 0x02000)
#define EXT2_S_ISFIFO(mode)	((mode & 0x0f000) == 0x01000)

#define IS_BUFFER_END(p, q, bsize)	(((char *)(p)) >= ((char *)(q) + bsize))

inline int set_umap_bit(unsigned char* buffer, ULONGLONG bit){
    return buffer[bit >> 3] |= (1 << (bit & 7));
}

inline int	test_umap_bit(unsigned char* buffer, ULONGLONG bit){
    return buffer[bit >> 3] & (1 << (bit & 7));
}

inline int clear_umap_bit(unsigned char* buffer, ULONGLONG bit){
    return buffer[bit >> 3] &= ~(1 << (bit & 7));
}

class ext2_volume;
struct ext2_file{
    typedef boost::shared_ptr<ext2_file> ptr;
    typedef std::vector<ptr> vtr;

    ext2_file() : inode_num(0), file_type(0), file_size(0){ memset(&inode, 0, sizeof(EXT2_INODE)); }
    uint32_t     inode_num;
    uint8_t      file_type;
    std::string  file_name;
    uint64_t     file_size;
    EXT2_INODE   inode;
    ext2_volume* volume;
    std::vector<uint64_t> get_data_blocks();
    bool is_dir() { return EXT2_S_ISDIR(inode.i_mode); }
    bool is_link() { return EXT2_S_ISLINK(inode.i_mode); }
    bool is_blk() { return EXT2_S_ISBLK(inode.i_mode); }
    bool is_ssock() { return EXT2_S_ISSOCK(inode.i_mode); }
    bool is_reg() { return EXT2_S_ISREG(inode.i_mode); }
    bool is_char() { return EXT2_S_ISCHAR(inode.i_mode); }
    bool is_fifo() { return EXT2_S_ISFIFO(inode.i_mode); }
};

#include "linuxfs_parser.h"

class ext2_volume : public linuxfs::volume{
public:
    static linuxfs::volume::ptr get(universal_disk_rw::ptr& rw, ULONGLONG _offset, ULONGLONG _size);
    virtual linuxfs::io_range::vtr      file_system_ranges();
private:
    friend struct ext2_file;
    ext2_file::ptr             read_inode(uint32_t inum);
    ext2_file::vtr             enumerate_dir(ext2_file::ptr dir);
    void                       get_filesystem_blocks(ext2_file::ptr dir, std::vector<uint64_t>& blocks);
    int read_data_block(EXT2_INODE *ino, uint64_t lbn, void *buf);
    struct ext2_dirent {
        typedef boost::shared_ptr<ext2_dirent> ptr;
        EXT2_DIR_ENTRY*                   next;
        boost::shared_ptr<EXT2_DIR_ENTRY> dirbuf;
        ext2_file::ptr parent;
        uint64_t       read_bytes;     // Bytes already read
        uint64_t       next_block;
        ext2_dirent() :read_bytes(0), next_block(0), next(NULL){}
    };

    inline ext2_dirent::ptr open_dir(ext2_file::ptr dir){
        if (dir->is_dir()){
            ext2_dirent::ptr dirent = ext2_dirent::ptr(new ext2_dirent());
            dirent->parent = dir;
            return dirent;
        }
        return NULL;
    }
    uint64_t extent_binarysearch(EXT4_EXTENT_HEADER *header, uint64_t lbn, bool isallocated);
    uint32_t fileblock_to_logical(EXT2_INODE *ino, uint32_t lbn);
    inline uint64_t extent_to_logical(EXT2_INODE *ino, uint64_t lbn)
    {
        uint64_t block;
        EXT4_EXTENT_HEADER *header;
        header = get_ext4_header(ino);
        block = extent_binarysearch(header, lbn, false);
        return block;
    }

    ext2_file::ptr read_dir(ext2_dirent::ptr& dirent);
    bool ext2_read_block(uint64_t blocknum, void *buffer);
    inline bool ext2_read_inode_table_block(uint64_t start_block, void *buffer, uint64_t size){
        uint32_t number_of_bytes_read = 0;
        memset(buffer, 0, size);
        return rw->read(offset + start_block *block_size, size, buffer, number_of_bytes_read);
    }

    ext2_volume(universal_disk_rw::ptr &_rw, ULONGLONG _offset, ULONGLONG _size) :
        volume(_rw, _offset, _size),
        cluster_ratio_bits(0),
        block_size(0),
        relative_sectors(0LL),
        inodes_per_group(0),
        inode_size(0),
        total_groups(0),
        total_blocks(0),
        blocks_per_group(0),
        log_block_size(0),
        log_cluster_size(0),
        first_data_block(0),
        clusters_per_group(0),
        last_read_block(0),
        is_64_bit(false),
        is_big_alloc(false),
        desc_size(sizeof(EXT2_GROUP_DESC))
    {
    }

    inline PEXT4_GROUP_DESC                           get_group_desc(uint32_t index){
        return (PEXT4_GROUP_DESC)(groups_desc.get() + (desc_size * index));
    }

    inline uint32_t                                   get_free_blocks_count(PEXT4_GROUP_DESC gb){
        return gb->bg_free_blocks_count | (is_64_bit ? (uint32_t)gb->bg_free_blocks_count_hi << 16 : 0);
    }

    inline uint32_t                                   get_free_inodes_count(PEXT4_GROUP_DESC gb){
        return gb->bg_free_inodes_count | (is_64_bit ? (uint32_t)gb->bg_free_inodes_count_hi << 16 : 0);
    }

    inline uint64_t                                   get_block_bitmap(PEXT4_GROUP_DESC gb){
        return gb->bg_block_bitmap | (is_64_bit ? (uint64_t)gb->bg_block_bitmap_hi << 32 : 0);
    }

    inline uint64_t                                   get_inode_bitmap(PEXT4_GROUP_DESC gb){
        return gb->bg_inode_bitmap | (is_64_bit ? (uint64_t)gb->bg_inode_bitmap_hi << 32 : 0);
    }

    inline uint64_t                                   get_inode_table_block(PEXT4_GROUP_DESC gb){
        return gb->bg_inode_table | (is_64_bit ? (uint64_t)gb->bg_inode_table_hi << 32 : 0);
    }
    int				                           cluster_ratio_bits;
    bool                                       is_64_bit;
    uint64_t             	                   relative_sectors;
    std::string                                linux_name;
    std::string                                volume_name;
    uint32_t                                   inodes_per_group;
    int                                        inode_size;
    int                                        block_size;
    uint32_t                                   blocks_per_group;
    uint32_t                                   clusters_per_group;
    uint32_t                                   log_block_size;      /* Block size */
    uint32_t                                   log_cluster_size;	/* Allocation cluster size */
    uint64_t                                   total_groups;
    uint64_t                                   total_blocks;
    uint32_t                                   first_data_block;
    macho::guid_                               volume_uuid;
    uint16_t                                   desc_size;		        /* size of group descriptor */
    boost::shared_ptr<BYTE>                    groups_desc;
    ext2_file::ptr                             root;
    boost::shared_ptr<char>                    inode_buffer;         // buffer to cache last used block of inodes
    uint64_t                                   last_read_block;      // block number of the last inode block read
    bool                                       is_big_alloc;

    static inline uint64_t ext2fs_div64_ceil(uint64_t a, uint64_t b)
    {
        if (!a)
            return 0;
        return ((a - 1) / b) + 1;
    }

    static inline unsigned int ext2fs_div_ceil(unsigned int a, unsigned int b)
    {
        if (!a)
            return 0;
        return ((a - 1) / b) + 1;
    }

    /*
    * Return the fs block count
    */
    static uint64_t ext2fs_blocks_count(EXT3_SUPER_BLOCK *super)
    {
        return super->s_blocks_count |
            (ext2fs_has_feature_64bit(super) ?
            (uint64_t)super->s_blocks_count_hi << 32 : 0);
    }

};

#endif