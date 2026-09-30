#!/usr/bin/env python3
"""Die Waffe aus Blender (GPT-6 Astra) als Raketenwerfer des Leuchtturm-Spiels.

Quelle ist die Datei, wie sie aus Blender kam:

    werkzeug/astra/weapon.glb   ein Netz (31 763 Ecken, 15 500 Dreiecke), vier
                                Texturen (Farbe 4096 x 4096, Leuchten, Normalen,
                                ORM), dazu der Standardwuerfel, der in der Szene
                                blieb; Lauf nach -x, oben +y, Griff nach unten

Das Werkzeug macht daraus daten/rl.glb nach den Regeln aus LEUCHTTURM.md
("Waffen in Blender"):

  - der Wuerfel faellt weg
  - das Netz wird mit Blender (ohne Oberflaeche, astra_blender.py, auf einer Kopie) von
    15 500 auf 6 000 Dreiecke gebracht; Blender wird ueber die Umgebungsvariable
    BLENDER oder den PATH gefunden, ohne bleibt das Netz unvereinfacht
  - das Netz wird gedreht (Lauf nach Blitz +z), auf 0,30 m Laenge gebracht und
    so gelegt, dass der Griff unter der Kamera haengt
  - "muendung": ein leerer Knoten an der Spitze des Laufs
  - Animationen "feuern" (Sequenz 0) und "heben" (Sequenz 1); die Datei hatte
    keine, sie sind hier erfunden - Stoss nach hinten, die Spitze steigt
  - Textur: die Farbtextur wird mit Blender neu abgemischt (veredeln.py, Modus astra):
    Fugenschatten, Kantenlicht, Risse, Rost - naeher am Konzeptbild, und der Renderer
    kann Tiefe nur so zeigen. Der Blitz3D-Renderer liest nur die Farbtextur (Normalen und ORM
    bleiben liegen) und keine Leuchttextur; die Farbe wird deshalb auf 1024 x 1024
    verkleinert, die Leuchttextur gleich daraufgerechnet und als JPEG gespeichert
    (textur.ps1, braucht Windows PowerShell). So schrumpft die Datei von 17 MB auf 0,8 MB.

Aufruf aus dem Projektwurzelverzeichnis:

    py samples/leuchtturm/werkzeug/astra.py
"""

import json
import os
import shutil
import struct
import subprocess
import sys
import tempfile

from glb import pos, quat, write
from kenney import HEBEN, NULL, RUHE

HERE = os.path.dirname(os.path.abspath(__file__))
QUELLE = os.path.join(HERE, "astra", "weapon.glb")
AUS = os.path.join(HERE, "..", "daten", "rl.glb")
GEN = "BLTZNXT samples/leuchtturm/werkzeug/astra.py - Modell GPT-6 Astra (Blender)"

DREIECKE = 6000         # Ziel der Vereinfachung in Blender
LAENGE = 0.30           # Meter, Lauf bis Kolben
MITTE = (-0.03, -0.02)   # Mitte von Breite und Hoehe im Modell: Waffen_Zeigen haengt es bei
                        # (0.13, -0.12, 0.10) an die Kamera
HINTEN = 0.0            # Ort des hinteren Endes im Modell (Blitz z)
TEXTUR = 1024           # Pixel


def lade(pfad):
    """(Ecken, Normalen, UV, Indizes, {Bildname: PNG-Bytes}) des ersten Netzes."""
    d = open(pfad, "rb").read()
    jl = struct.unpack_from("<I", d, 12)[0]
    g = json.loads(d[20:20 + jl])
    b = d[20 + jl + 8:]

    def lies(ai):
        a = g["accessors"][ai]
        v = g["bufferViews"][a["bufferView"]]
        off = v.get("byteOffset", 0) + a.get("byteOffset", 0)
        if a["componentType"] == 5126:
            k = {"VEC2": 2, "VEC3": 3}[a["type"]]
            return [struct.unpack_from("<%df" % k, b, off + i * 4 * k) for i in range(a["count"])]
        f = {5121: "B", 5123: "H", 5125: "I"}[a["componentType"]]
        return list(struct.unpack_from("<%d%s" % (a["count"], f), b, off))

    # Das Netz, das zur Waffe gehoert: das mit den meisten Ecken (der Wuerfel
    # aus der Blender-Vorgabe hat 24).
    pr = max((p for m in g["meshes"] for p in m["primitives"]),
             key=lambda p: g["accessors"][p["attributes"]["POSITION"]]["count"])
    at = pr["attributes"]
    bilder = {}
    for im in g["images"]:
        v = g["bufferViews"][im["bufferView"]]
        o = v.get("byteOffset", 0)
        bilder[im["name"].split("_")[-1]] = b[o:o + v["byteLength"]]
    return lies(at["POSITION"]), lies(at["NORMAL"]), lies(at["TEXCOORD_0"]), lies(pr["indices"]), bilder


def blender():
    """Pfad zu blender.exe: Umgebungsvariable BLENDER, sonst der PATH, sonst nichts."""
    return os.environ.get("BLENDER") or shutil.which("blender")


def vereinfache(quelle, tmp):
    """Das Netz mit Blender (ohne Oberflaeche, auf einer Kopie) auf DREIECKE bringen;
    ohne Blender bleibt es, wie es ist."""
    exe = blender()
    if not exe:
        print("astra.py: kein Blender (BLENDER setzen) - das Netz bleibt unvereinfacht")
        return quelle
    aus = os.path.join(tmp, "weapon_opt.glb")
    subprocess.run([exe, "-b", "--python", os.path.join(HERE, "astra_blender.py"), "--",
                    quelle, aus, str(DREIECKE)], check=True, stdout=subprocess.DEVNULL)
    return aus


def veredle(bilder, tmp):
    """Die Farbtextur mit Blender neu abmischen (veredeln.py, Modus astra):
    Fugenschatten, Kantenlicht, Risse, Rost. Ohne Blender bleibt sie, wie sie ist."""
    exe = blender()
    if not exe:
        return bilder
    basis, aus = os.path.join(tmp, "basis.png"), os.path.join(tmp, "veredelt.png")
    open(basis, "wb").write(bilder["BaseColor"])
    subprocess.run([exe, "-b", "--python", os.path.join(HERE, "veredeln.py"), "--", "astra",
                    os.path.join(tmp, "weapon_opt.glb"), aus, "basis=" + basis, "ao=0.7", "kante=0.8",
                    "radius=0.012"], check=True, stdout=subprocess.DEVNULL)
    return dict(bilder, BaseColor=open(aus, "rb").read())


def textur(bilder):
    """Farbe und Leuchten zu einer PNG verschmelzen (textur.ps1)."""
    if sys.platform != "win32":
        sys.exit("astra.py: die Textur wird mit Windows PowerShell (System.Drawing) gerechnet")
    with tempfile.TemporaryDirectory() as tmp:
        pfade = {}
        for name in ("BaseColor", "Emission"):
            pfade[name] = os.path.join(tmp, name + ".png")
            open(pfade[name], "wb").write(bilder[name])
        aus = os.path.join(tmp, "farbe.jpg")
        subprocess.run(["powershell", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File",
                        os.path.join(HERE, "textur.ps1"), pfade["BaseColor"], pfade["Emission"],
                        str(TEXTUR), aus], check=True)
        return open(aus, "rb").read()


def main():
    with tempfile.TemporaryDirectory() as tmp:
        p, n, uv, idx, bilder = lade(vereinfache(QUELLE, tmp))
        bilder = veredle(bilder, tmp)

    # glTF-Quelle: Lauf nach -x, oben +y. Gedreht um y so, dass der Lauf nach
    # glTF -z zeigt (Blitz +z): (x, y, z) -> (-z, y, x). Eine Drehung, keine
    # Spiegelung - die Dreiecke behalten ihre Richtung.
    dreh = lambda v: (-v[2], v[1], v[0])
    p = [dreh(v) for v in p]
    n = [dreh(v) for v in n]

    lo = [min(v[i] for v in p) for i in range(3)]
    hi = [max(v[i] for v in p) for i in range(3)]
    s = LAENGE / (hi[2] - lo[2])
    # Die Waffe ist dicker und hoeher als die von Kenney; der Platz in der Hand
    # (waffen.bb, fuer alle gleich) ist darum hier im Modell verschoben: so, dass
    # der Kern im Bild steht und der Kolben nicht hinter der Kamera endet.
    # glTF z ist Blitz -z: das hintere Ende (glTF +z, hi) kommt auf Blitz HINTEN.
    dx, dy, dz = MITTE[0] - (lo[0] + hi[0]) / 2 * s, MITTE[1] - (lo[1] + hi[1]) / 2 * s, -HINTEN - hi[2] * s
    p = [(v[0] * s + dx, v[1] * s + dy, v[2] * s + dz) for v in p]

    # Muendung: Mitte der vordersten Ecken (kleinstes glTF z)
    zmin = min(v[2] for v in p)
    spitze = [v for v in p if v[2] < zmin + 0.004]
    mx = (min(v[0] for v in spitze) + max(v[0] for v in spitze)) / 2
    my = (min(v[1] for v in spitze) + max(v[1] for v in spitze)) / 2
    muendung = pos(mx, my, -zmin)       # pos() rechnet von Blitz- nach glTF-Koordinaten

    nodes = [{"name": "rl", "mesh": 0, "children": [1]},
             {"name": "muendung", "translation": list(muendung)}]
    # feuern: Stoss nach hinten, die Spitze steigt, dann langsam zurueck;
    # die Waffe ist schwerer als die von Kenney, der Stoss darum etwas kraeftiger
    anims = [("feuern", [(0, "translation", [(0, NULL), (2, pos(0, 0.012, -0.07)), (30, NULL)]),
                         (0, "rotation", [(0, RUHE), (2, quat(1, 0, 0, -12)), (30, RUHE)])]),
             ("heben", [(0, "translation", [(f, t) for f, t, r in HEBEN]),
                        (0, "rotation", [(f, r) for f, t, r in HEBEN])])]
    write(AUS, nodes, [(p, n, None, idx, uv)], GEN, anims, texture=textur(bilder), texture_mime="image/jpeg")
    print("%s: %d Dreiecke, %d Byte" % (os.path.normpath(AUS), len(idx) // 3, os.path.getsize(AUS)))
    print("Laenge %.3f m, Hoehe %.3f m, Breite %.3f m, Muendung Blitz (%.3f, %.3f, %.3f)" % (
        (max(v[2] for v in p) - zmin), max(v[1] for v in p) - min(v[1] for v in p),
        max(v[0] for v in p) - min(v[0] for v in p), mx, my, -zmin))


if __name__ == "__main__":
    main()
