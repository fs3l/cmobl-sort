#ifndef _REAL_CODA_H
#define _REAL_CODA_H
#include <string.h>
#include "./BetterLibCoda.h"

class RealCoda;

class RealNobArray : public NobArray
{
public:
  RealNobArray() { handle = 0; }
  ~RealNobArray();
  size_t size() const { return len; }
  int32_t read_at(size_t pos) const;
  void write_at(size_t pos, int32_t value);

  size_t handle;
  RealCoda* cur;
  int32_t* data;
  size_t len;
};

class RealObIterator : public ObIterator
{
public:
  RealObIterator() { handle = 0; }
  ~RealObIterator();
  size_t size() const { return len; }
  int32_t read_next();
  void write_next(int32_t value);

  size_t handle;
  RealCoda* cur;
  int32_t* data;
  size_t len;
};

class RealCoda : public Coda
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

  friend class RealNobArray;
  friend class RealObIterator;

public:
  RealCoda()
  {
    ob_global = 0;
    nob_global = 0;
    ob_handle = 0;
    nob_handle = 0;
    memset(ob_pos,0,sizeof(size_t)*100);
    memset(ob_start,0,sizeof(size_t)*100);
    memset(nob_start,0,sizeof(size_t)*100);
  }
  ~RealCoda() {}
  void tx_begin();
  void tx_end() {}
  NobArray* make_nob_array(int32_t* data, size_t len);
  ObIterator* make_ob_iterator(int32_t* data, size_t len);
};

#endif
