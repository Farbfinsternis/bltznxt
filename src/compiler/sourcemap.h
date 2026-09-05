#ifndef BLITZNEXT_SOURCEMAP_H
#define BLITZNEXT_SOURCEMAP_H

#include <string>
#include <unordered_map>
#include <vector>

// Maps a line of the preprocessed stream back to the file and line it came
// from (WEAK-13, the cause of BUG-14).
//
// The preprocessor concatenates every #Include into one text; without this
// table the lexer, parser and semantic pass all count lines in that stream, so
// every diagnostic after the first Include points at the wrong place. The
// table is filled while the text is assembled — one entry per emitted line, in
// emission order — which makes a lookup a plain index.
//
// File names are interned: a project of n lines across k files stores k strings
// and n small integers, not n copies of a path.
//
// An empty map is legal and means "no mapping known": lookups then report the
// main file with the stream line unchanged, which is the old single-file
// behaviour.

struct SourceLoc {
  std::string file;
  int         line;
};

class SourceMap {
public:
  void setMainFile(const std::string &f) { mainFile_ = f; }
  const std::string &mainFile() const { return mainFile_; }

  // Records the origin of the next line of the stream.
  void addLine(const std::string &file, int line) {
    auto it = fileIds_.find(file);
    if (it == fileIds_.end()) {
      it = fileIds_.emplace(file, files_.size()).first;
      files_.push_back(file);
    }
    origins_.push_back({it->second, line});
  }

  bool empty() const { return origins_.empty(); }

  // streamLine is 1-based, the way the lexer counts.
  SourceLoc lookup(int streamLine) const {
    if (origins_.empty() || streamLine < 1)
      return {mainFile_, streamLine};
    if (static_cast<size_t>(streamLine) <= origins_.size()) {
      const Origin &o = origins_[streamLine - 1];
      return {files_[o.fileId], o.line};
    }
    // Past the end — the EOF token sits one line beyond the last one. Keep
    // counting in whatever file the stream ended in.
    const Origin &last = origins_.back();
    return {files_[last.fileId],
            last.line + static_cast<int>(streamLine - origins_.size())};
  }

  // "file:line:col" — the GCC form every diagnostic in this compiler uses.
  std::string format(int streamLine, int col) const {
    SourceLoc loc = lookup(streamLine);
    return loc.file + ":" + std::to_string(loc.line) + ":" +
           std::to_string(col);
  }

private:
  struct Origin {
    size_t fileId;
    int    line;
  };

  std::string                             mainFile_;
  std::vector<std::string>                files_;
  std::unordered_map<std::string, size_t> fileIds_;
  std::vector<Origin>                     origins_;
};

#endif // BLITZNEXT_SOURCEMAP_H
