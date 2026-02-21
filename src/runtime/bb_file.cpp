// =============================================================================
// bb_file.cpp — File & Stream I/O commands.
// =============================================================================

#include "api.h"
#include "bb_globals.h"
#include <filesystem>
#include <fstream>
#include <map>

namespace fs = std::filesystem;

struct bbFileStream {
  std::fstream fs;
  bool is_eof = false;
};

static std::map<bb_int, bbFileStream *> g_streams;
static bb_int g_lastStreamHandle = 0;

struct bbDir {
  fs::directory_iterator iter;
  fs::directory_iterator end; // Default constructor is end iterator
};

static std::map<bb_int, bbDir *> g_dirs;
static bb_int g_lastDirHandle = 0;

void bbFilesCleanup() {
  for (auto const &[handle, stream] : g_streams) {
    stream->fs.close();
    delete stream;
  }
  g_streams.clear();
}

bb_int openfile(bb_string file) {
  bbFileStream *api = new bbFileStream();
  api->fs.open(file, std::ios::in | std::ios::out | std::ios::binary);
  if (!api->fs.is_open()) {
    delete api;
    return 0;
  }
  bb_int handle = ++g_lastStreamHandle;
  g_streams[handle] = api;
  return handle;
}

bb_int readfile(bb_string file) {
  bbFileStream *api = new bbFileStream();
  api->fs.open(file, std::ios::in | std::ios::binary);
  if (!api->fs.is_open()) {
    delete api;
    return 0;
  }
  bb_int handle = ++g_lastStreamHandle;
  g_streams[handle] = api;
  return handle;
}

bb_int writefile(bb_string file) {
  bbFileStream *api = new bbFileStream();
  api->fs.open(file, std::ios::out | std::ios::binary | std::ios::trunc);
  if (!api->fs.is_open()) {
    delete api;
    return 0;
  }
  bb_int handle = ++g_lastStreamHandle;
  g_streams[handle] = api;
  return handle;
}

void closefile(bb_int stream) {
  if (g_streams.count(stream)) {
    g_streams[stream]->fs.close();
    delete g_streams[stream];
    g_streams.erase(stream);
  }
}

bb_int readbyte(bb_int stream) {
  if (!g_streams.count(stream))
    return 0;
  unsigned char b = 0;
  g_streams[stream]->fs.read((char *)&b, 1);
  return (bb_int)b;
}

bb_int readshort(bb_int stream) {
  if (!g_streams.count(stream))
    return 0;
  int16_t s = 0;
  g_streams[stream]->fs.read((char *)&s, 2);
  return (bb_int)s;
}

bb_int readint(bb_int stream) {
  if (!g_streams.count(stream))
    return 0;
  int32_t i = 0;
  g_streams[stream]->fs.read((char *)&i, 4);
  return (bb_int)i;
}

bb_float readfloat(bb_int stream) {
  if (!g_streams.count(stream))
    return 0;
  float f = 0;
  g_streams[stream]->fs.read((char *)&f, 4);
  return (bb_float)f;
}

bb_string readstring(bb_int stream) {
  bb_int len = readint(stream);
  if (len <= 0)
    return "";
  bb_string s(len, ' ');
  g_streams[stream]->fs.read(&s[0], len);
  return s;
}
// =============================================================================
// Directory Commands
// =============================================================================

bb_int readdir(bb_string path) {
  if (!fs::exists(path) || !fs::is_directory(path)) {
    return 0;
  }

  bbDir *dir = new bbDir();
  try {
    dir->iter = fs::directory_iterator(path);
  } catch (...) {
    delete dir;
    return 0;
  }

  bb_int handle = ++g_lastDirHandle;
  g_dirs[handle] = dir;
  return handle;
}

bb_string nextfile(bb_int dir_handle) {
  if (g_dirs.find(dir_handle) == g_dirs.end()) {
    return "";
  }

  bbDir *dir = g_dirs[dir_handle];
  if (dir->iter == dir->end) {
    return "";
  }

  // Standard iterator loop pattern: value then increment
  // But wait, if we increment at the END of the call, we are good.
  // HOWEVER, directory_iterator can point to ".", ".." sometimes depending on
  // OS? std::filesystem::directory_iterator does NOT skip . and .. by default
  // (wait, actually it usually DOES skip them on Windows/fs). C++17
  // filesystem::directory_iterator does SKIP dot and dot-dot. Blitz3D
  // NextFile DOES return . and .. We might need to fake . and .. if we want
  // 100% parity, or just accept that C++ fs is cleaner. For now, let's just
  // return what fs gives us.

  bb_string filename = dir->iter->path().filename().string();

  // Increment for next call
  // Use Error Code to avoid exceptions on increment
  std::error_code ec;
  dir->iter.increment(ec);
  if (ec) {
    // If error, treat as end?
    dir->iter = dir->end;
  }

  return filename;
}

void closedir(bb_int dir_handle) {
  if (g_dirs.count(dir_handle)) {
    delete g_dirs[dir_handle];
    g_dirs.erase(dir_handle);
  }
}

bb_string currentdir() {
  std::error_code ec;
  auto path = fs::current_path(ec);
  if (ec)
    return "";
  return path.string(); // Note: might modify to return absolute or specific
                        // format if needed
}

void changedir(bb_string dir) {
  std::error_code ec;
  fs::current_path(dir, ec);
}

void createdir(bb_string dir) {
  std::error_code ec;
  fs::create_directory(dir, ec);
}

void deletedir(bb_string dir) {
  std::error_code ec;
  fs::remove(dir, ec); // Removes only if empty
}

bb_string readline(bb_int stream) {
  if (!g_streams.count(stream))
    return "";
  bb_string s;
  std::getline(g_streams[stream]->fs, s);
  return s;
}

void writebyte(bb_int stream, bb_int val) {
  if (!g_streams.count(stream))
    return;
  unsigned char b = (unsigned char)val;
  g_streams[stream]->fs.write((char *)&b, 1);
}

void writeshort(bb_int stream, bb_int val) {
  if (!g_streams.count(stream))
    return;
  int16_t s = (int16_t)val;
  g_streams[stream]->fs.write((char *)&s, 2);
}

void writeint(bb_int stream, bb_int val) {
  if (!g_streams.count(stream))
    return;
  int32_t i = (int32_t)val;
  g_streams[stream]->fs.write((char *)&i, 4);
}

void writefloat(bb_int stream, bb_float val) {
  if (!g_streams.count(stream))
    return;
  float f = (float)val;
  g_streams[stream]->fs.write((char *)&f, 4);
}

void writestring(bb_int stream, bb_string str) {
  writeint(stream, (bb_int)str.length());
  if (!str.empty()) {
    g_streams[stream]->fs.write(str.data(), str.length());
  }
}

void writeline(bb_int stream, bb_string str) {
  if (!g_streams.count(stream))
    return;
  g_streams[stream]->fs << str << "\n";
}

bb_int filepos(bb_int stream) {
  if (!g_streams.count(stream))
    return 0;
  return (bb_int)g_streams[stream]->fs.tellg();
}

void seekfile(bb_int stream, bb_int pos) {
  if (!g_streams.count(stream))
    return;
  g_streams[stream]->fs.seekg((size_t)pos);
  g_streams[stream]->fs.seekp((size_t)pos);
}

bb_int eof(bb_int stream) {
  if (!g_streams.count(stream))
    return 1;
  return g_streams[stream]->fs.eof() ? 1 : 0;
}

bb_int filetype(bb_string file) {
  if (!fs::exists(file))
    return 0;
  if (fs::is_directory(file))
    return 2;
  return 1;
}

bb_int filesize(bb_string file) {
  if (!fs::exists(file))
    return 0;
  return (bb_int)fs::file_size(file);
}

void deletefile(bb_string file) {
  if (fs::exists(file))
    fs::remove(file);
}

void copyfile(bb_string src, bb_string dst) {
  if (fs::exists(src))
    fs::copy(src, dst, fs::copy_options::overwrite_existing);
}
