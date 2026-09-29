// Gliederung: Funktionen, Typen, Marken.
//
// Die Original-IDE führt drei Listen unter dem Editor und liest sie zeilenweise
// (blitzide/editor.cpp, formatLine): eine Zeile, die mit `Function` beginnt,
// nennt eine Funktion; mit `Type` einen Typ; mit `.` eine Marke. Kein Parser
// der ganzen Datei — deshalb schnell und tolerant gegenüber Fehlern im Rest.
//
// Abweichungen vom Original, alle zum Besseren:
//   * eingerückte Zeilen zählen mit (das Original sieht nur Spalte 0)
//   * `Type` und `Function` müssen ein ganzes Wort sein (`typeName = 3` ist keine Typdeklaration)
//   * `EndFunction`/`EndType` sind keine Deklarationen

const FUNCTION_RE = /^\s*function\b\s*([A-Za-z_][A-Za-z0-9_]*)/i;
const TYPE_RE = /^\s*type\b\s*([A-Za-z_][A-Za-z0-9_]*)/i;
const LABEL_RE = /^\s*\.\s*([A-Za-z_][A-Za-z0-9_]*)/;

/**
 * @param {string} text  Quelltext mit beliebigen Zeilenenden
 * @returns {{ funcs: Entry[], types: Entry[], labels: Entry[] }}
 *   Einträge in Dateireihenfolge; `line` ist 1-basiert
 * @typedef {{ name: string, line: number }} Entry
 */
export function parseOutline(text) {
	const out = { funcs: [], types: [], labels: [] };
	const lines = String(text).split(/\r\n|\r|\n/);
	for (let i = 0; i < lines.length; i++) {
		const line = lines[i];
		if (!line || line.trimStart().startsWith(';')) continue;
		let m = FUNCTION_RE.exec(line);
		if (m) {
			out.funcs.push({ name: m[1], line: i + 1 });
			continue;
		}
		m = TYPE_RE.exec(line);
		if (m) {
			out.types.push({ name: m[1], line: i + 1 });
			continue;
		}
		m = LABEL_RE.exec(line);
		if (m) out.labels.push({ name: m[1], line: i + 1 });
	}
	return out;
}
