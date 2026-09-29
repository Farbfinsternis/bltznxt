// Erweiterung "explorer": die Dateiliste in der Seitenleiste.
//
// Die Original-IDE kennt keine; ein Blitz3D-Programm hat kein Projekt, es ist
// an den Ordner seiner Quelle gebunden (PLAN.md, "Kein Projekt"). Die Liste
// bleibt dabei: sie zeigt einen Ordner, kein Projekt.
//
//   * Ohne Auswahl folgt sie der aktiven Datei: deren Ordner steht oben. Liegt
//     die Datei schon im gezeigten Ordner (ein Include in einem Unterordner),
//     bleibt die Liste stehen.
//   * "Open Folder..." (Datei-Menü) wählt einen Ordner fest; der Knopf mit dem
//     Pin daneben löst ihn wieder.
//   * Ein Klick öffnet die Datei im Editor; Bilder, Klänge und Modelle öffnen
//     im Standardprogramm des Systems (das tut im Original der `mediaview.exe`).
//   * Ausgeblendet wird, was beim Arbeiten nur stört (Einstellung
//     `explorer.hide`): Programme, DLLs, Sicherungskopien.
//
// Aktualisiert wird beim Speichern, nach jedem Bau, beim Zurückkehren in das
// Fenster und auf Befehl — es gibt keinen Dateiwächter.

import { basename, decideRoot, dirname, isInside, isMedia, joinPath, visibleEntries } from './tree.js';

const SAME = (a, b) => String(a).toLowerCase() === String(b).toLowerCase();
const EXPANDED_MAX = 200;

const svg = (d) => `<svg viewBox="0 0 16 16" aria-hidden="true"><path d="${d}"/></svg>`;
const icons = {
	folderOpen: svg('M1 3h5l1.5 1.5H14V6H4.2L2 13H1V3zm3.6 4.5H15L12.6 13H2.5l2.1-5.5z'),
	folder: svg('M1 3h5l1.5 1.5H15V13H1V3z'),
	file: svg('M3 1h6l4 4v10H3V1zm5.5 1.5V6H12L8.5 2.5z'),
	refresh: svg('M13.5 8a5.5 5.5 0 1 1-1.6-3.9L10 6h5V1l-1.7 1.7A7 7 0 1 0 15 8h-1.5z'),
	pin: svg('M10 1l5 5-1.5 1.5-1-.5-3 3 .5 2.5-1.5 1.5-2.5-2.5-4 4-1-1 4-4L2.5 8.5 4 7l2.5.5 3-3-.5-1L10 1z'),
	open: svg('M1 3h5l1.5 1.5H14V6H4.2L2 13H1V3zm3.6 4.5H15L12.6 13H2.5l2.1-5.5z')
};

const controllers = new WeakMap();

export default {
	id: 'explorer',
	dependsOn: ['files'],

	messages: {
		en: {
			'explorer.title': 'Files',
			'explorer.openFolder': 'Open Folder...',
			'explorer.dialog.folder': 'Open Folder',
			'explorer.refresh': 'Refresh',
			'explorer.followActive': 'Follow Active File',
			'explorer.empty': 'No folder open.\nOpen a file, or choose File > Open Folder...',
			'explorer.unreadable': 'Cannot read this folder',
			'explorer.empty.folder': '(empty)'
		}
	},

	contributes: (ctx) => {
		const controller = createController(ctx);
		controllers.set(ctx, controller);
		return {
			settings: {
				'explorer.hide': {
					type: 'array',
					// Programme, Bibliotheken und Sicherungskopien: das Ergebnis des Arbeitens, nicht die Arbeit
					default: ['*.exe', '*.dll', '*.bb_bak*', '.git', '.svn', '.vs', 'node_modules', '__pycache__', '*.pyc', 'Thumbs.db', 'desktop.ini'],
					description: 'Names hidden in the file list; * and ? work as wildcards.'
				}
			},
			commands: [
				{ id: 'explorer.openFolder', title: 'explorer.openFolder', enabledWhen: 'files.available', run: () => controller.chooseFolder() },
				{ id: 'explorer.refresh', title: 'explorer.refresh', enabledWhen: 'explorer.hasRoot', run: () => controller.refresh() },
				{ id: 'explorer.followActive', title: 'explorer.followActive', enabledWhen: 'explorer.pinned', run: () => controller.unpin() }
			],
			menus: [{ menu: 'file', command: 'explorer.openFolder', group: '1_new', order: 3 }],
			views: [
				{
					id: 'explorer',
					location: 'sidebar',
					title: 'explorer.title',
					order: 10,
					mount: (el) => controller.mount(el)
				}
			]
		};
	},

	activate(ctx) {
		controllers.get(ctx).start();
	}
};

// ---------------------------------------------------------------------------

function createController(ctx) {
	const { documents, settings, state, context, platform, i18n } = ctx;
	const t = i18n.t;

	/** @type {string | null} der gezeigte Ordner */
	let root = null;
	/** Ordnerinhalt nach Pfad: Array | 'error' */
	const contents = new Map();
	/** aufgeklappte Ordner (Pfade) */
	let expanded = new Set(state.get('explorer.expanded', []));
	let view = null;
	let scheduled = null;

	const files = () => ctx.services.get('files');
	const hide = () => settings.get('explorer.hide') || [];

	// ---- Wurzel bestimmen -----------------------------------------------------------------
	function recomputeRoot() {
		const active = documents.active;
		const next = decideRoot({
			explicitRoot: state.get('explorer.root', null),
			activePath: active && active.uri ? active.uri : null,
			currentRoot: root
		});
		context.set('explorer.hasRoot', Boolean(next));
		context.set('explorer.pinned', Boolean(state.get('explorer.root', null)));
		if (next !== root) {
			root = next;
			contents.clear();
			// Wurzel und die aus dem Zustand wiederhergestellten aufgeklappten Ordner
			if (root) refresh();
		}
		render();
	}

	// ---- Laden ------------------------------------------------------------------------------
	async function load(dir) {
		try {
			contents.set(dir, await platform.invoke('files', 'list', dir));
		} catch (err) {
			console.warn(`[explorer] ${dir}:`, err.message);
			contents.set(dir, 'error');
		}
		render();
	}

	/** Alles Gezeigte neu lesen (Wurzel und aufgeklappte Ordner, die noch da sind). */
	async function refresh() {
		if (!root) return;
		const dirs = [root, ...[...expanded].filter((p) => isInside(root, p))];
		await Promise.all(dirs.map((d) => load(d)));
	}

	// Mehrere Anlässe kurz hintereinander (Speichern + Bauen) ergeben eine Aktualisierung
	function scheduleRefresh() {
		if (scheduled !== null) clearTimeout(scheduled);
		scheduled = setTimeout(() => {
			scheduled = null;
			refresh();
		}, 150);
	}

	// ---- Ordner wählen --------------------------------------------------------------------------
	async function chooseFolder() {
		const chosen = await platform.invoke('dialog', 'folder', { title: t('explorer.dialog.folder'), defaultPath: root || undefined });
		if (!chosen) return;
		const path = await platform.invoke('files', 'resolve', chosen);
		state.set('explorer.root', path);
		recomputeRoot();
	}

	function unpin() {
		state.remove('explorer.root');
		root = null; // neu bestimmen: der Ordner der aktiven Datei
		recomputeRoot();
	}

	// ---- Klicks -------------------------------------------------------------------------------------
	function toggle(path) {
		if (expanded.has(path)) expanded.delete(path);
		else {
			expanded.add(path);
			if (!contents.has(path)) load(path);
		}
		state.set('explorer.expanded', [...expanded].slice(-EXPANDED_MAX));
		render();
	}

	async function openEntry(path, name) {
		if (isMedia(name)) {
			try {
				await platform.invoke('shell', 'openExternal', path);
			} catch (err) {
				console.warn('[explorer] Datei konnte nicht geöffnet werden:', err.message);
			}
			return;
		}
		await files().openPath(path);
	}

	// ---- Zeichnen ------------------------------------------------------------------------------------
	/** Sichtbare Zeilen: Ordner und Dateien, aufgeklappte Ordner mit ihrem Inhalt. */
	function rows() {
		const out = [];
		const walk = (dir, depth) => {
			const entries = contents.get(dir);
			if (entries === undefined) return;
			if (entries === 'error') {
				out.push({ kind: 'note', depth, text: t('explorer.unreadable') });
				return;
			}
			const visible = visibleEntries(entries, hide());
			if (!visible.length && depth > 0) out.push({ kind: 'note', depth, text: t('explorer.empty.folder') });
			for (const e of visible) {
				const path = joinPath(dir, e.name);
				const open = e.dir && expanded.has(path);
				out.push({ kind: e.dir ? 'dir' : 'file', name: e.name, path, depth, open });
				if (open) walk(path, depth + 1);
			}
		};
		if (root) walk(root, 0);
		return out;
	}

	const el = (tag, className, text) => {
		const node = document.createElement(tag);
		if (className) node.className = className;
		if (text !== undefined) node.textContent = text;
		return node;
	};

	function iconButton(icon, title, command) {
		const b = el('button', 'explorer-button');
		b.type = 'button';
		b.title = title;
		b.setAttribute('aria-label', title);
		b.dataset.command = command;
		b.innerHTML = icon;
		b.disabled = !ctx.commands.isEnabled(command);
		b.addEventListener('click', () => ctx.commands.execute(command).catch((err) => console.error(err)));
		return b;
	}

	function render() {
		if (!view) return;
		const { header, tree } = view;

		// Kopf: Ordnername mit den Knöpfen
		const title = el('span', 'explorer-title', root ? basename(root) : t('explorer.title'));
		title.title = root || '';
		const buttons = el('span', 'explorer-buttons');
		buttons.append(
			iconButton(icons.open, t('explorer.openFolder'), 'explorer.openFolder'),
			iconButton(icons.refresh, t('explorer.refresh'), 'explorer.refresh')
		);
		if (state.get('explorer.root', null)) buttons.append(iconButton(icons.pin, t('explorer.followActive'), 'explorer.followActive'));
		header.replaceChildren(title, buttons);

		// Baum
		const active = documents.active && documents.active.uri;
		const list = rows();
		if (!root) {
			tree.replaceChildren(el('div', 'explorer-empty', t('explorer.empty')));
			return;
		}
		// Die Zeilen werden bei jeder Änderung neu gebaut; der Fokus soll dabei nicht verloren gehen
		const focused = tree.contains(document.activeElement) ? document.activeElement.dataset.path : null;
		tree.replaceChildren(
			...list.map((row) => {
				if (row.kind === 'note') {
					const note = el('div', 'explorer-note', row.text);
					note.style.paddingLeft = `${12 + row.depth * 14}px`;
					return note;
				}
				const isDir = row.kind === 'dir';
				const div = el('div', 'explorer-row' + (isDir ? ' dir' : ' file') + (active && SAME(active, row.path) ? ' active' : '') + (/\.bb$/i.test(row.name) ? ' source' : ''));
				div.setAttribute('role', 'treeitem');
				div.setAttribute('aria-level', String(row.depth + 1));
				if (isDir) div.setAttribute('aria-expanded', row.open ? 'true' : 'false');
				div.tabIndex = -1;
				div.dataset.path = row.path;
				div.title = row.path;
				div.style.paddingLeft = `${8 + row.depth * 14}px`;
				const chevron = el('span', 'explorer-chevron', isDir ? (row.open ? '▾' : '▸') : '');
				const icon = el('span', 'explorer-icon');
				icon.innerHTML = isDir ? (row.open ? icons.folderOpen : icons.folder) : icons.file;
				div.append(chevron, icon, el('span', 'explorer-name', row.name));
				div.addEventListener('click', () => {
					div.focus();
					if (isDir) toggle(row.path);
					else openEntry(row.path, row.name);
				});
				return div;
			})
		);
		if (focused) tree.querySelector(`.explorer-row[data-path="${CSS.escape(focused)}"]`)?.focus();
	}

	/** Pfeiltasten wie in jedem Baum: rauf/runter, rechts auf-, links zuklappen, Enter öffnet. */
	function onKey(ev) {
		const items = [...view.tree.querySelectorAll('.explorer-row')];
		const i = items.indexOf(document.activeElement);
		const move = (n) => {
			if (n >= 0 && n < items.length) items[n].focus();
		};
		const cur = items[i];
		switch (ev.key) {
			case 'ArrowDown': move(i < 0 ? 0 : i + 1); break;
			case 'ArrowUp': move(i < 0 ? 0 : i - 1); break;
			case 'ArrowRight':
				if (cur && cur.classList.contains('dir') && cur.getAttribute('aria-expanded') === 'false') cur.click();
				else move(i + 1);
				break;
			case 'ArrowLeft':
				if (cur && cur.classList.contains('dir') && cur.getAttribute('aria-expanded') === 'true') cur.click();
				else if (cur) {
					const parent = dirname(cur.dataset.path);
					const p = items.findIndex((x) => SAME(x.dataset.path, parent));
					if (p >= 0) move(p);
				}
				break;
			case 'Enter':
			case ' ':
				if (cur) cur.click();
				break;
			default:
				return;
		}
		ev.preventDefault();
	}

	function mount(container) {
		container.classList.add('explorer');
		const header = el('div', 'explorer-header');
		const tree = el('div', 'explorer-tree');
		tree.setAttribute('role', 'tree');
		tree.addEventListener('keydown', onKey);
		container.append(header, tree);
		view = { header, tree };
		render();
		return () => {
			view = null;
		};
	}

	function start() {
		context.set('explorer.hasRoot', false);
		context.set('explorer.pinned', false);
		recomputeRoot();

		ctx.subscriptions.add(documents.on('activated', recomputeRoot));
		ctx.subscriptions.add(documents.on('renamed', recomputeRoot));
		ctx.subscriptions.add(documents.on('saved', scheduleRefresh));
		ctx.subscriptions.add(ctx.events.on('build:finished', scheduleRefresh));
		ctx.subscriptions.add(settings.onDidChange((e) => e.key === 'explorer.hide' && render()));
		// Zurück im Fenster: es kann sich etwas geändert haben (ein anderes Programm, ein Editor, git)
		if (typeof window !== 'undefined') {
			const onFocus = () => scheduleRefresh();
			window.addEventListener('focus', onFocus);
			ctx.subscriptions.add(() => window.removeEventListener('focus', onFocus));
		}
		// Die Auswahl der aktiven Datei zeigt sich in der Liste
		ctx.subscriptions.add(documents.on('activated', render));
		ctx.subscriptions.add(() => {
			if (scheduled !== null) clearTimeout(scheduled);
		});

		ctx.services.provide('explorer', {
			get root() { return root; },
			refresh,
			rows: () => rows().map((r) => ({ ...r }))
		});
	}

	return { mount, start, chooseFolder, refresh, unpin };
}
