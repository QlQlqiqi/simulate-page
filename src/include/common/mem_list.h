#include <cstddef>

struct Run
{
  // std::size_t offset_;
  struct Run *next_;
};

struct Kmem
{
  struct Run *freelist_;
};