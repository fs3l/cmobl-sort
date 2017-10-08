#ifndef _LIB_CODA_H
#define _LIB_CODA_H

#include "./Enclave.h"
#include "./Enclave_t.h"

class ObIterator
{
public:
  virtual ~ObIterator() = 0;
  virtual size_t size() const = 0;
  virtual int32_t read_next() = 0;
  virtual void write_next(int32_t value) = 0;
};

class NobArray
{
public:
  virtual ~NobArray() = 0;
  virtual size_t size() const = 0;
  virtual int32_t read_at(size_t pos) const = 0;
  virtual void write_at(size_t pos, int32_t value) = 0;
};

class Coda
{
public:
  virtual ~Coda() = 0;
  virtual void tx_begin() = 0;
  virtual void tx_end() = 0;
  virtual ObIterator* make_ob_iterator(int32_t* data, size_t len) = 0;
  virtual NobArray* make_nob_array(int32_t* data, size_t len) = 0;
};

#endif
