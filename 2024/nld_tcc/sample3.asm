.bits 16

.text

start:
	mov si, message	 	 
        call print_msg

	int 0x20

print_msg:
        mov ah, 0x0E
        mov bx, 7 
loop:
        lodsb
        and al, al
        jz ok
        int 0x10
        jmp loop
ok:
        ret 

.data

message: .ascii 'Hello World!'

         .byte 0x0D,0x0A,0



