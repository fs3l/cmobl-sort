.globl	asm_cache_hit_simulate
.type	asm_cache_hit_simulate, @function
asm_cache_hit_simulate:
mov %rdi,%rcx
mov $0,%rax
loop2:
cmp %rsi,%rax
jge endloop2
#movl $5,(%rcx)
nop
add $1,%rax
#add $4,%rcx
jmp loop2
endloop2:
ret
