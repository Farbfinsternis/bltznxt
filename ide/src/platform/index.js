// Plattform-Schnittstelle der IDE.
//
// Der gesamte Renderer-Code spricht nur über dieses Modul mit der Außenwelt.
// Wer hier `window.bltznxt` umgeht und direkt Node benutzt, macht aus dem
// späteren Backend-Wechsel (Electron -> Tauri) einen Umbau der ganzen IDE
// statt eines Austauschs dieser Datei.
//
// Das Modul ist absichtlich dumm: es reicht Aufrufe an Dienste des Backends
// durch (`invoke`) und kennt keinen einzigen davon. Was ein Dienst kann,
// beschreiben die Erweiterungen, die ihn benutzen (etwa extensions/toolchain).
//
// Zweite Regel, siehe README.md "Repository Layout": die IDE weiß nichts über
// den Compiler außer seiner Kommandozeile. Insbesondere wird die Liste der
// eingebauten Befehle bei jedem Start frisch per `blitzcc +k` erfragt und
// nicht hier einkopiert.

const bridge = typeof window !== 'undefined' ? window.bltznxt : undefined;

/** Läuft die IDE mit angebundenem Backend? Im reinen Browser (vite dev) nicht. */
export const hasBackend = Boolean(bridge);

/**
 * Ruft eine Methode eines Backend-Dienstes auf.
 * @param {string} service  z.B. 'toolchain', 'store'
 * @param {string} method
 * @param {...any} args     müssen strukturiert kopierbar sein (kein Funktionen, keine Klassen)
 * @returns {Promise<any>}
 */
export function invoke(service, method, ...args) {
	if (!bridge) {
		return Promise.reject(
			new Error(`platform.invoke(${service}.${method}) ist ohne Backend nicht verfügbar. Vorher hasBackend prüfen.`)
		);
	}
	return bridge.invoke(service, method, ...args);
}

/**
 * Auf ein Ereignis des Backends hören (etwa "window:close-requested").
 * @param {string} name
 * @param {(payload: any) => void} fn
 * @returns {() => void} Abmeldefunktion; ohne Backend eine leere
 */
export function on(name, fn) {
	return bridge && typeof bridge.on === 'function' ? bridge.on(name, fn) : () => {};
}

// ---------------------------------------------------------------------------
// Ohne Backend (reiner Browser via `npm run vite`) läuft die IDE weiter:
// kleine Dateien liegen dann im localStorage, der Rest fehlt.
// ---------------------------------------------------------------------------

const memory = new Map();

function fallbackStorage() {
	try {
		const s = window.localStorage;
		s.getItem('__probe__');
		return s;
	} catch {
		return null;
	}
}

/**
 * Kleine Textdateien im Benutzerordner der IDE (Einstellungen, Zustand).
 * Liefert immer etwas Brauchbares, auch ohne Backend.
 */
export const store = {
	/** @returns {Promise<string|null>} */
	async read(name) {
		if (hasBackend) return invoke('store', 'read', name);
		const s = fallbackStorage();
		const v = s ? s.getItem('bltznxt:' + name) : memory.get(name);
		return v === undefined ? null : v;
	},
	async write(name, text) {
		if (hasBackend) return invoke('store', 'write', name, text);
		const s = fallbackStorage();
		if (s) s.setItem('bltznxt:' + name, text);
		else memory.set(name, text);
	},
	async remove(name) {
		if (hasBackend) return invoke('store', 'remove', name);
		const s = fallbackStorage();
		if (s) s.removeItem('bltznxt:' + name);
		else memory.delete(name);
	}
};

export default { hasBackend, invoke, on, store };
