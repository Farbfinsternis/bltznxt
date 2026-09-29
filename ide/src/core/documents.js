// Dokumentmodell.
//
// Ein Dokument ist Text mit Herkunft. Es kennt weder Editor noch Tab noch
// Festplatte: der Editor liest und schreibt `text`, die Dateierweiterung
// füllt und speichert es, die Tab-Leiste zeigt es an. Deshalb kann jede dieser
// Schichten ersetzt werden.
//
//   kind 'file'     hat einen Pfad (`uri`), auch wenn er noch nicht existiert
//   kind 'scratch'  namenlos: "einfach schnell etwas schreiben". Bekommt erst
//                   beim Speichern einen Pfad und wird dann zur Datei.
//
// Kodierung und Zeilenende gehören zum Dokument, weil eine Datei beim
// Speichern so herauskommen muss, wie sie hereinkam (Windows-1252, CRLF, ...).
// `text` selbst hat immer LF als Zeilenende; `eol` gilt erst beim Schreiben.
// So ist "geändert?" ein einfacher Textvergleich, und kein Editor muss wissen,
// welches Zeilenende die Datei hat.

import { createEmitter } from './events.js';

/**
 * @typedef {object} Document
 * @property {number} id
 * @property {string | null} uri          Pfad, `null` bei namenlosen
 * @property {'file' | 'scratch'} kind
 * @property {string} title               Dateiname oder "<untitled>"
 * @property {string} text
 * @property {string} savedText           Stand der letzten Speicherung
 * @property {boolean} dirty
 * @property {string} encoding            z.B. 'utf-8', 'windows-1252'
 * @property {'\n' | '\r\n'} eol
 */

const basename = (p) => String(p).split(/[\\/]/).pop();
const lf = (t) => String(t).replace(/\r\n?/g, '\n');

/**
 * @param {{ caseInsensitivePaths?: boolean }} [opts]
 *   Windows vergleicht Pfade ohne Rücksicht auf Groß-/Kleinschreibung (wie
 *   die Original-IDE beim "Ist die Datei schon offen?").
 */
export function createDocuments({ caseInsensitivePaths = true } = {}) {
	/** @type {Map<number, Document>} */
	const docs = new Map();
	const events = createEmitter();
	let nextId = 1;
	let untitledCount = 0;
	/** @type {number | null} */
	let activeId = null;

	const samePath = (a, b) =>
		caseInsensitivePaths ? String(a).toLowerCase() === String(b).toLowerCase() : a === b;

	function makeTitle(uri, untitledNumber) {
		if (uri) return basename(uri);
		return untitledNumber > 1 ? `<untitled ${untitledNumber}>` : '<untitled>';
	}

	function wrap(doc) {
		// `dirty` und `title` folgen aus dem Rest — nie getrennt gespeichert.
		return Object.defineProperties(doc, {
			dirty: { get() { return this.text !== this.savedText; }, enumerable: true },
			title: { get() { return makeTitle(this.uri, this._untitled); }, enumerable: true }
		});
	}

	/**
	 * Öffnet ein Dokument. Ist der Pfad schon offen, kommt das vorhandene
	 * zurück (und wird aktiv) — wie im Original.
	 * @param {{ uri?: string | null, text?: string, kind?: 'file'|'scratch', encoding?: string, eol?: '\n'|'\r\n', activate?: boolean }} [init]
	 * @returns {Document}
	 */
	function open(init = {}) {
		const uri = init.uri ?? null;
		if (uri) {
			for (const d of docs.values()) {
				if (d.uri && samePath(d.uri, uri)) {
					activate(d.id);
					return d;
				}
			}
		}
		const text = lf(init.text ?? '');
		const doc = wrap({
			id: nextId++,
			uri,
			kind: init.kind ?? (uri ? 'file' : 'scratch'),
			text,
			savedText: uri ? text : '',
			encoding: init.encoding ?? 'utf-8',
			eol: init.eol ?? '\n',
			_untitled: uri ? 0 : ++untitledCount
		});
		docs.set(doc.id, doc);
		events.emit('opened', doc);
		if (init.activate !== false) activate(doc.id);
		return doc;
	}

	const get = (id) => docs.get(id);
	const list = () => [...docs.values()];
	const find = (uri) => list().find((d) => d.uri && samePath(d.uri, uri));

	function activate(id) {
		if (id !== null && !docs.has(id)) throw new Error(`Unbekanntes Dokument ${id}`);
		if (activeId === id) return;
		activeId = id;
		events.emit('activated', id === null ? null : docs.get(id));
	}

	/**
	 * Schließt ein Dokument ohne Rückfrage — die Rückfrage bei Änderungen ist
	 * Sache des Befehls, der schließt. Das Nachbar-Dokument wird aktiv.
	 * @returns {Document | undefined}
	 */
	function close(id) {
		const doc = docs.get(id);
		if (!doc) return undefined;
		const ids = [...docs.keys()];
		const index = ids.indexOf(id);
		docs.delete(id);
		events.emit('closed', doc);
		if (activeId === id) {
			const rest = ids.filter((i) => i !== id);
			activeId = null;
			const next = rest[Math.min(index, rest.length - 1)];
			if (next !== undefined) activate(next);
			else events.emit('activated', null);
		}
		return doc;
	}

	function setText(id, text) {
		const doc = docs.get(id);
		if (!doc) throw new Error(`Unbekanntes Dokument ${id}`);
		text = lf(text);
		if (doc.text === text) return;
		doc.text = text;
		events.emit('changed', doc);
	}

	/**
	 * Merkt sich den jetzigen Text als gespeichert. Mit `uri` (Speichern
	 * unter, oder erstes Speichern eines namenlosen) bekommt das Dokument
	 * einen neuen Ort.
	 * @param {number} id
	 * @param {{ uri?: string, encoding?: string, eol?: '\n'|'\r\n' }} [where]
	 */
	function markSaved(id, where = {}) {
		const doc = docs.get(id);
		if (!doc) throw new Error(`Unbekanntes Dokument ${id}`);
		const moved = where.uri !== undefined && where.uri !== doc.uri;
		if (where.uri !== undefined) {
			doc.uri = where.uri;
			doc.kind = 'file';
		}
		if (where.encoding) doc.encoding = where.encoding;
		if (where.eol) doc.eol = where.eol;
		doc.savedText = doc.text;
		events.emit('saved', doc);
		if (moved) events.emit('renamed', doc);
	}

	return {
		open,
		get,
		find,
		list,
		activate,
		close,
		setText,
		markSaved,
		get active() {
			return activeId === null ? null : docs.get(activeId) ?? null;
		},
		/** @param {'opened'|'closed'|'activated'|'changed'|'saved'|'renamed'} name */
		on: (name, fn) => events.on(name, fn)
	};
}
