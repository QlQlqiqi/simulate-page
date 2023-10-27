#include "kv/kv.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>

namespace kv {
template <typename T, typename S>
KV<T, S>::KV(const char *path, const std::size_t &sz, const std::size_t &offset) {
  if ((sz - offset) % PAGE_SIZE != 0) {
    fprintf(stderr, "(sz - offset) should be multiple of PAGE_SIZE(%d)\n", PAGE_SIZE);
    return;
  }
  if (offset < (sz - offset) / PAGE_SIZE * 32 + 16) {
    fprintf(stderr, "offset should be greater or equal than (sz - offset) / PAGE_SIZE * 32 + 16\n");
    return;
  }

  int fd = open(path, O_CREAT | O_RDWR, 0644);
  // 打开文件失败
  if (fd == -1) {
    fprintf(stderr, "open file <%s> failed\n", path);
    return;
  }

  // 让这个文件的大小变为 sz + offset
  if (ftruncate(fd, sz) == -1) {
    close(fd);
    fprintf(stderr, "ftruncate exec failed\n");
    return;
  }

  // 使用 mmap 映射
  void *ptr = mmap(nullptr, sz, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (ptr == MAP_FAILED) {
    close(fd);
    fprintf(stderr, "mmap exec failed\n");
    return;
  }

  va_ = static_cast<char *>(ptr);
  offset_ = offset;
  sz_ = sz;

  // 记录不能分配的 page
  auto alloced_page_offset = std::map<std::size_t, bool>();

  // 读 next_ 和 key2va_
  auto p = va_;
  while (true) {
    std::size_t k = 0;
    memcpy(&k, p, 8);
    p += 8;
    // 读完了
    if (k == 0) {
      break;
    }
    std::size_t v = 0;
    memcpy(&v, p, 8);
    p += 8;
    next_[(char *)(k + va_)] = (char *)(v + va_);
  }

  while (true) {
    std::size_t k = 0;
    memcpy(&k, p, 8);
    p += 8;
    // 读完了
    if (k == 0) {
      break;
    }
    std::size_t offset = 0;
    memcpy(&offset, p, 8);
    p += 8;
    // key2va_[k] = (char *)(v + va_);
    S v;
    memcpy(&v, va_ + offset, sizeof(S));
    PutHash(k, v);
  }

  for (auto &item : key2va_) {
    auto p = item.second;
    while (true) {
      alloced_page_offset[(std::size_t)(p - va_)] = true;
      auto it = next_.find(p);
      if (it == next_.end()) {
        break;
      }
      p = it->second;
    }
  }

  // 分配一段内存
  FreeRange(alloced_page_offset);
}

template <typename T, typename S>
KV<T, S>::~KV() {
  // 先写同步
  Flush();

  kmem_.freelist_ = nullptr;
  // 解除这段内存的映射
  munmap(va_, sz_);
  fprintf(stdout, "unmap successfully\n");
}

template <typename T, typename S>
auto KV<T, S>::Put(const T &k, const S &v) -> bool {
  // 内存不够
  if (kmem_.freelist_ == nullptr) {
    fprintf(stderr, "there is no enough free memory\n");
    return false;
  }

  return PutHash(HashUtil<T>::Hash(k), v);
}

template <typename T, typename S>
auto KV<T, S>::PutHash(const hash_t &hash_code, const S &v) -> bool {
  auto key_it = key2va_.find(hash_code);
  // 存在一个 (k, v)
  if (key_it != key2va_.end()) {
    fprintf(stderr, "Put collide failed\n");
    return false;
  }

  char *page_ptr = AllocPage();
  // 不够分配
  if (page_ptr == nullptr) {
    return false;
  }

  key2va_[hash_code] = page_ptr;

  std::size_t alloced_idx = 0;
  auto sz = sizeof(v);

  // 把 v 复制到 pagein
  do {
    std::size_t len = std::min(sz - alloced_idx, (unsigned long)PAGE_SIZE);
    const char *tmp = static_cast<const char *>(static_cast<const void *>(&v)) + alloced_idx;
    memcpy(page_ptr, tmp, len);
    alloced_idx += PAGE_SIZE;
    // 如果还未复制完，需要新的 page
    if (alloced_idx < sz) {
      auto next_page_ptr = AllocPage();
      if (next_page_ptr == nullptr) {
        goto bad;
      }
      next_[page_ptr] = next_page_ptr;
      page_ptr = next_page_ptr;
    }
  } while (alloced_idx < sz);
  return true;

bad:
  // 把分配了部分的 page 释放掉
  DeleteHash(hash_code);
  return false;
}

template <typename T, typename S>
auto KV<T, S>::Get(const T &k, S &v) -> bool {
  auto hash_code = HashUtil<T>::Hash(k);
  auto key_it = key2va_.find(hash_code);
  if (key_it == key2va_.end()) {
    return false;
  }
  auto page_ptr = key_it->second;

  std::size_t alloced_idx = 0;
  auto sz = sizeof(v);

  // 把页表中的内容复制出来
  do {
    memcpy(reinterpret_cast<char *>(&v) + alloced_idx, (void *)page_ptr,
           std::min(sz - alloced_idx, (unsigned long)PAGE_SIZE));
    alloced_idx += PAGE_SIZE;
    // 下一个页表
    auto it = next_.find(page_ptr);
    if (it != next_.end()) {
      page_ptr = it->second;
    }
  } while (alloced_idx < sz);
  return true;
}

template <typename T, typename S>
auto KV<T, S>::Delete(const T &k) -> bool {
  // 查找分配的第一个页表
  return DeleteHash(HashUtil<T>::Hash(k));
}

template <typename T, typename S>
auto KV<T, S>::DeleteHash(const hash_t &hash_code) -> bool {
  // 查找分配的第一个页表
  auto key_it = key2va_.find(hash_code);
  if (key_it == key2va_.end()) {
    return true;
  }
  auto free_page_ptr = key_it->second;
  key2va_.erase(key_it);
  // 依次释放所有页表
  while (true) {
    auto it = next_.find(free_page_ptr);
    if (it == next_.end()) {
      break;
    }
    FreePage(free_page_ptr);
    // next page
    free_page_ptr = it->second;
    next_.erase(it);
  }
  return true;
}

template <typename T, typename S>
void KV<T, S>::Flush() {
  auto p = va_;
  for (auto &it : next_) {
    std::size_t k = it.first - va_;
    std::size_t v = it.second - va_;
    memcpy(p, &k, 8);
    p += 8;
    memcpy(p, &v, 8);
    p += 8;
  }

  // next_ 和 key2va_ 的分隔
  std::size_t div = 0;
  memcpy(p, &div, 8);
  p += 8;

  for (auto &it : key2va_) {
    std::size_t k = it.first;
    std::size_t v = it.second - va_;
    memcpy(p, &k, 8);
    p += 8;
    memcpy(p, &v, 8);
    p += 8;
    // std::cout << k << ", " << v << std::endl;
  }
  // 结束标志
  memcpy(p, &div, 8);
  p += 8;

  // 写同步
  if (msync(va_, sz_, MS_SYNC) == -1) {
    fprintf(stderr, "Flush failed: msync exec failed\n");
    return;
  }
}

template <typename T, typename S>
void KV<T, S>::FreeRange(const std::map<std::size_t, bool> &alloced_page_offset) {
  if (va_ == nullptr) {
    return;
  }

  kmem_.freelist_ = nullptr;

  auto start = va_;
  for (std::size_t i = offset_; i < sz_; i += PAGE_SIZE) {
    if (alloced_page_offset.find(i) != alloced_page_offset.end()) {
      continue;
    }
    auto r = (struct Run *)(start + i);
    r->next_ = kmem_.freelist_;
    kmem_.freelist_ = r;
  }
}

template <typename T, typename S>
void KV<T, S>::FreePage(const void *va) {
  auto r = (struct Run *)va;
  r->next_ = kmem_.freelist_;
  kmem_.freelist_ = r;
}

template <typename T, typename S>
auto KV<T, S>::AllocPage() -> char * {
  if (kmem_.freelist_ == nullptr) {
    return nullptr;
  }
  auto addr = kmem_.freelist_;
  kmem_.freelist_ = kmem_.freelist_->next_;
  return reinterpret_cast<char *>(addr);
}
}  // namespace kv