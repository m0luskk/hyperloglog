#include <tgmath.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "qoption.h"
#include "murmur.h"

typedef struct {
  const size_t _initial_bits;
  const size_t _register_count; // == 2^_initial_bits
  uint8_t* const _registers;
} hll;

typedef uint64_t hash_t;

DECLARE_OPTION(hash_t)
DECLARE_OPTION(hll)


static inline struct option_hash_t hash(size_t size, const char str[static size]) {
    if (str == nullptr || str[0] == '\0') return option_hash_t_none();

    hash_t hash[2] = {};
    MurmurHash3_x86_128(str, size, 42, hash);

    return option_hash_t_value(hash[0]);
}

static inline struct option_hll hyperloglog_create(size_t initial_bits) {
  if (initial_bits >= sizeof(hash_t) * 8 || initial_bits == 0) return option_hll_none();

  size_t regs_count = (size_t)pow(2, initial_bits);

  uint8_t* regs = calloc(regs_count, sizeof(uint8_t));
  if (regs == nullptr) return option_hll_none(); 

  return option_hll_value((hll) {
    ._initial_bits = initial_bits,
    ._register_count = regs_count,
    ._registers = regs,
  });
}

static inline void hyperloglog_destroy(hll* hll) {
  free(hll->_registers);
}

static inline void hyperloglog_add_element(hll* hll, size_t value_size, const void* value) {
  auto hash_opt  = hash(value_size, value);
  if (!hash_opt.has_value) return;
  hash_t hash = option_hash_t_unwrap(hash_opt);
  
  hash_t reg_idx = hash >> (sizeof(hash_t) * 8 - hll->_initial_bits);

  hash_t shifted = hash << hll->_initial_bits;
  hash_t leftmost_pos = 0;
  if (shifted == 0) {
      leftmost_pos = (sizeof(hash_t) * 8 - hll->_initial_bits) + 1;
  } else {
      leftmost_pos = __builtin_clz(shifted) + 1;
  }

  if (leftmost_pos > hll->_registers[reg_idx]) {
    hll->_registers[reg_idx] = (uint8_t)leftmost_pos;
  }
}

static inline double hyperloglog_cardinality(hll* hll) {
    size_t m = hll->_register_count;
    double alpha;
    
    if (m == 16) alpha = 0.673;
    else if (m == 32) alpha = 0.697;
    else if (m == 64) alpha = 0.709;
    else alpha = 0.7213 / (1.0 + 1.079 / m);

    double sum = 0.0;
    for (size_t j = 0; j < m; j++) {
        sum += pow(2.0, -((double)hll->_registers[j]));
    }
    
    double estimate = alpha * m * m / sum;

    if (estimate <= 2.5 * m) {
        size_t zeros = 0;
        for (size_t j = 0; j < m; j++) {
            if (hll->_registers[j] == 0) zeros++;
        }
        if (zeros > 0) {
            estimate = m * log((double)m / zeros);
        }
    }
    
    return estimate;
}
