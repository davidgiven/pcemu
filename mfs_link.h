/****************************************************************************
*                                                                           *
*                            Third Year Project                             *
*                                                                           *
*                            An IBM PC Emulator                             *
*                          For Unix and X Windows                           *
*                                                                           *
*                             By David Hedley                               *
*                                                                           *
*                                                                           *
* This program is Copyrighted.  Consult the file COPYRIGHT for more details *
*                                                                           *
****************************************************************************/

/* Attempt to fix mfs. dtrg */

/* #define u_long UINT32 */

struct vm86_regs
{
    INT16 eax, ebx, ecx, edx, esi, edi, ebp, esp;
    INT16 cs,ds,es,ss;
    INT16 eip;
    UINT16 eflags;
};

struct vm86_struct 
{
    struct vm86_regs regs;
    UINT32 flags;
    UINT32 screen_bitmap;
};

extern struct vm86_struct vm86s;
extern unsigned char *memory;

#define us unsigned short

#ifndef BIOS

#define LWORD(reg)      (*((unsigned short *)&REG(reg)))
#define HWORD(reg)      (*((unsigned short *)&REG(reg) + 1))

#define SEG_ADR(type, seg, reg)  type(&memory[(LWORD(seg) << 4)+LWORD(e##reg)])


#define REGS  vm86s.regs
#define REG(reg) (REGS.##reg)

#define CF     (1 << 0)
#define TF     (1 <<  8)
#define IF     (1 <<  9)
#define NT     0


#define IS_REDIRECTED(i) (memory[(i << 2)+2] != 0xf000)

#define INTE7_SEG (memory[(0xe7 << 2)+2] + (memory[(0xe7 << 2)+3] << 8))
#define INTE7_OFF (memory[0xe7 << 2] + (memory[(0xe7 << 2) + 1] << 8))

#define error printf

/* Some compilers cannot handle variable numbers of arguments in #defines
#ifdef DEBUG
#   define d_printf(arg1, a...) printf(arg1, ##a)
#else
#   define d_printf(arg1, a...) 
#endif
*/

#endif

int mfs_intentry(void);
int mfs_redirector(void);




