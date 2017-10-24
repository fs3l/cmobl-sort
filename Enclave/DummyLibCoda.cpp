#include "./DummyLibCoda.h"

ObIterator* DummyCoda::make_ob_iterator(int32_t* data, size_t len) {
  DummyObIterator* res = new DummyObIterator();
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

NobArray* DummyCoda::make_nob_array(int32_t* data, size_t len) {
  DummyNobArray* res = new DummyNobArray();
  for (int i=0; i< len; i++)
    nob_mem[nob_global+i] = data[i];
  nob_handle++;
  nob_start[nob_handle] = nob_global;
  nob_global+=len;
  res->handle = nob_handle;
  res->cur = this;
  res->data = data;
  res->len = len;
  return res;
}

DummyNobArray::~DummyNobArray() {
  for (int i=0;i<len;i++) {
    data[i] = cur->nob_mem[cur->nob_start[handle]+i];
  }
}

DummyObIterator::~DummyObIterator() {
  for (int i=0;i<len;i++) {
    data[i] = cur->ob_mem[cur->ob_start[handle]+i];
  }
}

int32_t DummyNobArray::read_at(size_t pos) const {
  EPrintf("in %s and pos=%d\n",__func__,pos);
  return cur->nob_mem[cur->nob_start[handle]+pos];
}

void DummyNobArray::write_at(size_t pos, int32_t value) {
  EPrintf("in %s and pos=%d,value=%d\n",__func__,pos,value);
  cur->nob_mem[cur->nob_start[handle]+pos] = value;
}

int32_t DummyObIterator::read_next() {
  EPrintf("in %s\n",__func__);
  int32_t res =  cur->ob_mem[cur->ob_start[handle]+cur->ob_pos[handle]];
  cur->ob_pos[handle]++;
  return res;
}

void DummyObIterator::write_next(int32_t value) {
  EPrintf("in %s and value=%d\n",__func__,value);
  cur->ob_mem[cur->ob_start[handle]+cur->ob_pos[handle]]=value;
  cur->ob_pos[handle]++;
}
