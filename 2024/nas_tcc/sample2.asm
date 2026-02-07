.bits 32

.text

start:
	mov esi, message	 	 
        call print_msg

	mov eax, 1
	int 0x40

print_msg:
        mov ah, 0x0E
        mov ebx, 7 
loop:
        lodsb
        and al, al
        jz ok
        int 0x31
        jmp loop
ok:
        ret 

message: .ascii 'Hello World!'
         .byte 0x0D,0x0A,0

