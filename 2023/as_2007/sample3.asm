// V7/x86 source code

// Erdogan Tan - 13/10/2023
// note: a.out header and org 100h problem. AS.EXE assembles
//       this file correcly but not proper for dos COM running yet.

// todo: direct COM binary with 100h start address or adding
//       ORG pseudo operation to the assembler's source code in
//	 addition to COM file assembling option.
//
//	 -b: (raw) binary, -e: ELF -c: COM -x: EXE out option.	

.code16

.text
	mov	$0x10, ax
	mov	ax, ds

	mov	msg, si
	call	print_msg
terminate:
	int	$0x20
hang:
	jmp	hang

print_msg:
	mov	$7, bx
	mov	$0x0E, ah
print_loop:
	lodsb
	and	al, al
	jz	print_ret
	int	$0x10
	jmp	print_loop
print_ret:
	ret

msg:	
	.byte	0x0A
	.ascii "UNIX v7 x86 Assembler - DOS .COM binary file test."
	.byte	0x0A, 0