///
/// @file  P3.cpp
/// @brief P3(x, a) is the 3rd partial sieve function, it is used
///        in Lehmer's prime counting formula.
///
/// Copyright (C) 2019 Kim Walisch, <kim.walisch@gmail.com>
///
/// This file is distributed under the BSD License. See the COPYING
/// file in the top level directory.
///

#include <primecount-internal.hpp>
#include <generate.hpp>
#include <imath.hpp>
#include <print.hpp>

#include <stdint.h>

using namespace std;

namespace primecount {

/// P3(x, a) counts the numbers <= x that have exactly 3
/// prime factors each exceeding the a-th prime.
/// Memory usage: O(pi(sqrt(x)))
///
int64_t P3(int64_t x, int64_t a, int threads)
{
  print("");
  print("=== P3(x, a) ===");
  print("Computation of the 3rd partial sieve function");

  double time = get_time();
  auto primes = generate_primes<int32_t>(isqrt(x));

  int64_t y = iroot<3>(x);
  int64_t pi_y = pi_bsearch(primes, y);
  int64_t sum = 0;

  threads = ideal_num_threads(threads, pi_y, 100);

  #pragma omp parallel for num_threads(threads) schedule(dynamic) reduction(+: sum)
  for (int64_t i = a + 1; i <= pi_y; i++)
  {
    int64_t xi = x / primes[i];
    int64_t bi = pi_bsearch(primes, isqrt(xi));

    for (int64_t j = i; j <= bi; j++)
      sum += pi_bsearch(primes, xi / primes[j]) - (j - 1);
  }

  print("P3", sum, time);
  return sum;
}

} // namespace
