// Erweiterung "outline": die Listen funcs / types / labels der Original-IDE.
//
// Unter dem Editor drei Reiter; ein Klick auf einen Eintrag springt zur Zeile.
// Die Listen kommen aus dem Text des aktiven Dokuments, zeilenweise gelesen
// (parse.js) — bei jeder Änderung neu, kurz verzögert.

import { parseOutline } from './parse.js';

const TABS = ['funcs', 'types', 'labels'];
const REFRESH_DELAY_MS = 150;

export default {
	id: 'outline',
	dependsOn: ['editor'],

	messages: {
		en: {
			'outline.title': 'Outline',
			'outline.funcs': 'funcs',
			'outline.types': 'types',
			'outline.labels': 'labels'
		}
	},

	contributes: {
		views: [
			{
				id: 'outline',
				location: 'panel',
				title: 'outline.title',
				order: 10,
				when: 'document.active',
				mount: (el, ctx) => mountOutline(el, ctx)
			}
		]
	}
};

function mountOutline(el, ctx) {
	const { documents, i18n, state } = ctx;
	el.classList.add('outline');

	const tabsEl = document.createElement('div');
	tabsEl.className = 'outline-tabs';
	const listEl = document.createElement('div');
	listEl.className = 'outline-list';
	listEl.setAttribute('role', 'listbox');
	el.append(tabsEl, listEl);

	let current = TABS.includes(state.get('outline.tab')) ? state.get('outline.tab') : 'funcs';
	let outline = { funcs: [], types: [], labels: [] };
	let timer = null;

	function render() {
		tabsEl.replaceChildren(
			...TABS.map((name) => {
				const b = document.createElement('button');
				b.type = 'button';
				b.className = 'outline-tab' + (name === current ? ' active' : '');
				b.dataset.tab = name;
				b.textContent = `${i18n.t('outline.' + name)} (${outline[name].length})`;
				b.addEventListener('click', () => {
					current = name;
					state.set('outline.tab', name);
					render();
				});
				return b;
			})
		);
		listEl.replaceChildren(
			...outline[current].map((entry) => {
				const row = document.createElement('div');
				row.className = 'outline-item';
				row.setAttribute('role', 'option');
				row.dataset.line = String(entry.line);
				row.textContent = entry.name;
				row.title = `${entry.name} — ${entry.line}`;
				row.addEventListener('click', () => {
					const editor = ctx.services.tryGet('editor');
					if (editor) editor.reveal(entry.line, 1);
				});
				return row;
			})
		);
	}

	function refresh() {
		timer = null;
		const doc = documents.active;
		outline = doc ? parseOutline(doc.text) : { funcs: [], types: [], labels: [] };
		render();
	}

	function schedule() {
		if (timer !== null) clearTimeout(timer);
		timer = setTimeout(refresh, REFRESH_DELAY_MS);
	}

	const subs = [
		documents.on('activated', refresh), // Wechsel des Tabs: sofort
		documents.on('changed', (doc) => {
			if (documents.active && doc.id === documents.active.id) schedule();
		}),
		documents.on('closed', schedule),
		i18n.onDidChange(render)
	];
	refresh();

	return () => {
		if (timer !== null) clearTimeout(timer);
		for (const off of subs) off();
	};
}
