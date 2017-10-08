#ifndef _LIB_CODA_H
#define _LIB_CODA_H

#include "./Enclave.h"

class ObIterator
{
public:
  size_t size() const;
  int32_t read_next(int32_t* value);
  int32_t write_next(int32_t value);
};

class NobArray
{
public:
  size_t size() const;
  int32_t read_at(size_t pos, int32_t* value);
  int32_t write_at(size_t pos, int32_t value);
};

class Coda
{
public:
  void tx_begin();
  void tx_end();
  ObIterator make_ob_iterator(int32_t* data, size_t len);
  NobArray make_nob_array(int32_t* data, size_t len);
};

#endif
