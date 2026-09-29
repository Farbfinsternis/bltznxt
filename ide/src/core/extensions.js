// Erweiterungen.
//
// Alles, was die IDE kann, ist eine Erweiterung: Datei-Tabs, Bauen, Hilfe,
// Gliederung, Einstellungen. Die eingebauten benutzen genau diese
// Schnittstelle und keine andere — was eine eingebaute Erweiterung nicht darf,
// darf der Kern nicht heimlich bieten. Fehlt ihr etwas, fehlt dem Kern ein
// Erweiterungspunkt, und den bauen wir.
//
// Form:
//   {
//     id: 'build-run',
//     dependsOn: ['toolchain'],            // Aktivierungsreihenfolge, optional
//     messages: { en: { ... }, de: { ... } },
//     contributes: {                        // oder eine Funktion: contributes(ctx) => {...}
//       commands:   [{ id, title, enabledWhen, run }],   // -> Befehlsregister
//       settings:   { 'key': { type, default, ... } },   // -> Einstellungen
//       menubar, menus, keybindings, toolbar, views, statusItems, ...
//                                                        // -> Beiträge, je Schlüssel ein Punkt
//     },
//     activate(ctx)  { ... },              // optional, darf async sein
//     deactivate()   { ... }               // optional
//   }
//
// Die Schnittstelle ist bis zur Paritätsprüfung (PLAN.md, P6) *experimentell*
// und darf sich ändern. Aktivierung soll nicht auf Äußeres warten (Compiler
// suchen, Dateien lesen): solche Arbeit startet `activate`, ohne sie
// abzuwarten — sonst steht der Start der ganzen IDE still.

import { createDisposables, createEmitter } from './events.js';

/**
 * @typedef {'pending' | 'active' | 'failed' | 'skipped' | 'inactive'} ExtensionState
 */

/**
 * @param {{
 *   commands: ReturnType<typeof import('./commands.js').createCommands>,
 *   contributions: ReturnType<typeof import('./contributions.js').createContributions>,
 *   services: ReturnType<typeof import('./services.js').createServices>,
 *   context: ReturnType<typeof import('./context.js').createContext>,
 *   settings: ReturnType<typeof import('./settings.js').createSettings>,
 *   i18n: ReturnType<typeof import('./i18n.js').createI18n>,
 *   events: ReturnType<typeof import('./events.js').createEmitter>,
 *   extra?: Record<string, any>
 * }} deps  `extra` wird an jeden `ctx` gehängt (etwa `documents`)
 */
export function createExtensionHost(deps) {
	const { commands, contributions, services, context, settings, i18n, events, extra = {} } = deps;

	/** @type {Map<string, { manifest: any, state: ExtensionState, error?: Error, disposables: ReturnType<typeof createDisposables>, ctx?: any }>} */
	const extensions = new Map();
	const hostEvents = createEmitter();
	/** Reihenfolge der tatsächlichen Aktivierung — Abschalten läuft rückwärts dazu. */
	const activated = [];

	function add(manifest) {
		if (!manifest || typeof manifest.id !== 'string' || !manifest.id) {
			throw new Error('Erweiterung ohne id');
		}
		if (extensions.has(manifest.id)) {
			throw new Error(`Erweiterung "${manifest.id}" ist schon hinzugefügt`);
		}
		extensions.set(manifest.id, {
			manifest,
			state: 'pending',
			disposables: createDisposables()
		});
	}

	/** Reihenfolge nach `dependsOn`; Zyklen und fehlende Abhängigkeiten sind Fehler der jeweiligen Erweiterung. */
	function order() {
		/** @type {string[]} */
		const sorted = [];
		const mark = new Map(); // id -> 'visiting' | 'done'
		const problems = new Map(); // id -> Error

		function visit(id, trail) {
			if (mark.get(id) === 'done') return;
			if (mark.get(id) === 'visiting') {
				const cycle = [...trail.slice(trail.indexOf(id)), id].join(' -> ');
				problems.set(id, new Error(`Zyklus in dependsOn: ${cycle}`));
				return;
			}
			mark.set(id, 'visiting');
			for (const dep of extensions.get(id).manifest.dependsOn || []) {
				if (!extensions.has(dep)) {
					problems.set(id, new Error(`Abhängigkeit "${dep}" fehlt`));
					continue;
				}
				visit(dep, [...trail, id]);
			}
			mark.set(id, 'done');
			sorted.push(id);
		}

		for (const id of extensions.keys()) visit(id, []);
		return { sorted, problems };
	}

	function makeContext(id, entry) {
		const track = (d) => entry.disposables.add(d);
		return {
			id,
			commands: {
				register: (def) => track(commands.register(def)),
				execute: (...a) => commands.execute(...a),
				has: commands.has,
				isEnabled: commands.isEnabled
			},
			contributions: {
				contribute: (point, items) => {
					const handle = contributions.contribute(point, items, id);
					track(handle.dispose);
					return handle;
				},
				get: contributions.get,
				onDidChange: contributions.onDidChange
			},
			services: {
				provide: (name, impl) => {
					track(services.provide(name, impl));
					return impl;
				},
				get: services.get,
				tryGet: services.tryGet,
				has: services.has,
				waitFor: services.waitFor
			},
			context,
			settings: {
				get: settings.get,
				set: settings.set,
				reset: settings.reset,
				onDidChange: settings.onDidChange,
				registerSchema: (entries) => track(settings.registerSchema(entries))
			},
			i18n: {
				t: i18n.t,
				add: (messages) => track(i18n.add(messages)),
				setLanguage: i18n.setLanguage,
				get language() { return i18n.language; },
				onDidChange: i18n.onDidChange
			},
			events,
			/** Alles, was beim Abschalten der Erweiterung weg muss. */
			subscriptions: { add: track },
			...extra
		};
	}

	function applyContributions(id, entry, ctx) {
		const { manifest } = entry;
		const track = (d) => entry.disposables.add(d);
		if (manifest.messages) track(i18n.add(manifest.messages));

		// `contributes` darf eine Funktion sein: sie bekommt den Kontext und
		// kann Befehle schreiben, die auf Einstellungen und Dienste zugreifen.
		const contributes = (typeof manifest.contributes === 'function' ? manifest.contributes(ctx) : manifest.contributes) || {};
		for (const [point, value] of Object.entries(contributes)) {
			if (point === 'commands') {
				for (const def of value) track(commands.register(def));
			} else if (point === 'settings') {
				track(settings.registerSchema(value));
			} else {
				const items = point === 'views'
					// Ansichten bekommen den Kontext ihrer Erweiterung mit auf den Weg
					? value.map((v) => (typeof v.mount === 'function' ? { ...v, mount: (el) => v.mount(el, ctx) } : v))
					: value;
				track(contributions.contribute(point, items, id).dispose);
			}
		}
	}

	async function activateOne(id, problems) {
		const entry = extensions.get(id);
		const fail = (error, state = 'failed') => {
			entry.state = state;
			entry.error = error;
			console.error(`[extensions] ${id}: ${error.message}`);
			hostEvents.emit('failed', { id, error, state });
		};

		if (problems.has(id)) return fail(problems.get(id));
		for (const dep of entry.manifest.dependsOn || []) {
			if (extensions.get(dep).state !== 'active') {
				return fail(new Error(`Abhängigkeit "${dep}" ist nicht aktiv`), 'skipped');
			}
		}

		try {
			const ctx = makeContext(id, entry);
			entry.ctx = ctx;
			applyContributions(id, entry, ctx);
			if (typeof entry.manifest.activate === 'function') await entry.manifest.activate(ctx);
			entry.state = 'active';
			activated.push(id);
			hostEvents.emit('activated', { id });
		} catch (err) {
			// Halb aktivierte Erweiterung wieder abbauen, damit keine
			// Menüpunkte ohne Befehl zurückbleiben.
			entry.disposables.dispose();
			fail(err instanceof Error ? err : new Error(String(err)));
		}
	}

	/** Aktiviert alle hinzugefügten Erweiterungen; ein Fehler stoppt die anderen nicht. */
	async function activateAll() {
		const { sorted, problems } = order();
		for (const id of sorted) {
			if (extensions.get(id).state === 'pending') await activateOne(id, problems);
		}
		for (const [id] of problems) {
			if (extensions.get(id).state === 'pending') await activateOne(id, problems);
		}
	}

	/** Schaltet alle in umgekehrter Aktivierungsreihenfolge ab. */
	async function deactivateAll() {
		const ids = activated.splice(0).reverse();
		for (const id of ids) {
			const entry = extensions.get(id);
			if (entry.state !== 'active') continue;
			try {
				if (typeof entry.manifest.deactivate === 'function') await entry.manifest.deactivate();
			} catch (err) {
				console.error(`[extensions] ${id}: deactivate fehlgeschlagen:`, err);
			}
			entry.disposables.dispose();
			entry.state = 'inactive';
		}
	}

	return {
		add,
		activateAll,
		deactivateAll,
		/** @returns {Array<{ id: string, state: ExtensionState, error?: string }>} */
		list: () =>
			[...extensions].map(([id, e]) => ({ id, state: e.state, error: e.error?.message })),
		stateOf: (id) => extensions.get(id)?.state,
		onDidActivate: (fn) => hostEvents.on('activated', fn),
		onDidFail: (fn) => hostEvents.on('failed', fn)
	};
}
