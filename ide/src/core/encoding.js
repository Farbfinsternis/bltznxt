// Dateikodierung: Bytes <-> Text, verlustfrei.
//
// Blitz3D-Quelltexte sind Bytes, meist Windows-1252, nicht UTF-8; der Editor
// denkt in UTF-16. Ein naives "als UTF-8 lesen und schreiben" zerstört die
// Umlaute bestehender Programme. Deshalb:
//
//   Lesen     BOM -> 'utf-8-bom'; sonst gültiges UTF-8 mit mindestens einem
//             Nicht-ASCII-Byte -> 'utf-8'; alles andere (auch reines ASCII)
//             -> Windows-1252. Windows-1252 kennt für jedes der 256 Bytes genau
//             ein Zeichen, jede Datei lässt sich also ohne Verlust lesen und
//             als dieselben Bytes wieder schreiben.
//   Schreiben Der Text hat LF (siehe documents.js); hier wird es zum
//             Zeilenende der Datei (`eol`). Zeichen, die die Kodierung nicht
//             kennt, werden nicht still ersetzt, sondern gemeldet.
//
// Bekannte Grenze: Eine Datei mit gemischten Zeilenenden (CRLF und LF) kommt
// nach dem Speichern mit dem häufigeren heraus. `decodeFile` meldet das in
// `mixedEol`, damit ein Aufrufer warnen kann.
//
// Reines JavaScript ohne Node und ohne DOM (TextDecoder/TextEncoder gibt es in
// beiden), damit es sich unter `node --test` prüfen lässt.

/** @typedef {'windows-1252' | 'utf-8' | 'utf-8-bom'} Encoding */

export const ENCODINGS = ['windows-1252', 'utf-8', 'utf-8-bom'];

// Byte <-> Zeichen für Windows-1252, aus dem Decoder der Laufzeit gebaut —
// keine handgeschriebene Tabelle, die einen Tippfehler enthalten könnte.
const decoder1252 = new TextDecoder('windows-1252');
/** @type {string[]} */
const BYTE_TO_CHAR = [];
/** @type {Map<string, number>} */
const CHAR_TO_BYTE = new Map();
for (let b = 0; b < 256; b++) {
	const ch = decoder1252.decode(Uint8Array.of(b));
	BYTE_TO_CHAR.push(ch);
	CHAR_TO_BYTE.set(ch, b);
}

const utf8Strict = new TextDecoder('utf-8', { fatal: true, ignoreBOM: true });
const utf8Encoder = new TextEncoder();

function decode1252(bytes) {
	// Stückweise, damit große Dateien keine Argumentliste sprengen
	const parts = [];
	const CHUNK = 8192;
	for (let i = 0; i < bytes.length; i += CHUNK) {
		let s = '';
		const end = Math.min(bytes.length, i + CHUNK);
		for (let j = i; j < end; j++) s += BYTE_TO_CHAR[bytes[j]];
		parts.push(s);
	}
	return parts.join('');
}

function hasBom(bytes) {
	return bytes.length >= 3 && bytes[0] === 0xef && bytes[1] === 0xbb && bytes[2] === 0xbf;
}

function hasHighByte(bytes) {
	for (let i = 0; i < bytes.length; i++) if (bytes[i] >= 0x80) return true;
	return false;
}

/** Zeilenenden zählen: CRLF, einzelnes LF, einzelnes CR. */
function countEols(text) {
	let crlf = 0;
	let lf = 0;
	let cr = 0;
	for (let i = 0; i < text.length; i++) {
		const c = text.charCodeAt(i);
		if (c === 13) {
			if (text.charCodeAt(i + 1) === 10) {
				crlf++;
				i++;
			} else {
				cr++;
			}
		} else if (c === 10) {
			lf++;
		}
	}
	return { crlf, lf, cr };
}

/**
 * @param {Uint8Array} bytes
 * @param {{ defaultEol?: '\n' | '\r\n' }} [opts]  Zeilenende, wenn die Datei keines enthält
 * @returns {{ text: string, encoding: Encoding, eol: '\n' | '\r\n', mixedEol: boolean, binary: boolean }}
 *   `text` hat LF; `binary`: enthält NUL-Bytes, sieht also nicht nach Text aus
 */
export function decodeFile(bytes, { defaultEol = '\r\n' } = {}) {
	let encoding;
	let raw;

	if (hasBom(bytes)) {
		try {
			raw = utf8Strict.decode(bytes.subarray(3));
			encoding = 'utf-8-bom';
		} catch {
			// BOM, aber danach kein gültiges UTF-8: als Bytes lesen, nicht werfen
			encoding = 'windows-1252';
			raw = decode1252(bytes);
		}
	} else if (hasHighByte(bytes)) {
		try {
			raw = utf8Strict.decode(bytes);
			encoding = 'utf-8';
		} catch {
			encoding = 'windows-1252';
			raw = decode1252(bytes);
		}
	} else {
		// Reines ASCII: jede Kodierung liest es gleich; Blitz-Vorgabe ist 1252
		encoding = 'windows-1252';
		raw = decode1252(bytes);
	}

	const { crlf, lf, cr } = countEols(raw);
	const kinds = [crlf, lf, cr].filter((n) => n > 0).length;
	let eol = defaultEol;
	if (crlf + lf + cr > 0) eol = crlf >= lf + cr ? '\r\n' : '\n';

	let binary = false;
	for (let i = 0; i < bytes.length && i < 8000; i++) {
		if (bytes[i] === 0) {
			binary = true;
			break;
		}
	}

	return {
		text: raw.replace(/\r\n?/g, '\n'),
		encoding,
		eol,
		mixedEol: kinds > 1,
		binary
	};
}

/**
 * @param {string} text        mit LF
 * @param {Encoding} encoding
 * @param {'\n' | '\r\n'} eol
 * @returns {{ bytes: Uint8Array } | { unmappable: string[] }}
 *   `unmappable`: die (höchstens zehn verschiedenen) Zeichen, die die Kodierung nicht kennt
 */
export function encodeFile(text, encoding, eol) {
	const out = eol === '\r\n' ? text.replace(/\n/g, '\r\n') : text;

	if (encoding === 'utf-8' || encoding === 'utf-8-bom') {
		const body = utf8Encoder.encode(out);
		if (encoding === 'utf-8') return { bytes: body };
		const bytes = new Uint8Array(body.length + 3);
		bytes.set([0xef, 0xbb, 0xbf], 0);
		bytes.set(body, 3);
		return { bytes };
	}

	if (encoding !== 'windows-1252') throw new Error(`Unbekannte Kodierung "${encoding}"`);

	const bytes = new Uint8Array(out.length); // ein Zeichen = ein Byte; astrale scheitern ohnehin
	const missing = new Set();
	let n = 0;
	for (const ch of out) {
		const b = CHAR_TO_BYTE.get(ch);
		if (b === undefined) {
			if (missing.size < 10) missing.add(ch);
		} else if (missing.size === 0) {
			bytes[n++] = b;
		}
	}
	if (missing.size > 0) return { unmappable: [...missing] };
	return { bytes: bytes.subarray(0, n) };
}
