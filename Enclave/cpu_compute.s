.globl	asm_compute_CPU
.type	asm_compute_CPU, @function
asm_compute_CPU:
  pushq	%rbp
  movq	%rsp, %rbp
  movl	$0, -4(%rbp) #index pointer for slot 0
  movl	$0, -8(%rbp) #index pointer for slot 1 
  movl	$0, -12(%rbp) #index pointer for slot 2
  movl	$0, -16(%rbp) #index pointer for slot 3
  #preload start
  movq E_data@GOTPCREL(%rip), %rax
  mov (%rax),%rcx
  mov 4(%rax),%rcx
  mov 8(%rax),%rcx
  mov 12(%rax),%rcx
  movq E_perm@GOTPCREL(%rip), %rax
  mov (%rax),%rcx
  mov 4(%rax),%rcx
  mov 8(%rax),%rcx
  mov 12(%rax),%rcx
  movq E_output@GOTPCREL(%rip), %rax
  mov (%rax),%rcx
  mov 4(%rax),%rcx
  mov 8(%rax),%rcx
  mov 12(%rax),%rcx
  movq E_output@GOTPCREL(%rip), %rcx
  movq E_data@GOTPCREL(%rip), %rax
  movq E_perm@GOTPCREL(%rip), %rbx
  #preload end
  xbegin ll
  movl (%rax),%esi  #esi = data[0]
  movl (%rbx),%edx  #edx = perm[0]
  sarl $2,%edx      #edx=edx/4 
  movl %esi,(%rcx,%rdx,4)
  movl 4(%rax),%esi
  movl 4(%rbx),%edx
  #sall $2,%edx      #edx=edx/4 
  movl %esi,(%rcx,%rdx,4)
  movl 8(%rax),%esi
  movl 8(%rbx),%edx
  #sall $2,%edx      #edx=edx/4 
  movl %esi,(%rcx,%rdx,4)
  movl 12(%rax),%esi
  movl 12(%rbx),%edx
  #sall $2,%edx      #edx=edx/4 
  movl %esi,(%rcx,%rdx,4)
  xend
  mov $0, %rax
  popq %rbp
  ret
ll:
  mov $777,%rax
  popq %rbp
  ret
