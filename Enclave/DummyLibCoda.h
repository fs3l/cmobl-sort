#ifndef _DUMMY_CODA_H
#define _DUMMY_CODA_H

#include "./BetterLibCoda.h"

class DummyCoda;

class DummyNobArray : public NobArray
{
public:
  DummyNobArray() { handle = 0; }
  ~DummyNobArray() {}
  size_t handle;
  DummyCoda* cur;
  size_t size() const { return 0; }
  int32_t read_at(size_t pos) const;
  void write_at(size_t pos, int32_t value);
};

class DummyObIterator : public ObIterator
{
public:
  DummyObIterator() { handle = 0; }
  ~DummyObIterator() {}
  size_t handle;
  DummyCoda* cur;
  size_t size() const { return 0; }
  int32_t read_next();
  void write_next(int32_t value);
};

class DummyCoda : public Coda
{
private:
  int32_t nob_mem[10000] __attribute__((aligned(64)));
  int32_t ob_mem[10000] __attribute__((aligned(64)));
  size_t ob_global;
  size_t nob_global;
  size_t ob_start[100];
  size_t nob_start[100];
  size_t ob_pos[100];
  size_t ob_handle;
  size_t nob_handle;

  friend class DummyNobArray;
  friend class DummyObIterator;

public:
  DummyCoda()
  {
    ob_global = 0;
    nob_global = 0;
    ob_handle = 0;
    nob_handle = 0;
  }
  ~DummyCoda() {}
  void tx_begin() {}
  void tx_end() {}
  NobArray* make_nob_array(int32_t* data, size_t len);
  ObIterator* make_ob_iterator(int32_t* data, size_t len);
};

#endif
