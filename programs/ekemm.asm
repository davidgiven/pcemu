	name	driver
	page	55,132
	title	'EKEMM --- EMS 3.2 Driver For Hedley's PCEMU'

;
;
;
; Based on a template (c) Ray Duncan, April 1985
; Changes Copyright 1994, Eric Korpela.
; There is no warantee expressed or implied in this software.
; See the file COPYRIGHT and ems.c for details.

code	segment	public	'CODE'

driver	proc	far

	assume	cs:code,ds:code,es:code

	org 	0

Max_Cmd	equ	16	; MS-DOS command code maximum

cr	equ	0dh
lf	equ	0ah
eom	equ	'$'

	page
;
; Device Driver Header
;
Header	dd	-1		; link to next device, -1=end of list

	dw	0C000h		; Device Attribute word
	dw	Strat		; Device "Strategy" entry point
	dw	Intr		; Device "Interrupt" entry point
	db	'EMMXXXX0'	; character device name

;
;  Double-word pointer to Request Header
;  Passed to Strategy routine by MS-DOS
;

RH_Ptr	dd	?
	page
;
;  MS-DOS Command Codes dispatch table.
;  The "Interrupt" Routine uses this table and the 
;  Command Code supplied in the Request Header to
;  transfer to the appropriate driver subroutine.

Dispatch:
	dw	Init		;  0=initialize driver
	dw	Media_Chk	;  1=media check on block device
	dw	Build_Bpb	;  2=build BIOS parameter block
	dw	IOCTL_Rd	;  3=I/O Control read
	dw	Read		;  4=Read from Device
	dw	ND_Read		;  5=non-destructive read
	dw	Inp_Stat	;  6=return current input status
	dw	Inp_Flush	;  7=flush device input buffers
	dw	Write		;  8=write to device
	dw	Write_Vfy	;  9=write with verify
	dw	Outp_Stat	; 10=return current output status
	dw	Outp_Flush	; 11=flush output buffers
	dw	IOCTL_Wrt	; 12=I/O control write
	dw	Dev_Open	; 13=Device Open
	dw	Dev_Close	; 14=Device Close
	dw	Rem_Media	; 15=removable media
	dw	Out_Busy	; 16=output until busy
	page
;
;  MS-DOS Request Header Structure Definition
;
;  The first 13 bytes of all Request Headers are the same
;  and are referred to as the "Static" part of the header.
;  The number and meaning of the following bytes varies.
;  In this "Struc" definition we show the Request Header
;  contents for Read and Write calls.
;
Request	struc			; Request Header template structure
				; begining of static portion
Rlength	db	?		; length of request header
Unit	db	?		; unit number for this request
Command	db	?		; request header's command code
Status	dw	?		; Driver's return status word
				; bit 15	= Error
				; bits 10-14	= Reserved
				; bit 9		= Busy
				; bit 8		= Done
				; bits 0-7	= Error code if bit 15 = 1
Reserve	db	8 dup (?)	; reserved area
				; end of Static Portion, the remainder in 
				; this example is for Read Write functions.
Media	db	?		; Media Descriptor byte
Address	dd	?		; Memory Address for Transfer
Count	dw	?		; byte/sector count value
Sector	dw	?		; Starting sector value
Request ends			; end of request header template
	page

; Device Driver Strategy Routine
;
; Each time a request is made for this device, MS-DOS
; first calls the "Strategy Routine", then immediately
; calls the "Interrupt routine".

; The Strategy routine is passed the address of the
; Request Header in ES:BX, which it saves in a local
; variable and then returns to MS-DOS.

Strat	proc	far
				; save addresss of Request Header
	mov	word ptr cs:[RH_Ptr],bx
	mov	word ptr cs:[RH_Ptr+2],es

	ret			; back to MS-DOS

Strat	endp
	page

; Device Driver Interrupt Routine

; This entry point is called by MS-DOS immediately after
; the call to the "Strategy Routine",  which saved the long
; address of the Request Header in the local variable "RH_Ptr".

; The "Interrupt Routine" uses the Command Code passed in 
; the Request Header to transfer to the appropriate device
; handling routine.  Each command code routine must place
; any necessary return information into the Request Header,
; then perform a near return with AX=Status.

Intr	proc	far

	push	ax		; save general registers
	push	bx
	push	cx
	push	dx
	push	ds
	push	es
	push	di
	push	si
	push	bp

	push	cs		; make local data addressable
	pop	ds

	les	di,[RH_Ptr]	; let ES:DI = Request header
	
				; get BX = command code
	mov	bl,es:[di.Command]
	xor	bh,bh
	cmp	bx,Max_Cmd	; make sure it's legal.
	jle	Intr1		; jump if fuction code is OK
	mov	ax,8003h	; set Error bit and "Unknown Command" code
	jmp	Intr2

Intr1:	shl	bx,1		; form index to dispatch table
				; and branch to driver routine.
	call	word ptr [bx+Dispatch]
				; should return AX = status.
	les	di,[RH_Ptr]	; restore ES:DI = addr of Request header

Intr2:	or	ax,0100h	; merge Done bit into status, and
				; store into request header
	mov	es:[di.Status],ax

	pop	bp		; restore general registers.
	pop	si
	pop	di
	pop	es
	pop	ds
	pop	dx
	pop	cx
	pop	bx
	pop	ax
	ret			; back to MS-DOS

	page

;
; Command Code subroutines called by Interrupt Routine
;
; These routines are called with ES:DI pointing to the 
; Request Header.
;
; They should return AX = 0 if function was completed
; successfully, or AX = 8000H + Error code if function failed.
;

Write	proc	near		; Function 8 = write
	xor	ax,ax
	ret
Write	endp

Media_Chk	equ	Offset Write	; Function 1 = Media Check

Build_Bpb	equ	Offset Write	; Function 2 = Build BPB

IOCTL_Rd	proc	near	; Function 3 = I/O Control Read

	xor	ax,ax
	mov	es:[di.Count],ax
        mov	al,0FFh
	ret

IOCTL_Rd	endp

Read	equ	Offset IOCTL_Rd	; Function 4 = Read

ND_Read	equ	Offset IOCTL_Rd	; Function 5 = Non-Destructive Read

Inp_Stat	equ	Offset Write	; Function 6 = Input Status

Inp_Flush	equ	Offset Write	; Function 7 = Input Flush

Write_Vfy	equ	Offset Write 	; Function 9 = Write with Verify

Outp_Stat	equ	Offset IOCTL_Wrt	; Function 10 = Output Status

Outp_Flush	equ	Offset Write	; Function 11 = Flush Output Buffers

IOCTL_Wrt	proc 	near	; Function 12 = I/O Control Write
	mov	ax,0FFh
	ret
IOCTL_Wrt	endp

Int67		proc	far
	int 4Bh
	iret	
Int67	endp	
Dev_Open	equ	Offset Write	; Function 13 = Device Open

Dev_Close	equ	Offset Write	; Function 14 = Device Close

Rem_Media	equ	Offset Write	; Function 15 = Removable Media

Out_Busy	equ	Offset Write	; Function 16 = Output until busy

	page

; The initialization code for the driver is called only
; once, when the driver is loaded.  It is reasponsible for
; initializing the hardware, setting up any necessary
; interrupt vectors, and it must return the address 
; of the first free memory after the driver to MS-DOS.
; If it is a block device driver, Init must also return the
; address of the BIOS parameter block pointer array; if all
; units are the same, all pointers can be to the same BPB.
; Only MS-DOS services 01-0CH and 30H can be called by the
; Initialization Function.
;
; In this example, Init returns its own address to the DOS as
; the start of free memory after the driver, so that the memory
; occupied by INIT will be reclaimed after it is finished
; with its work.

Init	proc	near		; Function 0 = Initialize Driver

	push	es		; save address of Request Header.
	push	di

	mov	ax,cs		; convert load address to ASCII.
	mov	bx, offset DHaddr
	call	hexasc

	mov	ah,9		; print sign-on message and
	mov	dx,offset Ident	; the load address of the driver.
	int	21h
	push	ds
	push	dx
        mov	al,67h
	mov	dx,cs
        mov	ds,dx
	mov	dx,offset Int67
        mov	ah,25h
	int	21h

	pop	dx
	pop	ds
	pop	di		; restore Request Header addr.
	pop	es

				; set first usable memory address
	mov	word ptr es:[di.Address],offset init
	mov	word ptr es:[di.Address+2],cs

	xor	ax,ax		; Return Status
	ret

Init	endp

Ident	db	cr,lf,lf
	db	'EKEMS Device Driver 1.0'
        db	cr,lf
	db	'Device Header at '

DHaddr	db	'XXXX:0000'
	db	cr,lf,lf,eom

Intr	endp

	page

; HEXASC --- converts a binary 16 bit number into
;            a "hexidecimal" ASCII string.
;
; Call with	AX	= value to convert
;		DS:BX	= address to store 4 character string
;
; Returns	AX, BX	destroyed, other registers preserved

hexasc	proc	near

	push	cx	; save registers.
	push	dx

	mov	dx,4	; initialize character counter.

hexasc1:
	mov	cx,4	; isolate next 4 bits.
	rol	ax,cl
	mov	cx,ax
	and	cx,0fh
	add	cx,'0'	; convert to ASCII.
	cmp	cx,'9'	; is it 0-9?
	jbe	hexasc2	; yes, jump.
			; add fudge factor for A-F.
	add	cx,'A'-'9'-1

hexasc2:		; store this character.
	mov	[bx],cl
	inc	bx	; bump string pointer.

	dec	dx	; count characters converted.
	jnz	hexasc1	; loop, not four yet.

	pop	dx	; restore registers.
	pop	cx
	ret		; back to caller

hexasc	endp

Driver	endp

code	ends

	end

	
