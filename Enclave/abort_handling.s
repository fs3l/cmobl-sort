.globl	asm_abort_handler
.type	asm_abort_handler, @function
asm_abort_handler:
  mov %eax,%edi
  push %rax
  push %rbx
  push %rcx
  push %rdx
  push %rdi
  push %rsi
  push %r8
  push %r9
  push %r10
  push %r11
  push %r12
  push %r13
  push %r14
  push %r15
  call tx_abort
  pop %r15
  pop %r14
  pop %r13
  pop %r12
  pop %r11
  pop %r10
  pop %r9
  pop %r8
  pop %rsi
  pop %rdi
  pop %rdx
  pop %rcx
  pop %rbx
  pop %rax
  jmp %r14
  ret
