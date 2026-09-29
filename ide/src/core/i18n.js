// Texte aus einer Tabelle.
//
// Jeder sichtbare Text der IDE ist ein Schlüssel ("file.save"), kein Literal im
// Code. Englisch ist die Vorgabe und der Rückfall; eine weitere Sprache ist
// eine weitere Tabelle, die eine Erweiterung mitbringt — ohne Eingriff in
// irgendeine andere.
//
// Findet `t` keinen Eintrag, kommt der Schlüssel selbst zurück. Damit dürfen
// Beiträge auch einfach fertigen Text tragen (etwa den Dateinamen im Menü
// "Zuletzt geöffnet"), und ein vergessener Schlüssel fällt im UI sofort auf.

import { createEmitter } from './events.js';

/**
 * @param {{ language?: string, fallback?: string }} [opts]
 */
export function createI18n({ language = 'en', fallback = 'en' } = {}) {
	/** @type {Map<string, Array<Record<string, string>>>} */
	const tables = new Map();
	const events = createEmitter();
	let current = language;

	/**
	 * @param {Record<string, Record<string, string>>} messages  { en: {...}, de: {...} }
	 * @returns {() => void}
	 */
	function add(messages) {
		const added = [];
		for (const [lang, table] of Object.entries(messages || {})) {
			let list = tables.get(lang);
			if (!list) tables.set(lang, (list = []));
			list.push(table);
			added.push([list, table]);
		}
		events.emit('change');
		return () => {
			for (const [list, table] of added) {
				const i = list.indexOf(table);
				if (i >= 0) list.splice(i, 1);
			}
			events.emit('change');
		};
	}

	function lookup(lang, key) {
		const list = tables.get(lang);
		if (!list) return undefined;
		// Später hinzugefügte Tabellen gewinnen: eine Erweiterung darf Texte
		// einer anderen überschreiben.
		for (let i = list.length - 1; i >= 0; i--) {
			if (Object.prototype.hasOwnProperty.call(list[i], key)) return list[i][key];
		}
		return undefined;
	}

	/**
	 * @param {string} key
	 * @param {Record<string, string | number>} [params]  ersetzt "{name}"
	 */
	function t(key, params) {
		if (typeof key !== 'string') return '';
		let text = lookup(current, key);
		if (text === undefined && current !== fallback) text = lookup(fallback, key);
		if (text === undefined) text = key;
		if (params) {
			text = text.replace(/\{(\w+)\}/g, (m, name) => (name in params ? String(params[name]) : m));
		}
		return text;
	}

	return {
		add,
		t,
		get language() {
			return current;
		},
		setLanguage(lang) {
			if (lang === current) return;
			current = lang;
			events.emit('change');
		},
		/** @param {() => void} fn */
		onDidChange: (fn) => events.on('change', fn)
	};
}
