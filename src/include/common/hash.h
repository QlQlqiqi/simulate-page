#ifndef HASH_H
#define HASH_H

#include <cstddef>
#include <functional>
#include "common/defs.h"

namespace kv {
template <typename T>
class HashUtil {
 public:
  static inline auto Hash(const T &t) -> hash_t { return HashUtil::hash(t); }

 private:
  static std::hash<T> hash;
};

template <typename T>
std::hash<T> HashUtil<T>::hash = std::hash<T>{};
}  // namespace kv

#endif