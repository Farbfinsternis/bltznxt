// Die Shell: Fenster-Aufbau, sonst nichts.
//
// Sie zeichnet, was die Erweiterungen beitragen — Menüleiste, Symbolleiste,
// Tab-Leiste, Ansichten, unteres Panel, Statuszeile — und gibt Tastendrücke an
// das Kürzel-Register weiter. Sie kennt keine einzelne Funktion der IDE. Wer
// sie durch etwas anderes ersetzt (anderes Layout, natives Menü), ersetzt
// diese Dateien und nichts vom Kern.
//
// Gesetzt wird:
//   Dienste  'dialogs' (Meldungen), 'statusbar' (Texte der Statuszeile),
//            'menus' (Menü an einer Stelle aufklappen)
//   Beiträge lesend: menubar, menus, toolbar, views, statusItems, keybindings
//   Kontext  'dialog.open'

import { h, iconNode, batch } from './dom.js';
import { keyFromEvent } from '../core/keybindings.js';
import { createMenubar } from './menubar.js';
import { createDialogs } from './dialogs.js';

const byOrder = (a, b) => (a.order ?? 0) - (b.order ?? 0);
const byGroupOrder = (a, b) => {
	const ga = a.group ?? '';
	const gb = b.group ?? '';
	return ga === gb ? byOrder(a, b) : ga < gb ? -1 : 1;
};

/**
 * @param {{ root: HTMLElement, app: ReturnType<typeof import('../app.js').createApp> }} deps
 */
export function createShell({ root, app }) {
	const { contributions, commands, context, settings, documents, i18n, keybindings } = app;

	// ---- Gerüst -------------------------------------------------------------
	const menubarEl = h('div', { class: 'menubar', role: 'menubar' });
	const toolbarEl = h('div', { class: 'toolbar', role: 'toolbar' });
	const tabsEl = h('div', { class: 'tabs', role: 'tablist' });
	const editorArea = h('div', { class: 'editor-area' });
	const panelTabsEl = h('div', { class: 'panel-tabs', role: 'tablist' });
	const panelBodyEl = h('div', { class: 'panel-body' });
	const panelEl = h('div', { class: 'panel', hidden: true }, panelTabsEl, panelBodyEl);
	const statusLeft = h('div', { class: 'status-left' });
	const statusRight = h('div', { class: 'status-right' });
	const statusbarEl = h('div', { class: 'statusbar' }, statusLeft, statusRight);
	const menuLayer = h('div', { class: 'menu-layer' });
	const dialogLayer = h('div', { class: 'dialog-layer' });

	const ide = h('div', { class: 'ide' }, menubarEl, toolbarEl, tabsEl, editorArea, panelEl, statusbarEl, menuLayer, dialogLayer);
	root.replaceChildren(ide);

	// ---- Dienste ------------------------------------------------------------
	const dialogs = createDialogs({ root: dialogLayer, context });
	app.services.provide('dialogs', dialogs);

	/** @type {Map<string, { text: string, tooltip: string }>} */
	const statusTexts = new Map();
	app.services.provide('statusbar', {
		setText(id, text, tooltip = '') {
			statusTexts.set(id, { text: String(text), tooltip });
			renderStatus();
		},
		clear(id) {
			statusTexts.delete(id);
			renderStatus();
		}
	});

	const menubar = createMenubar({ bar: menubarEl, layer: menuLayer, app });
	// Für Rechtsklickmenüs: `menus.popup('edit', x, y)`
	app.services.provide('menus', { popup: (id, x, y) => menubar.popup(id, x, y), close: () => menubar.close() });

	// ---- Sichtbarkeit von Symbol- und Statusleiste ----------------------------
	// Einstellung gehört der Erweiterung "workbench"; fehlt sie, gilt: sichtbar.
	let toolbarsShown = true;
	function applyChromeVisibility() {
		const show = settings.get('workbench.showToolbars', true);
		toolbarsShown = show;
		ide.dataset.toolbars = show ? 'on' : 'off';
		statusbarEl.hidden = !show;
		toolbarEl.hidden = !show || toolbarEl.childElementCount === 0;
	}

	// ---- Symbolleiste ---------------------------------------------------------
	function renderToolbar() {
		const items = contributions
			.get('toolbar')
			.filter((t) => context.evaluate(t.when) && commands.has(t.command))
			.sort(byGroupOrder);
		const nodes = [];
		let lastGroup = null;
		for (const t of items) {
			const group = t.group ?? '';
			if (nodes.length && group !== lastGroup) nodes.push(h('span', { class: 'toolbar-separator' }));
			lastGroup = group;
			const def = commands.get(t.command);
			const title = i18n.t(t.title ?? def.title ?? t.command);
			nodes.push(
				h('button', {
					class: 'toolbar-button',
					type: 'button',
					title: t.key ? `${title} (${t.key})` : title,
					'aria-label': title,
					'data-command': t.command,
					disabled: !commands.isEnabled(t.command),
					onclick: () => app.run(t.command, ...(t.args || []))
				}, iconNode(t.icon ?? def.icon ?? title.slice(0, 1)))
			);
		}
		toolbarEl.replaceChildren(...nodes);
		// Eine Symbolleiste ohne Knöpfe bekommt keinen Leerstreifen
		toolbarEl.hidden = !toolbarsShown || nodes.length === 0;
	}

	// ---- Tab-Leiste -----------------------------------------------------------
	// Zeigt die Dokumente des Dokumentspeichers. Schließen ist ein Befehl
	// ("file.close"), den eine Erweiterung liefert — ohne ihn gibt es keinen
	// Schließen-Knopf.
	function renderTabs() {
		const active = documents.active;
		const closable = commands.has('file.close');
		tabsEl.hidden = documents.list().length === 0;
		tabsEl.replaceChildren(
			...documents.list().map((doc) =>
				h('div', {
					class: 'tab' + (active && active.id === doc.id ? ' active' : '') + (doc.dirty ? ' dirty' : ''),
					role: 'tab',
					'aria-selected': active && active.id === doc.id ? 'true' : 'false',
					'data-doc': doc.id,
					title: doc.uri || doc.title,
					onclick: () => documents.activate(doc.id),
					onauxclick: (ev) => {
						if (ev.button === 1 && closable) app.run('file.close', doc.id);
					}
				},
					h('span', { class: 'tab-title', text: doc.title }),
					h('span', { class: 'tab-dirty', text: doc.dirty ? '●' : '' }),
					closable && h('button', {
						class: 'tab-close',
						type: 'button',
						'aria-label': i18n.t('tab.close'),
						text: '×',
						onclick: (ev) => {
							ev.stopPropagation();
							app.run('file.close', doc.id);
						}
					})
				)
			)
		);
	}

	// ---- Ansichten -------------------------------------------------------------
	// Jede Ansicht wird einmal eingehängt (`mount`) und danach nur ein- oder
	// ausgeblendet, je nach ihrem `when`. Was sie zeigt, entscheidet sie selbst.
	/** @type {Map<string, { el: HTMLElement, dispose?: () => void, view: any }>} */
	const mounted = new Map();

	function mountView(view, parent) {
		if (mounted.has(view.id)) return mounted.get(view.id);
		const el = h('div', { class: 'view', 'data-view': view.id });
		parent.append(el);
		let dispose;
		try {
			dispose = typeof view.mount === 'function' ? view.mount(el) : undefined;
		} catch (err) {
			console.error(`[shell] Ansicht "${view.id}" konnte nicht eingehängt werden:`, err);
			el.textContent = `View "${view.id}" failed: ${err.message}`;
			el.classList.add('view-error');
		}
		const entry = { el, dispose, view };
		mounted.set(view.id, entry);
		return entry;
	}

	let activePanelView = null;

	function renderViews() {
		const views = contributions.get('views');
		const ids = new Set(views.map((v) => v.id));

		// Ansichten, deren Erweiterung abgeschaltet wurde, abbauen
		for (const [id, entry] of mounted) {
			if (!ids.has(id)) {
				try { if (typeof entry.dispose === 'function') entry.dispose(); } catch (err) { console.error(err); }
				entry.el.remove();
				mounted.delete(id);
			}
		}

		for (const view of views.filter((v) => (v.location ?? 'editor') === 'editor')) {
			const entry = mountView(view, editorArea);
			entry.el.hidden = !context.evaluate(view.when);
		}

		const panelViews = views.filter((v) => v.location === 'panel').sort(byOrder);
		const visible = panelViews.filter((v) => context.evaluate(v.when));
		if (!visible.some((v) => v.id === activePanelView)) activePanelView = visible.length ? visible[0].id : null;
		panelEl.hidden = visible.length === 0;
		panelTabsEl.replaceChildren(
			...visible.map((v) =>
				h('button', {
					class: 'panel-tab' + (v.id === activePanelView ? ' active' : ''),
					type: 'button',
					role: 'tab',
					text: i18n.t(v.title ?? v.id),
					onclick: () => {
						activePanelView = v.id;
						renderViews();
					}
				})
			)
		);
		for (const v of panelViews) {
			const entry = mountView(v, panelBodyEl);
			entry.el.hidden = v.id !== activePanelView;
		}
	}

	// ---- Statuszeile -----------------------------------------------------------
	function renderStatus() {
		const items = contributions.get('statusItems').filter((s) => context.evaluate(s.when)).sort((a, b) => (b.priority ?? 0) - (a.priority ?? 0));
		const make = (s) => {
			const t = statusTexts.get(s.id);
			const text = t ? t.text : s.text !== undefined ? i18n.t(s.text) : '';
			if (!text) return null;
			const tag = s.command ? 'button' : 'span';
			return h(tag, {
				class: 'status-item' + (s.command ? ' clickable' : ''),
				type: s.command ? 'button' : null,
				'data-status': s.id,
				title: t?.tooltip || (s.tooltip ? i18n.t(s.tooltip) : null),
				onclick: s.command ? () => app.run(s.command, ...(s.args || [])) : null,
				text
			});
		};
		statusLeft.replaceChildren(...items.filter((s) => s.align !== 'right').map(make).filter(Boolean));
		statusRight.replaceChildren(...items.filter((s) => s.align === 'right').map(make).filter(Boolean));
	}

	// ---- Zeichnen bei Änderungen ---------------------------------------------------
	const renderAll = batch(() => {
		applyChromeVisibility();
		renderToolbar();
		renderTabs();
		renderViews();
		renderStatus();
	});
	const renderMenubar = batch(() => menubar.render());

	const offs = [
		contributions.onDidChange('toolbar', renderAll),
		contributions.onDidChange('views', renderAll),
		contributions.onDidChange('statusItems', renderAll),
		contributions.onDidChange('menubar', renderMenubar),
		contributions.onDidChange('menus', renderMenubar),
		commands.onDidChange(() => { renderAll(); renderMenubar(); }),
		context.onDidChange(renderAll),
		settings.onDidChange(renderAll),
		i18n.onDidChange(() => { renderAll(); renderMenubar(); })
	];
	for (const name of ['opened', 'closed', 'activated', 'changed', 'saved', 'renamed']) {
		offs.push(documents.on(name, renderAll));
	}

	// ---- Tastenkürzel -----------------------------------------------------------------
	// Im Fang-Durchlauf, damit ein Kürzel gilt, auch wenn der Editor den Fokus
	// hat. Ist der Befehl gerade gesperrt, wird die Taste nicht verbraucht.
	function onKeyDown(ev) {
		if (context.get('dialog.open')) return;
		const key = keyFromEvent(ev);
		if (!key) return;
		const bound = keybindings.resolve(key);
		if (!bound || !commands.isEnabled(bound.command)) return;
		ev.preventDefault();
		ev.stopPropagation();
		menubar.close();
		app.run(bound.command, ...bound.args);
	}
	window.addEventListener('keydown', onKeyDown, true);

	renderAll();
	renderMenubar();

	return {
		dialogs,
		/** Für Tests und Diagnose. */
		elements: { ide, menubar: menubarEl, toolbar: toolbarEl, tabs: tabsEl, editorArea, panel: panelEl, statusbar: statusbarEl },
		render() {
			renderAll();
			menubar.render();
		},
		destroy() {
			window.removeEventListener('keydown', onKeyDown, true);
			for (const off of offs) off();
			for (const entry of mounted.values()) {
				try { if (typeof entry.dispose === 'function') entry.dispose(); } catch (err) { console.error(err); }
			}
			mounted.clear();
			root.replaceChildren();
		}
	};
}
