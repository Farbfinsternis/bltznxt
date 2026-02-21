
#ifndef STD_COMPILER_H
#define STD_COMPILER_H

// BltzNext: Standalone std.h
// Replaces legacy config.h + stdutil.h with minimal inline definitions.

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <list>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

// using namespace std; <--- REMOVED

// --- stdutil shims ---

// d_new: debug allocator macro from original Blitz3D
#include "memory.h"

#include <typeinfo>

// d_new: debug allocator now using MemoryManager
template <typename T, typename... Args> T *d_new(Args &&...args) {
  T *ptr = new T(std::forward<Args>(args)...);
  MemoryManager::track(ptr, standardDeleter<T>, typeid(T).name());
  return ptr;
}

// a_ptr: lazy auto_ptr from original stdutil.h
template <class T> class a_ptr {
public:
  a_ptr(T *t = 0) : t(t) {}
  ~a_ptr() { /* delete t; */ }
  a_ptr &operator=(T *t) {
    this->t = t;
    return *this;
  }
  T &operator*() const { return *t; }
  T *operator->() const { return t; }
  operator T &() const { return *t; }
  operator T *() const { return t; }
  T *release() {
    T *tt = t;
    t = 0;
    return tt;
  }

private:
  T *t;
};

// String utility functions used by the original compiler
inline int atoi(const std::string &s) { return std::atoi(s.c_str()); }
inline double atof(const std::string &s) { return std::atof(s.c_str()); }

inline std::string itoa(int n) {
  char buf[32];
  snprintf(buf, sizeof(buf), "%d", n);
  return std::string(buf);
}

inline std::string ftoa(float n) {
  char buf[64];
  snprintf(buf, sizeof(buf), "%f", n);
  return std::string(buf);
}

inline std::string tolower(const std::string &s) {
  std::string r = s;
  for (size_t i = 0; i < r.size(); ++i)
    r[i] = std::tolower((unsigned char)r[i]);
  return r;
}

inline std::string toupper(const std::string &s) {
  std::string r = s;
  for (size_t i = 0; i < r.size(); ++i)
    r[i] = std::toupper((unsigned char)r[i]);
  return r;
}

#endif