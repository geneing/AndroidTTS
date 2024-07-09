// Copyright 2005-2024 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the 'License');
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an 'AS IS' BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.

#include <fst/extensions/ngram/nthbit.h>

#include <cstdint>


namespace fst {

#if !defined(__BMI2__)  // BMI2 has everything in the header
#if SIZE_MAX == UINT32_MAX

// 32-bit platforms will be slow when using 64-bit operations; use this
// table-based version instead. This only contains constant shifts, which
// have been benchmarked to be fast.

// These tables were generated using:
//
//  uint32_t nth_bit_scan(uint64_t v, uint32_t r) {
//    for (int i = 0; i < 64; ++i) {
//      if ((r -= v & 1) == 0) return i;
//      v >>= 1;
//    }
//    return -1;
//  }
//
//  printf("static const uint8_t nth_bit_bit_count[256] = {\n");
//  for (size_t i = 0; i < 256; ++i) {
//    printf("%d, ", __builtin_popcount(i));
//    if (i % 16 == 15) printf("\n");
//  }
//  printf("};\n");
//
//  printf("static const uint8_t nth_bit_bit_pos[8][256] = {{\n");
//  for (size_t j = 0; j < 8; ++j) {
//    for (size_t i = 0; i < 256; ++i) {
//      uint8_t pos = nth_bit_scan(i, j);
//      printf("%d, ", pos);
//      if (i % 16 == 15) printf("\n");
//    }
//    if (j != 7) printf("}, {\n");
//  }
//  printf("}};\n");
//
// This table contains the popcount of 1-byte values:
// nth_bit_bit_count[v] == __builtin_popcount(v).
static const uint8_t nth_bit_bit_count[256] = {
    0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4, 1, 2, 2, 3, 2, 3, 3, 4,
    2, 3, 3, 4, 3, 4, 4, 5, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 1, 2, 2, 3, 2, 3, 3, 4,
    2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6,
    4, 5, 5, 6, 5, 6, 6, 7, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 2, 3, 3, 4, 3, 4, 4, 5,
    3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6,
    4, 5, 5, 6, 5, 6, 6, 7, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8,
};

// This table contains the bit position of the r-th set bit in v, for 1-byte v,
// (or 255 if there are fewer than r bits set, but those values are never used):
// nth_bit_bit_pos[r][v] == nth_bit_scan(v, r).
static const uint8_t nth_bit_bit_pos[8][256] = {
    {
        255, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0,
        1,   0, 3, 0, 1, 0, 2, 0, 1, 0, 5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0,
        2,   0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 6, 0,
        1,   0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0,
        3,   0, 1, 0, 2, 0, 1, 0, 5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0,
        1,   0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 7, 0, 1, 0,
        2,   0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0,
        1,   0, 2, 0, 1, 0, 5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        4,   0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 6, 0, 1, 0, 2, 0,
        1,   0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0,
        2,   0, 1, 0, 5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0,
        1,   0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    },
    {
        255, 255, 255, 1, 255, 2, 2, 1, 255, 3, 3,   1, 3, 2, 2,   1, 255, 4,
        4,   1,   4,   2, 2,   1, 4, 3, 3,   1, 3,   2, 2, 1, 255, 5, 5,   1,
        5,   2,   2,   1, 5,   3, 3, 1, 3,   2, 2,   1, 5, 4, 4,   1, 4,   2,
        2,   1,   4,   3, 3,   1, 3, 2, 2,   1, 255, 6, 6, 1, 6,   2, 2,   1,
        6,   3,   3,   1, 3,   2, 2, 1, 6,   4, 4,   1, 4, 2, 2,   1, 4,   3,
        3,   1,   3,   2, 2,   1, 6, 5, 5,   1, 5,   2, 2, 1, 5,   3, 3,   1,
        3,   2,   2,   1, 5,   4, 4, 1, 4,   2, 2,   1, 4, 3, 3,   1, 3,   2,
        2,   1,   255, 7, 7,   1, 7, 2, 2,   1, 7,   3, 3, 1, 3,   2, 2,   1,
        7,   4,   4,   1, 4,   2, 2, 1, 4,   3, 3,   1, 3, 2, 2,   1, 7,   5,
        5,   1,   5,   2, 2,   1, 5, 3, 3,   1, 3,   2, 2, 1, 5,   4, 4,   1,
        4,   2,   2,   1, 4,   3, 3, 1, 3,   2, 2,   1, 7, 6, 6,   1, 6,   2,
        2,   1,   6,   3, 3,   1, 3, 2, 2,   1, 6,   4, 4, 1, 4,   2, 2,   1,
        4,   3,   3,   1, 3,   2, 2, 1, 6,   5, 5,   1, 5, 2, 2,   1, 5,   3,
        3,   1,   3,   2, 2,   1, 5, 4, 4,   1, 4,   2, 2, 1, 4,   3, 3,   1,
        3,   2,   2,   1,
    },
    {
        255, 255, 255, 255, 255, 255, 255, 2, 255, 255, 255, 3, 255, 3, 3, 2,
        255, 255, 255, 4,   255, 4,   4,   2, 255, 4,   4,   3, 4,   3, 3, 2,
        255, 255, 255, 5,   255, 5,   5,   2, 255, 5,   5,   3, 5,   3, 3, 2,
        255, 5,   5,   4,   5,   4,   4,   2, 5,   4,   4,   3, 4,   3, 3, 2,
        255, 255, 255, 6,   255, 6,   6,   2, 255, 6,   6,   3, 6,   3, 3, 2,
        255, 6,   6,   4,   6,   4,   4,   2, 6,   4,   4,   3, 4,   3, 3, 2,
        255, 6,   6,   5,   6,   5,   5,   2, 6,   5,   5,   3, 5,   3, 3, 2,
        6,   5,   5,   4,   5,   4,   4,   2, 5,   4,   4,   3, 4,   3, 3, 2,
        255, 255, 255, 7,   255, 7,   7,   2, 255, 7,   7,   3, 7,   3, 3, 2,
        255, 7,   7,   4,   7,   4,   4,   2, 7,   4,   4,   3, 4,   3, 3, 2,
        255, 7,   7,   5,   7,   5,   5,   2, 7,   5,   5,   3, 5,   3, 3, 2,
        7,   5,   5,   4,   5,   4,   4,   2, 5,   4,   4,   3, 4,   3, 3, 2,
        255, 7,   7,   6,   7,   6,   6,   2, 7,   6,   6,   3, 6,   3, 3, 2,
        7,   6,   6,   4,   6,   4,   4,   2, 6,   4,   4,   3, 4,   3, 3, 2,
        7,   6,   6,   5,   6,   5,   5,   2, 6,   5,   5,   3, 5,   3, 3, 2,
        6,   5,   5,   4,   5,   4,   4,   2, 5,   4,   4,   3, 4,   3, 3, 2,
    },
    {
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 3,   255, 255, 255, 255, 255, 255, 255, 4,   255, 255, 255, 4,
        255, 4,   4,   3,   255, 255, 255, 255, 255, 255, 255, 5,   255, 255,
        255, 5,   255, 5,   5,   3,   255, 255, 255, 5,   255, 5,   5,   4,
        255, 5,   5,   4,   5,   4,   4,   3,   255, 255, 255, 255, 255, 255,
        255, 6,   255, 255, 255, 6,   255, 6,   6,   3,   255, 255, 255, 6,
        255, 6,   6,   4,   255, 6,   6,   4,   6,   4,   4,   3,   255, 255,
        255, 6,   255, 6,   6,   5,   255, 6,   6,   5,   6,   5,   5,   3,
        255, 6,   6,   5,   6,   5,   5,   4,   6,   5,   5,   4,   5,   4,
        4,   3,   255, 255, 255, 255, 255, 255, 255, 7,   255, 255, 255, 7,
        255, 7,   7,   3,   255, 255, 255, 7,   255, 7,   7,   4,   255, 7,
        7,   4,   7,   4,   4,   3,   255, 255, 255, 7,   255, 7,   7,   5,
        255, 7,   7,   5,   7,   5,   5,   3,   255, 7,   7,   5,   7,   5,
        5,   4,   7,   5,   5,   4,   5,   4,   4,   3,   255, 255, 255, 7,
        255, 7,   7,   6,   255, 7,   7,   6,   7,   6,   6,   3,   255, 7,
        7,   6,   7,   6,   6,   4,   7,   6,   6,   4,   6,   4,   4,   3,
        255, 7,   7,   6,   7,   6,   6,   5,   7,   6,   6,   5,   6,   5,
        5,   3,   7,   6,   6,   5,   6,   5,   5,   4,   6,   5,   5,   4,
        5,   4,   4,   3,
    },
    {
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 4,   255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 5,   255, 255, 255, 255, 255, 255, 255, 5,
        255, 255, 255, 5,   255, 5,   5,   4,   255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 6,   255, 255, 255, 255,
        255, 255, 255, 6,   255, 255, 255, 6,   255, 6,   6,   4,   255, 255,
        255, 255, 255, 255, 255, 6,   255, 255, 255, 6,   255, 6,   6,   5,
        255, 255, 255, 6,   255, 6,   6,   5,   255, 6,   6,   5,   6,   5,
        5,   4,   255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 7,   255, 255, 255, 255, 255, 255, 255, 7,   255, 255,
        255, 7,   255, 7,   7,   4,   255, 255, 255, 255, 255, 255, 255, 7,
        255, 255, 255, 7,   255, 7,   7,   5,   255, 255, 255, 7,   255, 7,
        7,   5,   255, 7,   7,   5,   7,   5,   5,   4,   255, 255, 255, 255,
        255, 255, 255, 7,   255, 255, 255, 7,   255, 7,   7,   6,   255, 255,
        255, 7,   255, 7,   7,   6,   255, 7,   7,   6,   7,   6,   6,   4,
        255, 255, 255, 7,   255, 7,   7,   6,   255, 7,   7,   6,   7,   6,
        6,   5,   255, 7,   7,   6,   7,   6,   6,   5,   7,   6,   6,   5,
        6,   5,   5,   4,
    },
    {
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 5,   255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 6,   255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 6,
        255, 255, 255, 255, 255, 255, 255, 6,   255, 255, 255, 6,   255, 6,
        6,   5,   255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 7,   255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 7,   255, 255, 255, 255, 255, 255,
        255, 7,   255, 255, 255, 7,   255, 7,   7,   5,   255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 7,   255, 255,
        255, 255, 255, 255, 255, 7,   255, 255, 255, 7,   255, 7,   7,   6,
        255, 255, 255, 255, 255, 255, 255, 7,   255, 255, 255, 7,   255, 7,
        7,   6,   255, 255, 255, 7,   255, 7,   7,   6,   255, 7,   7,   6,
        7,   6,   6,   5,
    },
    {
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 6,   255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 7,   255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 7,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 7,   255, 255, 255, 255, 255, 255, 255, 7,   255, 255, 255, 7,
        255, 7,   7,   6,
    },
    {
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 7,
    }};

int nth_bit(const uint64_t v, uint32_t r) {
  DCHECK_NE(v, 0);
  DCHECK_LE(0, r);
  DCHECK_LT(r, __builtin_popcountll(v));

  uint32_t next_byte = v & 255;
  uint32_t byte_popcount = nth_bit_bit_count[next_byte];
  if (r < byte_popcount) return nth_bit_bit_pos[r][next_byte];
  r -= byte_popcount;
  next_byte = (v >> 8) & 255;
  byte_popcount = nth_bit_bit_count[next_byte];
  if (r < byte_popcount) return 8 + nth_bit_bit_pos[r][next_byte];
  r -= byte_popcount;
  next_byte = (v >> 16) & 255;
  byte_popcount = nth_bit_bit_count[next_byte];
  if (r < byte_popcount) return 16 + nth_bit_bit_pos[r][next_byte];
  r -= byte_popcount;
  next_byte = (v >> 24) & 255;
  byte_popcount = nth_bit_bit_count[next_byte];
  if (r < byte_popcount) return 24 + nth_bit_bit_pos[r][next_byte];
  r -= byte_popcount;
  next_byte = (v >> 32) & 255;
  byte_popcount = nth_bit_bit_count[next_byte];
  if (r < byte_popcount) return 32 + nth_bit_bit_pos[r][next_byte];
  r -= byte_popcount;
  next_byte = (v >> 40) & 255;
  byte_popcount = nth_bit_bit_count[next_byte];
  if (r < byte_popcount) return 40 + nth_bit_bit_pos[r][next_byte];
  r -= byte_popcount;
  next_byte = (v >> 48) & 255;
  byte_popcount = nth_bit_bit_count[next_byte];
  if (r < byte_popcount) return 48 + nth_bit_bit_pos[r][next_byte];
  r -= byte_popcount;
  next_byte = (v >> 56) & 255;
  byte_popcount = nth_bit_bit_count[next_byte];
  if (r < byte_popcount) return 56 + nth_bit_bit_pos[r][next_byte];
  return -1;
}

#elif SIZE_MAX == UINT64_MAX  // 64-bit, non-BMI2
// This table is generated using:
//
//  printf("const uint8_t kSelectInByte[8 * 256] = {\n");
//  for (int j = 0; j < 8; ++j) {
//    for (int i = 0; i < 256; ++i) {
//      if (i > 0) printf(" ");
//      if (i % 16 == 0) printf("\n  ");
//      const int k = findbitn(i, j);
//      printf("%d,", k == -1 ? 0 : k);
//    }
//    printf("\n");
//  }
//  printf("};\n");
//
namespace internal {

const uint8_t kSelectInByte[8 * 256] = {
  0, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
  4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
  5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
  4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
  6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
  4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
  5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
  4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
  7, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
  4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
  5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
  4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
  6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
  4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
  5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
  4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,

  0, 0, 0, 1, 0, 2, 2, 1, 0, 3, 3, 1, 3, 2, 2, 1,
  0, 4, 4, 1, 4, 2, 2, 1, 4, 3, 3, 1, 3, 2, 2, 1,
  0, 5, 5, 1, 5, 2, 2, 1, 5, 3, 3, 1, 3, 2, 2, 1,
  5, 4, 4, 1, 4, 2, 2, 1, 4, 3, 3, 1, 3, 2, 2, 1,
  0, 6, 6, 1, 6, 2, 2, 1, 6, 3, 3, 1, 3, 2, 2, 1,
  6, 4, 4, 1, 4, 2, 2, 1, 4, 3, 3, 1, 3, 2, 2, 1,
  6, 5, 5, 1, 5, 2, 2, 1, 5, 3, 3, 1, 3, 2, 2, 1,
  5, 4, 4, 1, 4, 2, 2, 1, 4, 3, 3, 1, 3, 2, 2, 1,
  0, 7, 7, 1, 7, 2, 2, 1, 7, 3, 3, 1, 3, 2, 2, 1,
  7, 4, 4, 1, 4, 2, 2, 1, 4, 3, 3, 1, 3, 2, 2, 1,
  7, 5, 5, 1, 5, 2, 2, 1, 5, 3, 3, 1, 3, 2, 2, 1,
  5, 4, 4, 1, 4, 2, 2, 1, 4, 3, 3, 1, 3, 2, 2, 1,
  7, 6, 6, 1, 6, 2, 2, 1, 6, 3, 3, 1, 3, 2, 2, 1,
  6, 4, 4, 1, 4, 2, 2, 1, 4, 3, 3, 1, 3, 2, 2, 1,
  6, 5, 5, 1, 5, 2, 2, 1, 5, 3, 3, 1, 3, 2, 2, 1,
  5, 4, 4, 1, 4, 2, 2, 1, 4, 3, 3, 1, 3, 2, 2, 1,

  0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 3, 0, 3, 3, 2,
  0, 0, 0, 4, 0, 4, 4, 2, 0, 4, 4, 3, 4, 3, 3, 2,
  0, 0, 0, 5, 0, 5, 5, 2, 0, 5, 5, 3, 5, 3, 3, 2,
  0, 5, 5, 4, 5, 4, 4, 2, 5, 4, 4, 3, 4, 3, 3, 2,
  0, 0, 0, 6, 0, 6, 6, 2, 0, 6, 6, 3, 6, 3, 3, 2,
  0, 6, 6, 4, 6, 4, 4, 2, 6, 4, 4, 3, 4, 3, 3, 2,
  0, 6, 6, 5, 6, 5, 5, 2, 6, 5, 5, 3, 5, 3, 3, 2,
  6, 5, 5, 4, 5, 4, 4, 2, 5, 4, 4, 3, 4, 3, 3, 2,
  0, 0, 0, 7, 0, 7, 7, 2, 0, 7, 7, 3, 7, 3, 3, 2,
  0, 7, 7, 4, 7, 4, 4, 2, 7, 4, 4, 3, 4, 3, 3, 2,
  0, 7, 7, 5, 7, 5, 5, 2, 7, 5, 5, 3, 5, 3, 3, 2,
  7, 5, 5, 4, 5, 4, 4, 2, 5, 4, 4, 3, 4, 3, 3, 2,
  0, 7, 7, 6, 7, 6, 6, 2, 7, 6, 6, 3, 6, 3, 3, 2,
  7, 6, 6, 4, 6, 4, 4, 2, 6, 4, 4, 3, 4, 3, 3, 2,
  7, 6, 6, 5, 6, 5, 5, 2, 6, 5, 5, 3, 5, 3, 3, 2,
  6, 5, 5, 4, 5, 4, 4, 2, 5, 4, 4, 3, 4, 3, 3, 2,

  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3,
  0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 4, 0, 4, 4, 3,
  0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 5, 0, 5, 5, 3,
  0, 0, 0, 5, 0, 5, 5, 4, 0, 5, 5, 4, 5, 4, 4, 3,
  0, 0, 0, 0, 0, 0, 0, 6, 0, 0, 0, 6, 0, 6, 6, 3,
  0, 0, 0, 6, 0, 6, 6, 4, 0, 6, 6, 4, 6, 4, 4, 3,
  0, 0, 0, 6, 0, 6, 6, 5, 0, 6, 6, 5, 6, 5, 5, 3,
  0, 6, 6, 5, 6, 5, 5, 4, 6, 5, 5, 4, 5, 4, 4, 3,
  0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 7, 0, 7, 7, 3,
  0, 0, 0, 7, 0, 7, 7, 4, 0, 7, 7, 4, 7, 4, 4, 3,
  0, 0, 0, 7, 0, 7, 7, 5, 0, 7, 7, 5, 7, 5, 5, 3,
  0, 7, 7, 5, 7, 5, 5, 4, 7, 5, 5, 4, 5, 4, 4, 3,
  0, 0, 0, 7, 0, 7, 7, 6, 0, 7, 7, 6, 7, 6, 6, 3,
  0, 7, 7, 6, 7, 6, 6, 4, 7, 6, 6, 4, 6, 4, 4, 3,
  0, 7, 7, 6, 7, 6, 6, 5, 7, 6, 6, 5, 6, 5, 5, 3,
  7, 6, 6, 5, 6, 5, 5, 4, 6, 5, 5, 4, 5, 4, 4, 3,

  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5,
  0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 5, 0, 5, 5, 4,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6,
  0, 0, 0, 0, 0, 0, 0, 6, 0, 0, 0, 6, 0, 6, 6, 4,
  0, 0, 0, 0, 0, 0, 0, 6, 0, 0, 0, 6, 0, 6, 6, 5,
  0, 0, 0, 6, 0, 6, 6, 5, 0, 6, 6, 5, 6, 5, 5, 4,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7,
  0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 7, 0, 7, 7, 4,
  0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 7, 0, 7, 7, 5,
  0, 0, 0, 7, 0, 7, 7, 5, 0, 7, 7, 5, 7, 5, 5, 4,
  0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 7, 0, 7, 7, 6,
  0, 0, 0, 7, 0, 7, 7, 6, 0, 7, 7, 6, 7, 6, 6, 4,
  0, 0, 0, 7, 0, 7, 7, 6, 0, 7, 7, 6, 7, 6, 6, 5,
  0, 7, 7, 6, 7, 6, 6, 5, 7, 6, 6, 5, 6, 5, 5, 4,

  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6,
  0, 0, 0, 0, 0, 0, 0, 6, 0, 0, 0, 6, 0, 6, 6, 5,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7,
  0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 7, 0, 7, 7, 5,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7,
  0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 7, 0, 7, 7, 6,
  0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 7, 0, 7, 7, 6,
  0, 0, 0, 7, 0, 7, 7, 6, 0, 7, 7, 6, 7, 6, 6, 5,

  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7,
  0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 7, 0, 7, 7, 6,

  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7
};
// clang-format on

}  // namespace internal
#endif                        // 64-bit, non-BMI2
#endif                        // !defined(__BMI2__)

}  // namespace fst