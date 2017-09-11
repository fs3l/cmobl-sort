.globl	asm_tx_begin
.type	asm_tx_begin, @function
asm_tx_begin:
# rdi=inter1,rsi=sqrtN*blowupfactor and rdx=sqrtN
#  pushq	%rbp
# movq	%rsp, %rbp
#prefetch external
  movl %esi, %r8d
  mov $0, %eax
  mov %rdi, %rcx
loop1:
  cmpl  %r8d,%eax  #r8d is the LSB of the r8
 jge    endloop1
  movl   $5, (%rcx)
 addl   $1, %eax
  add    $4, %rcx
  jmp    loop1
endloop1:
#xbegin
 xbegin asm_abort_handler
#prefetch internal 
  mov %rdi,%rcx
  mov $0, %eax
loop:
  cmpl  %r8d,%eax
  jge    endloop
  movl   $5, (%rcx)
   addl   $1, %eax
  add   $4, %rcx
  jmp    loop
endloop:
#app_logic: start permutation
  mov %rdi,%r8  #r8 = inter1
  mov %rsi,%r12 # r12 = size1
  sall $2,%r12d  # r12=4*size1
  add %r12,%r8   #r8 is data
  mov %r8,%r9    # r9 = data
  mov %rdx,%r12  #r12 = size2
  sall $2,%r12d  #r12=r12*4
  add %r12,%r9  #r9 is now permu
  mov $0,%eax  #eax = 0
loop2:
  cmpl %edx,%eax
  jge endloop2
  movl (%r8),%r10d  #data[i] -> r10
  movl (%r9),%r11d  #permu[i] -> r11
  sall $2,%r11d     #r11 = r11 *4
  mov %rdi,%rcx  #rcx = inter1
  add  %r11,%rcx    #rcx is now output[permu[i]]
  movl %r10d,(%rcx) #assign value
  addl $1,%eax  #increment
  add $4,%r8   #increment
  add $4,%r9  #increment
  jmp loop2
endloop2:
#popq %rbp
#xend
  xend
  ret
