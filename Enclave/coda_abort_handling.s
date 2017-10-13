.globl	coda_abort_handler
.type	coda_abort_handler, @function
coda_abort_handler:
  mov %eax,%edi
  push %r14
  push %r15
  call coda_tx_abort
  pop %r15
  pop %r14
#  mov coda_context@GOTPCREL(%rip), %r15
  mov (%r15), %rax
  mov 8(%r15), %rbx
  mov 16(%r15), %rcx
  mov 24(%r15), %rdx
  mov 32(%r15), %rdi
  mov 40(%r15), %rsi
  mov 48(%r15), %r8
  mov 56(%r15), %r9
  mov 64(%r15), %r10
  mov 72(%r15), %r11
  mov 80(%r15), %r12
  mov 88(%r15), %r13
  jmp %r14
