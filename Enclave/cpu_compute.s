.globl	asm_compute_CPU
.type	asm_compute_CPU, @function
asm_compute_CPU:
  #preload start
  movq enc_data@GOTPCREL(%rip), %rax
  mov (%rax),%rcx
  mov 4(%rax),%rcx
  mov 8(%rax),%rcx
  mov 12(%rax),%rcx
  movq enc_perm@GOTPCREL(%rip), %rax
  mov (%rax),%rcx
  mov 4(%rax),%rcx
  mov 8(%rax),%rcx
  mov 12(%rax),%rcx
  movq enc_output@GOTPCREL(%rip), %rax
  mov (%rax),%rcx
  mov 4(%rax),%rcx
  mov 8(%rax),%rcx
  mov 12(%rax),%rcx
  #preload end
 # xbegin ll
  movq enc_data@GOTPCREL(%rip), %rax
  movl (%rax),%ecx
  movq enc_perm@GOTPCREL(%rip), %rax
  movl (%rax),%edx
  movq enc_output@GOTPCREL(%rip), %rax
  movl %ecx,(%rax,%rdx,4)
  movq enc_data@GOTPCREL(%rip), %rax
  movl 4(%rax),%ecx
  movq enc_perm@GOTPCREL(%rip), %rax
  movl 4(%rax),%edx
  movq enc_output@GOTPCREL(%rip), %rax
  movl %ecx,(%rax,%rdx,4)
  movq enc_data@GOTPCREL(%rip), %rax
  movl 8(%rax),%ecx
  movq enc_perm@GOTPCREL(%rip), %rax
  movl 8(%rax),%edx
  movq enc_output@GOTPCREL(%rip), %rax
  movl %ecx,(%rax,%rdx,4)
  movq enc_data@GOTPCREL(%rip), %rax
  movl 12(%rax),%ecx
  movq enc_perm@GOTPCREL(%rip), %rax
  movl 12(%rax),%edx
  movq enc_output@GOTPCREL(%rip), %rax
  movl %ecx,(%rax,%rdx,4)
#  xend
ll:
  ret
