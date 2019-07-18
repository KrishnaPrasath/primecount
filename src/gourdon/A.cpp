///
/// @file  A.cpp
///
/// Copyright (C) 2019 Kim Walisch, <kim.walisch@gmail.com>
///
/// This file is distributed under the BSD License. See the COPYING
/// file in the top level directory.
///

#include <gourdon.hpp>
#include <PiTable.hpp>
#include <primecount-internal.hpp>
#include <fast_div.hpp>
#include <generate.hpp>
#include <int128_t.hpp>
#include <min.hpp>
#include <imath.hpp>
#include <print.hpp>
#include <S2Status.hpp>
#include <S2.hpp>

#include <stdint.h>
#include <vector>

using namespace std;
using namespace primecount;

namespace {

template <typename T, typename Primes>
T A_OpenMP(T x,
           int64_t y,
           int64_t start,
           Primes& primes,
           int threads)
{
  T sum = 0;
  int64_t x13 = iroot<3>(x);
  int64_t thread_threshold = 1000;
  threads = ideal_num_threads(threads, x13, thread_threshold);

  PiTable pi(isqrt(x));
  int64_t pi_x13 = pi[x13];
  S2Status status(x);

  #pragma omp parallel for schedule(dynamic) num_threads(threads) reduction(+: sum)
  for (int64_t b = pi[start] + 1; b <= pi_x13; b++)
  {
    int64_t prime = primes[b];
    T x2 = x / prime;
    int64_t j = b + 1;
    int64_t max_j = pi[isqrt(x2)];

    for (; j <= max_j; j++)
    {
      int64_t xn = fast_div64(x2, primes[j]);
      if (xn < y)
        sum += pi[xn] * 2;
      else
        sum += pi[xn];
    }

    if (is_print())
      status.print(b, pi_x13);
  }

  return sum;
}

} // namespace

namespace primecount {

int64_t A(int64_t x,
          int64_t y,
          int threads)
{
  print("");
  print("=== A(x, y) ===");
  print(x, y, threads);

  double time = get_time();
  int64_t y2 = y * y;
  int64_t start = max(iroot<4>(x), x / y2);
  int64_t max_prime = (int64_t) isqrt(x / start);

  auto primes = generate_primes<int32_t>(max_prime);
  int64_t a = A_OpenMP((intfast64_t) x, y, start, primes, threads);

  print("A", a, time);
  return a;
}

#ifdef HAVE_INT128_T

int128_t A(int128_t x,
           int64_t y,
           int threads)
{
  print("");
  print("=== A(x, y) ===");
  print(x, y, threads);

  double time = get_time();
  int128_t a;

  int128_t y2 = (int128_t) y * y;
  int128_t start = max(iroot<4>(x), x / y2);
  int64_t max_prime = (int64_t) isqrt(x / start);

  // uses less memory
  if (max_prime <= numeric_limits<uint32_t>::max())
  {
    auto primes = generate_primes<uint32_t>(max_prime);
    a = A_OpenMP((intfast128_t) x, y, start, primes, threads);
  }
  else
  {
    auto primes = generate_primes<int64_t>(max_prime);
    a = A_OpenMP((intfast128_t) x, y, start, primes, threads);
  }

  print("A", a, time);
  return a;
}

#endif

} // namespace
