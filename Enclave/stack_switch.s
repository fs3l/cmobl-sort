.globl	coda_stack_switch
.type	coda_stack_switch, @function
coda_stack_switch:
  mov old_rsp@GOTPCREL(%rip), %r15
  mov %rsp,(%r15)
  mov old_rbp@GOTPCREL(%rip), %r15
  mov %rbp,(%r15)
  mov tail@GOTPCREL(%rip), %r15
  mov (%r15), %r15 #content of memory address of tail 
  mov %rbp,%rax
  sub %rsp,%rax
  sub %rax,%r15
  sub $100, %r15
  mov %r15,%rdx
  mov $0, %r8
  loop_cy:
  cmp %rax,%r8
  jge loopend_cy
  movb (%rsp,1),%cl
  movb %cl, (%rdx,1)
  add $1,%rsp
  add $1,%rdx
  add $1,%r8
  jmp loop_cy
  loopend_cy:
  mov $0, %r8
  loop_old:
  cmp $100,%r8  #8 for the ebp and 8 for the return address
  jge loopend_old
  movb (%rsp,1),%cl
  movb %cl, (%rdx,1)
  add $1,%rsp
  add $1,%rdx
  add $1,%r8
  jmp loop_old
  loopend_old:
  mov %r15,%rsp
  mov %rsp,%rbp
  add %rax,%rbp
  ret
