#pragma once
//
// "Did you mean ...?" for a name the compiler does not know (WEAK-14, Stufe 2).
//
// Used for three kinds of name: a command or function (blitzcc.cpp), a Type,
// and the field of a Type (semant.h). The rules are the same everywhere, so
// they live here once rather than in each place that reports such a name.

#include "lexer.h" // toUpper
#include <algorithm>
#include <string>
#include <vector>

// Edit distance between two names, counting a swap of two neighbouring letters
// as one edit and not two ("Lne" is one swap away from "Len"). That is the
// common typo, and without it a swap loses against an unrelated name that
// happens to be one insertion away. Capped: once every value in a row exceeds
// "limit" the result cannot come back below it, so the rest of the table is not
// worth filling in.
inline size_t editDistance(const std::string &a, const std::string &b,
                           size_t limit) {
  if (a.size() > b.size() + limit || b.size() > a.size() + limit)
    return limit + 1;
  const size_t inf = limit + 1;
  std::vector<size_t> prev2(b.size() + 1, inf), prev(b.size() + 1),
                      cur(b.size() + 1);
  for (size_t j = 0; j <= b.size(); ++j) prev[j] = j;
  for (size_t i = 1; i <= a.size(); ++i) {
    cur[0] = i;
    size_t best = cur[0];
    for (size_t j = 1; j <= b.size(); ++j) {
      size_t cost = (a[i - 1] == b[j - 1]) ? 0 : 1;
      cur[j] = std::min({cur[j - 1] + 1, prev[j] + 1, prev[j - 1] + cost});
      if (i > 1 && j > 1 && a[i - 1] == b[j - 2] && a[i - 2] == b[j - 1])
        cur[j] = std::min(cur[j], prev2[j - 2] + 1); // swapped neighbours
      best = std::min(best, cur[j]);
    }
    if (best > limit) return limit + 1;
    prev2.swap(prev);
    prev.swap(cur);
  }
  return prev[b.size()];
}

// The closest known name to "name", or "" when nothing is close enough. The
// tolerance grows with the length of the typo - one edit in a short name is a
// different thing than one edit in "CreateListener". Candidates are compared
// uppercased (Blitz3D is case-insensitive) but suggested in the spelling the
// table or the declaration uses.
inline std::string closestName(const std::string &name,
                               const std::vector<std::string> &candidates) {
  const std::string up = toUpper(name);
  size_t limit = up.size() <= 3 ? 1 : up.size() <= 7 ? 2 : 3;
  // The tolerance must stay below the length of the typo itself: at one
  // character every other single letter is one edit away, and "did you mean
  // 'x'?" for a field named y is noise, not help.
  if (up.size() < 2) return std::string();
  limit = std::min(limit, up.size() - 1);
  std::string best;
  size_t bestDist = limit + 1;
  for (const auto &cand : candidates) {
    size_t d = editDistance(up, toUpper(cand), limit);
    // Strictly better only: on a tie the first candidate wins, and the caller
    // passes them in a fixed order, so the message is reproducible.
    if (d < bestDist) { bestDist = d; best = cand; }
  }
  return bestDist <= limit ? best : std::string();
}

// The message tail for a name that has a near miss, empty when it has none.
inline std::string didYouMean(const std::string &name,
                              const std::vector<std::string> &candidates) {
  std::string hint = closestName(name, candidates);
  return hint.empty() ? std::string() : " - did you mean '" + hint + "'?";
}
