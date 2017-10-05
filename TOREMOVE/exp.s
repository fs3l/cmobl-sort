.globl	asm_cache_hit_simulate
.type	asm_cache_hit_simulate, @function
asm_cache_hit_simulate:
  mov $0,%rax
loop3:
  cmp %rsi,%rax
  jge endloop3
#  movl $5,(%rdi)
#  nop
#  nop
  add $1,%rax
  jmp loop3
endloop3:
  ret
