///
/// @file   S2_trivial_xy.cpp
/// @brief  Test the computation of the trivial special leaves
///         S2_trivial(x, y) used in the Deleglise-Rivat prime
///         counting algorithm.
///
///         Trivial special leaves are leaves that satisfy:
///         phi(x / n, b - 1) = 1
///         with n = primes[b] * primes[l]
///
/// Copyright (C) 2019 Kim Walisch, <kim.walisch@gmail.com>
///
/// This file is distributed under the BSD License. See the COPYING
/// file in the top level directory.
///

#include <S2.hpp>
#include <primecount.hpp>
#include <primecount-internal.hpp>
#include <generate.hpp>
#include <PhiTiny.hpp>
#include <imath.hpp>

#include <stdint.h>
#include <iostream>
#include <cstdlib>
#include <random>

using namespace std;
using namespace primecount;

void check(bool OK)
{
  cout << "   " << (OK ? "OK" : "ERROR") << "\n";
  if (!OK)
    exit(1);
}

int main()
{
  random_device rd;
  mt19937 gen(rd());
  uniform_int_distribution<int> dist(0, 10000000);

  for (int i = 0; i < 100; i++)
  {
    int64_t x = dist(gen);
    int64_t x13 = iroot<3>(x);
    double alpha = get_alpha_deleglise_rivat(x);
    int64_t y = (int64_t)(x13 * alpha);
    int64_t z = x / y;
    int64_t c = PhiTiny::get_c(y);
    int64_t s2_trivial = 0;

    auto primes = generate_primes<int64_t>(y);

    for (size_t b = c + 1; b < primes.size(); b++)
    {
      for (size_t l = b + 1; l < primes.size(); l++)
      {
        int64_t n = primes[b] * primes[l];

        if (n > x)
          break;
        if (phi(x / n, b - 1) == 1)
          s2_trivial++;
      }
    }

    cout << "S2_trivial(" << x << ", " << y << ") = " << s2_trivial;
    check(s2_trivial == S2_trivial(x, y, z, c));
  }

  cout << endl;
  cout << "All tests passed successfully!" << endl;

  return 0;
}
