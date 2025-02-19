///
/// @file  aligned_vector.hpp
///
/// Copyright (C) 2019 Kim Walisch, <kim.walisch@gmail.com>
///
/// This file is distributed under the BSD License. See the COPYING
/// file in the top level directory.
///

#ifndef ALIGNED_VECTOR_HPP
#define ALIGNED_VECTOR_HPP

#include <cstddef>
#include <vector>

/// Maximum cache line size of current CPUs.
/// Note that in 2019 all x86 CPU have a cache line size of 64 bytes.
/// However there are CPUs out there that have much larger cache line
/// sizes e.g. IBM z13 CPUs from 2015 have a cache line size of 256
/// bytes. Hence in order to be future-proof we set the maximum cache
/// line size to 1 kilobyte.
///
#ifndef CACHE_LINE_SIZE
  #define CACHE_LINE_SIZE 1024
#endif

namespace primecount {

/// The aligned_vector class aligns each of its
/// elements on a new cache line in order to avoid
/// false sharing (cache trashing) when multiple
/// threads write to adjacent elements
///
template <typename T>
class aligned_vector
{
public:
  aligned_vector(std::size_t size)
    : vect_(size) { }
  std::size_t size() const { return vect_.size(); }
  T& operator[](std::size_t pos) { return vect_[pos].val[0]; }
private:
  struct align_t
  {
    T val[CACHE_LINE_SIZE / sizeof(T)];
  };
  std::vector<align_t> vect_;
};

} // namespace

#endif
