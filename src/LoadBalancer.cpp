///
/// @file  LoadBalancer.cpp
/// @brief The LoadBalancer assigns work to the individual threads
///        in the computation of the special leaves in the
///        Lagarias-Miller-Odlyzko, Deleglise-Rivat and Gourdon
///        prime counting algorithms.
///
///        Simply parallelizing the computation of the special
///        leaves in the Lagarias-Miller-Odlyzko algorithm by
///        subdividing the sieve interval by the number of threads
///        into equally sized subintervals does not scale because
///        the distribution of the special leaves is highly skewed
///        and most special leaves are in the first few segments
///        whereas later on there are very few special leaves.
///
///        This LoadBalancer gradually increases the number of
///        segments to sieve as long the expected runtime of the
///        sieve distance is smaller than the expected finish time
///        of the algorithm. Near the end the LoadBalancer will
///        gradually decrease the number of segments to sieve in
///        order to prevent that 1 thread will run much longer
///        than all the other threads.
///
/// Copyright (C) 2019 Kim Walisch, <kim.walisch@gmail.com>
///
/// This file is distributed under the BSD License. See the COPYING
/// file in the top level directory.
///

#include <LoadBalancer.hpp>
#include <primecount-internal.hpp>
#include <S2Status.hpp>
#include <Sieve.hpp>
#include <imath.hpp>
#include <int128_t.hpp>
#include <min.hpp>
#include <print.hpp>

#include <stdint.h>
#include <cmath>

using namespace std;

namespace primecount {

LoadBalancer::LoadBalancer(maxint_t x,
                           int64_t sieve_limit,
                           maxint_t sum_approx) :
  low_(0),
  max_low_(0),
  sieve_limit_(sieve_limit),
  segments_(1),
  sum_(0),
  sum_approx_(sum_approx),
  time_(get_time()),
  status_(x)
{
  // start with a tiny segment_size as most
  // special leaves are in the first few segments
  // and we need to ensure that all threads are
  // assigned an equal amount of work
  int64_t sqrt_limit = isqrt(sieve_limit);
  int64_t log = ilog(sqrt_limit);
  log = max(log, 1);
  segment_size_ = sqrt_limit / log;

  int64_t min_size = 1 << 9;
  segment_size_ = max(segment_size_, min_size);
  segment_size_ = Sieve::get_segment_size(segment_size_);

  // try to use a segment size that fits exactly
  // into the CPUs L1 data cache
  int64_t l1_dcache_size = 1 << 15;
  max_size_ = l1_dcache_size * 30;
  max_size_ = max(max_size_, sqrt_limit);
  max_size_ = Sieve::get_segment_size(max_size_);
}

maxint_t LoadBalancer::get_sum() const
{
  return sum_;
}

bool LoadBalancer::get_work(int64_t* low,
                            int64_t* segments,
                            int64_t* segment_size,
                            maxint_t sum,
                            Runtime& runtime)
{
  #pragma omp critical (get_work)
  {
    sum_ += sum;

    update(low, segments, runtime);

    *low = low_;
    *segments = segments_;
    *segment_size = segment_size_;
    low_ += segments_ * segment_size_;

    if (is_print())
      status_.print(sum_, sum_approx_);
  }

  return *low <= sieve_limit_;
}

void LoadBalancer::update(int64_t* low,
                          int64_t* segments,
                          Runtime& runtime)
{
  if (*low > max_low_)
  {
    max_low_ = *low;
    segments_ = *segments;

    // We only start increasing the segment_size and segments
    // per thread once the first special leaves have been
    // found. Near the start there is a very large number of
    // leaves and we don't want a single thread to compute
    // them all by himself (which would cause scaling issues).
    if (sum_ == 0)
      return;

    if (segment_size_ < max_size_)
      segment_size_ = min(segment_size_ * 2, max_size_);
    else
      update_segments(runtime);
  }
}

/// Remaining seconds till finished
double LoadBalancer::remaining_secs() const
{
  double percent = status_.getPercent(low_, sieve_limit_, sum_, sum_approx_);
  percent = in_between(10, percent, 100);
  double total_secs = get_time() - time_;
  double secs = total_secs * (100 / percent) - total_secs;
  return secs;
}

/// Increase or decrease the number of segments based on the
/// remaining runtime. Near the end it is important that
/// threads run only for a short amount of time in order to
/// ensure all threads finish nearly at the same time.
///
void LoadBalancer::update_segments(Runtime& runtime)
{
  double rem = remaining_secs();
  double threshold = rem / 4;
  double min_secs = 0.01;

  // Each thread should run at least 10x
  // longer than its initialization time
  threshold = max(threshold, runtime.init * 10);
  threshold = max(threshold, min_secs);

  // divider must not be 0
  double divider = max(runtime.secs, min_secs / 10);
  double factor = threshold / divider;

  // Reduce the thread runtime if it is much
  // larger than its initialization time
  if (runtime.secs > min_secs &&
      runtime.secs > runtime.init * 1000)
  {
    double old = factor;
    factor = (runtime.init * 1000) / runtime.secs;
    factor = min(factor, old);
  }

  factor = in_between(0.5, factor, 2.0);
  double new_segments = round(segments_ * factor);
  segments_ = (int64_t) new_segments;
  segments_ = max(segments_, 1);
}

} // namespace
