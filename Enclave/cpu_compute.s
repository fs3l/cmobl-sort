.globl	asm_compute_CPU
.type	asm_compute_CPU, @function
asm_compute_CPU:
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
  movl (%rax),%esi
  movl (%rbx),%edx
  movl %esi,(%rcx,%rdx,4)
  movl 4(%rax),%esi
  movl 4(%rbx),%edx
  movl %esi,(%rcx,%rdx,4)
  movl 8(%rax),%esi
  movl 8(%rbx),%edx
  movl %esi,(%rcx,%rdx,4)
  movl 12(%rax),%esi
  movl 12(%rbx),%edx
  movl %esi,(%rcx,%rdx,4)
  xend
  //mov $0, %rax
  ret
ll:
  mov $777,%rax
  ret
