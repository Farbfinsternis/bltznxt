// Die eingebauten Erweiterungen.
//
// Eine Liste, sonst nichts: wer eine entfernt, ersetzt oder hinzufügt, ändert
// nur diese Datei. Die Reihenfolge ist gleichgültig — `dependsOn` und Dienste
// (`waitFor`) regeln, wer wen braucht.
//
// Kommen später Erweiterungen aus einem Benutzerordner, hängen sie hier
// dieselbe Form an; für die Paritätsphase (PLAN.md, P1–P5) kommen die
// eingebauten Funktionen als weitere Einträge dazu.

import workbench from './workbench/index.js';
import toolchain from './toolchain/index.js';
import languageBlitz from './language-blitz/index.js';
import themes from './themes/index.js';
import editor from './editor/index.js';
import outline from './outline/index.js';
import files from './files/index.js';
import buildRun from './build-run/index.js';

export const builtinExtensions = [workbench, themes, toolchain, languageBlitz, editor, outline, files, buildRun];
