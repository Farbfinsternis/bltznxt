// Zustand: was die IDE sich merkt, ohne dass der Benutzer es einstellt.
//
// Zuletzt geöffnete Dateien, letzter Ordner, später Fensterposition und letzter
// Build. Getrennt von den Einstellungen (settings.js), weil beide Dateien
// verschiedene Besitzer haben: die Einstellungen bearbeitet der Benutzer, den
// Zustand schreibt die IDE — und eine Datei, die man bearbeitet, soll die IDE
// nicht bei jedem Öffnen einer Datei umschreiben.
//
// Kein Schema, keine Schichten: Schlüssel sind Text, Werte beliebiges JSON.
// Wie bei den Einstellungen bleiben unbekannte Schlüssel erhalten, und ein
// kaputter Text ergibt leeren Zustand statt eines Wurfs.

import { createEmitter } from './events.js';

export const STATE_VERSION = 1;

export function createState() {
	/** @type {Record<string, any>} */
	let values = {};
	const events = createEmitter();
	let version = STATE_VERSION;

	const same = (a, b) => JSON.stringify(a) === JSON.stringify(b);

	/** @param {string} key @param {any} [fallback] */
	function get(key, fallback) {
		return Object.prototype.hasOwnProperty.call(values, key) ? values[key] : fallback;
	}

	function set(key, value) {
		if (value === undefined) return remove(key);
		if (Object.prototype.hasOwnProperty.call(values, key) && same(values[key], value)) return;
		values[key] = value;
		events.emit('change', { key, value });
	}

	function remove(key) {
		if (!Object.prototype.hasOwnProperty.call(values, key)) return;
		delete values[key];
		events.emit('change', { key, value: undefined });
	}

	/** @returns {{ ok: boolean, error?: string }} */
	function load(text) {
		values = {};
		version = STATE_VERSION;
		if (!text || !text.trim()) return { ok: true };
		try {
			const doc = JSON.parse(text);
			if (doc === null || typeof doc !== 'object' || Array.isArray(doc)) throw new Error('Wurzel ist kein Objekt');
			if (Number.isInteger(doc.version)) version = Math.max(doc.version, STATE_VERSION);
			if (doc.state && typeof doc.state === 'object' && !Array.isArray(doc.state)) values = { ...doc.state };
			return { ok: true };
		} catch (err) {
			values = {};
			return { ok: false, error: String(err.message || err) };
		}
	}

	const serialize = () => JSON.stringify({ version, state: values }, null, '\t') + '\n';

	return {
		get,
		set,
		remove,
		load,
		serialize,
		/** @param {(e: {key: string, value: any}) => void} fn */
		onDidChange: (fn) => events.on('change', fn)
	};
}
