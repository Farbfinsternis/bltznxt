#!/usr/bin/env python3
"""Haelt src/compiler/commands.h gegen die Befehlsliste des originalen Blitz3D.

    blitzcc +k > keywords.txt          # aus der Blitz3D-Installation
    python scripts/compare_commands.py keywords.txt src/compiler/commands.h

Verglichen werden Stelligkeit (min..max), Rueckgabetyp und die Reihenfolge der
Parameter. Der letzte Punkt ist der wichtigste: Blitz3D kennt keine benannten
Argumente, also ist eine vertauschte Reihenfolge ein stilles Falschergebnis und
faellt sonst nirgends auf. So sind am 2026-09-07 JoyDown, JoyHit, ReadBytes und
WriteBytes aufgefallen (BUG-44).

Reine Benennungsunterschiede (`h` statt `entity`) sind belanglos und werden
zusammengefasst statt einzeln aufgezaehlt.

commands.h beschreibt bewusst, was UNSERE Runtime annimmt - das ist nicht in
jedem Fall dieselbe Menge wie beim Original (README, "The Blitz3D Source as a
Reference"). Der Bericht ist deshalb eine Fundliste, keine Fehlerliste:
- "wir strenger" lehnt gueltige Blitz3D-Programme ab und ist immer ein Fehler;
- "wir laxer" nimmt mehr an als die Sprache und ist eine Entscheidung;
- "Reihenfolge" ist immer ein Fehler.
"""
import io, re, sys


def parse_original(path):
    # Die +k-Ausgabe listet erst die blossen Schluesselwoerter (eine Zeile, kein
    # Leerzeichen, keine Klammer), danach die Befehle. Eine parameterlose
    # Anweisung sieht aus wie ein Schluesselwort und ist nur daran zu erkennen,
    # dass sie nach der Grenze steht: "Cls " und "FlushJoy " haben lediglich ein
    # Leerzeichen am Zeilenende, das ein rstrip() verschluckt.
    out, in_cmds = {}, False
    for raw in io.open(path, encoding="latin-1"):
        line = raw.rstrip("\n")
        if not line.strip():
            continue
        if not in_cmds:
            if " " in line or "(" in line:
                in_cmds = True
            else:
                continue
        if "(" in line:
            head, _, rest = line.partition("(")
            params, kind = rest.rsplit(")", 1)[0], "func"
        else:
            head, _, params = line.partition(" ")
            kind = "stmt"
        m = re.match(r"^([A-Za-z_][A-Za-z0-9_]*)([#$%]?)$", head.strip())
        if not m:
            continue
        name, suf = m.group(1), m.group(2)
        ret = suf if suf else ("%" if kind == "func" else "")
        p = params.strip()
        if not p:
            mn = mx = 0
        else:
            mx = p.count(",") + 1
            req = p.split("[")[0]
            mn = 0 if not req.strip(" ,") else req.count(",") + 1
        out[name.lower()] = (name, ret, mn, mx, line.strip())
    return out


def parse_ours(path):
    out = {}
    for line in io.open(path, encoding="utf-8"):
        m = re.match(r'\s*\{\s*"([^"]+)"\s*,\s*"([^"]*)"\s*,\s*"([^"]*)"\s*\}', line)
        if not m:
            continue
        name, ret, params = m.groups()
        parts = [x for x in params.split(",") if x.strip()]
        out[name.lower()] = (name, ret, len([x for x in parts if not x.strip().endswith("?")]),
                             len(parts), params)
    return out


def names_orig(src):
    p = src.partition("(")[2].rsplit(")", 1)[0] if "(" in src else src.partition(" ")[2]
    p = p.replace("[", "").replace("]", "")
    return [x.strip().rstrip("#$%").lower() for x in p.split(",") if x.strip()]


def names_ours(params):
    return [re.sub(r"[#$%?]+$", "", x.strip()).lower() for x in params.split(",") if x.strip()]


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        return 2
    orig, ours = parse_original(sys.argv[1]), parse_ours(sys.argv[2])
    strict, lax, mixed, rett, order, renamed = [], [], [], [], [], 0

    for k in sorted(ours):
        n, r, mn, mx, params = ours[k]
        if k not in orig:
            continue
        on, orr, omn, omx, osrc = orig[k]
        row = "%-18s wir %d..%-2d  orig %d..%-2d  | %s" % (n, mn, mx, omn, omx, osrc)
        if (mn, mx) != (omn, omx):
            if mn > omn and mx >= omx:
                strict.append(row)
            elif mn <= omn and mx > omx:
                lax.append(row)
            else:
                mixed.append(row)
        elif r != orr and r != ".":
            rett.append("%-18s wir '%s'  orig '%s'  | %s" % (n, r, orr, osrc))
        a, b = names_ours(params), names_orig(osrc)
        if a and b and len(a) == len(b) and a != b:
            if _looks_reordered(a, b):
                order.append("%-18s wir (%s)  orig (%s)" % (n, ",".join(a), ",".join(b)))
            else:
                renamed += 1

    print("Original: %d Befehle, wir: %d" % (len(orig), len(ours)))
    _show("Parameter womoeglich vertauscht - jede Zeile von Hand pruefen", order)
    _show("WIR STRENGER - lehnt gueltige Programme ab", strict)
    _show("Grenzen gemischt (Maximum zu klein oder Minimum zu gross)", mixed)
    _show("WIR LAXER - Erweiterung, lehnt nichts ab", lax)
    _show("Rueckgabetyp weicht ab", rett)
    print("Nur andere Parameternamen (belanglos): %d" % renamed)
    return 0


def _looks_reordered(a, b):
    """Verdacht auf vertauschte Parameter.

    Nicht ueber Namensgleichheit - unsere Header kuerzen (`h` fuer `entity`,
    `s` fuer `string`), das erzeugt sonst Dutzende Fehlalarme. Das tragfaehige
    Signal ist: ein Name, den beide Seiten fuehren (gleich oder als Praefix,
    also auch `file` gegen `filehandle`), steht an VERSCHIEDENEN Positionen.
    Genau daran waren JoyDown/JoyHit (port gegen button) und
    ReadBytes/WriteBytes (bank gegen file) zu erkennen.
    """
    if len(a) < 2:
        return False
    for ia, x in enumerate(a):
        if len(x) < 3:
            continue
        for ib, y in enumerate(b):
            if len(y) < 3:
                continue
            if (x.startswith(y) or y.startswith(x)) and ia != ib:
                return True
    return False


def _show(title, rows):
    print()
    print("== %s (%d) ==" % (title, len(rows)))
    for r in rows:
        print("  " + r)


if __name__ == "__main__":
    sys.exit(main())
