.text
_fprintf:
L1:
	pushq %rbp
	movq %rsp,%rbp
L6:
	movq 16(%rbp),%rdi
	movq 24(%rbp),%rsi
	leaq 32(%rbp),%rdx
	call _vfprintf
L3:
	popq %rbp
	ret
L8:
.globl _vfprintf
.globl _fprintf
