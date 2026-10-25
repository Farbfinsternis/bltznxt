#ifndef BLITZNEXT_PREPROCESSOR_H
#define BLITZNEXT_PREPROCESSOR_H

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "ansipath.h"
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
    // Pfade sind ANSI-Bytes, std::filesystem bekommt sie nur ueber ansiPath
    // (BUG-201).
    if (!rootSet_) {
      rootDir_ = ansiPath(path).parent_path();
      rootSet_ = true;
    }

    // Guard against circular includes
    std::string canonical;
    try {
      canonical = ansiString(fs::canonical(ansiPath(path)));
    } catch (...) {
      canonical = path; // file may not exist yet; use raw path as key
    }

    // Windows-Dateinamen kennen keine Gross-/Kleinschreibung: "helper.bb" und
    // "HELPER.BB" sind dieselbe Datei und kommen wie im Original nur einmal
    // herein (BUG-94).
    std::transform(canonical.begin(), canonical.end(), canonical.begin(),
                   [](unsigned char c){ return (char)std::tolower(c); });
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
      // Include ist im Original eine Anweisung wie jede andere (parser.cpp):
      // "Print 1 : Include "a.bb" : Print 2" fuehrt alle drei aus. Die Zeile
      // wird deshalb an den Doppelpunkten ausserhalb von Strings und vor dem
      // Kommentar geteilt; bis BUG-94 zaehlte nur ein Include am Zeilenanfang,
      // der Rest der Zeile ging verloren, und eines hinter einem Doppelpunkt
      // wurde abgelehnt.
      std::vector<std::pair<size_t, size_t>> segs;   // [Anfang, Ende)
      {
        size_t start = 0, stop = lineText.size();
        bool inStr = false;
        for (size_t i = 0; i < lineText.size(); ++i) {
          const char c = lineText[i];
          if (c == '"') inStr = !inStr;
          else if (!inStr && c == ';') { stop = i; break; }
          else if (!inStr && c == ':') { segs.push_back({start, i}); start = i + 1; }
        }
        segs.push_back({start, stop});
      }
      // Ist der Abschnitt ein Include? Dann die Lage der Anfuehrungszeichen.
      auto includeIn = [&lineText](std::pair<size_t, size_t> sg, size_t &q1, size_t &q2) {
        const size_t a = lineText.find_first_not_of(" \t", sg.first);
        if (a == std::string::npos || a >= sg.second) return false;
        size_t k = a + (lineText[a] == '#' ? 1 : 0);   // "#Include" auch
        if (k + 7 > sg.second) return false;
        std::string kw = lineText.substr(k, 7);
        std::transform(kw.begin(), kw.end(), kw.begin(),
                       [](unsigned char c){ return (char)std::toupper(c); });
        if (kw != "INCLUDE") return false;
        // ein ganzes Wort, nicht etwa INCLUDEFILES
        if (k + 7 < sg.second && (std::isalnum((unsigned char)lineText[k + 7]) ||
                                  lineText[k + 7] == '_'))
          return false;
        q1 = lineText.find('"', k + 7);
        if (q1 == std::string::npos || q1 >= sg.second) return false;
        q2 = lineText.find('"', q1 + 1);
        return q2 != std::string::npos && q2 < sg.second;
      };

      bool anyInclude = false;
      for (auto &sg : segs) {
        size_t a, b;
        if (includeIn(sg, a, b)) { anyInclude = true; break; }
      }
      if (!anyInclude) {
        result += lineText + "\n";
        map.addLine(path, lineNo);
        continue;
      }

      // Die Abschnitte zwischen den Includes bleiben Zeilen dieser Quellzeile,
      // vorn mit Leerzeichen aufgefuellt, damit die Spalten in Meldungen
      // stimmen.
      size_t runA = std::string::npos, runB = 0;
      auto flush = [&]() {
        if (runA == std::string::npos) return;
        const std::string text = lineText.substr(runA, runB - runA);
        if (text.find_first_not_of(" \t") != std::string::npos) {
          result += std::string(runA, ' ') + text + "\n";
          map.addLine(path, lineNo);
        }
        runA = std::string::npos;
      };
      for (auto &sg : segs) {
        size_t q1, q2;
        if (!includeIn(sg, q1, q2)) {
          if (runA == std::string::npos) runA = sg.first;
          runB = sg.second;
          continue;
        }
        flush();
        std::string incFile = lineText.substr(q1 + 1, q2 - q1 - 1);
        // generic_string() keeps the separators uniform: rootDir_ may come
        // from a forward-slash command line while the concatenation adds a
        // native one, and a diagnostic path is what the IDE matches on.
        // Backslashes are Blitz3D's separator and must work here too.
        std::replace(incFile.begin(), incFile.end(), '\\', '/');
        std::string resolved =
            ansiGeneric((rootDir_ / ansiPath(incFile)).lexically_normal());
        if (!fs::is_regular_file(ansiPath(resolved))) {
          // Blitz3D stops here: "Unable to open include file", placed
          // just past the closing quote.
          std::cerr << path << ":" << lineNo << ":" << (q2 + 2)
                    << ": error: Unable to open include file '"
                    << lineText.substr(q1 + 1, q2 - q1 - 1) << "'\n";
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
      }
      flush();
    }
    return result;
  }

private:
  std::filesystem::path rootDir_;
  bool rootSet_   = false;
  int  errorCount_ = 0;
};

#endif // BLITZNEXT_PREPROCESSOR_H
