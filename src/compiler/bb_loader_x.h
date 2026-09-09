#ifndef BLITZNEXT_BB_LOADER_X_H
#define BLITZNEXT_BB_LOADER_X_H

#include "bb_mesh.h"
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

// ============================================================
//  DirectX-Netze  -  bb_loader_x.h   (3D-13, Teil 3)
//
//  `.x` ist das haeufigste Modellformat der mitgelieferten Beispiele: sie
//  laden 36 verschiedene Dateien, davon **28 im Textformat und 8 binaer**
//  (ueber die ganze Installation gezaehlt: 43 Text-, 8 Binaerdateien).
//  Beide Kodierungen liest dieser Leser; sie unterscheiden sich nur darin,
//  wie die Marken auf die Datei kommen. Der Objektbaum darueber und alle
//  Bedeutungsregeln weiter unten gelten fuer beide.
//
//  **Das Original parst .x nicht selbst.** `blitz3d/loader_x.cpp` uebergibt
//  die ganze syntaktische Schicht an `d3dxof.dll`
//  (`DirectXFileCreate`, `RegisterTemplates(D3DRM_XTEMPLATES)`) und laeuft
//  anschliessend nur den Objektbaum nach GUIDs ab. Den Parser gibt es hier
//  also nicht zu uebernehmen, wohl aber die **Bedeutung** - und die ist dem
//  Quelltext entnommen, nicht geraten:
//
//  * `MeshTextureCoords` und `MeshNormals` gelten **nur, wenn ihre Anzahl
//    genau der Vertexzahl entspricht**, und liegen dann in Vertexreihenfolge
//    vor. Der eigene Flaechenindex von `MeshNormals` wird nicht ausgewertet.
//    Es ist also nichts zu verschweissen - anders als bei den meisten
//    .x-Lesern.
//  * **Die v-Koordinate wird nicht gespiegelt.** Bei `.3ds` rechnet das
//    Original `1-v`, hier uebernimmt es `tu`/`tv` unveraendert. Wer die
//    Regel vom einen Format aufs andere uebertraegt, dreht jede Textur um.
//  * Fehlen brauchbare Normalen, werden sie berechnet (`updateNormals`).
//  * `Material` sind vier Farbwerte, danach Glanz und zwei weitere Farben.
//    Die **Deckkraft gilt nur, wenn sie ungleich 0 ist** (`if( data[3] )`),
//    und ein `TextureFilename` setzt die Farbe auf weiss zurueck - dieselbe
//    Regel wie bei `.3ds`.
//  * Flaechen sind **Vielecke** und werden als Faecher ab dem ersten Punkt
//    zerlegt; kehrt die Loadermatrix die Haendigkeit um, tauschen dabei die
//    beiden hinteren Ecken.
//  * `MeshMaterialList` darf seine Materialien auch als **Verweis** auf ein
//    frueher benanntes `Material` fuehren.
//
//  **Frame-Transformationen wirken.** `LoadMesh` verwirft laut Doku die
//  Hierarchie - aber nicht die Lage: die Matrizen werden beim Einschmelzen in
//  die Vertices gerechnet. Am Original nachgemessen, weil der Quelltext das
//  ueber mehrere Dateien verteilt: `plane.x` misst mit angewandten Frames
//  19.8346 x 4.4206 x 13.5502 und ohne sie 65.07 x 14.50 x 44.46; das
//  Original meldet 19.8346 x 4.42056 x 13.5502. Bei `ladders.X` genauso.
//  Die Matrix steht zeilenweise und wirkt auf Zeilenvektoren (v' = v * M),
//  das Kind vor dem Elternteil.
// ============================================================

// ---- Zerteilen ----

enum bb_xtok_ { BB_XT_END, BB_XT_NAME, BB_XT_NUM, BB_XT_STR, BB_XT_SYM };

struct bb_XLexer_ {
  const char* p;
  const char* e;
  bb_xtok_    kind = BB_XT_END;
  std::string text;
  double      num = 0;

  // Nur fuer die Binaerkodierung
  bool     bin        = false;   // Marken statt Zeichen
  int      fsize      = 32;      // Groesse einer Kommazahl in der Datei
  unsigned list_left  = 0;       // noch offene Elemente einer Zahlenliste
  bool     list_float = false;

  void skip() {
    for (;;) {
      while (p < e && (unsigned char)*p <= ' ') ++p;
      // Kommentare: '#' und '//' bis Zeilenende
      if (p < e && (*p == '#' || (*p == '/' && p + 1 < e && p[1] == '/'))) {
        while (p < e && *p != '\n') ++p;
        continue;
      }
      // GUIDs interessieren uns nicht
      if (p < e && *p == '<') {
        while (p < e && *p != '>') ++p;
        if (p < e) ++p;
        continue;
      }
      return;
    }
  }

  bool next() { return bin ? next_bin() : next_txt(); }

  bool next_txt() {
    skip();
    text.clear();
    if (p >= e) { kind = BB_XT_END; return false; }
    char c = *p;
    if (c == '{' || c == '}' || c == ';' || c == ',') {
      kind = BB_XT_SYM; text.assign(1, c); ++p; return true;
    }
    if (c == '"') {
      ++p;
      const char* s = p;
      while (p < e && *p != '"') ++p;
      text.assign(s, p);
      if (p < e) ++p;
      kind = BB_XT_STR; return true;
    }
    if (c == '-' || c == '.' || (c >= '0' && c <= '9')) {
      const char* s = p;
      if (*p == '-') ++p;
      while (p < e && ((*p >= '0' && *p <= '9') || *p == '.')) ++p;
      if (p < e && (*p == 'e' || *p == 'E')) {
        ++p;
        if (p < e && (*p == '+' || *p == '-')) ++p;
        while (p < e && *p >= '0' && *p <= '9') ++p;
      }
      text.assign(s, p);
      num = std::atof(text.c_str());
      kind = BB_XT_NUM; return true;
    }
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_') {
      const char* s = p;
      while (p < e && (((*p >= 'A' && *p <= 'Z') || (*p >= 'a' && *p <= 'z') ||
                        (*p >= '0' && *p <= '9') || *p == '_' || *p == '-'))) ++p;
      text.assign(s, p);
      kind = BB_XT_NAME; return true;
    }
    ++p;                       // Unbekanntes Zeichen ueberlesen
    return next_txt();
  }

  // ---- Binaerkodierung ----
  //
  // Dieselben Tokenarten, andere Kodierung: ein Strom von 16-Bit-Marken, an
  // denen je nach Marke Daten haengen. Der Objektbaum darueber bleibt
  // derselbe, deshalb liest bb_x_parse_obj_ beide Formate ohne Aenderung.
  //
  // Das Original parst .x gar nicht selbst, sondern gibt an d3dxof.dll ab -
  // im Quelltext ist zum Binaerformat also nichts zu holen. Die Marken sind
  // stattdessen an den acht Binaerdateien der Installation nachgemessen: mit
  // dieser Tabelle endet der Tokenstrom bei allen acht **genau** am
  // Dateiende, die Klammern sind ausgeglichen und keine unbekannte Marke
  // bleibt uebrig.
  //
  // Eine Falle dabei: nach einem Text steht **kein** abschliessendes Wort.
  // Wer dort vier Byte verbraucht, verschluckt die beiden folgenden Marken -
  // in der Praxis ein ';' und ein '}'. Aufgefallen ist das daran, dass die
  // Klammerbilanz je Datei genau um die Zahl der Texte danebenlag (4, 1, 1
  // und 20).

  bool bin_u16_(unsigned& v) {
    if (e - p < 2) return false;
    v = (unsigned)(unsigned char)p[0] | ((unsigned)(unsigned char)p[1] << 8);
    p += 2;
    return true;
  }

  bool bin_u32_(unsigned& v) {
    if (e - p < 4) return false;
    v = (unsigned)(unsigned char)p[0]         | ((unsigned)(unsigned char)p[1] << 8)
      | ((unsigned)(unsigned char)p[2] << 16) | ((unsigned)(unsigned char)p[3] << 24);
    p += 4;
    return true;
  }

  bool bin_sym_(const char* s) { kind = BB_XT_SYM;  text = s; return true; }
  bool bin_name_(const char* s) { kind = BB_XT_NAME; text = s; return true; }

  bool next_bin() {
    for (;;) {
      text.clear();

      // Eine angefangene Zahlenliste Element fuer Element ausgeben.
      if (list_left) {
        --list_left;
        if (list_float) {
          if (fsize == 64) {
            double d;
            if (e - p < 8) break;
            std::memcpy(&d, p, 8); p += 8; num = d;
          } else {
            float f;
            if (e - p < 4) break;
            std::memcpy(&f, p, 4); p += 4; num = (double)f;
          }
        } else {
          unsigned v;
          if (!bin_u32_(v)) break;
          num = (double)(int)v;
        }
        kind = BB_XT_NUM;
        return true;
      }

      unsigned t;
      if (!bin_u16_(t)) break;
      switch (t) {
        case 1: case 2: {                       // Name, Text
          unsigned n;
          if (!bin_u32_(n)) break;
          if ((unsigned)(e - p) < n) break;
          text.assign(p, p + n); p += n;
          kind = (t == 1) ? BB_XT_NAME : BB_XT_STR;
          return true;
        }
        case 3: {                               // einzelne ganze Zahl
          unsigned v;
          if (!bin_u32_(v)) break;
          num = (double)(int)v;
          kind = BB_XT_NUM;
          return true;
        }
        case 5:                                 // GUID - interessiert uns nicht
          if (e - p < 16) break;
          p += 16;
          continue;
        case 6: case 7:                         // Liste ganzer bzw. Kommazahlen
          if (!bin_u32_(list_left)) break;
          list_float = (t == 7);
          continue;                             // eine leere Liste faellt durch
        case 10: return bin_sym_("{");
        case 11: return bin_sym_("}");
        case 12: return bin_sym_("(");
        case 13: return bin_sym_(")");
        case 14: return bin_sym_("[");
        case 15: return bin_sym_("]");
        case 16: return bin_sym_("<");
        case 17: return bin_sym_(">");
        case 18: return bin_sym_(".");
        case 19: return bin_sym_(",");
        case 20: return bin_sym_(";");
        case 31: return bin_name_("template");
        // Typwoerter kommen nur in Vorlagen vor, und die werden ueber die
        // Klammern uebersprungen - sie muessen nur nichts kaputt machen.
        case 40: return bin_name_("WORD");
        case 41: return bin_name_("DWORD");
        case 42: return bin_name_("FLOAT");
        case 43: return bin_name_("DOUBLE");
        case 44: return bin_name_("CHAR");
        case 45: return bin_name_("UCHAR");
        case 46: return bin_name_("SWORD");
        case 47: return bin_name_("SDWORD");
        case 48: return bin_name_("VOID");
        case 49: return bin_name_("LPSTR");
        case 50: return bin_name_("UNICODE");
        case 51: return bin_name_("CSTRING");
        case 52: return bin_name_("array");
        default: break;                         // unbekannte Marke: abbrechen
      }
      break;
    }
    kind = BB_XT_END;
    return false;
  }

};

// ---- Objektbaum ----

struct bb_XObj_ {
  std::string              type, name;
  std::vector<double>      nums;
  std::vector<std::string> strs;
  std::vector<std::string> refs;      // blosse Namen = Verweise
  std::vector<bb_XObj_>    kids;
};

// Ein Objekt ab der oeffnenden Klammer lesen.
inline bool bb_x_parse_obj_(bb_XLexer_& lx, bb_XObj_& o, int depth) {
  if (depth > 64) return false;                 // Schutz gegen entartete Dateien
  while (lx.next()) {
    if (lx.kind == BB_XT_SYM) {
      if (lx.text == "}") return true;
      if (lx.text == "{") {
        // Ein Verweis steht in eigenen Klammern: { Name }. Wer die
        // oeffnende Klammer nur ueberliest, laesst die schliessende das
        // **umgebende** Objekt beenden - alles danach faellt weg. Genau
        // daran hat ship.x statt vier nur ein Material gezeigt.
        while (lx.next()) {
          if (lx.kind == BB_XT_SYM && lx.text == "}") break;
          if (lx.kind == BB_XT_NAME) o.refs.push_back(lx.text);
        }
        continue;
      }
      continue;                                  // ';' und ',' tragen nichts
    }
    if (lx.kind == BB_XT_NUM) { o.nums.push_back(lx.num); continue; }
    if (lx.kind == BB_XT_STR) { o.strs.push_back(lx.text); continue; }
    if (lx.kind == BB_XT_NAME) {
      const std::string first = lx.text;
      bb_XLexer_ save = lx;
      if (!lx.next()) return false;
      if (lx.kind == BB_XT_SYM && lx.text == "{") {
        bb_XObj_ k; k.type = first;
        if (!bb_x_parse_obj_(lx, k, depth + 1)) return false;
        o.kids.push_back(std::move(k));
        continue;
      }
      if (lx.kind == BB_XT_NAME) {
        const std::string second = lx.text;
        bb_XLexer_ save2 = lx;
        if (lx.next() && lx.kind == BB_XT_SYM && lx.text == "{") {
          bb_XObj_ k; k.type = first; k.name = second;
          if (!bb_x_parse_obj_(lx, k, depth + 1)) return false;
          o.kids.push_back(std::move(k));
          continue;
        }
        lx = save2;
        o.refs.push_back(first);
        o.refs.push_back(second);
        continue;
      }
      lx = save;
      o.refs.push_back(first);
      continue;
    }
  }
  return false;                                  // Klammer nie geschlossen
}

inline bool bb_x_parse_(const std::string& src, std::vector<bb_XObj_>& roots,
                        bool bin = false, int fsize = 32) {
  bb_XLexer_ lx;
  lx.p = src.data();
  lx.e = src.data() + src.size();
  lx.bin = bin;
  lx.fsize = fsize;
  while (lx.next()) {
    if (lx.kind != BB_XT_NAME) continue;
    const std::string first = lx.text;
    if (first == "template") {                   // Vorlagen ueberspringen
      int depth = 0;
      while (lx.next()) {
        if (lx.kind != BB_XT_SYM) continue;
        if (lx.text == "{") ++depth;
        else if (lx.text == "}" && --depth == 0) break;
      }
      continue;
    }
    bb_XLexer_ save = lx;
    if (!lx.next()) break;
    if (lx.kind == BB_XT_SYM && lx.text == "{") {
      bb_XObj_ o; o.type = first;
      if (!bb_x_parse_obj_(lx, o, 0)) return !roots.empty();
      roots.push_back(std::move(o));
      continue;
    }
    if (lx.kind == BB_XT_NAME) {
      const std::string second = lx.text;
      if (lx.next() && lx.kind == BB_XT_SYM && lx.text == "{") {
        bb_XObj_ o; o.type = first; o.name = second;
        if (!bb_x_parse_obj_(lx, o, 0)) return !roots.empty();
        roots.push_back(std::move(o));
        continue;
      }
    }
    lx = save;
  }
  return !roots.empty();
}

// ---- 4x4, zeilenweise, Zeilenvektoren ----

struct bb_XMat_ { float m[16]; };

inline bb_XMat_ bb_x_ident_() {
  return { { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 } };
}

// v' = v * M
inline void bb_x_point_(const bb_XMat_& M, const float* v, float* o) {
  o[0] = M.m[0] * v[0] + M.m[4] * v[1] + M.m[8]  * v[2] + M.m[12];
  o[1] = M.m[1] * v[0] + M.m[5] * v[1] + M.m[9]  * v[2] + M.m[13];
  o[2] = M.m[2] * v[0] + M.m[6] * v[1] + M.m[10] * v[2] + M.m[14];
}

inline void bb_x_dir_(const bb_XMat_& M, const float* v, float* o) {
  o[0] = M.m[0] * v[0] + M.m[4] * v[1] + M.m[8]  * v[2];
  o[1] = M.m[1] * v[0] + M.m[5] * v[1] + M.m[9]  * v[2];
  o[2] = M.m[2] * v[0] + M.m[6] * v[1] + M.m[10] * v[2];
}

// Kind vor Elternteil.
inline bb_XMat_ bb_x_mul_(const bb_XMat_& a, const bb_XMat_& b) {
  bb_XMat_ r{};
  for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 4; ++j) {
      float s = 0;
      for (int k = 0; k < 4; ++k) s += a.m[i * 4 + k] * b.m[k * 4 + j];
      r.m[i * 4 + j] = s;
    }
  return r;
}

#endif // BLITZNEXT_BB_LOADER_X_H
