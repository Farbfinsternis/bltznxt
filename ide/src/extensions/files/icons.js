// Kleine Symbole für die Symbolleiste (16 x 16, einfarbig).
// Als Text, damit sie Daten der Erweiterung bleiben und mit der Textfarbe
// des Themes mitfärben (fill: currentColor, siehe shell.css).

const svg = (d) => `<svg viewBox="0 0 16 16" aria-hidden="true"><path d="${d}"/></svg>`;

export const icons = {
	// Blatt mit Ecke und Plus
	new: svg('M3 1h6l4 4v10H3V1zm5.5 1.5V6H12L8.5 2.5zM7 8h2v2h2v2H9v2H7v-2H5v-2h2V8z'),
	// aufgeklappter Ordner
	open: svg('M1 3h5l1.5 1.5H14V6H4.2L2 13H1V3zm3.6 4.5H15L12.6 13H2.5l2.1-5.5z'),
	// Diskette
	save: svg('M2 1h10l2 2v12H2V1zm2 1.5v4h7v-4H4zM4 9v5h8V9H4zm5.5-6.5h1v3h-1v-3z'),
	// Kreuz
	close: svg('M3.5 2.5 8 7l4.5-4.5 1 1L9 8l4.5 4.5-1 1L8 9l-4.5 4.5-1-1L7 8 2.5 3.5l1-1z')
};
