#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#define CHUNKS_TOTAL (((((((((((((((((8 * 8) * 16) * CHUNK_NUM) + (((8 * 8) * 10) * CHUNK_NUM)) + (((8 * 8) * 10) * CHUNK_NUM)) + (((8 * 8) * 10) * CHUNK_NUM)) + (((4 * 4) * 16) * CHUNK_NUM)) + (((4 * 4) * 10) * CHUNK_NUM)) + (((4 * 4) * 10) * CHUNK_NUM)) + (((4 * 4) * 10) * CHUNK_NUM)) + (((2 * 2) * 16) * CHUNK_NUM)) + (((2 * 2) * 10) * CHUNK_NUM)) + (((2 * 2) * 10) * CHUNK_NUM)) + (((2 * 2) * 10) * CHUNK_NUM)) + (256 * CHUNK_NUM)) + (128 * CHUNK_NUM)) + CHUNK_NUM)

#define LOOKUP_SIZE (1 << 15)

#define WITNESS_SIZE (279024 + 1)

typedef struct RoundOneOut {
  uint64_t *witness_digits;
  uint32_t *chunks;
  uint32_t *frequencies;
  uint32_t chunks_total;
  uint32_t lookup_size;
} RoundOneOut;

#ifdef __cplusplus
extern "C" {
#endif

struct RoundOneOut *round1(const uint8_t *input_ptr, uintptr_t input_len);

uint64_t *round2(struct RoundOneOut *round1_out, uint64_t *rand_digits);
uint64_t *round2_new( uint64_t *wtns_digits, uint64_t *rand_digits, uint64_t *inv1_digits, uint64_t *inv2_digits, uint64_t *prod_digits);

void witness_from_digits(uint64_t *witness);

void write_public_inputs(uint64_t *wtns_ptr, uintptr_t public_size);

void free_witness(uint64_t *ptr);

#ifdef __cplusplus
}
#endif
