// Ereignisse: der lose Draht zwischen Erweiterungen.
//
// Eine Erweiterung, die "ein Dokument wurde gespeichert" wissen will, hört auf
// das Ereignis und importiert keine Nachbardatei. Ein Hörer, der wirft, darf
// die anderen nicht aussperren — der Fehler geht auf die Konsole.

/**
 * @returns {{
 *   on(name: string, fn: (payload: any) => void): () => void,
 *   once(name: string, fn: (payload: any) => void): () => void,
 *   emit(name: string, payload?: any): void
 * }}
 */
export function createEmitter() {
	/** @type {Map<string, Set<Function>>} */
	const handlers = new Map();

	function on(name, fn) {
		let set = handlers.get(name);
		if (!set) handlers.set(name, (set = new Set()));
		set.add(fn);
		return () => {
			set.delete(fn);
		};
	}

	function once(name, fn) {
		const off = on(name, (payload) => {
			off();
			fn(payload);
		});
		return off;
	}

	function emit(name, payload) {
		const set = handlers.get(name);
		if (!set) return;
		// Kopie: ein Hörer darf sich beim Aufruf abmelden.
		for (const fn of [...set]) {
			try {
				fn(payload);
			} catch (err) {
				console.error(`[events] Hörer für "${name}" hat einen Fehler geworfen:`, err);
			}
		}
	}

	return { on, once, emit };
}

/**
 * Sammelt Aufräumfunktionen und ruft sie in umgekehrter Reihenfolge auf.
 * Eine Erweiterung legt hier alles ab, was beim Abschalten weg muss.
 */
export function createDisposables() {
	/** @type {Array<() => void>} */
	const list = [];
	return {
		/** @param {(() => void) | { dispose(): void } | null | undefined} d */
		add(d) {
			if (!d) return d;
			list.push(typeof d === 'function' ? d : () => d.dispose());
			return d;
		},
		dispose() {
			while (list.length) {
				const fn = list.pop();
				try {
					fn();
				} catch (err) {
					console.error('[events] Aufräumen fehlgeschlagen:', err);
				}
			}
		},
		get size() {
			return list.length;
		}
	};
}
