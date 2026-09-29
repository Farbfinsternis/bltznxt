// Erweiterung "files": Dateien und Tabs, wie im Datei-Menü der Original-IDE.
//
//   Neu, Öffnen, Schließen, Alle schließen, Speichern, Speichern unter,
//   Alle speichern, nächster/vorheriger Tab, zuletzt geöffnet (10), Beenden;
//   Start mit Dateiname; Rückfrage bei ungespeicherten Änderungen;
//   Sicherungskopien `datei.bb_bak1`, `_bak2` beim Speichern.
//
// Bytes lesen und schreiben ist der Dienst "files" im Main-Prozess; was sie
// bedeuten (Kodierung, Zeilenende), entscheidet core/encoding.js. Hier steht
// der Ablauf: fragen, lesen, dekodieren, ins Dokumentmodell legen — und beim
// Speichern zurück, ohne dass sich ein Byte ändert, das der Benutzer nicht
// angefasst hat.
//
// Ohne Backend (reiner Browser) bleiben "Neu", die Tabs und "Schließen"
// nutzbar; alles mit Datei ist gesperrt (Kontext `files.available`).

import { decodeFile, encodeFile } from '../../core/encoding.js';
import { icons } from './icons.js';

const RECENT_MAX = 10; // wie im Original
const BLITZ_FILTERS = [
	{ name: 'Blitz Basic files (*.bb)', extensions: ['bb'] },
	{ name: 'All files', extensions: ['*'] }
];

const basename = (p) => String(p).split(/[\\/]/).pop();
const dirname = (p) => {
	const i = Math.max(String(p).lastIndexOf('\\'), String(p).lastIndexOf('/'));
	return i > 0 ? String(p).slice(0, i) : '';
};
const joinPath = (dir, name) => (dir ? `${dir}${dir.includes('\\') ? '\\' : '/'}${name}` : name);
const hasExtension = (p) => /\.[^\\/.]+$/.test(basename(p));
const sameName = (a, b) => String(a).toLowerCase() === String(b).toLowerCase();

export default {
	id: 'files',

	messages: {
		en: {
			'menu.recent': 'Recent Files',
			'file.new': 'New',
			'file.open': 'Open...',
			'file.close': 'Close',
			'file.closeAll': 'Close All',
			'file.save': 'Save',
			'file.saveAs': 'Save As...',
			'file.saveAll': 'Save All',
			'file.nextTab': 'Next File',
			'file.prevTab': 'Previous File',
			'file.openRecent': 'Open Recent File',
			'file.exit': 'Exit',

			'file.dialog.open': 'Open Blitz Basic File...',
			'file.dialog.save': 'Save Blitz Basic program as...',
			'file.confirmClose.title': 'Unsaved changes',
			'file.confirmClose.text': 'File {name} has been modified!\nSave changes before closing?',
			'file.error.title': 'File error',
			'file.error.read': 'Error reading file "{path}"\n{message}',
			'file.error.write': 'Error writing file "{path}"\n{message}',
			'file.error.openElsewhere': 'The file "{path}" is already open in another tab.\nClose that tab first.',
			'file.binary.title': 'Not a text file',
			'file.binary.text': '"{name}" does not look like a text file.\nOpen it anyway?',
			'file.unmappable.title': 'Characters cannot be saved',
			'file.unmappable.text':
				'The file contains characters that Windows-1252 cannot store ({chars}).\nSave it as UTF-8 instead?',
			'file.unmappable.utf8': 'Save as UTF-8',
			'button.yes': 'Yes',
			'button.no': 'No',
			'button.cancel': 'Cancel',
			'button.open': 'Open',

			'file.welcome.title': 'No file open',
			'file.welcome.hint': 'New: {new}    Open: {open}'
		}
	},

	contributes: (ctx) => ({
		settings: {
			'files.backups': {
				type: 'number',
				default: 2, // Original: edit_backup 2
				description: 'How many backup copies (file.bb_bak1, _bak2, ...) to keep when saving. 0 turns backups off.'
			},
			'files.defaultEncoding': {
				type: 'string',
				enum: ['windows-1252', 'utf-8'],
				default: 'windows-1252',
				description: 'Encoding of new files. Existing files keep theirs.'
			},
			'files.defaultEol': {
				type: 'string',
				enum: ['crlf', 'lf'],
				default: 'crlf',
				description: 'Line ending of new files. Existing files keep theirs.'
			}
		},

		commands: commandsOf(ctx),

		// Anordnung wie das Datei-Menü des Originals (blitzide.rc)
		menus: [
			{ menu: 'file', command: 'file.new', group: '1_new', order: 1 },
			{ menu: 'file', command: 'file.open', group: '1_new', order: 2 },
			{ menu: 'file', command: 'file.close', group: '2_close', order: 1 },
			{ menu: 'file', command: 'file.closeAll', group: '2_close', order: 2 },
			{ menu: 'file', command: 'file.save', group: '3_save', order: 1 },
			{ menu: 'file', command: 'file.saveAs', group: '3_save', order: 2 },
			{ menu: 'file', command: 'file.saveAll', group: '3_save', order: 3 },
			{ menu: 'file', command: 'file.nextTab', group: '4_tabs', order: 1 },
			{ menu: 'file', command: 'file.prevTab', group: '4_tabs', order: 2 },
			{ menu: 'file', submenu: 'file.recent', title: 'menu.recent', group: '6_recent' },
			{ menu: 'file', command: 'file.exit', group: '9_exit' }
		],

		keybindings: [
			{ command: 'file.new', key: 'Ctrl+N' },
			{ command: 'file.open', key: 'Ctrl+O' },
			{ command: 'file.close', key: 'Ctrl+F4' },
			{ command: 'file.save', key: 'Ctrl+S' },
			{ command: 'file.nextTab', key: 'Ctrl+Tab' },
			{ command: 'file.prevTab', key: 'Ctrl+Shift+Tab' }
		],

		// Symbolleiste des Originals: Neu, Öffnen, Speichern, Schließen
		toolbar: [
			{ command: 'file.new', icon: icons.new, group: '1_file', order: 1 },
			{ command: 'file.open', icon: icons.open, group: '1_file', order: 2 },
			{ command: 'file.save', icon: icons.save, group: '1_file', order: 3 },
			{ command: 'file.close', icon: icons.close, group: '1_file', order: 4 }
		],

		views: [{ id: 'files.welcome', location: 'editor', when: '!document.active', mount: (el, c) => mountWelcome(el, c) }]
	}),

	activate(ctx) {
		ctx.context.set('files.available', ctx.platform.hasBackend);
		const api = createFileOps(ctx);
		ctx.services.provide('files', api);

		// Zuletzt geöffnet: ein lebendiger Menübeitrag
		const recentMenu = ctx.contributions.contribute('menus', []);
		const showRecent = () =>
			recentMenu.update(
				api.recent().map((path) => ({ menu: 'file.recent', command: 'file.openRecent', title: path, args: [path] }))
			);
		showRecent();
		ctx.subscriptions.add(ctx.state.onDidChange((e) => e.key === 'files.recent' && showRecent()));

		// Fenster schließen: erst fragen (siehe electron/services/window.js)
		ctx.subscriptions.add(ctx.platform.on('window:close-requested', () => api.exit()));

		// `blitzide datei.bb` — die Dateien von der Kommandozeile
		if (ctx.platform.hasBackend) {
			ctx.platform
				.invoke('host', 'launchFiles')
				.then(async (paths) => {
					for (const p of paths) await api.openPath(p);
				})
				.catch((err) => console.error('[files] Startdateien nicht lesbar:', err));
		}
	}
};

// ---------------------------------------------------------------------------
// Befehle
// ---------------------------------------------------------------------------

function commandsOf(ctx) {
	// Die Abläufe liegen in createFileOps; die Befehle greifen erst beim Aufruf
	// darauf zu (der Dienst "files" ist dann da).
	const ops = () => ctx.services.get('files');
	const anyDoc = 'document.active';
	return [
		{ id: 'file.new', title: 'file.new', run: () => ops().newFile() },
		{ id: 'file.open', title: 'file.open', enabledWhen: 'files.available', run: () => ops().open() },
		{ id: 'file.close', title: 'file.close', enabledWhen: anyDoc, run: (id) => ops().close(id) },
		{ id: 'file.closeAll', title: 'file.closeAll', enabledWhen: 'documents.count', run: () => ops().closeAll() },
		{ id: 'file.save', title: 'file.save', enabledWhen: `${anyDoc} && files.available`, run: () => ops().save() },
		{ id: 'file.saveAs', title: 'file.saveAs', enabledWhen: `${anyDoc} && files.available`, run: () => ops().save({ as: true }) },
		{ id: 'file.saveAll', title: 'file.saveAll', enabledWhen: 'documents.count && files.available', run: () => ops().saveAll() },
		{ id: 'file.nextTab', title: 'file.nextTab', enabledWhen: 'documents.multiple', run: () => ops().cycle(1) },
		{ id: 'file.prevTab', title: 'file.prevTab', enabledWhen: 'documents.multiple', run: () => ops().cycle(-1) },
		{ id: 'file.openRecent', title: 'file.openRecent', enabledWhen: 'files.available', run: (path) => ops().openPath(path) },
		{ id: 'file.exit', title: 'file.exit', run: () => ops().exit() }
	];
}

// ---------------------------------------------------------------------------
// Abläufe
// ---------------------------------------------------------------------------

/**
 * Der Dienst "files" (Renderer-Seite): alles, was Dateien und Tabs ändert.
 * Getrennt von den Befehlen, damit andere Erweiterungen (Bauen: "alle
 * speichern"; Hilfe: ".bb öffnen") dieselben Abläufe benutzen.
 */
export function createFileOps(ctx) {
	const { documents, settings, state, platform, i18n } = ctx;
	const t = i18n.t;
	const invoke = (service, method, ...args) => platform.invoke(service, method, ...args);
	const dialogs = () => ctx.services.get('dialogs');
	const say = (title, text, extra = {}) =>
		dialogs().message({ title: t(title), text, buttons: [{ id: 'ok', label: t('button.ok'), default: true }], ...extra });

	// ---- zuletzt geöffnet ---------------------------------------------------
	const recent = () => (state.get('files.recent', []) || []).filter((p) => typeof p === 'string');

	function remember(path) {
		const list = recent().filter((p) => !sameName(p, path));
		list.unshift(path);
		state.set('files.recent', list.slice(0, RECENT_MAX));
		state.set('files.lastDir', dirname(path));
	}

	// ---- neu ----------------------------------------------------------------
	function newFile() {
		return documents.open({
			text: '',
			kind: 'scratch',
			encoding: settings.get('files.defaultEncoding'),
			eol: settings.get('files.defaultEol') === 'lf' ? '\n' : '\r\n'
		});
	}

	// ---- öffnen -------------------------------------------------------------
	async function openPath(p) {
		let path;
		try {
			path = await invoke('files', 'resolve', p);
		} catch (err) {
			await say('file.error.title', t('file.error.read', { path: p, message: err.message }));
			return null;
		}
		const existing = documents.find(path);
		if (existing) {
			documents.activate(existing.id);
			return existing;
		}

		let read;
		try {
			read = await invoke('files', 'read', path);
		} catch (err) {
			await say('file.error.title', t('file.error.read', { path, message: err.message }));
			return null;
		}

		const decoded = decodeFile(read.bytes, {
			defaultEol: settings.get('files.defaultEol') === 'lf' ? '\n' : '\r\n'
		});
		if (decoded.binary) {
			const choice = await dialogs().message({
				title: t('file.binary.title'),
				text: t('file.binary.text', { name: basename(path) }),
				buttons: [{ id: 'cancel', label: t('button.cancel') }, { id: 'open', label: t('button.open') }],
				cancelId: 'cancel'
			});
			if (choice !== 'open') return null;
		}

		// Ein zweiter Aufruf für dieselbe Datei kann während des Lesens fertig
		// geworden sein; `documents.open` erkennt den Pfad und liefert dasselbe.
		const doc = documents.open({ uri: read.path, text: decoded.text, encoding: decoded.encoding, eol: decoded.eol });
		remember(read.path);
		return doc;
	}

	async function open() {
		const paths = await invoke('dialog', 'open', {
			title: t('file.dialog.open'),
			defaultPath: state.get('files.lastDir', '') || undefined,
			filters: BLITZ_FILTERS,
			multi: true
		});
		if (!paths) return [];
		const opened = [];
		for (const p of paths) {
			const doc = await openPath(p);
			if (doc) opened.push(doc);
		}
		return opened;
	}

	// ---- speichern ----------------------------------------------------------
	/**
	 * @param {{ as?: boolean, doc?: object }} [opts]  ohne `doc` das aktive Dokument
	 * @returns {Promise<boolean>} `false`: abgebrochen oder gescheitert (dann wurde es gemeldet)
	 */
	async function save({ as = false, doc = documents.active } = {}) {
		if (!doc) return true;
		let target = doc.uri;

		if (!target || as) {
			documents.activate(doc.id);
			const suggested = doc.uri || joinPath(state.get('files.lastDir', ''), 'untitled.bb');
			const chosen = await invoke('dialog', 'save', {
				title: t('file.dialog.save'),
				defaultPath: suggested,
				filters: BLITZ_FILTERS
			});
			if (!chosen) return false;
			target = await invoke('files', 'resolve', hasExtension(chosen) ? chosen : `${chosen}.bb`);

			const other = documents.find(target);
			if (other && other.id !== doc.id) {
				await say('file.error.title', t('file.error.openElsewhere', { path: target }));
				return false;
			}
		}

		let encoding = doc.encoding;
		let encoded = encodeFile(doc.text, encoding, doc.eol);
		if (encoded.unmappable) {
			const choice = await dialogs().message({
				title: t('file.unmappable.title'),
				text: t('file.unmappable.text', { chars: encoded.unmappable.join(' ') }),
				buttons: [{ id: 'cancel', label: t('button.cancel') }, { id: 'utf8', label: t('file.unmappable.utf8'), default: true }],
				cancelId: 'cancel'
			});
			if (choice !== 'utf8') return false;
			encoding = 'utf-8';
			encoded = encodeFile(doc.text, encoding, doc.eol);
		}

		try {
			await invoke('files', 'backup', target, settings.get('files.backups'));
			await invoke('files', 'write', target, encoded.bytes);
		} catch (err) {
			await say('file.error.title', t('file.error.write', { path: target, message: err.message }));
			return false;
		}

		documents.markSaved(doc.id, { uri: target, encoding, eol: doc.eol });
		remember(target);
		return true;
	}

	/** Von hinten nach vorn wie das Original; bei Abbruch oder Fehler ist Schluss. */
	async function saveAll() {
		const list = documents.list();
		for (let i = list.length - 1; i >= 0; i--) {
			if (!documents.get(list[i].id)) continue;
			if (!(await save({ doc: list[i] }))) return false;
		}
		return true;
	}

	// ---- schließen ----------------------------------------------------------
	/** @returns {Promise<boolean>} `false`: der Benutzer hat abgebrochen */
	async function close(id) {
		const doc = id === undefined ? documents.active : documents.get(id);
		if (!doc) return true;
		if (doc.dirty) {
			documents.activate(doc.id);
			const choice = await dialogs().message({
				title: t('file.confirmClose.title'),
				text: t('file.confirmClose.text', { name: doc.uri || doc.title }),
				buttons: [
					{ id: 'yes', label: t('button.yes'), default: true },
					{ id: 'no', label: t('button.no') },
					{ id: 'cancel', label: t('button.cancel') }
				],
				cancelId: 'cancel'
			});
			if (choice === 'cancel') return false;
			if (choice === 'yes' && !(await save({ doc }))) return false;
		}
		documents.close(doc.id);
		return true;
	}

	async function closeAll() {
		const ids = documents.list().map((d) => d.id).reverse();
		for (const id of ids) {
			if (!(await close(id))) return false;
		}
		return true;
	}

	async function exit() {
		if (!(await closeAll())) return false;
		if (platform.hasBackend) await invoke('window', 'close');
		else if (typeof window !== 'undefined') window.close();
		return true;
	}

	// ---- Tabs ---------------------------------------------------------------
	function cycle(step) {
		const list = documents.list();
		if (list.length < 2) return;
		const index = list.findIndex((d) => documents.active && d.id === documents.active.id);
		documents.activate(list[(index + step + list.length) % list.length].id);
	}

	return { newFile, open, openPath, save, saveAll, close, closeAll, exit, cycle, recent };
}

// ---------------------------------------------------------------------------
// Ansicht, wenn nichts offen ist
// ---------------------------------------------------------------------------

function mountWelcome(el, ctx) {
	const keyOf = (id) => ctx.keybindings.keyFor(id) || '';
	el.className += ' welcome';
	const title = document.createElement('div');
	title.className = 'welcome-title';
	const hint = document.createElement('div');
	hint.className = 'welcome-hint';
	const fill = () => {
		title.textContent = ctx.i18n.t('file.welcome.title');
		hint.textContent = ctx.i18n.t('file.welcome.hint', { new: keyOf('file.new'), open: keyOf('file.open') });
	};
	fill();
	el.append(title, hint);
	return ctx.i18n.onDidChange(fill);
}
