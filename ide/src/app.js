// Zusammenbau: der einzige Ort, der Kern und Erweiterungen verdrahtet.
//
// Hier steht kein Feature der IDE. `createApp` baut die Register des Kerns,
// hält den Kontext aktuell (Dokumentzustand als Kontextschlüssel), lädt und
// sichert die Einstellungen und aktiviert die Erweiterungen. Alles Weitere
// kommt aus den Erweiterungen.
//
// Läuft ohne DOM: ein Test kann eine App bauen, Erweiterungen hinzufügen und
// den Ablauf prüfen, ohne ein Fenster zu öffnen.

import { createEmitter } from './core/events.js';
import { createContext } from './core/context.js';
import { createCommands } from './core/commands.js';
import { createContributions } from './core/contributions.js';
import { createServices } from './core/services.js';
import { createSettings } from './core/settings.js';
import { createI18n } from './core/i18n.js';
import { createDocuments } from './core/documents.js';
import { createKeybindings } from './core/keybindings.js';
import { createExtensionHost } from './core/extensions.js';

const SETTINGS_FILE = 'settings.json';
const SAVE_DELAY_MS = 300;

/**
 * @param {{
 *   platform: { store: { read(name: string): Promise<string|null>, write(name: string, text: string): Promise<void> } },
 *   extensions?: any[],
 *   schedule?: (fn: () => void, ms: number) => any,
 *   cancel?: (handle: any) => void
 * }} opts  `schedule`/`cancel` sind für Tests austauschbar
 */
export function createApp({ platform, extensions = [], schedule = setTimeout, cancel = clearTimeout }) {
	const events = createEmitter();
	const context = createContext();
	const commands = createCommands({ context });
	const contributions = createContributions();
	const services = createServices();
	const settings = createSettings();
	const i18n = createI18n({ language: 'en' });
	const documents = createDocuments();
	const keybindings = createKeybindings({ contributions, context });

	const host = createExtensionHost({
		commands, contributions, services, context, settings, i18n, events,
		// Was jede Erweiterung im ctx sieht, ohne es sich per Dienst zu holen
		extra: { documents, platform, keybindings }
	});

	// ---- Dokumentzustand als Kontext ---------------------------------------
	//
	// Damit Menüs, Kürzel und Ansichten ("document.active && document.dirty")
	// darauf reagieren können, ohne den Dokumentspeicher zu kennen.
	function syncDocumentContext() {
		const list = documents.list();
		context.set('documents.count', list.length);
		context.set('documents.anyDirty', list.some((d) => d.dirty));
		context.set('document.active', documents.active !== null);
		context.set('document.dirty', Boolean(documents.active && documents.active.dirty));
		context.set('document.kind', documents.active ? documents.active.kind : '');
	}
	for (const name of ['opened', 'closed', 'activated', 'changed', 'saved', 'renamed']) {
		documents.on(name, syncDocumentContext);
	}
	syncDocumentContext();

	// ---- Befehle ausführen, Fehler sichtbar machen -------------------------
	/**
	 * Für Menüs, Kürzel, Symbolleiste: führt aus und meldet einen Fehler dem
	 * Benutzer, statt ihn als unbehandelte Ablehnung verschwinden zu lassen.
	 */
	async function run(id, ...args) {
		try {
			return await commands.execute(id, ...args);
		} catch (err) {
			console.error(`[app] Befehl "${id}" ist fehlgeschlagen:`, err);
			const dialogs = services.tryGet('dialogs');
			if (dialogs) {
				await dialogs.message({
					title: i18n.t('error.title'),
					text: i18n.t('error.command', { id, message: String(err && err.message ? err.message : err) })
				});
			}
			return { executed: false, error: err };
		}
	}

	// ---- Einstellungen: laden und (verzögert) sichern ----------------------
	let saveHandle = null;
	let storeBroken = false;

	async function saveSettingsNow() {
		saveHandle = null;
		try {
			await platform.store.write(SETTINGS_FILE, settings.serialize());
			storeBroken = false;
		} catch (err) {
			if (!storeBroken) console.error('[app] Einstellungen konnten nicht gesichert werden:', err);
			storeBroken = true;
		}
	}

	/** @returns {Promise<{ error: string, backup: string } | null>} Angabe, wenn die Datei unlesbar war */
	async function loadSettings() {
		let text = null;
		try {
			text = await platform.store.read(SETTINGS_FILE);
		} catch (err) {
			console.error('[app] Einstellungen konnten nicht gelesen werden:', err);
		}
		const result = settings.load(text);
		if (!result.ok) {
			// Nicht stillschweigend überschreiben: die kaputte Datei bleibt als
			// settings.json.bad erhalten, bevor die nächste Sicherung sie ersetzt.
			try {
				await platform.store.write(`${SETTINGS_FILE}.bad`, text ?? '');
			} catch (err) {
				console.error('[app] Sicherung der kaputten Einstellungen fehlgeschlagen:', err);
			}
			return { error: result.error, backup: `${SETTINGS_FILE}.bad` };
		}
		return null;
	}

	settings.onDidChange(() => {
		if (saveHandle !== null) cancel(saveHandle);
		saveHandle = schedule(saveSettingsNow, SAVE_DELAY_MS);
	});

	// ---- Start und Ende -----------------------------------------------------
	for (const ext of extensions) host.add(ext);

	async function start() {
		const corrupt = await loadSettings();
		await host.activateAll();
		// Erst jetzt: vorher hört noch keine Erweiterung zu.
		if (corrupt) events.emit('settings:corrupt', corrupt);
		events.emit('app:started');
	}

	async function stop() {
		if (saveHandle !== null) {
			cancel(saveHandle);
			await saveSettingsNow();
		}
		await host.deactivateAll();
	}

	return {
		events, context, commands, contributions, services, settings, i18n,
		documents, keybindings, host, platform,
		run, start, stop
	};
}
