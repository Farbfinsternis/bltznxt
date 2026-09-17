#ifndef BLITZNEXT_PREPROCESSOR_H
#define BLITZNEXT_PREPROCESSOR_H

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "sourcemap.h"

class Preprocessor {
public:
  bool hasErrors() const { return errorCount_ > 0; }

  // `map` collects the origin of every emitted line (WEAK-13) and is the only
  // way a later pass can tell which file a line of the stream came from.
  std::string process(const std::string &path,
                      std::vector<std::string> &includedFiles,
                      SourceMap &map) {
    namespace fs = std::filesystem;

    // Every Include is relative to the main file's directory, also from a
    // file in a subfolder: blitzcc changes into that directory before it
    // parses, and "sub\a.bb" including "b.bb" next to itself is rejected
    // (BUG-151, measured 2026-09-17). The first call is the main file.
    if (!rootSet_) {
      rootDir_ = fs::path(path).parent_path();
      rootSet_ = true;
    }

    // Guard against circular includes
    std::string canonical;
    try {
      canonical = fs::canonical(path).string();
    } catch (...) {
      canonical = path; // file may not exist yet; use raw path as key
    }

    for (auto &f : includedFiles)
      if (f == canonical) return "";
    includedFiles.push_back(canonical);

    std::ifstream file(path);
    if (!file.is_open()) {
      std::cerr << "[Preprocessor] Cannot open: " << path << "\n";
      return "";
    }

    std::string lineText;
    std::string result;
    int         lineNo = 0;

    while (std::getline(file, lineText)) {
      ++lineNo;
      // Trim leading whitespace for keyword detection only
      size_t first = lineText.find_first_not_of(" \t");
      std::string trimmed = (first == std::string::npos) ? "" : lineText.substr(first);

      // Upper-case copy for case-insensitive keyword check
      std::string upper = trimmed;
      std::transform(upper.begin(), upper.end(), upper.begin(),
                     [](unsigned char c){ return (char)std::toupper(c); });

      // Both spellings are Blitz3D: "Include" and the documented "#Include".
      size_t kwStart = (!upper.empty() && upper[0] == '#') ? 1 : 0;

      // Match the INCLUDE keyword as a whole word (not e.g. INCLUDEFILES)
      if (upper.compare(kwStart, 7, "INCLUDE") == 0) {
        size_t afterKw = kwStart + 7; // past "INCLUDE"
        // Must be followed by whitespace or a quote — not another identifier char
        bool isWord = (afterKw >= upper.size()) ||
                      (!std::isalnum((unsigned char)upper[afterKw]) &&
                       upper[afterKw] != '_');
        if (isWord) {
          size_t q1 = trimmed.find('"');
          size_t q2 = (q1 != std::string::npos)
                          ? trimmed.find('"', q1 + 1)
                          : std::string::npos;
          if (q1 != std::string::npos && q2 != std::string::npos) {
            std::string incFile = trimmed.substr(q1 + 1, q2 - q1 - 1);
            // generic_string() keeps the separators uniform: rootDir_ may come
            // from a forward-slash command line while the concatenation adds a
            // native one, and a diagnostic path is what the IDE matches on.
            // Backslashes are Blitz3D's separator and must work here too.
            std::replace(incFile.begin(), incFile.end(), '\\', '/');
            std::string resolved =
                (rootDir_ / incFile).lexically_normal().generic_string();
            if (!fs::is_regular_file(resolved)) {
              // Blitz3D stops here: "Unable to open include file", placed
              // just past the closing quote.
              std::cerr << path << ":" << lineNo << ":" << (first + q2 + 2)
                        << ": error: Unable to open include file '"
                        << trimmed.substr(q1 + 1, q2 - q1 - 1) << "'\n";
              ++errorCount_;
              result += "\n";
              map.addLine(path, lineNo);
              continue;
            }
            // The recursion appends its lines — and its map entries —
            // right here, so text and table stay in step.
            result += process(resolved, includedFiles, map);
            result += "\n";
            // The blank line above stands in for the Include statement.
            map.addLine(path, lineNo);
            continue;
          }
        }
      }

      result += lineText + "\n";
      map.addLine(path, lineNo);
    }
    return result;
  }

private:
  std::filesystem::path rootDir_;
  bool rootSet_   = false;
  int  errorCount_ = 0;
};

#endif // BLITZNEXT_PREPROCESSOR_H
