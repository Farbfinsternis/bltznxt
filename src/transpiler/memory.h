#ifndef MEMORY_H
#define MEMORY_H

#include <functional>
#include <vector>

class MemoryManager {
public:
  using Deleter = void (*)(void *);

  static void track(void *ptr, Deleter deleter, const char *typeName);
  static void cleanup();

private:
  struct Allocation {
    void *ptr;
    Deleter deleter;
    const char *typeName;
  };
  static std::vector<Allocation> allocations;
};

template <typename T> void standardDeleter(void *ptr) {
  delete static_cast<T *>(ptr);
}

#endif // MEMORY_H
