#include "kv/kv.h"
#include <cstddef>
#include <cstdio>
#include <iostream>
#include <string>
 
const char *file_path = DISK_PATH;
const std::size_t sz = 0x600;
const std::size_t offset = 0x400;

int main() {
  auto kv = kv::KV<int, int>(file_path, sz, offset);

  {
    // 插入 (0, 0), (1, 1), ... , (9, 9)
    for (int i = 1; i < 10; i++) {
        printf("put kv (%d, %d)\n", i, i);
      kv.Put(i, i);
    }
  }

  {
    // 查询 key from 0 to 9
    for (int i = 1; i < 10; i++) {
      int value;
      if (kv.Get(i, value)) {
        std::cout << "Get: " << i << ", " << value << std::endl;
      }
    }
  }

  {
    // 删除 key from 0 to 5
    for (int i = 0; i < 6; i++) {
      if (kv.Delete(i)) {
        std::cout << "delete key: " << i << std::endl;
      }
    }
  }
  
  {
    // 插入 (0, 0), (1, 1), ... , (9, 9)
    for (int i = 0; i < 10; i++) {
        printf("put kv (%d, %d)\n", i, i);
      kv.Put(i, i);
    }
  }

  {
    // 查询 key from 0 to 9
    for (int i = 0; i < 10; i++) {
      int value;
      if (kv.Get(i, value)) {
        std::cout << value << std::endl;
      }
    }
  }

  // kv.Flush();
  return 0;
}