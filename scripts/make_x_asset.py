#!/usr/bin/env python3
"""Erzeugt die .x-Testdatei fuer tests/assets/.

Wie bei make_3ds_asset.py gilt: die Modelldateien der Blitz3D-Installation
sind fremdes Material und gehoeren nicht ins Repository, und ihre erwarteten
Zahlen muesste man erst ablesen. Diese hier ist so gebaut, dass die Erwartung
feststeht und dass sie genau die Stellen trifft, an denen ein .x-Leser
danebengreifen kann:

    test_frames.x
      * zwei Netze, eines davon in einem Frame mit Verschiebung um +10 in x.
        Damit ist nachweisbar, dass die Frame-Matrizen angewandt werden -
        ohne sie waere das Ergebnis nur 2 breit statt 11.
      * die Matrix steht zeilenweise und wirkt auf Zeilenvektoren.
      * ein Viereck je Netz, also Zerlegung in je zwei Dreiecke.
      * zwei Materialien: eines steht **inline**, das andere wird als
        Verweis `{name}` gefuehrt - die Form, an der ein Parser die
        schliessende Klammer leicht dem falschen Objekt zuordnet.
      * die Vorlage heisst absichtlich `TextureFileName` mit grossem N, wie
        in interior.X der Installation. Das Original vergleicht GUIDs, dem
        ist die Schreibweise egal; ein namensbasierter Leser muss ohne
        Ruecksicht auf Gross- und Kleinschreibung vergleichen.
      * MeshNormals und MeshTextureCoords mit genau passender Anzahl - nur
        dann uebernimmt das Original sie.

    Erwartet: w=11, h=2, d=0, zwei Flaechen, vier Dreiecke.

Aufruf aus dem Projektwurzelverzeichnis:

    python scripts/make_x_asset.py
"""

import os

HEADER = """xof 0302txt 0032

template Header {
 <3D82AB43-62DA-11cf-AB39-0020AF71E433>
 WORD major;
 WORD minor;
 DWORD flags;
}

Header {
 1;
 0;
 1;
}

Material rot {
 1.000000;0.000000;0.000000;1.000000;;
 38.400000;
 1.000000;1.000000;1.000000;;
 0.000000;0.000000;0.000000;;
 TextureFileName {
  "gibtsnicht.bmp";
 }
}
"""


def quad(x0, x1, y0, y1):
    """Vier Ecken eines Vierecks in der z=0-Ebene, gegen den Uhrzeigersinn."""
    return [(x0, y0, 0.0), (x1, y0, 0.0), (x1, y1, 0.0), (x0, y1, 0.0)]


def mesh(name, verts, material_inline):
    v = ",\n  ".join("%f;%f;%f;" % p for p in verts)
    n = len(verts)
    # ein Viereck ueber alle vier Ecken
    face = "1;\n  4;0,1,2,3;;\n"
    normals = ",\n  ".join("0.000000;0.000000;1.000000;" for _ in verts)
    uvs = ",\n  ".join("%f;%f;" % (i % 2, i // 2) for i in range(n))
    if material_inline:
        mats = """  MeshMaterialList {
   1;
   1;
   0;;
   Material {
    0.000000;1.000000;0.000000;1.000000;;
    5.000000;
    1.000000;1.000000;1.000000;;
    0.000000;0.000000;0.000000;;
   }
  }
"""
    else:
        mats = """  MeshMaterialList {
   1;
   1;
   0;;
   {rot}
  }
"""
    return """ Mesh %s {
  %d;
  %s;
  %s
%s  MeshNormals {
   %d;
   %s;
   1;
   4;0,1,2,3;;
  }
  MeshTextureCoords {
   %d;
   %s;
  }
 }
""" % (name, n, v, face, mats, n, normals, n, uvs)


def main():
    out = os.path.join("tests", "assets")
    os.makedirs(out, exist_ok=True)

    # Netz 1: Viereck 0..2, im Wurzelframe
    m1 = mesh("netz1", quad(0.0, 2.0, 0.0, 2.0), material_inline=True)
    # Netz 2: Viereck 0..1, in einem Frame um +10 in x verschoben
    m2 = mesh("netz2", quad(0.0, 1.0, 0.0, 1.0), material_inline=False)

    body = HEADER + """
Frame wurzel {
 FrameTransformMatrix {
  1.000000,0.000000,0.000000,0.000000,
  0.000000,1.000000,0.000000,0.000000,
  0.000000,0.000000,1.000000,0.000000,
  0.000000,0.000000,0.000000,1.000000;;
 }
%s
 Frame versetzt {
  FrameTransformMatrix {
   1.000000,0.000000,0.000000,0.000000,
   0.000000,1.000000,0.000000,0.000000,
   0.000000,0.000000,1.000000,0.000000,
   10.000000,0.000000,0.000000,1.000000;;
  }
%s
 }
}
""" % (m1, m2)

    path = os.path.join(out, "test_frames.x")
    with open(path, "w", newline="\n") as f:
        f.write(body)
    print("%s: %d Bytes" % (path, len(body)))
    print("  erwartet: w=11 h=2 d=0, zwei Flaechen, vier Dreiecke")


if __name__ == "__main__":
    main()
