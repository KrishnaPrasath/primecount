///
/// @file  PiTable.hpp
/// @brief The PiTable class is a compressed lookup table for
///        prime counts. Each bit in the lookup table corresponds
///        to an odd integer and that bit is set to 1 if the
///        integer is a prime. PiTable uses only (n / 8) bytes of
///        memory and returns the number of primes <= n in O(1)
///        operations.
///
/// Copyright (C) 2019 Kim Walisch, <kim.walisch@gmail.com>
///
/// This file is distributed under the BSD License. See the COPYING
/// file in the top level directory.
///

#ifndef PITABLE_HPP
#define PITABLE_HPP

#include <popcnt.hpp>

#include <stdint.h>
#include <array>
#include <cassert>
#include <vector>

#if defined(__GNUC__) || \
    defined(__clang__)
  #define unlikely(x) __builtin_expect(!!(x), 0)
#else
  #define unlikely(x) (x)
#endif

namespace primecount {

class PiTable
{
public:
  PiTable(uint64_t max);

  /// Get number of primes <= n
  int64_t operator[](uint64_t n) const
  {
    assert(n <= max_);

    // Since we store only odd numbers in our lookup table,
    // we cannot store 2 which is the only even prime.
    // As a workaround we mark 1 as a prime (1st bit) and
    // add a check to return 0 for pi[1].
    if (unlikely(n == 1))
      return 0;

    uint64_t bitmask = unset_bits_[n % 128];
    uint64_t prime_count = pi_[n / 128].prime_count;
    uint64_t bit_count = popcnt64(pi_[n / 128].bits & bitmask);
    return prime_count + bit_count;
  }

  int64_t size() const
  {
    return max_ + 1;
  }

private:
  struct PiData
  {
    uint64_t prime_count = 0;
    uint64_t bits = 0;
  };

  static const std::array<uint64_t, 128> unset_bits_;
  std::vector<PiData> pi_;
  uint64_t max_;
};

} // namespace

#endif
