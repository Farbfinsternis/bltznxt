// =============================================================================
// bb_data.cpp — Data Management (Data/Read/Restore) and Arrays (Dim).
// =============================================================================

#include "bb_data.h"
#include "bb_globals.h"
#include <string>

void bbDim(bb_string name, std::initializer_list<bb_int> dims) {
  bb_array &arr = g_arrays[name];
  arr.dims = dims;
  size_t size = 1;
  for (auto d : dims)
    size *= (d + 1);
  arr.data.assign(size, bb_value((bb_int)0));
}

bb_value &bbArrayAccess(bb_string name, std::initializer_list<bb_int> indices) {
  bb_array &arr = g_arrays[name];
  if (arr.data.empty()) {
    static bb_value dummy;
    return dummy;
  }
  size_t index = 0;
  size_t stride = 1;

  auto it_idx = indices.begin();
  auto it_dim = arr.dims.begin();

  for (; it_idx != indices.end() && it_dim != arr.dims.end();
       ++it_idx, ++it_dim) {
    index += (*it_idx) * stride;
    stride *= (*it_dim + 1);
  }

  if (index >= arr.data.size()) {
    static bb_value dummy;
    return dummy;
  }
  return arr.data[index];
}

void bbRegisterData(const bb_data_value &val, const std::string &label) {
  if (!label.empty()) {
    g_data_labels[label] = g_data.size();
  }
  g_data.push_back(val);
}

void bbRestore(const std::string &label) {
  if (label.empty()) {
    g_data_ptr = 0;
  } else if (g_data_labels.count(label)) {
    g_data_ptr = g_data_labels[label];
  }
}

bb_int bbReadInt() {
  if (g_data_ptr >= g_data.size())
    return 0;
  const auto &val = g_data[g_data_ptr++];
  if (val.type == BB_DATA_INT)
    return val.i;
  if (val.type == BB_DATA_FLOAT)
    return (bb_int)val.f;
  return 0;
}

bb_float bbReadFloat() {
  if (g_data_ptr >= g_data.size())
    return 0.0;
  const auto &val = g_data[g_data_ptr++];
  if (val.type == BB_DATA_FLOAT)
    return val.f;
  if (val.type == BB_DATA_INT)
    return (bb_float)val.i;
  return 0.0;
}

bb_string bbReadString() {
  if (g_data_ptr >= g_data.size())
    return "";
  const auto &val = g_data[g_data_ptr++];
  if (val.type == BB_DATA_STRING)
    return val.s;
  if (val.type == BB_DATA_INT)
    return std::to_string(val.i);
  if (val.type == BB_DATA_FLOAT)
    return std::to_string(val.f);
  return "";
}
