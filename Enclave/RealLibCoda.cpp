#include "./RealLibCoda.h"

//coda is a singleton 
struct RealCoda theCoda;
unsigned long old_rsp;
int coda_aborts = 0;
void increment_meta(INDEX* meta) {
  if ((*meta%1024)==15)
    *meta =  *meta+1009; //1024-15
  else *meta =  *meta+1;
}


INDEX cal_ob(INDEX offset) {
  return (offset/16)*1024 + offset%16+16;
}

INDEX cal_nob(INDEX offset) {
  return (offset/528)*1024 + offset%528 + 80;
}

HANDLE declare_ob_iterator(DATA* data, LENGTH len) {
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

HANDLE declare_nob_array(DATA* data, LENGTH len) {
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

void ob_write_next(HANDLE h, DATA d) {
  //first, find the meta data slot
  INDEX offset = ((h*3)/16)*1024 + (h*3)%16;
  INDEX ob_start = theCoda.txmem[offset];
  INDEX ob_len = theCoda.txmem[offset+1];
  INDEX ob_cur = theCoda.txmem[offset+2];
  //calculate the target address
  INDEX target = cal_ob(ob_start+ob_cur);
  //increment cur position for ob
  theCoda.txmem[offset+2]++;
  theCoda.txmem[target] = d;
}

extern "C" {
void coda_tx_abort(int code) { coda_aborts++; }
}
