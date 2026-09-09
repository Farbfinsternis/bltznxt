// scripts/winding_check.cpp
//
// Prueft die Umlaufrichtung unserer Netze - ohne das Original und ohne
// ReadPixel (das sieht wegen BUG-63 das 3D-Bild nicht).
//
// Die Idee: bei einem geschlossenen Netz mit richtiger Umlaufrichtung ist die
// Silhouette **mit und ohne** Rueckseitenentfernung gleich breit, denn die
// nahen Flaechen verdecken die fernen ohnehin. Ist eine Flaeche nach innen
// gedreht, wird sie weggeschnitten und man sieht die ferne Seite: schmaler.
// EntityFX 16 schaltet die Rueckseitenentfernung ab.
//
// Damit sind am 2026-09-09 fuenf falsche Flaechengruppen gefunden worden, die
// am Bild nicht auffallen: die beiden Seiten des Wuerfels (BUG-65), beide
// Deckel des Zylinders und der Boden des Kegels (BUG-70). Ein Kegel ohne
// Boden sieht von der Seite genauso aus wie einer mit.
//
// Bauen (aus dem Projektwurzelverzeichnis):
//
//   tools/mingw64/bin/g++.exe -std=c++17 -static \
//     -Isrc/compiler -Ilibs/sd3/x86_64-w64-mingw32/include \
//     scripts/winding_check.cpp -o bin/winding_check.exe \
//     libs/sd3/x86_64-w64-mingw32/lib/libSDL3.dll.a -lwinmm -lopengl32
//
// Aufrufen ebenfalls aus dem Wurzelverzeichnis, damit tests/assets/ gefunden
// wird:  bin/winding_check.exe ausgabe.txt

#include "bb_runtime.h"
#include <cstdio>
#include <vector>

int main(int argc, char** argv) {
  const char* out = (argc > 1) ? argv[1] : "wind.txt";
  const int W = 200, H = 200;
  bb_Graphics3D(W, H, 32, 2);

  int cam = bb_CreateCamera();
  bb_CameraClsColor(cam, 0, 0, 255);
  bb_PositionEntity(cam, 0, 0, -5);

  std::FILE* f = std::fopen(out, "w");
  std::vector<unsigned char> px(static_cast<size_t>(W) * H * 4);

  auto breite = [&](int ent, int fx, float pitch, float yaw) -> int {
    bb_RotateEntity(ent, pitch, yaw, 0);
    bb_EntityFX(ent, fx);
    bb_UpdateWorld();
    bb_RenderWorld();
    glReadPixels(0, 0, W, H, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
    int n = 0;
    for (int x = 0; x < W; ++x) {
      const size_t k = (static_cast<size_t>(H / 2) * W + x) * 4;
      if (px[k] > 200 && px[k + 1] < 60 && px[k + 2] < 60) ++n;
    }
    return n;
  };

  struct Lage { const char* name; float pitch, yaw; };
  const Lage lagen[] = { { "Z vorn",   0,   0 },
                         { "Z hinten", 0, 180 },
                         { "X vorn",   0,  90 },
                         { "X hinten", 0, -90 },
                         { "Y vorn",  90,   0 },
                         { "Y hinten", -90, 0 } };

  auto pruefe = [&](const char* name, int ent) {
    if (!ent) { std::fprintf(f, "%s FEHLT\n", name); return; }
    bb_EntityColor(ent, 255, 0, 0);
    for (const Lage& l : lagen) {
      const int mit  = breite(ent, 1,      l.pitch, l.yaw);
      const int ohne = breite(ent, 1 + 16, l.pitch, l.yaw);
      std::fprintf(f, "%-16s %-8s mit=%3d ohne=%3d %s\n",
                   name, l.name, mit, ohne,
                   (mit == ohne) ? "gleich" : "NACH INNEN GEDREHT");
    }
    bb_HideEntity(ent);
  };

  pruefe("wuerfel",  bb_CreateCube());
  pruefe("kugel",    bb_CreateSphere(8));
  pruefe("zylinder", bb_CreateCylinder(8, false));
  pruefe("kegel",    bb_CreateCone(8, false));

  int box = bb_LoadMesh("tests/assets/test_box.3ds");
  if (box) bb_ScaleEntity(box, 0.2f, 0.2f, 0.2f);
  pruefe("test_box.3ds", box);

  std::fclose(f);
  return 0;
}
