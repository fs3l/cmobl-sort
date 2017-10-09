#include "./RealLibCoda.h"

ObIterator* RealCoda::make_ob_iterator(int32_t* data, size_t len) {
  RealObIterator* res = new RealObIterator();
  for (int i=0; i< len; i++)
    ob_mem[ob_global+i] = data[i];
  ob_handle++;
  ob_start[ob_handle] = ob_global;
  ob_global+=len;
  res->handle = ob_handle;
  res->cur = this;
  res->data = data;
  res->len = len;
  return res;
}


NobArray* RealCoda::make_nob_array(int32_t* data, size_t len) {
  RealNobArray* res = new RealNobArray();
  return res;
}

RealNobArray::~RealNobArray() {
  for (int i=0;i<len;i++) {
    data[i] = cur->nob_mem[cur->nob_start[handle]+i];
  }
}

RealObIterator::~RealObIterator() {
  for (int i=0;i<len;i++) {
    data[i] = cur->ob_mem[cur->ob_start[handle]+i];
  }
}

int32_t RealNobArray::read_at(size_t pos) const {
  return cur->nob_mem[cur->nob_start[handle]+pos];
}

void RealNobArray::write_at(size_t pos, int32_t value) {
  cur->nob_mem[cur->nob_start[handle]+pos] = value;
}

int32_t RealObIterator::read_next() {
  int32_t res =  cur->ob_mem[cur->ob_start[handle]+cur->ob_pos[handle]];
  cur->ob_pos[handle]++;
  return res;
}

void RealObIterator::write_next(int32_t value) {
  cur->ob_mem[cur->ob_start[handle]+cur->ob_pos[handle]]=value;
  cur->ob_pos[handle]++;
}
