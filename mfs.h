/*
adapted from dos.h in the mach dos emulator for the linux dosemu dos
emulator.
Andrew.Tridgell@anu.edu.au 30th March 1993
*/


/* ### Added by DH ### */
#ifndef DOSEMU
#    define DOSEMU
#endif

#include <unistd.h>             /* for SEEK_END */

/* ### End of addition ### */

#ifdef DOSEMU
/* definitions to make mach emu code compatible with dosemu */

/* ### Commented out by DH ###
*  #include "emu.h"
*/


#ifndef SOLARIS
typedef unsigned char boolean_t;
#endif

/* ### Commented out by DH ### 
*  #define d_namlen d_reclen
*/

#ifndef MAX_DRIVE
#define MAX_DRIVE 26
#endif

#define USE_DF_AND_AFS_STUFF

/* ### Commented out by DH ###
*  #define VOLUMELABEL "Linux"
*  #define LINUX_RESOURCE "\\\\LINUX\\FS"
*/

/* ### Added by DH ### */

#include "mfs_link.h"

#if !defined(__hpux) && !defined(SOLARIS) && !defined(SGI) && !defined(RS6000) && 0
#define strerror(x) sys_errlist[x]
#endif

#ifdef RS6000
#define strerror(x) mstrerror(x)
#endif

#define VOLUMELABEL "PCEmu"
#define LINUX_RESOURCE "\\\\PCEMU\\FS"

#define Write2Bytes(d,x) (*(BYTE *)(d) = (x) & 0xff, \
                          *((BYTE *)(d)+1) = ((x) >> 8) & 0xff)
#define Write4Bytes(d,x) (*(BYTE *)(d) = (x) & 0xff, \
                          *((BYTE *)(d)+1) = ((x) >> 8) & 0xff, \
                          *((BYTE *)(d)+2) = ((x) >> 16) & 0xff, \
                          *((BYTE *)(d)+3) = ((x) >> 24) & 0xff)

#define Read2Bytes(t,x) (t)(*(BYTE *)(x) + (*((BYTE *)(x) + 1) << 8))
#define Read4Bytes(t,x) (t)(*(BYTE *)(x) + ((UINT32)*((BYTE *)(x) + 1) << 8) + \
                            ((UINT32)*((BYTE *)(x) + 2) << 16) + \
                            ((UINT32)*((BYTE *)(x) + 3) << 24))

/* ### End of addition ### */

/* ### Added by DH ### */
#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif
/* ### End of addition ### */

/* ### Commented out by DH
*  #define FALSE 0
*  #define TRUE 1
*/

#define UNCHANGED 2
#define REDIRECT 3

#define us_debug_level 10
#define Debug_Level_0 0
#define dbg_fd stderr

/* Some compilers cannot handle variable argument #defines
#define d_Stub(arg1, s, a...)   d_printf("MFS: "s, ##a)
#define Debug0(args)		d_Stub args
#define Debug1(args)		d_Stub args
*/

#define Debug0(args)
#define Debug1(args)

typedef struct vm86_regs state_t;

#define uesp esp

/* ### Added '(&memory[ ... ])' to line below ### */
#define Addr_8086(x,y)	(&memory[(( ((x) & 0xffff) << 4) + ((y) & 0xffff))])
#define Addr(s,x,y)	Addr_8086(((s)->x), ((s)->y))
#define MASK8(x)	((x) & 0xff)
#define MASK16(x)	((x) & 0xffff)
#define HIGH(x)		MASK8((UINT32)(x) >> 8)
#define LOW(x)		MASK8((UINT32)(x))
#undef WORD
#define WORD(x)		MASK16((UINT32)(x))
#define SETHIGH(x,y) 	(*(x) = (*(x) & ~0xff00) | ((MASK8(y))<<8))
#define SETLOW(x,y) 	(*(x) = (*(x) & ~0xff) | (MASK8(y)))
#define SETWORD(x,y)	(*(x) = (*(x) & ~0xffff) | (MASK16(y)))
#endif

/*
 * Copyright (c) 1991 Carnegie Mellon University
 * All Rights Reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
/*
 *
 * Purpose:
 *	V86 DOS disk emulation header file
 *
 * HISTORY:
 * $Log$
 * Revision 1.1  2001/01/22 01:35:32  michaelh
 * Initial revision
 *
 * Revision 1.1.1.1  2000/12/12 06:08:44  michaelh
 * Created.
 *
 * Revision 1.6  1994/01/25  20:02:44  root
 * Exchange stderr <-> stdout.
 *
 * Revision 1.5  1994/01/20  21:14:24  root
 * Indent.
 *
 * Revision 1.4  1994/01/19  17:51:14  root
 * Added CDS_FLAG_NOTNET = 0x80 for mfs.c
 *
 * Revision 1.3  1993/12/22  11:45:36  root
 * Fixes for ftruncate
 *
 * Revision 1.2  1993/11/17  22:29:33  root
 * *** empty log message ***
 *
 * Revision 1.1  1993/11/12  12:32:17  root
 * Initial revision
 *
 * Revision 1.1  1993/07/07  00:49:06  root
 * Initial revision
 *
 * Revision 1.3  1993/05/04  05:29:22  root
 * added console switching, new parse commands, and serial emulation
 *
 * Revision 1.2  1993/04/07  21:04:26  root
 * big move
 *
 * Revision 1.1  1993/04/05  17:25:13  root
 * Initial revision
 *
 * Revision 2.3  91/12/06  15:29:23  grm
 * 	Redefined sda_cur_psp, and added psp_parent_psp.  The
 * 	psp_parent_psp was used to find out what the psp of the parent of
 * 	the command.com program was.  It seems that it is undefined.
 * 	[91/12/06            grm]
 *
 * Revision 2.2  91/12/05  16:42:08  grm
 * 	Added sft_rel and _abs_cluster macros.  Used to debug the
 * 	MS-Write network drive problem.
 * 	[91/12/04            grm]
 * 	Added constants for Dos 4+
 * 	[91/07/16  17:47:20  grm]
 *
 * 	Changed to allow for usage with Dos v4.01 and 5.00.
 * 	[91/06/28  18:53:42  grm]
 *
 * 	New Copyright
 * 	[91/05/28  15:12:28  grm]
 *
 * 	Added structures for the dos_general routines.
 * 	[91/04/30  13:43:58  grm]
 *
 * 	Structures and macros for the dos_fs.c
 * 	network redirector interface.
 * 	[91/04/30  13:36:42  grm]
 *
 * 	Name changed from dos_general.h to dos.h.
 * 	Added external declarations for dos_foo.c files.
 * 	[91/03/01  14:40:55  grm]
 *
 * 	Type works.  Interrim changes.
 * 	[91/02/11  18:22:47  grm]
 *
 * 	Fancy dir.
 * 	[91/02/06  16:59:01  grm]
 *
 * 	Created.
 * 	[91/02/06  14:29:39  grm]
 *
 */

#include <sys/types.h>

/*
 * Dos error codes
 */
/* MS-DOS version 2 error codes */
#define FUNC_NUM_IVALID		0x01
#define FILE_NOT_FOUND		0x02
#define PATH_NOT_FOUND		0x03
#define TOO_MANY_OPEN_FILES	0x04
#define ACCESS_DENIED		0x05
#define HANDLE_INVALID		0x06
#define MEM_CB_DEST		0x07
#define INSUF_MEM		0x08
#define MEM_BLK_ADDR_IVALID	0x09
#define ENV_INVALID		0x0a
#define FORMAT_INVALID		0x0b
#define ACCESS_CODE_INVALID	0x0c
#define DATA_INVALID		0x0d
#define UNKNOWN_UNIT		0x0e
#define DISK_DRIVE_INVALID	0x0f
#define ATT_REM_CUR_DIR		0x10
#define NOT_SAME_DEV		0x11
#define NO_MORE_FILES		0x12
/* mappings to critical-error codes */
#define WRITE_PROT_DISK		0x13
#define UNKNOWN_UNIT_CERR	0x14
#define DRIVE_NOT_READY		0x15
#define UNKNOWN_COMMAND		0x16
#define DATA_ERROR_CRC		0x17
#define BAD_REQ_STRUCT_LEN	0x18
#define SEEK_ERROR		0x19
#define UNKNOWN_MEDIA_TYPE	0x1a
#define SECTOR_NOT_FOUND	0x1b
#define PRINTER_OUT_OF_PAPER	0x1c
#define WRITE_FAULT		0x1d
#define READ_FAULT		0x1e
#define GENERAL_FAILURE		0x1f

/* MS-DOS version 3 and later extended error codes */
#define SHARING_VIOLATION	0x20
#define FILE_LOCK_VIOLATION	0x21
#define DISK_CHANGE_INVALID	0x22
#define FCB_UNAVAILABLE		0x23
#define SHARING_BUF_EXCEEDED	0x24

#define NETWORK_NAME_NOT_FOUND	0x35

#define FILE_ALREADY_EXISTS	0x50

#define DUPLICATE_REDIR		0x55

struct dir_ent {
  char name[8];			/* dos name and ext */
  char ext[3];
  u_short mode;			/* unix st_mode value */
  long size;			/* size of file */
  time_t time;			/* st_mtime */
  struct dir_ent *next;
};

struct dos_name {
  char name[8];
  char ext[3];
};

typedef struct far_record {
  u_short offset;
  u_short segment;
} far_t;

#define DOSVER_31_33	1
#define DOSVER_41	2
#define DOSVER_50	3
#define DOSVER_60	4

typedef u_char *sdb_t;

/* ### Changed all *(u_short *)... to Read2Bytes(u_short,...) ### */
/* ### Changed all *(u_long *)... to Read4Bytes(u_long,...) ### */
#define sdb_drive_letter(sdb)	(*(u_char  *)&sdb[sdb_drive_letter_off])
#define sdb_template_name(sdb)	((char   *)&sdb[sdb_template_name_off])
#define sdb_template_ext(sdb)	((char   *)&sdb[sdb_template_ext_off])
#define	sdb_attribute(sdb)	(*(u_char  *)&sdb[sdb_attribute_off])
/* ### Split next definition into read (..._r) and write (..._w) ### */
#define sdb_dir_entry_r(sdb)	(Read2Bytes(u_short,&sdb[sdb_dir_entry_off]))
#define sdb_dir_entry_w(sdb,x)	(Write2Bytes(&sdb[sdb_dir_entry_off], x))
#define sdb_p_cluster(sdb)	(Read2Bytes(u_short,&sdb[sdb_p_cluster_off]))
#define	sdb_file_name(sdb)	((char     *)&sdb[sdb_file_name_off])
#define	sdb_file_ext(sdb)	((char     *)&sdb[sdb_file_ext_off])
#define	sdb_file_attr(sdb)	(*(u_char  *)&sdb[sdb_file_attr_off])
#define	sdb_file_time(sdb)	(*(u_short *)&sdb[sdb_file_time_off])
#define	sdb_file_date(sdb)	(*(u_short *)&sdb[sdb_file_date_off])
#define sdb_file_st_cluster(sdb)(Read2Bytes(u_short,&sdb[sdb_file_st_cluster_off]))
#define sdb_file_size(sdb,x)	(Write4Bytes(&sdb[sdb_file_size_off],x))

typedef u_char *sft_t;

/* ### Split next definition into read (..._r) and write (..._w) ### */
#define sft_handle_cnt_r(sft) 	(Read2Bytes(u_short,&sft[sft_handle_cnt_off]))
#define sft_handle_cnt_w(sft,x) 	(Write2Bytes(&sft[sft_handle_cnt_off], x))
/* ### Split next definition into read (..._r) and write (..._w) ### */
#define sft_open_mode_r(sft)  	(Read2Bytes(u_short,&sft[sft_open_mode_off]))
#define sft_open_mode_w(sft,x)  	(Write2Bytes(&sft[sft_open_mode_off],x))
#define sft_attribute_byte(sft) (*(u_char  *)&sft[sft_attribute_byte_off])
/* ### Split next definition into read (..._r) and write (..._w) ### */
#define sft_device_info_r(sft)  	(Read2Bytes(u_short,&sft[sft_device_info_off]))
#define sft_device_info_w(sft,x)  	(Write2Bytes(&sft[sft_device_info_off], x))
#define	sft_dev_drive_ptr(sft,x)	(Write4Bytes(&sft[sft_dev_drive_ptr_off], x))
#define	sft_start_cluster(sft)	(Read2Bytes(u_short,&sft[sft_start_cluster_off]))
#define	sft_time(sft)		(*(u_short *)&sft[sft_time_off])
#define	sft_date(sft)		(*(u_short *)&sft[sft_date_off])
/* ### Split next definition into read (..._r) and write (..._w) ### */
#define	sft_size_r(sft)		(Read4Bytes(u_long,&sft[sft_size_off]))
#define	sft_size_w(sft, x)		(Write4Bytes(&sft[sft_size_off], x))
/* ### Split next definition into read (..._r) and write (..._w) ### */
#define	sft_position_r(sft)	(Read4Bytes(u_long,&sft[sft_position_off]))
#define	sft_position_w(sft,x)	(Write4Bytes(&sft[sft_position_off],x))
#define sft_rel_cluster(sft)	(Read2Bytes(u_short,&sft[sft_rel_cluster_off]))
#define sft_abs_cluster(sft,x)	(Write2Bytes(&sft[sft_abs_cluster_off], x))
#define	sft_directory_sector(sft,x) (Write2Bytes(&sft[sft_directory_sector_off],x))
#define	sft_directory_entry(sft)  (*(u_char  *)&sft[sft_directory_entry_off])
#define	sft_name(sft)		( (char    *)&sft[sft_name_off])
#define	sft_ext(sft)		( (char    *)&sft[sft_ext_off])

/* ### Split next definition into read (..._r) and write (..._w) ### */
#define	sft_fd_r(sft)		(Read2Bytes(u_short,&sft[sft_fd_off]))
#define	sft_fd_w(sft,x)		(Write2Bytes(&sft[sft_fd_off], x))

typedef u_char *cds_t;

#define	cds_current_path(cds)	((char	   *)&cds[cds_current_path_off])
/* ### Split next definition into read (..._r) and write (..._w) ### */
#define	cds_flags_r(cds)		(Read2Bytes(u_short,&cds[cds_flags_off]))
#define	cds_flags_w(cds,x)		(Write2Bytes(&cds[cds_flags_off], x))
#define cds_DBP_pointer(cds)	(Read4Bytes(u_long,&cds[cds_DBP_pointer_off]))
#define cds_cur_cluster(cds,x)	(Write2Bytes(&cds[cds_cur_cluster_off],x))
/* ### Split next difinition into read (..._r) and write (..._w) ### */
#define	cds_rootlen_r(cds)	(Read2Bytes(u_short,&cds[cds_rootlen_off]))
#define	cds_rootlen_w(cds,x)	(Write2Bytes(&cds[cds_rootlen_off], x))
#define drive_cds(dd) ((cds_t)(((long)cds_base)+(cds_record_size*(dd))))

#define CDS_FLAG_REMOTE		0x8000
#define CDS_FLAG_READY		0x4000
#define CDS_FLAG_NOTNET		0x0080
#define CDS_FLAG_SUBST		0x1000
#define CDS_DEFAULT_ROOT_LEN	2

#define FAR(x) (Addr_8086(x.segment, x.offset))

/* ### Commented out by DH ### 
*  #define FARPTR(x) (Addr_8086((x)->segment, (x)->offset))
*/

/* ### Added by DH ### */
#define FARPTR(x) &memory[Read2Bytes(u_short, (x)) + \
                          (Read2Bytes(u_short, (BYTE *)(x)+2) << 4)]
/* ### End of addition ### */

typedef u_short *psp_t;

#define PSPPTR(x) (Addr_8086(x, 0))

typedef u_char *sda_t;

#define	sda_current_dta(sda)	((char *)(FARPTR((far_t *)&sda[sda_current_dta_off])))
#define sda_cur_psp(sda)		(Read2Bytes(u_short,&sda[sda_cur_psp_off]))
#define sda_filename1(sda)		((char  *)&sda[sda_filename1_off])
#define	sda_filename2(sda)		((char  *)&sda[sda_filename2_off])
#define sda_sdb(sda)			((sdb_t    )&sda[sda_sdb_off])
#define	sda_cds(sda)		((cds_t)(FARPTR((far_t *)&sda[sda_cds_off])))
#define sda_search_attribute(sda)	(*(u_char *)&sda[sda_search_attribute_off])
#define sda_open_mode(sda)		(*(u_char *)&sda[sda_open_mode_off])
#define sda_rename_source(sda)		((sdb_t    )&sda[sda_rename_source_off])
#define sda_user_stack(sda)		((char *)(FARPTR((far_t *)&sda[sda_user_stack_off])))

/*
 *  Data for extended open/create operations, DOS 4 or greater:
 */
#define sda_ext_act(sda)		(Read2Bytes(u_short,&sda[sda_ext_act_off]))
#define sda_ext_attr(sda)		(Read2Bytes(u_short,&sda[sda_ext_attr_off]))
#define sda_ext_mode(sda)		(Read2Bytes(u_short,&sda[sda_ext_mode_off]))

#define psp_parent_psp(psp)		(Read2Bytes(u_short,&psp[0x16]))
#define psp_handles(psp)		((char *)(FARPTR((far_t *)&psp[0x34])))

#define lol_cdsfarptr(lol)		(Read4Bytes(u_long,&lol[lol_cdsfarptr_off]))
#define lol_last_drive(lol)		(*(u_char *)&lol[lol_last_drive_off])

typedef u_char *lol_t;

#ifdef OLD_OBSOLETE
typedef struct lol_record {
  u_char filler1[22];
  far_t cdsfarptr;
  u_char filler2[6];
  u_char last_drive;
} *lol_t;

#endif

/* dos attribute byte flags */
#define REGULAR_FILE 	0x00
#define READ_ONLY_FILE	0x01
#define HIDDEN_FILE	0x02
#define SYSTEM_FILE	0x04
#define VOLUME_LABEL	0x08
#define DIRECTORY	0x10
#define ARCHIVE_NEEDED	0x20

/* dos access mode constants */
#define READ_ACC	0x00
#define WRITE_ACC	0x01
#define READ_WRITE_ACC	0x02

#define COMPAT_MODE	0x00
#define DENY_ALL	0x01
#define DENY_WRITE	0x02
#define DENY_READ	0x03
#define DENY_ANY	0x40

#define CHILD_INHERIT	0x00
#define NO_INHERIT	0x01

#define A_DRIVE		0x01
#define B_DRIVE		0x02
#define C_DRIVE		0x03
#define D_DRIVE		0x04

#define GET_REDIRECTION	2
#define REDIRECT_DEVICE 3
#define CANCEL_REDIRECTION 4
#define EXTENDED_GET_REDIRECTION 5
