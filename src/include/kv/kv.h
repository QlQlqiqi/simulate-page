#ifndef KV_H
#define KV_H

#include <cstddef>
#include <map>
#include <string>
#include "common/defs.h"
#include "common/hash.h"
#include "common/mem_list.h"

namespace kv {
template <typename T, typename S>
class KV {
 public:
  /**
   * 将一段大小为 sz 的内存映射到 path 文件（如果不存在则创建），并指定映射的偏移量；
   * (sz - offset) 必须是 PAGE_SIZE 的整数倍；
   * key 比较的方式是对其进行 hash，比较 hash code
   *
   * next_ 和 key2va_ 需要持久化存储在 [va_, va_ + offset) 这段内存中，所以 offset 不应该
   * 相对于 sz 过于小；在 64-bit 机器上，offset >= ((sz - offset) / PAGE_SIZE * 32) + 16
   */
  explicit KV(const char *path, const std::size_t &sz, const std::size_t &offset);

  ~KV();

  /** insert (k, v)，不能冲突 */
  auto Put(const T &k, const S &v) -> bool;

  /**
   * 如果成功，则将 value 放入 v，并返回 true；否则返回 false
   */
  auto Get(const T &k, S &v) -> bool;

  /** 删除 key */
  auto Delete(const T &k) -> bool;

  /** 阻塞写数据到磁盘，包括 next_ 和 key2va_ */
  void Flush();

 private:
  auto PutHash(const hash_t &hash_code, const S &v) -> bool;
  auto DeleteHash(const hash_t &hash_code) -> bool;

  /** 释放一段内存 */
  void FreeRange(const std::map<std::size_t, bool> &alloced_page_offset);

  /** 释放一个页表 */
  void FreePage(const void *va);

  /** 分配一个页表 */
  auto AllocPage() -> char *;

  // free page list
  struct Kmem kmem_;
  // 映射的虚拟地址
  char *va_{nullptr};
  // 偏移
  std::size_t offset_;
  // 映射的内存大小
  std::size_t sz_{0};
  // 如果一个 value 需要多个 page 存储，则记录下一个 page 的 va
  std::map<char *, char *> next_;
  // 记录 key's hash code 到 va 的映射
  std::map<hash_t, char *> key2va_;
};
}  // namespace kv

#include "kv/kv.cpp"
#endif