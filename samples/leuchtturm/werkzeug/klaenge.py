#!/usr/bin/env python3
"""Platzhalter-Klaenge fuer das Leuchtturm-Spiel (Schritt 6: 3D-Klang).

Bis es echte Aufnahmen gibt, rechnet dieses Skript die Klaenge aus Rauschen
und Sinustoenen zusammen - frei von Rechten und jederzeit neu erzeugbar.
Mono, 16 Bit, 44.1 kHz, nach daten/klang/:

    mg.wav          kurzer, harter Knall
    rl.wav          dumpfes Abfeuern mit Zischen
    rakete.wav      Schub der fliegenden Rakete, nahtlos wiederholbar
    explosion.wav   tiefer Donner, lang ausklingend
    rail.wav        steigender Ton mit Rauschen, elektrisch
    treffer.wav     heller Klick auf der Scheibe
    zerplatzen.wav  Scheibe zerspringt
    wieder.wav      Scheibe erscheint wieder
    wechsel.wav     Klacken beim Waffenwechsel
    leer.wav        Klicken ohne Munition

Aufruf aus dem Projektwurzelverzeichnis:

    python samples/leuchtturm/werkzeug/klaenge.py
"""

import math
import os
import random
import struct
import wave

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "..", "daten", "klang")
RATE = 44100


def rauschen(n, seed):
    r = random.Random(seed)
    return [r.uniform(-1, 1) for _ in range(n)]


def tiefpass(x, a):
    """Einfacher Tiefpass: a nahe 1 = dumpf."""
    y, v = [], 0.0
    for s in x:
        v = a * v + (1 - a) * s
        y.append(v)
    return y


def huelle(n, anstieg, abfall):
    """Anstieg in Sekunden, dann exponentieller Abfall mit Zeitkonstante abfall."""
    a = max(1, int(anstieg * RATE))
    return [min(1.0, i / a) * math.exp(-max(0, i - a) / (abfall * RATE)) for i in range(n)]


def ton(n, f0, f1, seed=0):
    """Sinus mit gleitender Frequenz von f0 nach f1."""
    y, ph = [], 0.0
    for i in range(n):
        f = f0 + (f1 - f0) * i / n
        ph += 2 * math.pi * f / RATE
        y.append(math.sin(ph))
    return y


def mische(*teile):
    n = max(len(t) for t in teile)
    return [sum(t[i] for t in teile if i < len(t)) for i in range(n)]


def mal(x, h, g=1.0):
    return [a * b * g for a, b in zip(x, h)]


def schreibe(name, x, pegel=0.9):
    m = max(1e-9, max(abs(s) for s in x))
    daten = b"".join(struct.pack("<h", int(max(-1, min(1, s / m * pegel)) * 32767)) for s in x)
    os.makedirs(OUT, exist_ok=True)
    pfad = os.path.join(OUT, name + ".wav")
    with wave.open(pfad, "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(RATE)
        w.writeframes(daten)
    print("%s: %.2f s" % (os.path.normpath(pfad), len(x) / RATE))


def sek(s):
    return int(s * RATE)


def main():
    n = sek(0.18)
    schreibe("mg", mische(mal(tiefpass(rauschen(n, 1), 0.3), huelle(n, 0.001, 0.03)),
                          mal(ton(n, 180, 60), huelle(n, 0.001, 0.02), 0.6)))

    n = sek(0.6)
    schreibe("rl", mische(mal(tiefpass(rauschen(n, 2), 0.85), huelle(n, 0.002, 0.12)),
                          mal(tiefpass(rauschen(n, 3), 0.2), huelle(n, 0.05, 0.25), 0.4),
                          mal(ton(n, 90, 40), huelle(n, 0.002, 0.08), 0.8)))

    # Schub: gefiltertes Rauschen, Anfang und Ende ueberblendet, damit die
    # Wiederholung keine Naht hat
    n = sek(0.5)
    x = tiefpass(rauschen(n + sek(0.1), 4), 0.7)
    k = sek(0.1)
    x = [x[i] * (i / k) + x[i + n] * (1 - i / k) if i < k else x[i] for i in range(n)]
    schreibe("rakete", x, 0.6)

    n = sek(1.6)
    schreibe("explosion", mische(mal(tiefpass(rauschen(n, 5), 0.97), huelle(n, 0.003, 0.45)),
                                 mal(tiefpass(rauschen(n, 6), 0.6), huelle(n, 0.001, 0.12), 0.5),
                                 mal(ton(n, 60, 25), huelle(n, 0.003, 0.3), 0.7)))

    n = sek(0.9)
    schreibe("rail", mische(mal(ton(n, 300, 1400), huelle(n, 0.005, 0.25), 0.7),
                            mal(ton(n, 305, 1420), huelle(n, 0.005, 0.25), 0.5),
                            mal(tiefpass(rauschen(n, 7), 0.5), huelle(n, 0.001, 0.15), 0.6)))

    n = sek(0.08)
    schreibe("treffer", mische(mal(ton(n, 2400, 1800), huelle(n, 0.0005, 0.015)),
                               mal(rauschen(n, 8), huelle(n, 0.0005, 0.008), 0.4)), 0.7)

    n = sek(0.7)
    glas = mische(*[mal(ton(n, f, f * 0.97), huelle(n, 0.001, 0.12 + 0.05 * i), 0.4)
                    for i, f in enumerate((1800, 2650, 3400, 4100))])
    schreibe("zerplatzen", mische(glas, mal(rauschen(n, 9), huelle(n, 0.001, 0.05), 0.8)))

    n = sek(0.5)
    schreibe("wieder", mal(mische(ton(n, 400, 1200), ton(n, 600, 1800)), huelle(n, 0.15, 0.15)), 0.5)

    n = sek(0.12)
    schreibe("wechsel", mische(mal(tiefpass(rauschen(n, 10), 0.4), huelle(n, 0.0005, 0.01)),
                               mal(ton(n, 900, 700), huelle(n, 0.0005, 0.02), 0.5)), 0.5)

    n = sek(0.06)
    schreibe("leer", mal(ton(n, 1500, 1500), huelle(n, 0.0005, 0.008)), 0.5)


if __name__ == "__main__":
    main()
