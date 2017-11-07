#ifndef _MELSHUFFLE_H
#define _MELSHUFFLE_H

#ifdef __cplusplus
extern "C" {
#endif

int melshuffle(long M_data_ref, long M_perm_ref, long M_output_ref, int c_size,
               int input_size);

#ifdef __cplusplus
}
#endif

#endif
