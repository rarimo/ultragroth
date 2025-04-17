#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct RoundOneOut {
  uint64_t *witness_digits;
  uint32_t *chunks;
  uint32_t *m;
  uint32_t *pushed_size;
} RoundOneOut;

#ifdef __cplusplus
extern "C" {
#endif

struct RoundOneOut *round1(const uint8_t *input_ptr, uintptr_t input_len);

uint64_t *round2(struct RoundOneOut *round1_out, uint64_t *rand_digits);

void bts(uint64_t *witness);

void free_witness(uint64_t *ptr);

void free_RoundOneOut(struct RoundOneOut *ptr);

#ifdef __cplusplus
}
#endif
