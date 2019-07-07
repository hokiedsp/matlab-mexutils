#pragma once

// CUSTOM STL allocator using the Intel MKL memory management functions
#include <memory>
#include <mex.h>

/**
 * \brief Allocator to use MATLAB memory manager
 *
 * A custom C++ Allocator to use MATLAB memory manager.
 *
 * \note DO NOT USE THIS ALLOCATOR BEYOND SIMPLE DATA TYPES!!
 *       In general, the use of C-based memory management functions should
 *       not be used in C++ Allocator class. However, it works fine as long
 *       as it is managing a simple data type (int, double, char, etc.).
 *       There may be unforeseen consequence if this class is used to allocate
 *       memory for a struct or class.
 *
 * \note Intended Application: Use this Allocator for a generic C++ template
 *       library which must dynamically allocate memory but also releases the
 *       allocated memory to the user, which is under a specific memory
 * management regime (i.e., MATLAB) to avoid copying.
 */
#include <type_traits>
template <typename T> struct mexAllocator
{
  template <typename U> friend struct mexAllocator;
  using value_type = T;
  using pointer = T *;
  using propagate_on_container_copy_assignment = std::true_type;
  using propagate_on_container_move_assignment = std::true_type;
  using propagate_on_container_swap = std::true_type;
  template <typename U> mexAllocator() {}
  pointer allocate(std::size_t n)
  {
    pointer p = NULL;
    if (!hint)
    {
      p = reinterpret_cast<pointer>(mxCalloc(n, sizeof(T)));
      mexMakeMemoryPersistent(p);
    }
    else
    {
      p = reinterpret_cast<pointer>(mxRealloc((void *)hint, n * sizeof(T)));
    }
    return p;
  }
  void deallocate(pointer p, std::size_t n) { mxFree(p); }
  // template <typename U> bool operator==(ArenaAllocator<U> const &rhs) const
  // {
  //   return arena == rhs.arena;
  // }
  // template <typename U> bool operator!=(ArenaAllocator<U> const &rhs) const
  // {
  //   return arena != rhs.arena;
  // }
};
