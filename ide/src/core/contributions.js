// Beiträge (Contribution Points).
//
// Ein Erweiterungspunkt ist nur ein Name: "menus", "keybindings", "toolbar",
// "views" ... Jeder darf dazu Daten beitragen, jeder darf sie lesen. Der Kern
// kennt keine Liste erlaubter Punkte — eine Erweiterung, die einen neuen
// Punkt erfindet (etwa "snippets"), braucht dafür keine Änderung am Kern,
// nur einen Leser, der ihn versteht.
//
// Beiträge sind änderbar: `update` ersetzt die Daten eines Beitrags, damit
// etwa die Liste der zuletzt geöffneten Dateien ein lebendiger Menübeitrag
// sein kann statt einer Sonderlösung.

import { createEmitter } from './events.js';

export function createContributions() {
	/** @type {Map<string, Array<{ owner: string, items: any[] }>>} */
	const byPoint = new Map();
	const events = createEmitter();

	/**
	 * @param {string} point
	 * @param {any[]} items
	 * @param {string} [owner]
	 * @returns {{ dispose(): void, update(items: any[]): void }}
	 */
	function contribute(point, items, owner = '?') {
		if (!Array.isArray(items)) throw new Error(`Beitrag zu "${point}" muss eine Liste sein`);
		let list = byPoint.get(point);
		if (!list) byPoint.set(point, (list = []));
		const entry = { owner, items: [...items] };
		list.push(entry);
		events.emit(point, { point, owner });
		return {
			dispose() {
				const i = list.indexOf(entry);
				if (i >= 0) {
					list.splice(i, 1);
					events.emit(point, { point, owner });
				}
			},
			update(next) {
				entry.items = [...next];
				events.emit(point, { point, owner });
			}
		};
	}

	/** Alle Einträge eines Punkts in der Reihenfolge der Beiträge. */
	function get(point) {
		const list = byPoint.get(point);
		return list ? list.flatMap((e) => e.items) : [];
	}

	/** Wie `get`, aber mit dem Besitzer je Eintrag. */
	function getEntries(point) {
		const list = byPoint.get(point);
		return list ? list.flatMap((e) => e.items.map((item) => ({ owner: e.owner, item }))) : [];
	}

	/** @param {(e: {point: string, owner: string}) => void} fn */
	const onDidChange = (point, fn) => events.on(point, fn);

	return { contribute, get, getEntries, onDidChange };
}
