.text
_toupper:
L1:
	pushq %rbp
	movq %rsp,%rbp
L9:
	movl %edi,%eax
	leal -97(%rax),%esi
	cmpl $26,%esi
	jae L3
L4:
	addl $-32,%eax
L3:
	popq %rbp
	ret
L11:
.globl _toupper
