#include "./RealLibCoda.h"

//coda is a singleton 
struct RealCoda theCoda;
unsigned long old_rsp;
unsigned long old_rbp;
unsigned long tail = (unsigned long)&theCoda.txmem[1024*1024-4];
uint64_t coda_context[100];
int coda_aborts = 0;
void increment_meta(INDEX* meta) {
  if ((*meta%1024)==15)
    *meta =  *meta+1009; //1024-15
  else *meta =  *meta+1;
}


INDEX cal_ob(INDEX offset) {
  return (offset/16)*1024 + offset%16+16;
}

INDEX cal_ob_rw(INDEX offset) {
  return (offset/48)*1024 + offset%48 + 32;
}

INDEX cal_nob(INDEX offset) {
  return (offset/640)*1024 + offset%640 + 80;
}

HANDLE initialize_ob_iterator(DATA* data, LENGTH len) {
  //first, allocate a handle for it
  theCoda.handle++;
  //second, fill in the meta data
  INDEX meta = theCoda.cur_meta;
  theCoda.txmem[meta] = theCoda.cur_ob;
  increment_meta(&meta);
  theCoda.txmem[meta] = len;
  increment_meta(&meta);
  theCoda.txmem[meta] = 0;
  increment_meta(&meta);
  theCoda.cur_meta = meta;
  //third, fill the data
  INDEX iob = theCoda.cur_ob;
  for(INDEX i=0;i<len;i++) {
    theCoda.txmem[cal_ob(iob)] = data[i];
    iob++;
  }
  theCoda.cur_ob = iob;
  return theCoda.handle;
}

HANDLE initialize_ob_rw_iterator(DATA* data, LENGTH len) {
  //first, allocate a handle for it
  theCoda.handle++;
  //second, fill in the meta data
  INDEX meta = theCoda.cur_meta;
  theCoda.txmem[meta] = theCoda.cur_ob_rw;
  increment_meta(&meta);
  theCoda.txmem[meta] = 0;
  increment_meta(&meta);
  theCoda.txmem[meta] = 0;
  increment_meta(&meta);
  theCoda.cur_meta = meta;
  //third, fill the data
  INDEX iob = theCoda.cur_ob_rw;
  for(INDEX i=0;i<len;i++) {
    theCoda.txmem[cal_ob_rw(iob)] = data[i];
    iob++;
  }
  theCoda.cur_ob_rw = iob;
  return theCoda.handle;
}


HANDLE initialize_nob_array(DATA* data, LENGTH len) {
  //first, allocate a handle for it
  theCoda.handle++;
  // second, fill the meta data
  INDEX meta = theCoda.cur_meta;
  theCoda.txmem[meta] = theCoda.cur_nob;
  increment_meta(&meta);
  theCoda.txmem[meta] = len;
  increment_meta(&meta);
  theCoda.txmem[meta] = 0;
  increment_meta(&meta);
  theCoda.cur_meta = meta;
  // third, fill the coda
  INDEX inob = theCoda.cur_nob;
  for(INDEX i=0;i<len;i++) {
    theCoda.txmem[cal_nob(inob)] = data[i];
    inob++;
  }
  theCoda.cur_nob = inob;
  return theCoda.handle;
}


DATA nob_read_at(HANDLE h, INDEX pos) {
  //first, find the meta data slot
  INDEX offset = ((h*3)/16)*1024 + (h*3)%16;
  INDEX ob_start = theCoda.txmem[offset];
  INDEX ob_len = theCoda.txmem[offset+1];
  //calculate the target address
  INDEX target = cal_nob(ob_start+pos);
  return theCoda.txmem[target];
}

void nob_write_at(HANDLE h, INDEX pos, DATA d) {
  //first, find the meta data slot
  INDEX offset = ((h*3)/16)*1024 + (h*3)%16;
  INDEX ob_start = theCoda.txmem[offset];
  INDEX ob_len = theCoda.txmem[offset+1];
  //calculate the target address
  INDEX target = cal_nob(ob_start+pos);
  theCoda.txmem[target] = d;
}

DATA ob_read_next(HANDLE h) {
  //first, find the meta data slot
  INDEX offset = ((h*3)/16)*1024 + (h*3)%16;
  INDEX ob_start = theCoda.txmem[offset];
  INDEX ob_len = theCoda.txmem[offset+1];
  INDEX ob_cur = theCoda.txmem[offset+2];
  //calculate the target address
  INDEX target = cal_ob(ob_start+ob_cur);
  //increment cur position for ob
  theCoda.txmem[offset+2]++;
  return theCoda.txmem[target];
}

void ob_rw_write_next(HANDLE h, DATA d) {
  //first, find the meta data slot
  INDEX offset = ((h*3)/16)*1024 + (h*3)%16;
  INDEX ob_start = theCoda.txmem[offset];
  INDEX ob_read_pointer = theCoda.txmem[offset+1];
  INDEX ob_write_pointer = theCoda.txmem[offset+2];
  //calculate the target address
  INDEX target = cal_ob_rw(ob_start+ob_write_pointer);
  //increment cur position for ob
  theCoda.txmem[offset+2]++;
  theCoda.txmem[target] = d;
}

DATA ob_rw_read_next(HANDLE h) {
  //first, find the meta data slot
  INDEX offset = ((h*3)/16)*1024 + (h*3)%16;
  INDEX ob_start = theCoda.txmem[offset];
  INDEX ob_read_pointer = theCoda.txmem[offset+1];
  INDEX ob_write_pointer = theCoda.txmem[offset+2];
  //calculate the target address
  INDEX target = cal_ob_rw(ob_start+ob_read_pointer);
  //increment cur position for ob
  theCoda.txmem[offset+1]++;
  return theCoda.txmem[target];
}


void coda_txbegin()
{
  //uint64_t ret = coda_stack_switch();  
  __asm__(
      "mov %%rax,%0\n\t"
      "mov %%rbx,%1\n\t"
      "mov %%rcx,%2\n\t"
      "mov %%rdx,%3\n\t"
      "mov %%rdi,%4\n\t"
      "mov %%rsi,%5\n\t"
      "mov %%r8,%6\n\t"
      "mov %%r9,%7\n\t"
      "mov %%r10,%8\n\t"
      "mov %%r11,%9\n\t"
      "mov %%r12,%10\n\t"
      "mov %%r13,%11\n\t"
      "mov %%r15,%12\n\t"
      "mov %13,%%r15\n\t"
      "lea (%%rip),%%r14\n\t"
      : "=r"(coda_context[0]), "=r"(coda_context[1]), "=r"(coda_context[2]),
      "=r"(coda_context[3]), "=r"(coda_context[4]), "=r"(coda_context[5]),
      "=r"(coda_context[6]), "=r"(coda_context[7]), "=r"(coda_context[8]),
      "=r"(coda_context[9]), "=r"(coda_context[10]), "=r"(coda_context[11]),
      "=r"(coda_context[12])
        : "r"(&coda_context[0])
               :);

  __asm__ ("mov %0,%%rdi\n\t"
      :
      :"r"(theCoda.txmem)
      :"%rdi");
  __asm__(
      "mov $0, %%eax\n\t"
      "mov %%rdi, %%rcx\n\t"
      "loop_ep_%=:\n\t"
      "cmpl  $7690,%%eax\n\t"
      "jge    endloop_ep_%=\n\t"
      "movl   (%%rcx),%%r11d\n\t"
      "addl   $1, %%eax\n\t"
      "add    $4, %%rcx\n\t"
      "jmp    loop_ep_%=\n\t"
      "endloop_ep_%=:\n\t"
      // "mov $0, %%eax\n\t"
      // "mov %%rdi, %%rcx\n\t"
      // "add $1048320, %%rcx\n\t"
      // "loop_ep1_%=:\n\t"
      //  "cmpl  $256,%%eax\n\t"
      //  "jge    endloop_ep1_%=\n\t"
      //  "movl   (%%rcx),%%r11d\n\t"
      //  "addl   $1, %%eax\n\t"
      //  "add    $4, %%rcx\n\t"
      //  "jmp    loop_ep1_%=\n\t"
      //  "endloop_ep1_%=:\n\t"
          //  "xbegin coda_abort_handler\n\t"
    "mov $0, %%eax\n\t"
    "mov %%rdi, %%rcx\n\t"
    "loop_ip_%=:\n\t"
    "cmpl  $7690,%%eax\n\t"
    "jge    endloop_ip_%=\n\t"
    "movl   (%%rcx),%%r11d\n\t"
    "addl   $1, %%eax\n\t"
    "add    $4, %%rcx\n\t"
    "jmp    loop_ip_%=\n\t"
    "endloop_ip_%=:\n\t"
    //"mov $0, %%eax\n\t"
    // "mov %%rdi, %%rcx\n\t"
    // "add $1048320, %%rcx\n\t"
    // "loop_ip1_%=:\n\t"
    // "cmpl  $256,%%eax\n\t"
    // "jge    endloop_ip1_%=\n\t"
    // "movl   (%%rcx),%%r11d\n\t"
    // "addl   $1, %%eax\n\t"
    //  "add    $4, %%rcx\n\t"
    //  "jmp    loop_ip1_%=\n\t"
    //  "endloop_ip1_%=:\n\t"
    :::);
}

extern "C" {
  void coda_tx_abort(int code) { coda_aborts++; }
}
