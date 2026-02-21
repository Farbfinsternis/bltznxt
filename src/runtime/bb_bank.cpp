// =============================================================================
// bb_bank.cpp — Memory Bank management commands.
// =============================================================================

#include "api.h"
#include <cstring>
#include <map>
#include <vector>

struct bbBank {
  std::vector<unsigned char> data;
};

static std::map<bb_int, bbBank *> g_banks;
static bb_int g_lastBankHandle = 0;

void bbBanksCleanup() {
  for (auto const &[handle, bank] : g_banks) {
    delete bank;
  }
  g_banks.clear();
}

bb_int createbank(bb_int size) {
  bbBank *bank = new bbBank();
  if (size > 0)
    bank->data.resize((size_t)size, 0);
  bb_int handle = ++g_lastBankHandle;
  g_banks[handle] = bank;
  return handle;
}

void freebank(bb_int handle) {
  if (g_banks.count(handle)) {
    delete g_banks[handle];
    g_banks.erase(handle);
  }
}

bb_int banksize(bb_int handle) {
  if (!g_banks.count(handle))
    return 0;
  return (bb_int)g_banks[handle]->data.size();
}

void resizebank(bb_int handle, bb_int size) {
  if (g_banks.count(handle)) {
    g_banks[handle]->data.resize((size_t)size, 0);
  }
}

bb_int peekbyte(bb_int handle, bb_int offset) {
  if (!g_banks.count(handle))
    return 0;
  bbBank *bank = g_banks[handle];
  if (offset < 0 || offset >= (bb_int)bank->data.size())
    return 0;
  return (bb_int)bank->data[(size_t)offset];
}

bb_int peekshort(bb_int handle, bb_int offset) {
  if (!g_banks.count(handle))
    return 0;
  bbBank *bank = g_banks[handle];
  if (offset < 0 || offset + 1 >= (bb_int)bank->data.size())
    return 0;
  int16_t val;
  std::memcpy(&val, &bank->data[(size_t)offset], 2);
  return (bb_int)val;
}

bb_int peekint(bb_int handle, bb_int offset) {
  if (!g_banks.count(handle))
    return 0;
  bbBank *bank = g_banks[handle];
  if (offset < 0 || offset + 3 >= (bb_int)bank->data.size())
    return 0;
  int32_t val;
  std::memcpy(&val, &bank->data[(size_t)offset], 4);
  return (bb_int)val;
}

bb_float peekfloat(bb_int handle, bb_int offset) {
  if (!g_banks.count(handle))
    return 0;
  bbBank *bank = g_banks[handle];
  if (offset < 0 || offset + 3 >= (bb_int)bank->data.size())
    return 0;
  float val;
  std::memcpy(&val, &bank->data[(size_t)offset], 4);
  return (bb_float)val;
}

void pokebyte(bb_int handle, bb_int offset, bb_int val) {
  if (!g_banks.count(handle))
    return;
  bbBank *bank = g_banks[handle];
  if (offset < 0 || offset >= (bb_int)bank->data.size())
    return;
  bank->data[(size_t)offset] = (unsigned char)val;
}

void pokeshort(bb_int handle, bb_int offset, bb_int val) {
  if (!g_banks.count(handle))
    return;
  bbBank *bank = g_banks[handle];
  if (offset < 0 || offset + 1 >= (bb_int)bank->data.size())
    return;
  int16_t v = (int16_t)val;
  std::memcpy(&bank->data[(size_t)offset], &v, 2);
}

void pokeint(bb_int handle, bb_int offset, bb_int val) {
  if (!g_banks.count(handle))
    return;
  bbBank *bank = g_banks[handle];
  if (offset < 0 || offset + 3 >= (bb_int)bank->data.size())
    return;
  int32_t v = (int32_t)val;
  std::memcpy(&bank->data[(size_t)offset], &v, 4);
}

void pokefloat(bb_int handle, bb_int offset, bb_float val) {
  if (!g_banks.count(handle))
    return;
  bbBank *bank = g_banks[handle];
  if (offset < 0 || offset + 3 >= (bb_int)bank->data.size())
    return;
  float v = (float)val;
  std::memcpy(&bank->data[(size_t)offset], &v, 4);
}
