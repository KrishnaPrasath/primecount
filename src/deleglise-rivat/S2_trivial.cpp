///
/// @file  S2_trivial.cpp
/// @brief Calculate the contribution of the trivial special leaves
///        in parallel using OpenMP.
///
/// Copyright (C) 2018 Kim Walisch, <kim.walisch@gmail.com>
///
/// This file is distributed under the BSD License. See the COPYING
/// file in the top level directory.
///

#include <PiTable.hpp>
#include <primecount.hpp>
#include <primecount-internal.hpp>
#include <primesieve.hpp>
#include <calculator.hpp>
#include <generate.hpp>
#include <int128_t.hpp>
#include <json.hpp>
#include <imath.hpp>

#include <stdint.h>
#include <algorithm>

#ifdef _OPENMP
  #include <omp.h>
#endif

using namespace std;
using namespace primecount;

namespace {

template <typename T>
void backup(T x,
            int64_t y,
            int64_t z,
            T s2_trivial,
            double time)
{
  auto json = load_backup();

  json["S2_trivial"]["x"] = to_string(x);
  json["S2_trivial"]["y"] = y;
  json["S2_trivial"]["z"] = z;
  json["S2_trivial"]["s2_trivial"] = to_string(s2_trivial);
  json["S2_trivial"]["percent"] = 100.0;
  json["S2_trivial"]["seconds"] = get_wtime() - time;

  store_backup(json);
}

template <typename T>
bool resume(T x,
            int64_t y,
            int64_t z,
            T& s2_trivial,
            double& time)
{
  auto json = load_backup();

  if (is_resume(json, "S2_trivial", x, y, z))
  {
    double percent = json["S2_trivial"]["percent"];
    double seconds = json["S2_trivial"]["seconds"];

    s2_trivial = calculator::eval<T>(json["S2_trivial"]["s2_trivial"]);
    time = get_wtime() - seconds;
    print_resume(percent, x);
    return true;
  }

  return false;
}

template <typename T>
T S2_trivial_OpenMP(T x,
                    int64_t y,
                    int64_t z,
                    int64_t c,
                    int threads)
{
  int64_t thread_threshold = ipow(10, 7);
  threads = ideal_num_threads(threads, y, thread_threshold);

  PiTable pi(y);
  int64_t pi_y = pi[y];
  int64_t sqrtz = isqrt(z);
  int64_t prime_c = nth_prime(c);

  T s2_trivial = 0;

  // Find all trivial leaves: n = primes[b] * primes[l]
  // which satisfy phi(x / n), b - 1) = 1
  #pragma omp parallel for num_threads(threads) reduction(+: s2_trivial)
  for (int64_t i = 0; i < threads; i++)
  {
    int64_t start = max(prime_c, sqrtz) + 1;
    int64_t thread_distance = ceil_div(y - start, threads);
    start += thread_distance * i;
    int64_t stop = min(start + thread_distance, y);
    primesieve::iterator it(start - 1, stop);
    T prime;

    while ((prime = it.next_prime()) < stop)
    {
      int64_t xn = (int64_t) max(x / (prime * prime), prime);
      s2_trivial += pi_y - pi[xn];
    }
  }

  return s2_trivial;
}

} // namespace

namespace primecount {

int64_t S2_trivial(int64_t x,
                   int64_t y,
                   int64_t z,
                   int64_t c,
                   int threads)
{
  print_log("");
  print_log("=== S2_trivial(x, y) ===");
  print_log("Computation of the trivial special leaves");
  print_log(x, y, c, threads);

  double time = get_wtime();
  int64_t s2_trivial;

  if (!resume(x, y, z, s2_trivial, time))
  {
    s2_trivial = S2_trivial_OpenMP(x, y, z, c, threads);
    backup(x, y, z, s2_trivial, time);
  }

  print_log("S2_trivial", s2_trivial, time);
  return s2_trivial;
}

#ifdef HAVE_INT128_T

int128_t S2_trivial(int128_t x,
                    int64_t y,
                    int64_t z,
                    int64_t c,
                    int threads)
{
  print_log("");
  print_log("=== S2_trivial(x, y) ===");
  print_log("Computation of the trivial special leaves");
  print_log(x, y, c, threads);

  double time = get_wtime();
  int128_t s2_trivial;

  if (!resume(x, y, z, s2_trivial, time))
  {
    s2_trivial = S2_trivial_OpenMP(x, y, z, c, threads);
    backup(x, y, z, s2_trivial, time);
  }

  print_log("S2_trivial", s2_trivial, time);
  return s2_trivial;
}

#endif

} // namespace
