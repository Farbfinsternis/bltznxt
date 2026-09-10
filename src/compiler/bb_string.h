#ifndef BLITZNEXT_BB_STRING_H
#define BLITZNEXT_BB_STRING_H

#include <iostream>
#include <string>
#include <algorithm>
#include <cctype>   // toupper, tolower
#include <cstdio>   // snprintf
#include <cstdlib>  // atoi, atof (String -> Zahl wie in der Referenz)

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

// In Blitz3D a "+" with a string on either side makes the whole expression a
// string and casts the other side to string (compiler/exprnode.cpp,
// ArithExprNode::semant). These overloads give the generated C++ the same rule,
// so "Score: " + n behaves as it does in Blitz3D. Vergleiche folgen derselben
// Regel und stehen gleich darunter (BUG-79); die uebrigen Operatoren bleiben
// auf Zeichenketten mit Absicht unaufgeloest — Blitz3D lehnt sie ebenfalls ab.
inline bbString operator+(const bbString &s, int n)    { return s + bb_Str(n); }
inline bbString operator+(int n, const bbString &s)    { return bb_Str(n) + s; }
inline bbString operator+(const bbString &s, float f)  { return s + bb_Str((double)f); }
inline bbString operator+(float f, const bbString &s)  { return bb_Str((double)f) + s; }
inline bbString operator+(const bbString &s, double f) { return s + bb_Str(f); }
inline bbString operator+(double f, const bbString &s) { return bb_Str(f) + s; }

// ---- Vergleiche mit gemischten Typen (BUG-79) ------------------------------
//
// Steht auf EINER Seite eines Vergleichs eine Zeichenkette, so vergleicht
// Blitz3D BEIDE Seiten als Zeichenkette. Die Referenz bestimmt dafuer in
// RelExprNode::semant (compiler/exprnode.cpp) einen gemeinsamen Vergleichstyp
// und castet beide Seiten darauf: String schlaegt Float schlaegt Int, also
// dieselbe Stufenfolge wie bei der Arithmetik.
//
// Am laufenden Original gemessen (2026-09-10), weil die Richtung sonst leicht
// falsch herum geraet: 9 < "10" ist FALSCH und "9" > 10 ist WAHR - verglichen
// wird "9" gegen "10", nicht 9 gegen 10. Ebenso ist 10 = "10.0" falsch und
// 2.0 < "12" falsch. Verglichen wird zeichenweise, mit Unterscheidung von
// Gross und Klein ("A" < "a") und vorzeichenlos (Chr(200) > "A") - beides tut
// std::string von sich aus.
//
// Wie bei den operator+ darueber steht die Regel in der Runtime und nicht im
// Emitter: der Emitter kennt den Typ eines Ausdrucks nicht, die
// Ueberladungsaufloesung schon. Der erzeugte C++-Text bleibt dadurch Zeichen
// fuer Zeichen derselbe; es uebersetzt nur, was bisher gar nicht uebersetzte.
//
// Umgewandelt wird ueber dasselbe bb_Str() wie beim Verketten. Damit stimmt
// der Vergleich mit unserer eigenen Ausgabe ueberein, und er wird zusammen mit
// BUG-68 richtig: das Original schreibt Str(1.0) als "1.0", wir als "1",
// weshalb 1.0 = "1" bei uns wahr und im Original falsch ist. Das ist die
// bekannte Abweichung der Zahlformatierung, kein zweiter Befund.

inline bool operator==(const bbString &s, int n)    { return s == bb_Str(n); }
inline bool operator==(int n, const bbString &s)    { return bb_Str(n) == s; }
inline bool operator==(const bbString &s, float f)  { return s == bb_Str((double)f); }
inline bool operator==(float f, const bbString &s)  { return bb_Str((double)f) == s; }
inline bool operator==(const bbString &s, double f) { return s == bb_Str(f); }
inline bool operator==(double f, const bbString &s) { return bb_Str(f) == s; }

inline bool operator!=(const bbString &s, int n)    { return s != bb_Str(n); }
inline bool operator!=(int n, const bbString &s)    { return bb_Str(n) != s; }
inline bool operator!=(const bbString &s, float f)  { return s != bb_Str((double)f); }
inline bool operator!=(float f, const bbString &s)  { return bb_Str((double)f) != s; }
inline bool operator!=(const bbString &s, double f) { return s != bb_Str(f); }
inline bool operator!=(double f, const bbString &s) { return bb_Str(f) != s; }

inline bool operator< (const bbString &s, int n)    { return s <  bb_Str(n); }
inline bool operator< (int n, const bbString &s)    { return bb_Str(n) <  s; }
inline bool operator< (const bbString &s, float f)  { return s <  bb_Str((double)f); }
inline bool operator< (float f, const bbString &s)  { return bb_Str((double)f) <  s; }
inline bool operator< (const bbString &s, double f) { return s <  bb_Str(f); }
inline bool operator< (double f, const bbString &s) { return bb_Str(f) <  s; }

inline bool operator> (const bbString &s, int n)    { return s >  bb_Str(n); }
inline bool operator> (int n, const bbString &s)    { return bb_Str(n) >  s; }
inline bool operator> (const bbString &s, float f)  { return s >  bb_Str((double)f); }
inline bool operator> (float f, const bbString &s)  { return bb_Str((double)f) >  s; }
inline bool operator> (const bbString &s, double f) { return s >  bb_Str(f); }
inline bool operator> (double f, const bbString &s) { return bb_Str(f) >  s; }

inline bool operator<=(const bbString &s, int n)    { return s <= bb_Str(n); }
inline bool operator<=(int n, const bbString &s)    { return bb_Str(n) <= s; }
inline bool operator<=(const bbString &s, float f)  { return s <= bb_Str((double)f); }
inline bool operator<=(float f, const bbString &s)  { return bb_Str((double)f) <= s; }
inline bool operator<=(const bbString &s, double f) { return s <= bb_Str(f); }
inline bool operator<=(double f, const bbString &s) { return bb_Str(f) <= s; }

inline bool operator>=(const bbString &s, int n)    { return s >= bb_Str(n); }
inline bool operator>=(int n, const bbString &s)    { return bb_Str(n) >= s; }
inline bool operator>=(const bbString &s, float f)  { return s >= bb_Str((double)f); }
inline bool operator>=(float f, const bbString &s)  { return bb_Str((double)f) >= s; }
inline bool operator>=(const bbString &s, double f) { return s >= bb_Str(f); }
inline bool operator>=(double f, const bbString &s) { return bb_Str(f) >= s; }

// ---- Implizite Umwandlung an Zuweisungsgrenzen (BUG-53) --------------------
//
// Blitz3D wandelt zwischen Integer, Float und String in BEIDE Richtungen um,
// wo ein Wert an einen bekannten Zieltyp geht: Zuweisung, Deklaration mit
// Initialisierung, Funktionsparameter und Return. Am Original gemessen sind
// alle sechs Richtungen an allen vier Stellen zulaessig; nur Objekte wandeln
// nie ("Illegal type conversion").
//
// Der Emitter kennt den ZIELTYP (aus der Deklaration bzw. der Signatur), aber
// nicht den Typ des Quellausdrucks. Deshalb sind diese Helfer ueber alle
// Quelltypen ueberladen, einschliesslich eines Durchreichers fuer den Fall,
// dass gar nichts umzuwandeln ist: der Emitter darf bedenkenlos wrappen, die
// Ueberladungsaufloesung entscheidet. Dieselbe Bauform wie die operator+ oben.
inline const bbString &bb_Str(const bbString &s) { return s; }

// String -> Zahl folgt der Referenz, die dafuer atoi/atof benutzt: fuehrender
// Zahlanteil zaehlt, der Rest wird ignoriert, gar keine Ziffer ergibt 0. Kein
// Fehler, keine Ausnahme - anders als bb_Int(bbString) fuer das Sprach-Int().
inline int bb_ToInt(const bbString &s) { return std::atoi(s.c_str()); }
inline int bb_ToInt(int n)             { return n; }
// Float -> Int bleibt hier bewusst das, was der erzeugte C++-Code bisher schon
// tat (Abschneiden). Dass Blitz3D stattdessen rundet, ist eine eigene, bereits
// notierte Abweichung; sie hier stillschweigend mitzuaendern wuerde den Befund
// verwischen und mehr aendern, als dieser Fix verantwortet.
inline int bb_ToInt(float f)           { return static_cast<int>(f); }
inline int bb_ToInt(double f)          { return static_cast<int>(f); }

inline float bb_ToFloat(const bbString &s) { return (float)std::atof(s.c_str()); }
inline float bb_ToFloat(int n)             { return (float)n; }
inline float bb_ToFloat(float f)           { return f; }
inline float bb_ToFloat(double f)          { return (float)f; }

// Eine Schleifengrenze ist KEIN Vergleich in diesem Sinn, obwohl der Emitter
// dafuer "<=" schreibt. Am Original gemessen (2026-09-10): "For i = 1 To '10'"
// laeuft zehnmal, waehrend "10" > "9" ein Zeichenkettenvergleich ist - die
// Grenze wird also zur Zahl gewandelt, nicht der Zaehler zur Zeichenkette.
// Gemessen ist auch, dass sie zur KOMMAZAHL wird: "To '3.7'" laeuft dreimal,
// "To 3.7" dagegen viermal, weil eine konstante Kommazahl auf den Zaehlertyp
// gerundet wird. Ein gewandelter String nimmt diesen Weg nicht.
//
// Ohne diesen Helfer haetten die Vergleichsueberladungen weiter oben aus einem
// lauten Uebersetzungsfehler ein stilles Falschergebnis gemacht: "For i = 1
// To '10'" haette einmal statt zehnmal gelaufen, weil "2" <= "10" falsch ist.
// Fuer Zahlen ist er ein reiner Durchreicher und aendert nichts - auch nicht
// den Typ, weshalb hier kein bb_ToFloat(int) steht.
inline int    bb_ToNum(int n)              { return n; }
inline float  bb_ToNum(float f)            { return f; }
inline double bb_ToNum(double f)           { return f; }
inline float  bb_ToNum(const bbString &s)  { return bb_ToFloat(s); }

// Ein "Case" ist ebenfalls kein Vergleich im Sinn der Ueberladungen oben,
// obwohl der Emitter dafuer "==" schreibt. Am Original gemessen (2026-09-10):
// der Case-Wert wird auf den Typ des SELECT-Ausdrucks gewandelt, nicht auf
// einen gemeinsamen Typ beider Seiten.
//   Select 10   : Case "010"  trifft   (Zeichenkette -> Zahl, atoi)
//   Select "9.0": Case 9      trifft NICHT ("9" gegen "9.0")
//   Select 1    : Case "1.7"  trifft   (atoi schneidet ab, es wird nicht gerundet)
//   Select 1.5# : Case "1.5"  trifft
//   Select "abc": Case 0      trifft NICHT
// Die dritte Zeile ist der Beleg, dass hier atoi und keine Rundung wirkt; die
// zweite, dass die Richtung am SELECT haengt und nicht am staerkeren Typ.
//
// Ohne diesen Helfer haetten die Vergleichsueberladungen "Select 10 : Case
// '010'" still am Default vorbeigefuehrt - vorher scheiterte diese Form
// wenigstens laut an C++. Zahl gegen Zahl bleibt Zeichen fuer Zeichen das,
// was der erzeugte Code vorher schon rechnete.
inline bool bb_CaseEq(int s, int c)                        { return s == c; }
inline bool bb_CaseEq(int s, float c)                      { return s == c; }
inline bool bb_CaseEq(int s, double c)                     { return s == c; }
inline bool bb_CaseEq(float s, int c)                      { return s == c; }
inline bool bb_CaseEq(float s, float c)                    { return s == c; }
inline bool bb_CaseEq(float s, double c)                   { return s == c; }
inline bool bb_CaseEq(double s, int c)                     { return s == c; }
inline bool bb_CaseEq(double s, float c)                   { return s == c; }
inline bool bb_CaseEq(double s, double c)                  { return s == c; }
inline bool bb_CaseEq(int s, const bbString &c)            { return s == bb_ToInt(c); }
inline bool bb_CaseEq(float s, const bbString &c)          { return s == bb_ToFloat(c); }
inline bool bb_CaseEq(double s, const bbString &c)         { return (float)s == bb_ToFloat(c); }
inline bool bb_CaseEq(const bbString &s, int c)            { return s == bb_Str(c); }
inline bool bb_CaseEq(const bbString &s, float c)          { return s == bb_Str((double)c); }
inline bool bb_CaseEq(const bbString &s, double c)         { return s == bb_Str(c); }
inline bool bb_CaseEq(const bbString &s, const bbString &c){ return s == c; }

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
