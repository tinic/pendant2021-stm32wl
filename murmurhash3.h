#ifndef MURMURHASH3_H_
#define MURMURHASH3_H_

#include <stdint.h>

void MurmurHash3_128 ( const void * key, const int len,
                           uint32_t seed, void * out );

#endif  // #ifndef MURMURHASH3_H_