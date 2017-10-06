#include "./LibCoda.h"

int g_aborts = 0;
int g_starts = 0;
int g_finishes = 0;

int32_t cache_size;

void tx_abort(int code) { g_aborts++; }

