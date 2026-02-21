// =============================================================================
// bb_string.cpp — String manipulation commands.
// =============================================================================

#include "api.h"
#include <algorithm>
#include <cstring> // For stricmp/strcasecmp
#include <iomanip>
#include <sstream>

#ifdef _WIN32
#define bb_stricmp _stricmp
#else
#define bb_stricmp strcasecmp
#endif

int _bbStrCompare(const std::string &s1, const std::string &s2) {
  return bb_stricmp(s1.c_str(), s2.c_str());
}

bb_string bb_left(bb_string str, bb_int n) {
  if (n <= 0)
    return "";
  if (n >= (bb_int)str.length())
    return str;
  return str.substr(0, (size_t)n);
}

bb_string bb_right(bb_string str, bb_int n) {
  if (n <= 0)
    return "";
  if (n >= (bb_int)str.length())
    return str;
  return str.substr(str.length() - (size_t)n);
}

bb_string bb_mid(bb_string str, bb_int start, bb_int count) {
  if (start < 1)
    start = 1;
  if (start > (bb_int)str.length())
    return "";
  if (count < 0)
    return str.substr((size_t)start - 1);
  return str.substr((size_t)start - 1, (size_t)count);
}

bb_string bb_replace(bb_string str, bb_string find, bb_string rep) {
  if (find.empty())
    return str;
  size_t pos = 0;
  while ((pos = str.find(find, pos)) != std::string::npos) {
    str.replace(pos, find.length(), rep);
    pos += rep.length();
  }
  return str;
}

bb_int bb_instr(bb_string str, bb_string find, bb_int start) {
  if (start < 1)
    start = 1;
  if (start > (bb_int)str.length())
    return 0;
  size_t pos = str.find(find, (size_t)start - 1);
  if (pos == std::string::npos)
    return 0;
  return (bb_int)pos + 1;
}

bb_string bb_upper(bb_string str) {
  std::transform(str.begin(), str.end(), str.begin(), ::toupper);
  return str;
}

bb_string bb_lower(bb_string str) {
  std::transform(str.begin(), str.end(), str.begin(), ::tolower);
  return str;
}

bb_string bb_trim(bb_string str) {
  size_t first = str.find_first_not_of(" \t\r\n");
  if (std::string::npos == first)
    return "";
  size_t last = str.find_last_not_of(" \t\r\n");
  return str.substr(first, (last - first + 1));
}

bb_string bb_lset(bb_string str, bb_int n) {
  if ((bb_int)str.length() >= n)
    return str.substr(0, (size_t)n);
  return str + std::string((size_t)n - str.length(), ' ');
}

bb_string bb_rset(bb_string str, bb_int n) {
  if ((bb_int)str.length() >= n)
    return str.substr(str.length() - (size_t)n);
  return std::string((size_t)n - str.length(), ' ') + str;
}

bb_string bb_chr(bb_int code) {
  char c = (char)code;
  return std::string(1, c);
}

bb_int bb_asc(bb_string str) {
  if (str.empty())
    return 0;
  return (bb_int)(unsigned char)str[0];
}

bb_int bb_len(bb_string str) { return (bb_int)str.length(); }

bb_string bb_hex(bb_int val) {
  std::stringstream ss;
  ss << std::uppercase << std::hex << (uint32_t)val;
  return ss.str();
}

bb_string bb_bin(bb_int val) {
  bb_string s = "";
  uint32_t v = (uint32_t)val;
  if (v == 0)
    return "0";
  while (v > 0) {
    s = (v & 1 ? "1" : "0") + s;
    v >>= 1;
  }
  return s;
}

bb_string bb_str(bb_int val) { return std::to_string(val); }

bb_string string(bb_string str, bb_int count) {
  bb_string res = "";
  for (int i = 0; i < count; ++i)
    res += str;
  return res;
}
