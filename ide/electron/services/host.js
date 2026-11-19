// Dienst "host": Auskünfte über den Start der Anwendung.
//
//   launchFiles()   Dateien, die auf der Kommandozeile standen — wie
//                   `blitzide datei.bb` im Original. Der Renderer öffnet sie.
//   info()          { platform } — 'win32', 'linux', ...

'use strict';

const fs = require('fs');
const path = require('path');

let files = [];

/**
 * Aus `process.argv` die Dateien holen. Ungepackt lautet die Zeile
 * `electron <app-pfad> datei.bb`, gepackt `bltznxt-ide.exe datei.bb`.
 * @param {string[]} argv
 * @param {boolean} isPackaged
 * @param {string} [cwd]
 */
function setLaunchArgs(argv, isPackaged, cwd = process.cwd()) {
	const rest = argv.slice(isPackaged ? 1 : 2);
	files = rest
		.filter((a) => a && !a.startsWith('-'))
		.map((a) => path.resolve(cwd, a.replace(/^"|"$/g, '')))
		.filter((p) => {
			try {
				return fs.statSync(p).isFile();
			} catch {
				return false;
			}
		});
}

module.exports = {
	api: {
		launchFiles: async () => [...files],
		// Das Betriebssystem, für das gebaut wird: unter Windows heißt ein
		// Programm "spiel.exe", sonst "spiel" (Dialog "Programm erstellen")
		info: async () => ({ platform: process.platform })
	},
	setLaunchArgs
};
