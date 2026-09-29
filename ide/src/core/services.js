// Dienste nach Namen.
//
// Eine Erweiterung holt sich `services.get('documents')` statt eine
// Nachbardatei zu importieren. Wer den Dienst liefert, ist ihr egal — eine
// andere Erweiterung darf ihn ersetzen, solange sie dieselben Methoden bietet.
// `waitFor` löst die Reihenfolgefrage: eine Erweiterung, die einen Dienst
// braucht, der später kommen kann, wartet darauf, statt die Aktivierung
// festzulegen.

import { createEmitter } from './events.js';

export function createServices() {
	/** @type {Map<string, any>} */
	const provided = new Map();
	const events = createEmitter();

	/**
	 * @param {string} name
	 * @param {any} impl
	 * @returns {() => void}
	 */
	function provide(name, impl) {
		if (provided.has(name)) throw new Error(`Dienst "${name}" ist schon vergeben`);
		provided.set(name, impl);
		events.emit(`provided:${name}`, impl);
		return () => {
			if (provided.get(name) === impl) provided.delete(name);
		};
	}

	const has = (name) => provided.has(name);
	const tryGet = (name) => provided.get(name);

	function get(name) {
		if (!provided.has(name)) throw new Error(`Dienst "${name}" ist nicht vorhanden`);
		return provided.get(name);
	}

	/** Liefert den Dienst, sobald er da ist (sofort, wenn schon vorhanden). */
	function waitFor(name) {
		if (provided.has(name)) return Promise.resolve(provided.get(name));
		return new Promise((resolve) => {
			events.once(`provided:${name}`, resolve);
		});
	}

	return { provide, has, get, tryGet, waitFor };
}
