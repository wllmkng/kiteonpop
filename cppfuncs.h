#ifndef _CPPFUNCS_H_
#define _CPPFUNCS_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

uint8_t calc_hash_cpp(uint8_t *value, uint64_t len);

#ifdef __cplusplus
}
#endif

#endif