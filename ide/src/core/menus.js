// Menümodell.
//
// Baut aus den Beiträgen ein Menü als reine Daten — ohne DOM. Die Shell zeigt
// dieses Modell an; ein Test prüft es, ohne ein Fenster zu öffnen; ein anderes
// Backend (natives Menü) könnte es ebenso verwenden.
//
// Beiträge:
//   menubar: { id: 'file', title: 'menu.file', order: 10 }
//   menus:   { menu: 'file', command: 'file.save', group: '2_save', order: 10,
//              when?: '...', checkedWhen?: '...', title?: '...', args?: [...] }
//            { menu: 'file', submenu: 'file.recent', title: 'menu.recent', group: ... }
//            (die Einträge des Untermenüs tragen `menu: 'file.recent'`)
//
// Anordnung: nach `group` (als Text verglichen, darum "1_new", "2_save"), dann
// `order`, dann Reihenfolge des Beitrags. Zwischen Gruppen steht ein Trenner.
// Ein Menü ohne sichtbaren Eintrag entfällt ganz.

/**
 * @typedef {{ type: 'item', command: string, title: string, key?: string, enabled: boolean, checked: boolean, args: any[] }} MenuItem
 * @typedef {{ type: 'separator' }} MenuSeparator
 * @typedef {{ type: 'submenu', id: string, title: string, items: Array<MenuItem | MenuSeparator | MenuSubmenu> }} MenuSubmenu
 * @typedef {{ id: string, title: string, items: Array<MenuItem | MenuSeparator | MenuSubmenu> }} Menu
 */

function compare(a, b) {
	const ga = a.entry.group ?? '';
	const gb = b.entry.group ?? '';
	if (ga !== gb) return ga < gb ? -1 : 1;
	const oa = a.entry.order ?? 0;
	const ob = b.entry.order ?? 0;
	if (oa !== ob) return oa - ob;
	return a.index - b.index;
}

/**
 * @param {{
 *   contributions: ReturnType<typeof import('./contributions.js').createContributions>,
 *   commands: ReturnType<typeof import('./commands.js').createCommands>,
 *   keybindings: ReturnType<typeof import('./keybindings.js').createKeybindings>,
 *   context: ReturnType<typeof import('./context.js').createContext>,
 *   i18n: ReturnType<typeof import('./i18n.js').createI18n>
 * }} deps
 * @returns {Menu[]}
 */
export function buildMenus({ contributions, commands, keybindings, context, i18n }) {
	const entries = contributions.get('menus').map((entry, index) => ({ entry, index }));

	function itemsOf(menuId, seen) {
		if (seen.has(menuId)) return []; // Untermenü, das sich selbst enthält
		const nextSeen = new Set(seen).add(menuId);

		const visible = entries
			.filter(({ entry }) => entry.menu === menuId && context.evaluate(entry.when))
			.sort(compare);

		/** @type {Array<MenuItem | MenuSeparator | MenuSubmenu>} */
		const out = [];
		let lastGroup = null;
		for (const { entry } of visible) {
			let node;
			if (entry.submenu) {
				const items = itemsOf(entry.submenu, nextSeen);
				if (!items.length) continue;
				node = { type: 'submenu', id: entry.submenu, title: i18n.t(entry.title ?? entry.submenu), items };
			} else if (entry.command) {
				const def = commands.get(entry.command);
				if (!def) continue; // Befehl (noch) nicht da: Eintrag bleibt unsichtbar
				node = {
					type: 'item',
					command: entry.command,
					title: i18n.t(entry.title ?? def.title ?? entry.command),
					key: keybindings.keyFor(entry.command),
					enabled: commands.isEnabled(entry.command),
					checked: entry.checkedWhen ? context.evaluate(entry.checkedWhen) : false,
					args: entry.args || []
				};
			} else {
				continue;
			}
			const group = entry.group ?? '';
			if (out.length && group !== lastGroup) out.push({ type: 'separator' });
			lastGroup = group;
			out.push(node);
		}
		return out;
	}

	return contributions
		.get('menubar')
		.map((entry, index) => ({ entry, index }))
		.sort((a, b) => (a.entry.order ?? 0) - (b.entry.order ?? 0) || a.index - b.index)
		.filter(({ entry }) => context.evaluate(entry.when))
		.map(({ entry }) => ({
			id: entry.id,
			title: i18n.t(entry.title ?? entry.id),
			items: itemsOf(entry.id, new Set())
		}))
		.filter((menu) => menu.items.length > 0);
}
