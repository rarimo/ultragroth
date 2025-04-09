#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#define CHUNKS_TOTAL (8 * 8 * 10 * 25)
#define WITNESS_SIZE 69761
#define LOOKUP_SIZE (1 << 10)


typedef struct RoundOneOut {
  uint64_t *witness_digits;
  uint32_t *chunks;
  uint32_t *m;
} RoundOneOut;

#ifdef __cplusplus
extern "C" {
#endif

struct RoundOneOut *round1(const uint8_t *input_file, const uint8_t *sym_file);
uint64_t *round2(struct RoundOneOut *round1_out, uint64_t *rand_digits, const uint8_t *sym_file);
void bts(uint64_t *witness);
void free_vec(uint64_t *ptr, uintptr_t size);

#ifdef __cplusplus
}
#endif
