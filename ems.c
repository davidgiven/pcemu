/* ems.c- Adds LIM-EMS 3.2 to the PC emulator 
   Copyright 1994, Eric Korpela
   Permission is given to copy, modify and distribute this code
   provided that this copyright notice is not modified.         
   
BECAUSE THE PROGRAM IS LICENSED FREE OF CHARGE, THERE IS NO WARRANTY
FOR THE PROGRAM, TO THE EXTENT PERMITTED BY APPLICABLE LAW.  EXCEPT
WHEN OTHERWISE STATED IN WRITING THE COPYRIGHT HOLDERS AND/OR OTHER
PARTIES PROVIDE THE PROGRAM "AS IS" WITHOUT WARRANTY OF ANY KIND,
EITHER EXPRESSED OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE.  THE ENTIRE RISK AS TO THE QUALITY AND PERFORMANCE OF THE
PROGRAM IS WITH YOU.  SHOULD THE PROGRAM PROVE DEFECTIVE, YOU ASSUME
THE COST OF ALL NECESSARY SERVICING, REPAIR OR CORRECTION.

IN NO EVENT UNLESS REQUIRED BY APPLICABLE LAW OR AGREED TO IN WRITING
WILL ANY COPYRIGHT HOLDER, OR ANY OTHER PARTY WHO MAY MODIFY AND/OR
REDISTRIBUTE THE PROGRAM AS PERMITTED ABOVE, BE LIABLE TO YOU FOR
DAMAGES, INCLUDING ANY GENERAL, SPECIAL, INCIDENTAL OR CONSEQUENTIAL
DAMAGES ARISING OUT OF THE USE OR INABILITY TO USE THE PROGRAM
(INCLUDING BUT NOT LIMITED TO LOSS OF DATA OR DATA BEING RENDERED
INACCURATE OR LOSSES SUSTAINED BY YOU OR THIRD PARTIES OR A FAILURE OF
THE PROGRAM TO OPERATE WITH ANY OTHER PROGRAMS), EVEN IF SUCH HOLDER
OR OTHER PARTY HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
*/
/* bug reports to <korpela@ssl.berkeley.edu> */
#include "global.h"

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <sys/stat.h>

#include "cpu.h"
#include "vga.h"
#include "bios.h"
#include "debugger.h"
#include "vgahard.h"
#include "hardware.h"

#define BIOS
#define Total_EMS_Pages 1024
#define Total_EMS_Handles 255
#ifndef PAGEFRAME 
#define PAGEFRAME 0xe000
#endif


unsigned Unallocated_Pages, Free_EMS_Handles, 
  Allocated_Pages[Total_EMS_Handles+1], 
  EMS_Context_Pages[4], EMS_Context_Handles[4],
  Mapping_Context_Pages[Total_EMS_Handles+1][4],
  Mapping_Context_Handles[Total_EMS_Handles+1][4];

BYTE Handle_Flag[Total_EMS_Handles+1];

BYTE *EMS_Pointer[Total_EMS_Handles+1];

void EMS_Setup(void)	
{ 
  unsigned tmp,tmp2;
  D(printf("In EMS setup\n"););
  Unallocated_Pages=Total_EMS_Pages; 
  for (tmp=0;tmp<=Total_EMS_Handles+1;tmp++) 
  {
    /* Added the =0. Was just for(tmp2;tmp2... Accident? dtrg */
    for (tmp2=0;tmp2<4;tmp2++) 
    {
      Mapping_Context_Pages[tmp][tmp2]=0;
      Mapping_Context_Handles[tmp][tmp2]=0;
    }
    Handle_Flag[tmp]=0;
    Allocated_Pages[tmp]=0;
    Free_EMS_Handles=Total_EMS_Handles;
  } 
  EMS_Pointer[0]=(BYTE *)malloc(0x04000*4);
  Allocated_Pages[0]=4;
  Handle_Flag[0]=1;
}

void EMS_Map(unsigned handle, unsigned logical_page, unsigned physical_page)
{
  unsigned tmp,tmp1;
  BYTE *ptr0,*ptr1,*ptr2;
  D(printf("Mapping EMS handle=%04X, logical_page=%04X, physical_page=%1X\n"
           ,handle,logical_page,physical_page););
  if ((Allocated_Pages[handle]>logical_page) || (handle==0))
  { 
    ptr0=&memory[(PAGEFRAME << 4) + physical_page * 0x04000 ];
    D(printf("Mapping Page to 0x%05X\n",(PAGEFRAME << 4) + physical_page * 0x04000););
    ptr2=EMS_Pointer[handle]+logical_page*0x04000;
    if ((EMS_Context_Handles[physical_page]!=0)&&((EMS_Context_Handles[physical_page]!=handle)||(EMS_Context_Pages[physical_page]!=logical_page)))
    { 
      ptr1=EMS_Pointer[EMS_Context_Handles[physical_page]]
           +EMS_Context_Pages[physical_page]*0x04000;
      memcpy(ptr1,ptr0,0x04000);
      D(printf("EMS ptr0(0x%08X) ---> ptr1(0x%08X)\n",ptr0,ptr1););
    }
    if ((EMS_Context_Handles[physical_page]!=handle)||(EMS_Context_Pages[physical_page]!=logical_page))
    {
      D(printf("EMS ptr2(0x%08X) ---> ptr0(0x%08X)\n",ptr2,ptr0););
      memcpy(ptr0,ptr2,0x04000);
    }
    *bregs[AH]=0x00;
  } else *bregs[AH]=0x8A;
  EMS_Context_Handles[physical_page]=handle;
  EMS_Context_Pages[physical_page]=logical_page;
  tmp1=0;
  for (tmp=0;tmp<4;tmp++)
    if   ((EMS_Context_Handles[tmp]==handle)
       &&(EMS_Context_Pages[tmp]==logical_page)&&(handle!=0)) tmp1++;
  if ((tmp1!=1)&&(handle!=0))
    printf("WARNING--Multiply mapped page: Handle 0x%02X, Page 0x%04X\n",handle,logical_page);
}  

void int_ems(void)
{
    unsigned tmp,tmp2,tmp3;
    D(printf("In INT 0x67 (EMS) AH = 0x%02X  AL = 0x%02X\n",*bregs[AH],*bregs[AL]););
    CalcAll();
    switch(*bregs[AH])
    {
    case 0x40:  *bregs[AH] = 0;
                break;
    case 0x41:  wregs[BX] = ChangeE(PAGEFRAME);
                *bregs[AH] = 0x00;
                break;
    case 0x42:  *bregs[AH] = 0x00;
                wregs[BX] = ChangeE((WORD)Unallocated_Pages);
                wregs[DX] = ChangeE((WORD)Total_EMS_Pages);
                break;
    case 0x43:  tmp=ChangeE(wregs[BX]);
                if (tmp>Total_EMS_Pages) {*bregs[AH]=0x87;break;}
                if (tmp>Unallocated_Pages) {*bregs[AH]=0x88;break;}
                if (tmp==0) {*bregs[AH]=0x89;break;}
                if (Free_EMS_Handles==0) {*bregs[AH]=0x85;break;}
                for (tmp2=1;tmp2<Total_EMS_Handles;tmp2++)
                    { if (Handle_Flag[tmp2]==0) break; }
                Handle_Flag[tmp2]=1;
                wregs[DX]=ChangeE(tmp2);
                EMS_Pointer[tmp2]=(BYTE *)malloc(0x4000*tmp);
                memset(EMS_Pointer[tmp2],0,0x4000*tmp);
                Allocated_Pages[tmp2]=tmp;
                Unallocated_Pages-=tmp;
                Free_EMS_Handles--;
                *bregs[AH]=0x00;
                break;
    case 0x44:  tmp=ChangeE(wregs[DX]);
                tmp2=ChangeE(wregs[BX]);
                tmp3=*bregs[AL];
                if (tmp3>3) {*bregs[AH]=0x8B;break;}
                EMS_Map(tmp,tmp2,tmp3); 
                break;
    case 0x45:  tmp=ChangeE(wregs[DX]);
                if (tmp==0) 
                {
                  printf("Warning--Attempt to unallocate handle 0x00\n");
                  *bregs[AH]=0x83;
                }
                Handle_Flag[tmp]=0;
                Unallocated_Pages+=Allocated_Pages[tmp];
                Allocated_Pages[tmp]=0;
                free(EMS_Pointer[tmp]);
                Free_EMS_Handles++;
                *bregs[AH]=0x00;
                break;
    case 0x46:  *bregs[AH]=0x00;
                *bregs[AL]=0x32;
                break;
    case 0x47:  tmp=ChangeE(wregs[DX]);
                for (tmp2=0;tmp2<4;tmp2++) 
                {
                  Mapping_Context_Pages[tmp][tmp2]=EMS_Context_Pages[tmp2];
                  Mapping_Context_Handles[tmp][tmp2]=EMS_Context_Handles[tmp2];
                }
                *bregs[AH]=0x00;
                break;
    case 0x48:  tmp=ChangeE(wregs[DX]);
                for (tmp2=0;tmp2<4;tmp2++) 
                {
                  EMS_Map(Mapping_Context_Handles[tmp][tmp2],
                          Mapping_Context_Pages[tmp][tmp2],tmp2);
                }
                *bregs[AH]=0x00;
                break;
    case 0x4b:  wregs[BX]=ChangeE(Total_EMS_Handles-Free_EMS_Handles);
                *bregs[AH]=0x00;
                break;
    case 0x4c:  tmp=ChangeE(wregs[DX]);
                wregs[BX]=ChangeE(Allocated_Pages[tmp]);
                *bregs[AH]=0x00;
                break;
    case 0x4d:  tmp=Total_EMS_Handles-Free_EMS_Handles;
                tmp3=0;
                for (tmp2=1;tmp2<Total_EMS_Handles;tmp2++) 
                  if (Handle_Flag[tmp2]==1)
                  {
                    PutMemW(SegToMemPtr(ES),ChangeE(wregs[DI])+(WORD)tmp3,tmp2);
                    PutMemW(SegToMemPtr(ES),ChangeE(wregs[DI])+(WORD)(2+tmp3)
                       ,Allocated_Pages[tmp2]);
                    tmp3+=4;
                  }
                wregs[BX]=ChangeE(tmp);
                *bregs[AH]=0x00;
                break;
    case 0x4e:  *bregs[AH]=0;
                  for (tmp=0;tmp<4;tmp++)
                  switch(*bregs[AL])
                  {
                    case 0x00:  PutMemW(SegToMemPtr(ES),
                                    ChangeE(wregs[DI])+(WORD)(tmp*4),
                                    EMS_Context_Pages[tmp]);
                                PutMemW(SegToMemPtr(ES),
                                    ChangeE(wregs[DI])+(WORD)(tmp*4+2),
                                    EMS_Context_Handles[tmp]);
                                break;
                    case 0x01:  EMS_Map(GetMemW(SegToMemPtr(DS),
                                    ChangeE(wregs[SI])+(WORD)(tmp*4+2)),
                                    GetMemW(SegToMemPtr(DS),
                                    ChangeE(wregs[SI])+(WORD)(tmp*4)),tmp);
                                break;
                    case 0x02:  PutMemW(SegToMemPtr(ES),
                                    ChangeE(wregs[DI])+(WORD)(tmp*4),
                                    EMS_Context_Pages[tmp]);
                                PutMemW(SegToMemPtr(ES),
                                    ChangeE(wregs[DI])+(WORD)(tmp*4+2),
                                    EMS_Context_Handles[tmp]);
                                EMS_Map(GetMemW(SegToMemPtr(DS),
                                    ChangeE(wregs[SI])+(WORD)(tmp*4+2)),
                                    GetMemW(SegToMemPtr(DS),
                                    ChangeE(wregs[SI])+(WORD)(tmp*4)),tmp);
                                break;
                    case 0x03:  *bregs[AL]=8;
                                break;
                    case 0x08:  break;
                    default:    *bregs[AH]=84;
                  }

   default:	printf("Unrecognized EMS function 0x%02X\n",*bregs[AH]);
                *bregs[AH]=0x84;   
  }
}

