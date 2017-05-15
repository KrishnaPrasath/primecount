///
/// @file  LoadBalancer.cpp
/// @brief The LoadBalancer assigns work to the individual
///        threads in the computation of the special leaves
///        in the Lagarias-Miller-Odlyzko and
///        Deleglise-Rivat prime counting algorithms.
///
/// Simply parallelizing the computation of the special leaves in the
/// Lagarias-Miller-Odlyzko and Deleglise-Rivat algorithms by
/// subdividing the sieve interval by the number of threads into
/// equally sized subintervals does not scale because the distribution
/// of the special leaves is highly skewed and most special leaves are
/// in the first few segments whereas later on there are very few
/// special leaves.
///
/// Copyright (C) 2017 Kim Walisch, <kim.walisch@gmail.com>
///
/// This file is distributed under the BSD License. See the COPYING
/// file in the top level directory.
///

#include <LoadBalancer.hpp>
#include <primecount-internal.hpp>
#include <S2Status.hpp>
#include <imath.hpp>
#include <int128_t.hpp>
#include <min.hpp>

#include <stdint.h>
#include <cmath>

#ifdef _OPENMP
  #include <omp.h>
#endif

using namespace std;

namespace primecount {

LoadBalancer::LoadBalancer(maxint_t x,
                          int64_t y,
                          int64_t z,
                          double alpha,
                          maxint_t s2_approx) :
  status_(x),
  low_(1),
  limit_(z + 1),
  segments_(1),
  s2_approx_(s2_approx),
  S2_total_(0),
  time_(get_wtime())
{
  init_size(z);
  smallest_hard_leaf_ = (int64_t) (x / (y * sqrt(alpha) * iroot<6>(x)));
}

void LoadBalancer::init_size(int64_t z)
{
  int64_t min_size = 1 << 9;
  int64_t sqrtz = isqrt(z);
  segment_size_ = max(min_size, sqrtz);
  segment_size_ = next_power_of_2(segment_size_);
}

maxint_t LoadBalancer::get_result()
{
  return S2_total_;
}

bool LoadBalancer::is_increase(Runtime& runtime)
{
  double min_seconds = max(0.01, runtime.init * 10);
  double total_time = get_wtime() - time_;
  double percent = status_.skewed_percent(S2_total_, s2_approx_);

  if (runtime.seconds < min_seconds)
    return true;

  // avoid division by 0
  percent = in_between(1, percent, 99.9);

  // remaining seconds till finished
  double remaining = total_time * (100 / percent) - total_time;
  double threshold = remaining / 4;
  threshold = max(min_seconds, threshold);

  return runtime.seconds < threshold;
}

bool LoadBalancer::get_work(int64_t* low,
                            int64_t* segments,
                            int64_t* segment_size,
                            maxint_t S2,
                            Runtime& runtime)
{
  #pragma omp critical (S2_schedule)
  {
    int64_t high = low_ + segments_ * segment_size_;

    // Most hard special leaves are located just past
    // smallest_hard_leaf_. In order to prevent assigning
    // the bulk of work to a single thread we reduce
    // the number of segments to a minimum.
    //
    if (low_ <= smallest_hard_leaf_ &&
        high >= smallest_hard_leaf_)
    {
      segments_ = 1;
    }

    *low = low_;
    *segments = segments_;
    *segment_size = segment_size_;

    S2_total_ += S2;
    low_ += segments_ * segment_size_;
    low_ = min(low_, limit_);

    if (is_increase(runtime))
      segments_ += ceil_div(segments_, 3);
    else
      segments_ -= segments_ / 4;
  }

  if (is_print())
    status_.print(S2_total_, s2_approx_);

  return *low < limit_;
}

} // namespace
