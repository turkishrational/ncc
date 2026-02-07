.bits 32

.text

kernel:
	int 0x80 ; Call kernel

	jmp open

kernel2:
	ret 

open:
        push dword [_mode]
        push dword [_flags]
        push dword [_path]
        mov eax, 5
	call kernel2
        add esp, 12
        ret

.data

_mode:  .dword 5
_flags: .dword 3
_path:  .dword 7
