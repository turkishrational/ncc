.bits 16

.text

kernel:
	mov si, message	 	 
        call print_msg
	ret 

print_msg:
        mov ah, 0x0E
        mov bx, 7 
loop:
        lodsb
        and al, al
        jz ok
        int 0x10
ok:
        ret 

.data

message .byte "Hello World!", 0x0D, 0x0A, 0>
