#include "memory.h"
#include <algorithm>
#include <iostream>

std::vector<MemoryManager::Allocation> MemoryManager::allocations;

void MemoryManager::track(void *ptr, Deleter deleter, const char *typeName) {
  allocations.push_back({ptr, deleter, typeName});
}

void MemoryManager::cleanup() {
  std::cerr << "MemoryManager::cleanup() started. " << allocations.size()
            << " objects." << std::endl;
  // Delete in reverse order of allocation (LIFO) to respect dependencies
  int count = 0;
  for (auto it = allocations.rbegin(); it != allocations.rend(); ++it) {
    if (it->ptr && it->deleter) {
      if (it->typeName &&
          std::string(it->typeName).find("Node") != std::string::npos) {
        // std::cerr << "Skipping Node " << count++ << std::endl;
        count++;
        continue;
      }
      std::cerr << "Deleting " << count++
                << " type: " << (it->typeName ? it->typeName : "unknown")
                << " at " << it->ptr << std::endl;
      it->deleter(it->ptr);
    }
  }
  std::cerr << "MemoryManager::cleanup() finished." << std::endl;
  allocations.clear();
}
