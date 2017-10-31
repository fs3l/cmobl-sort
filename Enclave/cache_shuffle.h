#ifndef CACHE_SHUFFLE_H
#define CACHE_SHUFFLE_H

#ifdef __cplusplus
extern "C" {
#endif

void cache_shuffle(const int32_t* arr_in, const int32_t* perm_in,
                   int32_t* arr_out, int32_t len);
void cache_shuffle_test();

#ifdef __cplusplus
}
#endif

#endif
