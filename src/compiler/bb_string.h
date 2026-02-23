#ifndef BLITZNEXT_BB_STRING_H
#define BLITZNEXT_BB_STRING_H

#include <iostream>
#include <string>
#include <algorithm>
#include <cctype>   // toupper, tolower
#include <cstdio>   // snprintf

// ============================================================
//  BlitzNext String Runtime  —  bb_string.h
//
//  Blitz3D string indices are 1-based throughout.
// ============================================================

typedef std::string bbString;

// ---- String conversion ----

inline bbString bb_Str(int n)    { return std::to_string(n); }
inline bbString bb_Str(double f) {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%g", f);
    return buf;
}

inline int bb_Int(const bbString &s) {
    try { return std::stoi(s); }
    catch (...) {
        std::cerr << "[runtime] Int(): invalid value \"" << s << "\"\n";
        return 0;
    }
}
inline float bb_Float(const bbString &s) {
    try { return std::stof(s); }
    catch (...) {
        std::cerr << "[runtime] Float(): invalid value \"" << s << "\"\n";
        return 0.0f;
    }
}
inline int      bb_Len(const bbString &s)   { return static_cast<int>(s.size()); }

// ---- Extraction ----

// Left(s, n) — first n characters
inline bbString bb_Left(const bbString &s, int n) {
    if (n <= 0) return "";
    return s.substr(0, std::min(n, (int)s.size()));
}

// Right(s, n) — last n characters
inline bbString bb_Right(const bbString &s, int n) {
    if (n <= 0) return "";
    int len = (int)s.size();
    return s.substr(std::max(0, len - n));
}

// Mid(s, pos) — from 1-based pos to end of string
inline bbString bb_Mid(const bbString &s, int pos) {
    if (pos < 1) pos = 1;
    int idx = pos - 1;
    if (idx >= (int)s.size()) return "";
    return s.substr(idx);
}

// Mid(s, pos, n) — n characters starting at 1-based pos
inline bbString bb_Mid(const bbString &s, int pos, int n) {
    if (pos < 1) pos = 1;
    if (n <= 0) return "";
    int idx = pos - 1;
    if (idx >= (int)s.size()) return "";
    return s.substr(idx, n);
}

// ---- Search ----

// Instr(s, sub) — 1-based index of first occurrence, or 0
inline int bb_Instr(const bbString &s, const bbString &sub) {
    auto pos = s.find(sub);
    return pos == bbString::npos ? 0 : (int)pos + 1;
}

// Instr(s, sub, start) — search from 1-based start position
inline int bb_Instr(const bbString &s, const bbString &sub, int start) {
    if (start < 1) start = 1;
    auto pos = s.find(sub, (size_t)(start - 1));
    return pos == bbString::npos ? 0 : (int)pos + 1;
}

// ---- Replace ----

// Replace(s, from, to) — replace all occurrences of from with to
inline bbString bb_Replace(bbString s, const bbString &from, const bbString &to) {
    if (from.empty()) return s;
    size_t pos = 0;
    while ((pos = s.find(from, pos)) != bbString::npos) {
        s.replace(pos, from.size(), to);
        pos += to.size();
    }
    return s;
}

// ---- Transformation ----

// Upper(s) / Lower(s) — case conversion
inline bbString bb_Upper(bbString s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c){ return (char)std::toupper(c); });
    return s;
}

inline bbString bb_Lower(bbString s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c){ return (char)std::tolower(c); });
    return s;
}

// Trim(s) — strip leading and trailing whitespace
inline bbString bb_Trim(const bbString &s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == bbString::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

// LSet(s, n) — left-aligned: pad with spaces on right, or truncate to n chars
inline bbString bb_LSet(const bbString &s, int n) {
    if (n <= 0) return "";
    if ((int)s.size() >= n) return s.substr(0, n);
    return s + bbString(n - (int)s.size(), ' ');
}

// RSet(s, n) — right-aligned: pad with spaces on left, or truncate to n chars
inline bbString bb_RSet(const bbString &s, int n) {
    if (n <= 0) return "";
    if ((int)s.size() >= n) return s.substr(0, n);
    return bbString(n - (int)s.size(), ' ') + s;
}

// ---- Character encoding ----

// Chr(n) — single-character string from ASCII code
inline bbString bb_Chr(int n) { return bbString(1, static_cast<char>(n & 0xFF)); }

// Asc(s) — ASCII code of first character, or 0 for empty string
inline int bb_Asc(const bbString &s) {
    return s.empty() ? 0 : static_cast<unsigned char>(s[0]);
}

// ---- Numeric encoding ----

// Hex(n) — uppercase hex string, no prefix; negative treated as unsigned 32-bit
inline bbString bb_Hex(int n) {
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%X", static_cast<unsigned int>(n));
    return buf;
}

// Bin(n) — binary string, no prefix, no leading zeros (minimum "0")
inline bbString bb_Bin(int n) {
    unsigned int u = static_cast<unsigned int>(n);
    if (u == 0) return "0";
    bbString result;
    while (u > 0) {
        result = (char)('0' + (u & 1)) + result;
        u >>= 1;
    }
    return result;
}

// String(s, n) — repeat string s n times
inline bbString bb_String(const bbString &s, int n) {
    bbString result;
    result.reserve(s.size() * (size_t)std::max(0, n));
    for (int i = 0; i < n; ++i) result += s;
    return result;
}

#endif // BLITZNEXT_BB_STRING_H
