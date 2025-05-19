#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct RoundOneOut {
  uint64_t *witness_digits;
  uint32_t *chunks;
  uint32_t *frequencies;
  uint32_t chunks_total;
  uint32_t lookup_size;
  uint8_t *error;
  uint32_t error_size;
} RoundOneOut;

typedef struct RoundTwoOut {
  uint64_t *witness_digits;
  uint8_t *error;
  uint32_t error_size;
} RoundTwoOut;

#ifdef __cplusplus
extern "C" {
#endif

struct RoundOneOut round1(const uint8_t *input_ptr, uintptr_t input_len);

RoundTwoOut round2( uint64_t *wtns_digits, uint64_t *rand_digits, uint64_t *inv1_digits, uint64_t *inv2_digits, uint64_t *prod_digits);

uint64_t *round2_debug(RoundOneOut out, uint64_t *rand_digits);

void witness_from_digits(uint64_t *witness, uint32_t size);

void write_public_inputs(uint64_t *wtns_ptr, uintptr_t public_size);

void free_witness(uint64_t *ptr);

#ifdef __cplusplus
}
#endif
