#ifndef MULTIQUEUE_H
#define MULTIQUEUE_H

#include <stdlib.h>
#include <string.h>

#include "./Enclave.h"
#include "./RealLibCoda.h"
#include "./utility.h"

/**
 * MultiQueue stores each queue (including a free block queue) as a double
 * linked list.
 *
 * data layout:
 *   global meta data:
 *     data array length       [32 bits]
 *     number of queues        [16 bits]
 *     max capacity            [16 bits]
 *   queue meta data: (if begin/end address == 0, it means empty list)
 *     free block: (queue_id = -1)
 *       begin address         [16 bits]
 *       end address           [16 bits]
 *     queue 1:    (queue_id = 0)
 *       begin address         [16 bits]
 *       end address           [16 bits]
 *     ...
 *     queue n:    (queue_id = n - 1)
 *       begin address         [16 bits]
 *       end address           [16 bits]
 *   block data:
 *     ...
 *     block meta data: (address == 0 means NULL, i.e. first or last element)
 *       address to prev block [16 bits]
 *       address to next block [16 bits]
 *     payload                 [sizeof(element) aligned with 32bits]
 *     ...
 */

template <class T>
class MultiQueue
{
public:
  MultiQueue(int32_t capacity, int32_t num_of_queue)
  {
    const int32_t max_uint16 = 65536;
    const int32_t element_bytes = sizeof(T);
    const int32_t element_block_bytes =
        (element_bytes % sizeof(int32_t) == 0)
            ? element_bytes
            : element_bytes - element_bytes % sizeof(int32_t) + sizeof(int32_t);
    const int32_t element_size = element_bytes / sizeof(int32_t);
    const int32_t element_block_size = element_block_bytes / sizeof(int32_t);

    if (capacity > max_uint16 || num_of_queue > max_uint16)
      Eabort("fail to init MultiQueue");

    int32_t data_len = 2;                             // global meta data
    data_len += 1 + num_of_queue;                     // queue meta data;
    data_len += capacity * (1 + element_block_size);  // block data;

    if (data_len > max_uint16) Eabort("fail to init MultiQueue");
    data = new int32_t[data_len];

    // set global meta data
    data[0] = data_len;
    data[1] = (num_of_queue << 16) | capacity;
    // set queue meta data
    data[2] = ((3 + num_of_queue) << 16) | (data_len - 1 - element_block_size);
    for (int32_t i = 0; i < num_of_queue; ++i) data[3 + i] = 0;
    // set block data
    for (int32_t i = 0, block_addr = 3 + num_of_queue; i < capacity; ++i) {
      int32_t prev_addr = block_addr - (1 + element_block_size);
      int32_t next_addr = block_addr + (1 + element_block_size);
      if (i == 0)
        prev_addr = 0;
      else if (i == capacity - 1)
        next_addr = 0;
      data[block_addr] = (prev_addr << 16) | next_addr;
      block_addr = next_addr;
    }
  }
  ~MultiQueue() { delete[] data; }
  void init_nob() { nob = initialize_nob_array(data, data[0]); }
  void reset_nob()
  {
    for (int32_t i = 1; i < data[0]; ++i) data[i] = nob_read_at(nob, i);
  }

  // return the size of the queue queue_id
  __attribute__((always_inline)) inline int32_t size(int32_t queue_id) const
  {
    int32_t addr = get_queue_begin_addr(queue_id);
    int32_t size = 0;
    while (addr != 0) {
      ++size;
      addr = get_block_next_addr(addr);
    }
    return size;
  }

  // return true if the queue queue_id is empty
  __attribute__((always_inline)) inline bool empty(int32_t queue_id) const
  {
    return get_queue_begin_addr(queue_id) == 0;
  }

  // return true if the entire MultiQueue is full, i.e., no more free block
  __attribute__((always_inline)) inline bool full() const { return empty(-1); }
  // return the first element in the queue queue_id
  __attribute__((always_inline)) inline void front(int32_t queue_id,
                                                   T *element) const
  {
    int32_t addr = get_queue_begin_addr(queue_id);
    read_element(addr + 1, element);
  }

  // return the last element in the queue queue_id
  __attribute__((always_inline)) inline void back(int32_t queue_id,
                                                  T *element) const
  {
    int32_t addr = get_queue_end_addr(queue_id);
    read_element(addr + 1, element);
  }

  // push an element in the front of the queue queue_id
  __attribute__((always_inline)) inline void push_front(int32_t queue_id,
                                                        T element)
  {
    int32_t addr = alloc_block();
    write_element(addr + 1, element);
    int32_t begin_addr = get_queue_begin_addr(queue_id);
    if (begin_addr == 0) {  // was empty queue
      set_block_addr(addr, 0, 0);
      set_queue_addr(queue_id, addr, addr);
    } else {
      set_block_prev_addr(begin_addr, addr);
      set_block_addr(addr, 0, begin_addr);
      set_queue_begin_addr(queue_id, addr);
    }
  }

  // push an element in the back of the queue queue_id
  __attribute__((always_inline)) inline void push_back(int32_t queue_id,
                                                       T element)
  {
    int32_t addr = alloc_block();
    write_element(addr + 1, element);
    int32_t end_addr = get_queue_end_addr(queue_id);
    if (end_addr == 0) {  // was empty queue
      set_block_addr(addr, 0, 0);
      set_queue_addr(queue_id, addr, addr);
    } else {
      set_block_next_addr(end_addr, addr);
      set_block_addr(addr, end_addr, 0);
      set_queue_end_addr(queue_id, addr);
    }
  }

  // pop the element in the front of the queue queue_id
  __attribute__((always_inline)) inline void pop_front(int32_t queue_id)
  {
    int32_t addr = get_queue_begin_addr(queue_id);
    int32_t next_addr = get_block_next_addr(addr);
    if (next_addr == 0) {  // now empty queue
      set_queue_addr(queue_id, 0, 0);
    } else {
      set_block_prev_addr(next_addr, 0);
      set_queue_begin_addr(queue_id, next_addr);
    }
    free_block(addr);
  }

  // pop the element in the back of the queue queue_id
  __attribute__((always_inline)) inline void pop_back(int32_t queue_id)
  {
    int32_t addr = get_queue_end_addr(queue_id);
    int32_t prev_addr = get_block_prev_addr(addr);
    if (prev_addr == 0) {  // now empty queue
      set_queue_addr(queue_id, 0, 0);
    } else {
      set_block_next_addr(prev_addr, 0);
      set_queue_end_addr(queue_id, prev_addr);
    }
    free_block(addr);
  }

private:
  __attribute__((always_inline)) inline int32_t get_queue_begin_addr(
      int32_t queue_id) const
  {
    return read(3 + queue_id) >> 16;
  }
  __attribute__((always_inline)) inline int32_t get_queue_end_addr(
      int32_t queue_id) const
  {
    return read(3 + queue_id) & 0xffff;
  }
  __attribute__((always_inline)) inline void set_queue_addr(int32_t queue_id,
                                                            int32_t begin_addr,
                                                            int32_t end_addr)
  {
    write(3 + queue_id, (begin_addr << 16) | end_addr);
  }
  __attribute__((always_inline)) inline void set_queue_begin_addr(
      int32_t queue_id, int32_t begin_addr)
  {
    set_queue_addr(queue_id, begin_addr, get_queue_end_addr(queue_id));
  }
  __attribute__((always_inline)) inline void set_queue_end_addr(
      int32_t queue_id, int32_t end_addr)
  {
    set_queue_addr(queue_id, get_queue_begin_addr(queue_id), end_addr);
  }
  __attribute__((always_inline)) inline int32_t get_block_prev_addr(
      int32_t block_addr) const
  {
    return read(block_addr) >> 16;
  }
  __attribute__((always_inline)) inline int32_t get_block_next_addr(
      int32_t block_addr) const
  {
    return read(block_addr) & 0xffff;
  }
  __attribute__((always_inline)) inline void set_block_addr(int32_t block_addr,
                                                            int32_t prev_addr,
                                                            int32_t next_addr)
  {
    write(block_addr, (prev_addr << 16) | next_addr);
  }
  __attribute__((always_inline)) inline void set_block_prev_addr(
      int32_t block_addr, int32_t prev_addr)
  {
    set_block_addr(block_addr, prev_addr, get_block_next_addr(block_addr));
  }
  __attribute__((always_inline)) inline void set_block_next_addr(
      int32_t block_addr, int32_t next_addr)
  {
    set_block_addr(block_addr, get_block_prev_addr(block_addr), next_addr);
  }
  __attribute__((always_inline)) inline void read_element(int32_t addr,
                                                          T *element) const
  {
    for (int32_t i = 0; i < sizeof(T) / sizeof(int32_t) /* element_size */;
         ++i) {
      *(int32_t *)((unsigned char *)(element) + i * sizeof(int32_t)) =
          read(addr + i);
    }
  }
  __attribute__((always_inline)) inline void write_element(int32_t addr,
                                                           T element)
  {
    for (int32_t i = 0; i < sizeof(T) / sizeof(int32_t) /* element_size */;
         ++i) {
      write(addr + i,
            *(int32_t *)((unsigned char *)(&element) + i * sizeof(int32_t)));
    }
  }
  __attribute__((always_inline)) inline int32_t alloc_block()
  {
    int32_t free_addr = get_queue_begin_addr(-1);
    int32_t next_addr = get_block_next_addr(free_addr);
    if (next_addr == 0) {  // no more free block
      set_queue_addr(-1, 0, 0);
    } else {
      set_block_prev_addr(next_addr, 0);
      set_queue_begin_addr(-1, next_addr);
    }
    return free_addr;
  }
  __attribute__((always_inline)) inline void free_block(int32_t addr)
  {
    int32_t begin_addr = get_queue_begin_addr(-1);
    if (begin_addr == 0) {  // was full
      set_block_addr(addr, 0, 0);
      set_queue_addr(-1, addr, addr);
    } else {
      set_block_prev_addr(begin_addr, addr);
      set_block_addr(addr, 0, begin_addr);
      set_queue_begin_addr(-1, addr);
    }
  }

protected:
  int32_t read(int32_t addr) const { return nob_read_at(nob, addr); }
  void write(int32_t addr, int32_t val) { nob_write_at(nob, addr, val); }
  int32_t *data;
  HANDLE nob;
};

#endif
