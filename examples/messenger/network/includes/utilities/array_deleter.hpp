#ifndef ARRAY_DELETER_HPP
#define ARRAY_DELETER_HPP

template< typename T >
struct array_deleter
{
  void operator ()( T const * p)
  { 
    delete[] p; 
  }
};

#endif