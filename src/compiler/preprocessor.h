#ifndef BLITZNEXT_PREPROCESSOR_H
#define BLITZNEXT_PREPROCESSOR_H

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

class Preprocessor {
public:
  std::string process(const std::string &path,
                      std::vector<std::string> &includedFiles) {
    namespace fs = std::filesystem;

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

    // Base directory of the current file — used to resolve relative includes
    fs::path baseDir = fs::path(path).parent_path();

    std::string lineText;
    std::string result;

    while (std::getline(file, lineText)) {
      // Trim leading whitespace for keyword detection only
      size_t first = lineText.find_first_not_of(" \t");
      std::string trimmed = (first == std::string::npos) ? "" : lineText.substr(first);

      // Upper-case copy for case-insensitive keyword check
      std::string upper = trimmed;
      std::transform(upper.begin(), upper.end(), upper.begin(),
                     [](unsigned char c){ return (char)std::toupper(c); });

      // Match the INCLUDE keyword as a whole word (not e.g. INCLUDEFILES)
      if (upper.find("INCLUDE") == 0) {
        size_t afterKw = 7; // length of "INCLUDE"
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
            // Resolve relative to the including file's directory
            fs::path resolved = baseDir / incFile;
            result += process(resolved.string(), includedFiles);
            result += "\n";
            continue;
          }
        }
      }

      result += lineText + "\n";
    }
    return result;
  }
};

#endif // BLITZNEXT_PREPROCESSOR_H
