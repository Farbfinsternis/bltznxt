// Einstellungen.
//
// Geschichtet: Vorgabe (aus dem Schema der Erweiterung) < Benutzer < Ordner.
// Schlüssel sind flach und mit Punkt geschrieben ("editor.tabSize"), das
// Schema kommt von der Erweiterung, der sie gehören:
//
//   { 'editor.tabSize': { type: 'number', default: 4, description: '...' } }
//
// Zwei Regeln, die das Original-Format ("ein unbekannter Schlüssel setzt alles
// zurück") bewusst nicht hatte:
//   1. Unbekannte Schlüssel bleiben in der Datei erhalten. Sie gehören einer
//      Erweiterung, die gerade nicht geladen ist, oder einer neueren Version.
//   2. Ein Wert vom falschen Typ wird ignoriert (es gilt die Vorgabe), aber
//      nicht gelöscht — der Benutzer sieht und korrigiert ihn in der Datei.
//
// Das Dateiformat trägt eine Version. Ändert sich der Aufbau, kommt eine
// Migration dazu, kein Bruch.

import { createEmitter } from './events.js';

export const SETTINGS_VERSION = 1;

const LAYERS = ['user', 'folder'];

/** @param {any} schemaEntry @param {any} value */
function matchesSchema(schemaEntry, value) {
	if (!schemaEntry) return true;
	if (schemaEntry.enum && !schemaEntry.enum.includes(value)) return false;
	switch (schemaEntry.type) {
		case 'boolean': return typeof value === 'boolean';
		case 'number': return typeof value === 'number' && Number.isFinite(value);
		case 'string': return typeof value === 'string';
		case 'array': return Array.isArray(value);
		case 'object': return value !== null && typeof value === 'object' && !Array.isArray(value);
		default: return true;
	}
}

/**
 * @param {{ migrations?: Record<number, (settings: Record<string, any>) => Record<string, any>> }} [opts]
 *   migrations[n] hebt Einstellungen der Version n auf n+1.
 */
export function createSettings({ migrations = {} } = {}) {
	/** @type {Map<string, any>} */
	const schema = new Map();
	/** @type {Record<string, Record<string, any>>} */
	const layers = { user: {}, folder: {} };
	const events = createEmitter();
	let version = SETTINGS_VERSION;

	/**
	 * @param {Record<string, { type?: string, default?: any, description?: string, enum?: any[] }>} entries
	 * @returns {() => void}
	 */
	function registerSchema(entries) {
		const keys = [];
		for (const [key, def] of Object.entries(entries)) {
			if (schema.has(key)) throw new Error(`Einstellung "${key}" ist schon vergeben`);
			schema.set(key, def);
			keys.push(key);
		}
		events.emit('schema', { keys });
		return () => {
			for (const key of keys) schema.delete(key);
			events.emit('schema', { keys });
		};
	}

	/**
	 * @param {string} key
	 * @param {any} [fallback]  gilt nur, wenn es weder Wert noch Schema-Vorgabe gibt
	 */
	function get(key, fallback) {
		const def = schema.get(key);
		for (const layer of ['folder', 'user']) {
			if (Object.prototype.hasOwnProperty.call(layers[layer], key)) {
				const value = layers[layer][key];
				if (matchesSchema(def, value)) return value;
			}
		}
		return def && 'default' in def ? def.default : fallback;
	}

	/**
	 * @param {string} key
	 * @param {any} value
	 * @param {{ layer?: 'user' | 'folder' }} [opts]
	 */
	function set(key, value, { layer = 'user' } = {}) {
		if (!LAYERS.includes(layer)) throw new Error(`Unbekannte Schicht "${layer}"`);
		const def = schema.get(key);
		if (!matchesSchema(def, value)) {
			throw new Error(`Einstellung "${key}": Wert ${JSON.stringify(value)} passt nicht zum Schema`);
		}
		const before = get(key);
		layers[layer][key] = value;
		const after = get(key);
		if (before !== after) events.emit('change', { key, value: after, layer });
	}

	/** Wert der Schicht entfernen; es gilt wieder die darunterliegende. */
	function reset(key, { layer = 'user' } = {}) {
		if (!Object.prototype.hasOwnProperty.call(layers[layer], key)) return;
		const before = get(key);
		delete layers[layer][key];
		const after = get(key);
		if (before !== after) events.emit('change', { key, value: after, layer });
	}

	/**
	 * Benutzereinstellungen aus dem Text einer Datei laden. Nie ein Wurf: ein
	 * kaputter Text ergibt `{ ok: false, error }` und leere Einstellungen — die
	 * Datei zu sichern, bevor sie überschrieben wird, ist Sache des Aufrufers.
	 * @param {string | null | undefined} text  leer/`null` = keine Datei
	 * @returns {{ ok: boolean, error?: string, migrated?: boolean }}
	 */
	function load(text) {
		const before = new Map([...schema.keys()].map((k) => [k, get(k)]));
		layers.user = {};
		version = SETTINGS_VERSION;
		let result = { ok: true };

		if (text && text.trim()) {
			try {
				const doc = JSON.parse(text);
				if (doc === null || typeof doc !== 'object' || Array.isArray(doc)) {
					throw new Error('Wurzel ist kein Objekt');
				}
				let from = Number.isInteger(doc.version) ? doc.version : SETTINGS_VERSION;
				let data = doc.settings && typeof doc.settings === 'object' ? { ...doc.settings } : {};
				// Von einer neueren Version geschrieben: wir lesen, was wir verstehen,
				// lassen den Rest stehen und behalten die Versionsnummer, damit die
				// neuere Version die Datei beim nächsten Start nicht für alt hält.
				version = Math.max(from, SETTINGS_VERSION);
				const start = from;
				while (from < SETTINGS_VERSION) {
					const step = migrations[from];
					if (!step) throw new Error(`Keine Migration von Version ${from}`);
					data = step(data);
					from++;
				}
				layers.user = data;
				if (start < SETTINGS_VERSION) result = { ok: true, migrated: true };
			} catch (err) {
				layers.user = {};
				result = { ok: false, error: String(err.message || err) };
			}
		}

		for (const [key, was] of before) {
			const now = get(key);
			if (was !== now) events.emit('change', { key, value: now, layer: 'user' });
		}
		return result;
	}

	/** @returns {string} der Text für die Datei */
	function serialize() {
		return JSON.stringify({ version, settings: layers.user }, null, '\t') + '\n';
	}

	/** Die Ordner-Schicht ersetzen (Einstellungen pro Ordner, später). */
	function setFolderLayer(values) {
		const before = new Map([...schema.keys()].map((k) => [k, get(k)]));
		layers.folder = { ...(values || {}) };
		for (const [key, was] of before) {
			const now = get(key);
			if (was !== now) events.emit('change', { key, value: now, layer: 'folder' });
		}
	}

	return {
		registerSchema,
		get,
		set,
		reset,
		load,
		serialize,
		setFolderLayer,
		/** Beschreibung eines Schlüssels (für die Einstellungsseite). */
		describe: (key) => schema.get(key),
		keys: () => [...schema.keys()],
		/** @param {(e: {key: string, value: any, layer: string}) => void} fn */
		onDidChange: (fn) => events.on('change', fn),
		onDidChangeSchema: (fn) => events.on('schema', fn)
	};
}
