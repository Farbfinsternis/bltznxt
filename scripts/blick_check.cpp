// BUG-84: Wohin blickt eine Kamera nach RotateEntity 0,90,0?
//
// Dieselbe Szene wie blick.bb im Original: ein roter Wuerfel bei +X, ein
// gruener bei -X, die Kamera im Ursprung. Die Bildmitte verraet, wohin sie
// blickt. Aus der Sprache heraus ist das bei uns wegen BUG-63 nicht lesbar,
// deshalb wie in scripts/winding_check.cpp direkt ueber glReadPixels.
//
// Gemessen im Original (2026-09-11): yaw +90 -> gruen (Blick nach -X),
// yaw -90 -> rot. Vor dem Fix zu BUG-84 meldete diese Datei genau umgekehrt
// rot/gruen - das war der Nachweis, dass nicht nur zurueckgelesene Zahlen
// betroffen waren, sondern das **Bild**.
//
// Bauen (aus dem Projektwurzelverzeichnis):
//
//   tools/mingw64/bin/g++.exe -std=c++17 -static \
//     -Isrc/compiler -Ilibs/sd3/x86_64-w64-mingw32/include \
//     scripts/blick_check.cpp -o bin/blick_check.exe \
//     libs/sd3/x86_64-w64-mingw32/lib/libSDL3.dll.a -lwinmm -lopengl32
//
//   bin/blick_check.exe ausgabe.txt

#include "bb_runtime.h"
#include <cstdio>
#include <vector>

int main(int argc, char** argv) {
  const char* out = (argc > 1) ? argv[1] : "blick_ours.txt";
  const int W = 200, H = 200;
  bb_Graphics3D(W, H, 32, 2);

  int cam = bb_CreateCamera();
  bb_CameraClsColor(cam, 0, 0, 255);

  int rechts = bb_CreateCube();
  bb_PositionEntity(rechts, 5, 0, 0);
  bb_ScaleEntity(rechts, 2, 2, 2);
  bb_EntityColor(rechts, 255, 0, 0);
  bb_EntityFX(rechts, 1);

  int links = bb_CreateCube();
  bb_PositionEntity(links, -5, 0, 0);
  bb_ScaleEntity(links, 2, 2, 2);
  bb_EntityColor(links, 0, 255, 0);
  bb_EntityFX(links, 1);

  std::FILE* f = std::fopen(out, "w");
  std::vector<unsigned char> px(static_cast<size_t>(W) * H * 4);

  auto mitte = [&](float yaw, const char* name) {
    bb_RotateEntity(cam, 0, yaw, 0);
    bb_UpdateWorld();
    bb_RenderWorld();
    glReadPixels(0, 0, W, H, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
    const size_t k = (static_cast<size_t>(H / 2) * W + W / 2) * 4;
    std::fprintf(f, "%s: r=%d g=%d b=%d\n", name,
                 (int)px[k], (int)px[k + 1], (int)px[k + 2]);

    // Dazu die Sichtmatrix selbst: wo landen die beiden Wuerfel im
    // Sichtraum? In GL liegt vor der Kamera das negative z.
    bb_CameraEntity_* c = bb_cam_(cam);
    float p[3];
    mat4_xform_pt_(p, c->view, 5, 0, 0);
    std::fprintf(f, "   +X wuerfel im sichtraum z=%.2f\n", p[2]);
    mat4_xform_pt_(p, c->view, -5, 0, 0);
    std::fprintf(f, "   -X wuerfel im sichtraum z=%.2f\n", p[2]);
  };

  mitte(90,  "yaw 90");
  mitte(-90, "yaw -90");

  // ---- BUG-71: wirkt eine Verschiebung sofort im Bild? ----
  //
  // Im Original wirkt `PositionEntity` sofort, auch fuer das Gezeichnete -
  // gemessen (2026-09-11): der Wuerfel verschwindet aus der Bildmitte, ohne
  // dass ein UpdateWorld dazwischen steht. Bei uns schrieb lange nur
  // UpdateWorld die Weltmatrix, also zeichnete der Renderpfad ihn an der
  // alten Stelle weiter. Die Kontrollstellung davor gehoert dazu: eine
  // Messung, die immer dasselbe meldet, misst meistens sich selbst.
  {
    bb_RotateEntity(cam, 0, 0, 0);
    bb_PositionEntity(cam, 0, 0, -5);
    bb_FreeEntity(links);
    bb_PositionEntity(rechts, 0, 0, 0);   // vor die Kamera
    bb_UpdateWorld();
    bb_RenderWorld();
    glReadPixels(0, 0, W, H, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
    const size_t k = (static_cast<size_t>(H / 2) * W + W / 2) * 4;
    std::fprintf(f, "kontrollstellung: r=%d (erwartet hoch)\n", (int)px[k]);

    bb_PositionEntity(rechts, 100, 0, 0); // weit weg - OHNE UpdateWorld
    bb_RenderWorld();
    glReadPixels(0, 0, W, H, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
    std::fprintf(f, "nach PositionEntity ohne UpdateWorld: r=%d (erwartet 0)\n",
                 (int)px[k]);
  }

  std::fclose(f);
  return 0;
}
