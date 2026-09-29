// Tastenkürzel.
//
// Ein Kürzel ist ein Beitrag zum Punkt "keybindings":
//   { command: 'file.save', key: 'Ctrl+S', when: 'document.active', args: [] }
// Schreibweise: Modifikatoren in beliebiger Reihenfolge, Groß-/Kleinschreibung
// egal ("ctrl+shift+s", "F5", "Shift+Esc"). Intern immer in einer festen Form
// (Ctrl, Alt, Shift, Meta, dann die Taste), damit der Vergleich ein
// Stringvergleich ist.
//
// `passive: true` markiert ein Kürzel, das nur angezeigt wird: die Taste
// behandelt jemand anderes von sich aus (Strg+C im Texteingabefeld des
// Editors). `resolve` liefert es nie, `keyFor` zeigt es im Menü.
//
// Bei mehreren Kürzeln auf derselben Taste gewinnt das zuletzt beigetragene,
// dessen `when` erfüllt ist: eine Erweiterung kann ein Kürzel übernehmen, ohne
// den Beitrag der ersten zu ändern.

const ALIASES = {
	esc: 'Escape',
	escape: 'Escape',
	return: 'Enter',
	enter: 'Enter',
	del: 'Delete',
	delete: 'Delete',
	ins: 'Insert',
	insert: 'Insert',
	space: 'Space',
	' ': 'Space',
	tab: 'Tab',
	backspace: 'Backspace',
	left: 'ArrowLeft',
	right: 'ArrowRight',
	up: 'ArrowUp',
	down: 'ArrowDown',
	arrowleft: 'ArrowLeft',
	arrowright: 'ArrowRight',
	arrowup: 'ArrowUp',
	arrowdown: 'ArrowDown',
	pgup: 'PageUp',
	pageup: 'PageUp',
	pgdn: 'PageDown',
	pagedown: 'PageDown',
	home: 'Home',
	end: 'End'
};

const MODIFIER_ORDER = ['Ctrl', 'Alt', 'Shift', 'Meta'];
const MODIFIER_ALIASES = {
	ctrl: 'Ctrl',
	control: 'Ctrl',
	strg: 'Ctrl',
	alt: 'Alt',
	shift: 'Shift',
	meta: 'Meta',
	cmd: 'Meta',
	win: 'Meta'
};

function normalizeName(name) {
	const alias = ALIASES[name.toLowerCase()];
	if (alias) return alias;
	if (/^f\d{1,2}$/i.test(name)) return name.toUpperCase();
	return name.length === 1 ? name.toUpperCase() : name;
}

/**
 * "ctrl+shift+s" -> "Ctrl+Shift+S"
 * @param {string} spec
 */
export function normalizeKey(spec) {
	const parts = String(spec).split('+').map((p) => p.trim());
	// "Ctrl++" (das Pluszeichen als Taste) ergibt am Ende ein leeres Stück
	if (parts.length >= 2 && parts[parts.length - 1] === '' && parts[parts.length - 2] === '') {
		parts.splice(parts.length - 2, 2, '+');
	}
	const mods = new Set();
	let key = '';
	for (const part of parts) {
		const mod = MODIFIER_ALIASES[part.toLowerCase()];
		if (mod && part !== '') mods.add(mod);
		else key = normalizeName(part);
	}
	if (!key) throw new Error(`Kürzel ohne Taste: "${spec}"`);
	return [...MODIFIER_ORDER.filter((m) => mods.has(m)), key].join('+');
}

/**
 * Aus einem Tastaturereignis (oder etwas Gleichförmigem) die normalisierte
 * Form. Ein reiner Modifikator ergibt `null`.
 * @param {{ key: string, ctrlKey?: boolean, altKey?: boolean, shiftKey?: boolean, metaKey?: boolean }} ev
 */
export function keyFromEvent(ev) {
	if (!ev || typeof ev.key !== 'string') return null;
	if (['Control', 'Shift', 'Alt', 'Meta', 'AltGraph', 'Dead', 'Unidentified'].includes(ev.key)) return null;
	const mods = [];
	if (ev.ctrlKey) mods.push('Ctrl');
	if (ev.altKey) mods.push('Alt');
	if (ev.shiftKey) mods.push('Shift');
	if (ev.metaKey) mods.push('Meta');
	return [...mods, normalizeName(ev.key)].join('+');
}

/**
 * @param {{
 *   contributions: ReturnType<typeof import('./contributions.js').createContributions>,
 *   context: ReturnType<typeof import('./context.js').createContext>
 * }} deps
 */
export function createKeybindings({ contributions, context }) {
	/** @type {Map<string, Array<{ command: string, args: any[], when?: any, key: string }>>} */
	let byKey = new Map();
	/** @type {Map<string, string>} */
	let byCommand = new Map();

	function rebuild() {
		byKey = new Map();
		byCommand = new Map();
		for (const item of contributions.get('keybindings')) {
			let key;
			try {
				key = normalizeKey(item.key);
			} catch (err) {
				console.error(`[keybindings] ${err.message}`);
				continue;
			}
			const entry = { command: item.command, args: item.args || [], when: item.when, key };
			if (!item.passive) {
				if (!byKey.has(key)) byKey.set(key, []);
				byKey.get(key).push(entry);
			}
			// Anzeige im Menü: das erste Kürzel eines Befehls
			if (!byCommand.has(item.command)) byCommand.set(item.command, key);
		}
	}

	rebuild();
	contributions.onDidChange('keybindings', rebuild);

	/**
	 * Welcher Befehl gehört zu dieser Taste — jetzt, im aktuellen Kontext?
	 * @param {string} key  normalisiert
	 * @returns {{ command: string, args: any[] } | null}
	 */
	function resolve(key) {
		const list = byKey.get(key);
		if (!list) return null;
		for (let i = list.length - 1; i >= 0; i--) {
			if (context.evaluate(list[i].when)) return { command: list[i].command, args: list[i].args };
		}
		return null;
	}

	/** Das erste Kürzel eines Befehls für die Menüanzeige, oder `undefined`. */
	const keyFor = (command) => byCommand.get(command);

	return { resolve, keyFor };
}
