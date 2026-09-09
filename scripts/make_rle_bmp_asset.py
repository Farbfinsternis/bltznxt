#!/usr/bin/env python3
"""Erzeugt die lauflaengenkodierten BMP-Testdateien fuer tests/assets/ (BUG-67).

Die einzige RLE-BMP der Blitz3D-Installation (Games/wing_ring/media/F15.BMP)
ist fremdes Material und gehoert nicht ins Repository. Diese hier sind eigens
gebaut, damit die Erwartung ohne Nachschlagen feststeht - und sie benutzen
absichtlich alle vier Kommandos des Formats, was eine echte Textur meist nicht
tut:

    test_rle8.bmp    8x4, 8 Bit je Bildpunkt, Kompression BI_RLE8 (1).
    test_rle4.bmp    8x4, 4 Bit je Bildpunkt, Kompression BI_RLE4 (2),
                     dasselbe Bild - beide muessen Bildpunkt fuer Bildpunkt
                     dasselbe ergeben.

Das Bild, oberste Zeile zuerst (Palette: 0 schwarz, 1 rot, 2 gruen, 3 blau):

    blau   blau   blau  blau  blau  blau  blau  blau     Lauf ueber 8
    schwrz schwrz gruen gruen schwz schwz schwz schwz     Sprung, dann Lauf
    rot    gruen  blau  schwz schwz schwz schwz schwz     Rohdaten
    rot    rot    rot   rot   schwz schwz schwz schwz     Lauf, dann Zeilenende

Die schwarzen Felder sind der eigentliche Punkt: sie stehen dort, wo das
Zeilenende- und das Sprungkommando Bildpunkte ueberspringen. Was dabei
uebersprungen wird, bleibt auf Palettenindex 0 - so macht es FreeImage 2.4.1,
ueber das das Original seine Bilder laedt, weil sein Puffer genullt angelegt
ist.

Aufruf aus dem Projektwurzelverzeichnis:

    python scripts/make_rle_bmp_asset.py

Zum Format: nach Dateikopf, Informationskopf und Palette folgen Bytepaare.
Ist das erste Byte groesser 0, ist es eine Laufzahl und das zweite der Inhalt
(bei 4 Bit zwei abwechselnde Halbbytes). Ist es 0, ist das zweite ein Befehl:
0 Zeilenende, 1 Bildende, 2 Sprung um die naechsten zwei Bytes, ab 3 die Zahl
roh folgender Bildpunkte, aufgefuellt auf eine gerade Byte-Zahl. Die Bildzeilen
stehen von unten nach oben in der Datei.
"""

import os
import struct

PALETTE = [(0, 0, 0), (255, 0, 0), (0, 255, 0), (0, 0, 255)]
W, H = 8, 4


def bmp(width, height, bpp, comp, palette, data):
    pal = b"".join(bytes([b, g, r, 0]) for (r, g, b) in palette)
    info = struct.pack("<IiiHHIIiiII", 40, width, height, 1, bpp, comp,
                       len(data), 2835, 2835, len(palette), 0)
    off = 14 + len(info) + len(pal)
    head = b"BM" + struct.pack("<IHHI", off + len(data), 0, 0, off)
    return head + info + pal + data


def rle8():
    d = bytearray()
    d += bytes([4, 1])                    # unterste Zeile: vier rote
    d += bytes([0, 0])                    # Zeilenende, Rest bleibt Index 0
    d += bytes([0, 3, 1, 2, 3, 0])        # Rohdaten rot/gruen/blau + Fuellbyte
    d += bytes([0, 0])
    d += bytes([0, 2, 2, 0])              # Sprung um zwei nach rechts
    d += bytes([2, 2])                    # zwei gruene
    d += bytes([0, 0])
    d += bytes([8, 3])                    # oberste Zeile: acht blaue
    d += bytes([0, 1])                    # Bildende
    return bmp(W, H, 8, 1, PALETTE, bytes(d))


def rle4():
    d = bytearray()
    d += bytes([4, 0x11])                 # Lauf: beide Halbbytes rot
    d += bytes([0, 0])
    d += bytes([0, 3, 0x12, 0x30])        # Rohdaten 1,2,3 -> zwei Bytes, schon gerade
    d += bytes([0, 0])
    d += bytes([0, 2, 2, 0])
    d += bytes([2, 0x22])
    d += bytes([0, 0])
    d += bytes([8, 0x33])
    d += bytes([0, 1])
    return bmp(W, H, 4, 2, PALETTE, bytes(d))


def main():
    out = os.path.join("tests", "assets")
    for name, blob in (("test_rle8.bmp", rle8()), ("test_rle4.bmp", rle4())):
        path = os.path.join(out, name)
        with open(path, "wb") as fh:
            fh.write(blob)
        print("geschrieben:", path, len(blob), "Bytes")


if __name__ == "__main__":
    main()
