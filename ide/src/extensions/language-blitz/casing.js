// Schreibweise der Schlüsselwörter.
//
// Die Original-IDE korrigiert getippte Wörter zur Schreibweise der Befehlsliste
// (`graphics` -> `Graphics`), sobald der Cursor das Wort verlässt
// (blitzide/editor.cpp, formatLine). Hier die Regel als reine Funktion:
//
//   * Kommentare (ab `;`) und Zeichenketten bleiben unberührt
//   * Hexzahlen (`$FF00`) und Zahlen mit Suffix ebenso
//   * ein Wort, das den Cursor berührt, wird nicht angefasst — man tippt es ja noch
//   * Bezeichner sind [A-Za-z_][A-Za-z0-9_]*; die Typkennzeichen `$ % #` gehören nicht dazu
//     (`left$` wird `Left$`)

/**
 * Baut die Nachschlagetabelle klein -> Schreibweise. Bei doppelten Namen mit
 * unterschiedlicher Schreibweise gewinnt der erste.
 * @param {Iterable<string>} names
 * @returns {Map<string, string>}
 */
export function buildCaseMap(names) {
	const map = new Map();
	for (const name of names) {
		const key = name.toLowerCase();
		if (!map.has(key)) map.set(key, name);
	}
	return map;
}

const isDigit = (c) => c >= '0' && c <= '9';
const isIdStart = (c) => (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c === '_';
const isIdPart = (c) => isIdStart(c) || isDigit(c);
const isHex = (c) => isDigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');

/**
 * Was an einer Zeile zu ändern ist.
 * @param {string} line
 * @param {Map<string, string>} map
 * @param {number | null} cursorColumn  1-basiert; `null` = kein Cursor in dieser Zeile
 * @returns {Array<{ startColumn: number, endColumn: number, text: string }>}  1-basiert, Ende exklusiv
 */
export function casingEdits(line, map, cursorColumn = null) {
	const edits = [];
	let i = 0;
	while (i < line.length) {
		const c = line[i];
		if (c === ';') break; // Kommentar bis zum Zeilenende
		if (c === '"') {
			const end = line.indexOf('"', i + 1);
			i = end < 0 ? line.length : end + 1;
		} else if (c === '$' && i + 1 < line.length && isHex(line[i + 1])) {
			i++;
			while (i < line.length && isHex(line[i])) i++;
		} else if (isDigit(c)) {
			// Zahl, samt allem, was dranhängt (1e5, 12abc): keine Wörter darin
			while (i < line.length && (isIdPart(line[i]) || line[i] === '.')) i++;
		} else if (isIdStart(c)) {
			const start = i;
			while (i < line.length && isIdPart(line[i])) i++;
			const word = line.slice(start, i);
			const proper = map.get(word.toLowerCase());
			if (proper === undefined || proper === word) continue;
			// Spalten sind 1-basiert; der Cursor "berührt" das Wort auch direkt dahinter
			const first = start + 1;
			const after = i + 1;
			if (cursorColumn !== null && cursorColumn >= first && cursorColumn <= after) continue;
			edits.push({ startColumn: first, endColumn: after, text: proper });
		} else {
			i++;
		}
	}
	return edits;
}
