// V7/x86 source code

.code32

.text

kernel:
	int $0x80

	jmp open

kernel2:
	ret 

open:
        push [_mode]
        push [_flags]
        push [_path]
        mov  $5, eax
	call kernel2
        add  $12, esp
        ret

.data

_mode:  .long 5
_flags: .long 3
_path:  .long 7
