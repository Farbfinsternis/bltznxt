// Dienst "dialogs": Meldungen und Rückfragen.
//
//   const id = await dialogs.message({
//     title: 'Save changes?',
//     text: 'File a.bb has been modified.',
//     buttons: [{ id: 'yes', label: 'Yes', default: true }, { id: 'no', label: 'No' }, { id: 'cancel', label: 'Cancel' }],
//     cancelId: 'cancel'      // Rückgabe bei Escape / Klick daneben
//   });
//
// Texte werden als reiner Text gesetzt, nie als HTML. Solange ein Dialog offen
// ist, ruht der Kontextschlüssel `dialog.open` — die Shell gibt dann keine
// Tastenkürzel weiter.

import { h } from './dom.js';

/**
 * @param {{ root: HTMLElement, context: { set(key: string, value: any): void } }} deps
 */
export function createDialogs({ root, context }) {
	let openCount = 0;

	/**
	 * @param {{ title?: string, text?: string, buttons?: Array<{id: string, label: string, default?: boolean}>, cancelId?: string }} opts
	 * @returns {Promise<string>} die id des gewählten Knopfes
	 */
	function message({ title = '', text = '', buttons, cancelId } = {}) {
		const list = buttons && buttons.length ? buttons : [{ id: 'ok', label: 'OK', default: true }];
		const cancel = cancelId ?? list[list.length - 1].id;
		const previousFocus = document.activeElement;

		return new Promise((resolve) => {
			const finish = (id) => {
				document.removeEventListener('keydown', onKey, true);
				overlay.remove();
				if (--openCount === 0) context.set('dialog.open', false);
				if (previousFocus && typeof previousFocus.focus === 'function') previousFocus.focus();
				resolve(id);
			};

			const buttonEls = list.map((b) =>
				h('button', { class: 'dialog-button', type: 'button', 'data-id': b.id, text: b.label, onclick: () => finish(b.id) })
			);
			const defaultIndex = Math.max(0, list.findIndex((b) => b.default));

			const overlay = h('div', { class: 'dialog-overlay' },
				h('div', { class: 'dialog', role: 'dialog', 'aria-modal': 'true', 'aria-label': title },
					h('div', { class: 'dialog-title', text: title }),
					h('div', { class: 'dialog-text', text }),
					h('div', { class: 'dialog-buttons' }, ...buttonEls)
				)
			);

			function onKey(ev) {
				if (ev.key === 'Escape') {
					ev.preventDefault();
					ev.stopPropagation();
					finish(cancel);
				} else if (ev.key === 'Tab') {
					// Fokus bleibt im Dialog
					const i = buttonEls.indexOf(document.activeElement);
					const next = (i + (ev.shiftKey ? buttonEls.length - 1 : 1)) % buttonEls.length;
					ev.preventDefault();
					buttonEls[next].focus();
				} else if (ev.key === 'Enter' && !buttonEls.includes(document.activeElement)) {
					ev.preventDefault();
					finish(list[defaultIndex].id);
				}
			}

			document.addEventListener('keydown', onKey, true);
			if (openCount++ === 0) context.set('dialog.open', true);
			root.append(overlay);
			buttonEls[defaultIndex].focus();
		});
	}

	return { message };
}
