// Dienst "dialogs": Meldungen, Rückfragen und eine Texteingabe.
//
//   const id = await dialogs.message({
//     title: 'Save changes?',
//     text: 'File a.bb has been modified.',
//     buttons: [{ id: 'yes', label: 'Yes', default: true }, { id: 'no', label: 'No' }, { id: 'cancel', label: 'Cancel' }],
//     cancelId: 'cancel'      // Rückgabe bei Escape / Klick daneben
//   });
//
//   const text = await dialogs.prompt({ title: 'Program Command Line', label: 'Program command line:', value: '' });
//   // -> der Text, oder null bei Abbruch
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
	 * Der gemeinsame Dialog: Titel, Text, optional ein Eingabefeld, Knöpfe.
	 * @returns {Promise<{ id: string, value: string }>}
	 */
	function open({ title = '', text = '', buttons, cancelId, input } = {}) {
		const list = buttons && buttons.length ? buttons : [{ id: 'ok', label: 'OK', default: true }];
		const cancel = cancelId ?? list[list.length - 1].id;
		const previousFocus = document.activeElement;

		return new Promise((resolve) => {
			const finish = (id) => {
				document.removeEventListener('keydown', onKey, true);
				const value = inputEl ? inputEl.value : '';
				overlay.remove();
				if (--openCount === 0) context.set('dialog.open', false);
				if (previousFocus && typeof previousFocus.focus === 'function') previousFocus.focus();
				resolve({ id, value });
			};

			const buttonEls = list.map((b) =>
				h('button', { class: 'dialog-button', type: 'button', 'data-id': b.id, text: b.label, onclick: () => finish(b.id) })
			);
			const defaultIndex = Math.max(0, list.findIndex((b) => b.default));
			const inputEl = input
				? h('input', { class: 'dialog-input', type: 'text', value: input.value ?? '', spellcheck: 'false', 'aria-label': input.label || title })
				: null;
			if (inputEl) inputEl.value = input.value ?? '';

			const overlay = h('div', { class: 'dialog-overlay' },
				h('div', { class: 'dialog', role: 'dialog', 'aria-modal': 'true', 'aria-label': title },
					h('div', { class: 'dialog-title', text: title }),
					text && h('div', { class: 'dialog-text', text }),
					input && input.label && h('label', { class: 'dialog-label', text: input.label }),
					inputEl,
					h('div', { class: 'dialog-buttons' }, ...buttonEls)
				)
			);

			// Fokusreihenfolge: Eingabefeld, dann die Knöpfe
			const focusable = inputEl ? [inputEl, ...buttonEls] : buttonEls;

			function onKey(ev) {
				if (ev.key === 'Escape') {
					ev.preventDefault();
					ev.stopPropagation();
					finish(cancel);
				} else if (ev.key === 'Tab') {
					const i = focusable.indexOf(document.activeElement);
					const next = (i + (ev.shiftKey ? focusable.length - 1 : 1)) % focusable.length;
					ev.preventDefault();
					focusable[next].focus();
				} else if (ev.key === 'Enter' && !buttonEls.includes(document.activeElement)) {
					ev.preventDefault();
					finish(list[defaultIndex].id);
				}
			}

			document.addEventListener('keydown', onKey, true);
			if (openCount++ === 0) context.set('dialog.open', true);
			root.append(overlay);
			if (inputEl) {
				inputEl.focus();
				inputEl.select();
			} else {
				buttonEls[defaultIndex].focus();
			}
		});
	}

	/**
	 * @param {{ title?: string, text?: string, buttons?: Array<{id: string, label: string, default?: boolean}>, cancelId?: string }} opts
	 * @returns {Promise<string>} die id des gewählten Knopfes
	 */
	async function message(opts) {
		return (await open(opts)).id;
	}

	/**
	 * @param {{ title?: string, text?: string, label?: string, value?: string, okLabel?: string, cancelLabel?: string }} [opts]
	 * @returns {Promise<string | null>} der eingegebene Text, `null` bei Abbruch
	 */
	async function prompt({ title = '', text = '', label = '', value = '', okLabel = 'OK', cancelLabel = 'Cancel' } = {}) {
		const result = await open({
			title,
			text,
			input: { label, value },
			buttons: [{ id: 'ok', label: okLabel, default: true }, { id: 'cancel', label: cancelLabel }],
			cancelId: 'cancel'
		});
		return result.id === 'ok' ? result.value : null;
	}

	return { message, prompt };
}
