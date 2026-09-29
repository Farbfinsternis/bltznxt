// Symbole der Erweiterung "build-run" (16 x 16, einfarbig).

const svg = (d) => `<svg viewBox="0 0 16 16" aria-hidden="true"><path d="${d}"/></svg>`;

export const icons = {
	// Dreieck: Programm starten
	run: svg('M4 2.5v11l9-5.5-9-5.5z'),
	// Quadrat: Programm beenden
	stop: svg('M3.5 3.5h9v9h-9z')
};
