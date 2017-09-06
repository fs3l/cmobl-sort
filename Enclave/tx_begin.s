.globl	asm_tx_begin
.type	asm_tx_begin, @function
asm_tx_begin:
  pushq	%rbp
  movq	%rsp, %rbp
  mov %esi, %edx
  mov $0, %eax
  mov %rdi, %rbx
  mov %rbx,%rcx
loop1:
  cmpl  %edx,%eax
  jge    endloop1
  movl   $5, (%rcx)
  addl   $1, %eax
  add    $4, %rcx
  jmp    loop1
endloop1:
  xbegin asm_abort_handler
  mov %rbx,%rcx
loop:
  cmpl  %edx,%eax
  jge    endloop
  movl   $5, (%rcx)
  addl   $1, %eax
  add   $4, %rcx
  jmp    loop
endloop:
popq %rbp
  ret
