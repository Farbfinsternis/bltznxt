// Symbole der Bearbeiten-Leiste (16 x 16, einfarbig).

const svg = (d) => `<svg viewBox="0 0 16 16" aria-hidden="true"><path d="${d}"/></svg>`;

export const icons = {
	// Schere
	cut: svg('M5 1l2.6 6.3L6.4 9A2.5 2.5 0 1 0 8 10.6L9.6 8.5 11.5 13H13L9.4 4.4 11 1H9.5L8 4.6 6.5 1H5zM3.5 11.5a1 1 0 1 1 2 0 1 1 0 0 1-2 0z'),
	// zwei Blätter
	copy: svg('M5 1h7v2H7v8H5V1zm3 3h6v11H8V4zm1.5 1.5v8h3v-8h-3z'),
	// Klemmbrett
	paste: svg('M5.5 1.5h5V3h2v11.5h-9V3h2V1.5zM6.8 2.8v1.4h2.4V2.8H6.8zM5 4.5v8.5h6V4.5H5z'),
	// Lupe
	find: svg('M6.5 1a5.5 5.5 0 1 0 3.3 9.9l3.9 3.9 1.1-1.1-3.9-3.9A5.5 5.5 0 0 0 6.5 1zm0 1.5a4 4 0 1 1 0 8 4 4 0 0 1 0-8z')
};
