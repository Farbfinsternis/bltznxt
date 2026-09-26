#ifndef BLITZNEXT_BB_NUMCONV_H
#define BLITZNEXT_BB_NUMCONV_H

// Die zwei Zahlwandlungen des Originals, die Runtime UND Compiler brauchen:
// die Runtime fuer Str/Print/Int, der Compiler fuer die Konstantenfaltung
// (BUG-99) - ein "Const c$ = 1.5" muss dasselbe ergeben wie "Str 1.5" zur
// Laufzeit. Deshalb nur Standardheader hier.

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <string>

// Kommazahl -> Zeichenkette wie ftoa() in stdutil/stdutil.cpp der Referenz
// (BUG-68). Sechs signifikante Stellen wie _ecvt. Liegt der Dezimalpunkt bei
// dec <= -3 oder dec > 8 (Wert unter 0.001 bzw. ab 10^9), schreibt das
// Original ueber MSVCs _gcvt: e-Schreibweise, ueberzaehlige Nullen weg, der
// Punkt bleibt stehen, Exponent mindestens dreistellig ("1.e+008",
// "1.23e-004"). Sonst Festkomma mit mindestens einer Nachkommastelle
// ("2.0", "1234570.0"). Am Original gemessen (build/str20260918): auch
// "NaN", "Infinity", "-Infinity", und -0.0 ergibt "0.0".
inline std::string bb_FloatToStr_(float n) {
    const int digits = 6;
    if (std::isnan(n)) return "NaN";
    if (std::isinf(n)) return n > 0 ? "Infinity" : "-Infinity";

    // _ecvt(n, 6): die sechs Ziffern und die Lage des Dezimalpunkts. MSVC
    // rundet dabei einen exakten Gleichstand vom Nullpunkt weg (38854.25 ->
    // "38854.3", 826052.5 -> "826053.0"), printf zur geraden Ziffer. Darum 30
    // exakte Stellen holen und selbst nach der siebten entscheiden - ein float
    // hat hoechstens 24 Bit, die siebte Ziffer ist damit sicher.
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.29e", std::fabs((double)n));
    std::string t;
    for (const char *c = buf; *c && *c != 'e'; ++c)
        if (*c != '.') t += *c;
    int dec = std::atoi(std::strchr(buf, 'e') + 1) + 1;
    const bool up = t[digits] >= '5';
    t.resize(digits);
    if (up) {
        int k = digits - 1;
        while (k >= 0 && t[k] == '9') t[k--] = '0';
        if (k >= 0) ++t[k];
        else { t = "1" + t.substr(0, digits - 1); ++dec; }
    }
    const bool neg = n < 0;  // -0.0 hat im Original kein Vorzeichen
    if (n == 0) dec = 0;     // _ecvt(0) meldet den Punkt vor der ersten Ziffer

    if (dec <= -3 || dec > 8) {
        // _gcvt(n, 6) in e-Schreibweise.
        std::string m = t.substr(0, 1) + "." + t.substr(1);
        while (m.back() == '0') m.pop_back();
        int e = dec - 1;
        char ex[16];
        std::snprintf(ex, sizeof(ex), "e%c%03d", e < 0 ? '-' : '+', e < 0 ? -e : e);
        return (neg ? "-" : "") + m + ex;
    }

    if (dec <= 0) {
        t = "0." + std::string(-dec, '0') + t;
        dec = 1;
    } else if (dec < digits) {
        t = t.substr(0, dec) + "." + t.substr(dec);
    } else {
        t = t + std::string(dec - digits, '0') + ".0";
        dec += dec - digits;
    }
    // Ueberzaehlige Nullen hinten abschneiden, eine Nachkommastelle bleibt.
    int dp1 = dec + 1, p = (int)t.length();
    while (--p > dp1 && t[p] == '0') {}
    t = t.substr(0, p + 1);
    return neg ? "-" + t : t;
}

// Float -> Int, wie das Original es an jeder Stelle der Sprache tut: CastNode
// und FloatConstNode (compiler/exprnode.cpp) wandeln mit fistp im
// x87-Vorgabemodus, also zur naechsten Ganzzahl, bei .5 zur geraden. Am
// Original gemessen (BUG-95): 2.5 -> 2, 3.5 -> 4, -2.5 -> -2, 1.9 -> 2, gleich
// ob Int(), Zuweisung, Parameter, Return, Vorgabe, Feld, Array, For oder Case.
// Nur Read schneidet ab - das laeuft ueber bb_DataVal, nicht hierueber.
inline int bb_FloatToInt_(double value) {
    // Blitz3D's numeric float type is 32 bit, including folded literals.
    double f = static_cast<float>(value);
    double lower = std::floor(f);
    double fraction = f - lower;
    double rounded = lower;
    if (fraction > 0.5 || (fraction == 0.5 && std::fmod(lower, 2.0) != 0.0))
        rounded += 1.0;
    // x87's integer indefinite for NaN, infinity and out-of-range results.
    if (!std::isfinite(rounded) || rounded < std::numeric_limits<int>::min() ||
        rounded > std::numeric_limits<int>::max())
        return std::numeric_limits<int>::min();
    return static_cast<int>(rounded);
}

#endif // BLITZNEXT_BB_NUMCONV_H
