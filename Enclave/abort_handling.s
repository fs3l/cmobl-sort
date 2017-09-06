.globl	asm_abort_handler
.type	asm_abort_handler, @function
asm_abort_handler:
  mov %eax,%edi
  call tx_abort
  ret
