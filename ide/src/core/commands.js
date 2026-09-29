// Befehlsregister.
//
// Menüs, Tastenkürzel, Symbolleiste, Kontextmenüs und später eine
// Befehlspalette verweisen ausschließlich auf Befehls-IDs — so wie die
// `ID_*`-Konstanten der Original-IDE, nur offen für jeden. Wer einen Befehl
// ersetzen will, registriert ihn unter derselben ID neu (nachdem er den alten
// abgemeldet hat); wer ihn auslösen will, ruft `execute`.

import { createEmitter } from './events.js';

/**
 * @typedef {object} CommandDef
 * @property {string} id            eindeutig, z.B. "file.save"
 * @property {string} [title]       Text oder Nachrichtenschlüssel
 * @property {string} [icon]        SVG-Text oder ein kurzes Zeichen
 * @property {string | (() => boolean)} [enabledWhen]  when-Ausdruck
 * @property {(...args: any[]) => any} run
 */

/**
 * @param {{ context: ReturnType<typeof import('./context.js').createContext> }} deps
 */
export function createCommands({ context }) {
	/** @type {Map<string, CommandDef>} */
	const commands = new Map();
	const events = createEmitter();

	/**
	 * @param {CommandDef} def
	 * @returns {() => void} meldet den Befehl wieder ab
	 */
	function register(def) {
		if (!def || typeof def.id !== 'string' || !def.id) {
			throw new Error('Befehl ohne id');
		}
		if (typeof def.run !== 'function') {
			throw new Error(`Befehl "${def.id}" hat keine run-Funktion`);
		}
		if (commands.has(def.id)) {
			throw new Error(`Befehl "${def.id}" ist schon registriert`);
		}
		commands.set(def.id, def);
		events.emit('change', { id: def.id, kind: 'added' });
		return () => {
			// Nur den eigenen Eintrag entfernen: ist die ID inzwischen neu
			// vergeben, darf das späte Abmelden den Nachfolger nicht treffen.
			if (commands.get(def.id) === def) {
				commands.delete(def.id);
				events.emit('change', { id: def.id, kind: 'removed' });
			}
		};
	}

	const has = (id) => commands.has(id);
	const get = (id) => commands.get(id);
	const list = () => [...commands.values()];

	/** Freigegeben = registriert und `enabledWhen` erfüllt. */
	function isEnabled(id) {
		const def = commands.get(id);
		return Boolean(def) && context.evaluate(def.enabledWhen);
	}

	/**
	 * Führt einen Befehl aus. Unbekannte ID: Fehler (ein Tippfehler in einem
	 * Menü soll auffallen). Gesperrter Befehl: kein Fehler, kein Lauf — die
	 * Rückgabe ist dann `{ executed: false }`.
	 * @returns {Promise<{ executed: boolean, result?: any }>}
	 */
	async function execute(id, ...args) {
		const def = commands.get(id);
		if (!def) throw new Error(`Unbekannter Befehl "${id}"`);
		if (!context.evaluate(def.enabledWhen)) return { executed: false };
		const result = await def.run(...args);
		events.emit('executed', { id, args });
		return { executed: true, result };
	}

	return {
		register,
		has,
		get,
		list,
		isEnabled,
		execute,
		onDidChange: (fn) => events.on('change', fn),
		onDidExecute: (fn) => events.on('executed', fn)
	};
}
