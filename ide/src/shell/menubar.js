// Menüleiste.
//
// Die Leiste zeigt die Titel der Menüs; das Aufklappmenü wird erst beim
// Öffnen aus dem Menümodell gebaut (core/menus.js), damit Sperren, Haken und
// Kürzel immer den jetzigen Stand zeigen. Die Leiste selbst wird neu
// gezeichnet, wenn sich Menübeiträge ändern.

import { h } from './dom.js';
import { buildMenus } from '../core/menus.js';

/**
 * @param {{ bar: HTMLElement, layer: HTMLElement, app: ReturnType<typeof import('../app.js').createApp> }} deps
 */
export function createMenubar({ bar, layer, app }) {
	let openId = null;
	let menus = [];

	const model = () =>
		buildMenus({
			contributions: app.contributions,
			commands: app.commands,
			keybindings: app.keybindings,
			context: app.context,
			i18n: app.i18n
		});

	function close() {
		openId = null;
		layer.replaceChildren();
		for (const b of bar.querySelectorAll('.menu-title')) b.classList.remove('open');
	}

	function renderItems(items, level) {
		const list = h('div', { class: 'menu-list', role: 'menu' });
		for (const item of items) {
			if (item.type === 'separator') {
				list.append(h('div', { class: 'menu-separator', role: 'separator' }));
				continue;
			}
			if (item.type === 'submenu') {
				const sub = h('div', { class: 'menu-item has-sub', role: 'menuitem' },
					h('span', { class: 'menu-check' }),
					h('span', { class: 'menu-label', text: item.title }),
					h('span', { class: 'menu-key', text: '▸' })
				);
				const holder = renderItems(item.items, level + 1);
				holder.classList.add('menu-sub');
				sub.append(holder);
				list.append(sub);
				continue;
			}
			const el = h('div', {
				class: 'menu-item' + (item.enabled ? '' : ' disabled'),
				role: item.checked ? 'menuitemcheckbox' : 'menuitem',
				'aria-checked': item.checked ? 'true' : null,
				'aria-disabled': item.enabled ? null : 'true',
				'data-command': item.command,
				onclick: (ev) => {
					ev.stopPropagation();
					if (!item.enabled) return;
					close();
					app.run(item.command, ...item.args);
				}
			},
				h('span', { class: 'menu-check', text: item.checked ? '✓' : '' }),
				h('span', { class: 'menu-label', text: item.title }),
				h('span', { class: 'menu-key', text: item.key || '' })
			);
			list.append(el);
		}
		return list;
	}

	function open(id, anchor) {
		const menu = model().find((m) => m.id === id);
		if (!menu) return close();
		layer.replaceChildren();
		for (const b of bar.querySelectorAll('.menu-title')) b.classList.toggle('open', b === anchor);
		const list = renderItems(menu.items, 0);
		list.classList.add('menu-dropdown');
		const rect = anchor.getBoundingClientRect();
		list.style.left = `${rect.left}px`;
		list.style.top = `${rect.bottom}px`;
		layer.append(list);
		openId = id;
	}

	function render() {
		menus = model();
		bar.replaceChildren(
			...menus.map((menu) => {
				const title = h('button', {
					class: 'menu-title',
					type: 'button',
					'data-menu': menu.id,
					text: menu.title,
					onclick: (ev) => {
						ev.stopPropagation();
						if (openId === menu.id) close();
						else open(menu.id, title);
					},
					// Ist ein Menü offen, wechselt schon das Darüberfahren
					onmouseenter: () => {
						if (openId !== null && openId !== menu.id) open(menu.id, title);
					}
				});
				return title;
			})
		);
		if (openId !== null) close();
	}

	// Klick daneben und Escape schließen
	document.addEventListener('click', () => {
		if (openId !== null) close();
	});
	document.addEventListener('keydown', (ev) => {
		if (ev.key === 'Escape' && openId !== null) {
			ev.preventDefault();
			close();
		}
	}, true);
	window.addEventListener('blur', close);

	return { render, close, get isOpen() { return openId !== null; } };
}
