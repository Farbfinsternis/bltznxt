#!/usr/bin/env python3
"""Generiert src/compiler/commands.h aus den Runtime-Headern.

WEAK-17: Die Befehlstabelle war handgepflegt und wich von der Runtime ab —
`Print` stand ohne Typ da, `Rand` mit einem Parameter statt einem oder zwei,
`Text` mit fünf Pflichtparametern von denen zwei optional sind. Damit war sie
für den semantischen Pass unbrauchbar.

Einzige Wahrheit ist jetzt die Runtime selbst: dieses Skript liest die
`inline`-Signaturen aus `src/compiler/bb_*.h` und schreibt daraus die Tabelle.
Was der Compiler über einen Befehl weiß, ist damit per Konstruktion das, was
der erzeugte C++-Code tatsächlich aufruft.

Aufruf aus dem Projektwurzelverzeichnis:

    python scripts/gen_commands.py            # schreibt src/compiler/commands.h
    python scripts/gen_commands.py --check    # meldet nur Abweichungen, Exit 1
"""

import glob
import io
import os
import re
import sys

RET_MAP = {"void": "", "int": "%", "bool": "%", "float": "#", "double": "#",
           "bbString": "$"}
PARAM_MAP = {"int": "%", "bool": "%", "float": "#", "double": "#",
             "bbString": "$", "const bbString &": "$", "const bbString&": "$"}

# Interne Helfer, die keine Blitz3D-Befehle sind. Alles mit abschliessendem
# Unterstrich gilt ohnehin als intern.
SKIP = {"Main"}

SIG_RE = re.compile(r"^inline\s+(void|int|float|double|bool|bbString)\s+"
                    r"bb_([A-Z][A-Za-z0-9_]*)\s*\(", re.MULTILINE)

# Print, Write, Min und Max nehmen in der Runtime jeden Typ (Templates). Der
# Typ ist damit nicht festgelegt — '.' heisst hier wie sonst 'beliebig'.
TEMPLATE_RE = re.compile(r"^template\s*<[^>]*>\s*inline\s+[A-Za-z_][\w:<>&, ]*?\s+"
                         r"bb_([A-Z][A-Za-z0-9_]*)\s*\(", re.MULTILINE)

# Blitz3D-Konstanten, die keine Funktionen sind und deshalb nicht in den
# Signaturen auftauchen. Der Emitter bildet sie direkt ab (visit(VarExpr)).
CONSTANTS = [("Pi", "#")]


def split_params(text):
    """Zerlegt eine Parameterliste an Kommas der obersten Klammerebene."""
    out, depth, cur = [], 0, ""
    for ch in text:
        if ch in "(<[":
            depth += 1
        elif ch in ")>]":
            depth -= 1
        if ch == "," and depth == 0:
            out.append(cur)
            cur = ""
        else:
            cur += ch
    if cur.strip():
        out.append(cur)
    return [p.strip() for p in out if p.strip()]


def parse_param(p, index):
    """('name', '%', optional) oder None, wenn der Typ nicht abbildbar ist.

    Die Runtime schreibt ungenutzte Parameter als `int /*frame*/ = 0` oder
    ganz ohne Namen (`int`). Beides ist eine gueltige Signatur, also wird der
    Kommentar als Name genommen und sonst einer erfunden.
    """
    optional = "=" in p
    decl = p.split("=")[0].strip()

    commented = re.search(r"/\*\s*([A-Za-z_][A-Za-z0-9_]*)\s*\*/", decl)
    decl = re.sub(r"/\*.*?\*/", " ", decl).strip()

    m = re.match(r"^(.*?)([A-Za-z_][A-Za-z0-9_]*)$", decl)
    if m and m.group(1).strip():
        type_part, name = m.group(1).strip(), m.group(2)
    else:
        # namenlos: der ganze Rest ist der Typ
        type_part = decl
        name = commented.group(1) if commented else "arg%d" % index
    if commented and m and m.group(1).strip() == "":
        name = commented.group(1)
    type_part = re.sub(r"\s+", " ", type_part).strip()
    if type_part.endswith("&"):
        type_part = type_part[:-1].strip() + " &"
    if type_part not in PARAM_MAP:
        return None
    return name, PARAM_MAP[type_part], optional


def scan_header(path):
    """[(name, ret, [(pname, ptype, optional)])] fuer eine Header-Datei."""
    src = io.open(path, encoding="utf-8", newline="").read()
    found = []
    for m in SIG_RE.finditer(src):
        name = m.group(2)
        if name in SKIP or name.endswith("_"):
            continue
        # Parameterliste bis zur passenden schliessenden Klammer einlesen
        i = src.index("(", m.start())
        depth, j = 0, i
        while j < len(src):
            if src[j] == "(":
                depth += 1
            elif src[j] == ")":
                depth -= 1
                if depth == 0:
                    break
            j += 1
        params = []
        ok = True
        for idx, p in enumerate(split_params(src[i + 1:j]), 1):
            parsed = parse_param(p, idx)
            if parsed is None:
                ok = False
                break
            params.append(parsed)
        if not ok:
            continue
        found.append((name, RET_MAP[m.group(1)], params))

    for m in TEMPLATE_RE.finditer(src):
        name = m.group(1)
        if name in SKIP or name.endswith("_"):
            continue
        i = src.index("(", m.start())
        depth, j = 0, i
        while j < len(src):
            if src[j] == "(":
                depth += 1
            elif src[j] == ")":
                depth -= 1
                if depth == 0:
                    break
            j += 1
        params = []
        for idx, p in enumerate(split_params(src[i + 1:j]), 1):
            pm = re.match(r"^.*?([A-Za-z_][A-Za-z0-9_]*)$", p.split("=")[0].strip())
            params.append((pm.group(1) if pm else "arg%d" % idx, ".", "=" in p))
        found.append((name, ".", params))
    return found


def merge(entries):
    """Ueberladungen zu einem Eintrag verschmelzen.

    Rand(int) und Rand(int,int) werden zu einem Befehl mit einem
    Pflicht- und einem optionalen Parameter. Wo sich Typen an derselben
    Position unterscheiden, steht '.' fuer 'beliebig'.
    """
    merged = {}
    for name, ret, params in entries:
        if name not in merged:
            merged[name] = {"ret": ret, "params": [list(p) for p in params],
                            "min": len(params)}
            continue
        cur = merged[name]
        cur["min"] = min(cur["min"], len(params))
        if ret != cur["ret"]:
            cur["ret"] = "."
        # Die breiteste Ueberladung liefert die Parameternamen: bb_Rand(max)
        # und bb_Rand(min, max) sollen als "min, max" erscheinen, nicht als
        # "max, max".
        widest = len(params) > len(cur["params"])
        for idx, (pname, ptype, opt) in enumerate(params):
            if idx < len(cur["params"]):
                if cur["params"][idx][1] != ptype:
                    cur["params"][idx][1] = "."
                if widest:
                    cur["params"][idx][0] = pname
            else:
                cur["params"].append([pname, ptype, True])
    for name, info in merged.items():
        required = 0
        for idx, p in enumerate(info["params"]):
            if idx >= info["min"] or p[2]:
                p[2] = True
            else:
                required += 1
        info["required"] = required
    return merged


def render(merged):
    rows = []
    for name in sorted(merged, key=str.lower):
        info = merged[name]
        parts = []
        for pname, ptype, opt in info["params"]:
            parts.append(pname + (ptype if ptype != "." else "") + ("?" if opt else ""))
        rows.append((name, info["ret"], ",".join(parts)))

    w_name = max(len(r[0]) for r in rows) + 3
    lines = []
    lines.append("#ifndef BLITZNEXT_COMMANDS_H")
    lines.append("#define BLITZNEXT_COMMANDS_H")
    lines.append("")
    lines.append('#include "lexer.h" // toLower')
    lines.append("#include <string>")
    lines.append("#include <unordered_map>")
    lines.append("")
    lines.append("// ---- Known commands -------------------------------------------------------")
    lines.append("//")
    lines.append("// ERZEUGT von scripts/gen_commands.py aus den Runtime-Headern")
    lines.append("// (src/compiler/bb_*.h). NICHT VON HAND BEARBEITEN — Aenderungen gehen beim")
    lines.append("// naechsten Lauf verloren. Wer einen Befehl hinzufuegt, schreibt ihn in die")
    lines.append("// Runtime und laesst das Skript neu laufen; `--check` meldet Abweichungen.")
    lines.append("//")
    lines.append("// Damit ist die Tabelle per Konstruktion das, was der erzeugte C++-Code")
    lines.append("// tatsaechlich aufruft (WEAK-17).")
    lines.append("//")
    lines.append("// Felder:")
    lines.append('//   name   kanonische Schreibweise, z.B. "Print"')
    lines.append('//   ret    Rueckgabetyp: "%" int, "#" float, "$" string, "" void,')
    lines.append('//          "." uneinheitlich ueber Ueberladungen hinweg')
    lines.append("//   params Kommaliste \"name<typ>[?]\", '?' = optional, fehlender Typ =")
    lines.append('//          beliebig. Beispiel: "handle%,frame%?"')
    lines.append("")
    lines.append("struct CmdInfo {")
    lines.append("  const char *name;")
    lines.append("  const char *ret;")
    lines.append("  const char *params;")
    lines.append("};")
    lines.append("")
    lines.append("inline constexpr CmdInfo kCommands[] = {")
    for name, ret, params in rows:
        n = '"%s",' % name
        lines.append("  { %-*s %-4s %s }," % (w_name, n, '"%s",' % ret, '"%s"' % params))
    lines.append("};")
    lines.append("")
    lines.append("// Blitz3D is case-insensitive, the generated C++ is not: the runtime function")
    lines.append("// for Print is bb_Print, whatever the source spelled. Returns the canonical")
    lines.append("// spelling from kCommands[] for any casing of a built-in, or nullptr when the")
    lines.append("// name is not a built-in command.")
    lines.append("inline const char *canonicalCommand(const std::string &name) {")
    lines.append("  static const std::unordered_map<std::string, const char *> byLower = [] {")
    lines.append("    std::unordered_map<std::string, const char *> m;")
    lines.append("    for (const auto &c : kCommands) m.emplace(toLower(c.name), c.name);")
    lines.append("    return m;")
    lines.append("  }();")
    lines.append("  auto it = byLower.find(toLower(name));")
    lines.append("  return (it == byLower.end()) ? nullptr : it->second;")
    lines.append("}")
    lines.append("")
    lines.append("// Menschenlesbare Signatur fuer -k / +k: \"s$, n%\" — das '?' der optionalen")
    lines.append("// Parameter wird zu \"[...]\", damit die Ausgabe lesbar bleibt.")
    lines.append("inline std::string commandSignature(const CmdInfo &c) {")
    lines.append("  std::string out, rest = c.params;")
    lines.append("  size_t pos = 0;")
    lines.append("  while (pos < rest.size()) {")
    lines.append("    size_t comma = rest.find(',', pos);")
    lines.append("    std::string tok = rest.substr(pos, comma == std::string::npos")
    lines.append("                                          ? std::string::npos")
    lines.append("                                          : comma - pos);")
    lines.append("    pos = (comma == std::string::npos) ? rest.size() : comma + 1;")
    lines.append("    bool optional = !tok.empty() && tok.back() == '?';")
    lines.append("    if (optional) tok.pop_back();")
    lines.append("    if (!out.empty()) out += \", \";")
    lines.append("    out += optional ? (\"[\" + tok + \"]\") : tok;")
    lines.append("  }")
    lines.append("  return out;")
    lines.append("}")
    lines.append("")
    lines.append("#endif // BLITZNEXT_COMMANDS_H")
    return "\n".join(lines) + "\n"


def main():
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    headers = sorted(glob.glob(os.path.join(root, "src", "compiler", "bb_*.h")))
    entries = []
    for h in headers:
        entries += scan_header(h)
    for cname, cret in CONSTANTS:
        entries.append((cname, cret, []))
    merged = merge(entries)
    text = render(merged)

    out = os.path.join(root, "src", "compiler", "commands.h")
    if "--check" in sys.argv:
        old = io.open(out, encoding="utf-8", newline="").read() if os.path.exists(out) else ""
        if old.replace("\r\n", "\n") != text:
            print("commands.h weicht von den Runtime-Headern ab "
                  "(scripts/gen_commands.py neu laufen lassen)")
            return 1
        print("commands.h ist aktuell (%d Befehle)" % len(merged))
        return 0

    io.open(out, "w", encoding="utf-8", newline="\n").write(text)
    print("commands.h geschrieben: %d Befehle aus %d Headern"
          % (len(merged), len(headers)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
