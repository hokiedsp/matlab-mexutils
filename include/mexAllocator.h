#pragma once

// CUSTOM STL allocator using the Intel MKL memory management functions
#include <mex.h>
#include <memory>

template <typename T>
class mexAllocator : public std::allocator<T>
{
public:
  mexAllocator() {}
  mexAllocator &operator=(const mexAllocator &rhs)
  {
    std::allocator<T>::operator=(rhs);
    return *this;
  }

  pointer allocate(size_type n, const void *hint = NULL)
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

  void deallocate(pointer p, size_type n)
  {
    mxFree(p);
  }

  void construct(pointer p, const T &val)
  {
    new (p) T(val);
  }

  void destroy(pointer p)
  {
    p->~T();
  }
};
